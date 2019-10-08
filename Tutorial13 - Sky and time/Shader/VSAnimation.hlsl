#include "Header.hlsli"

#define KMaxBoneMatrixCount 60

cbuffer cbSpace : register(b0)
{
	float4x4 WVP;
	float4x4 World;
}

cbuffer cbBones : register(b1)
{
	float4x4 BoneMatrices[KMaxBoneMatrixCount];
}

VS_OUTPUT main(VS_INPUT_ANIMATION input)
{
	VS_OUTPUT output;

	float4x4 FinalBone = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	
	FinalBone = BoneMatrices[input.BoneIndex.x] * input.BoneWeight.x;
	FinalBone += BoneMatrices[input.BoneIndex.y] * input.BoneWeight.y;
	FinalBone += BoneMatrices[input.BoneIndex.z] * input.BoneWeight.z;
	FinalBone += BoneMatrices[input.BoneIndex.w] * input.BoneWeight.w;

	float4 ResultPosition = float4(mul(input.Position, FinalBone).xyz, 1);
	float4 ResultNormal = normalize(mul(input.Normal, FinalBone));

	output.Position = mul(ResultPosition, WVP);
	output.WorldPosition = mul(ResultPosition, World);
	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = normalize(mul(ResultNormal, World));
	output.WVPNormal = normalize(mul(ResultNormal, WVP));

	return output;
}