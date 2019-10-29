#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 VP;
}

VS_PARTICLE_OUTPUT main(VS_PARTICLE_INPUT Input)
{
	VS_PARTICLE_OUTPUT Output;
	
	Output.Position = mul(float4(Input.Position.xyz, 1), VP);
	Output.Rotation = Input.Rotation;
	Output.Scaling = Input.Scaling;

	return Output;
}