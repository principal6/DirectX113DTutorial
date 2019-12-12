#include "Terrain.hlsli"
#include "iDSCBs.hlsli"

static const float KDisplacementFactor = 0.25f;

cbuffer cbTime : register(b1)
{
	float Time;
	float3 Pads;
}

SamplerState CurrentSampler : register(s0);
Texture2D DisplacementTexture : register(t0);

[domain("tri")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT TessFactors, float3 Domain : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 3> Patch)
{
	DS_OUTPUT Output;

	Output.TexCoord = Patch[0].TexCoord * Domain.x + Patch[1].TexCoord * Domain.y + Patch[2].TexCoord * Domain.z;

	Output.WorldNormal = Patch[0].WorldNormal * Domain.x + Patch[1].WorldNormal * Domain.y + Patch[2].WorldNormal * Domain.z;
	Output.WorldNormal = normalize(Output.WorldNormal);

	Output.bUseVertexColor = Patch[0].bUseVertexColor + Patch[1].bUseVertexColor + Patch[2].bUseVertexColor;
	Output.IsHighlighted = Patch[0].IsHighlighted + Patch[1].IsHighlighted + Patch[2].IsHighlighted;

#ifndef DEBUG_SHADER
	Output.InstanceID = Patch[0].InstanceID;
#endif

	float4 P1 = Patch[0].WorldPosition;
	float4 P2 = Patch[1].WorldPosition;
	float4 P3 = Patch[2].WorldPosition;

	float4 N1 = normalize(Patch[0].WorldNormal);
	float4 N2 = normalize(Patch[1].WorldNormal);
	float4 N3 = normalize(Patch[2].WorldNormal);

	float4 Bezier = GetBezierPosition(P1, P2, P3, N1, N2, N3, Domain);
	
	float2 AnimatedUV = Output.TexCoord.xy - float2(0, Time);
	float Displacement = DisplacementTexture.SampleLevel(CurrentSampler, AnimatedUV, 0).r;
	Bezier += Output.WorldNormal * Displacement * KDisplacementFactor;
	Bezier.y -= KDisplacementFactor;

	Output.Position = Output.WorldPosition = Bezier;

	// This bool is used for drawing normals.
	// If the value is not 0, it means that it is already in projection space by GSNormal.
	if (Output.bUseVertexColor == 0)
	{
		Output.Position = mul(float4(Output.Position.xyz, 1), ViewProjection);
	}

	Output.Color = Patch[0].Color * Domain.x + Patch[1].Color * Domain.y + Patch[2].Color * Domain.z;

	Output.WorldTangent = Patch[0].WorldTangent * Domain.x + Patch[1].WorldTangent * Domain.y + Patch[2].WorldTangent * Domain.z;
	Output.WorldTangent = normalize(Output.WorldTangent);

	Output.WorldBitangent = Patch[0].WorldBitangent * Domain.x + Patch[1].WorldBitangent * Domain.y + Patch[2].WorldBitangent * Domain.z;
	Output.WorldBitangent = normalize(Output.WorldBitangent);

	return Output;
}
