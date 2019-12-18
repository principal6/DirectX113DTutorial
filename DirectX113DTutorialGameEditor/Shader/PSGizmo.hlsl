#include "Base.hlsli"

cbuffer cbColorFactor : register(b3)
{
	float4 ColorFactor;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = (ColorFactor.w == 0) ? float4(ColorFactor.xyz, 1) : Input.Color * ColorFactor;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.rgb = pow(OutputColor.rgb, 0.4545);

	return OutputColor;
}