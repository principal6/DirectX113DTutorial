#include "Shared.hlsli"

struct VS_PARTICLE_INPUT
{
	float4	Position	: POSITION;
	float4	TexColor	: TEXCOLOR;
	float	Rotation	: ROTATION;
	float2	Scaling		: SCALING;
};

struct VS_PARTICLE_OUTPUT
{
	float4	Position		: SV_POSITION;
	float4	WorldPosition	: POSITION;
	float4	TexColor		: TEXCOLOR;
	float	Rotation		: ROTATION;
	float2	Scaling			: SCALING;
};
