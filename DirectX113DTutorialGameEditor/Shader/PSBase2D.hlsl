#include "Base2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture2D : register(t0);

cbuffer cbFlags : register(b0)
{
	bool UseTexture;
	bool3 Pads;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = Input.Color;

	if (UseTexture == true)
	{
		OutputColor = CurrentTexture2D.Sample(CurrentSampler, Input.TexCoord.xy);
	}

	return OutputColor;
}