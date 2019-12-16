#include "Deferred.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	return CurrentTexture.Sample(CurrentSampler, Input.TexCoord.xy);
}

float4 Opaque(VS_OUTPUT Input) : SV_TARGET
{
	return float4(CurrentTexture.Sample(CurrentSampler, Input.TexCoord.xy).xyz, 1);
}

float4 Monochrome(VS_OUTPUT Input) : SV_TARGET
{
	return float4(CurrentTexture.Sample(CurrentSampler, Input.TexCoord.xy).xxx, 1);
}