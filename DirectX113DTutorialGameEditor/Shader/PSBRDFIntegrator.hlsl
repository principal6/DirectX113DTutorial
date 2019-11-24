#include "Deferred.hlsli"
#include "BRDF.hlsli"

float2 main(VS_OUTPUT Input) : SV_TARGET
{
	const uint KSampleCount = 1024;
	const uint KOrder = GetHammersleyOrder(KSampleCount);
	const float KBase = GetHammersleyBase(KOrder);

	const float3 Wn = float3(0, 1, 0);
	float WnDotWo = Input.TexCoord.x; // cos input
	float Roughness = 1.0 - Input.TexCoord.y; // roughness input
	float Alpha = max(Roughness * Roughness, 0.001);

	float3 Wo;
	Wo.x = sqrt(1.0f - WnDotWo * WnDotWo); // sin (sin^2x = 1 - cos^2x)
	Wo.y = WnDotWo; // cos
	Wo.z = 0;
	float A = 0;
	float B = 0;

	for (uint i = 0; i < KSampleCount; i++)
	{
		float2 HammersleyPoint = Hammersley(i, KSampleCount, KOrder, KBase);
		float3 Wm = ImportanceSampleGGX(HammersleyPoint, Roughness, Wn);
		float3 Wi = 2 * dot(Wo, Wm) * Wm - Wo;
		float WnDotWi = saturate(Wi.y);
		float WnDotWm = saturate(Wm.y);
		float WoDotWm = saturate(dot(Wo, Wm));
		if (WnDotWi > 0)
		{
			float G = G2_Smith_GGX(WnDotWi, WnDotWo, Alpha);
			float G_Vis = G * WoDotWm / (WnDotWm * WnDotWo);
			float Fc = pow(1 - WoDotWm, 5);
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	return float2(A, B) / KSampleCount;
}