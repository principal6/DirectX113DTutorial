#include "Header.hlsli"

cbuffer cbTime : register(b0)
{
	float SkyTime;
	float3 Pads;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result = input.Color;
	float ColorFactor = SkyTime;

	if (SkyTime <= 0.0f) ColorFactor = 0.0f;
	
	Result.r *= pow(ColorFactor, 0.5f);
	Result.g *= ColorFactor;

	float BlueFactor = ColorFactor;
	if (BlueFactor <= 0.06f) BlueFactor = 0.06f;
	Result.b *= BlueFactor;

	return Result;
}