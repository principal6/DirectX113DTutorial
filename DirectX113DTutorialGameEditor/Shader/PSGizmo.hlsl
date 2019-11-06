#include "Base.hlsli"

cbuffer cbColorFactor : register(b0)
{
	float4 ColorFactor;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return input.Color * ColorFactor;
}