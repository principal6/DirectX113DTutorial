#include "Billboard.hlsli"

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] =
		Output.EdgeTessFactor[2] = Output.EdgeTessFactor[3] =
		Output.InsideTessFactor[0] = Output.InsideTessFactor[1] = 1;

	return Output;
}

[domain("quad")]
[outputcontrolpoints(1)]
[outputtopology("triangle_cw")]
[partitioning("integer")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_BILLBOARD_OUTPUT main(InputPatch<VS_BILLBOARD_OUTPUT, 1> ControlPoints, uint ControlPointID : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
	HS_BILLBOARD_OUTPUT Output = ControlPoints[0];
	return Output;
}