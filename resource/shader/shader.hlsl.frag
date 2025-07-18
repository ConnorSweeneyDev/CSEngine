struct Fragment_input
{
  float3 color : COLOR;
};

float4 main(Fragment_input input) : SV_Target
{
  float4 color = float4(input.color, 1.0f);
  if (color.a == 0.0f) discard;
  return color;
}
