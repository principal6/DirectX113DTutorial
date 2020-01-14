#include "PhysicsEngine.h"
#include "../Core/Object3D.h"
#include "../Core/Math.h"

using std::sort;
using std::swap;

CPhysicsEngine::CPhysicsEngine()
{
}

CPhysicsEngine::~CPhysicsEngine()
{
}

void CPhysicsEngine::ClearData()
{
	m_PlayerObject = nullptr;

	m_vEnvironmentObjects.clear();
	m_mapEnvironmentObjects.clear();

	m_vMonsterObjects.clear();
	m_mapMonsterObjects.clear();

	m_WorldFloorHeight = KDefaultWorldFloorHeight;
}

void CPhysicsEngine::SetWorldFloorHeight(float WorldFloorHeight)
{
	m_WorldFloorHeight = WorldFloorHeight;
}

float CPhysicsEngine::GetWorldFloorHeight() const
{
	return m_WorldFloorHeight;
}

void CPhysicsEngine::SetGravity(const XMVECTOR& Gravity)
{
	m_Gravity = Gravity;
}

void CPhysicsEngine::RegisterObject(CObject3D* const Object3D, EObjectRole eObjectRole)
{
	switch (eObjectRole)
	{
	case EObjectRole::None:
		break;
	case EObjectRole::Player:
		RegisterPlayerObject(Object3D);
		break;
	case EObjectRole::Environment:
		RegisterEnvironmentObject(Object3D);
		break;
	case EObjectRole::Monster:
		RegisterMonsterObject(Object3D);
		break;
	default:
		break;
	}
}

void CPhysicsEngine::DeregisterObject(CObject3D* const Object3D)
{
	DeregisterPlayerObject(Object3D);
	DeregisterEnvironmentObject(Object3D);
	DeregisterMonsterObject(Object3D);
}

void CPhysicsEngine::RegisterPlayerObject(CObject3D* const Object3D)
{
	if (!Object3D) return;

	// invariant
	if (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end())
	{
		m_mapEnvironmentObjects.erase(Object3D);
	}

	// invariant
	if (m_mapMonsterObjects.find(Object3D) != m_mapMonsterObjects.end())
	{
		m_mapMonsterObjects.erase(Object3D);
	}

	m_PlayerObject = Object3D;
}

void CPhysicsEngine::RegisterEnvironmentObject(CObject3D* const Object3D)
{
	if (!Object3D) return;
	if (m_PlayerObject == Object3D) return;

	// avoid duplication
	if (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end()) return;

	// invariant
	if (m_mapMonsterObjects.find(Object3D) != m_mapMonsterObjects.end())
	{
		m_mapMonsterObjects.erase(Object3D);
	}

	m_vEnvironmentObjects.emplace_back(Object3D);
	m_mapEnvironmentObjects[Object3D] = 1;
}

void CPhysicsEngine::RegisterMonsterObject(CObject3D* const Object3D)
{
	if (!Object3D) return;
	if (m_PlayerObject == Object3D) return;

	// avoid duplication
	if (m_mapMonsterObjects.find(Object3D) != m_mapMonsterObjects.end()) return;

	// invariant
	if (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end())
	{
		m_mapEnvironmentObjects.erase(Object3D);
	}

	m_vMonsterObjects.emplace_back(Object3D);
	m_mapMonsterObjects[Object3D] = 1;
}

void CPhysicsEngine::DeregisterPlayerObject(CObject3D* const Object3D)
{
	if (m_PlayerObject == Object3D) m_PlayerObject = nullptr;
}

void CPhysicsEngine::DeregisterEnvironmentObject(CObject3D* const Object3D)
{
	if (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end())
	{
		m_mapEnvironmentObjects.erase(Object3D);
		size_t iObject{};
		for (const auto& EnvironmentObject : m_vEnvironmentObjects)
		{
			if (EnvironmentObject == Object3D) break;
			++iObject;
		}
		if (iObject < m_vEnvironmentObjects.size() - 1)
		{
			swap(m_vEnvironmentObjects[iObject], m_vEnvironmentObjects.back());
		}
		m_vEnvironmentObjects.pop_back();
	}
}

