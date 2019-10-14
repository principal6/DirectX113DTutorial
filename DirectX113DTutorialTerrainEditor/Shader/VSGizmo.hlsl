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
	output.Position.z *= 0.01f;

	output.WorldPosition = mul(input.Position, World);
	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = normalize(mul(input.Normal, World));
	output.WVPNormal = normalize(mul(input.Normal, WVP));

	return output;
}