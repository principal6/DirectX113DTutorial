#include "Header.hlsli"

float4 main(VS_LINE_OUTPUT input) : SV_TARGET
{
	return input.Color;
}