void CPhysicsEngine::DeregisterMonsterObject(CObject3D* const Object3D)
{
	if (m_mapMonsterObjects.find(Object3D) != m_mapMonsterObjects.end())
	{
		m_mapMonsterObjects.erase(Object3D);
		size_t iObject{};
		for (const auto& MonsterObject : m_vMonsterObjects)
		{
			if (MonsterObject == Object3D) break;
			++iObject;
		}
		if (iObject < m_vMonsterObjects.size() - 1)
		{
			swap(m_vMonsterObjects[iObject], m_vMonsterObjects.back());
		}
		m_vMonsterObjects.pop_back();
	}
}

bool CPhysicsEngine::IsPlayerObject(CObject3D* const Object3D) const
{
	return (m_PlayerObject == Object3D);
}

bool CPhysicsEngine::IsEnvironmentObject(CObject3D* const Object3D) const
{
	return (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end());
}

bool CPhysicsEngine::IsMonsterObject(CObject3D* const Object3D) const
{
	return (m_mapMonsterObjects.find(Object3D) != m_mapMonsterObjects.end());
}

EObjectRole CPhysicsEngine::GetObjectRole(CObject3D* const Object3D) const
{
	if (IsPlayerObject(Object3D)) return EObjectRole::Player;
	if (IsEnvironmentObject(Object3D)) return EObjectRole::Environment;
	if (IsMonsterObject(Object3D)) return EObjectRole::Monster;

	return EObjectRole::None;
}

CObject3D* CPhysicsEngine::GetPlayerObject() const
{
	return m_PlayerObject;
}

void CPhysicsEngine::ShouldApplyGravity(bool Value)
{
	m_bShouldApplyGravity = Value;
}

bool CPhysicsEngine::PickObject(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection)
{
	XMVECTOR TCmp{ KVectorGreatest };
	bool bIntersected{ false };

	m_PickedPoint = KVectorZero;

	for (const auto& EnvironmentObject : m_vEnvironmentObjects)
	{
		if (EnvironmentObject->IsInstanced())
		{
			for (const auto& Instance : EnvironmentObject->GetInstanceCPUDataVector())
			{
				if (DetectRayObjectIntersection(RayOrigin, RayDirection, Instance.Transform.Translation,
					EnvironmentObject->GetOuterBoundingSphere(), EnvironmentObject->GetInnerBoundingVolumeVector(), TCmp))
				{
					bIntersected = true;
					m_PickedObject = EnvironmentObject;
					break;
				}
			}
		}
		else
		{
			if (DetectRayObjectIntersection(RayOrigin, RayDirection, EnvironmentObject->GetTransform().Translation,
				EnvironmentObject->GetOuterBoundingSphere(), EnvironmentObject->GetInnerBoundingVolumeVector(), TCmp))
			{
				bIntersected = true;
				m_PickedObject = EnvironmentObject;
			}
		}
	}

	if (bIntersected)
	{
		m_PickedPoint = RayOrigin + RayDirection * TCmp;
	}

	return bIntersected;
}

bool CPhysicsEngine::DetectRayObjectIntersection(const XMVECTOR& RayOrigin, const XMVECTOR& RayDirection,
	const XMVECTOR& Position, const SBoundingVolume& OuterBS, const std::vector<SBoundingVolume>& vInnerBVs, XMVECTOR& T)
{
	XMVECTOR TOuter{ KVectorGreatest };
	if (IntersectRaySphere(RayOrigin, RayDirection,
		OuterBS.Data.BS.Radius, Position + OuterBS.Center, &TOuter))
	{
		if (vInnerBVs.empty())
		{
			if (XMVector3Less(TOuter, T)) T = TOuter;
			return true;
		}
		else
		{
			TOuter = KVectorGreatest;
			XMVECTOR TInner{ KVectorGreatest };
			bool bIntersected{ false };
			for (const auto& BoundingVolume : vInnerBVs)
			{
				if (BoundingVolume.eType == EBoundingVolumeType::BoundingSphere)
				{
					if (IntersectRaySphere(RayOrigin, RayDirection,
						BoundingVolume.Data.BS.Radius, Position + BoundingVolume.Center, &TInner))
					{
						if (XMVector3Less(TInner, TOuter)) TOuter = TInner;
						bIntersected = true;
					}
				}
				else
				{
					if (IntersectRayAABB(RayOrigin, RayDirection,
						Position + BoundingVolume.Center,
						BoundingVolume.Data.AABBHalfSizes.x, BoundingVolume.Data.AABBHalfSizes.y, BoundingVolume.Data.AABBHalfSizes.z, &TInner))
					{
						if (XMVectorGetX(TInner) > 0 && XMVector3Less(TInner, TOuter))
						{
							TOuter = TInner;
							bIntersected = true;
						}
					}
				}
			}

			if (bIntersected)
			{
				T = TOuter;
				return true;
			}
		}
	}
	return false;
}

