#include "Header2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture2D : register(t0);

cbuffer cbFlags : register(b0)
{
	bool UseTexture;
	bool3 Pads;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result = input.Color;
	if (UseTexture == true) Result = CurrentTexture2D.Sample(CurrentSampler, input.UV);
	return Result;
}