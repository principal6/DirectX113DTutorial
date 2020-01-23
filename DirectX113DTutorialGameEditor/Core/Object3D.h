#pragma once

#include "SharedHeader.h"

class CAssimpLoader;
class CConstantBuffer;
class CMaterialData;
class CMaterialTextureSet;
class CShader;
class CTexture;
struct SMeshAnimation;
struct SMeshTreeNode;
struct SMESHData;
enum class ETextureType;

enum class EFlagsObject3DRendering
{
	None				= 0x00,
	IgnoreOwnTextures	= 0x01,
	DrawOneInstance		= 0x02,
	UseVoidPS			= 0x04
};
ENUM_CLASS_FLAG(EFlagsObject3DRendering)

class CObject3D
{
public:
	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "BLEND_INDICES"	, 0, DXGI_FORMAT_R32G32B32A32_UINT	, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLEND_WEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 1, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "INSTANCE_WORLD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD"	, 1, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD"	, 2, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_WORLD"	, 3, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "IS_HIGHLIGHTED"	, 0, DXGI_FORMAT_R32_FLOAT			, 2, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "ANIM_TICK"		, 0, DXGI_FORMAT_R32_FLOAT			, 2, 68, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "CURR_ANIM_ID"	, 0, DXGI_FORMAT_R32_UINT			, 2, 72, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	enum class EFlagsRendering
	{
		None = 0x00,
		NoCulling = 0x01,
	};

	struct SCBMaterialData // Update at least per every object (even an object could have multiple materials)
	{
		//XMFLOAT3	AmbientColor{};
		//float		SpecularExponent{ 1 };
		XMFLOAT3	DiffuseColor{};
		//float		SpecularIntensity{ 0 };
		//XMFLOAT3	SpecularColor{};
		float		Roughness{};

