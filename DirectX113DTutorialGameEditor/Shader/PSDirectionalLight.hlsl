#include "Deferred.hlsli"
#include "BRDF.hlsli"
#include "iPSCBs.hlsli"

SamplerState PointClampSampler : register(s0);
SamplerState LinearWrapSampler : register(s1);
SamplerState LinearClampSampler : register(s2);

Texture2D GBuffer_DepthStencil : register(t0);
Texture2D GBuffer_BaseColor_Rough : register(t1);
Texture2D GBuffer_Normal : register(t2);
Texture2D GBuffer_Metal_AO : register(t3);
Texture2D ShadowMap0 : register(t4);
Texture2D ShadowMap1 : register(t5);
Texture2D ShadowMap2 : register(t6);
Texture2D ShadowMap3 : register(t7);
Texture2D ShadowMap4 : register(t8);

TextureCube EnvironmentTexture : register(t50);
TextureCube IrradianceTexture : register(t51);
TextureCube PrefilteredRadianceTexture : register(t52);
Texture2D IntegratedBRDFTexture : register(t53);

cbuffer cbGBufferUnpacking : register(b3)
{
	float4 PerspectiveValues;
	float4x4 InverseViewMatrix;
	float2 ScreenSize;
	float2 Reserved2;
}

cbuffer cbShadowMap : register(b4)
{
	float4x4	ShadowMapSpaceMatrix0;
	float4x4	ShadowMapSpaceMatrix1;
	float4x4	ShadowMapSpaceMatrix2;
	float4x4	ShadowMapSpaceMatrix3;
	float4x4	ShadowMapSpaceMatrix4;
	float4		ShadowMapZFars;
	uint		LODCount; // @important: [1, 5]
	float3		Reserved3;
}

#define UV Input.TexCoord.xy
#define N WorldNormal
#define BaseColor BaseColor_Rough.xyz
#define Roughness BaseColor_Rough.a
#define Metalness Metal_AO.x
#define AmbientOcclusion Metal_AO.y
#define EyePosition InverseViewMatrix[3].xyz
#define Li DirectionalLightColor

float CheckIfInShadow(uint ShadowMapLOD, float CmpDepth, float2 TexCoord)
{
	float IsInShadow = 0;
	float ShadowMapDepth = 0;
	if (ShadowMapLOD == 0) ShadowMapDepth = ShadowMap0.SampleLevel(LinearClampSampler, TexCoord, 0).x;
	else if (ShadowMapLOD == 1) ShadowMapDepth = ShadowMap1.SampleLevel(LinearClampSampler, TexCoord, 0).x;
	else if (ShadowMapLOD == 2) ShadowMapDepth = ShadowMap2.SampleLevel(LinearClampSampler, TexCoord, 0).x;
	else if (ShadowMapLOD == 3) ShadowMapDepth = ShadowMap3.SampleLevel(LinearClampSampler, TexCoord, 0).x;
	else if (ShadowMapLOD == 4) ShadowMapDepth = ShadowMap4.SampleLevel(LinearClampSampler, TexCoord, 0).x;

	if (CmpDepth > ShadowMapDepth + 0.001) IsInShadow = 1.0;
	return IsInShadow;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float ProjectionSpaceDepth = GBuffer_DepthStencil.SampleLevel(PointClampSampler, UV, 0).x;
	if (ProjectionSpaceDepth == 1.0) discard; // @important: early out
	float ViewSpaceDepth = PerspectiveValues.z / (ProjectionSpaceDepth + PerspectiveValues.w);
	float4 ObjectWorldPosition = mul(float4(Input.ScreenPosition * PerspectiveValues.xy * ViewSpaceDepth, ViewSpaceDepth, 1.0), InverseViewMatrix);

	float4 BaseColor_Rough = GBuffer_BaseColor_Rough.SampleLevel(PointClampSampler, UV, 0);
	float3 WorldNormal = normalize((GBuffer_Normal.SampleLevel(PointClampSampler, UV, 0).xyz * 2.0) - 1.0);
	float4 Metal_AO = GBuffer_Metal_AO.SampleLevel(PointClampSampler, UV, 0);

	float4 OutputColor = float4(0, 0, 0, 1);
	{
		float3 Wo = normalize(EyePosition - ObjectWorldPosition.xyz);
		float3 F0 = lerp(KFresnel_dielectric, BaseColor, Metalness);
		float NdotWo = max(dot(N, Wo), 0.001);

		// Shadow
		float IsInShadow = 0.0; // 0 for "not in shadow", 1 for "in shadow"
		if (ViewSpaceDepth <= ShadowMapZFars[LODCount - 1])
		{
			uint ShadowMapLOD = 0;
			if (ViewSpaceDepth > ShadowMapZFars.w) ShadowMapLOD = 4;
			else if (ViewSpaceDepth > ShadowMapZFars.z) ShadowMapLOD = 3;
			else if (ViewSpaceDepth > ShadowMapZFars.y) ShadowMapLOD = 2;
			else if (ViewSpaceDepth > ShadowMapZFars.x) ShadowMapLOD = 1;

			float4 LightSpacePosition = float4(0, 0, 0, 1);
			if (ShadowMapLOD == 0) LightSpacePosition = mul(ObjectWorldPosition, ShadowMapSpaceMatrix0);
			else if (ShadowMapLOD == 1) LightSpacePosition = mul(ObjectWorldPosition, ShadowMapSpaceMatrix1);
			else if (ShadowMapLOD == 2) LightSpacePosition = mul(ObjectWorldPosition, ShadowMapSpaceMatrix2);
			else if (ShadowMapLOD == 3) LightSpacePosition = mul(ObjectWorldPosition, ShadowMapSpaceMatrix3);
			else if (ShadowMapLOD == 4) LightSpacePosition = mul(ObjectWorldPosition, ShadowMapSpaceMatrix4);
			LightSpacePosition /= LightSpacePosition.w;

			float2 ShadowMapSize = float2(0, 0);
			ShadowMap0.GetDimensions(ShadowMapSize.x, ShadowMapSize.y);

			float2 ShadowMapUV = float2(LightSpacePosition.x * 0.5 + 0.5, -LightSpacePosition.y * 0.5 + 0.5);
			
			// 4x4 PCF
			float2 dUV = float2(1.0, 1.0) / ShadowMapSize;
			for (float y = -1.5; y <= 1.5; y += 1.0)
			{
				for (float x = -1.5; x <= 1.5; x += 1.0)
				{
					IsInShadow += CheckIfInShadow(ShadowMapLOD, LightSpacePosition.z, ShadowMapUV + float2(dUV.x * x, dUV.y * y));
				}
			}
			IsInShadow /= 16.0;
		}
		
		// Direct light
		float3 DirectLight = float3(0, 0, 0);
		if (IsInShadow < 1.0)
		{
			float3 Wi = DirectionalLightDirection.xyz;
			float3 M = normalize(Wi + Wo);

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

			DirectLight = (Lo_diff + Lo_spec);
		}

		// Indirect light
		float3 IndirectLight = float3(0, 0, 0);
		float3 Ei = IrradianceTexture.SampleBias(LinearWrapSampler, N, Roughness * (float)(IrradianceTextureMipLevels - 1)).rgb;
		if (Metalness == 0.0)
		{
			IndirectLight = Ei * BaseColor * K1DIVPI;
		}
		else
		{
			float3 Wi = normalize(-Wo + (2.0 * NdotWo * N));
			float NdotWi = dot(N, Wi);

			float3 PrefilteredRadiance = PrefilteredRadianceTexture.SampleLevel(LinearWrapSampler, Wi,
				Roughness * (float)(PrefilteredRadianceTextureMipLevels - 1)).rgb;
			float2 IntegratedBRDF = IntegratedBRDFTexture.SampleLevel(LinearClampSampler, float2(NdotWo, 1 - Roughness), 0).rg;

			float3 F_Macrosurface = Fresnel_Schlick(F0, NdotWi);
			float Ks = dot(KMonochromatic, F_Macrosurface);
			float Kd = 1.0 - Ks;

			float3 Lo_diff = Kd * Ei * BaseColor * K1DIVPI;
			float3 Lo_spec = Ks * PrefilteredRadiance * (BaseColor * IntegratedBRDF.x + IntegratedBRDF.y);

			IndirectLight = (Lo_diff + Lo_spec) * AmbientOcclusion;
		}

		OutputColor.xyz = DirectLight * (1 - IsInShadow) + IndirectLight; // @important
	}
	OutputColor = pow(OutputColor, 0.4545);

	return OutputColor;
}

