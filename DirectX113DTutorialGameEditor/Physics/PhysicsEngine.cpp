#include "PhysicsEngine.h"
#include "../Core/Object3D.h"
#include "../Core/Math.h"

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

	m_WorldFloorHeight = 0;
}

void CPhysicsEngine::SetWorldFloorHeight(float Value)
{
	m_WorldFloorHeight = Value;
}

float CPhysicsEngine::GetWorldFloorHeight() const
{
	return m_WorldFloorHeight;
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

void CPhysicsEngine::Update(float DeltaTime)
{
	if (m_PlayerObject)
	{
		if (m_bShouldApplyGravity)
		{
			m_PlayerObject->ComponentPhysics.LinearAcceleration += XMVectorSet(0, -10.0f, 0, 0); // Gravity
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
					if (DetectIntersection(
						A->ComponentTransform.Translation + A->GetEditorBoundingSphereCenterOffset(), A->GetEditorBoundingSphere(),
						InstanceCPUData.Translation + InstanceCPUData.EditorBoundingSphere.Center, InstanceCPUData.EditorBoundingSphere))
					{
						if (DetectFineCollisionDynamicStatic(
							A, A->ComponentTransform.Translation, A->GetEditorBoundingSphere(), A->ComponentPhysics.vBoundingVolumes,
							B, InstanceCPUData.Translation, B->GetEditorBoundingSphere(), B->ComponentPhysics.vBoundingVolumes))
						{
							// Final collision
							bCollisionDetected = true;

							A->ComponentPhysics.LinearVelocity = KVectorZero;
						}
					}
				}
			}
			else
			{
				if (DetectIntersection(
					A->ComponentTransform.Translation + A->GetEditorBoundingSphereCenterOffset(), A->GetEditorBoundingSphere(),
					B->ComponentTransform.Translation, B->GetEditorBoundingSphere()))
				{
					if (DetectFineCollisionDynamicStatic(
						A, A->ComponentTransform.Translation, A->GetEditorBoundingSphere(), A->ComponentPhysics.vBoundingVolumes,
						B, B->ComponentTransform.Translation, B->GetEditorBoundingSphere(), B->ComponentPhysics.vBoundingVolumes))
					{
						// Final collision
						bCollisionDetected = true;

						A->ComponentPhysics.LinearVelocity = KVectorZero;
					}
				}
			}
		}
	}

	return bCollisionDetected;
}

