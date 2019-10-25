#include "Header.hlsli"

cbuffer cbCamera : register(b0)
{
	float4 EyePosition;
};

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> Patch, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	const float KEdgeMax = 8.0f;
	const float KInsideMax = 16.0f;
	float4 CenterPosition = (Patch[0].WorldPosition + Patch[1].WorldPosition + Patch[2].WorldPosition) / 3.0f;
	float Distance = distance(CenterPosition, EyePosition);
	float Edge = KEdgeMax;
	float Inside = KInsideMax / Distance;

	Output.EdgeTessFactor[0] = KEdgeMax;
	Output.EdgeTessFactor[1] = KEdgeMax;
	Output.EdgeTessFactor[2] = KEdgeMax;

	Output.InsideTessFactor = Inside;
	
	return Output;
}

[domain("tri")]
[maxtessfactor(64.0f)]
[outputcontrolpoints(3)]
[outputtopology("triangle_cw")]
[partitioning("fractional_even")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> Patch, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{
	HS_OUTPUT Output;

	Output = Patch[i];

	return Output;
}
