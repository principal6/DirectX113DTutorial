#include "Header.hlsli"

cbuffer cbCamera : register(b0)
{
	float4 EyePosition;
};

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> Patch, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	const float KTessFactor = 32.0f;
	float4 CenterPosition = (Patch[0].WorldPosition + Patch[1].WorldPosition + Patch[2].WorldPosition) / 3.0f;
	float Distance = distance(CenterPosition, EyePosition);

	float ResultTess = KTessFactor / Distance;
	if (Distance <= 37.5f) ResultTess = KTessFactor * 0.5f;
	if (Distance <= 25.0f) ResultTess = KTessFactor * 0.75f;
	if (Distance <= 12.5f) ResultTess = KTessFactor;
	if (Distance >= 75.0f) ResultTess = 0.0f;

	Output.EdgeTessFactor[0] = ResultTess;
	Output.EdgeTessFactor[1] = ResultTess;
	Output.EdgeTessFactor[2] = ResultTess;

	Output.InsideTessFactor = ResultTess;

	return Output;
}

[domain("tri")]
[maxtessfactor(64.0f)]
[outputcontrolpoints(3)]
[outputtopology("triangle_cw")]
[partitioning("fractional_even")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> Patch, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
	HS_OUTPUT Output;

	Output = Patch[i];

	return Output;
}
