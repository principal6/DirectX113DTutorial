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

	float4x4 InstanceWorld = float4x4(Input.InstanceWorld0, Input.InstanceWorld1, Input.InstanceWorld2, Input.InstanceWorld3);

	Output.WorldPosition = mul(Input.Position, InstanceWorld);
	Output.Position = mul(Output.WorldPosition, ViewProjection);

	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	Output.WorldNormal = normalize(mul(Input.Normal, InstanceWorld));
	Output.WorldTangent = normalize(mul(Input.Tangent, InstanceWorld));
	Output.WorldBitangent = CalculateBitangent(Output.WorldNormal, Output.WorldTangent);

	Output.bUseVertexColor = 0;
	Output.IsHighlighted = Input.IsHighlighted;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif

	return Output;
}