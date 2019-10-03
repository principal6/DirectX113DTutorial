struct VS_INPUT
{
	float4 Position : POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD;
	float4 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
	float2 UV : TEXCOORD;
	float4 Normal : NORMAL0;
	float4 WVPNormal : NORMAL1;
};