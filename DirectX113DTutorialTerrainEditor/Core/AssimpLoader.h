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

struct SModel
{
	vector<SMesh>					vMeshes{};
	vector<CMaterial>				vMaterials{};

	vector<SModelNode>				vNodes{};
	unordered_map<string, size_t>	mapNodeNameToIndex{};
	uint32_t						ModelBoneCount{};

	vector<SModelAnimation>			vAnimations{};
	bool							bIsModelAnimated{};
	bool							bUseMultipleTexturesInSingleMesh{ false };
};

class CAssimpLoader
{
public:
	CAssimpLoader() {}
	~CAssimpLoader() {}

	void LoadStaticModelFromFile(const string& FileName, SModel& Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void LoadAnimatedModelFromFile(const string& FileName, SModel& Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);

private:
	XMVECTOR ConvertaiVector3DToXMVECTOR(const aiVector3D& Vector, float w);
	XMVECTOR ConvertaiQuaternionToXMVECTOR(const aiQuaternion& Quaternion);
	XMMATRIX ConvertaiMatrix4x4ToXMMATRIX(const aiMatrix4x4& Matrix);

	vector<SMesh> LoadMeshesFromFile(const aiScene* Scene);
	vector<CMaterial> LoadMaterialsFromFile(const aiScene* Scene, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);

	void LoadNodes(const aiScene* Scene, aiNode* aiCurrentNode, int32_t ParentNodeIndex, SModel& Model);
	void LoadBones(const aiScene* Scene, SModel& Model);
	void MatchWeightsAndVertices(SModel& Model);
	void LoadAnimations(const aiScene* Scene, SModel& Model);
};