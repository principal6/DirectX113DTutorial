struct HS_OUTPUT
{
	float HemisphereDirection : DIRECTION;
};

struct DS_OUTPUT
{
	float4 Position	: SV_POSITION;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]	: SV_TessFactor;
	float InsideTessFactor[2] : SV_InsideTessFactor;
};