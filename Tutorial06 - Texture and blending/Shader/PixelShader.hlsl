#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture2D : register(t0);

cbuffer cbTexture : register(b0)
{
	bool UseTexture;
	bool3 Pad;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	if (UseTexture == true)
	{
		return CurrentTexture2D.Sample(CurrentSampler, input.UV);
	}
	else
	{
		return input.Color;
	}
}