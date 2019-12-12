#include "Terrain.hlsli"
#include "iDSCBs.hlsli"

cbuffer cbDisplacement : register(b1)
{
	bool UseDisplacement;
	float DisplacementFactor;
	float2 Pads;
}

static const float KDisplacementFactor = 0.25f;

SamplerState CurrentSampler : register(s0);
Texture2D DisplacementTexture : register(t0);

[domain("tri")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT ConstantData, float3 Domain : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 3> ControlPoints)
{
	DS_OUTPUT Output;

	Output.TexCoord = ControlPoints[0].TexCoord * Domain.x + ControlPoints[1].TexCoord * Domain.y + ControlPoints[2].TexCoord * Domain.z;
	Output.bUseVertexColor = ControlPoints[0].bUseVertexColor + ControlPoints[1].bUseVertexColor + ControlPoints[2].bUseVertexColor;
	Output.IsHighlighted = ControlPoints[0].IsHighlighted + ControlPoints[1].IsHighlighted + ControlPoints[2].IsHighlighted;

#ifndef DEBUG_SHADER
	Output.InstanceID = ControlPoints[0].InstanceID;
#endif

	float4 P1 = ControlPoints[0].WorldPosition;
	float4 P2 = ControlPoints[1].WorldPosition;
	float4 P3 = ControlPoints[2].WorldPosition;

	float4 N1 = normalize(ControlPoints[0].WorldNormal);
	float4 N2 = normalize(ControlPoints[1].WorldNormal);
	float4 N3 = normalize(ControlPoints[2].WorldNormal);

	float4 WorldPosition = GetBezierPosition(P1, P2, P3, N1, N2, N3, Domain);
	//float4 WorldNormal = GetBezierNormal(P1, P2, P3, N1, N2, N3, Domain);
	float4 WorldNormal =
		normalize(ControlPoints[0].WorldNormal * Domain.x + ControlPoints[1].WorldNormal * Domain.y + ControlPoints[2].WorldNormal * Domain.z);
	
	if (UseDisplacement)
	{
		float Displacement = DisplacementTexture.SampleLevel(CurrentSampler, Output.TexCoord.xy, 0).r;
		WorldPosition += WorldNormal * Displacement * KDisplacementFactor;
	}

	Output.Position = Output.WorldPosition = WorldPosition;
	Output.WorldNormal = WorldNormal;
	Output.WorldTangent = 
		normalize(ControlPoints[0].WorldTangent * Domain.x + ControlPoints[1].WorldTangent * Domain.y + ControlPoints[2].WorldTangent * Domain.z);
	Output.WorldBitangent = CalculateBitangent(Output.WorldNormal, Output.WorldTangent);
	Output.WorldTangent = float4(cross(Output.WorldNormal.xyz, Output.WorldBitangent.xyz), 0);

	// This bool is used for drawing normals.
	// If the value is not 0, it means that it is already in projection space by GSNormal.
	if (Output.bUseVertexColor == 0)
	{
		Output.Position = mul(float4(Output.Position.xyz, 1), ViewProjection);
	}

	Output.Color = ControlPoints[0].Color * Domain.x + ControlPoints[1].Color * Domain.y + ControlPoints[2].Color * Domain.z;

	return Output;
}
