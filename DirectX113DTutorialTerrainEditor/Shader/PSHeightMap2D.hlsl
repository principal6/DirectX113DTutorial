#include "Header2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D HeightMapTexture : register(t0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result = input.Color;
	float4 Height = HeightMapTexture.Sample(CurrentSampler, input.UV, 0);

	Result.rgb = Height.r;
	Result.a = 1.0f;

	return Result;
}