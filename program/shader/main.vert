struct Input
{
  float3 position : TEXCOORD0;
  float4 color : TEXCOORD1;
  float2 texcoord : TEXCOORD2;
};

struct Output
{
  float4 position : SV_Position;
  float4 color : TEXCOORD0;
  float2 texcoord : TEXCOORD1;
};

cbuffer Matrices : register(b0, space1)
{
  float4x4 projection_matrix;
  float4x4 view_matrix;
  float4x4 model_matrix;
};

Output main(Input input)
{
  Output output = {mul(projection_matrix, mul(view_matrix, mul(model_matrix, float4(input.position, 1.0f)))),
                   input.color, input.texcoord};
  return output;
}
