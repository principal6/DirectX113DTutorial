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

	struct SInstanceCPUData
	{
		string		Name{};
		XMVECTOR	Translation{};
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		float		Pitch{};
		float		Yaw{};
		float		Roll{};
	};

private:
	struct SComponentTransform
	{
		XMVECTOR	Translation{};
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX	MatrixWorld{ XMMatrixIdentity() };

		float		Pitch{};
		float		Yaw{};
		float		Roll{};
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

	struct SComponentRender
	{
		CShader*	PtrVS{};
		CShader*	PtrPS{};
		bool		bIsTransparent{ false };
		bool		bShouldAnimate{ false };
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

	struct SInstanceBuffer
	{
		ComPtr<ID3D11Buffer>	Buffer{};
		UINT					Stride{ sizeof(SInstanceGPUData) };
		UINT					Offset{};
	};

public:
	CObject3D(const string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, CGame* const PtrGame) :
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

	void CreateFromFile(const string& FileName, bool bIsModelRigged);

public:
	void AddAnimationFromFile(const string& FileName);
	void SetAnimationID(int ID);
	int GetAnimationID() const;
	int GetAnimationCount() const;

public:
	void AddMaterial(const CMaterial& Material);
	void SetMaterial(size_t Index, const CMaterial& Material);
	size_t GetMaterialCount() const;

	void CreateInstances(int InstanceCount);
	void InsertInstance(bool bShouldCreateInstanceBuffers = true);
	void InsertInstance(const string& Name);
	void DeleteInstance(const string& Name);
	SInstanceCPUData& GetInstance(int InstanceID);
	SInstanceCPUData& GetInstance(const string& Name);
	void CreateInstanceBuffers();

	void UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize);
	void UpdateMeshBuffer(size_t MeshIndex = 0);
	void UpdateInstanceBuffers();
	void UpdateInstanceBuffer(size_t MeshIndex = 0);

	void UpdateWorldMatrix();
	void UpdateInstanceWorldMatrix(int InstanceID);
	void UpdateAllInstancesWorldMatrix();

	void Animate(float DeltaTime);
	void Draw(bool bIgnoreOwnTexture = false) const;

public:
	bool ShouldTessellate() const { return m_bShouldTesselate; }
	void ShouldTessellate(bool Value);

public:
	bool IsCreated() const { return m_bIsCreated; }
	bool IsRiggedModel() const { return m_Model.bIsModelAnimated; }
	bool IsInstanced() const { return (m_vInstanceCPUData.size() > 0) ? true : false; }
	int GetInstanceCount() const { return (int)m_vInstanceCPUData.size(); }
	const SModel& GetModel() const { return m_Model; }
	SModel& GetModel() { return m_Model; }
	const string& GetName() const { return m_Name; }
	const string& GetModelFileName() const { return m_ModelFileName; }
	const map<string, size_t>& GetInstanceMap() const { return m_mapInstanceNameToIndex; }

private:
	void CreateMeshBuffers();
	void CreateMeshBuffer(size_t MeshIndex, bool IsAnimated);

	void CreateInstanceBuffer(size_t MeshIndex);

	void CreateMaterialTextures();

	void CalculateAnimatedBoneMatrices(const SModel::SNode& Node, XMMATRIX ParentTransform);

	void LimitFloatRotation(float& Value, const float Min, const float Max);

public:
	static constexpr size_t KInstanceNameMaxLength{ 100 };

private:
	static constexpr float KBoundingSphereDefaultRadius{ 1.0f };

public:
	SComponentTransform			ComponentTransform{};
	SComponentRender			ComponentRender{};
	SComponentPhysics			ComponentPhysics{};
	EFlagsRendering				eFlagsRendering{};

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};
	CGame* const				m_PtrGame{};

private:
	string						m_Name{};
	string						m_ModelFileName{};
	bool						m_bIsCreated{ false };
	SModel						m_Model{};
	vector<SMeshBuffers>		m_vMeshBuffers{};
	vector<SInstanceBuffer>		m_vInstanceBuffers{};

	XMMATRIX					m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	int							m_CurrentAnimationID{};
	float						m_CurrentAnimationTick{};
	bool						m_bShouldTesselate{ false };

private:
	vector<SInstanceGPUData>	m_vInstanceGPUData{};
	vector<SInstanceCPUData>	m_vInstanceCPUData{};
	map<string, size_t>			m_mapInstanceNameToIndex{};

private:
	CAssimpLoader				m_AssimpLoader{};
};

ENUM_CLASS_FLAG(CObject3D::EFlagsRendering)