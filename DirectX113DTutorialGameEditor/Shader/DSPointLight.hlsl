#include "DeferredLight.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
	float4x4 WVP;
}

[domain("quad")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT ConstantData, float2 Domain : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 1> ControlPoints)
{
	DS_OUTPUT Output;

	float2 ClipSpaceDomain = Domain * 2.0 - 1.0;
	float2 ClipSpaceDomainAbsolute = abs(ClipSpaceDomain);
	float MaxLength = max(ClipSpaceDomainAbsolute.x, ClipSpaceDomainAbsolute.y);

	float3 LocalSpacePosition = float3
		(
			ClipSpaceDomain.x * ControlPoints[0].HemisphereDirection,
			ClipSpaceDomain.y,
			(MaxLength - 1.0) * ControlPoints[0].HemisphereDirection
		);
	Output.Position = float4(normalize(LocalSpacePosition), 1);
	Output.Position = mul(Output.Position, WVP);

	return Output;
}
