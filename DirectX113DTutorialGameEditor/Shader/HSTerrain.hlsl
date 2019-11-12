#include "Terrain.hlsli"

cbuffer cbCamera : register(b0)
{
	float4 EyePosition;
};

cbuffer cbTessFactor : register(b1)
{
	float TessFactor;
	float3 Pads;
}

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> Patch, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	const float KEdgeMax = 0.5f * TessFactor;
	const float KInsideMax = 1.0f * TessFactor;

	float Distance0 = distance(Patch[0].WorldPosition, EyePosition);
	float Distance1 = distance(Patch[1].WorldPosition, EyePosition);
	float Distance2 = distance(Patch[2].WorldPosition, EyePosition);
	Distance0 = ThresholdDistance(Distance0);
	Distance1 = ThresholdDistance(Distance1);
	Distance2 = ThresholdDistance(Distance2);

	float E0 = KEdgeMax / Distance0;
	float E1 = KEdgeMax / Distance1;
	float E2 = KEdgeMax / Distance2;
	float Edge0 = max(E1, E2);
	float Edge1 = max(E0, E2);
	float Edge2 = max(E0, E1);
	Output.EdgeTessFactor[0] = Edge0;
	Output.EdgeTessFactor[1] = Edge1;
	Output.EdgeTessFactor[2] = Edge2;

	float4 CenterPosition = (Patch[0].WorldPosition + Patch[1].WorldPosition + Patch[2].WorldPosition) / 3.0f;
	float DistanceCenter = distance(CenterPosition, EyePosition);
	DistanceCenter = ThresholdDistance(DistanceCenter);
	float Inside = KInsideMax / DistanceCenter;
	Output.InsideTessFactor = Inside;
	
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
