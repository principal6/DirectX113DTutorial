#include "Base.hlsli"

cbuffer cbGizmoSpace : register(b1)
{
	float4x4 WVP;
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.Position = mul(Input.Position, WVP);
	Output.Position.z *= 0.01f;
	Output.Color = Input.Color;

	Output.WorldPosition = Input.Position;
	Output.TexCoord = Input.TexCoord;
	Output.WorldNormal = Input.Normal;
	Output.WorldTangent = Input.Tangent;
	Output.WorldBitangent = float4(1, 0, 0, 0);
	Output.bUseVertexColor = 0;
	Output.IsHighlighted = Input.IsHighlighted;
#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif

	return Output;
}