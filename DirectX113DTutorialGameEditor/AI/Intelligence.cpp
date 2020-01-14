#include "Intelligence.h"
#include "../Core/Object3D.h"

using std::swap;
using std::string;
using std::to_string;

static std::string GetIdentifierString(const SObjectIdentifier& Identifier)
{
	return (to_string((size_t)Identifier.Object3D) + to_string((size_t)Identifier.PtrInstanceName));
}

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

void CIntelligence::RegisterPriority(const SObjectIdentifier& Identifier, EObjectPriority ePriority, bool bShouldChangePriority)
{
	size_t NewPriority{ (size_t)ePriority };

	string IdentifierString{ GetIdentifierString(Identifier) };

	if (bShouldChangePriority)
	{
		if (m_umapPriority.find(IdentifierString) != m_umapPriority.end())
		{
			size_t OldPriority{ m_umapPriority.at(IdentifierString) };
			if (OldPriority == NewPriority) return; // @important: early out

			size_t iOldBehaviorSet{ m_umapBehaviorSets[OldPriority].at(IdentifierString) };
			if (iOldBehaviorSet < m_vBehaviorSets[OldPriority].size() - 1)
			{
				swap(m_vBehaviorSets[OldPriority][iOldBehaviorSet], m_vBehaviorSets[OldPriority].back());
			}
			
			m_vBehaviorSets[OldPriority].pop_back();
			m_umapBehaviorSets[OldPriority].erase(IdentifierString);
			m_umapPriority.erase(IdentifierString);
		}
	}
	else
	{
		if (m_umapPriority.find(IdentifierString) != m_umapPriority.end()) return; // @important: early out
	}
	
	m_umapPriority[IdentifierString] = NewPriority;
	m_vBehaviorSets[NewPriority].emplace_back();
	m_vBehaviorSets[NewPriority].back().Identifier = Identifier;
	m_umapBehaviorSets[NewPriority][IdentifierString] = m_vBehaviorSets[NewPriority].size() - 1;
}

void CIntelligence::PushBackBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior)
{
	// @important
	// if not registered, register with the lowest priority
	RegisterPriority(Identifier, EObjectPriority::C_Trivial);

	size_t Priority{ m_umapPriority.at(GetIdentifierString(Identifier)) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(GetIdentifierString(Identifier)) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	BehaviorSet.dqBehaviors.emplace_back(Behavior);
}

void CIntelligence::PushFrontBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior)
{
	// @important
	// if not registered, register with the lowest priority
	RegisterPriority(Identifier, EObjectPriority::C_Trivial);

	string IdentifierString{ GetIdentifierString(Identifier) };

	size_t Priority{ m_umapPriority.at(IdentifierString) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(IdentifierString) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	BehaviorSet.dqBehaviors.emplace_front(Behavior);
}

void CIntelligence::PopFrontBehavior(const SObjectIdentifier& Identifier)
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	assert(m_umapPriority.find(IdentifierString) != m_umapPriority.end());

	size_t Priority{ m_umapPriority.at(IdentifierString) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(IdentifierString) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	if (BehaviorSet.dqBehaviors.size()) BehaviorSet.dqBehaviors.pop_front();
}

void CIntelligence::PopFrontBehaviorIf(const SObjectIdentifier& Identifier, EBehaviorType eBehaviorType)
{
	if (HasBehavior(Identifier))
	{
		if (PeekBehavior(Identifier).eBehaviorType == eBehaviorType)
		{
			PopFrontBehavior(Identifier);
		}
	}
}

bool CIntelligence::HasBehavior(const SObjectIdentifier& Identifier) const
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	if (m_umapPriority.find(IdentifierString) == m_umapPriority.end()) return false;

	size_t Priority{ m_umapPriority.at(IdentifierString) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(IdentifierString) };
	auto& BehaviorSet{ m_vBehaviorSets[Priority][iBehaviorSet] };
	if (BehaviorSet.dqBehaviors.empty()) return false;

	return true;
}

