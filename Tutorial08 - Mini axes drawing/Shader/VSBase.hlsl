#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 WVP;
	float4x4 World;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.Position = mul(input.Position, WVP);
	output.Color = input.Color;
	output.UV = input.UV;

	output.Normal = mul(input.Normal, World);
	output.WVPNormal = normalize(mul(input.Normal, WVP));

	return output;
}