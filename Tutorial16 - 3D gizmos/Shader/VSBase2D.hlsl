#include "Header2D.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 Projection;
	float4x4 World;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.Position = mul(input.Position, World);
	output.Position = mul(output.Position, Projection);
	output.Position.w = 1.0f;

	output.Color = input.Color;
	output.UV = input.UV;

	return output;
}