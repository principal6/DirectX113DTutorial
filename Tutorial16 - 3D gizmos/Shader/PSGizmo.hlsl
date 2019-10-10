#include "Header.hlsli"

cbuffer cbColorFactor
{
	float4 ColorFactor;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return input.Color * ColorFactor;
}