struct VS_INPUT
{
	float4 Position	: POSITION;
	float4 Color	: COLOR;
	float2 UV		: TEXCOORD;
	float4 Normal	: NORMAL;
};

struct VS_INPUT_ANIMATION
{
	float4	Position		: POSITION;
	float4	Color			: COLOR;
	float2	UV				: TEXCOORD;
	float4	Normal			: NORMAL;

	uint4	BoneIndex		: BLENDINDICES;
	float4	BoneWeight		: BLENDWEIGHT;
};

struct VS_LINE_INPUT
{
	float4 Position	: POSITION;
	float4 Color	: COLOR;
};

struct VS_OUTPUT
{
	float4 Position			: SV_POSITION;
	float4 WorldPosition	: POSITION;
	float4 Color			: COLOR;
	float2 UV				: TEXCOORD;
	float4 WorldNormal		: NORMAL0;
	float4 WVPNormal		: NORMAL1;
};

struct VS_LINE_OUTPUT
{
	float4 Position			: SV_POSITION;
	float4 Color			: COLOR;
};