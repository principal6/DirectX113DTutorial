static const float KPI = 3.14159;

struct VS_BILLBOARD_INPUT
{
	uint	InstanceID			: SV_InstanceID;
	float4	InstancePosition	: POSITION;
	float	InstanceRotation	: ROTATION;
	float2	InstanceScaling		: SCALING;
	float	IsHighlighted		: IS_HIGHLIGHTED;
};

struct VS_BILLBOARD_OUTPUT
{
	uint	InstanceID			: INSTANCE_ID;
	float4	InstancePosition	: SV_Position;
	float	InstanceRotation	: ROTATION;
	float2	InstanceScaling		: SCALING;
	float	IsHighlighted		: IS_HIGHLIGHTED;
};

#define HS_BILLBOARD_OUTPUT VS_BILLBOARD_OUTPUT

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]	: SV_TessFactor;
	float InsideTessFactor[2] : SV_InsideTessFactor;
};

struct DS_BILLBOARD_OUTPUT
{
	float4	Position		: SV_Position;
	float2	TexCoord		: TEXCOORD;
	uint	InstanceID		: INSTANCE_ID;
	float	IsHighlighted	: IS_HIGHLIGHTED;
};