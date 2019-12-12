// Shared constant buffers for HS

cbuffer cbSpace : register(b0)
{
	float4x4	World;
	float4x4	View;
	float4x4	Projection;
	float4x4	ViewProjection;
	float4x4	WVP;
	float4		EyePosition;
}

cbuffer cbGlobalLight : register(b1)
{
	float4x4	DirectionalLightSpaceMatrix;
	float4		DirectionalLightDirection;
	float3		DirectionalLightColor;
	float		Exposure;
	float3		AmbientLightColor;
	float		AmbientLightIntensity;
	uint		IrradianceTextureMipLevels;
	uint		PrefilteredRadianceTextureMipLevels;
	float2		Reserved;
}