#pragma once

#include "AssimpLoader.h"

class CGame;
class CShader;

class CObject3D
{
public:
	enum class EFlagsRendering
	{
		None = 0x00,
		NoCulling = 0x01,
		NoLighting = 0x02,
		NoTexture = 0x04,
		NoDepthComparison = 0x08,
		UseRawVertexColor = 0x10
	};

private:
	static constexpr float KBoundingSphereDefaultRadius{ 1.0f };

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
		CShader*	PtrVS{};
		CShader*	PtrPS{};
		bool		bIsTransparent{ false };
		bool		bShouldAnimate{ false };
	};

	struct SComponentPhysics
	{
		struct SBoundingSphere
		{
			float		Radius{ KBoundingSphereDefaultRadius };
			XMVECTOR	CenterOffset{};
		};

		SBoundingSphere	BoundingSphere{};
		bool			bIsPickable{ true };
		bool			bIgnoreBoundingSphere{ false };
	};

	struct SMeshBuffers
	{
		ComPtr<ID3D11Buffer>	VertexBuffer{};
		UINT					VertexBufferStride{ sizeof(SVertex3D) };
		UINT					VertexBufferOffset{};

		ComPtr<ID3D11Buffer>	VertexBufferAnimation{};
		UINT					VertexBufferAnimationStride{ sizeof(SVertexAnimation) };
		UINT					VertexBufferAnimationOffset{};

		ComPtr<ID3D11Buffer>	IndexBuffer{};
	};

public:
	CObject3D(const string& Name, ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, CGame* PtrGame) :
		m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGame{ PtrGame }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
		assert(m_PtrGame);
	}
	~CObject3D() {}

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
	void Create(const SMesh& Mesh);
	void Create(const SMesh& Mesh, const CMaterial& Material);
	void Create(const vector<SMesh>& vMeshes, const vector<CMaterial>& vMaterials);
	void Create(const SModel& Model);

	void CreateFromFile(const string& FileName);

public:
	void AddMaterial(const CMaterial& Material);
	void SetMaterial(size_t Index, const CMaterial& Material);
	size_t GetMaterialCount() const;

	void UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize);
	void UpdateMeshBuffer(size_t MeshIndex = 0);
	void UpdateWorldMatrix();

	void Animate();
	void Draw(bool bIgnoreOwnTexture = false) const;

public:
	bool ShouldTessellate() const { return m_bShouldTesselate; }
	void ShouldTessellate(bool Value);

public:
	bool IsCreated() const { return m_bIsCreated; }
	const SModel& GetModel() const { return m_Model; }
	SModel& GetModel() { return m_Model; }
	const string& GetName() const { return m_Name; }
	const string& GetModelFileName() const { return m_ModelFileName; }

private:
	void CreateMeshBuffers();
	void CreateMeshBuffer(size_t MeshIndex, bool IsAnimated);

	void CreateMaterialTextures();
	void CreateMaterialTexture(CMaterial::CTexture::EType eType, CMaterial& Material);

	void CalculateAnimatedBoneMatrices(const SModel::SNode& Node, XMMATRIX ParentTransform);

public:
	static constexpr UINT			KDiffuseTextureSlotOffset{ 0 }; // PS
	static constexpr UINT			KNormalTextureSlotOffset{ 5 }; // PS
	static constexpr UINT			KOpacityTextureSlotOffset{ 10 }; // PS
	static constexpr UINT			KDisplacementTextureSlotOffset{ 0 }; // DS

public:
	SComponentTransform				ComponentTransform{};
	SComponentRender				ComponentRender{};
	SComponentPhysics				ComponentPhysics{};
	EFlagsRendering					eFlagsRendering{};

private:
	ID3D11Device*					m_PtrDevice{};
	ID3D11DeviceContext*			m_PtrDeviceContext{};
	CGame*							m_PtrGame{};

private:
	string									m_Name{};
	string									m_ModelFileName{};
	bool									m_bIsCreated{ false };
	SModel									m_Model{};
	vector<SMeshBuffers>					m_vMeshBuffers{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vDiffuseTextures{}; // Each texture is for each material of this Object3D
	vector<unique_ptr<CMaterial::CTexture>>	m_vNormalTextures{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vDisplacementTextures{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vOpacityTextures{};

	XMMATRIX								m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	size_t									m_CurrentAnimationIndex{};
	float									m_CurrentAnimationTick{};
	bool									m_bShouldTesselate{ false };

private:
	static CAssimpLoader					ms_AssimpLoader;
};

ENUM_CLASS_FLAG(CObject3D::EFlagsRendering)