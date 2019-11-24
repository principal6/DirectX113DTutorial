#include "Deferred.hlsli"

SamplerState CubemapSampler : register(s0);
TextureCube CubemapTexture : register(t0);

cbuffer cbRadiancePrefiltering : register(b0)
{
	float Roughness;
	float RangeFactor;
	float2 Pads;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	const uint KSampleCount = 2048;
	const uint KOrder = GetHammersleyOrder(KSampleCount);
	const float KBase = GetHammersleyBase(KOrder);

	float3 Wn = normalize(Input.TexCoord.xyz);
	float3 Wo = Wn;
	float3 PrefilteredRadiance = float3(0, 0, 0);
	float TotalWeight = 0;
	for (uint i = 0; i < KSampleCount; i++)
	{
		float2 HammersleyPoint = Hammersley(i, KSampleCount, KOrder, KBase);
		float3 Wm = ImportanceSampleGGX(HammersleyPoint, Roughness, Wn);
		float3 Wi = 2 * dot(Wo, Wm) * Wm - Wo;
		float WnDotWi = saturate(dot(Wn, Wi));
		if (WnDotWi > 0)
		{
			PrefilteredRadiance += CubemapTexture.SampleLevel(CubemapSampler, Wi, 0).rgb * WnDotWi;
			TotalWeight += WnDotWi;
		}
	}

	PrefilteredRadiance = PrefilteredRadiance / TotalWeight;

	// HDR range adjustment
	PrefilteredRadiance *= RangeFactor;

	return float4(PrefilteredRadiance, 1);
}