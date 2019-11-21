#include "Deferred.hlsli"

SamplerState PointSampler : register(s0);
Texture2D DeferredTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	return DeferredTexture.Sample(PointSampler, Input.TexCoord.xy);
}