#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D WaterNormalTexture : register(t0);

cbuffer cbCamera : register(b0)
{
	float4	EyePosition;
}

cbuffer cbTime : register(b1)
{
	float Time;
	float3 Pads;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 ResultAlbedo = input.Color;

	float3x3 TextureSpace = float3x3(input.WorldTangent.xyz, input.WorldBitangent.xyz, input.WorldNormal.xyz);
	float2 ResultUV = input.UV.xy - float2(0, Time);
	float4 ResultNormal = normalize((WaterNormalTexture.Sample(CurrentSampler, ResultUV) * 2.0f) - 1.0f);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));

	// Fixed directional light (for edit mode)
	float4 Directional = CalculateDirectional(ResultAlbedo, float4(1, 1, 1, 1), 32.0f, 10.0f,
		float4(1, 1, 1, 1), normalize(float4(1, 1, 0, 1)), normalize(EyePosition - input.WorldPosition), ResultNormal);
	ResultAlbedo.xyz = Directional.xyz;
	
	return ResultAlbedo;
}