const XMVECTOR& CPhysicsEngine::GetPickedPoint() const
{
	return m_PickedPoint;
}

CObject3D* CPhysicsEngine::GetPickedObject() const
{
	return m_PickedObject;
}

void CPhysicsEngine::Update(float DeltaTime)
{
	if (DeltaTime <= 0) return;

	UpdateObject(DeltaTime, m_PlayerObject);

	for (auto& Monster : m_vMonsterObjects)
	{
		UpdateObject(DeltaTime, Monster);
	}
}

void CPhysicsEngine::UpdateObject(float DeltaTime, CObject3D* const Object)
{
	SObjectIdentifier Identifier{ Object };
	if (Object->IsInstanced())
	{
		for (const auto& Instance : Object->GetInstanceCPUDataVector())
		{
			Identifier.PtrInstanceName = Instance.Name.c_str();

			_UpdateObject(Identifier, DeltaTime);
		}

		Object->UpdateAllInstances(); // @important
	}
	else
	{
		_UpdateObject(Identifier, DeltaTime);
	}
}

void CPhysicsEngine::_UpdateObject(const SObjectIdentifier& Identifier, float DeltaTime)
{
	auto& Translation{ Identifier.Object3D->GetTransform(Identifier).Translation };
	auto& LinearAcceleration{ Identifier.Object3D->GetPhysics(Identifier).LinearAcceleration };
	auto& LinearVelocity{ Identifier.Object3D->GetPhysics(Identifier).LinearVelocity };

	if (m_bShouldApplyGravity)
	{
		Identifier.Object3D->AddLinearAcceleration(Identifier, m_Gravity); // Gravity
	}

	Identifier.Object3D->AddLinearVelocity(Identifier, LinearAcceleration * DeltaTime);
	Identifier.Object3D->Translate(Identifier, LinearVelocity * DeltaTime);
	Identifier.Object3D->SetLinearAcceleration(Identifier, KVectorZero);

	if (XMVectorGetY(Translation) < m_WorldFloorHeight)
	{
		Identifier.Object3D->TranslateTo(Identifier, XMVectorSetY(Translation, m_WorldFloorHeight));
		Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorSetY(LinearVelocity, 0.0f));
	}
	else
	{
		DetectResolveEnvironmentCollisions(Identifier);
	}
}

bool CPhysicsEngine::DetectResolveEnvironmentCollisions(const SObjectIdentifier& A_Identifier)
{
	bool bCollisionDetected{ false };

	// Initialize data
	m_DynamicClosestPoint = m_StaticClosestPoint = KVectorZero;
	m_vCoarseCollisionList.clear();

	// @important: A is dynamic && B(Environment) is static
	{
		for (auto& EnvironmentObject : m_vEnvironmentObjects)
		{
			CObject3D* const B{ EnvironmentObject };
			SObjectIdentifier B_Identifier{ B };
			if (B->IsInstanced())
			{
				for (const auto& InstanceCPUData : B->GetInstanceCPUDataVector())
				{
					B_Identifier.PtrInstanceName = InstanceCPUData.Name.c_str();
					DetectEnvironmentCoarseCollision(A_Identifier, B_Identifier);
				}
			}
			else
			{
				DetectEnvironmentCoarseCollision(A_Identifier, B_Identifier);
			}
		}
		
		// Time to fine collision
		sort(m_vCoarseCollisionList.begin(), m_vCoarseCollisionList.end(), std::less<SCollisionItem>());
		for (const auto& CoarseCollision : m_vCoarseCollisionList)
		{
			if (DetectResolveFineCollision(CoarseCollision))
			{
				if (CoarseCollision.A.PtrInstanceName)
				{
					int deb{};
				}
				bCollisionDetected = true;
			}
		}
	}

	return bCollisionDetected;
}

