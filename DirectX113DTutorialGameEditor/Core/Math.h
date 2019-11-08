#pragma once

#include "SharedHeader.h"

static const XMVECTOR KVectorZero{ XMVectorZero() };
static const XMVECTOR KVectorGreatest{ XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX) };

static int GetRandom(int Min, int Max);
static float GetRandom(float Min, float Max);
static bool IntersectRaySphere(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, float Radius, const XMVECTOR& Center, XMVECTOR* OutPtrT) noexcept;
static XMVECTOR CalculateTriangleNormal(const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2);
static bool IsPointInTriangle(const XMVECTOR& Point, const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2);
static bool IntersectRayPlane(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, const XMVECTOR& PlaneP, const XMVECTOR& PlaneN, XMVECTOR* OutPtrT);
static bool IntersectRayTriangle(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
	const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2, XMVECTOR* OutPtrT);

static float Lerp(float a, float b, float t)
{
	// (1 - t)a + tb
	// = a - ta + tb
	// = a + t(b - a)
	return a + t * (b - a);
}

static int GetRandom(int Min, int Max)
{
	if (Min >= Max) return Min;

	return (rand() % (Max - Min + 1)) + Min;
}

static float GetRandom(float Min, float Max)
{
	if (Min >= Max) return Min;

	constexpr float KFreedom{ 100.0f };
	float Range{ Max - Min };
	int IntRange{ (int)(KFreedom * Range) };
	int IntRangeHalf{ (int)(IntRange / 2) };

	return static_cast<float>((rand() % (IntRange + 1) / KFreedom) + Min);
}

static bool IntersectRaySphere(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, float Radius, const XMVECTOR& Center, XMVECTOR* OutPtrT) noexcept
{
	XMVECTOR r{ XMVectorSet(Radius, Radius, Radius, 1.0f) };
	XMVECTOR CO{ RayOrigin - Center };

	XMVECTOR a{ XMVector3Dot(RayDirection, RayDirection) };
	XMVECTOR b{ 2.0f * XMVector3Dot(RayDirection, CO) };
	XMVECTOR c{ XMVector3Dot(CO, CO) - r * r };
	XMVECTOR Discriminant{ b * b - 4.0f * a * c };

	if (XMVector3GreaterOrEqual(Discriminant, KVectorZero))
	{
		XMVECTOR TPlus{ (-b + XMVectorSqrt(Discriminant)) / 2.0f * a };
		XMVECTOR TMinus{ (-b - XMVectorSqrt(Discriminant)) / 2.0f * a };

		XMVECTOR TResult{ TMinus };
		if (XMVector3Less(TResult, KVectorZero)) TResult = TPlus;

		if (OutPtrT) *OutPtrT = TResult;

		return true;
	}

	return false;
}

static XMVECTOR CalculateTriangleNormal(const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2)
{
	XMVECTOR TriangleEdge01{ XMVector3Normalize(TriangleV1 - TriangleV0) };
	XMVECTOR TriangleEdge20{ XMVector3Normalize(TriangleV0 - TriangleV2) };
	return XMVector3Normalize(XMVector3Cross(TriangleEdge01, -TriangleEdge20));
}

static bool IsPointInTriangle(const XMVECTOR& Point, const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2)
{
	XMVECTOR TriangleEdge01{ XMVector3Normalize(TriangleV1 - TriangleV0) };
	XMVECTOR TriangleEdge12{ XMVector3Normalize(TriangleV2 - TriangleV1) };
	XMVECTOR TriangleEdge20{ XMVector3Normalize(TriangleV0 - TriangleV2) };
	XMVECTOR TriangleNormal{ XMVector3Normalize(XMVector3Cross(TriangleEdge01, -TriangleEdge20)) };

	XMVECTOR PV1{ XMVector3Normalize(TriangleV1 - Point) };
	if (XMVector3GreaterOrEqual(XMVector3Dot(XMVector3Cross(PV1, TriangleEdge01), TriangleNormal), KVectorZero))
	{
		XMVECTOR PV2{ XMVector3Normalize(TriangleV2 - Point) };
		if (XMVector3GreaterOrEqual(XMVector3Dot(XMVector3Cross(PV2, TriangleEdge12), TriangleNormal), KVectorZero))
		{
			XMVECTOR PV0{ XMVector3Normalize(TriangleV0 - Point) };
			if (XMVector3GreaterOrEqual(XMVector3Dot(XMVector3Cross(PV0, TriangleEdge20), TriangleNormal), KVectorZero))
			{
				return true;
			}
		}
	}

	return false;
}

static bool IntersectRayPlane(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, const XMVECTOR& PlaneP, const XMVECTOR& PlaneN, XMVECTOR* OutPtrT)
{
	XMVECTOR PlaneD{ -XMVector3Dot(PlaneP, PlaneN) };
	XMVECTOR Numerator{ -XMVector3Dot(RayOrigin, PlaneN) - PlaneD };
	XMVECTOR Denominator{ XMVector3Dot(RayDirection, PlaneN) };

	XMVECTOR T{ XMVectorDivide(Numerator, Denominator) };

	if (XMVector3Greater(T, KVectorZero))
	{
		if (OutPtrT) *OutPtrT = T;
		return true;
	}
	return false;
}

static bool IntersectRayTriangle(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
	const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2, XMVECTOR* OutPtrT)
{
	XMVECTOR TriangleEdge01{ TriangleV1 - TriangleV0 };
	XMVECTOR TriangleEdge02{ TriangleV2 - TriangleV0 };
	XMVECTOR TriangleNormal{ XMVector3Cross(TriangleEdge01 , TriangleEdge02) };
	XMVECTOR TrianglePlaneD{ -XMVector3Dot(TriangleV0, TriangleNormal) };

	XMVECTOR Numerator{ -XMVector3Dot(RayOrigin, TriangleNormal) - TrianglePlaneD };
	XMVECTOR Denominator{ XMVector3Dot(RayDirection, TriangleNormal) };

	XMVECTOR T{ XMVectorDivide(Numerator, Denominator) };

	if (XMVector3Greater(T, KVectorZero))
	{
		XMVECTOR Point{ RayOrigin + T * RayDirection };
		if (IsPointInTriangle(Point, TriangleV0, TriangleV1, TriangleV2))
		{
			if (OutPtrT) *OutPtrT = T;

			return true;
		}
	}

	return false;
}