#include "DeferredLight.hlsli"
#include "iDSCBs.hlsli"

[domain("quad")]
DS_POINT_LIGHT_OUTPUT main(HS_CONSTANT_DATA_OUTPUT ConstantData, float2 Domain : SV_DomainLocation, const OutputPatch<HS_POINT_LIGHT_OUTPUT, 1> ControlPoints)
{
	DS_POINT_LIGHT_OUTPUT Output;

	float2 ClipSpaceDomain = Domain * 2.0 - 1.0;
	float2 ClipSpaceDomainAbsolute = abs(ClipSpaceDomain);
	float MaxLength = max(ClipSpaceDomainAbsolute.x, ClipSpaceDomainAbsolute.y);

	float3 LocalSpacePosition = float3
		(
			ClipSpaceDomain.x * ControlPoints[0].HemisphereDirection,
			ClipSpaceDomain.y,
			(MaxLength - 1.0) * ControlPoints[0].HemisphereDirection
		);

	float4 WorldVertexPosition = float4(normalize(LocalSpacePosition) * ControlPoints[0].Range + ControlPoints[0].WorldPosition.xyz, 1);
	Output.Position = mul(WorldVertexPosition, ViewProjection);
	Output.WorldPosition = ControlPoints[0].WorldPosition; // @important: light's center position in world space
	Output.Color = ControlPoints[0].Color;
	Output.InverseRange = 1.0 / ControlPoints[0].Range;
	return Output;
}
