#include "Header.hlsli"

cbuffer cbTime : register(b0)
{
	float	SkyTime;
	float3	Pads;
};

SamplerState CurrentSampler : register(s0);
Texture2D CloudTexture : register(t0);
Texture2D NoiseTexture : register(t1);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float2 TimeCoord = float2(0.0f, SkyTime);
	float2 SampleCoord = Input.UV.xy + TimeCoord;
	float4 Result = CloudTexture.Sample(CurrentSampler, SampleCoord);
	return Result;
}