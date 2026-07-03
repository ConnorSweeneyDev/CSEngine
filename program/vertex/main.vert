struct Input
{
  float2 corner : TEXCOORD0;         // quad corner position, -1..1
  float2 uv_corner : TEXCOORD1;      // quad corner uv, 0..1
  float4 model_column_0 : TEXCOORD2; // model matrix column 0 (per instance)
  float4 model_column_1 : TEXCOORD3; // model matrix column 1
  float4 model_column_2 : TEXCOORD4; // model matrix column 2
  float4 model_column_3 : TEXCOORD5; // model matrix column 3
  float4 color : TEXCOORD6;          // xyz = tint colour, w = tint strength
  float4 uv_rect : TEXCOORD7;        // xy = frame uv min (left, bottom), zw = max (right, top)
  float4 material : TEXCOORD8;       // x = lit, y = shadowed, z = brightness, w = transparency
  float depth : TEXCOORD9;           // clip-space depth bias (NDC), nearer = larger; coplanar ordering only
};
struct Output
{
  float4 position : SV_Position; // clip-space position
  float4 color : TEXCOORD0;      // xyz = tint colour, w = tint strength
  float2 texcoord : TEXCOORD1;   // texture uv for this fragment
  float3 world : TEXCOORD2;      // world-space position (for lighting / pixel snap)
  float4 material : TEXCOORD3;   // x = lit, y = shadowed, z = brightness, w = transparency
};

cbuffer Matrices : register(b0, space1)
{
  float4x4 projection_matrix;
  float4x4 view_matrix;
};

Output main(Input input)
{
  float4x4 model_matrix = {input.model_column_0, input.model_column_1, input.model_column_2, input.model_column_3};
  float4 world_position = mul(float4(input.corner, 0.0f, 1.0f), model_matrix);
  float4 clip = mul(projection_matrix, mul(view_matrix, world_position));
  clip.z -= input.depth * clip.w;
  Output output = {clip, input.color, lerp(input.uv_rect.xy, input.uv_rect.zw, input.uv_corner), world_position.xyz,
                   input.material};
  return output;
}
