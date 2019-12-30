#pragma once

#include "SharedHeader.h"

using namespace DirectX;
using std::max;
using std::min;

static const XMVECTOR KVectorZero{ XMVectorZero() };
static const XMVECTOR KVectorGreatest{ XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX) };

static float Lerp(float a, float b, float t);
static XMVECTOR Lerp(const XMVECTOR& a, const XMVECTOR& b, float t);
static XMVECTOR Slerp(const XMVECTOR& P0, const XMVECTOR& P1, float t);
static int GetRandom(int Min, int Max);
static float GetRandom(float Min, float Max);
static bool IntersectPointSphere(const XMVECTOR& PointInSpace, float SphereRadius, const XMVECTOR& SphereCenter);
static bool IntersectRaySphere(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, float Radius, const XMVECTOR& Center, XMVECTOR* const OutPtrT) noexcept;
static XMVECTOR CalculateTriangleNormal(const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2);
static bool IsPointInTriangle(const XMVECTOR& Point, const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2);
static bool IntersectRayPlane(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, const XMVECTOR& PlaneP, const XMVECTOR& PlaneN, XMVECTOR* const OutPtrT);
static bool IntersectRayTriangle(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
	const XMVECTOR& TriangleV0, const XMVECTOR& TriangleV1, const XMVECTOR& TriangleV2, XMVECTOR* OutPtrT);
static float GetPlanePointDistnace(const XMVECTOR& PlaneP, const XMVECTOR& PlaneN, const XMVECTOR& Point);
static bool IntersectRayCylinder(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, 
	float CylinderHeight, float CylinderRadius, XMVECTOR* OutPtrT = nullptr);
static bool IntersectRayHollowCylinderCentered(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
	float CylinderHeight, float CylinderInnerRadius, float CylinderOuterRadius, XMVECTOR* OutPtrT = nullptr);
static bool IntersectSphereSphere(const XMVECTOR& CenterA, float RadiusA, const XMVECTOR& CenterB, float RadiusB);
static bool IntersectSphereAABB(const XMVECTOR& SphereCenter, float SphereRadius, const XMVECTOR& AABBCenter, float HalfSizeX, float HalfSizeY, float HalfSizeZ);
static bool IntersectAABBAABB(const XMVECTOR& ACenter, float AHalfSizeX, float AHalfSizeY, float AHalfSizeZ,
	const XMVECTOR& BCenter, float BHalfSizeX, float BHalfSizeY, float BHalfSizeZ);
static XMVECTOR GetClosestPointSphere(const XMVECTOR& Point, const XMVECTOR SphereCenter, float SphereRadius);
static XMVECTOR GetClosestPointAABB(const XMVECTOR& Point, const XMVECTOR AABBCenter, float HalfSizeX, float HalfSizeY, float HalfSizeZ);

static float Lerp(float a, float b, float t)
{
	// (1 - t)a + tb
	// = a - ta + tb
	// = a + t(b - a)
	return a + t * (b - a);
}

static XMVECTOR Lerp(const XMVECTOR& a, const XMVECTOR& b, float t)
{
	// (1 - t)a + tb
	// = a - ta + tb
	// = a + t(b - a)
	return a + t * (b - a);
}

