struct Input
{
  float4 color : TEXCOORD0;    // xyz = tint colour, w = tint strength
  float2 texture : TEXCOORD1;  // texture uv for this fragment
  float3 world : TEXCOORD2;    // world-space position (for lighting / pixel snap)
  float4 material : TEXCOORD3; // x = lit, y = shadowed, z = brightness, w = transparency
};
struct Light
{
  float4 position;   // xyz = world position, w = range (distance)
  float4 brightness; // xyz = colour-weighted brightness, w = shadow factor (0 = this light casts no shadows)
  float4 direction;  // xyz = aim direction, w = global (1 = directional/global, 0 = spot)
  float4 cone;       // x = cos(outer angle), y = cos(inner angle), z = shadow softness, w = 1 / (y - x)
};
struct Occluder
{
  float4 rect;  // world-space xy bounds: minx, miny, maxx, maxy
  float4 frame; // layer-space uv of the current frame: left, bottom, right, top
  float4 data;  // x = z plane, y = opacity, z = array layer, w = shadow softness
};

Texture2D<float4> Texture : register(t0, space2);
SamplerState TextureSampler : register(s0, space2);
Texture2DArray<float4> Occluders : register(t1, space2);
SamplerState OccluderSampler : register(s1, space2);
StructuredBuffer<Light> lights : register(t2, space2);
StructuredBuffer<Occluder> occluders : register(t3, space2);
cbuffer Lights : register(b0, space3)
{
  float4 meta; // x = active light count, y = active occluder count, z = occluder array width, w = height
};

static const float SHADOW_RADIUS = 8.0f;       // max penumbra radius (world units) at softness 1
static const float SHADOW_GOLDEN = 2.3999632f; // golden angle (radians)

float2 layer_texel() { return 1.0f / float2(max(meta.z, 1.0f), max(meta.w, 1.0f)); }
float occluder_sharp(Occluder occluder, float2 uv)
{
  float2 lo = min(occluder.frame.xy, occluder.frame.zw);
  float2 hi = max(occluder.frame.xy, occluder.frame.zw);
  if (uv.x < lo.x || uv.x > hi.x || uv.y < lo.y || uv.y > hi.y) return 0.0f;
  float2 texel = layer_texel();
  float2 snapped = (floor(uv / texel) + 0.5f) * texel;
  return Occluders.SampleLevel(OccluderSampler, float3(snapped, occluder.data.z), 0).a;
}
float occluder_soft(Occluder occluder, float2 uv)
{
  float2 lo = min(occluder.frame.xy, occluder.frame.zw);
  float2 hi = max(occluder.frame.xy, occluder.frame.zw);
  if (uv.x < lo.x || uv.x > hi.x || uv.y < lo.y || uv.y > hi.y) return 0.0f;
  float2 inset = 0.5f * layer_texel();
  return Occluders.SampleLevel(OccluderSampler, float3(clamp(uv, lo + inset, hi - inset), occluder.data.z), 0).a;
}
float transmittance(float3 surface, float3 towards, int count, float light_shadow, float light_softness)
{
  float transmission = 1.0f;
  float denominator = towards.z - surface.z;
  if (abs(denominator) < 1e-4f) return transmission;
  for (int index = 0; index < count; ++index)
  {
    Occluder occluder = occluders[index];
    if (abs(occluder.data.x - surface.z) < 1e-3f) continue;
    float t = (occluder.data.x - surface.z) / denominator;
    if (t <= 0.0f || t >= 1.0f - 1e-4f) continue;
    float2 hit = lerp(surface.xy, towards.xy, t);
    float softness = saturate(occluder.data.w * light_softness);
    float world_blur = softness * SHADOW_RADIUS;
    if (hit.x < occluder.rect.x - world_blur || hit.x > occluder.rect.z + world_blur ||
        hit.y < occluder.rect.y - world_blur || hit.y > occluder.rect.w + world_blur)
      continue;
    float u = (hit.x - occluder.rect.x) / (occluder.rect.z - occluder.rect.x);
    float v = (hit.y - occluder.rect.y) / (occluder.rect.w - occluder.rect.y);
    float2 uv = lerp(occluder.frame.xy, occluder.frame.zw, float2(u, v));
    float alpha;
    if (world_blur <= 1e-3f)
      alpha = occluder_sharp(occluder, uv);
    else
    {
      float2 uv_per_world = float2((occluder.frame.z - occluder.frame.x) / (occluder.rect.z - occluder.rect.x),
                                   (occluder.frame.w - occluder.frame.y) / (occluder.rect.w - occluder.rect.y));
      float2 radius = uv_per_world * world_blur;
      int taps = (int)clamp(world_blur * 4.0f, 16.0f, 64.0f);
      float sum = 0.0f;
      for (int k = 0; k < taps; ++k)
      {
        float fraction = (k + 0.5f) / taps;
        float2 offset = sqrt(fraction) * float2(cos(k * SHADOW_GOLDEN), sin(k * SHADOW_GOLDEN));
        sum += occluder_soft(occluder, uv + offset * radius);
      }
      alpha = sum / taps;
    }
    transmission *= 1.0f - saturate(alpha * occluder.data.y * light_shadow);
  }
  return transmission;
}

float4 main(Input input, bool front : SV_IsFrontFace) : SV_Target0
{
  float4 texture_color = Texture.Sample(TextureSampler, input.texture);
  if (texture_color.a == 0.0f) discard;
  float3 tint = (input.color.rgb - 0.5f) * 2.0f * input.color.a;
  float3 surface = texture_color.rgb + tint;
  float3 pixel = float3(floor(input.world.xy) + 0.5f, input.world.z);
  float3 illumination = float3(1.0f, 1.0f, 1.0f);

  if (input.material.x > 0.5f)
  {
    illumination = float3(0.0f, 0.0f, 0.0f);
    int count = (int)meta.x;
    int occluder_count = (input.material.y > 0.5f) ? (int)meta.y : 0;
    for (int index = 0; index < count; ++index)
    {
      Light light = lights[index];
      float attenuation;
      float3 towards;
      if (light.direction.w > 0.5f)
      {
        attenuation = 1.0f;
        towards = pixel - light.direction.xyz * 100000.0f;
      }
      else
      {
        float3 offset = pixel - light.position.xyz;
        float distance = length(offset);
        float range = max(light.position.w, 1e-4f);
        attenuation = saturate(1.0f - distance / range);
        attenuation *= attenuation;
        float3 to_pixel = distance > 0.0f ? offset / distance : light.direction.xyz;
        float alignment = dot(to_pixel, light.direction.xyz);
        float cone = saturate((alignment - light.cone.x) * light.cone.w);
        attenuation *= cone * cone * (3.0f - 2.0f * cone);
        towards = light.position.xyz;
      }
      float facing = (front ? 1.0f : -1.0f) * (towards.z - pixel.z);
      if (facing < -1e-4f) attenuation = 0.0f;
      float light_shadow = light.brightness.w;
      if (light_shadow > 0.0f && occluder_count > 0 && attenuation > 0.0f)
        attenuation *= transmittance(pixel, towards, occluder_count, light_shadow, light.cone.z);
      illumination += light.brightness.rgb * attenuation;
    }
    illumination *= input.material.z;
  }

  return float4(saturate(surface * illumination), texture_color.a * input.material.w);
}
