#pragma once

#include "SharedHeader.h"

class CObject3DLine;

class CGameObject3DLine
{
	friend class CGame;

	struct SComponentTransform
	{
		XMVECTOR		Translation{};
		XMVECTOR		RotationQuaternion{};
		XMVECTOR		Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX		MatrixWorld{ XMMatrixIdentity() };
	};

	struct SComponentRender
	{
		CObject3DLine*	PtrObject3DLine{};
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
	CGameObject3DLine(const string& Name) : m_Name{ Name } {}
	~CGameObject3DLine() {}

private:
	void UpdateWorldMatrix();

private:
	string				m_Name{};

public:
	SComponentTransform	ComponentTransform{};
	SComponentRender	ComponentRender{};
	bool				bIsVisible{ true };
};