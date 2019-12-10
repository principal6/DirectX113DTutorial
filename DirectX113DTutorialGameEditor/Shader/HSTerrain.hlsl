#include "Terrain.hlsli"

cbuffer cbLight : register(b0)
{
	float4	DirectionalLightDirection;
	float3	DirectionalLightColor;
	float	Exposure;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

cbuffer cbTessFactor : register(b1)
{
	float EdgeTessFactor;
	float InsideTessFactor;
	float2 Pads;
}

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(InputPatch<VS_OUTPUT, 3> ControlPoints, uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	float E0 = EdgeTessFactor / ThresholdDistance(distance(ControlPoints[0].WorldPosition, EyePosition));
	float E1 = EdgeTessFactor / ThresholdDistance(distance(ControlPoints[1].WorldPosition, EyePosition));
	float E2 = EdgeTessFactor / ThresholdDistance(distance(ControlPoints[2].WorldPosition, EyePosition));
	Output.EdgeTessFactor[0] = max(E1, E2);
	Output.EdgeTessFactor[1] = max(E0, E2);
	Output.EdgeTessFactor[2] = max(E0, E1);

	float DistanceCenter = 
		ThresholdDistance(distance((ControlPoints[0].WorldPosition + ControlPoints[1].WorldPosition + ControlPoints[2].WorldPosition) * 0.3334, EyePosition));
	float Inside = InsideTessFactor / DistanceCenter;
	Output.InsideTessFactor = Inside;
	
	return Output;
}

[domain("tri")]
[maxtessfactor(64.0)]
[outputcontrolpoints(3)]
[outputtopology("triangle_cw")]
[partitioning("fractional_odd")]
[patchconstantfunc("CalcHSPatchConstants")]
HS_OUTPUT main(InputPatch<VS_OUTPUT, 3> ControlPoints, uint ControlPointID : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{
	HS_OUTPUT Output = ControlPoints[ControlPointID];
	return Output;
}
