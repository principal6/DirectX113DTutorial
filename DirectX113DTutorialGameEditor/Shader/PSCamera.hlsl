#include "Base.hlsli"

cbuffer cbMaterial : register(b0)
{
	float3	MaterialAmbient;
	float	SpecularExponent;
	float3	MaterialDiffuse;
	float	SpecularIntensity;
	float3	MaterialSpecular;
	bool	bHasDiffuseTexture;

	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;
	bool2	Pads2;
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

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbient, 1);
	float4 DiffuseColor = float4(MaterialDiffuse, 1);
	float4 SpecularColor = float4(MaterialSpecular, 1);

	// # Gamma correction
	DiffuseColor.xyz = pow(DiffuseColor.xyz, 2.0f);
	AmbientColor.xyz = pow(AmbientColor.xyz, 2.0f);
	SpecularColor.xyz = pow(SpecularColor.xyz, 2.0f);

	float4 ResultColor = DiffuseColor;
	if (bIsSelected == true)
	{
		ResultColor += ResultColor * sin(NormalizedTime * KPI);
	}

	// # Gamma correction
	ResultColor.xyz = pow(ResultColor.xyz, 0.5f);

	return ResultColor;
}