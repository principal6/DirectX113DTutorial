#include "Base2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D HeightMapTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 Height = HeightMapTexture.Sample(CurrentSampler, Input.UV, 0);

	float4 OutputColor = Input.Color;
	OutputColor.rgb = Height.r;
	OutputColor.a = 1.0f;

	return OutputColor;
}