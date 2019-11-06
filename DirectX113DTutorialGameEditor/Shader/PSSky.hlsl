#include "Base.hlsli"

cbuffer cbTime : register(b0)
{
	float	SkyTime;
	float3	Pads;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Result;

	float4 PDirection = normalize(input.WorldPosition);
	float AbsAngle = abs(dot(PDirection, KUpDirection));

	Result.r = pow(sin(SkyTime * K4PI - KPIDIV2), 3.0f) * 0.4f;
	Result.r *= pow((1 - AbsAngle), 1.6f);
	
	Result.g = pow(sin(SkyTime * K2PI - KPIDIV2) / 2.0f + 0.5f, 3.0f) * 0.7f;
	Result.b = pow(sin(SkyTime * K2PI - KPIDIV2) / 2.0f + 0.5f, 3.0f);
	if (Result.b < 0.1f) Result.b = 0.1f;
	Result.a = 1.0f;

	return Result;
}