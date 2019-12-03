struct VS_INPUT
{
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float	Range			: RANGE;
};

struct VS_OUTPUT
{
	float4	Position		: SV_Position;
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float	Range			: RANGE;
};

struct HS_OUTPUT
{
	float4	WorldPosition		: POSITION;
	float4	Color				: COLOR;
	float	Range				: RANGE;
	float	HemisphereDirection	: DIRECTION;
};

struct DS_OUTPUT
{
	float4	Position			: SV_Position;
	float4	WorldPosition		: POSITION0;
	float4	ProjectionPosition	: POSITION1;
	float4	Color				: COLOR;
	float	InverseRange		: INV_RANGE;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]	: SV_TessFactor;
	float InsideTessFactor[2] : SV_InsideTessFactor;
};