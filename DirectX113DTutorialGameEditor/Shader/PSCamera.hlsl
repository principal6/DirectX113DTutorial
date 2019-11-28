#include "Base.hlsli"
#include "Shared.hlsli"

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

cbuffer cbEditorTime : register(b1)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Pad2;
}

cbuffer cbCameraSelection : register(b2)
{
	uint SelectedCameraID;
	uint CurrentCameraID;
	float2 Pads;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	if (Input.InstanceID == CurrentCameraID) discard;

	float4 AmbientColor = float4(MaterialAmbientColor, 1);
	float4 DiffuseColor = float4(MaterialDiffuseColor, 1);
	float4 SpecularColor = float4(MaterialSpecularColor, 1);

	float4 OutputColor = DiffuseColor;
	if (Input.InstanceID == SelectedCameraID)
	{
		OutputColor += float4(0.6, 0.3, 0.6, 0) * sin(NormalizedTime * KPI);
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	//OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}