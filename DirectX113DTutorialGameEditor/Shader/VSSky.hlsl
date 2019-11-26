#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
	float4x4 WVP;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.WorldPosition = mul(input.Position, World);
	output.Position = mul(output.WorldPosition, ViewProjection).xyww;

	output.Color = input.Color;
	output.TexCoord = input.TexCoord; // static TexCoord
	//output.TexCoord = normalize(input.Position.xyz); // dynamic TexCoord

	output.WorldNormal = normalize(input.Normal);
	output.WorldTangent = normalize(mul(input.Tangent, World));
	output.WorldBitangent = CalculateBitangent(output.WorldNormal, output.WorldTangent);

	output.bUseVertexColor = 0;

	return output;
}