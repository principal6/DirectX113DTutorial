#pragma once

#include "SharedHeader.h"
#include "Material.h"
#include "AssimpLoader.h"
#include "MeshPorter.h"

class CGame;
class CShader;

class CObject3D
{
public:
	enum class EFlagsRendering
	{
		None = 0x00,
		NoCulling = 0x01,
	};

	struct SCBTessFactorData
	{
		SCBTessFactorData() {}
		SCBTessFactorData(float UniformTessFactor) : EdgeTessFactor{ UniformTessFactor }, InsideTessFactor{ UniformTessFactor } {}

		float		EdgeTessFactor{ 2.0f };
		float		InsideTessFactor{ 2.0f };
		float		Pads[2]{};
	};

	struct SCBDisplacementData
	{
		BOOL		bUseDisplacement{ TRUE };
		float		DisplacementFactor{ 1.0f };
		float		Pads[2]{};
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
		XMVECTOR	Translation{ 0, 0, 0, 1 };
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX	MatrixWorld{ XMMatrixIdentity() };

		float		Pitch{};
		float		Yaw{};
		float		Roll{};
	};

	struct SComponentPhysics
	{
		bool		bIsPickable{ true };
	};

	struct SComponentRender
	{
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
		UINT					VertexBufferAnimationStride{ sizeof(SAnimationVertex) };
		UINT					VertexBufferAnimationOffset{};

		ComPtr<ID3D11Buffer>	IndexBuffer{};
	};

	struct SInstanceBuffer
	{
		ComPtr<ID3D11Buffer>	Buffer{};
		UINT					Stride{ sizeof(SObject3DInstanceGPUData) };
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
	void Create(const SMesh& Mesh, const CMaterialData& MaterialData);
	void Create(const SMESHData& MESHData);

	void CreateFromFile(const std::string& FileName, bool bIsModelRigged);

private:
	void CreateMeshBuffers();
	void CreateMeshBuffer(size_t MeshIndex, bool IsAnimated);

	void CreateMaterialTextures();
	void CreateMaterialTexture(size_t Index);

public:
	void LoadOB3D(const std::string& OB3DFileName, bool bIsRigged);
	void SaveOB3D(const std::string& OB3DFileName);

public:
	bool HasAnimations();
	void AddAnimationFromFile(const std::string& FileName, const std::string& AnimationName);
	void SetAnimationID(int ID);
	int GetAnimationID() const;
	int GetAnimationCount() const;
	void SetAnimationName(int ID, const std::string& Name);
	const std::string& GetAnimationName(int ID) const;
	const SCBAnimationData& GetAnimationData() const;
	const DirectX::XMMATRIX* GetAnimationBoneMatrices() const;
	
	bool HasBakedAnimationTexture() const;
	bool CanBakeAnimationTexture() const;
	void BakeAnimationTexture();
	void SaveBakedAnimationTexture(const std::string& FileName);
	void LoadBakedAnimationTexture(const std::string& FileName);

public:
	void AddMaterial(const CMaterialData& MaterialData);
	void SetMaterial(size_t Index, const CMaterialData& MaterialData);
	size_t GetMaterialCount() const;

public:
	void CreateInstances(size_t InstanceCount);
	void CreateInstances(const std::vector<SObject3DInstanceCPUData>& vInstanceData);
	bool InsertInstance();
	bool InsertInstance(const std::string& InstanceName);
	bool ChangeInstanceName(const std::string& OldName, const std::string& NewName);
	void DeleteInstance(const std::string& InstanceName);
	void ClearInstances();
	SObject3DInstanceCPUData& GetInstanceCPUData(const std::string& InstanceName);
	SObject3DInstanceCPUData& GetLastInstanceCPUData();
	const std::vector<SObject3DInstanceCPUData>& GetInstanceCPUDataVector() const;
	SObject3DInstanceGPUData& GetInstanceGPUData(const std::string& InstanceName);

	void CreateInstanceBuffers();
	void UpdateInstanceBuffers();
	void UpdateInstanceBuffer(size_t MeshIndex = 0);

private:
	void CreateInstanceBuffer(size_t MeshIndex);

public:
	void UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize);
	void UpdateMeshBuffer(size_t MeshIndex = 0);

	void UpdateWorldMatrix();

