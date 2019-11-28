#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
	float4x4 WVP;
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.WorldPosition = mul(Input.Position, World);
	Output.Position = mul(Output.WorldPosition, ViewProjection).xyww;

	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord; // static TexCoord
	//Output.TexCoord = normalize(Input.Position.xyz); // dynamic TexCoord

	Output.WorldNormal = normalize(Input.Normal);
	Output.WorldTangent = normalize(mul(Input.Tangent, World));
	Output.WorldBitangent = CalculateBitangent(Output.WorldNormal, Output.WorldTangent);

	Output.bUseVertexColor = 0;
	Output.InstanceID = Input.InstanceID;

	return Output;
}