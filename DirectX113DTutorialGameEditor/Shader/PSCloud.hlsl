#include "Base.hlsli"

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
	float2 SampleCoord = Input.TexCoord.xy + TimeCoord;
	float4 OutputColor = CloudTexture.Sample(CurrentSampler, SampleCoord);

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}