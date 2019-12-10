#include "Base.hlsli"

SamplerState CurrentSampler : register(s0);

TextureCube EnvironmentTexture : register(t50);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float3 DiffuseColor = Input.Color.xyz;

	DiffuseColor = EnvironmentTexture.Sample(CurrentSampler, Input.TexCoord).xyz;

	return float4(DiffuseColor, Input.Color.a);
}