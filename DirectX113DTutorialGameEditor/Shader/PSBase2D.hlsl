#include "Base2D.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture2D : register(t0);

float4 main(VS_2D_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = CurrentTexture2D.Sample(CurrentSampler, Input.TexCoord.xy);
	return OutputColor;
}

float4 RawVertexColor(VS_2D_OUTPUT Input) : SV_TARGET
{
	return Input.Color;
}