#include "Line.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
	float4x4 WVP;
}

VS_LINE_OUTPUT main(VS_LINE_INPUT input)
{
	VS_LINE_OUTPUT output;

	output.Position = mul(input.Position, World);
	output.Position = mul(output.Position, ViewProjection);
	output.Color = input.Color;

	return output;
}