		float		Metalness{};
		uint32_t	FlagsHasTexture{};
		uint32_t	FlagsIsTextureSRGB{};
		uint32_t	TotalMaterialCount{};
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
		BOOL		bUseGPUSkinning{};
		BOOL		bIsInstanced{};
		uint32_t	AnimationID{};
		float		AnimationTick{};
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
	CObject3D(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CObject3D();

public:
	void* operator new(size_t Size)
	{
		return _aligned_malloc(Size, 16);
	}

	void operator delete(void* Pointer)
	{
		_aligned_free(Pointer);
	}

// Object creation
public:
	void Create(const SMESHData& MESHData);
	void Create(const SMesh& Mesh, const CMaterialData& MaterialData);
	void Create(const SMesh& Mesh);
	void CreateFromFile(const std::string& FileName, bool bIsModelRigged);

// Object creation (internal)
private:
	void InitializeModelData();
	void _CreateMeshBuffers();
	void __CreateMeshBuffer(size_t MeshIndex, bool IsAnimated);
	void _CreateMaterialTextures();
	void __CreateMaterialTexture(size_t Index);
	void _CreateConstantBuffers();
	void _InitializeAnimationData();
	void CalculateEditorBoundingSphereData();

// Import & export
public:
	void LoadOB3D(const std::string& OB3DFileName, bool bIsRigged);
	void SaveOB3D(const std::string& OB3DFileName);
	void ExportEmbeddedTextures(const std::string& Directory);

// Import & export (internal)
private:
	void ExportEmbeddedTexture(CMaterialTextureSet* const MaterialTextureSet, CMaterialData& MaterialData,
		ETextureType eTextureType, const std::string& Directory);

// Instance creation & deletion
public:
	void CreateInstances(const std::vector<SObject3DInstanceCPUData>& vInstanceCPUData, const std::vector<SObject3DInstanceGPUData>& vInstanceGPUData);
	void CreateInstances(const std::vector<SObject3DInstanceCPUData>& vInstanceCPUData);
	void CreateInstances(size_t InstanceCount);
	bool InsertInstance();
	bool InsertInstance(const std::string& InstanceName);
	void DeleteInstance(const std::string& InstanceName);
	void ClearInstances();

// Instance setting
public:
	bool ChangeInstanceName(const std::string& OldName, const std::string& NewName);
	// @important: name overriding is prevented internally
	void SetInstanceCPUData(const std::string& InstanceName, const SObject3DInstanceCPUData& Prime);
	const SObject3DInstanceCPUData& GetInstanceCPUData(const std::string& InstanceName) const;
	const SObject3DInstanceGPUData& GetInstanceGPUData(const std::string& InstanceName) const;
	const std::vector<SObject3DInstanceCPUData>& GetInstanceCPUDataVector() const;
	size_t GetInstanceIndex(const std::string& InstanceName) const;
	const XMMATRIX& GetInstanceWorldMatrix(const std::string& InstanceName) const;
	const std::string& GetLastInstanceName() const;

// Instance setting (internal)
private:
	SObject3DInstanceCPUData& GetInstanceCPUData(const std::string& InstanceName);
	SObject3DInstanceGPUData& GetInstanceGPUData(const std::string& InstanceName);

// Instance buffer
private:
	void _CreateInstanceBuffer(size_t MeshIndex);
	void CreateInstanceBuffers();
	void _UpdateInstanceBuffer(size_t MeshIndex = 0);
	void UpdateInstanceBuffers();

// Animation adding & setting (general)
public:
	void AddAnimationFromFile(const std::string& FileName, const std::string& AnimationName);
	void RegisterAnimation(uint32_t AnimationID, EAnimationRegistrationType eRegisteredType);
	void SetAnimationName(uint32_t AnimationID, const std::string& Name);
	void SetAnimationTicksPerSecond(uint32_t AnimationID, float TPS);
	void SetAnimationBehaviorStartTick(uint32_t AnimationID, float BehaviorStartTick);

// Animation info (general)
public:
	bool HasAnimations() const;
	size_t GetAnimationCount() const;
	const std::string& GetAnimationName(uint32_t AnimationID) const;
	const SCBAnimationData& GetAnimationData() const;
	EAnimationRegistrationType GetRegisteredAnimationType(uint32_t AnimationID) const;
	float GetAnimationTicksPerSecond(uint32_t AnimationID) const;
	float GetAnimationBehaviorStartTick(uint32_t AnimationID) const;
	float GetAnimationDuration(uint32_t AnimationID) const;
	const DirectX::XMMATRIX* GetAnimationBoneMatrices() const;

// Animation info (identifier)
public:
	bool IsCurrentAnimationRegisteredAs(const SObjectIdentifier& Identifier, EAnimationRegistrationType eRegistrationType) const;
	float GetAnimationTick(const SObjectIdentifier& Identifier) const;
	uint32_t GetAnimationID(const SObjectIdentifier& Identifier) const;
	size_t GetAnimationPlayCount(const SObjectIdentifier& Identifier) const;
	float GetCurrentAnimationBehaviorStartTick(const SObjectIdentifier& Identifier) const;

// Animation info (object & instance)
private:
	bool IsObjectCurrentAnimationRegisteredAs(EAnimationRegistrationType eRegistrationType) const;
	float GetObjectAnimationTick() const;
	uint32_t GetObjectAnimationID() const;
	size_t GetObjectAnimationPlayCount() const;
	float GetObjectCurrentAnimationBehaviorStartTick() const;

	bool IsInstanceCurrentAnimationRegisteredAs(const std::string& InstanceName, EAnimationRegistrationType eRegistrationType) const;
	float GetInstanceAnimationTick(const std::string& InstanceName) const;
	uint32_t GetInstanceAnimationID(const std::string& InstanceName) const;
	size_t GetInstanceAnimationPlayCount(const std::string& InstanceName) const;
	float GetInstanceCurrentAnimationBehaviorStartTick(const std::string& InstanceName) const;

// Animation baking (general)
public:
	bool HasBakedAnimationTexture() const;
	bool CanBakeAnimationTexture() const;
	void BakeAnimationTexture();
	void SaveBakedAnimationTexture(const std::string& FileName);
	void LoadBakedAnimationTexture(const std::string& FileName);

// Animation setting (identifier)
public:
	void SetAnimation(const SObjectIdentifier& Identifier, uint32_t AnimationID,
		EAnimationOption eAnimationOption = EAnimationOption::Repeat, bool bShouldIgnoreCurrentAnimation = true);
	void SetAnimation(const SObjectIdentifier& Identifier, EAnimationRegistrationType eRegisteredType,
		EAnimationOption eAnimationOption = EAnimationOption::Repeat, bool bShouldIgnoreCurrentAnimation = true);

// Animation setting (object & instance)
private:
	void SetObjectAnimation(uint32_t AnimationID,
		EAnimationOption eAnimationOption = EAnimationOption::Repeat, bool bShouldIgnoreCurrentAnimation = true);
	void SetObjectAnimation(EAnimationRegistrationType eRegisteredType,
		EAnimationOption eAnimationOption = EAnimationOption::Repeat, bool bShouldIgnoreCurrentAnimation = true);

	void SetInstanceAnimation(const std::string& InstanceName, uint32_t AnimationID,
		EAnimationOption eAnimationOption = EAnimationOption::Repeat, bool bShouldIgnoreCurrentAnimation = true);
	void SetInstanceAnimation(const std::string& InstanceName, EAnimationRegistrationType eRegisteredType,
		EAnimationOption eAnimationOption = EAnimationOption::Repeat, bool bShouldIgnoreCurrentAnimation = true);

// Inner bounding volumes (general)
public:
	bool HasInnerBoundingVolumes() const;
	const std::vector<SBoundingVolume>& GetInnerBoundingVolumeVector() const;
	std::vector<SBoundingVolume>& GetInnerBoundingVolumeVector();

// Transform, Physics, Outer bounding sphere (identifier)
public:
	const SComponentTransform& GetTransform(const SObjectIdentifier& Identifier) const;
	const SComponentPhysics& GetPhysics(const SObjectIdentifier& Identifier) const;
	EFlagsRendering GetRenderingFlags() const;
	const SBoundingVolume& GetOuterBoundingSphere(const SObjectIdentifier& Identifier) const;

	void TranslateTo(const SObjectIdentifier& Identifier, const XMVECTOR& Prime);
	void RotatePitchTo(const SObjectIdentifier& Identifier, float Prime);
	void RotateYawTo(const SObjectIdentifier& Identifier, float Prime);
	void RotateRollTo(const SObjectIdentifier& Identifier, float Prime);
	void ScaleTo(const SObjectIdentifier& Identifier, const XMVECTOR& Prime);

	void Translate(const SObjectIdentifier& Identifier, const XMVECTOR& Delta);
	void RotatePitch(const SObjectIdentifier& Identifier, float Delta);
	void RotateYaw(const SObjectIdentifier& Identifier, float Delta);
	void RotateRoll(const SObjectIdentifier& Identifier, float Delta);
	void Scale(const SObjectIdentifier& Identifier, const XMVECTOR& Delta);
	
	void SetLinearAcceleration(const SObjectIdentifier& Identifier, const XMVECTOR& Prime);
	void SetLinearVelocity(const SObjectIdentifier& Identifier, const XMVECTOR& Prime);
	void AddLinearAcceleration(const SObjectIdentifier& Identifier, const XMVECTOR& Delta);
	void AddLinearVelocity(const SObjectIdentifier& Identifier, const XMVECTOR& Delta);

// Transform, Physics, Outer bounding sphere (object)
public:
	void SetTransform(const SComponentTransform& NewValue);
	void SetPhysics(const SComponentPhysics& NewValue);
	void SetRender(const SComponentRender& NewValue);
	const SComponentTransform& GetTransform() const;
	const SComponentPhysics& GetPhysics() const;
	const SComponentRender& GetRender() const;

	void TranslateTo(const XMVECTOR& Prime);
	void RotatePitchTo(float Prime);
	void RotateYawTo(float Prime);
	void RotateRollTo(float Prime);
	void ScaleTo(const XMVECTOR& Prime);

	void Translate(const XMVECTOR& Delta);
	void RotatePitch(float Delta);
	void RotateYaw(float Delta);
	void RotateRoll(float Delta);
	void Scale(const XMVECTOR& Delta);

	void SetLinearAcceleration(const XMVECTOR& Prime);
	void SetLinearVelocity(const XMVECTOR& Prime);
	void AddLinearAcceleration(const XMVECTOR& Delta);
	void AddLinearVelocity(const XMVECTOR& Delta);

// Transform, Physics, Outer bounding sphere (instance)
public:
	const SComponentTransform& GetInstanceTransform(const std::string& InstanceName) const;
	const SComponentPhysics& GetInstancePhysics(const std::string& InstanceName) const;
	const SBoundingVolume& GetInstanceOuterBoundingSphere(const std::string& InstanceName) const;

	void TranslateInstanceTo(const std::string& InstanceName, const XMVECTOR& Prime);
	void RotateInstancePitchTo(const std::string& InstanceName, float Prime);
	void RotateInstanceYawTo(const std::string& InstanceName, float Prime);
	void RotateInstanceRollTo(const std::string& InstanceName, float Prime);
	void ScaleInstanceTo(const std::string& InstanceName, const XMVECTOR& Prime);

	void TranslateInstance(const std::string& InstanceName, const XMVECTOR& Delta);
	void RotateInstancePitch(const std::string& InstanceName, float Delta);
	void RotateInstanceYaw(const std::string& InstanceName, float Delta);
	void RotateInstanceRoll(const std::string& InstanceName, float Delta);
	void ScaleInstance(const std::string& InstanceName, const XMVECTOR& Delta);

	void SetInstanceLinearAcceleration(const std::string& InstanceName, const XMVECTOR& Prime);
	void SetInstanceLinearVelocity(const std::string& InstanceName, const XMVECTOR& Prime);
	void AddInstanceLinearAcceleration(const std::string& InstanceName, const XMVECTOR& Delta);
	void AddInstanceLinearVelocity(const std::string& InstanceName, const XMVECTOR& Delta);

// Material
public:
	void AddMaterial(const CMaterialData& MaterialData);
	void SetMaterial(size_t Index, const CMaterialData& MaterialData);
	size_t GetMaterialCount() const;

	void ShouldIgnoreSceneMaterial(bool bShouldIgnore);
	bool ShouldIgnoreSceneMaterial() const;

public:
	void UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize);
	void UpdateMeshBuffer(size_t MeshIndex = 0);

public:
	void UpdateWorldMatrix();
	void UpdateInstanceWorldMatrix(const std::string& InstanceName, bool bUpdateInstanceBuffer = true);
	// @warning: this function doesn't update internal InstanceCPUData
	void UpdateInstanceWorldMatrix(const std::string& InstanceName, const XMMATRIX& WorldMatrix);
	void UpdateAllInstances(bool bUpdateWorldMatrix = true);

public:
	void SetInstanceHighlight(const std::string& InstanceName, bool bShouldHighlight);
	void SetAllInstancesHighlightOff();

private:
	void UpdateCBMaterial(const CMaterialData& MaterialData, uint32_t TotalMaterialCount) const;

public:
	void Animate(float DeltaTime);

private:
	void AnimateInstance(const std::string& InstanceName, float DeltaTime);
	void CalculateAnimatedBoneMatrices(const SMeshAnimation& CurrentAnimation, float AnimationTick, const SMeshTreeNode& Node, XMMATRIX ParentTransform);

public:
	void Draw(EFlagsObject3DRendering eFlagsRendering = EFlagsObject3DRendering::None, size_t OneInstanceIndex = 0) const;

public:
	bool ShouldTessellate() const;
	void ShouldTessellate(bool Value);

