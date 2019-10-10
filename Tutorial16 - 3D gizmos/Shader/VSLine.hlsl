#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 WVP;
	float4x4 World;
}

VS_LINE_OUTPUT main(VS_LINE_INPUT input)
{
	VS_LINE_OUTPUT output;

	output.Position = mul(input.Position, WVP);
	output.Color = input.Color;

	return output;
}