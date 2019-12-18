// Shared constant buffers for PS

cbuffer cbMaterial : register(b0)
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

cbuffer cbGlobalLight : register(b1)
{
	float4		DirectionalLightDirection;
	float3		DirectionalLightColor;
	float		Exposure;
	float3		AmbientLightColor;
	float		AmbientLightIntensity;
	uint		IrradianceTextureMipLevels;
	uint		PrefilteredRadianceTextureMipLevels;
	float2		Reserved;
}

cbuffer cbSpace : register(b2)
{
	float4x4	World;
	float4x4	View;
	float4x4	Projection;
	float4x4	ViewProjection;
	float4x4	WVP;
	float4		EyePosition;
}