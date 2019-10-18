#include "Header.hlsli"

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> Patch, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	Output.EdgeTessFactor[0] = 3.0f;
	Output.EdgeTessFactor[1] = 3.0f;
	Output.EdgeTessFactor[2] = 3.0f;

	Output.InsideTessFactor = 3.0f;

	return Output;
}

[domain("tri")]
[maxtessfactor(64.0f)]
[outputcontrolpoints(3)]
[outputtopology("triangle_cw")]
[partitioning("fractional_odd")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> Patch, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{
	HS_OUTPUT Output;

	Output = Patch[i];

	return Output;
}
