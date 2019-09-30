#pragma once

#include "Object3D.h"

struct SComponentTransform
{
	XMVECTOR Translation{};
	XMVECTOR Rotation{};
	XMVECTOR Scaling{ XMVectorSet(1, 1, 1, 0) };
	XMMATRIX MatrixWorld{ XMMatrixIdentity() };
};

struct SComponentRender
{
	CObject3D* PtrObject3D{};
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
	SComponentTransform	ComponentTransform{};
	SComponentRender	ComponentRender{};
};