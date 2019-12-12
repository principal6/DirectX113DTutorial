#include "Base.hlsli"
#include "iVSCBs.hlsli"

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
	Output.IsHighlighted = Input.IsHighlighted;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif

	return Output;
}