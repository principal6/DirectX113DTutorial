#include "Billboard.hlsli"

SamplerState LinearClampSampler : register(s0);
SamplerState PointClampSampler : register(s1);
Texture2D BillboardTexture : register(t0);

cbuffer cbEditorTime : register(b0)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Reserved;
}

float4 main(DS_BILLBOARD_OUTPUT Input) : SV_TARGET
{
	float4 Sampled = BillboardTexture.Sample(LinearClampSampler, Input.TexCoord);
	if (Input.IsHighlighted != 0.0f)
	{
		Sampled += float4(0.6, 0.3, 0.6, 0) * sin(NormalizedTime * KPI);
	}
	return Sampled;
}