static XMVECTOR Slerp(const XMVECTOR& P0, const XMVECTOR& P1, float t)
{
	const float KThreshold{ 0.99f };
	float Dot{ XMVectorGetX(XMVector3Dot(XMVector3Normalize(P0), XMVector3Normalize(P1))) };
	if (abs(Dot) < KThreshold)
	{
		float Theta{ acos(Dot) };
		return (P0 * sin((1 - t) * Theta) + P1 * sin(t * Theta)) / sin(Theta);
	}
	else
	{
		return Lerp(P0, P1, t);
	}
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

static bool IntersectPointSphere(const XMVECTOR& PointInSpace, float SphereRadius, const XMVECTOR& SphereCenter)
{
	float SphereRadiusSquare{ SphereRadius * SphereRadius };
	XMVECTOR CenterToPoint{ PointInSpace - SphereCenter };
	if (XMVectorGetX(XMVector3Dot(CenterToPoint, CenterToPoint)) <= SphereRadiusSquare)
	{
		return true;
	}
	return false;
}

static bool IntersectRaySphere(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, float Radius, const XMVECTOR& Center, XMVECTOR* const OutPtrT) noexcept
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

static bool IntersectRayPlane(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, const XMVECTOR& PlaneP, const XMVECTOR& PlaneN, XMVECTOR* const OutPtrT)
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

static float GetPlanePointDistnace(const XMVECTOR& PlaneP, const XMVECTOR& PlaneN, const XMVECTOR& Point)
{
	return XMVectorGetX(XMVector3Dot(PlaneN, Point - PlaneP));
}

static bool IntersectRayCylinder(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection, 
	float CylinderHeight, float CylinderRadius, XMVECTOR* OutPtrT)
{
	// Ray: O + tD
	
	// Cylinder
	// (centered at the origin toward Y axis)
	// Cyliner side: x^2 + z^2 = r^2
	// Cylinder upper cap: y = h, x^2 + z^2 <= r^2
	// Cylinder lower cap: y = 0, x^2 + z^2 <= r^2

	const float KRadiusSquare{ CylinderRadius * CylinderRadius };

	// Side intersection
	//  if  O' = (Ox, 0.0f, Oz)
	//  and D' = (Dx, 0.0f, Dz) 
	// (O' + tD')©÷ = r©÷
	// D'©÷t©÷ + 2O'D't + O'©÷ = r©÷
	// (D'¡¤D')t©÷ + 2(O'¡¤D')t + (O'¡¤O') - r©÷ = 0
	// And if discriminant { b©÷ - 4ac } > 0, t exists.
	//  where a = (D'¡¤D')
	//        b = 2(O'¡¤D')
	//    and c = (O'¡¤O') - r©÷
	{
		XMVECTOR OPrime{ XMVectorSetY(RayOrigin, 0.0f) };
		XMVECTOR DPrime{ XMVectorSetY(RayDirection, 0.0f) };
		float a{ XMVectorGetX(XMVector3Dot(DPrime, DPrime)) };
		float b{ 2 * XMVectorGetX(XMVector3Dot(OPrime, DPrime)) };
		float c{ XMVectorGetX(XMVector3Dot(OPrime, OPrime)) - KRadiusSquare };
		float discriminant{ b * b - 4 * a * c };
		if (discriminant >= 0.0f)
		{
			float t_positive{ (-b + sqrt(discriminant)) / (2 * a) };
			XMVECTOR PointOnCylinder{ RayOrigin + t_positive * RayDirection };
			float PointY{ XMVectorGetY(PointOnCylinder) };
			if (PointY >= 0.0f && PointY <= CylinderHeight)
			{
				if (OutPtrT) *OutPtrT = XMVectorSet(t_positive, t_positive, t_positive, t_positive);
				return true;
			}
		}
	}

	// Upper cap intersection
	{
		XMVECTOR t{};
		if (IntersectRayPlane(RayOrigin, RayDirection, XMVectorSet(0, CylinderHeight, 0, 1), XMVectorSet(0, +1.0f, 0, 0), &t))
		{
			XMVECTOR PointOnPlane{ RayOrigin + t * RayDirection };
			float X{ XMVectorGetX(PointOnPlane) };
			float Z{ XMVectorGetZ(PointOnPlane) };
			if (X * X + Z * Z <= KRadiusSquare)
			{
				if (OutPtrT) *OutPtrT = t;
				return true;
			}
		}
	}

	// Lower cap intersection
	{
		XMVECTOR t{};
		if (IntersectRayPlane(RayOrigin, RayDirection, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, -1.0f, 0, 0), &t))
		{
			XMVECTOR PointOnPlane{ RayOrigin + t * RayDirection };
			float X{ XMVectorGetX(PointOnPlane) };
			float Z{ XMVectorGetZ(PointOnPlane) };
			if (X * X + Z * Z <= KRadiusSquare)
			{
				if (OutPtrT) *OutPtrT = t;
				return true;
			}
		}
	}

	return false;
}

