#include "Shared.hlsli"

struct VS_2D_INPUT
{
	float4 Position	: POSITION;
	float4 Color	: COLOR;
	float3 TexCoord	: TEXCOORD;
};

struct VS_2D_OUTPUT
{
	float4 Position	: SV_POSITION;
	float4 Color	: COLOR;
	float3 TexCoord	: TEXCOORD;
};