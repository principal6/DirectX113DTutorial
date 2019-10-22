#pragma once

#include "SharedHeader.h"

class CObject2D;
class CMaterial;

class CGameObject2D
{
	struct SComponentTransform
	{
		XMVECTOR	Translation{};
		XMVECTOR	RotationQuaternion{};
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX	MatrixWorld{ XMMatrixIdentity() };
	};

	struct SComponentRender
	{
		CObject2D*	PtrObject2D{};
		CMaterial*	PtrMaterial{};
	};

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

public:
	void UpdateWorldMatrix();

private:
	string				m_Name{};

public:
	SComponentTransform	ComponentTransform{};
	SComponentRender	ComponentRender{};
	bool				bIsVisible{ true };
};