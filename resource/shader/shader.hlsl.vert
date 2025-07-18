cbuffer Matrices : register(b0, space1) { float4x4 projection; };

struct Vertex_input
{
  float3 position : POSITION;
};

struct Vertex_output
{
  float4 position : SV_Position;
};

Vertex_output main(Vertex_input input)
{
  Vertex_output output;
  float4 world_position = float4(input.position, 1.0f);
  output.position = mul(projection, world_position);
  return output;
}
