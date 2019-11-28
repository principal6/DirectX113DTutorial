#include "Base.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
	float4x4 WVP;
}

cbuffer cbBones : register(b1)
{
	float4x4 BoneMatrices[KBoneMatrixMaxCount]; // For CPU skinning
}

cbuffer cbAnimation : register(b2)
{
	bool bUseGPUSkinning;
	int AnimationID;
	int AnimtaionDuration;
	float AnimationTick;
}

Texture2D<float4> AnimationTexture : register(t0); // For GPU skinning

float4x4 GetBoneMatrixFromAnimationTexture(int BoneIndex, int AnimationOffset)
{
	float4 Row0 = AnimationTexture[int2(BoneIndex * 4 + 0, AnimationOffset)];
	float4 Row1 = AnimationTexture[int2(BoneIndex * 4 + 1, AnimationOffset)];
	float4 Row2 = AnimationTexture[int2(BoneIndex * 4 + 2, AnimationOffset)];
	float4 Row3 = AnimationTexture[int2(BoneIndex * 4 + 3, AnimationOffset)];
	return float4x4(Row0, Row1, Row2, Row3);
}

VS_OUTPUT main(VS_INPUT_ANIMATION Input)
{
	VS_OUTPUT Output;

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

		float4x4 CurrentTickBone = GetBoneMatrixFromAnimationTexture(Input.BoneIndex.x, AnimationOffset + iTickCurrent) * Input.BoneWeight.x;
		CurrentTickBone += GetBoneMatrixFromAnimationTexture(Input.BoneIndex.y, AnimationOffset + iTickCurrent) * Input.BoneWeight.y;
		CurrentTickBone += GetBoneMatrixFromAnimationTexture(Input.BoneIndex.z, AnimationOffset + iTickCurrent) * Input.BoneWeight.z;
		CurrentTickBone += GetBoneMatrixFromAnimationTexture(Input.BoneIndex.w, AnimationOffset + iTickCurrent) * Input.BoneWeight.w;

		float4x4 NextTickBone = GetBoneMatrixFromAnimationTexture(Input.BoneIndex.x, AnimationOffset + iTickNext) * Input.BoneWeight.x;
		NextTickBone += GetBoneMatrixFromAnimationTexture(Input.BoneIndex.y, AnimationOffset + iTickNext) * Input.BoneWeight.y;
		NextTickBone += GetBoneMatrixFromAnimationTexture(Input.BoneIndex.z, AnimationOffset + iTickNext) * Input.BoneWeight.z;
		NextTickBone += GetBoneMatrixFromAnimationTexture(Input.BoneIndex.w, AnimationOffset + iTickNext) * Input.BoneWeight.w;

		float4 CurrentTickPosition = float4(mul(Input.Position, CurrentTickBone).xyz, 1);
		float4 NextTickPosition = float4(mul(Input.Position, NextTickBone).xyz, 1);

		float t = AnimationTick - (float)iTickCurrent;
		ResultPosition = lerp(CurrentTickPosition, NextTickPosition, t);
	}
	else
	{
		FinalBone = BoneMatrices[Input.BoneIndex.x] * Input.BoneWeight.x;
		FinalBone += BoneMatrices[Input.BoneIndex.y] * Input.BoneWeight.y;
		FinalBone += BoneMatrices[Input.BoneIndex.z] * Input.BoneWeight.z;
		FinalBone += BoneMatrices[Input.BoneIndex.w] * Input.BoneWeight.w;

		ResultPosition = float4(mul(Input.Position, FinalBone).xyz, 1);
	}
	
	float4 ResultNormal = normalize(mul(Input.Normal, FinalBone));
	float4 ResultTangent = normalize(mul(Input.Tangent, FinalBone));

	Output.WorldPosition = mul(ResultPosition, World);
	Output.WorldPosition.w = 1.0f;
	Output.Position = mul(Output.WorldPosition, ViewProjection);
	
	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	Output.WorldNormal = normalize(mul(ResultNormal, World));
	Output.WorldTangent = normalize(mul(ResultTangent, World));
	Output.WorldBitangent = float4(normalize(cross(Output.WorldNormal.xyz, Output.WorldTangent.xyz)), 0);

	Output.bUseVertexColor = 0;
	Output.InstanceID = Input.InstanceID;

	return Output;
}