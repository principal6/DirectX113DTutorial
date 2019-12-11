struct VS_INPUT
{
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float4	Direction		: DIRECTION;
	float	Range			: RANGE;
	float	Theta			: THETA;
};

struct VS_OUTPUT
{
	float4	Position		: SV_Position;
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float4	Direction		: DIRECTION;
	float	Range			: RANGE;
	float	Theta			: THETA;
};

struct HS_POINT_LIGHT_OUTPUT
{
	float4	WorldPosition		: POSITION;
	float4	Color				: COLOR;
	float	Range				: RANGE;
	float	HemisphereDirection	: DIRECTION;
};

struct HS_SPOT_LIGHT_OUTPUT
{
	float4	WorldPosition		: POSITION;
	float4	Color				: COLOR;
	float4	Direction			: DIRECTION;
	float	Range				: RANGE;
	float	Theta				: THETA;
};

struct DS_POINT_LIGHT_OUTPUT
{
	float4	Position		: SV_Position;
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float	InverseRange	: INV_RANGE;
};

struct DS_SPOT_LIGHT_OUTPUT
{
	float4	Position		: SV_Position;
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float4	Direction		: DIRECTION;
	float	InverseRange	: INV_RANGE;
	float	CosThetaDiv2	: COS_THETA_DIV_TWO;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]	: SV_TessFactor;
	float InsideTessFactor[2] : SV_InsideTessFactor;
};