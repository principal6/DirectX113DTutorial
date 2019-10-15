#include "Header2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D MaskingTexture : register(t0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result = input.Color;
	float4 Masking = MaskingTexture.Sample(CurrentSampler, input.UV, 0);
	
	Result.rgb = Masking.rgb;
	Result.a = Masking.a;
	if (Result.a < 0.2f) Result.a = 2.0f;

	return Result;
}