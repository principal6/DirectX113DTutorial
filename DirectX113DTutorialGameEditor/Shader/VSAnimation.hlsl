#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
}

cbuffer cbBones : register(b1)
{
	float4x4 BoneMatrices[KBoneMatrixMaxCount];
}

VS_OUTPUT main(VS_INPUT_ANIMATION input)
{
	VS_OUTPUT output;

	float4x4 FinalBone = KMatrixIdentity;
	FinalBone = BoneMatrices[input.BoneIndex.x] * input.BoneWeight.x;
	FinalBone += BoneMatrices[input.BoneIndex.y] * input.BoneWeight.y;
	FinalBone += BoneMatrices[input.BoneIndex.z] * input.BoneWeight.z;
	FinalBone += BoneMatrices[input.BoneIndex.w] * input.BoneWeight.w;

	float4 ResultPosition = float4(mul(input.Position, FinalBone).xyz, 1);
	float4 ResultNormal = normalize(mul(input.Normal, FinalBone));
	float4 ResultTangent = normalize(mul(input.Tangent, FinalBone));

	output.WorldPosition = mul(ResultPosition, World);
	output.Position = mul(output.WorldPosition, ViewProjection);
	
	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = normalize(mul(ResultNormal, World));
	output.WorldTangent = normalize(mul(ResultTangent, World));
	output.WorldBitangent = float4(normalize(cross(output.WorldNormal.xyz, output.WorldTangent.xyz)), 0);

	output.bUseVertexColor = 0;

	return output;
}