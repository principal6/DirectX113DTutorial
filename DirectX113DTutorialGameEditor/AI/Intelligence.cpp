#include "Intelligence.h"
#include "../Core/Object3D.h"
#include "../Physics/PhysicsEngine.h"
#include "../Core/Math.h"
#include "SyntaxTree.h"

using std::swap;
using std::string;
using std::to_string;

static constexpr XMVECTOR KNegativeZAxis{ 0, 0, -1.0f, 0 };

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

void CIntelligence::LinkPhysicsEngine(CPhysicsEngine* PhysicsEngine)
{
	assert(PhysicsEngine);
	m_PhysicsEngine = PhysicsEngine;
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

void CIntelligence::ClearBehavior(const SObjectIdentifier& Identifier)
{
	while (HasBehavior(Identifier))
	{
		PopFrontBehavior(Identifier);
	}
}

void CIntelligence::PushBackBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior)
{
	// @important
	// if not registered, register with the lowest priority
	RegisterPriority(Identifier, EObjectPriority::C_Trivial);

	auto& BehaviorSet{ GetBehaviorSet(Identifier) };
	BehaviorSet.dqBehaviors.emplace_back(Behavior);
}

void CIntelligence::PushFrontBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior)
{
	// @important
	// if not registered, register with the lowest priority
	RegisterPriority(Identifier, EObjectPriority::C_Trivial);

	auto& BehaviorSet{ GetBehaviorSet(Identifier) };
	BehaviorSet.dqBehaviors.emplace_front(Behavior);
}

void CIntelligence::PopFrontBehavior(const SObjectIdentifier& Identifier)
{
	auto& BehaviorSet{ GetBehaviorSet(Identifier) };
	if (BehaviorSet.dqBehaviors.size()) BehaviorSet.dqBehaviors.pop_front();
}

void CIntelligence::PopFrontBehaviorIf(const SObjectIdentifier& Identifier, EBehaviorType eBehaviorType)
{
	if (HasBehavior(Identifier))
	{
		if (PeekFrontBehavior(Identifier).eBehaviorType == eBehaviorType)
		{
			PopFrontBehavior(Identifier);
		}
	}
}

bool CIntelligence::HasBehavior(const SObjectIdentifier& Identifier) const
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	if (m_umapPriority.find(IdentifierString) == m_umapPriority.end()) return false;

	auto& BehaviorSet{ GetBehaviorSet(Identifier) };
	return BehaviorSet.dqBehaviors.size();
}

bool CIntelligence::IsFrontBehavior(const SObjectIdentifier& Identifier, EBehaviorType eBehaviorType) const
{
	if (HasBehavior(Identifier))
	{
		return (PeekFrontBehavior(Identifier).eBehaviorType == eBehaviorType);
	}
	return false;
}

const SBehaviorData& CIntelligence::PeekFrontBehavior(const SObjectIdentifier& Identifier) const
{
	auto& BehaviorSet{ GetBehaviorSet(Identifier) };
	assert(BehaviorSet.dqBehaviors.size());

	return BehaviorSet.dqBehaviors.front();
}

const SBehaviorData& CIntelligence::PeekBackBehavior(const SObjectIdentifier& Identifier) const
{
	auto& BehaviorSet{ GetBehaviorSet(Identifier) };
	assert(BehaviorSet.dqBehaviors.size());

	return BehaviorSet.dqBehaviors.back();
}

SBehaviorSet& CIntelligence::GetBehaviorSet(const SObjectIdentifier& Identifier)
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	assert(m_umapPriority.find(IdentifierString) != m_umapPriority.end());

	size_t Priority{ m_umapPriority.at(IdentifierString) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(IdentifierString) };
	return m_vBehaviorSets[Priority][iBehaviorSet];
}

const SBehaviorSet& CIntelligence::GetBehaviorSet(const SObjectIdentifier& Identifier) const
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	assert(m_umapPriority.find(IdentifierString) != m_umapPriority.end());

	size_t Priority{ m_umapPriority.at(IdentifierString) };
	size_t iBehaviorSet{ m_umapBehaviorSets[Priority].at(IdentifierString) };
	return m_vBehaviorSets[Priority][iBehaviorSet];
}

