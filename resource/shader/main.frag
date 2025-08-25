struct Input
{
  float4 color : TEXCOORD0;
  float2 texture : TEXCOORD1;
};

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

float4 main(Input input) : SV_Target0
{
  float4 texture_color = Texture.Sample(Sampler, input.texture);
  if (texture_color.a == 0.0f) discard;
  return texture_color;
}