const SBehaviorData& CIntelligence::PeekBehavior(const SObjectIdentifier& Identifier) const
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	assert(m_umapPriority.find(IdentifierString) != m_umapPriority.end());

	size_t Priority{ m_umapPriority.at(IdentifierString) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(IdentifierString) };
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
				ExecuteBehavior(BehaviorSet.Identifier, BehaviorSet.dqBehaviors.front());

				if (BehaviorSet.dqBehaviors.front().eStatus == SBehaviorData::EStatus::Done)
				{
					BehaviorSet.dqBehaviors.pop_front();
				}
			}
		}
	}
}

void CIntelligence::ExecuteBehavior(const SObjectIdentifier& Identifier, SBehaviorData& Behavior)
{
	static constexpr XMVECTOR KNegativeZAxis{ 0, 0, -1.0f, 0 };
	
	if (Behavior.eStatus == SBehaviorData::EStatus::Waiting)
	{
		Behavior.eStatus = SBehaviorData::EStatus::Entering;
		m_bBehaviorStarted = false; // @important
	}

	switch (Behavior.eBehaviorType)
	{
	case EBehaviorType::WalkTo:
	{
		if (Behavior.eStatus == SBehaviorData::EStatus::Entering)
		{
			Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Walking);
		}

		const XMVECTOR& DestinationXZ{ Behavior.Vector };
		const XMVECTOR& PlayerXZ{ XMVectorSetY(Identifier.Object3D->GetTransform(Identifier).Translation, 0) };
		XMVECTOR Diff{ PlayerXZ - DestinationXZ };
		float Distance{ XMVectorGetX(XMVector3Length(Diff)) };
		if (Distance < 0.1f) // @important
		{
			Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorZero());
			Behavior.eStatus = SBehaviorData::EStatus::Done;
		}
		else
		{
			const XMVECTOR& Direction{ XMVector3Normalize(DestinationXZ - PlayerXZ) };
			float OldY{ XMVectorGetY(Identifier.Object3D->GetPhysics(Identifier).LinearVelocity) };
			Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorSetY(Direction * Behavior.Factor, OldY));

			XMVECTOR DirectionXY{ XMVectorSetY(Direction, 0) };
			float Dot{ XMVectorGetX(XMVector3Dot(DirectionXY, KNegativeZAxis)) };
			float CrossY{ XMVectorGetY(XMVector3Cross(DirectionXY, KNegativeZAxis)) };
			float Yaw{ acos(Dot) };
			if (CrossY > 0) Yaw = XM_2PI - Yaw;
			
			Identifier.Object3D->RotateYawTo(Identifier, Yaw);
		}
		break;
	}
	case EBehaviorType::Jump:
		if (Behavior.eStatus == SBehaviorData::EStatus::Entering)
		{
			Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Jumping, EAnimationOption::PlayToLastFrame);

			m_SavedVectorXZ = XMVectorSetY(Identifier.Object3D->GetPhysics(Identifier).LinearVelocity, 0);
			Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorZero());
		}
		else
		{
			if (!m_bBehaviorStarted)
			{
				if (Identifier.Object3D->GetAnimationTick(Identifier) >= Identifier.Object3D->GetCurrentAnimationBehaviorStartTick(Identifier))
				{
					Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorSetY(m_SavedVectorXZ, Behavior.Factor));
					m_bBehaviorStarted = true;
				}
			}
			else
			{
				if (XMVectorGetY(m_SavedVector) <= XMVectorGetY(Identifier.Object3D->GetPhysics(Identifier).LinearVelocity))
				{
					Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Landing, EAnimationOption::PlayToLastFrame);
					Behavior.eStatus = SBehaviorData::EStatus::Done;

					float Y{ XMVectorGetY(Identifier.Object3D->GetPhysics(Identifier).LinearVelocity) };
					Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorSet(0, Y, 0, 0));
				}
			}
		}
		m_SavedVector = Identifier.Object3D->GetPhysics(Identifier).LinearVelocity;
		break;
	default:
		Behavior.eStatus = SBehaviorData::EStatus::Done; // for safety issue
		break;
	}

	if (Behavior.eStatus == SBehaviorData::EStatus::Entering) Behavior.eStatus = SBehaviorData::EStatus::Processing;
	if (Behavior.eStatus == SBehaviorData::EStatus::Done)
	{
		Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Idle, EAnimationOption::Repeat,
			Identifier.Object3D->IsCurrentAnimationRegisteredAs(Identifier, EAnimationRegistrationType::Walking));
	}
}
