#pragma once
#include "SharedHeader.h"

struct SShadowMapFrustum
{
	XMVECTOR LightPosition{};
	XMVECTOR LightForward{};
	XMVECTOR LightUp{};
	XMVECTOR LightRight{};
	XMFLOAT3 HalfSize{};
};

// Frustum consists of 8 vertices:
// [0] NearUpperA
// [1] NearUpperB
// [2] NearLowerA
// [3] NearLowerB
// [4] FarUpperA
// [5] FarUpperB
// [6] FarLowerA
// [7] FarLowerB
struct SFrustumVertices
{
	XMVECTOR Vertices[8]{};
};

static float MaxFloat8(float(&Values)[8])
{
	float Result{ FLT_MIN };
	for (size_t iValue = 0; iValue < 8; ++iValue)
	{
		if (Values[iValue] > Result)
		{
			Result = Values[iValue];
		}
	}
	return Result;
}

static float MinFloat8(float(&Values)[8])
{
	float Result{ FLT_MAX };
	for (size_t iValue = 0; iValue < 8; ++iValue)
	{
		if (Values[iValue] < Result)
		{
			Result = Values[iValue];
		}
	}
	return Result;
}

static SFrustumVertices CalculateViewFrustumVertices(const XMMATRIX& Projection, const XMVECTOR& EyePosition, const XMVECTOR& ViewDirection,
	const XMVECTOR& DirectionToLight, float ViewFrustumZNear, float ViewFrustumZFar)
{
	XMVECTOR ViewRight{ XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), ViewDirection)) };
	XMVECTOR ViewUp{ XMVector3Cross(ViewDirection, ViewRight) };

	float ViewFrustumPlaneNearWidth{ ViewFrustumZNear / Projection.r[0].m128_f32[0] };
	float ViewFrustumPlaneNearHeight{ ViewFrustumZNear / Projection.r[1].m128_f32[1] };
	float ViewFrustumPlaneFarWidth{ ViewFrustumZFar / Projection.r[0].m128_f32[0] };
	float ViewFrustumPlaneFarHeight{ ViewFrustumZFar / Projection.r[1].m128_f32[1] };

	XMVECTOR ViewFrustumPlaneNearCenter{ EyePosition + ViewDirection * ViewFrustumZNear };
	XMVECTOR ViewFrustumPlaneFarCenter{ EyePosition + ViewDirection * ViewFrustumZFar };

	XMVECTOR ViewFrustumPlaneFarUpperA{ ViewFrustumPlaneFarCenter - ViewFrustumPlaneFarWidth * ViewRight + ViewFrustumPlaneFarHeight * ViewUp };
	XMVECTOR ViewFrustumPlaneFarUpperB{ ViewFrustumPlaneFarCenter + ViewFrustumPlaneFarWidth * ViewRight + ViewFrustumPlaneFarHeight * ViewUp };
	XMVECTOR ViewFrustumPlaneFarLowerA{ ViewFrustumPlaneFarCenter - ViewFrustumPlaneFarWidth * ViewRight - ViewFrustumPlaneFarHeight * ViewUp };
	XMVECTOR ViewFrustumPlaneFarLowerB{ ViewFrustumPlaneFarCenter + ViewFrustumPlaneFarWidth * ViewRight - ViewFrustumPlaneFarHeight * ViewUp };

	XMVECTOR ViewFrustumPlaneNearUpperA{ ViewFrustumPlaneNearCenter - ViewFrustumPlaneNearWidth * ViewRight + ViewFrustumPlaneNearHeight * ViewUp };
	XMVECTOR ViewFrustumPlaneNearUpperB{ ViewFrustumPlaneNearCenter + ViewFrustumPlaneNearWidth * ViewRight + ViewFrustumPlaneNearHeight * ViewUp };
	XMVECTOR ViewFrustumPlaneNearLowerA{ ViewFrustumPlaneNearCenter - ViewFrustumPlaneNearWidth * ViewRight - ViewFrustumPlaneNearHeight * ViewUp };
	XMVECTOR ViewFrustumPlaneNearLowerB{ ViewFrustumPlaneNearCenter + ViewFrustumPlaneNearWidth * ViewRight - ViewFrustumPlaneNearHeight * ViewUp };

	SFrustumVertices Result
	{
		ViewFrustumPlaneNearUpperA, ViewFrustumPlaneNearUpperB, ViewFrustumPlaneNearLowerA, ViewFrustumPlaneNearLowerB,
		ViewFrustumPlaneFarUpperA, ViewFrustumPlaneFarUpperB, ViewFrustumPlaneFarLowerA, ViewFrustumPlaneFarLowerB
	};

	return Result;
}

