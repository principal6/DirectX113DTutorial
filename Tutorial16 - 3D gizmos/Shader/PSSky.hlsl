#include "Header.hlsli"

#define KPIDIV2	1.57079f
#define KPI		3.14159f
#define K2PI	6.28318f
#define K4PI	12.56637f

cbuffer cbTime : register(b0)
{
	float	SkyTime;
	float3	Pads;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result;

	Result.r = pow(sin(SkyTime * K4PI - KPIDIV2), 3.0f) * 0.3f;
	Result.g = pow(sin(SkyTime * K2PI - KPIDIV2) / 2.0f + 0.5f, 3.0f) * 0.7f;
	Result.b = pow(sin(SkyTime * K2PI - KPIDIV2) / 2.0f + 0.5f, 3.0f);
	if (Result.b < 0.1f) Result.b = 0.1f;
	Result.a = 1.0f;

	return Result;
}