#pragma once

#include "../Core/SharedHeader.h"

class CObject3D;
class CIntelligence;

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
public:
	CIntelligence(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CIntelligence();

public:
	void ClearBehaviors();

public:
	void RegisterPriority(const SObjectIdentifier& Identifier, EObjectPriority ePriority, bool bShouldChangePriority = false);

public:
	void PushBackBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior);
	void PushFrontBehavior(const SObjectIdentifier& Identifier, const SBehaviorData& Behavior);
	void PopFrontBehavior(const SObjectIdentifier& Identifier);
	void PopFrontBehaviorIf(const SObjectIdentifier& Identifier, EBehaviorType eBehaviorType);
	bool HasBehavior(const SObjectIdentifier& Identifier) const;
	const SBehaviorData& PeekBehavior(const SObjectIdentifier& Identifier) const;

public:
	void Execute();

private:
	void ExecuteBehavior(const SObjectIdentifier& Identifier, SBehaviorData& Behavior);

private:
	static constexpr size_t					KPriorityCount{ 3 };

private:
	ID3D11Device* const						m_PtrDevice{};
	ID3D11DeviceContext* const				m_PtrDeviceContext{};

private:
	std::vector<SBehaviorSet>				m_vBehaviorSets[KPriorityCount]{};
	std::unordered_map<std::string, size_t>	m_umapBehaviorSets[KPriorityCount]{};
	std::unordered_map<std::string, size_t>	m_umapPriority{};

private:
	bool									m_bBehaviorStarted{ false };
	XMVECTOR								m_SavedVector{};
	XMVECTOR								m_SavedVectorXZ{};
};