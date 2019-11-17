#include "Base.hlsli"
#include "Shared.hlsli"

cbuffer cbMaterial : register(b0)
{
	float3	MaterialAmbientColor;
	float	MaterialSpecularExponent;
	float3	MaterialDiffuseColor;
	float	MaterialSpecularIntensity;
	float3	MaterialSpecularColor;
	float	MaterialRoughness;

	float	MaterialMetalness;
	bool	bHasDiffuseTexture;
	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;

	bool	bHasSpecularIntensityTexture;
	bool	bHasRoughnessTexture;
	bool	bHasMetalnessTexture;
	bool	Reserved;
}

cbuffer cbEditorTime : register(b1)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Pad2;
}

cbuffer cbCameraSelection : register(b2)
{
	bool bIsSelected;
	float3 Pad3;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbientColor, 1);
	float4 DiffuseColor = float4(MaterialDiffuseColor, 1);
	float4 SpecularColor = float4(MaterialSpecularColor, 1);

	float4 OutputColor = DiffuseColor;
	if (bIsSelected == true)
	{
		OutputColor += OutputColor * sin(NormalizedTime * KPI);
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}