	void UpdateInstanceWorldMatrix(const std::string& InstanceName);
	// @warning: this function doesn't update internal InstanceCPUData
	void UpdateInstanceWorldMatrix(const std::string& InstanceName, const XMMATRIX& WorldMatrix);

	void UpdateAllInstancesWorldMatrix();
	void SetInstanceHighlight(const std::string& InstanceName, bool bShouldHighlight);
	void SetAllInstancesHighlightOff();

private:
	size_t GetInstanceID(const std::string& InstanceName) const;

public:
	void Animate(float DeltaTime);
	void Draw(bool bIgnoreOwnTexture = false, bool bIgnoreInstances = false) const;

private:
	void CalculateAnimatedBoneMatrices(const SMESHData::SAnimation& CurrentAnimation, float AnimationTick, const SMESHData::STreeNode& Node, XMMATRIX ParentTransform);

public:
	bool ShouldTessellate() const { return m_bShouldTesselate; }
	void ShouldTessellate(bool Value);

	void SetTessFactorData(const CObject3D::SCBTessFactorData& Data);
	const CObject3D::SCBTessFactorData& GetTessFactorData() const;

	void SetDisplacementData(const CObject3D::SCBDisplacementData& Data);
	const CObject3D::SCBDisplacementData& GetDisplacementData() const;

public:
	bool IsCreated() const { return m_bIsCreated; }
	bool IsRigged() const { return m_Model.bIsModelRigged; }
	bool IsInstanced() const { return (m_vInstanceCPUData.size() > 0) ? true : false; }
	size_t GetInstanceCount() const { return m_vInstanceCPUData.size(); }
	const SMESHData& GetModel() const { return m_Model; }
	SMESHData& GetModel() { return m_Model; }
	void SetName(const std::string& Name);
	const std::string& GetName() const { return m_Name; }
	const std::string& GetModelFileName() const { return m_ModelFileName; }
	const std::string& GetOB3DFileName() const { return m_OB3DFileName; }
	const std::map<std::string, size_t>& GetInstanceNameToIndexMap() const { return m_mapInstanceNameToIndex; }
	CMaterialTextureSet* GetMaterialTextureSet(size_t iMaterial);

private:
	void LimitFloatRotation(float& Value, const float Min, const float Max);

public:
	static constexpr size_t KMaxAnimationNameLength{ 15 };
	static constexpr uint32_t KMaxBoneMatrixCount{ 60 };

private:
	static constexpr int32_t KAnimationTextureWidth{ 4 * (int32_t)KMaxBoneMatrixCount };
	static constexpr int32_t KAnimationTextureReservedHeight{ 1 };
	static constexpr int32_t KAnimationTextureReservedFirstPixelCount{ 2 };

public:
	SComponentTransform						ComponentTransform{};
	SComponentRender						ComponentRender{};
	SComponentPhysics						ComponentPhysics{};
	EFlagsRendering							eFlagsRendering{};
	SEditorBoundingSphere					EditorBoundingSphere{};

private:
	ID3D11Device* const						m_PtrDevice{};
	ID3D11DeviceContext* const				m_PtrDeviceContext{};
	CGame* const							m_PtrGame{};

private:
	std::string								m_Name{};
	std::string								m_ModelFileName{};
	std::string								m_OB3DFileName{};
	bool									m_bIsCreated{ false };
	SMESHData								m_Model{};
	std::vector<std::unique_ptr<CMaterialTextureSet>> m_vMaterialTextureSets{};
	std::vector<SMeshBuffers>				m_vMeshBuffers{};
	std::vector<SInstanceBuffer>			m_vInstanceBuffers{};
	SCBTessFactorData						m_CBTessFactorData{};
	SCBDisplacementData						m_CBDisplacementData{};

	XMMATRIX								m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	int										m_CurrentAnimationID{};
	float									m_CurrentAnimationTick{};
	bool									m_bShouldTesselate{ false };

	std::unique_ptr<CTexture>				m_BakedAnimationTexture{};
	SCBAnimationData						m_CBAnimationData{};
	bool									m_bIsBakedAnimationLoaded{ false };

private:
	std::vector<SObject3DInstanceGPUData>	m_vInstanceGPUData{};
	std::vector<SObject3DInstanceCPUData>	m_vInstanceCPUData{};
	std::map<std::string, size_t>			m_mapInstanceNameToIndex{};

private:
	CAssimpLoader							m_AssimpLoader{};
};

ENUM_CLASS_FLAG(CObject3D::EFlagsRendering)