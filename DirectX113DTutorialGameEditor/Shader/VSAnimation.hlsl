#include "Base.hlsli"
#include "iVSCBs.hlsli"

cbuffer cbBones : register(b1)
{
	float4x4 BoneMatrices[KBoneMatrixMaxCount]; // For CPU skinning
}

cbuffer cbAnimation : register(b2)
{
	bool bUseGPUSkinning;
	bool bIsInstanced;
	uint g_AnimationID;
	float g_AnimationTick;
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

static VS_OUTPUT Internal(in float4x4 WorldMatrix, in VS_INPUT_ANIMATION Input)
{
	VS_OUTPUT Output;

	Output.WorldPosition = mul(Input.Position, WorldMatrix);
	Output.WorldPosition.w = 1.0;
	Output.Position = mul(Output.WorldPosition, ViewProjection);

	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	Output.WorldNormal = normalize(mul(Input.Normal, WorldMatrix));
	Output.WorldTangent = normalize(mul(Input.Tangent, WorldMatrix));
	Output.WorldBitangent = float4(normalize(cross(Output.WorldNormal.xyz, Output.WorldTangent.xyz)), 0);

	Output.bUseVertexColor = 0;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif
	Output.IsHighlighted = 0;

	return Output;
}

static float4 GetGPUSkinnedPosition(float4 VertexPosition, uint AnimationID, float AnimationTick, uint4 BoneIndex, float4 BoneWeight)
{
	static const int KAnimationTextureReservedFirstPixelCount = 2;
	const int KAnimationCount = AnimationTexture[int2(1, 0)].x;
	const float4 KAnimationInfo = AnimationTexture[int2(KAnimationTextureReservedFirstPixelCount + AnimationID * 2 + 0, 0)];
	int AnimationOffset = KAnimationInfo.x;
	int AnimationDuration = KAnimationInfo.y;

	int iCurrTick = (int)AnimationTick;
	int iNextTick = (int)AnimationTick + 1;
	if (iNextTick > AnimationDuration) iNextTick = 0;

	float4x4 CurrTickBone = 
		GetBoneMatrixFromAnimationTexture(BoneIndex.x, AnimationOffset + iCurrTick) * BoneWeight.x +
		GetBoneMatrixFromAnimationTexture(BoneIndex.y, AnimationOffset + iCurrTick) * BoneWeight.y +
		GetBoneMatrixFromAnimationTexture(BoneIndex.z, AnimationOffset + iCurrTick) * BoneWeight.z +
		GetBoneMatrixFromAnimationTexture(BoneIndex.w, AnimationOffset + iCurrTick) * BoneWeight.w;

	float4x4 NextTickBone = 
		GetBoneMatrixFromAnimationTexture(BoneIndex.x, AnimationOffset + iNextTick) * BoneWeight.x +
		GetBoneMatrixFromAnimationTexture(BoneIndex.y, AnimationOffset + iNextTick) * BoneWeight.y +
		GetBoneMatrixFromAnimationTexture(BoneIndex.z, AnimationOffset + iNextTick) * BoneWeight.z +
		GetBoneMatrixFromAnimationTexture(BoneIndex.w, AnimationOffset + iNextTick) * BoneWeight.w;

	float4 CurrTickPosition = float4(mul(VertexPosition, CurrTickBone).xyz, 1);
	float4 NextTickPosition = float4(mul(VertexPosition, NextTickBone).xyz, 1);

	float t = AnimationTick - (float)iCurrTick;
	return lerp(CurrTickPosition, NextTickPosition, t);
}

VS_OUTPUT main(VS_INPUT_ANIMATION Input)
{
	float4x4 FinalBone = KMatrixIdentity;
	if (bUseGPUSkinning == true)
	{
		if (bIsInstanced == true)
		{
			Input.Position = GetGPUSkinnedPosition(Input.Position, Input.CurrAnimID, Input.AnimTick, Input.BoneIndex, Input.BoneWeight);
		}
		else
		{
			Input.Position = GetGPUSkinnedPosition(Input.Position, g_AnimationID, g_AnimationTick, Input.BoneIndex, Input.BoneWeight);
		}
	}
	else
	{
		FinalBone = BoneMatrices[Input.BoneIndex.x] * Input.BoneWeight.x;
		FinalBone += BoneMatrices[Input.BoneIndex.y] * Input.BoneWeight.y;
		FinalBone += BoneMatrices[Input.BoneIndex.z] * Input.BoneWeight.z;
		FinalBone += BoneMatrices[Input.BoneIndex.w] * Input.BoneWeight.w;

		Input.Position = float4(mul(Input.Position, FinalBone).xyz, 1);
	}
	Input.Normal = normalize(mul(Input.Normal, FinalBone));
	Input.Tangent = normalize(mul(Input.Tangent, FinalBone));

	if (bIsInstanced == true)
	{
		float4x4 InstanceWorld = float4x4(Input.InstanceWorld0, Input.InstanceWorld1, Input.InstanceWorld2, Input.InstanceWorld3);
		return Internal(InstanceWorld, Input);
	}
	return Internal(World, Input);
}