bool CPhysicsEngine::DetectEnvironmentCoarseCollision(const SObjectIdentifier& A, const SObjectIdentifier& B)
{
	const XMVECTOR* A_Translation{ &A.Object3D->GetTransform(A).Translation };
	const SBoundingVolume* A_BS{ &A.Object3D->GetOuterBoundingSphere(A) };
	const XMVECTOR* B_Translation{ &B.Object3D->GetTransform(B).Translation };
	const SBoundingVolume* B_BS{ &B.Object3D->GetOuterBoundingSphere(B) };
	
	// Coarse collision (sphere-sphere)
	XMVECTOR _A_T{ *A_Translation + A_BS->Center };
	XMVECTOR _B_T{ *B_Translation + B_BS->Center };

	if (DetectIntersection(_A_T, *A_BS, _B_T, *B_BS))
	{
		XMVECTOR Diff{ _B_T - _A_T };
		m_vCoarseCollisionList.emplace_back();
		m_vCoarseCollisionList.back().A = A;
		m_vCoarseCollisionList.back().A_Translation = A_Translation;
		m_vCoarseCollisionList.back().A_BS = A_BS;
		m_vCoarseCollisionList.back().B = B;
		m_vCoarseCollisionList.back().B_Translation = B_Translation;
		m_vCoarseCollisionList.back().B_BS = B_BS;
		m_vCoarseCollisionList.back().DistanceSquare = XMVectorGetX(XMVector3LengthSq(Diff));
		return true;
	}
	return false;
}

bool CPhysicsEngine::DetectResolveFineCollision(const SCollisionItem& Coarse)
{
	bool bCollided{ false };
	
	const XMVECTOR& A_Translation{ *Coarse.A_Translation };
	const XMVECTOR& B_Translation{ *Coarse.B_Translation };
	const SBoundingVolume& A_OuterBS{ *Coarse.A_BS };
	const SBoundingVolume& B_OuterBS{ *Coarse.B_BS };

	const auto& A_vInnerBVs{ Coarse.A.Object3D->GetInnerBoundingVolumeVector() };
	const auto& B_vInnerBVs{ Coarse.B.Object3D->GetInnerBoundingVolumeVector() };

	XMVECTOR _A_T{ A_Translation + Coarse.A_BS->Center };
	XMVECTOR _B_T{ B_Translation + Coarse.B_BS->Center };

	if (B_vInnerBVs.empty())
	{
		if (A_vInnerBVs.empty())
		{
			if (DetectIntersection(_A_T, A_OuterBS, _B_T, B_OuterBS))
			{
				GetClosestPoints(_A_T, A_OuterBS, _B_T, B_OuterBS);

				ResolvePenetration(Coarse);

				bCollided = true;
			}
		}
		else
		{
			for (const auto& A_BV_Item : A_vInnerBVs)
			{
				_A_T = A_Translation + A_BV_Item.Center;
				
				if (DetectIntersection(_A_T, A_BV_Item, _B_T, B_OuterBS))
				{
					GetClosestPoints(_A_T, A_BV_Item, _B_T, B_OuterBS);

					SCollisionItem Fine{ Coarse };
					Fine.A_BS = &A_BV_Item;
					ResolvePenetration(Fine);

					bCollided = true;
				}
			}
		}
	}
	else
	{
		if (A_vInnerBVs.empty())
		{
			for (const auto& B_BV_Item : B_vInnerBVs)
			{
				_B_T = B_Translation + B_BV_Item.Center;

				if (DetectIntersection(_A_T, A_OuterBS, _B_T, B_BV_Item))
				{
					GetClosestPoints(_A_T, A_OuterBS, _B_T, B_BV_Item);

					SCollisionItem Fine{ Coarse };
					Fine.B_BS = &B_BV_Item;
					ResolvePenetration(Fine);

					bCollided = true;
				}
			}
		}
		else
		{
			for (const auto& A_BV_Item : A_vInnerBVs)
			{
				_A_T = A_Translation + A_BV_Item.Center;
				
				for (const auto& B_BV_Item : B_vInnerBVs)
				{
					_B_T = B_Translation + B_BV_Item.Center;
					
					if (DetectIntersection(_A_T, A_BV_Item, _B_T, B_BV_Item))
					{
						GetClosestPoints(_A_T, A_BV_Item, _B_T, B_BV_Item);

						SCollisionItem Fine{ Coarse };
						Fine.A_BS = &A_BV_Item;
						Fine.B_BS = &B_BV_Item;
						ResolvePenetration(Fine);

						bCollided = true;
					}
				}
			}
		}
	}

	return bCollided;
}

