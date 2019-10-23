#include "Header.hlsli"

cbuffer cbCamera : register(b0)
{
	float4 EyePosition;
};

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> Patch, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	const float KTessFactor = 24.0f;
	float4 CenterPosition = (Patch[0].WorldPosition + Patch[1].WorldPosition + Patch[2].WorldPosition) / 3.0f;
	float Distance = distance(CenterPosition, EyePosition);

	if (Distance < 0.0f)
	{
		Output.EdgeTessFactor[0] = 0.0f;
		Output.EdgeTessFactor[1] = 0.0f;
		Output.EdgeTessFactor[2] = 0.0f;

		Output.InsideTessFactor = 0.0f;
	}
	else
	{
		Output.EdgeTessFactor[0] = KTessFactor / Distance;
		Output.EdgeTessFactor[1] = KTessFactor / Distance;
		Output.EdgeTessFactor[2] = KTessFactor / Distance;

		Output.InsideTessFactor = KTessFactor / Distance;
	}
	
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
