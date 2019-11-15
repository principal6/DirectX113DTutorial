#include "Line.hlsli"

float4 main(VS_LINE_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = Input.Color;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.rgb = pow(OutputColor.rgb, 0.4545);

	return OutputColor;
}