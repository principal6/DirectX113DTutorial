#pragma once

#include "SharedHeader.h"
#include "Texture.h"

static constexpr uint32_t KMaxWeightCount{ 4 };
static constexpr uint32_t KMaxBoneMatrixCount{ 60 };

class CGameWindow;

struct SVertex3D
{
	SVertex3D() {}
	SVertex3D(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord = XMVectorSet(0, 0, 0, 0)) :
		Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
	XMVECTOR TexCoord{};
	XMVECTOR Normal{};
};

struct SVertexAnimation
{
	uint32_t	BoneIDs[KMaxWeightCount]{};
	float		Weights[KMaxWeightCount]{};
};

struct SMesh
{
	vector<SVertex3D>			vVertices{};
	vector<SVertexAnimation>	vVerticesAnimation{};
	vector<STriangle>			vTriangles{};

	size_t						MaterialID{};
};

struct SModelNode
{
	struct SBlendWeight
	{
		uint32_t	MeshIndex{};
		uint32_t	VertexID{};
		float		Weight{};
	};

	int32_t					Index{};
	string					Name{};

	int32_t					ParentNodeIndex{};
	vector<int32_t>			vChildNodeIndices{};
	XMMATRIX				MatrixTransformation{};

	bool					bIsBone{ false };
	uint32_t				BoneIndex{};
	vector<SBlendWeight>	vBlendWeights{};
	XMMATRIX				MatrixBoneOffset{};
};

struct SModelNodeAnimation
{
	struct SKey
	{
		float		Time{};
		XMVECTOR	Value{};
	};

	uint32_t		Index{};
	string			NodeName{};
	vector<SKey>	vPositionKeys{};
	vector<SKey>	vRotationKeys{};
	vector<SKey>	vScalingKeys{};
};

struct SModelAnimation
{
	vector<SModelNodeAnimation>		vNodeAnimations{};

	float							Duration{};
	float							TicksPerSecond{};

	unordered_map<string, size_t>	mapNodeAnimationNameToIndex{};
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

struct SMaterial
{
	SMaterial() {}
	SMaterial(const XMFLOAT3& UniversalColor) : MaterialAmbient{ UniversalColor }, MaterialDiffuse{ UniversalColor }, MaterialSpecular{ UniversalColor } {}

	XMFLOAT3			MaterialAmbient{};
	float				SpecularExponent{ 1 };
	XMFLOAT3			MaterialDiffuse{};
	float				SpecularIntensity{ 0 };
	XMFLOAT3			MaterialSpecular{};

	bool				bHasTexture{ false };
	string				TextureFileName{};
	bool				bHasEmbeddedTexture{ false };
	vector<uint8_t>		vEmbeddedTextureRawData{};
};

struct SModel
{
	vector<SMesh>					vMeshes{};
	vector<SMaterial>				vMaterials{};

	vector<SModelNode>				vNodes{};
	unordered_map<string, size_t>	mapNodeNameToIndex{};
	uint32_t						ModelBoneCount{};

	vector<SModelAnimation>			vAnimations{};
	bool							bIsAnimated{};
};

class CObject3D
{
	friend class CGameWindow;

public:
	CObject3D(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, CGameWindow* PtrGameWindow) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGameWindow{ PtrGameWindow }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
		assert(m_PtrGameWindow);
	}
	~CObject3D() {}

	void Create(const SMesh& Mesh, const SMaterial& Material = SMaterial());
	void Create(const vector<SMesh>& vMeshes, const vector<SMaterial>& vMaterials);
	void Create(const SModel& Model);

	void Animate();

private:
	void CreateMeshBuffers(size_t MeshIndex, bool IsAnimated);

	void CalculateAnimatedBoneMatrices(const SModelNode& Node, XMMATRIX ParentTransform);

	void Draw() const;

	void DrawNormals() const;

private:
	ID3D11Device*					m_PtrDevice{};
	ID3D11DeviceContext*			m_PtrDeviceContext{};
	CGameWindow*					m_PtrGameWindow{};

private:
	SModel							m_Model{};
	vector<SMeshBuffers>			m_vMeshBuffers{};
	vector<unique_ptr<CTexture>>	m_vEmbeddedTextures{};

	XMMATRIX						m_AnimatedBoneMatrices[KMaxBoneMatrixCount]{};
	size_t							m_CurrentAnimationIndex{};
	float							m_CurrentAnimationTick{};
};