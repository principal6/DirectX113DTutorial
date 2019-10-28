#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer1DiffuseTexture : register(t1);
Texture2D Layer2DiffuseTexture : register(t2);
Texture2D Layer3DiffuseTexture : register(t3);
Texture2D Layer4DiffuseTexture : register(t4);

Texture2D Layer0NormalTexture : register(t5);
Texture2D Layer1NormalTexture : register(t6);
Texture2D Layer2NormalTexture : register(t7);
Texture2D Layer3NormalTexture : register(t8);
Texture2D Layer4NormalTexture : register(t9);

Texture2D MaskingTexture : register(t10);

cbuffer cbMaskingSpace : register(b0)
{
	float4x4 Matrix;
}

cbuffer cbLights : register(b1)
{
	float4	DirectionalLightDirection;
	float4	DirectionalLightColor;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 MaskingSpacePosition = mul(float4(input.WorldPosition.x, 0, -input.WorldPosition.z, 1), Matrix);
	float4 Masking = MaskingTexture.Sample(CurrentSampler, MaskingSpacePosition.xz);

	float4 DiffuseLayer0 = Layer0DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer1 = Layer1DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer2 = Layer2DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer3 = Layer3DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer4 = Layer4DiffuseTexture.Sample(CurrentSampler, input.UV.xy);

	float4 Albedo = DiffuseLayer0;
	Albedo.xyz = DiffuseLayer1.xyz * Masking.r + Albedo.xyz * (1.0f - Masking.r);
	Albedo.xyz = DiffuseLayer2.xyz * Masking.g + Albedo.xyz * (1.0f - Masking.g);
	Albedo.xyz = DiffuseLayer3.xyz * Masking.b + Albedo.xyz * (1.0f - Masking.b);
	Albedo.xyz = DiffuseLayer4.xyz * Masking.a + Albedo.xyz * (1.0f - Masking.a);

	float4 NormalLayer0 = normalize((Layer0NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer1 = normalize((Layer1NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer2 = normalize((Layer2NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer3 = normalize((Layer3NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer4 = normalize((Layer4NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);

	float3x3 TextureSpace = float3x3(input.WorldTangent.xyz, input.WorldBitangent.xyz, input.WorldNormal.xyz);
	float4 ResultNormal;
	ResultNormal = NormalLayer0;
	ResultNormal.xyz = NormalLayer1.xyz * Masking.r + ResultNormal.xyz * (1.0f - Masking.r);
	ResultNormal.xyz = NormalLayer2.xyz * Masking.g + ResultNormal.xyz * (1.0f - Masking.g);
	ResultNormal.xyz = NormalLayer3.xyz * Masking.b + ResultNormal.xyz * (1.0f - Masking.b);
	ResultNormal.xyz = NormalLayer4.xyz * Masking.a + ResultNormal.xyz * (1.0f - Masking.a);
	ResultNormal = normalize(ResultNormal);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));
	
	// Selection highlight (for edit mode)
	if (input.UV.z > 0.0f)
	{
		float4 Factors = float4(0.2f, 0.4f, 0.6f, 0);
		Albedo.xyz *= Factors.xyz;
	}

	// Gamma correction
	float4 ResultAlbedo = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
	float4 Directional = CalculateDirectional(Albedo, Albedo, 1, 0,
		DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - input.WorldPosition), ResultNormal);
	// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
	float Dot = dot(DirectionalLightDirection, KUpDirection);
	Directional.xyz *= pow(Dot, 0.6f);

	ResultAlbedo.xyz += Directional.xyz;

	// For normal & tangent drawing
	if (input.bUseVertexColor != 0)
	{
		return input.Color;
	}

	return ResultAlbedo;
}