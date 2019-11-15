#include "Terrain.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);

cbuffer cbTime : register(b0)
{
	float Time;
	float3 Pads;
}

cbuffer cbLights : register(b1)
{
	float4	DirectionalLightDirection;
	float4	DirectionalLightColor;
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
		float4 Ambient = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(Albedo, float4(1, 1, 1, 1), 32.0, 0.1,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - Input.WorldPosition), normalize(ResultNormal));

		OutputColor = Ambient + Directional;
	}
	OutputColor.a = Input.Color.a;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	return OutputColor;
}