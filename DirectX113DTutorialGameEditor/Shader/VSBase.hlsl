#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.WorldPosition = mul(input.Position, World);
	output.Position = mul(output.WorldPosition, ViewProjection);

	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = normalize(mul(input.Normal, World));
	output.WorldTangent = normalize(mul(input.Tangent, World));
	output.WorldBitangent = CalculateBitangent(output.WorldNormal, output.WorldTangent);

	output.bUseVertexColor = 0;

	return output;
}