void CIntelligence::RegisterPattern(const SObjectIdentifier& Identifier, CPattern* const Pattern)
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	if (m_umapPatternInfos.find(IdentifierString) != m_umapPatternInfos.end())
	{
		// already registered

		size_t iPatternInfo{ m_umapPatternInfos.at(IdentifierString) };
		m_vPatternInfos[iPatternInfo].Pattern = Pattern;
	}
	else
	{
		// registered for the first time

		SPatternInfo PatternInfo{};
		PatternInfo.ObjectIdentifier = Identifier;
		PatternInfo.Pattern = Pattern;
		PatternInfo.PatternState.MyPosition = &Identifier.Object3D->GetTransform(Identifier).Translation;
		PatternInfo.PatternState.EnemyPosition = &m_PhysicsEngine->GetPlayerObject()->GetTransform().Translation; // @important
		m_vPatternInfos.emplace_back(PatternInfo);
		
		m_umapPatternInfos[IdentifierString] = m_vPatternInfos.size() - 1;
	}
}

bool CIntelligence::HasPattern(const SObjectIdentifier& Identifier) const
{
	string IdentifierString{ GetIdentifierString(Identifier) };
	return m_umapPatternInfos.find(IdentifierString) != m_umapPatternInfos.end();
}

CPattern* CIntelligence::GetPattern(const SObjectIdentifier& Identifier) const
{
	assert(HasPattern(Identifier));
	string IdentifierString{ GetIdentifierString(Identifier) };
	size_t iPatternInfo{ m_umapPatternInfos.at(IdentifierString) };
	
	return m_vPatternInfos[iPatternInfo].Pattern;
}

