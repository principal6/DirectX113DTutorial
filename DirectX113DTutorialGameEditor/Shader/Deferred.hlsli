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
	float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};