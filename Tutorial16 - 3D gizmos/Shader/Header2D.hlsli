struct VS_INPUT
{
	float4 Position	: POSITION;
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
};

struct VS_OUTPUT
{
	float4 Position	: SV_POSITION;
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
};