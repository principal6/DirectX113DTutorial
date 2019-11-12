#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.WorldPosition = mul(Input.Position, World);
	Output.Position = mul(Output.WorldPosition, ViewProjection);

	Output.Color = Input.Color;
	Output.UV = Input.UV;

	//Output.WorldNormal = normalize(mul(Input.Normal, World));
	//Output.WorldTangent = normalize(mul(Input.Tangent, World));
	//Output.WorldBitangent = CalculateBitangent(Output.WorldNormal, Output.WorldTangent);

	float4 ResultNormal = normalize(mul(Input.Normal, World));
	float4 ResultBitangent = normalize(float4(cross(ResultNormal.xyz, Input.Tangent.xyz), 0));
	float4 ResultTangent = normalize(float4(cross(ResultBitangent.xyz, ResultNormal.xyz), 0));
	Output.WorldNormal = ResultNormal;
	Output.WorldTangent = normalize(mul(ResultTangent, World));
	Output.WorldBitangent = normalize(mul(ResultBitangent, World));

	Output.bUseVertexColor = 0;

	return Output;
}