static SShadowMapFrustum CalculateShadowMapFrustum(const XMMATRIX& Projection, const XMVECTOR& EyePosition, const XMVECTOR& ViewDirection,
	const XMVECTOR& DirectionToLight, float ViewFrustumZNear, float ViewFrustumZFar)
{
	SFrustumVertices ViewFrustumVertices{ CalculateViewFrustumVertices(Projection, EyePosition, ViewDirection, DirectionToLight, ViewFrustumZNear, ViewFrustumZFar) };

	SShadowMapFrustum ShadowMapFrustum{};
	ShadowMapFrustum.LightForward = -DirectionToLight;
	XMVECTOR DefaultUp{ XMVectorSet(0, 1, 0, 0) };
	if (XMVector3Equal(ShadowMapFrustum.LightForward, XMVectorSet(0, -1, 0, 0))) DefaultUp = XMVectorSet(-1, 0, 0, 0);
	ShadowMapFrustum.LightRight = XMVector3Normalize(XMVector3Cross(DefaultUp, ShadowMapFrustum.LightForward));
	ShadowMapFrustum.LightUp = XMVector3Cross(ShadowMapFrustum.LightForward, ShadowMapFrustum.LightRight);

	// x
	float XMax{};
	float XMin{};
	{
		float Dots[8]
		{
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[0] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[1] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[2] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[3] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[4] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[5] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[6] - EyePosition, ShadowMapFrustum.LightRight)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[7] - EyePosition, ShadowMapFrustum.LightRight))
		};
		XMax = MaxFloat8(Dots);
		XMin = MinFloat8(Dots);
	}

	// y
	float YMax{};
	float YMin{};
	{
		float Dots[8]
		{
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[0] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[1] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[2] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[3] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[4] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[5] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[6] - EyePosition, ShadowMapFrustum.LightUp)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[7] - EyePosition, ShadowMapFrustum.LightUp))
		};
		YMax = MaxFloat8(Dots);
		YMin = MinFloat8(Dots);
	} 

	// z
	float ZMax{};
	float ZMin{};
	{
		float Dots[8]
		{
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[0] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[1] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[2] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[3] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[4] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[5] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[6] - EyePosition, ShadowMapFrustum.LightForward)),
			XMVectorGetX(XMVector3Dot(ViewFrustumVertices.Vertices[7] - EyePosition, ShadowMapFrustum.LightForward))
		};
		ZMax = MaxFloat8(Dots);
		ZMin = MinFloat8(Dots);
	}

	ShadowMapFrustum.HalfSize.x = (XMax - XMin) * 0.5f;
	ShadowMapFrustum.HalfSize.y = (YMax - YMin) * 0.5f;
	ShadowMapFrustum.HalfSize.z = (ZMax - ZMin) * 0.5f;
	ShadowMapFrustum.HalfSize.z *= 1.5f; // @important: in order NOT to clip the shadows close to the viewer

	ShadowMapFrustum.LightPosition = EyePosition +
		(XMax - ShadowMapFrustum.HalfSize.x) * ShadowMapFrustum.LightRight +
		(YMax - ShadowMapFrustum.HalfSize.y) * ShadowMapFrustum.LightUp +
		(ZMax - ShadowMapFrustum.HalfSize.z * 2.0f) * ShadowMapFrustum.LightForward;
	
	return ShadowMapFrustum;
}

static SFrustumVertices CalculateShadowMapFrustumVertices(const XMVECTOR& DirectionToLight, const SShadowMapFrustum& ShadowMapFrustum)
{
	SFrustumVertices ShadowMapFrustumVertices{};

	// Near plane
	ShadowMapFrustumVertices.Vertices[0] = ShadowMapFrustum.LightPosition -
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x + ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;
	ShadowMapFrustumVertices.Vertices[1] = ShadowMapFrustum.LightPosition +
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x + ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;
	ShadowMapFrustumVertices.Vertices[2] = ShadowMapFrustum.LightPosition -
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x - ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;
	ShadowMapFrustumVertices.Vertices[3] = ShadowMapFrustum.LightPosition +
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x - ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;

	// Far plane
	ShadowMapFrustumVertices.Vertices[4] = ShadowMapFrustum.LightPosition +
		ShadowMapFrustum.LightForward * ShadowMapFrustum.HalfSize.z * 2.0f -
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x + ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;
	ShadowMapFrustumVertices.Vertices[5] = ShadowMapFrustum.LightPosition +
		ShadowMapFrustum.LightForward * ShadowMapFrustum.HalfSize.z * 2.0f +
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x + ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;
	ShadowMapFrustumVertices.Vertices[6] = ShadowMapFrustum.LightPosition +
		ShadowMapFrustum.LightForward * ShadowMapFrustum.HalfSize.z * 2.0f -
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x - ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;
	ShadowMapFrustumVertices.Vertices[7] = ShadowMapFrustum.LightPosition +
		ShadowMapFrustum.LightForward * ShadowMapFrustum.HalfSize.z * 2.0f +
		ShadowMapFrustum.LightRight * ShadowMapFrustum.HalfSize.x - ShadowMapFrustum.LightUp * ShadowMapFrustum.HalfSize.y;

	return ShadowMapFrustumVertices;
}