bool CPhysicsEngine::DetectIntersection(const XMVECTOR& APos, const SBoundingVolume& ABV, const XMVECTOR& BPos, const SBoundingVolume& BBV)
{
	if (ABV.eType == EBoundingVolumeType::BoundingSphere)
	{
		if (BBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			return IntersectSphereSphere(
				APos, ABV.Data.BS.Radius,
				BPos, BBV.Data.BS.Radius);
		}
		else
		{
			return IntersectSphereAABB(
				APos, ABV.Data.BS.Radius,
				BPos, BBV.Data.AABBHalfSizes.x, BBV.Data.AABBHalfSizes.y, BBV.Data.AABBHalfSizes.z);
		}
	}
	else
	{
		if (BBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			return IntersectSphereAABB(
				BPos, BBV.Data.BS.Radius,
				APos, ABV.Data.AABBHalfSizes.x, ABV.Data.AABBHalfSizes.y, ABV.Data.AABBHalfSizes.z);
		}
		else
		{
			return IntersectAABBAABB(
				APos, ABV.Data.AABBHalfSizes.x, ABV.Data.AABBHalfSizes.y, ABV.Data.AABBHalfSizes.z,
				BPos, BBV.Data.AABBHalfSizes.x, BBV.Data.AABBHalfSizes.y, BBV.Data.AABBHalfSizes.z);
		}
	}
	return false;
}

void CPhysicsEngine::ResolvePenetration(const SCollisionItem& FineCollision)
{
	CObject3D* const A_Object{ FineCollision.A.Object3D };

	const SBoundingVolume& A_BS{ *FineCollision.A_BS };
	const SBoundingVolume& B_BS{ *FineCollision.B_BS };
	XMVECTOR _A_T{ *FineCollision.A_Translation + A_BS.Center };
	XMVECTOR _B_T{ *FineCollision.B_Translation + B_BS.Center };

	XMVECTOR A_MovingDir{ 
		(FineCollision.A.PtrInstanceName) ? 
		XMVector3Normalize(A_Object->GetPhysics(FineCollision.A).LinearVelocity) :
		XMVector3Normalize(A_Object->GetPhysics(FineCollision.A).LinearVelocity) };
	XMVECTOR AToB{ _B_T - _A_T };

	if (A_BS.eType == EBoundingVolumeType::BoundingSphere)
	{
		if (B_BS.eType == EBoundingVolumeType::BoundingSphere)
		{
			// Sphere - Sphere

			// C == A - B
			// D == direction of A
			// |C - xD| = r©û + r©ü
			//
			// (C - xD)¡¤(C - xD) = (r©û + r©ü)©÷
			// C¡¤C - 2xC¡¤D + x©÷D¡¤D = (r©û + r©ü)©÷
			// D¡¤Dx©÷ - 2C¡¤Dx + C¡¤C - (r©û - r©ü)©÷ = 0

			XMVECTOR C{ -AToB };
			XMVECTOR D{ A_MovingDir };
			float r1{ A_BS.Data.BS.Radius };
			float r2{ B_BS.Data.BS.Radius };
			float radii{ r1 + r2 };

			float a{ XMVectorGetX(XMVector3Dot(D, D)) }; // TODO: optimization
			float b{ -2.0f * XMVectorGetX(XMVector3Dot(C, D)) };
			float c{ XMVectorGetX(XMVector3Dot(C, C)) - radii * radii };
			float discriminant{ b * b - 4.0f * a * c };

			if (discriminant > 0)
			{
				float x_bigger{ (-b + sqrt(discriminant)) / (2.0f * a) };
				m_PenetrationDepth = abs(x_bigger);

				XMVECTOR Resolution{ -A_MovingDir * x_bigger };

				A_Object->Translate(FineCollision.A, Resolution);
			}
		}
		else
		{
			// dynamic Sphere - static AABB

			XMVECTOR Diff{ m_StaticClosestPoint - m_DynamicClosestPoint };
			XMVECTOR N{ XMVector3Normalize(Diff) };

			XMVECTOR Resolution{ Diff };
			m_PenetrationDepth = XMVectorGetX(XMVector3Length(Resolution));

			if (XMVectorGetY(N) == +1.0f)
			{
				A_Object->SetLinearVelocity(FineCollision.A, XMVectorSetY(A_Object->GetPhysics(FineCollision.A).LinearVelocity, 0));
			}
			A_Object->Translate(FineCollision.A, Resolution);
		}
	}
	else
	{
		if (B_BS.eType == EBoundingVolumeType::BoundingSphere)
		{
			// dynamic AABB - static Sphere

			XMVECTOR Diff{ m_DynamicClosestPoint - _B_T };
			float Distance{ XMVectorGetX(XMVector3Length(Diff)) };
			XMVECTOR N{ XMVector3Normalize(Diff) };

			m_PenetrationDepth = B_BS.Data.BS.Radius - Distance;
			XMVECTOR Resolution{ m_PenetrationDepth * N };

			A_Object->Translate(FineCollision.A, Resolution);
		}
		else
		{
			// AABB - AABB

			XMVECTOR Diff{ m_StaticClosestPoint - m_DynamicClosestPoint };
			XMVECTOR N{ GetAABBAABBCollisionNormal(A_MovingDir, m_DynamicClosestPoint,
				_B_T, B_BS.Data.AABBHalfSizes.x, B_BS.Data.AABBHalfSizes.y, B_BS.Data.AABBHalfSizes.z) };

			XMVECTOR Resolution{ XMVector3Dot(Diff, N) * N };
			m_PenetrationDepth = XMVectorGetX(XMVector3Length(Resolution));

			if (XMVectorGetY(N) == +1.0f)
			{
				A_Object->SetLinearVelocity(FineCollision.A, XMVectorSetY(A_Object->GetPhysics(FineCollision.A).LinearVelocity, 0));
			}
			A_Object->Translate(FineCollision.A, Resolution);
		}
	}
}

