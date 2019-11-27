#include "Base.hlsli"

cbuffer cbTime : register(b0)
{
	float	SkyTime;
	float3	Pads;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 PDirection = normalize(input.WorldPosition);
	float AbsAngle = abs(dot(PDirection, KUpDirection));

	float4 OutputColor = float4(0, 0, 0, 1);
	OutputColor.r = pow(sin(SkyTime * K4PI - KPIDIV2), 3.0f) * 0.4f;
	OutputColor.r *= pow((1.0f - AbsAngle), 1.6f);
	
	OutputColor.g = pow(sin(SkyTime * K2PI - KPIDIV2) / 2.0f + 0.5f, 3.0f) * (0.7f + (1.0f - AbsAngle) * 0.3f);
	OutputColor.r += pow(OutputColor.g, 4.0f);
	OutputColor.b = pow(sin(SkyTime * K2PI - KPIDIV2) / 2.0f + 0.5f, 3.0f);
	if (OutputColor.b < 0.1f) OutputColor.b = 0.1f;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545f);

	return OutputColor;
}