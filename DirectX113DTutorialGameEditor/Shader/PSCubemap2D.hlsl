#include "Base2D.hlsli"

SamplerState CubemapSampler : register(s0);
TextureCube CubemapTexture : register(t0);

float4 main(VS_2D_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = CubemapTexture.Sample(CubemapSampler, Input.TexCoord);
	return OutputColor;
}