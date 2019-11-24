#include "Shared.hlsli"

static float KGaussianFilter[5][5] =
{
	{ 0.0037, 0.0146, 0.0256, 0.0146, 0.0037 },
	{ 0.0146, 0.0586, 0.0952, 0.0586, 0.0146 },
	{ 0.0256, 0.0952, 0.1502, 0.0952, 0.0256 },
	{ 0.0146, 0.0586, 0.0952, 0.0586, 0.0146 },
	{ 0.0037, 0.0146, 0.0256, 0.0146, 0.0037 }
};

static float3x3 KSobelKernelX =
{
	-1,  0, +1,
	-2,  0, +2,
	-1,  0, +1
};

static float3x3 KSobelKernelY =
{
	+1, +2, +1,
	 0,  0,  0,
	-1, -2, -1
};

/*
struct VS_INPUT
{
	uint VertexID : SV_VertexID;
};
*/

struct VS_INPUT
{
	float4 Position : POSITION;
	float3 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float3 TexCoord : TEXCOORD;
};

float3 ImportanceSampleGGX(float2 HammersleyPoint, float Roughness, float3 Normal)
{
	float Alpha = Roughness * Roughness;
	float Phi = 2 * KPI * HammersleyPoint.x;
	float CosTheta = sqrt((1 - HammersleyPoint.y) / (1 + (Alpha * Alpha - 1) * HammersleyPoint.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);
	float3 H = float3(SinTheta * cos(Phi), CosTheta, SinTheta * sin(Phi));

	float3 UpVector = abs(Normal.y) < 0.999 ? float3(0, 1, 0) : float3(0, 0, -1);

	float3 CubeSpaceX = normalize(cross(UpVector, Normal));
	float3 CubeSpaceZ = cross(CubeSpaceX, Normal);

	return CubeSpaceX * H.x + Normal * H.y + CubeSpaceZ * H.z;
}

uint GetHammersleyOrder(uint SampleCount)
{
	return (uint)log2((float)(SampleCount - 1)) + 1;
}

float GetHammersleyBase(uint Order)
{
	return pow(2.0f, -(float)Order);
}

float2 Hammersley(uint Seed, uint SampleCount, uint Order, float Base)
{
	uint InvertedBits = 0;
	uint ShiftStep = Order - 1;
	while (true)
	{
		InvertedBits |=
			((Seed & 0x1 << ((Order / 2 + (ShiftStep + 1) / 2) - 1)) >> ShiftStep) |
			((Seed & 0x1 << (Order / 2 - (ShiftStep + 1) / 2)) << ShiftStep);

		if (ShiftStep <= 1) break;

		ShiftStep -= 2;
	}

	float X = Base * (float)Seed;
	float Y = Base * (float)InvertedBits;

	return float2(X, Y);
}