#pragma once

#include "SharedHeader.h"

class CObject3D;
class CTexture;

struct SComponentTransform
{
	XMVECTOR Translation{};
	XMVECTOR Rotation{};
	XMVECTOR Scaling{ XMVectorSet(1, 1, 1, 0) };
	XMMATRIX MatrixWorld{ XMMatrixIdentity() };
};

struct SComponentRender
{
	struct SMaterial
	{
		XMFLOAT3	MaterialAmbient{};
		float		SpecularExponent{ 1 };
		XMFLOAT3	MaterialDiffuse{};
		float		SpecularIntensity{ 0 };
		XMFLOAT3	MaterialSpecular{};

	private:
		float		Pad{};
	};

	const CObject3D*	PtrObject3D{};
	const CTexture*		PtrTexture{};
	bool				IsTransparent{ false };
	SMaterial			Material{};
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