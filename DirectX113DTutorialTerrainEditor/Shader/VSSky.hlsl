#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 WVP;
	float4x4 World;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.Position = mul(input.Position, WVP).xyww;
	output.WorldPosition = mul(input.Position, World);

	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = input.Normal;
	output.WorldTangent = normalize(mul(input.Tangent, World));
	output.WorldBitangent = float4(normalize(cross(output.WorldNormal.xyz, output.WorldTangent.xyz)), 0);

	output.bUseVertexColor = 0;

	return output;
}