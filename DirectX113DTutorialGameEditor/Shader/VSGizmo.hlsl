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
	Output.Position = mul(Output.WorldPosition, ViewProjection);
	Output.Position.z *= 0.01f;

	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	Output.WorldNormal = normalize(mul(Input.Normal, World));
	Output.WorldTangent = normalize(mul(Input.Tangent, World));
	Output.WorldBitangent = CalculateBitangent(Output.WorldNormal, Output.WorldTangent);

	Output.bUseVertexColor = 0;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif

	return Output;
}