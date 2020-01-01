#include "Intelligence.h"
#include "../Core/Object3D.h"

using std::swap;

CIntelligence::CIntelligence(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CIntelligence::~CIntelligence()
{
}

void CIntelligence::ClearBehaviors()
{
	for (size_t iPriority = 0; iPriority < KPriorityCount; ++iPriority)
	{
		for (auto& BehaviorSet : m_vBehaviorSets[iPriority])
		{
			BehaviorSet.dqBehaviors.clear();
		}
	}
}

void CIntelligence::RegisterPriority(CObject3D* const Object3D, EBehaviorPriority ePriority, bool bShouldChangePriority)
{
	assert(Object3D);

	size_t NewPriority{ (size_t)ePriority };

	if (bShouldChangePriority)
	{
		if (m_umapPriority.find(Object3D) != m_umapPriority.end())
		{
			size_t OldPriority{ m_umapPriority.at(Object3D) };
			if (OldPriority == NewPriority) return; // @important: early out

			size_t iOldBehaviorSet{ m_umapBehaviorSets[OldPriority].at(Object3D) };
			if (iOldBehaviorSet < m_vBehaviorSets[OldPriority].size() - 1)
			{
				swap(m_vBehaviorSets[OldPriority][iOldBehaviorSet], m_vBehaviorSets[OldPriority].back());
			}
			
			m_vBehaviorSets[OldPriority].pop_back();
			m_umapBehaviorSets[OldPriority].erase(Object3D);
			m_umapPriority.erase(Object3D);
		}
	}
	else
	{
		if (m_umapPriority.find(Object3D) != m_umapPriority.end()) return; // @important: early out
	}
	
	m_umapPriority[Object3D] = NewPriority;
	m_vBehaviorSets[NewPriority].emplace_back();
	m_vBehaviorSets[NewPriority].back().Object3D = Object3D;
	m_umapBehaviorSets[NewPriority][Object3D] = m_vBehaviorSets[NewPriority].size() - 1;
}

void CIntelligence::PushBackBehavior(CObject3D* const Object3D, const SBehaviorData& Behavior)
{
	assert(Object3D);

	// @important
	// if not registered, register with the lowest priority
	RegisterPriority(Object3D, EBehaviorPriority::C_Trivial);

	size_t Priority{ m_umapPriority.at(Object3D) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(Object3D) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	BehaviorSet.dqBehaviors.emplace_back(Behavior);
}

void CIntelligence::PushFrontBehavior(CObject3D* const Object3D, const SBehaviorData& Behavior)
{
	assert(Object3D);

	// @important
	// if not registered, register with the lowest priority
	RegisterPriority(Object3D, EBehaviorPriority::C_Trivial);

	size_t Priority{ m_umapPriority.at(Object3D) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(Object3D) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	BehaviorSet.dqBehaviors.emplace_front(Behavior);
}

void CIntelligence::PopFrontBehavior(CObject3D* const Object3D)
{
	assert(m_umapPriority.find(Object3D) != m_umapPriority.end());

	size_t Priority{ m_umapPriority.at(Object3D) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(Object3D) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	if (BehaviorSet.dqBehaviors.size()) BehaviorSet.dqBehaviors.pop_front();
}

void CIntelligence::PopFrontBehaviorIf(CObject3D* const Object3D, EBehaviorType eBehaviorType)
{
	if (HasBehavior(Object3D))
	{
		if (PeekBehavior(Object3D).eBehaviorType == eBehaviorType)
		{
			PopFrontBehavior(Object3D);
		}
	}
}

bool CIntelligence::HasBehavior(CObject3D* const Object3D) const
{
	if (m_umapPriority.find(Object3D) == m_umapPriority.end()) return false;

	size_t Priority{ m_umapPriority.at(Object3D) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(Object3D) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	if (BehaviorSet.dqBehaviors.empty()) return false;

	return true;
}

const SBehaviorData& CIntelligence::PeekBehavior(CObject3D* const Object3D) const
{
	assert(m_umapPriority.find(Object3D) != m_umapPriority.end());

	size_t Priority{ m_umapPriority.at(Object3D) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(Object3D) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	assert(BehaviorSet.dqBehaviors.size());

	return BehaviorSet.dqBehaviors.front();
}

void CIntelligence::Execute()
{
	for (size_t iPriority = 0; iPriority < KPriorityCount; ++iPriority)
	{
		for (auto& BehaviorSet : m_vBehaviorSets[iPriority])
		{
			if (BehaviorSet.dqBehaviors.size())
			{
				ExecuteBehavior(BehaviorSet.Object3D, BehaviorSet.dqBehaviors.front());
				if (BehaviorSet.dqBehaviors.front().bDone)
				{
					BehaviorSet.dqBehaviors.pop_front();
				}
			}
		}
	}
}

void CIntelligence::ExecuteBehavior(CObject3D* const Object3D, SBehaviorData& Behavior)
{
	switch (Behavior.eBehaviorType)
	{
	case EBehaviorType::WalkTo:
	{
		const XMVECTOR& DestinationXZ{ Behavior.Vector };
		const XMVECTOR& PlayerXZ{ XMVectorSetY(Object3D->ComponentTransform.Translation, 0) };
		XMVECTOR Diff{ PlayerXZ - DestinationXZ };
		float Distance{ XMVectorGetX(XMVector3Length(Diff)) };
		if (Distance < 0.1f) // @important
		{
			Object3D->ComponentPhysics.LinearVelocity = XMVectorZero();
			Behavior.bDone = true;
		}
		else
		{
			const XMVECTOR& Direction{ XMVector3Normalize(DestinationXZ - PlayerXZ) };
			float OldY{ XMVectorGetY(Object3D->ComponentPhysics.LinearVelocity) };
			Object3D->ComponentPhysics.LinearVelocity = XMVectorSetY(Direction * Behavior.Factor, OldY);
		}
		break;
	}
	default:
		Behavior.bDone = true; // for safety issue
		break;
	}
}
