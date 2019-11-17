#include "Terrain.hlsli"
#include "BRDF.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D OpacityTexture : register(t2);
Texture2D SpecularIntensityTexture : register(t3);
// Displacement texture slot

cbuffer cbTime : register(b0)
{
	float Time;
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

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float2 AnimatedUV = Input.UV.xy - float2(0, Time); // UV animation
	
	float4 ResultNormal = normalize((NormalTexture.SampleLevel(CurrentSampler, AnimatedUV, 0) * 2.0f) - 1.0f);
	float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));

	//float4 Albedo = Input.Color;
	float4 Albedo = DiffuseTexture.SampleLevel(CurrentSampler, AnimatedUV, 0);

	// # Here we make sure that input RGB values are in linear-space!
	// (In this project, all textures are loaded with their values in gamma-space)
	// # Convert gamma-space RGB to linear-space RGB (sRGB)
	Albedo.xyz = pow(Albedo.xyz, 2.2);

	float4 OutputColor = Albedo;
	{
		float3 N = normalize(ResultNormal).xyz;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		float3 Ambient = CalculateClassicalAmbient(Albedo.xyz, AmbientLightColor, AmbientLightIntensity);
		float3 Directional = CalculateClassicalDirectional(Albedo.xyz, float3(1, 1, 1), 32.0, 0.1,
			DirectionalLightColor, L, V, N);

		OutputColor.xyz = Ambient + Directional;
	}
	OutputColor.a = Input.Color.a;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}