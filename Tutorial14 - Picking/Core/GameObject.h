#pragma once

#include "SharedHeader.h"

class CObject3D;
class CTexture;

static constexpr float KBoundingSphereRadiusDefault{ 1.0f };

enum class EFlagsGameObjectRendering
{
	None = 0x00,
	NoCulling = 0x01,
	NoLighting = 0x02,
	NoDepthComparison = 0x04,
};
ENUM_CLASS_FLAG(EFlagsGameObjectRendering)

struct SComponentTransform
{
	XMVECTOR	Translation{};
	XMVECTOR	RotationQuaternion{};
	XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
	XMMATRIX	MatrixWorld{ XMMatrixIdentity() };
};

struct SComponentRender
{
	CObject3D*	PtrObject3D{};
	CTexture*	PtrTexture{};
	bool		IsTransparent{ false };
};

struct SBoundingSphere
{
	float		Radius{ KBoundingSphereRadiusDefault };
	XMVECTOR	CenterOffset{};
};

struct SComponentPhysics
{
	SBoundingSphere	BoundingSphere{};
	bool			bIsPickable{ true };
};

class CGameObject
{
	friend class CGameWindow;

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
	CGameObject(const string& Name) : m_Name{ Name } {}
	~CGameObject() {}

	void UpdateWorldMatrix();

private:
	string							m_Name{};

public:
	SComponentTransform				ComponentTransform{};
	SComponentRender				ComponentRender{};
	SComponentPhysics				ComponentPhysics{};
	EFlagsGameObjectRendering		eFlagsGameObjectRendering{};
};