#pragma once

#include "SharedHeader.h"

class CObject3D;
class CShader;

static constexpr float KBoundingSphereRadiusDefault{ 1.0f };

enum class EFlagsGameObject3DRendering
{
	None				= 0x00,
	NoCulling			= 0x01,
	NoLighting			= 0x02,
	NoTexture			= 0x04,
	NoDepthComparison	= 0x08,
	UseRawVertexColor	= 0x10
};
ENUM_CLASS_FLAG(EFlagsGameObject3DRendering)

class CGameObject3D
{
	struct SComponentTransform
	{
		XMVECTOR	Translation{};
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX	MatrixWorld{ XMMatrixIdentity() };

		float		Pitch{};
		float		Yaw{};
		float		Roll{};
	};

	struct SComponentRender
	{
		CObject3D*	PtrObject3D{};
		CShader*	PtrVS{};
		CShader*	PtrPS{};
		bool		bIsTransparent{ false };
		bool		bShouldAnimate{ false };
	};

	struct SComponentPhysics
	{
		struct SBoundingSphere
		{
			float		Radius{ KBoundingSphereRadiusDefault };
			XMVECTOR	CenterOffset{};
		};

		SBoundingSphere	BoundingSphere{};
		bool			bIsPickable{ true };
		bool			bIgnoreBoundingSphere{ false };
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
	CGameObject3D(const string& Name) : m_Name{ Name } {}
	~CGameObject3D() {}

public:
	void UpdateWorldMatrix();
	const string& GetName() const { return m_Name; }

private:
	string							m_Name{};

public:
	SComponentTransform				ComponentTransform{};
	SComponentRender				ComponentRender{};
	SComponentPhysics				ComponentPhysics{};
	EFlagsGameObject3DRendering		eFlagsGameObject3DRendering{};
};