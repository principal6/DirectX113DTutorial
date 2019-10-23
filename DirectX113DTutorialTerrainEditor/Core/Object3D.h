#pragma once

#include "AssimpLoader.h"

class CGame;

class CObject3D
{
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
	CObject3D(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, CGame* PtrGame) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGame{ PtrGame }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
		assert(m_PtrGame);
	}
	~CObject3D() {}

	void Create(const SMesh& Mesh);
	void Create(const SMesh& Mesh, const CMaterial& Material);
	void Create(const vector<SMesh>& vMeshes, const vector<CMaterial>& vMaterials);
	void Create(const SModel& Model);

	void LoadStaticModel(const string& FileName);
	void LoadAnimatedModel(const string& FileName);

	void AddMaterial(const CMaterial& Material);
	void SetMaterial(size_t Index, const CMaterial& Material);
	size_t GetMaterialCount() const;

	void UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize);
	void UpdateMeshBuffer(size_t MeshIndex = 0);

	void Animate();
	void Draw(bool bIgnoreOwnTexture = false) const;

	bool ShouldTessellate() const { return m_bShouldTesselate; }
	void ShouldTessellate(bool Value);

public:
	const SModel& GetModel() const { return m_Model; }
	SModel& GetModel() { return m_Model; }

private:
	void CreateMeshBuffers();
	void CreateMeshBuffer(size_t MeshIndex, bool IsAnimated);

	void CreateMaterialTextures();

	void CalculateAnimatedBoneMatrices(const SModel::SNode& Node, XMMATRIX ParentTransform);

public:
	static constexpr size_t			KTerrainNormalTextureSlotOffset{ 5 };
	static constexpr size_t			KTerrainMaskingTextureSlot{ 10 };

private:
	ID3D11Device*					m_PtrDevice{};
	ID3D11DeviceContext*			m_PtrDeviceContext{};
	CGame*							m_PtrGame{};

private:
	SModel									m_Model{};
	vector<SMeshBuffers>					m_vMeshBuffers{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vDiffuseTextures{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vNormalTextures{};

	XMMATRIX								m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	size_t									m_CurrentAnimationIndex{};
	float									m_CurrentAnimationTick{};
	bool									m_bShouldTesselate{ false };

private:
	static CAssimpLoader					ms_AssimpLoader;
};