#include "HParticle.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D ParticleTexture : register(t0);

float4 main(VS_PARTICLE_OUTPUT Input) : SV_TARGET
{
	// w component of TexColor is used to tell if it uses texture or not.
	if (Input.TexColor.w < 0)
	{
		// z component of TexColor is used as the alpha value of the entire image.
		float4 Sampled = ParticleTexture.Sample(CurrentSampler, Input.TexColor.xy);
		Sampled.a *= Input.TexColor.z;

		return Sampled;
	}
	else
	{
		return Input.TexColor;
	}
}