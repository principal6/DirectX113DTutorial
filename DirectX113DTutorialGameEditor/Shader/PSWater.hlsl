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
	//float4 Albedo = Input.Color;
	float2 AnimatedUV = Input.UV.xy - float2(0, Time); // UV animation
	float4 Albedo = DiffuseTexture.SampleLevel(CurrentSampler, AnimatedUV, 0);
	float4 ResultNormal = normalize((NormalTexture.SampleLevel(CurrentSampler, AnimatedUV, 0) * 2.0f) - 1.0f);
	float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));

	// # Gamma correction
	Albedo.xyz = pow(Albedo.xyz, 2.0f);

	float4 ResultColor = Albedo;
	{
		float4 Ambient = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(Albedo, float4(1, 1, 1, 1), 32.0, 0.1,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - Input.WorldPosition), normalize(ResultNormal));

		ResultColor = Ambient + Directional;
	}

	// # Gamma correction
	ResultColor.xyz = pow(ResultColor.xyz, 0.5f);
	ResultColor.a = Input.Color.a;

	return ResultColor;
}