// Sphere - Sphere: closest point on any Sphere to the other ... is it necessary???
// Sphere - AABB: closest point on AABB to Sphere ¡Ú¡Ú
// AABB - AABB: to each center ... is it even necessary to calculate closest points in this case??? or just interval test...?
XMVECTOR GetClosestPointOfAToB(const XMVECTOR& ACenter, const SBoundingVolume& ABV, const XMVECTOR& BCenter, const SBoundingVolume& BBV)
{
	if (ABV.eType == EBoundingVolumeType::BoundingSphere)
	{
		if (BBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			return GetClosestPointSphere(BCenter, ACenter, ABV.Data.BS.Radius);
		}
		else
		{
			return GetClosestPointAABB(ACenter, BCenter, BBV.Data.AABBHalfSizes.x, BBV.Data.AABBHalfSizes.y, BBV.Data.AABBHalfSizes.z);
		}
	}
	else
	{
		return GetClosestPointAABB(BCenter, ACenter, ABV.Data.AABBHalfSizes.x, ABV.Data.AABBHalfSizes.y, ABV.Data.AABBHalfSizes.z);
	}
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
				m_DynamicClosestPoint = GetClosestPointOfAToB(_StaticPos, StaticBV, _DynamicPos, DynamicBV);
				m_StaticClosestPoint = GetClosestPointOfAToB(_DynamicPos, DynamicBV, _StaticPos, StaticBV);

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
					m_DynamicClosestPoint = GetClosestPointOfAToB(_StaticPos, StaticBV, _DynamicPos, DynamicBVElement);
					m_StaticClosestPoint = GetClosestPointOfAToB(_DynamicPos, DynamicBVElement, _StaticPos, StaticBV);

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
					m_DynamicClosestPoint = GetClosestPointOfAToB(_StaticPos, StaticBVElement, _DynamicPos, DynamicBV);
					m_StaticClosestPoint = GetClosestPointOfAToB(_DynamicPos, DynamicBV, _StaticPos, StaticBVElement);

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
						m_DynamicClosestPoint = GetClosestPointOfAToB(_StaticPos, StaticBVElement, _DynamicPos, DynamicBVElement);
						m_StaticClosestPoint = GetClosestPointOfAToB(_DynamicPos, DynamicBVElement, _StaticPos, StaticBVElement);

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

			// C == A - ClosestPoint
			// D == direction of A
			// |C - xD| = r
			//
			// (C - xD)¡¤(C - xD) = r©÷
			// C¡¤C - 2xC¡¤D + x©÷D¡¤D = r©÷
			// D¡¤Dx©÷ - 2C¡¤Dx + C¡¤C - r©÷ = 0
			
			XMVECTOR C{ DynamicPos - m_StaticClosestPoint };
			XMVECTOR D{ DynamicObjectMovingDir };
			float r{ DynamicBV.Data.BS.Radius };

			float a{ XMVectorGetX(XMVector3Dot(D, D)) };
			float b{ -2.0f * XMVectorGetX(XMVector3Dot(C, D)) };
			float c{ XMVectorGetX(XMVector3Dot(C, C)) - r * r };
			float discriminant{ b * b - 4.0f * a * c };

			if (discriminant > 0)
			{
				float x_bigger{ (-b + sqrt(discriminant)) / (2.0f * a) };
				m_PenetrationDepth = abs(x_bigger);

				XMVECTOR Resolution{ -DynamicObjectMovingDir * x_bigger };
				DynamicObject->ComponentTransform.Translation += Resolution;
			}
		}
	}
	else
	{
		if (StaticBV.eType == EBoundingVolumeType::BoundingSphere)
		{
			// dynamic AABB - static Sphere

			XMVECTOR C{ m_DynamicClosestPoint - StaticPos };
			XMVECTOR D{ DynamicObjectMovingDir };
			float r{ StaticBV.Data.BS.Radius };

			float a{ XMVectorGetX(XMVector3Dot(D, D)) };
			float b{ -2.0f * XMVectorGetX(XMVector3Dot(C, D)) };
			float c{ XMVectorGetX(XMVector3Dot(C, C)) - r * r };
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
			// AABB - AABB

			// Rd = resolution direction
			// Px = penetration on x axis
			// Py = penetration on y axis
			// Pz = penetration on z axis
			// Cp = component-wise production

			XMVECTOR Rd{ -DynamicObjectMovingDir };
			XMVECTOR P{ m_StaticClosestPoint - m_DynamicClosestPoint };
			float Px_abs{ abs(XMVectorGetX(P)) };
			float Py_abs{ abs(XMVectorGetY(P)) };
			float Pz_abs{ abs(XMVectorGetZ(P)) };

			XMVECTOR Cp{ Rd * P };
			float Cpx{ XMVectorGetX(Cp) };
			float Cpy{ XMVectorGetY(Cp) };
			float Cpz{ XMVectorGetZ(Cp) };

			XMVECTOR Resolution{};
			if (Cpx < 0)
			{
				float Rdx_abs{ abs(XMVectorGetX(Rd)) };
				Resolution = Rd / Rdx_abs * Px_abs;
			}
			else if (Cpy < 0)
			{
				float Rdy_abs{ abs(XMVectorGetY(Rd)) };
				Resolution = Rd / Rdy_abs * Py_abs;
			}
			else if (Cpz < 0)
			{
				float Rdz_abs{ abs(XMVectorGetZ(Rd)) };
				Resolution = Rd / Rdz_abs * Pz_abs;
			}
			m_PenetrationDepth = XMVectorGetX(XMVector3Length(Resolution));
			DynamicObject->ComponentTransform.Translation += Resolution;
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
