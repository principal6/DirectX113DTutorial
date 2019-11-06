#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4x4 InstanceWorld = float4x4(input.InstanceWorld0, input.InstanceWorld1, input.InstanceWorld2, input.InstanceWorld3);

	output.WorldPosition = mul(input.Position, InstanceWorld);
	output.Position = mul(output.WorldPosition, ViewProjection);

	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = normalize(mul(input.Normal, World));
	output.WorldTangent = normalize(mul(input.Tangent, World));
	output.WorldBitangent = CalculateBitangent(output.WorldNormal, output.WorldTangent);

	output.bUseVertexColor = 0;

	return output;
}