void CIntelligence::Execute()
{
	// Pattern to Behavior
	ConvertPatternsIntoBehaviors();

	// Behavior
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

void CIntelligence::ConvertPatternsIntoBehaviors()
{
	for (auto& PatternInfo : m_vPatternInfos)
	{
		auto Result{ PatternInfo.Pattern->Execute(PatternInfo.PatternState) };

		if (Result->Identifier == "WalkTo")
		{
			XMVECTOR DestVector{ XMVectorSet(
					stof(Result->vChildNodes[0]->Identifier),
					stof(Result->vChildNodes[1]->Identifier),
					stof(Result->vChildNodes[2]->Identifier),
					1) };

			if (!XMVector3Equal(m_PhysicsEngine->GetPlayerObject()->GetTransform().Translation, DestVector))
			{
				ClearBehavior(PatternInfo.ObjectIdentifier);

				SBehaviorData Behavior{};
				Behavior.eBehaviorType = EBehaviorType::WalkTo;
				Behavior.Vector = DestVector;

				PushBackBehavior(PatternInfo.ObjectIdentifier, Behavior);
			}
		}
		else if (Result->Identifier == "RotateYawTo")
		{
			XMVECTOR DestVector{ XMVectorSet(
					stof(Result->vChildNodes[0]->Identifier),
					stof(Result->vChildNodes[1]->Identifier),
					stof(Result->vChildNodes[2]->Identifier),
					1) };

			const XMVECTOR& MyPosition{ PatternInfo.ObjectIdentifier.Object3D->GetTransform(PatternInfo.ObjectIdentifier).Translation };

			XMVECTOR Direction{ XMVector3Normalize(DestVector - MyPosition) };
			XMVECTOR DirectionXY{ XMVectorSetY(Direction, 0) };
			float Dot{ XMVectorGetX(XMVector3Dot(DirectionXY, KNegativeZAxis)) };
			float CrossY{ XMVectorGetY(XMVector3Cross(DirectionXY, KNegativeZAxis)) };
			float Yaw{ acos(Dot) };
			if (CrossY > 0) Yaw = XM_2PI - Yaw;

			PatternInfo.ObjectIdentifier.Object3D->RotateYawTo(PatternInfo.ObjectIdentifier, Yaw);
		}
		else if (Result->Identifier == "Attack")
		{
			SBehaviorData Behavior{};
			Behavior.eBehaviorType = EBehaviorType::Attack;
			Behavior.Scalar = 0;

			if (!IsFrontBehavior(PatternInfo.ObjectIdentifier, EBehaviorType::Attack))
			{
				ClearBehavior(PatternInfo.ObjectIdentifier);
				const auto& LinearVelocity{ PatternInfo.ObjectIdentifier.Object3D->GetPhysics(PatternInfo.ObjectIdentifier).LinearVelocity };
				PatternInfo.ObjectIdentifier.Object3D->SetLinearVelocity(PatternInfo.ObjectIdentifier, XMVectorSet(0, XMVectorGetY(LinearVelocity), 0, 0));

				PushBackBehavior(PatternInfo.ObjectIdentifier, Behavior);
			}

			if (!HasBehavior(PatternInfo.ObjectIdentifier))
			{
				PushBackBehavior(PatternInfo.ObjectIdentifier, Behavior);
			}
		}
	}
}

void CIntelligence::ExecuteBehavior(const SObjectIdentifier& Identifier, SBehaviorData& Behavior)
{
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
			bool bIsAlreadyAnimated{ Identifier.Object3D->GetRegisteredAnimationType(
				Identifier.Object3D->GetAnimationID(Identifier)) == EAnimationRegistrationType::Walking };
			
			if (Behavior.bIsPlayer)
			{
				Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Walking);
			}
			else
			{
				if (!bIsAlreadyAnimated) Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Walking);
			}
		}

		const XMVECTOR& DestinationXZ{ Behavior.Vector };
		const XMVECTOR& MyXZ{ XMVectorSetY(Identifier.Object3D->GetTransform(Identifier).Translation, 0) };
		XMVECTOR Diff{ DestinationXZ - MyXZ };
		float Distance{ XMVectorGetX(XMVector3Length(Diff)) };
		if (Distance < 0.25f) // @important (stop distance)
		{
			Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorZero());
			Behavior.eStatus = SBehaviorData::EStatus::Done;
		}
		else
		{
			XMVECTOR Direction{ XMVector3Normalize(Diff) };
			float OldY{ XMVectorGetY(Identifier.Object3D->GetPhysics(Identifier).LinearVelocity) };
			Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorSetY(Direction * Behavior.Scalar, OldY));

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
					Identifier.Object3D->SetLinearVelocity(Identifier, XMVectorSetY(m_SavedVectorXZ, Behavior.Scalar));
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
	case EBehaviorType::Attack:
		if (Behavior.eStatus == SBehaviorData::EStatus::Entering)
		{
			EAnimationRegistrationType eOldType{ Identifier.Object3D->GetRegisteredAnimationType(Identifier.Object3D->GetAnimationID(Identifier)) };
			EAnimationRegistrationType eNewType{};
			if (Behavior.Scalar == 0.0f) eNewType = EAnimationRegistrationType::AttackingA;
			if (Behavior.Scalar == 1.0f) eNewType = EAnimationRegistrationType::AttackingB;

			bool bIsAlreadyAnimated{ eOldType == eNewType };
			if (Identifier.Object3D->GetAnimationPlayCount(Identifier) >= 1) bIsAlreadyAnimated = false;

			if (Behavior.bIsPlayer)
			{
				Identifier.Object3D->SetAnimation(Identifier, eNewType, EAnimationOption::PlayToLastFrame);
			}
			else
			{
				if (!bIsAlreadyAnimated) Identifier.Object3D->SetAnimation(Identifier, eNewType, EAnimationOption::PlayToLastFrame);
			}
		}
		else
		{
			if (Identifier.Object3D->GetAnimationPlayCount(Identifier) >= 1)
			{
				Behavior.eStatus = SBehaviorData::EStatus::Done;
			}
		}
		break;
	default:
		Behavior.eStatus = SBehaviorData::EStatus::Done; // for safety issue
		break;
	}

	if (Behavior.eStatus == SBehaviorData::EStatus::Entering) Behavior.eStatus = SBehaviorData::EStatus::Processing;
	if (Behavior.eStatus == SBehaviorData::EStatus::Done)
	{
		if (Behavior.bIsPlayer)
		{
			Identifier.Object3D->SetAnimation(Identifier, EAnimationRegistrationType::Idle, EAnimationOption::Repeat,
				Identifier.Object3D->IsCurrentAnimationRegisteredAs(Identifier, EAnimationRegistrationType::Walking));
		}
	}
}
