#include "iFont.hlsli"

cbuffer cbColor : register(b3)
{
	float4 Color;
};

SamplerState Sampler : register(s0);
Texture2D Texture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 Sampled = Texture.Sample(Sampler, Input.TexCoord);

	Sampled.rgb = Color.rgb;
	Sampled.a *= Color.a;

	return Sampled;
}
