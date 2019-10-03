#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 WVP;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.Position = mul(input.Position, WVP);
	output.Position.z = 0.0f;
	
	output.Color = input.Color;

	output.UV = float2(0, 0);
	output.Normal = output.WVPNormal = float4(0, 0, 0, 0);
	
	return output;
}