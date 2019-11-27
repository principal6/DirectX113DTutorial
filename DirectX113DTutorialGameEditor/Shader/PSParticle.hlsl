#include "Particle.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D ParticleTexture : register(t0);

float4 main(VS_PARTICLE_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = Input.TexColor;

	// w component of TexColor is used to tell if it uses texture or not.
	if (Input.TexColor.w < 0)
	{
		// z component of TexColor is used as the alpha value of the entire image.
		float4 Sampled = ParticleTexture.Sample(CurrentSampler, Input.TexColor.xy);
		Sampled.a *= Input.TexColor.z;

		OutputColor = Sampled;
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}