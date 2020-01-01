#pragma once

#include "../Core/SharedHeader.h"

class CObject3D;
class CIntelligence;

enum class EBehaviorPriority
{
	A_Crucial,
	B_Normal,
	C_Trivial
};

enum class EBehaviorType
{
	None,

	WalkTo
};

struct SBehaviorData
{
	friend class CIntelligence;

	EBehaviorType	eBehaviorType{};
	XMVECTOR		Vector{};
	float			Factor{ 1.0f };

private:
	bool			bDone{ false };
};

struct SBehaviorSet
{
	CObject3D*					Object3D{};
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
	void RegisterPriority(CObject3D* const Object3D, EBehaviorPriority ePriority, bool bShouldChangePriority = false);
	void PushBackBehavior(CObject3D* const Object3D, const SBehaviorData& Behavior);
	void PushFrontBehavior(CObject3D* const Object3D, const SBehaviorData& Behavior);
	void PopFrontBehavior(CObject3D* const Object3D);
	void PopFrontBehaviorIf(CObject3D* const Object3D, EBehaviorType eBehaviorType);
	bool HasBehavior(CObject3D* const Object3D) const;
	const SBehaviorData& PeekBehavior(CObject3D* const Object3D) const;

public:
	void Execute();

private:
	void ExecuteBehavior(CObject3D* const Object3D, SBehaviorData& Behavior);

private:
	static constexpr size_t KPriorityCount{ 3 };

private:
	ID3D11Device* const						m_PtrDevice{};
	ID3D11DeviceContext* const				m_PtrDeviceContext{};

private:
	std::vector<SBehaviorSet>				m_vBehaviorSets[KPriorityCount]{};
	std::unordered_map<CObject3D*, size_t>	m_umapBehaviorSets[KPriorityCount]{};
	std::unordered_map<CObject3D*, size_t>	m_umapPriority{};
};