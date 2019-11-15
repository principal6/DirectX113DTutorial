#include "Base2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D MaskingTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 Masking = MaskingTexture.Sample(CurrentSampler, Input.UV, 0);
	
	float4 OutputColor = Input.Color;
	OutputColor.rgb = Masking.rgb;
	OutputColor.a = Masking.a;
	if (OutputColor.a < 0.2f) OutputColor.a = 2.0f;

	return OutputColor;
}