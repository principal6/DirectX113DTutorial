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

cbuffer cbAnimation : register(b2)
{
	bool bUseGPUSkinning;
	int AnimationID;
	int AnimtaionDuration;
	float AnimationTick;
}

Texture2D<float4> AnimationTexture : register(t0);

float4x4 GetBoneMatrixFromAnimationTexture(int BoneIndex, int AnimationOffset)
{
	float4 Row0 = AnimationTexture[int2(BoneIndex * 4 + 0, AnimationOffset)];
	float4 Row1 = AnimationTexture[int2(BoneIndex * 4 + 1, AnimationOffset)];
	float4 Row2 = AnimationTexture[int2(BoneIndex * 4 + 2, AnimationOffset)];
	float4 Row3 = AnimationTexture[int2(BoneIndex * 4 + 3, AnimationOffset)];
	return float4x4(Row0, Row1, Row2, Row3);
}

VS_OUTPUT main(VS_INPUT_ANIMATION input)
{
	VS_OUTPUT output;

	float4x4 FinalBone = KMatrixIdentity;
	float4 ResultPosition;
	if (bUseGPUSkinning == true)
	{
		const int KAnimationTextureReservedFirstPixelCount = 2;
		const int KAnimationCount = AnimationTexture[int2(1, 0)].x;
		const float4 KAnimationInfo = AnimationTexture[int2(KAnimationTextureReservedFirstPixelCount + AnimationID * 2 + 0, 0)];
		int AnimationOffset = KAnimationInfo.x;
		int AnimationDuration = KAnimationInfo.y;

		int iTickCurrent = (int)AnimationTick;
		int iTickNext = (int)AnimationTick + 1;
		if (iTickNext > AnimationDuration) iTickNext = 0;

		float4x4 CurrentTickBone = GetBoneMatrixFromAnimationTexture(input.BoneIndex.x, AnimationOffset + iTickCurrent) * input.BoneWeight.x;
		CurrentTickBone += GetBoneMatrixFromAnimationTexture(input.BoneIndex.y, AnimationOffset + iTickCurrent) * input.BoneWeight.y;
		CurrentTickBone += GetBoneMatrixFromAnimationTexture(input.BoneIndex.z, AnimationOffset + iTickCurrent) * input.BoneWeight.z;
		CurrentTickBone += GetBoneMatrixFromAnimationTexture(input.BoneIndex.w, AnimationOffset + iTickCurrent) * input.BoneWeight.w;

		float4x4 NextTickBone = GetBoneMatrixFromAnimationTexture(input.BoneIndex.x, AnimationOffset + iTickNext) * input.BoneWeight.x;
		NextTickBone += GetBoneMatrixFromAnimationTexture(input.BoneIndex.y, AnimationOffset + iTickNext) * input.BoneWeight.y;
		NextTickBone += GetBoneMatrixFromAnimationTexture(input.BoneIndex.z, AnimationOffset + iTickNext) * input.BoneWeight.z;
		NextTickBone += GetBoneMatrixFromAnimationTexture(input.BoneIndex.w, AnimationOffset + iTickNext) * input.BoneWeight.w;

		float4 CurrentTickPosition = float4(mul(input.Position, CurrentTickBone).xyz, 1);
		float4 NextTickPosition = float4(mul(input.Position, NextTickBone).xyz, 1);

		float t = AnimationTick - (float)iTickCurrent;
		ResultPosition = lerp(CurrentTickPosition, NextTickPosition, t);
	}
	else
	{
		FinalBone = BoneMatrices[input.BoneIndex.x] * input.BoneWeight.x;
		FinalBone += BoneMatrices[input.BoneIndex.y] * input.BoneWeight.y;
		FinalBone += BoneMatrices[input.BoneIndex.z] * input.BoneWeight.z;
		FinalBone += BoneMatrices[input.BoneIndex.w] * input.BoneWeight.w;

		ResultPosition = float4(mul(input.Position, FinalBone).xyz, 1);
	}
	
	float4 ResultNormal = normalize(mul(input.Normal, FinalBone));
	float4 ResultTangent = normalize(mul(input.Tangent, FinalBone));

	output.WorldPosition = mul(ResultPosition, World);
	output.WorldPosition.w = 1.0f;
	output.Position = mul(output.WorldPosition, ViewProjection);
	
	output.Color = input.Color;
	output.UV = input.UV;

	output.WorldNormal = normalize(mul(ResultNormal, World));
	output.WorldTangent = normalize(mul(ResultTangent, World));
	output.WorldBitangent = float4(normalize(cross(output.WorldNormal.xyz, output.WorldTangent.xyz)), 0);

	output.bUseVertexColor = 0;

	return output;
}