void CPhysicsEngine::GetClosestPoints(const XMVECTOR& DynamicPos, const SBoundingVolume& DynamicBV, const XMVECTOR& StaticPos, const SBoundingVolume& StaticBV)
{
	if (DynamicBV.eType == EBoundingVolumeType::BoundingSphere)
	{
		if (StaticBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			// dynamic Sphere - static Sphere
			m_DynamicClosestPoint = GetClosestPointSphere(StaticPos, DynamicPos, DynamicBV.Data.BS.Radius);
			m_StaticClosestPoint = GetClosestPointSphere(DynamicPos, StaticPos, StaticBV.Data.BS.Radius);
		}
		else
		{
			// dynamic Sphere - static AABB
			m_StaticClosestPoint = GetClosestPointAABB(DynamicPos,
				StaticPos, StaticBV.Data.AABBHalfSizes.x, StaticBV.Data.AABBHalfSizes.y, StaticBV.Data.AABBHalfSizes.z);
			m_DynamicClosestPoint = GetClosestPointSphere(m_StaticClosestPoint, DynamicPos, DynamicBV.Data.BS.Radius);
		}
	}
	else
	{
		if (StaticBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			// dynamic AABB - static Sphere
			m_DynamicClosestPoint = GetClosestPointAABB(StaticPos, 
				DynamicPos, DynamicBV.Data.AABBHalfSizes.x, DynamicBV.Data.AABBHalfSizes.y, DynamicBV.Data.AABBHalfSizes.z);
			m_StaticClosestPoint = GetClosestPointSphere(m_DynamicClosestPoint, StaticPos, StaticBV.Data.BS.Radius);
		}
		else
		{
			// dynamic AABB - static AABB
			m_DynamicClosestPoint = GetClosestPointAABB(StaticPos,
				DynamicPos, DynamicBV.Data.AABBHalfSizes.x, DynamicBV.Data.AABBHalfSizes.y, DynamicBV.Data.AABBHalfSizes.z);
			m_StaticClosestPoint = GetClosestPointAABB(DynamicPos, 
				StaticPos, StaticBV.Data.AABBHalfSizes.x, StaticBV.Data.AABBHalfSizes.y, StaticBV.Data.AABBHalfSizes.z);
		}
	}
}

const XMVECTOR& CPhysicsEngine::GetDynamicClosestPoint() const
{
	return m_DynamicClosestPoint;
}

const XMVECTOR& CPhysicsEngine::GetStaticClosestPoint() const
{
	return m_StaticClosestPoint;
}