float4 NonIBL(VS_OUTPUT Input) : SV_TARGET
{
	float ProjectionSpaceDepth = GBuffer_DepthStencil.SampleLevel(PointClampSampler, UV, 0).x;
	if (ProjectionSpaceDepth == 1.0) discard; // @important: early out
	float ViewSpaceDepth = PerspectiveValues.z / (ProjectionSpaceDepth + PerspectiveValues.w);
	float4 ObjectWorldPosition = mul(float4(Input.ScreenPosition * PerspectiveValues.xy * ViewSpaceDepth, ViewSpaceDepth, 1.0), InverseViewMatrix);

	float4 BaseColor_Rough = GBuffer_BaseColor_Rough.SampleLevel(PointClampSampler, UV, 0);
	float3 WorldNormal = normalize((GBuffer_Normal.SampleLevel(PointClampSampler, UV, 0).xyz * 2.0) - 1.0);
	float4 Metal_AO = GBuffer_Metal_AO.SampleLevel(PointClampSampler, UV, 0);

	float4 OutputColor = float4(0, 0, 0, 1);

	{
		float3 Wo = normalize(EyePosition - ObjectWorldPosition.xyz);
		float3 F0 = lerp(KFresnel_dielectric, BaseColor, Metalness);
		float NdotWo = max(dot(N, Wo), 0.001);

		// Direct light
		{
			float3 Wi = DirectionalLightDirection.xyz;
			float3 M = normalize(Wi + Wo);

			float NdotWi = max(dot(N, Wi), 0.001);
			float NdotM = saturate(dot(N, M));
			float MdotWi = saturate(dot(M, Wi));

			float3 DiffuseBRDF = DiffuseBRDF_Lambertian(BaseColor);
			float3 SpecularBRDF = SpecularBRDF_GGX(F0, NdotWi, NdotWo, NdotM, MdotWi, Roughness);

			float3 F_Macrosurface = Fresnel_Schlick(F0, NdotWi);
			float Ks = dot(KMonochromatic, F_Macrosurface);
			float Kd = 1.0 - Ks;

			OutputColor.xyz += (Kd * DiffuseBRDF + Ks * SpecularBRDF) * Li * NdotWi;
		}

		// Indirect light
		OutputColor.xyz += CalculateHemisphericalAmbient(BaseColor, N, AmbientLightColor, AmbientLightColor * 0.5, AmbientLightIntensity);
	}
	OutputColor = pow(OutputColor, 0.4545);

	return OutputColor;
}