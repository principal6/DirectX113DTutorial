#include "Billboard.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 View;
	float4x4 Projection;
	float4x4 ViewProjection;
}

cbuffer cbBillboard : register(b1)
{
	float2 PixelSize;
	float2 ScreenSize;
	float2 WorldSpaceSize;
	bool bIsScreenSpace;
	float Reserved;
}

[domain("quad")]
DS_BILLBOARD_OUTPUT main(HS_CONSTANT_DATA_OUTPUT ConstantData, float2 Domain : SV_DomainLocation, const OutputPatch<HS_BILLBOARD_OUTPUT, 1> ControlPoints)
{
	DS_BILLBOARD_OUTPUT Output;

	const float2 KSignedDomain = Domain * 2.0 - 1.0;

	float4 ResultPosition = ControlPoints[0].InstancePosition;

	if (bIsScreenSpace == true)
	{
		// Screen-space billboard

		const float2 KNormalizedSize = float2(PixelSize.x / ScreenSize.x, PixelSize.y / ScreenSize.y);
		ResultPosition = mul(ResultPosition, ViewProjection);
		ResultPosition /= ResultPosition.w; // @important
		ResultPosition.x += KSignedDomain.x * KNormalizedSize.x * 0.5;
		ResultPosition.y -= KSignedDomain.y * KNormalizedSize.y * 0.5;
	}
	else
	{
		// World-space billboard

		ResultPosition = mul(ResultPosition, View);
		ResultPosition /= ResultPosition.w; // @important
		ResultPosition.x += KSignedDomain.x * WorldSpaceSize.x * 0.5;
		ResultPosition.y -= KSignedDomain.y * WorldSpaceSize.y * 0.5;
		ResultPosition = mul(ResultPosition, Projection);
	}

	Output.Position = ResultPosition;
	Output.TexCoord = float2(Domain.x, 1.0 - Domain.y);
	Output.InstanceID = ControlPoints[0].InstanceID;
	Output.IsHighlighted = ControlPoints[0].IsHighlighted;

	return Output;
}