static bool IntersectRayHollowCylinderCentered(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
	float CylinderHeight, float CylinderInnerRadius, float CylinderOuterRadius, XMVECTOR* OutPtrT)
{
	const float KHalfHeight{ CylinderHeight * 0.5f };
	const float KInnerRadiusSquare{ CylinderInnerRadius * CylinderInnerRadius };
	const float KOuterRadiusSquare{ CylinderOuterRadius * CylinderOuterRadius };

	// Side intersection
	{
		XMVECTOR OPrime{ XMVectorSetY(RayOrigin, 0.0f) };
		XMVECTOR DPrime{ XMVectorSetY(RayDirection, 0.0f) };
		float a{ XMVectorGetX(XMVector3Dot(DPrime, DPrime)) };
		float b{ 2 * XMVectorGetX(XMVector3Dot(OPrime, DPrime)) };
		float c{ XMVectorGetX(XMVector3Dot(OPrime, OPrime)) - KOuterRadiusSquare };
		float discriminant{ b * b - 4 * a * c };
		if (discriminant >= 0.0f)
		{
			float t_positive{ (-b + sqrt(discriminant)) / (2 * a) };
			float t_negative{ (-b - sqrt(discriminant)) / (2 * a) };

			XMVECTOR PointOnCylinderPositive{ RayOrigin + t_positive * RayDirection };
			XMVECTOR PointOnCylinderNegative{ RayOrigin + t_negative * RayDirection };
			float PointYPositive{ XMVectorGetY(PointOnCylinderPositive) };
			float PointYNegative{ XMVectorGetY(PointOnCylinderNegative) };

			if (PointYPositive >= -KHalfHeight && PointYPositive <= +KHalfHeight)
			{
				if (OutPtrT) *OutPtrT = XMVectorSet(t_positive, t_positive, t_positive, t_positive);
				return true;
			}

			if (PointYNegative >= -KHalfHeight && PointYNegative <= +KHalfHeight)
			{
				if (OutPtrT) *OutPtrT = XMVectorSet(t_negative, t_negative, t_negative, t_negative);
				return true;
			}
		}
	}

	// Upper cap intersection
	{
		XMVECTOR t{};
		if (IntersectRayPlane(RayOrigin, RayDirection, XMVectorSet(0, +KHalfHeight, 0, 1), XMVectorSet(0, +1.0f, 0, 0), &t))
		{
			XMVECTOR PointOnPlane{ RayOrigin + t * RayDirection };
			float X{ XMVectorGetX(PointOnPlane) };
			float Z{ XMVectorGetZ(PointOnPlane) };
			float DistanceSquare{ X * X + Z * Z };
			if (DistanceSquare <= KOuterRadiusSquare && DistanceSquare >= KInnerRadiusSquare)
			{
				if (OutPtrT) *OutPtrT = t;
				return true;
			}
		}
	}

	// Lower cap intersection
	{
		XMVECTOR t{};
		if (IntersectRayPlane(RayOrigin, RayDirection, XMVectorSet(0, -KHalfHeight, 0, 1), XMVectorSet(0, -1.0f, 0, 0), &t))
		{
			XMVECTOR PointOnPlane{ RayOrigin + t * RayDirection };
			float X{ XMVectorGetX(PointOnPlane) };
			float Z{ XMVectorGetZ(PointOnPlane) };
			float DistanceSquare{ X * X + Z * Z };
			if (DistanceSquare <= KOuterRadiusSquare && DistanceSquare >= KInnerRadiusSquare)
			{
				if (OutPtrT) *OutPtrT = t;
				return true;
			}
		}
	}

	return false;
}

static bool IntersectSphereSphere(const XMVECTOR& CenterA, float RadiusA, const XMVECTOR& CenterB, float RadiusB)
{
	auto Difference{ CenterA - CenterB };
	float DistanceSquare{ XMVectorGetX(XMVector3LengthSq(Difference)) };
	float RadiusSum{ RadiusA + RadiusB };
	if (DistanceSquare <= RadiusSum * RadiusSum)
	{
		return true;
	}
	return false;
}

static bool IntersectSphereAABB(const XMVECTOR& SphereCenter, float SphereRadius, const XMVECTOR& AABBCenter, float HalfSizeX, float HalfSizeY, float HalfSizeZ)
{
	XMVECTOR AABBMax{ AABBCenter + XMVectorSet(HalfSizeX, HalfSizeY, HalfSizeZ, 0) };
	XMVECTOR AABBMin{ AABBCenter - XMVectorSet(HalfSizeX, HalfSizeY, HalfSizeZ, 0) };
	XMVECTOR AABBClosestPoint{ XMVectorMin(XMVectorMax(SphereCenter, AABBMin), AABBMax) };

	XMVECTOR Difference{ SphereCenter - AABBClosestPoint };
	float DistanceSquare{ XMVectorGetX(XMVector3LengthSq(Difference)) };
	if (DistanceSquare <= SphereRadius * SphereRadius)
	{
		return true;
	}
	return false;
}

