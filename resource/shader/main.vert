struct Input
{
  float3 position : TEXCOORD0;
  float4 color : TEXCOORD1;
};

struct Output
{
  float4 color : TEXCOORD0;
  float4 position : SV_Position;
};

cbuffer matrices : register(b0, space1)
{
  float4x4 projection_matrix;
  float4x4 view_matrix;
  float4x4 model_matrix;
};

Output main(Input input)
{
  Output output;
  output.color = input.color;
  output.position = mul(projection_matrix, mul(view_matrix, mul(model_matrix, float4(input.position, 1.0f))));
  return output;
}
