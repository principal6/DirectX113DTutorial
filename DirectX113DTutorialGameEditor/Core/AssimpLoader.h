#pragma once

#include "SharedHeader.h"
#include "Material.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#pragma comment(lib, "assimp-vc142-mtd.lib")

static constexpr uint32_t KMaxWeightCount{ 4 };
static constexpr uint32_t KMaxBoneMatrixCount{ 60 };

struct SVertex3D
{
	SVertex3D() {}
	SVertex3D(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord = XMVectorSet(0, 0, 0, 0)) :
		Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
	XMVECTOR TexCoord{};
	XMVECTOR Normal{};
	XMVECTOR Tangent{};
	XMVECTOR Bitangent{};
};

struct SVertexAnimation
{
	uint32_t	BoneIDs[KMaxWeightCount]{};
	float		Weights[KMaxWeightCount]{};
};

struct SInstanceGPUData
{
	XMMATRIX	InstanceWorldMatrix{ KMatrixIdentity };
};

struct SMesh
{
	std::vector<SVertex3D>			vVertices{};
	std::vector<SVertexAnimation>	vVerticesAnimation{};
	std::vector<STriangle>			vTriangles{};

	size_t							MaterialID{};
};

struct SModel
{
	struct SNode
	{
		struct SBlendWeight
		{
			uint32_t	MeshIndex{};
			uint32_t	VertexID{};
			float		Weight{};
		};

		int32_t					Index{};
		std::string				Name{};

		int32_t					ParentNodeIndex{};
		std::vector<int32_t>	vChildNodeIndices{};
		XMMATRIX				MatrixTransformation{};

		bool						bIsBone{ false };
		uint32_t					BoneIndex{};
		std::vector<SBlendWeight>	vBlendWeights{};
		XMMATRIX					MatrixBoneOffset{};
	};

	struct SAnimation
	{
		struct SNodeAnimation
		{
			struct SKey
			{
				float		Time{};
				XMVECTOR	Value{};
			};

			uint32_t			Index{};
			std::string			NodeName{};
			std::vector<SKey>	vPositionKeys{};
			std::vector<SKey>	vRotationKeys{};
			std::vector<SKey>	vScalingKeys{};
		};

		std::vector<SNodeAnimation>				vNodeAnimations{};

		float									Duration{};
		float									TicksPerSecond{};

		std::unordered_map<std::string, size_t>	mapNodeAnimationNameToIndex{};

		std::string								Name{};
	};

	std::vector<SMesh>						vMeshes{};
	std::vector<CMaterial>					vMaterials{};

	std::vector<SNode>						vNodes{};
	std::unordered_map<std::string, size_t>	mapNodeNameToIndex{};
	uint32_t								ModelBoneCount{};

	std::vector<SAnimation>					vAnimations{};
	bool									bIsModelAnimated{};
	bool									bUseMultipleTexturesInSingleMesh{ false };
};

class CAssimpLoader
{
public:
	CAssimpLoader() {}
	~CAssimpLoader() {}

	void LoadStaticModelFromFile(const std::string& FileName, SModel& Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void LoadAnimatedModelFromFile(const std::string& FileName, SModel& Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void AddAnimationFromFile(const std::string& FileName, SModel& Model);

private:
	XMVECTOR ConvertaiVector3DToXMVECTOR(const aiVector3D& Vector, float w);
	XMVECTOR ConvertaiQuaternionToXMVECTOR(const aiQuaternion& Quaternion);
	XMMATRIX ConvertaiMatrix4x4ToXMMATRIX(const aiMatrix4x4& Matrix);

	void LoadMeshesFromFile(const aiScene* const Scene, std::vector<SMesh>& vMeshes);
	void LoadMaterialsFromFile(const aiScene* const Scene, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, std::vector<CMaterial>& vMaterials);

private:
	void LoadTextureData(const aiScene* const Scene, const aiString& TextureFileName, CMaterial& Material, CMaterial::CTexture::EType eTextureType);

private:
	void LoadNodes(const aiScene* const Scene, aiNode* const aiCurrentNode, int32_t ParentNodeIndex, SModel& Model);
	void LoadBones(const aiScene* const Scene, SModel& Model);
	void MatchWeightsAndVertices(SModel& Model);
	void LoadAnimations(const aiScene* const Scene, SModel& Model);

private:
	Assimp::Importer	m_AssimpImporter{};
	const aiScene*		m_Scene{};
};