#pragma once

#include "SharedHeader.h"

class CObject2D;
class CTexture;

struct SComponentTransform2D
{
	XMVECTOR	Translation{};
	XMVECTOR	RotationQuaternion{};
	XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
	XMMATRIX	MatrixWorld{ XMMatrixIdentity() };
};

struct SComponentRender2D
{
	CObject2D*	PtrObject2D{};
	CTexture*	PtrTexture{};
};

class CGameObject2D
{
	friend class CGame;

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
	CGameObject2D(const string& Name) : m_Name{ Name } {}
	~CGameObject2D() {}

private:
	void UpdateWorldMatrix();

private:
	string				m_Name{};

public:
	SComponentTransform2D	ComponentTransform{};
	SComponentRender2D		ComponentRender{};
	bool					bIsVisible{ true };
};