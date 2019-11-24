#include "Base.hlsli"

cbuffer cbTessFactor : register(b0)
{
	float EdgeTessFactor;
	float InsideTessFactor;
	float2 Pads;
}

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> Patch, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	Output.EdgeTessFactor[0] = EdgeTessFactor;
	Output.EdgeTessFactor[1] = EdgeTessFactor;
	Output.EdgeTessFactor[2] = EdgeTessFactor;

	Output.InsideTessFactor = InsideTessFactor;

	return Output;
}

[domain("tri")]
[maxtessfactor(64.0f)]
[outputcontrolpoints(3)]
[outputtopology("triangle_cw")]
[partitioning("fractional_odd")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> Patch, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
	HS_OUTPUT Output;

	Output = Patch[i];

	return Output;
}
