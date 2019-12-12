#include "Base.hlsli"
#include "Shared.hlsli"
#include "iPSCBs.hlsli"

cbuffer cbEditorTime : register(b3)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Reserved2;
}

cbuffer cbCameraInfo : register(b4)
{
	uint CurrentCameraID;
	float3 Pads;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
#ifndef DEBUG_SHADER
	if (Input.InstanceID == CurrentCameraID) discard;
#endif

	float4 AmbientColor = float4(MaterialAmbientColor, 1);
	float4 DiffuseColor = float4(MaterialDiffuseColor, 1);
	float4 SpecularColor = float4(MaterialSpecularColor, 1);

	float4 OutputColor = DiffuseColor;

	if (Input.IsHighlighted != 0.0)
	{
		OutputColor += float4(0.6, 0.3, 0.6, 0) * sin(NormalizedTime * KPI);
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	//OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}