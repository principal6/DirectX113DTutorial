#include "Header.hlsli"

float4 main(VS_PARTICLE_OUTPUT Input) : SV_TARGET
{
	return Input.TexColor;
}