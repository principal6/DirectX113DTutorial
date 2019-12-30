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

public:
	void RegisterObject(CObject3D* const Object3D, EObjectRole eObjectRole);
	void RegisterPlayerObject(CObject3D* const Object3D);
	void RegisterEnvironmentObject(CObject3D* const Object3D);
	void RegisterMonsterObject(CObject3D* const Object3D);

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
	void Update(float DeltaTime);

private:
	bool DetectCollisions();
	bool DetectFineCollisionDynamicStatic(
		CObject3D* const DynamicObject, const XMVECTOR& DynamicPos, const SBoundingVolume& DynamicBV, const std::vector<SBoundingVolume>& vDynamicBVs,
		CObject3D* const StaticObject, const XMVECTOR& StaticPos, const SBoundingVolume& StaticBV, const std::vector<SBoundingVolume>& vStaticBVs);

private:
	bool DetectIntersection(const XMVECTOR& APos, const SBoundingVolume& ABV, const XMVECTOR& BPos, const SBoundingVolume& BBV);
	void ResolvePenetration(
		CObject3D* const DynamicObject, const XMVECTOR& DynamicPos, const SBoundingVolume& DynamicBV,
		CObject3D* const StaticObject, const XMVECTOR& StaticPos, const SBoundingVolume& StaticBV);

// DEBUGGING
public:
	const XMVECTOR& GetDynamicClosestPoint() const;
	const XMVECTOR& GetStaticClosestPoint() const;

private:
	static constexpr XMVECTOR KDefaultGravity{ 0, -10.0f, 0, 0 };

private:
	CObject3D* m_PlayerObject{};

private:
	std::vector<CObject3D*> m_vEnvironmentObjects{};
	std::unordered_map<void*, uint32_t> m_mapEnvironmentObjects{}; // avoid duplication

private:
	std::vector<CObject3D*> m_vMonsterObjects{};
	std::unordered_map<void*, uint32_t> m_mapMonsterObjects{}; // avoid duplication

private:
	XMVECTOR m_DynamicClosestPoint{};
	XMVECTOR m_StaticClosestPoint{};
	XMVECTOR m_CollisionNormal{}; // From B to A
	float m_PenetrationDepth{};

private:
	float m_WorldFloorHeight{};
	XMVECTOR m_Gravity{ KDefaultGravity };

private:
	bool m_bShouldApplyGravity{};
};
