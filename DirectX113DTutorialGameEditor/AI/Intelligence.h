#pragma once

#include "../Core/SharedHeader.h"
#include "PatternTypes.h"

class CObject3D;
class CPhysicsEngine;
class CPattern;

enum class EObjectPriority
{
	A_Crucial,
	B_Normal,
	C_Trivial
};

enum class EBehaviorType
{
	None,

	WalkTo,
	Jump,
	Attack
};

struct SBehaviorData
{
	friend class CIntelligence;

	EBehaviorType	eBehaviorType{};
	XMVECTOR		Vector{};
	float			Scalar{ 1.0f };
	bool			bIsPlayer{ false };

private:
	enum class EStatus
	{
		Waiting,
		Entering,
		Processing,
		Done
	};
	EStatus			eStatus{ EStatus::Waiting };
};

struct SBehaviorSet
{
	SObjectIdentifier			Identifier{};
	std::deque<SBehaviorData>	dqBehaviors{};
};

class CIntelligence final
{
private:
	struct SInternalPatternData
	{
		SInternalPatternData() {}
		SInternalPatternData(const SObjectIdentifier& _ObjectIdentifier, CPattern* const _Pattern) : 
			ObjectIdentifier{ _ObjectIdentifier }, Pattern{ _Pattern } {}

		SObjectIdentifier	ObjectIdentifier{};
		CPattern*			Pattern{};
		SPatternState		PatternState{};
	};

public:
	CIntelligence(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CIntelligence();

public:
	void LinkPhysicsEngine(CPhysicsEngine* PhysicsEngine);
	void ClearBehaviors();

public:
	void RegisterPriority(const SObjectIdentifier& Identifier, EObjectPriority ePriority, bool bShouldChangePriority = false);

public:
	void ClearBehavior(const SObjectIdentifier& Identifier);
	void PushBackBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior);
	void PushFrontBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior);
	void PopFrontBehavior(const SObjectIdentifier& Identifier);
	void PopFrontBehaviorIf(const SObjectIdentifier& Identifier, EBehaviorType eBehaviorType);
	bool HasBehavior(const SObjectIdentifier& Identifier) const;
	bool IsFrontBehavior(const SObjectIdentifier& Identifier, EBehaviorType eBehaviorType) const;
	const SBehaviorData& PeekFrontBehavior(const SObjectIdentifier& Identifier) const;
	const SBehaviorData& PeekBackBehavior(const SObjectIdentifier& Identifier) const;

private:
	SBehaviorSet& GetBehaviorSet(const SObjectIdentifier& Identifier);
	const SBehaviorSet& GetBehaviorSet(const SObjectIdentifier& Identifier) const;

public:
	void RegisterPattern(const SObjectIdentifier& Identifier, CPattern* const Pattern);
	bool HasPattern(const SObjectIdentifier& Identifier) const;
	CPattern* GetPattern(const SObjectIdentifier& Identifier) const;

public:
	void Execute();

private:
	void ConvertPatternsIntoBehaviors();
	void ExecuteBehavior(const SObjectIdentifier& Identifier, SBehaviorData& Behavior);

private:
	static constexpr size_t							KPriorityCount{ 3 };

private:
	ID3D11Device* const								m_PtrDevice{};
	ID3D11DeviceContext* const						m_PtrDeviceContext{};

private:
	std::vector<SBehaviorSet>						m_vBehaviorSets[KPriorityCount]{};
	std::unordered_map<std::string, size_t>			m_umapBehaviorSets[KPriorityCount]{};
	std::unordered_map<std::string, size_t>			m_umapPriority{};

private:
	std::vector<SInternalPatternData>				m_vInternalPatternData{};
	std::unordered_map<std::string, size_t>			m_umapPatternInfos{};
	CPhysicsEngine*									m_PhysicsEngine{};

private:
	bool											m_bBehaviorStarted{ false };
	XMVECTOR										m_SavedVector{};
	XMVECTOR										m_SavedVectorXZ{};
};
