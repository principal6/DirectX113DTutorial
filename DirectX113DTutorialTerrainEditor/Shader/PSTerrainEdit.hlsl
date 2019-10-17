#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D MainTexture : register(t0);
Texture2D Layer0Texture : register(t1);
Texture2D Layer1Texture : register(t2);
Texture2D Layer2Texture : register(t3);
Texture2D Layer3Texture : register(t4);
Texture2D MaskingTexture : register(t5);

cbuffer cbMaskingSpace : register(b0)
{
	float4x4 Matrix;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result = MainTexture.Sample(CurrentSampler, input.UV.xy);

	float4 MaskingSpacePosition = mul(float4(input.WorldPosition.x, 0, -input.WorldPosition.z, 1), Matrix);
	float4 Masking = MaskingTexture.Sample(CurrentSampler, MaskingSpacePosition.xz);

	float4 Layer0 = Layer0Texture.Sample(CurrentSampler, input.UV.xy);
	float4 Layer1 = Layer1Texture.Sample(CurrentSampler, input.UV.xy);
	float4 Layer2 = Layer2Texture.Sample(CurrentSampler, input.UV.xy);
	float4 Layer3 = Layer3Texture.Sample(CurrentSampler, input.UV.xy);

	Result.xyz = Layer0.xyz * Masking.r + Result.xyz * (1.0f - Masking.r);
	Result.xyz = Layer1.xyz * Masking.g + Result.xyz * (1.0f - Masking.g);
	Result.xyz = Layer2.xyz * Masking.b + Result.xyz * (1.0f - Masking.b);
	Result.xyz = Layer3.xyz * Masking.a + Result.xyz * (1.0f - Masking.a);
	
	// Selection highlight
	if (input.UV.z > 0.0f)
	{
		float4 Factors = float4(0.2f, 0.4f, 0.6f, 0);
		Result.xyz *= Factors.xyz;
	}

	// Fixed directional light
	Result.xyz *= dot(input.WorldNormal, normalize(float4(1, 1, 0, 1)));

	return Result;
}