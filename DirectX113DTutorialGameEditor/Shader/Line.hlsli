#include "Shared.hlsli"

struct VS_LINE_INPUT
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
};

struct VS_LINE_OUTPUT
{
	float4 Position		: SV_POSITION;
	float4 Color		: COLOR;
};
