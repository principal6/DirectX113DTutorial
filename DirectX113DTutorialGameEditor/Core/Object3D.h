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
		UseRawVertexColor = 0x08
	};

	struct SBoundingSphere
	{
		float		Radius{ KBoundingSphereDefaultRadius };
		float		RadiusBias{ KBoundingSphereDefaultRadius };
		XMVECTOR	CenterOffset{};
	};

	struct SInstanceCPUData
	{
		std::string	Name{};
		XMVECTOR	Translation{};
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		float		Pitch{};
		float		Yaw{};
		float		Roll{};
		SBoundingSphere	BoundingSphere{};
	};

	struct SCBAnimationData
	{
		BOOL	bUseGPUSkinning{};
		int32_t	AnimationID{};
		int32_t AnimationDuration{};
		float	AnimationTick{};
	};

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
		SBoundingSphere	BoundingSphere{};
		bool			bIsPickable{ true };
	};

	struct SComponentRender
	{
		CShader*	PtrVS{};
		CShader*	PtrPS{};
		bool		bIsTransparent{ false };
		bool		bShouldAnimate{ false };
	};

private:
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
	CObject3D(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, CGame* const PtrGame) :
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
	void Create(const std::vector<SMesh>& vMeshes, const std::vector<CMaterial>& vMaterials);
	void Create(const SModel& Model);

	void CreateFromFile(const std::string& FileName, bool bIsModelRigged);

public:
	bool HasAnimations();
	void AddAnimationFromFile(const std::string& FileName, const std::string& AnimationName);
	void SetAnimationID(int ID);
	int GetAnimationID() const;
	int GetAnimationCount() const;
	void SetAnimationName(int ID, const std::string& Name);
	const std::string& GetAnimationName(int ID) const;
	
	bool CanBakeAnimationTexture() const;
	void BakeAnimationTexture();
	void SaveBakedAnimationTexture(const std::string& FileName);
	void LoadBakedAnimationTexture(const std::string& FileName);

public:
	void AddMaterial(const CMaterial& Material);
	void SetMaterial(size_t Index, const CMaterial& Material);
	size_t GetMaterialCount() const;

	void CreateInstances(int InstanceCount);
	void InsertInstance(bool bShouldCreateInstanceBuffers = true);
	void InsertInstance(const std::string& Name);
	void DeleteInstance(const std::string& Name);
	SInstanceCPUData& GetInstanceCPUData(int InstanceID);
	SInstanceCPUData& GetInstanceCPUData(const std::string& Name);
	SInstanceGPUData& GetInstanceGPUData(int InstanceID);
	SInstanceGPUData& GetInstanceGPUData(const std::string& Name);
	void CreateInstanceBuffers();

	void UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize);
	void UpdateMeshBuffer(size_t MeshIndex = 0);
	void UpdateInstanceBuffers();
	void UpdateInstanceBuffer(size_t MeshIndex = 0);

	void UpdateWorldMatrix();
	void UpdateInstanceWorldMatrix(uint32_t InstanceID);
	void UpdateAllInstancesWorldMatrix();

	void Animate(float DeltaTime);
	void Draw(bool bIgnoreOwnTexture = false, bool bIgnoreInstances = false) const;

public:
	bool ShouldTessellate() const { return m_bShouldTesselate; }
	void ShouldTessellate(bool Value);

public:
	bool IsCreated() const { return m_bIsCreated; }
	bool IsRiggedModel() const { return m_Model.bIsModelAnimated; }
	bool IsInstanced() const { return (m_vInstanceCPUData.size() > 0) ? true : false; }
	uint32_t GetInstanceCount() const { return (uint32_t)m_vInstanceCPUData.size(); }
	const SModel& GetModel() const { return m_Model; }
	SModel& GetModel() { return m_Model; }
	const std::string& GetName() const { return m_Name; }
	const std::string& GetModelFileName() const { return m_ModelFileName; }
	const std::map<std::string, size_t>& GetInstanceMap() const { return m_mapInstanceNameToIndex; }

private:
	void CreateMeshBuffers();
	void CreateMeshBuffer(size_t MeshIndex, bool IsAnimated);

	void CreateInstanceBuffer(size_t MeshIndex);

	void CreateMaterialTextures();

	void CalculateAnimatedBoneMatrices(const SModel::SAnimation& CurrentAnimation, float AnimationTick, const SModel::SNode& Node, XMMATRIX ParentTransform);

	void LimitFloatRotation(float& Value, const float Min, const float Max);

public:
	static constexpr size_t KInstanceNameZeroEndedMaxLength{ 32 };
	static constexpr size_t KMaxAnimationNameLength{ 15 };

private:
	static constexpr float KBoundingSphereDefaultRadius{ 1.0f };
	static constexpr int32_t KAnimationTextureWidth{ 4 * (int32_t)KMaxBoneMatrixCount };
	static constexpr int32_t KAnimationTextureReservedHeight{ 1 };
	static constexpr int32_t KAnimationTextureReservedFirstPixelCount{ 2 };

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
	std::string						m_Name{};
	std::string						m_ModelFileName{};
	bool							m_bIsCreated{ false };
	SModel							m_Model{};
	std::vector<SMeshBuffers>		m_vMeshBuffers{};
	std::vector<SInstanceBuffer>	m_vInstanceBuffers{};

	XMMATRIX						m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	int								m_CurrentAnimationID{};
	float							m_CurrentAnimationTick{};
	bool							m_bShouldTesselate{ false };

	std::unique_ptr<CMaterial::CTexture>	m_BakedAnimationTexture{};
	SCBAnimationData						m_CBAnimationData{};
	bool									m_bIsBakedAnimationLoaded{ false };

private:
	std::vector<SInstanceGPUData>	m_vInstanceGPUData{};
	std::vector<SInstanceCPUData>	m_vInstanceCPUData{};
	std::map<std::string, size_t>	m_mapInstanceNameToIndex{};

private:
	CAssimpLoader				m_AssimpLoader{};
};

ENUM_CLASS_FLAG(CObject3D::EFlagsRendering)