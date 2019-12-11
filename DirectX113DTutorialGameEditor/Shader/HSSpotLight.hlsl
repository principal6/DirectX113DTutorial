#include "DeferredLight.hlsli"

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(uint PatchID : SV_PrimitiveID)
{
	const float KTessFactor = 16.0;

	HS_CONSTANT_DATA_OUTPUT Output;

	Output.EdgeTessFactor[0] = Output.EdgeTessFactor[1] =
		Output.EdgeTessFactor[2] = Output.EdgeTessFactor[3] =
		Output.InsideTessFactor[0] = Output.InsideTessFactor[1] = KTessFactor;

	return Output;
}

[domain("quad")]
[maxtessfactor(64.0)]
[outputcontrolpoints(1)]
[outputtopology("triangle_cw")]
[partitioning("integer")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_SPOT_LIGHT_OUTPUT main(InputPatch<VS_OUTPUT, 1> ControlPoints, uint ControlPointID : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
	HS_SPOT_LIGHT_OUTPUT Output;

	Output.WorldPosition = ControlPoints[0].WorldPosition;
	Output.Color = ControlPoints[0].Color;
	Output.Direction = float4(ControlPoints[0].Direction.xyz, (PatchID == 0) ? 2 : 0);
	Output.Range = ControlPoints[0].Range;
	Output.Theta = ControlPoints[0].Theta;

	return Output;
}