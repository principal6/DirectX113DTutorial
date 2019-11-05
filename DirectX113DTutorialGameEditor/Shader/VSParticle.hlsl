#include "HParticle.hlsli"

VS_PARTICLE_OUTPUT main(VS_PARTICLE_INPUT Input)
{
	VS_PARTICLE_OUTPUT Output;
	
	Output.Position = float4(Input.Position.xyz, 1);
	Output.WorldPosition = Output.Position;
	Output.TexColor = Input.TexColor;
	Output.Rotation = Input.Rotation;
	Output.Scaling = Input.Scaling;

	return Output;
}