static bool IntersectAABBAABB(const XMVECTOR& ACenter, float AHalfSizeX, float AHalfSizeY, float AHalfSizeZ, 
	const XMVECTOR& BCenter, float BHalfSizeX, float BHalfSizeY, float BHalfSizeZ)
{
	XMVECTOR DifferenceAbs{ XMVectorAbs(ACenter - BCenter) };

	XMVECTOR AHalfSize{ XMVectorSet(AHalfSizeX, AHalfSizeY, AHalfSizeZ, 0) };
	XMVECTOR BHalfSize{ XMVectorSet(BHalfSizeX, BHalfSizeY, BHalfSizeZ, 0) };

	return XMVector3LessOrEqual(DifferenceAbs, AHalfSize + BHalfSize);
}

static XMVECTOR GetClosestPointSphere(const XMVECTOR& Point, const XMVECTOR SphereCenter, float SphereRadius)
{
	XMVECTOR CenterToPoint{ Point - SphereCenter };
	return SphereCenter + XMVector3Normalize(CenterToPoint) * SphereRadius;

	/*float LengthSquare{ XMVectorGetX(XMVector3LengthSq(CenterToPoint)) };
	if (LengthSquare >= SphereRadius * SphereRadius)
	{
		return SphereCenter + XMVector3Normalize(CenterToPoint) * SphereRadius;
	}
	else
	{
		return Point;
	}*/
}

static XMVECTOR GetClosestPointAABB(const XMVECTOR& Point, const XMVECTOR AABBCenter, float HalfSizeX, float HalfSizeY, float HalfSizeZ)
{
	const float CenterX{ XMVectorGetX(AABBCenter) };
	const float CenterY{ XMVectorGetY(AABBCenter) };
	const float CenterZ{ XMVectorGetZ(AABBCenter) };

	const float XMax{ CenterX + HalfSizeX };
	const float XMin{ CenterX - HalfSizeX };
	const float YMax{ CenterY + HalfSizeY };
	const float YMin{ CenterY - HalfSizeY };
	const float ZMax{ CenterZ + HalfSizeZ };
	const float ZMin{ CenterZ - HalfSizeZ };

	float PointX{ XMVectorGetX(Point) };
	float PointY{ XMVectorGetY(Point) };
	float PointZ{ XMVectorGetZ(Point) };
	PointX = min(max(PointX, XMin), XMax);
	PointY = min(max(PointY, YMin), YMax);
	PointZ = min(max(PointZ, ZMin), ZMax);
	
	return XMVectorSet(PointX, PointY, PointZ, 1);
}

static XMVECTOR GetClosestFaceNormalAABB(const XMVECTOR& ClosestPoint, const XMVECTOR AABBCenter, float HalfSizeX, float HalfSizeY, float HalfSizeZ)
{
	const float CenterX{ XMVectorGetX(AABBCenter) };
	const float CenterY{ XMVectorGetY(AABBCenter) };
	const float CenterZ{ XMVectorGetZ(AABBCenter) };

	const float XMax{ CenterX + HalfSizeX };
	const float XMin{ CenterX - HalfSizeX };
	const float YMax{ CenterY + HalfSizeY };
	const float YMin{ CenterY - HalfSizeY };
	const float ZMax{ CenterZ + HalfSizeZ };
	const float ZMin{ CenterZ - HalfSizeZ };

	float ClosestPointX{ XMVectorGetX(ClosestPoint) };
	float ClosestPointY{ XMVectorGetY(ClosestPoint) };
	float ClosestPointZ{ XMVectorGetZ(ClosestPoint) };

	if (ClosestPointX == XMax) return XMVectorSet(+1, 0, 0, 0);
	if (ClosestPointX == XMin) return XMVectorSet(-1, 0, 0, 0);
	if (ClosestPointY == YMax) return XMVectorSet(0, +1, 0, 0);
	if (ClosestPointY == YMin) return XMVectorSet(0, -1, 0, 0);
	if (ClosestPointZ == ZMax) return XMVectorSet(0, 0, +1, 0);
	if (ClosestPointZ == ZMin) return XMVectorSet(0, 0, -1, 0);

	return XMVectorSet(0, 0, 0, 0);
}