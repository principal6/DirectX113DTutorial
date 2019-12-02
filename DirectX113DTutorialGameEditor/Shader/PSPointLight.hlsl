#include "Deferred.hlsli"
#include "BRDF.hlsli"

SamplerState PointClampSampler : register(s0);

Texture2D GBuffer_DepthStencil : register(t0);
Texture2D GBuffer_BaseColor_Rough : register(t1);
Texture2D GBuffer_Normal : register(t2);
Texture2D GBuffer_Metal_AO : register(t3);

cbuffer cbGBufferUnpacking : register(b0)
{
	float4 PerspectiveValues;
	float4x4 InverseViewMatrix;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float ProjectionSpaceDepth = GBuffer_DepthStencil.SampleLevel(PointClampSampler, Input.TexCoord.xy, 0).x;
	float ViewSpaceDepth = PerspectiveValues.z / (ProjectionSpaceDepth + PerspectiveValues.w);
	float4 WorldPosition = mul(float4(Input.TexCoord.xy * PerspectiveValues.xy * ViewSpaceDepth, ViewSpaceDepth, 1.0), InverseViewMatrix);

	float4 BaseColor_Rough = GBuffer_BaseColor_Rough.SampleLevel(PointClampSampler, Input.TexCoord.xy, 0);
	float3 WorldNormal = (GBuffer_Normal.SampleLevel(PointClampSampler, Input.TexCoord.xy, 0).xyz) * 2.0 - 1.0;
	float4 Metal_AO = GBuffer_Metal_AO.SampleLevel(PointClampSampler, Input.TexCoord.xy, 0);

	float4 OutputColor = float4(BaseColor_Rough.xyz, 1);
	return OutputColor;
}