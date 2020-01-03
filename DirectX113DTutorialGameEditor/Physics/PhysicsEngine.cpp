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

	if (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end())
	{
		m_mapEnvironmentObjects.erase(Object3D);
	}
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

	if (m_mapEnvironmentObjects.find(Object3D) != m_mapEnvironmentObjects.end()) return;

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

	if (m_mapMonsterObjects.find(Object3D) != m_mapMonsterObjects.end()) return;

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
				if (DetectRayObjectIntersection(RayOrigin, RayDirection, Instance.Translation,
					EnvironmentObject->GetEditorBoundingSphere(), EnvironmentObject->ComponentPhysics.vBoundingVolumes, TCmp))
				{
					bIntersected = true;
					m_PickedObject = EnvironmentObject;
					break;
				}
			}
		}
		else
		{
			if (DetectRayObjectIntersection(RayOrigin, RayDirection, EnvironmentObject->ComponentTransform.Translation,
				EnvironmentObject->GetEditorBoundingSphere(), EnvironmentObject->ComponentPhysics.vBoundingVolumes, TCmp))
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

const XMVECTOR& CPhysicsEngine::GetPickedPoint() const
{
	return m_PickedPoint;
}

CObject3D* CPhysicsEngine::GetPickedObject() const
{
	return m_PickedObject;
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

void CPhysicsEngine::Update(float DeltaTime)
{
	if (DeltaTime <= 0) return;

	if (m_PlayerObject)
	{
		if (m_bShouldApplyGravity)
		{
			m_PlayerObject->ComponentPhysics.LinearAcceleration += m_Gravity; // Gravity
		}

		m_PlayerObject->ComponentPhysics.LinearVelocity += m_PlayerObject->ComponentPhysics.LinearAcceleration * DeltaTime;
		m_PlayerObject->ComponentTransform.Translation += m_PlayerObject->ComponentPhysics.LinearVelocity * DeltaTime;
		m_PlayerObject->ComponentPhysics.LinearAcceleration = KVectorZero;

		if (XMVectorGetY(m_PlayerObject->ComponentTransform.Translation) < m_WorldFloorHeight)
		{
			m_PlayerObject->ComponentTransform.Translation = XMVectorSetY(m_PlayerObject->ComponentTransform.Translation, m_WorldFloorHeight);
			m_PlayerObject->ComponentPhysics.LinearVelocity = XMVectorSetY(m_PlayerObject->ComponentPhysics.LinearVelocity, 0.0f);
		}
		else
		{
			DetectCollisions();
		}
	}
}

bool CPhysicsEngine::DetectCollisions()
{
	if (!m_PlayerObject) return false;
	
	bool bCollisionDetected{ false };

	//
	m_DynamicClosestPoint = m_StaticClosestPoint = KVectorZero;
	m_vCoarseCollisionList.clear();

	// PvE
	// @important: Player is dynamic && Environment is static
	{
		CObject3D* const A{ m_PlayerObject };

		for (auto& EnvironmentObject : m_vEnvironmentObjects)
		{
			CObject3D* const B{ EnvironmentObject };

			if (B->IsInstanced())
			{
				for (const auto& InstanceCPUData : B->GetInstanceCPUDataVector())
				{
					// Sphere-Sphere
					if (DetectIntersection(
						A->ComponentTransform.Translation + A->GetEditorBoundingSphereCenterOffset(), A->GetEditorBoundingSphere(),
						InstanceCPUData.Translation + InstanceCPUData.EditorBoundingSphere.Center, InstanceCPUData.EditorBoundingSphere))
					{
						XMVECTOR Diff{ InstanceCPUData.Translation + InstanceCPUData.EditorBoundingSphere.Center -
							A->ComponentTransform.Translation + A->GetEditorBoundingSphereCenterOffset() };

						m_vCoarseCollisionList.emplace_back();
						m_vCoarseCollisionList.back().B = B;
						m_vCoarseCollisionList.back().BTranslation = InstanceCPUData.Translation;
						m_vCoarseCollisionList.back().DistanceSquare = XMVectorGetX(XMVector3LengthSq(Diff));
					}
				}
			}
			else
			{
				// Sphere-Sphere
				if (DetectIntersection(
					A->ComponentTransform.Translation + A->GetEditorBoundingSphereCenterOffset(), A->GetEditorBoundingSphere(),
					B->ComponentTransform.Translation + B->GetEditorBoundingSphereCenterOffset(), B->GetEditorBoundingSphere()))
				{
					XMVECTOR Diff{ B->ComponentTransform.Translation + B->GetEditorBoundingSphereCenterOffset() -
							A->ComponentTransform.Translation + A->GetEditorBoundingSphereCenterOffset() };

					m_vCoarseCollisionList.emplace_back();
					m_vCoarseCollisionList.back().B = B;
					m_vCoarseCollisionList.back().BTranslation = B->ComponentTransform.Translation;
					m_vCoarseCollisionList.back().DistanceSquare = XMVectorGetX(XMVector3LengthSq(Diff));
				}
			}
		}
		
		sort(m_vCoarseCollisionList.begin(), m_vCoarseCollisionList.end(), std::less<SCoarseCollisionList>());
		for (const auto& CoarseCollision : m_vCoarseCollisionList)
		{
			if (DetectFineCollisionDynamicStatic(
				A, A->ComponentTransform.Translation,
				A->GetEditorBoundingSphere(), A->ComponentPhysics.vBoundingVolumes,
				CoarseCollision.B, CoarseCollision.BTranslation, 
				CoarseCollision.B->GetEditorBoundingSphere(), CoarseCollision.B->ComponentPhysics.vBoundingVolumes))
			{
				// Final collision
				bCollisionDetected = true;
			}
		}
	}

	return bCollisionDetected;
}

bool CPhysicsEngine::DetectFineCollisionDynamicStatic(
	CObject3D* const DynamicObject, const XMVECTOR& DynamicPos, const SBoundingVolume& DynamicBV, const std::vector<SBoundingVolume>& vDynamicBVs,
	CObject3D* const StaticObject, const XMVECTOR& StaticPos, const SBoundingVolume& StaticBV, const std::vector<SBoundingVolume>& vStaticBVs)
{
	XMVECTOR _DynamicPos{};
	XMVECTOR _StaticPos{};

	if (vStaticBVs.empty())
	{
		if (vDynamicBVs.empty())
		{
			_DynamicPos = XMVectorSetW(DynamicPos + DynamicBV.Center, 1);
			_StaticPos = XMVectorSetW(StaticPos + StaticBV.Center, 1);

			if (DetectIntersection(_DynamicPos, DynamicBV, _StaticPos, StaticBV))
			{
				GetClosestPoints(_DynamicPos, DynamicBV, _StaticPos, StaticBV);
				
				ResolvePenetration(DynamicObject, _DynamicPos, DynamicBV, StaticObject, _StaticPos, StaticBV);

				return true;
			}
		}
		else
		{
			_StaticPos = XMVectorSetW(StaticPos + StaticBV.Center, 1);

			for (const auto& DynamicBVElement : vDynamicBVs)
			{
				_DynamicPos = DynamicPos + DynamicBVElement.Center;

				if (DetectIntersection(_DynamicPos, DynamicBVElement, _StaticPos, StaticBV))
				{
					GetClosestPoints(_DynamicPos, DynamicBVElement, _StaticPos, StaticBV);

					ResolvePenetration(DynamicObject, _DynamicPos, DynamicBVElement, StaticObject, _StaticPos, StaticBV);

					return true;
				}
			}
		}
	}
	else
	{
		if (vDynamicBVs.empty())
		{
			_DynamicPos = XMVectorSetW(DynamicPos + DynamicBV.Center, 1);

			for (const auto& StaticBVElement : vStaticBVs)
			{
				_StaticPos = XMVectorSetW(StaticPos + StaticBVElement.Center, 1);

				if (DetectIntersection(_DynamicPos, DynamicBV, _StaticPos, StaticBVElement))
				{
					GetClosestPoints(_DynamicPos, DynamicBV, _StaticPos, StaticBVElement);

					ResolvePenetration(DynamicObject, _DynamicPos, DynamicBV, StaticObject, _StaticPos, StaticBVElement);

					return true;
				}
			}
		}
		else
		{
			for (const auto& DynamicBVElement : vDynamicBVs)
			{
				_DynamicPos = XMVectorSetW(DynamicPos + DynamicBVElement.Center, 1);

				for (const auto& StaticBVElement : vStaticBVs)
				{
					_StaticPos = XMVectorSetW(StaticPos + StaticBVElement.Center, 1);
					
					if (DetectIntersection(_DynamicPos, DynamicBVElement, _StaticPos, StaticBVElement))
					{
						GetClosestPoints(_DynamicPos, DynamicBVElement, _StaticPos, StaticBVElement);

						ResolvePenetration(DynamicObject, _DynamicPos, DynamicBVElement, StaticObject, _StaticPos, StaticBVElement);

						return true;
					}
				}
			}
		}
	}

	return false;
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

void CPhysicsEngine::ResolvePenetration(
	CObject3D* const DynamicObject, const XMVECTOR& DynamicPos, const SBoundingVolume& DynamicBV, 
	CObject3D* const StaticObject, const XMVECTOR& StaticPos, const SBoundingVolume& StaticBV)
{
	XMVECTOR DynamicObjectMovingDir{ XMVector3Normalize(DynamicObject->ComponentPhysics.LinearVelocity) };
	XMVECTOR DynamicToStatic{ StaticPos - DynamicPos };

	if (DynamicBV.eType == EBoundingVolumeType::BoundingSphere)
	{
		if (StaticBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			// Sphere - Sphere

			// C == A - B
			// D == direction of A
			// |C - xD| = r©û + r©ü
			//
			// (C - xD)¡¤(C - xD) = (r©û + r©ü)©÷
			// C¡¤C - 2xC¡¤D + x©÷D¡¤D = (r©û + r©ü)©÷
			// D¡¤Dx©÷ - 2C¡¤Dx + C¡¤C - (r©û - r©ü)©÷ = 0

			XMVECTOR C{ -DynamicToStatic };
			XMVECTOR D{ DynamicObjectMovingDir };
			float r1{ DynamicBV.Data.BS.Radius };
			float r2{ StaticBV.Data.BS.Radius };
			float radii{ r1 + r2 };

			float a{ XMVectorGetX(XMVector3Dot(D, D)) }; // TODO: optimization
			float b{ -2.0f * XMVectorGetX(XMVector3Dot(C, D)) };
			float c{ XMVectorGetX(XMVector3Dot(C, C)) - radii * radii };
			float discriminant{ b * b - 4.0f * a * c };

			if (discriminant > 0)
			{
				float x_bigger{ (-b + sqrt(discriminant)) / (2.0f * a) };
				m_PenetrationDepth = abs(x_bigger);

				XMVECTOR Resolution{ -DynamicObjectMovingDir * x_bigger };
				DynamicObject->ComponentTransform.Translation += Resolution;
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
				DynamicObject->ComponentPhysics.LinearVelocity = XMVectorSetY(DynamicObject->ComponentPhysics.LinearVelocity, 0);
			}
			DynamicObject->ComponentTransform.Translation += Resolution;
		}
	}
	else
	{
		if (StaticBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			// dynamic AABB - static Sphere

			XMVECTOR Diff{ m_DynamicClosestPoint - StaticPos };
			float Distance{ XMVectorGetX(XMVector3Length(Diff)) };
			XMVECTOR N{ XMVector3Normalize(Diff) };

			m_PenetrationDepth = StaticBV.Data.BS.Radius - Distance;
			XMVECTOR Resolution{ m_PenetrationDepth * N };
			DynamicObject->ComponentTransform.Translation += Resolution;
		}
		else
		{
			// AABB - AABB

			XMVECTOR Diff{ m_StaticClosestPoint - m_DynamicClosestPoint };
			XMVECTOR N{ GetAABBAABBCollisionNormal(DynamicObjectMovingDir, m_DynamicClosestPoint,
				StaticPos, StaticBV.Data.AABBHalfSizes.x, StaticBV.Data.AABBHalfSizes.y, StaticBV.Data.AABBHalfSizes.z) };

			XMVECTOR Resolution{ XMVector3Dot(Diff, N) * N };
			m_PenetrationDepth = XMVectorGetX(XMVector3Length(Resolution));
			if (XMVectorGetY(N) == +1.0f)
			{
				DynamicObject->ComponentPhysics.LinearVelocity = XMVectorSetY(DynamicObject->ComponentPhysics.LinearVelocity, 0);
			}
			DynamicObject->ComponentTransform.Translation += Resolution;
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
