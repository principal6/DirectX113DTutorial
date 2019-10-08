#pragma once

#include "SharedHeader.h"

class CObject3D;
class CTexture;

enum class EFlagsGameObjectRendering
{
	None = 0x00,
	NoCulling = 0x01,
	NoLighting = 0x02,
	NoDepthComparison = 0x04,
};
ENUM_CLASS_FLAG(EFlagsGameObjectRendering)

enum class EWorldMatrixCalculationOrder
{
	SRT,
	STR,
};

struct SComponentTransform
{
	XMVECTOR Translation{};
	XMVECTOR RotationQuaternion{};
	XMVECTOR Scaling{ XMVectorSet(1, 1, 1, 0) };
	XMMATRIX MatrixWorld{ XMMatrixIdentity() };
};

struct SComponentRender
{
	CObject3D*	PtrObject3D{};
	CTexture*	PtrTexture{};
	bool		IsTransparent{ false };
};

class CGameObject
{
public:
	void* operator new(size_t Size)
	{
		return _aligned_malloc(Size, 16);
	}

	void operator delete(void* Pointer)
	{
		_aligned_free(Pointer);
	}

public:
	CGameObject() {}
	~CGameObject() {}

	void UpdateWorldMatrix();

public:
	SComponentTransform				ComponentTransform{};
	SComponentRender				ComponentRender{};
	EFlagsGameObjectRendering		eFlagsGameObjectRendering{};
};