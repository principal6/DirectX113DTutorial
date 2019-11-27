#include "Deferred.hlsli"

SamplerState HDRSampler : register(s0);
Texture2D HDRTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float3 UVW = normalize(Input.TexCoord.xyz);
	float2 UV = float2(atan2(UVW.z, UVW.x) / K2PI, acos(UVW.y) * K1DIVPI);
	
	return HDRTexture.Sample(HDRSampler, UV);
}