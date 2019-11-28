#include "Foliage.hlsli"
#include "BRDF.hlsli"

#define FLAG_ID_DIFFUSE 0x01
#define FLAG_ID_NORMAL 0x02
#define FLAG_ID_OPACITY 0x04
#define FLAG_ID_SPECULARINTENSITY 0x08
#define FLAG_ID_ROUGHNESS 0x10
#define FLAG_ID_METALNESS 0x20
#define FLAG_ID_AMBIENTOCCLUSION 0x40

SamplerState CurrentSampler : register(s0);

Texture2D DiffuseTexture : register(t0);
// Normal texture slot
Texture2D OpacityTexture : register(t2);
// Specular intensity texture slot
Texture2D RoughnessTexture : register(t4);
// Metalness texture slot
// Ambient occlusion texture slot
// Displacement texture slot

cbuffer cbFlags : register(b0)
{
	bool bUseTexture;
	bool bUseLighting;
	bool bUsePhysicallyBasedRendering;
	uint EnvironmentTextureMipLevels;

	uint PrefilteredRadianceTextureMipLevels;
	float3 Pads;
}

cbuffer cbLight : register(b1)
{
	float4	DirectionalLightDirection;
	float3	DirectionalLightColor;
	float	Exposure;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

cbuffer cbMaterial : register(b2)
{
	float3	MaterialAmbientColor; // Classical
	float	MaterialSpecularExponent; // Classical
	float3	MaterialDiffuseColor;
	float	MaterialSpecularIntensity; // Classical
	float3	MaterialSpecularColor; // Classical
	float	MaterialRoughness;

	float	MaterialMetalness;
	uint	FlagsHasTexture;
	uint	FlagsIsTextureSRGB;
	uint	TotalMaterialCount; // for Terrain this is texture layer count
}

float4 main(VS_FOLIAGE_OUTPUT Input) : SV_TARGET
{
	float4 DiffuseColor = float4(1, 1, 1, 1);
	float Opacity = 1.0f;

	if (bUseTexture == true)
	{
		if (FlagsHasTexture & FLAG_ID_DIFFUSE)
		{
			DiffuseColor = DiffuseTexture.Sample(CurrentSampler, Input.TexCoord.xy);

			// # Here we make sure that input RGB values are in linear-space!
			if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
			{
				// # Convert gamma-space RGB to linear-space RGB
				DiffuseColor = pow(DiffuseColor, 2.2);
			}
		}

		if (FlagsHasTexture & FLAG_ID_OPACITY)
		{
			float4 Sampled = OpacityTexture.Sample(CurrentSampler, Input.TexCoord.xy);
			if (Sampled.r == Sampled.g && Sampled.g == Sampled.b)
			{
				Opacity = Sampled.r;
			}
			else
			{
				Opacity = Sampled.a;
			}
		}
	}

	float4 OutputColor = DiffuseColor;
	if (bUseLighting == true)
	{
		float3 N = normalize(Input.WorldNormal).xyz;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		float3 Ambient = CalculateClassicalAmbient(DiffuseColor.xyz, AmbientLightColor, AmbientLightIntensity);
		float3 Directional = CalculateClassicalDirectional(DiffuseColor.xyz, DiffuseColor.xyz, 1.0f, 0.0f,
			DirectionalLightColor, L, V, N);

		// Directional Light�� ��ġ�� ���򼱿� ����������� ���� ���⸦ ���ϰ� �Ѵ�.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		//OutputColor.xyz = Ambient + Directional;
		OutputColor.xyz = Directional;
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	if (FlagsHasTexture & FLAG_ID_OPACITY) OutputColor.a *= Opacity;

	return OutputColor;
}