#pragma once

#include "../Core/SharedHeader.h"

// SPatternState is created per SObjectIdentifier(Object/Instance) in CIntelligence
struct SPatternState
{
	SPatternState() {}
	SPatternState(const XMVECTOR* const _MyPosition) : MyPosition{ _MyPosition } {}

	size_t			StateID{};
	size_t			InstructionIndex{};
	long long		InstructionEndTime{}; // unit: ms
	float			WalkSpeed{ 1.0f };
	const XMVECTOR* MyPosition{};
	const XMVECTOR* EnemyPosition{};
};
