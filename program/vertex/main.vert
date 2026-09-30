struct Input
{
  float4 corner : TEXCOORD0;         // xy = quad corner position (-1..1), zw = quad corner uv (0..1)
  float4 model_column_0 : TEXCOORD1; // model matrix column 0 (per instance)
  float4 model_column_1 : TEXCOORD2; // model matrix column 1
  float4 model_column_2 : TEXCOORD3; // model matrix column 2
  float4 model_column_3 : TEXCOORD4; // model matrix column 3
  float4 color : TEXCOORD5;          // xyz = tint colour, w = tint strength
  float4 frame : TEXCOORD6;          // xy = frame uv min (left, bottom), zw = max (right, top)
  float4 material : TEXCOORD7;       // x = lit, y = depth bias, z = brightness, w = transparency
  uint4 illumination : TEXCOORD8;    // xy = illumination self, zw = illumination target
  uint4 occlusion : TEXCOORD9;       // xy = occlusion self, zw = occlusion show
};
struct Output
{
  float4 position : SV_Position;                  // clip-space position
  float4 color : TEXCOORD0;                       // xyz = tint colour, w = tint strength
  float2 texcoord : TEXCOORD1;                    // texture uv for this fragment
  float3 world : TEXCOORD2;                       // world-space position (for lighting / pixel snap)
  nointerpolation float4 material : TEXCOORD3;    // x = lit, y = depth bias, z = brightness, w = transparency
  nointerpolation uint4 illumination : TEXCOORD4; // xy = illumination self, zw = illumination target
  nointerpolation uint4 occlusion : TEXCOORD5;    // xy = occlusion self, zw = occlusion show
};

cbuffer matrices : register(b0, space1)
{
  float4x4 projection_matrix;
  float4x4 view_matrix;
};

Output main(Input input)
{
  float4x4 model_matrix = {input.model_column_0, input.model_column_1, input.model_column_2, input.model_column_3};
  float4 world_position = mul(float4(input.corner.xy, 0.0f, 1.0f), model_matrix);
  float4 clip = mul(projection_matrix, mul(view_matrix, world_position));
  clip.z -= input.material.y * clip.w;
  Output output = {clip,
                   input.color,
                   lerp(input.frame.xy, input.frame.zw, input.corner.zw),
                   world_position.xyz,
                   input.material,
                   input.illumination,
                   input.occlusion};
  return output;
}
