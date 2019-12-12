#include "Terrain.hlsli"
#include "BRDF.hlsli"
#include "iPSCBs.hlsli"

SamplerState CurrentSampler : register(s0);

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D OpacityTexture : register(t2);
Texture2D SpecularIntensityTexture : register(t3);
Texture2D RoughnessTexture : register(t4);
Texture2D MetalnessTexture : register(t5);
Texture2D AmbientOcclusionTexture : register(t6);
// Displacement texture slot

cbuffer cbTime : register(b3)
{
	float Time;
	float3 Pads;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float2 AnimatedTexCoord = Input.TexCoord.xy - float2(0, Time); // TexCoord animation
	
	float4 ResultNormal = normalize((NormalTexture.SampleLevel(CurrentSampler, AnimatedTexCoord, 0) * 2.0f) - 1.0f);
	float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));

	//float4 DiffuseColor = Input.Color;
	float4 DiffuseColor = DiffuseTexture.SampleLevel(CurrentSampler, AnimatedTexCoord, 0);

	// # Here we make sure that input RGB values are in linear-space!
	// # Convert gamma-space RGB to linear-space RGB
	DiffuseColor.xyz = pow(DiffuseColor.xyz, 2.2);

	float4 OutputColor = DiffuseColor;
	{
		float3 N = normalize(ResultNormal).xyz;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		float3 Ambient = CalculateClassicalAmbient(DiffuseColor.xyz, AmbientLightColor, AmbientLightIntensity);
		float3 Directional = CalculateClassicalDirectional(DiffuseColor.xyz, float3(1, 1, 1), 32.0, 0.1,
			DirectionalLightColor, L, V, N);

		OutputColor.xyz = Ambient + Directional;
	}
	OutputColor.a = Input.Color.a;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}