	void SetTessFactorData(const CObject3D::SCBTessFactorData& Data);
	const CObject3D::SCBTessFactorData& GetTessFactorData() const;

	void SetDisplacementData(const CObject3D::SCBDisplacementData& Data);
	const CObject3D::SCBDisplacementData& GetDisplacementData() const;

public:
	bool IsCreated() const;
	bool IsRigged() const;
	bool IsInstanced() const;
	bool IsPickable() const;
	void IsPickable(bool NewValue);
	bool IsTransparent() const;
	void IsTransparent(bool NewValue);
	bool IsGPUSkinned() const;
	size_t GetInstanceCount() const;
	const SMESHData& GetModel() const;
	SMESHData& GetModel();
	void SetName(const std::string& Name);
	const std::string& GetName() const;
	void SetModelFileName(const std::string& FileName);
	const std::string& GetModelFileName() const;
	const std::string& GetOB3DFileName() const;
	const std::map<std::string, size_t>& GetInstanceNameToIndexMap() const;
	CMaterialTextureSet* GetMaterialTextureSet(size_t iMaterial);
	void SetOuterBoundingSphereCenterOffset(const XMVECTOR& Center);
	void SetOuterBoundingSphereRadiusBias(float Radius);
	const XMVECTOR& GetOuterBoundingSphereCenterOffset() const;
	float GetOuterBoundingSphereRadius() const;
	float GetOuterBoundingSphereRadiusBias() const;
	const SBoundingVolume& GetOuterBoundingSphere() const;
	const XMMATRIX& GetWorldMatrix() const;

private:
	void LimitFloatRotation(float& Value, const float Min, const float Max);

public:
	static constexpr float KRotationMaxLimit{ +XM_2PI };
	static constexpr float KRotationMinLimit{ -XM_2PI };
	static constexpr float KScalingMaxLimit{ +100.0f };
	static constexpr float KScalingMinLimit{ +0.001f };
	static constexpr size_t KMaxAnimationNameLength{ 15 };
	static constexpr uint32_t KMaxBoneMatrixCount{ 60 };

private:
	static constexpr int32_t KAnimationTextureWidth{ 4 * (int32_t)KMaxBoneMatrixCount };
	static constexpr int32_t KAnimationTextureReservedHeight{ 1 };
	static constexpr int32_t KAnimationTextureReservedFirstPixelCount{ 2 };

private:
	ID3D11Device* const										m_PtrDevice{};
	ID3D11DeviceContext* const								m_PtrDeviceContext{};

private:
	SComponentTransform										m_ComponentTransform{};
	SComponentPhysics										m_ComponentPhysics{};
	SComponentRender										m_ComponentRender{};
	EFlagsRendering											m_eFlagsRendering{};
	XMMATRIX												m_WorldMatrix{ XMMatrixIdentity() };
	SBoundingVolume											m_OuterBoundingSphere{};
	std::vector<SBoundingVolume>							m_vInnerBoundingVolumes{};

private:
	std::string												m_Name{};
	std::string												m_ModelFileName{};
	std::string												m_OB3DFileName{};
	bool													m_bIsCreated{ false };
	bool													m_bIsPickable{ true };
	bool													m_bShouldTesselate{ false };
	std::unique_ptr<SMESHData>								m_Model{};
	std::vector<std::unique_ptr<CMaterialTextureSet>>		m_vMaterialTextureSets{};

private:
	std::vector<SMeshBuffers>								m_vMeshBuffers{};
	std::vector<SInstanceBuffer>							m_vInstanceBuffers{};

private:
	std::unique_ptr<CConstantBuffer>						m_CBMaterial{};
	mutable SCBMaterialData									m_CBMaterialData{};
	SCBTessFactorData										m_CBTessFactorData{};
	SCBDisplacementData										m_CBDisplacementData{};

private:
	float													m_AnimationTick{};
	uint32_t												m_CurrentAnimationID{};
	size_t													m_CurrentAnimationPlayCount{};
	EAnimationOption										m_eCurrentAnimationOption{};

private:
	XMMATRIX												m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	bool													m_bIsBakedAnimationLoaded{ false };
	std::unique_ptr<CTexture>								m_BakedAnimationTexture{};
	SCBAnimationData										m_CBAnimationData{};

private:
	std::unordered_map<EAnimationRegistrationType, size_t>	m_umapRegisteredAnimationTypeToIndex{};
	std::unordered_map<size_t, EAnimationRegistrationType>	m_umapRegisteredAnimationIndexToType{};
	std::vector<uint32_t>									m_vRegisteredAnimationIDs{};
	std::vector<float>										m_vAnimationBehaviorStartTicks{};

private:
	std::vector<SObject3DInstanceGPUData>					m_vInstanceGPUData{};
	std::vector<SObject3DInstanceCPUData>					m_vInstanceCPUData{};
	std::map<std::string, size_t>							m_mapInstanceNameToIndex{};
};

ENUM_CLASS_FLAG(CObject3D::EFlagsRendering)
