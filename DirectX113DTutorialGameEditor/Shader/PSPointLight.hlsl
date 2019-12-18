#include "DeferredLight.hlsli"
#include "BRDF.hlsli"

cbuffer cbGBufferUnpacking : register(b3)
{
	float4 PerspectiveValues;
	float4x4 InverseViewMatrix;
	float2 ScreenSize;
	float2 Reserved;
}

SamplerState PointClampSampler : register(s0);

Texture2D GBuffer_DepthStencil : register(t0);
Texture2D GBuffer_BaseColor_Rough : register(t1);
Texture2D GBuffer_Normal : register(t2);
Texture2D GBuffer_Metal_AO : register(t3);

#define N WorldNormal
#define BaseColor BaseColor_Rough.xyz
#define Roughness BaseColor_Rough.a
#define Metalness Metal_AO.x
#define AmbientOcclusion Metal_AO.y
#define EyePosition InverseViewMatrix[3].xyz

float4 main(DS_POINT_LIGHT_OUTPUT Input) : SV_TARGET
{
	float2 ScreenPosition = float2(Input.Position.x / ScreenSize.x, 1.0 - (Input.Position.y / ScreenSize.y)) * 2.0 - 1.0;
	float2 UV = float2(ScreenPosition.x * 0.5 + 0.5, 0.5 - ScreenPosition.y * 0.5);

	float ObjectProjectionDepth = GBuffer_DepthStencil.SampleLevel(PointClampSampler, UV, 0).x;
	if (ObjectProjectionDepth == 1.0) discard; // @important: early out
	float ObjectViewDepth = PerspectiveValues.z / (ObjectProjectionDepth + PerspectiveValues.w);
	float4 ObjectWorldPosition = mul(float4(ScreenPosition * PerspectiveValues.xy * ObjectViewDepth, ObjectViewDepth, 1.0), InverseViewMatrix);

	float4 BaseColor_Rough = GBuffer_BaseColor_Rough.SampleLevel(PointClampSampler, UV, 0);
	float3 WorldNormal = normalize((GBuffer_Normal.SampleLevel(PointClampSampler, UV, 0).xyz * 2.0) - 1.0);
	float4 Metal_AO = GBuffer_Metal_AO.SampleLevel(PointClampSampler, UV, 0);

	float4 OutputColor = float4(0, 0, 0, 1);
	{
		float4 LightWorldPosition = Input.WorldPosition;
		float3 Li = Input.Color.rgb;

		float3 ToLight = LightWorldPosition.xyz - ObjectWorldPosition.xyz;
		float DistanceToLight = length(ToLight);

		float3 Wi = normalize(LightWorldPosition.xyz - ObjectWorldPosition.xyz);
		float3 Wo = normalize(EyePosition - ObjectWorldPosition.xyz);
		float3 M = normalize(Wi + Wo);
		float3 F0 = lerp(KFresnel_dielectric, BaseColor, Metalness);
		float NdotWo = max(dot(N, Wo), 0.001);
		float NdotWi = max(dot(N, Wi), 0.001);
		float NdotM = saturate(dot(N, M));
		float MdotWi = saturate(dot(M, Wi));

		float3 DiffuseBRDF = DiffuseBRDF_Lambertian(BaseColor);
		float3 SpecularBRDF = SpecularBRDF_GGX(F0, NdotWi, NdotWo, NdotM, MdotWi, Roughness);

		float3 F_Macrosurface = Fresnel_Schlick(F0, NdotWi);
		float Ks = dot(KMonochromatic, F_Macrosurface);
		float Kd = 1.0 - Ks;

		float3 Lo_diff = (Kd * DiffuseBRDF) * Li * NdotWi;
		float3 Lo_spec = (Ks * SpecularBRDF) * Li * NdotWi;

		float3 Lo = Lo_diff + Lo_spec;

		float DistanceAttenuation = saturate(1.0 - DistanceToLight * Input.InverseRange);
		DistanceAttenuation *= DistanceAttenuation;

		OutputColor.xyz = Lo * DistanceAttenuation;
	}
	OutputColor = pow(OutputColor, 0.4545);
	return OutputColor;
}

float4 Volume(DS_POINT_LIGHT_OUTPUT Input) : SV_TARGET
{
	return float4(1.0, 0.5, 0.0, 1.0);
}