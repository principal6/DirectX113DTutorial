#include "Base.hlsli"
#include "iVSCBs.hlsli"

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
	Output.IsHighlighted = Input.IsHighlighted;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif

	return Output;
}