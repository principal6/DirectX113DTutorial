#include "Deferred.hlsli"

SamplerState PointClampSampler : register(s0);
Texture2D DeferredTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	return DeferredTexture.Sample(PointClampSampler, Input.TexCoord.xy);
}

float4 opaque(VS_OUTPUT Input) : SV_TARGET
{
	return float4(DeferredTexture.Sample(PointClampSampler, Input.TexCoord.xy).xyz, 1);
}