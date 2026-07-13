struct Input
{
  float4 color : TEXCOORD0;    // xyz = tint colour, w = tint strength
  float2 texture : TEXCOORD1;  // texture uv for this fragment
  float4 world : TEXCOORD2;    // xyz = world-space position (for lighting / pixel snap), w = occluder index
  float4 material : TEXCOORD3; // x = lit, y = shadowed, z = brightness, w = transparency
};
struct Light
{
  float4 position;   // xyz = world position, w = range (distance)
  float4 brightness; // xyz = colour-weighted brightness, w = shadow factor (0 = this light casts no shadows)
  float4 direction;  // xyz = aim direction, w = global (1 = directional/global, 0 = spot)
  float4 cone;       // x = cos(outer angle), y = cos(inner angle), z = penetration, w = shadow softness
};
struct Occluder
{
  float4 rectangle; // world-space xy bounds: minx, miny, maxx, maxy
  float4 frame;     // layer-space uv of the current frame: left, bottom, right, top
  float4 surface;   // x = z plane, y = array layer, z = transparency, w = rotated (odd quarter turn)
  float4 shadow;    // x = penetration, y = cast (0 = only absorbs its own light), z = darkness, w = softness
};

Texture2D<float4> texture_buffer : register(t0, space2);
SamplerState texture_sampler : register(s0, space2);
Texture2DArray<float4> occluder_buffers : register(t1, space2);
SamplerState occluder_sampler : register(s1, space2);
StructuredBuffer<Light> lights : register(t2, space2);
StructuredBuffer<Occluder> occluders : register(t3, space2);
cbuffer light_data : register(b0, space3)
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
  float2 texel = layer_texel();
  float2 inset = 0.5f * texel;
  if (uv.x < lo.x - inset.x || uv.x > hi.x + inset.x || uv.y < lo.y - inset.y || uv.y > hi.y + inset.y) return 0.0f;
  float2 snapped = clamp((floor(uv / texel) + 0.5f) * texel, lo + inset, hi - inset);
  return occluder_buffers.SampleLevel(occluder_sampler, float3(snapped, occluder.surface.y), 0).a;
}
float occluder_soft(Occluder occluder, float2 uv)
{
  float2 lo = min(occluder.frame.xy, occluder.frame.zw);
  float2 hi = max(occluder.frame.xy, occluder.frame.zw);
  if (uv.x < lo.x || uv.x > hi.x || uv.y < lo.y || uv.y > hi.y) return 0.0f;
  float2 inset = 0.5f * layer_texel();
  return occluder_buffers
    .SampleLevel(occluder_sampler, float3(clamp(uv, lo + inset, hi - inset), occluder.surface.y), 0)
    .a;
}
float2 occluder_uv(Occluder occluder, float2 hit)
{
  float u = (hit.x - occluder.rectangle.x) / (occluder.rectangle.z - occluder.rectangle.x);
  float v = (hit.y - occluder.rectangle.y) / (occluder.rectangle.w - occluder.rectangle.y);
  if (occluder.surface.w > 0.5f)
  {
    float temporary = u;
    u = v;
    v = temporary;
  }
  return lerp(occluder.frame.xy, occluder.frame.zw, float2(u, v));
}
float transmittance(float3 pixel, float3 towards, int count, float shadow, float softness)
{
  float transmission = 1.0f;
  float denominator = towards.z - pixel.z;
  if (abs(denominator) < 1e-4f) return transmission;
  for (int index = 0; index < count; ++index)
  {
    Occluder occluder = occluders[index];
    if (occluder.shadow.y < 0.5f || occluder.shadow.z <= 0.0f || occluder.surface.z <= 0.0f) continue;
    if (abs(occluder.surface.x - pixel.z) < 1e-3f) continue;
    float t = (occluder.surface.x - pixel.z) / denominator;
    if (t <= 0.0f || t >= 1.0f - 1e-4f) continue;
    float2 hit = lerp(pixel.xy, towards.xy, t);
    float world_softness = saturate(occluder.shadow.w * softness);
    float world_blur = world_softness * SHADOW_RADIUS;
    if (hit.x < occluder.rectangle.x - world_blur || hit.x > occluder.rectangle.z + world_blur ||
        hit.y < occluder.rectangle.y - world_blur || hit.y > occluder.rectangle.w + world_blur)
      continue;
    float2 uv = occluder_uv(occluder, hit);
    float alpha;
    if (world_blur <= 1e-3f)
      alpha = occluder_sharp(occluder, uv);
    else
    {
      float2 world_size =
        float2(occluder.rectangle.z - occluder.rectangle.x, occluder.rectangle.w - occluder.rectangle.y);
      if (occluder.surface.w > 0.5f) world_size = world_size.yx;
      float2 uv_per_world = float2((occluder.frame.z - occluder.frame.x) / world_size.x,
                                   (occluder.frame.w - occluder.frame.y) / world_size.y);
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
    transmission *= 1.0f - saturate(alpha * occluder.surface.z * occluder.shadow.z * shadow);
  }
  return transmission;
}
float penetration(float3 pixel, float3 source, int count, float strength, float softness, float self, bool beyond)
{
  float extra = 0.0f;
  float2 delta = pixel.xy - source.xy;
  float span = length(delta);
  if (span <= 1e-4f) return extra;
  if (beyond)
  {
    bool amplifying = false;
    for (int index = 0; index < count; ++index)
      if (strength * occluders[index].shadow.x > 1.0f)
      {
        amplifying = true;
        break;
      }
    if (!amplifying) return extra;
  }
  float2 normal = float2(-delta.y, delta.x) / span;
  for (int index = 0; index < count; ++index)
  {
    Occluder occluder = occluders[index];
    if (occluder.shadow.y < 0.5f && abs((float)index - self) > 0.5f) continue;
    if (abs(occluder.surface.x - pixel.z) > 1e-3f) continue;
    float combined = strength * occluder.shadow.x;
    if (abs(combined - 1.0f) < 1e-3f || occluder.surface.z <= 0.0f) continue;
    float world_blur = saturate(occluder.shadow.w * softness) * SHADOW_RADIUS;
    float enter = 0.0f;
    float exit = 1.0f;
    if (abs(delta.x) < 1e-6f)
    {
      if (source.x < occluder.rectangle.x - world_blur || source.x > occluder.rectangle.z + world_blur) continue;
    }
    else
    {
      float first = (occluder.rectangle.x - world_blur - source.x) / delta.x;
      float second = (occluder.rectangle.z + world_blur - source.x) / delta.x;
      enter = max(enter, min(first, second));
      exit = min(exit, max(first, second));
    }
    if (abs(delta.y) < 1e-6f)
    {
      if (source.y < occluder.rectangle.y - world_blur || source.y > occluder.rectangle.w + world_blur) continue;
    }
    else
    {
      float first = (occluder.rectangle.y - world_blur - source.y) / delta.y;
      float second = (occluder.rectangle.w + world_blur - source.y) / delta.y;
      enter = max(enter, min(first, second));
      exit = min(exit, max(first, second));
    }
    if (exit <= enter) continue;
    float crossing = (exit - enter) * span;
    int taps = (int)clamp(crossing, 4.0f, 32.0f);
    int blurs = world_blur <= 1e-3f ? 1 : (int)clamp(world_blur * 2.0f, 2.0f, 8.0f);
    float sum = 0.0f;
    for (int k = 0; k < taps; ++k)
    {
      float t = lerp(enter, exit, (k + 0.5f) / taps);
      float2 hit = source.xy + delta * t;
      if (blurs == 1)
        sum += occluder_sharp(occluder, occluder_uv(occluder, hit));
      else
      {
        float total = 0.0f;
        for (int j = 0; j < blurs; ++j)
        {
          float2 shifted = hit + normal * (((j + 0.5f) / blurs) * 2.0f - 1.0f) * world_blur;
          total += occluder_soft(occluder, occluder_uv(occluder, shifted));
        }
        sum += total / blurs;
      }
    }
    float inside = crossing * (sum / taps) * occluder.surface.z;
    extra += inside * (1.0f / max(combined, 1e-4f) - 1.0f);
  }
  return extra;
}

float4 main(Input input, bool front : SV_IsFrontFace) : SV_Target0
{
  float4 texture_color = texture_buffer.Sample(texture_sampler, input.texture);
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
        bool beyond = distance >= range;
        float reach = distance;
        if (occluder_count > 0 && abs(light.position.z - pixel.z) < 1e-3f)
          reach = max(distance + penetration(pixel, light.position.xyz, occluder_count, light.cone.z, light.cone.w,
                                             input.world.w, beyond),
                      0.0f);
        else if (beyond)
          continue;
        attenuation = saturate(1.0f - reach / range);
        attenuation *= attenuation;
        float3 to_pixel = distance > 1e-3f ? offset / distance : light.direction.xyz;
        float alignment = dot(to_pixel, light.direction.xyz);
        float cone = saturate((alignment - light.cone.x) / max(light.cone.y - light.cone.x, 1e-4f));
        attenuation *= cone * cone * (3.0f - 2.0f * cone);
        towards = light.position.xyz;
      }
      float facing = (front ? 1.0f : -1.0f) * (towards.z - pixel.z);
      if (facing < -1e-4f) attenuation = 0.0f;
      float shadow = light.brightness.w;
      if (shadow > 0.0f && occluder_count > 0 && attenuation > 0.0f)
        attenuation *= transmittance(pixel, towards, occluder_count, shadow, light.cone.w);
      illumination += light.brightness.rgb * attenuation;
    }
    illumination *= input.material.z;
  }

  return float4(saturate(surface * illumination), texture_color.a * input.material.w);
}
