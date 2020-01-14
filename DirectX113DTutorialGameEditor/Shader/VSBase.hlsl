#include "Base.hlsli"
#include "iVSCBs.hlsli"

static VS_OUTPUT Internal(in float4x4 WorldMatrix, in VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.WorldPosition = mul(Input.Position, WorldMatrix);
	Output.Position = mul(Output.WorldPosition, ViewProjection);

	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	Output.WorldNormal = normalize(mul(Input.Normal, WorldMatrix));
	Output.WorldTangent = normalize(mul(Input.Tangent, WorldMatrix));
	Output.WorldBitangent = normalize(float4(cross(Output.WorldNormal.xyz, Output.WorldTangent.xyz), 0));

	Output.bUseVertexColor = 0;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif
	Output.IsHighlighted = Input.IsHighlighted;

	return Output;
}

VS_OUTPUT main(VS_INPUT Input)
{
	return Internal(World, Input);
}

VS_OUTPUT Instanced(VS_INPUT Input)
{
	float4x4 InstanceWorld = float4x4(Input.InstanceWorld0, Input.InstanceWorld1, Input.InstanceWorld2, Input.InstanceWorld3);
	return Internal(InstanceWorld, Input);
}
