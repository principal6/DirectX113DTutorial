#pragma once

#include "../Core/SharedHeader.h"

class CObject3D;

enum class EObjectRole
{
	None,
	Player,
	Environment,
	Monster
};

struct SCollisionItem
{
	SObjectIdentifier		A{};
	const XMVECTOR*			A_Translation{};
	const SBoundingVolume*	A_BS{};
	SObjectIdentifier		B{};
	const XMVECTOR*			B_Translation{};
	const SBoundingVolume*	B_BS{};
	float					DistanceSquare{};

	bool operator<(const SCollisionItem& b) const
	{
		return DistanceSquare < b.DistanceSquare;
	}
};

class CPhysicsEngine final
{
public:
	CPhysicsEngine();
	~CPhysicsEngine();

public:
	void ClearData();

public:
	void SetWorldFloorHeight(float WorldFloorHeight);
	float GetWorldFloorHeight() const;

	void SetGravity(const XMVECTOR& Gravity);

// Object registration & deregistration
public:
	void RegisterObject(CObject3D* const Object3D, EObjectRole eObjectRole);
	void DeregisterObject(CObject3D* const Object3D);

private:
	void RegisterPlayerObject(CObject3D* const Object3D);
	void RegisterEnvironmentObject(CObject3D* const Object3D);
	void RegisterMonsterObject(CObject3D* const Object3D);

	void DeregisterPlayerObject(CObject3D* const Object3D);
	void DeregisterEnvironmentObject(CObject3D* const Object3D);
	void DeregisterMonsterObject(CObject3D* const Object3D);

// Object registration helpers
public:
	bool IsPlayerObject(CObject3D* const Object3D) const;
	bool IsEnvironmentObject(CObject3D* const Object3D) const;
	bool IsMonsterObject(CObject3D* const Object3D) const;
	EObjectRole GetObjectRole(CObject3D* const Object3D) const;

public:
	CObject3D* GetPlayerObject() const;

public:
	void ShouldApplyGravity(bool Value);

public:
	bool PickObject(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection);

private:
	bool DetectRayObjectIntersection(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
		const XMVECTOR& Position, const SBoundingVolume& OuterBS, const std::vector<SBoundingVolume>& vInnerBVs, XMVECTOR& T);

public:
	const XMVECTOR& GetPickedPoint() const;
	CObject3D* GetPickedObject() const;

public:
	void Update(float DeltaTime);

private:
	void UpdateObject(float DeltaTime, CObject3D* const Object);
	void _UpdateObject(const SObjectIdentifier& Identifier, float DeltaTime);

private:
	bool DetectResolveEnvironmentCollisions(const SObjectIdentifier& A_Identifier);
	bool DetectEnvironmentCoarseCollision(const SObjectIdentifier& A, const SObjectIdentifier& B);
	bool DetectResolveFineCollision(const SCollisionItem& Coarse);

private:
	bool DetectIntersection(const XMVECTOR& APos, const SBoundingVolume& ABV, const XMVECTOR& BPos, const SBoundingVolume& BBV);
	void ResolvePenetration(const SCollisionItem& FineCollision);
	void GetClosestPoints(const XMVECTOR& DynamicPos, const SBoundingVolume& DynamicBV, const XMVECTOR& StaticPos, const SBoundingVolume& StaticBV);

// DEBUGGING
public:
	const XMVECTOR& GetDynamicClosestPoint() const;
	const XMVECTOR& GetStaticClosestPoint() const;

private:
	static constexpr XMVECTOR KDefaultGravity{ 0, -10.0f, 0, 0 };
	static constexpr float KDefaultWorldFloorHeight{ -5.0f };

private:
	CObject3D* m_PlayerObject{};

private:
	std::vector<CObject3D*> m_vEnvironmentObjects{};
	std::unordered_map<void*, uint32_t> m_mapEnvironmentObjects{}; // avoid duplication

private:
	std::vector<CObject3D*> m_vMonsterObjects{};
	std::unordered_map<void*, uint32_t> m_mapMonsterObjects{}; // avoid duplication

private:
	std::vector<SCollisionItem> m_vCoarseCollisionList{};

private:
	XMVECTOR m_DynamicClosestPoint{};
	XMVECTOR m_StaticClosestPoint{};
	XMVECTOR m_CollisionNormal{}; // From B to A
	float m_PenetrationDepth{};

private:
	float m_WorldFloorHeight{ KDefaultWorldFloorHeight };
	XMVECTOR m_Gravity{ KDefaultGravity };

private:
	bool m_bShouldApplyGravity{};

private:
	XMVECTOR m_PickedPoint{};
	CObject3D* m_PickedObject{};
};
