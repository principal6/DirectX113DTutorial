#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture2D : register(t0);

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result = CurrentTexture2D.Sample(CurrentSampler, input.UV.xy);

	if (input.UV.z > 0.0f)
	{
		float4 Factors = float4(0.2f, 0.4f, 0.6f, 0);
		Result.xyz *= Factors.xyz;
	}

	Result.xyz *= dot(input.WorldNormal, normalize(float4(1, 1, 0, 1)));

	return Result;
}