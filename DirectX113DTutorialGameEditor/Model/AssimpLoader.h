#pragma once

#include "../Core/SharedHeader.h"
#include "ObjectTypes.h"

struct aiScene;
struct aiString;
struct aiNode;
struct SMESHData;
class CMaterialData;
struct STextureData;
enum class ETextureType;

class CAssimpLoader
{
public:
	CAssimpLoader() {}
	~CAssimpLoader() {}

	void LoadStaticModelFromFile(const std::string& FileName, SMESHData* const Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void LoadAnimatedModelFromFile(const std::string& FileName, SMESHData* const Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void AddAnimationFromFile(const std::string& FileName, SMESHData* const Model);

private:
	void LoadMeshesFromFile(const aiScene* const Scene, std::vector<SMesh>& vMeshes);
	void LoadMaterialsFromFile(const aiScene* const Scene, ID3D11Device* const Device, ID3D11DeviceContext* const DeviceContext,
		std::vector<CMaterialData>& vMaterialData);

private:
	void LoadTextureData(const aiScene* const Scene, const aiString* const TextureFileName, CMaterialData& MaterialData, ETextureType eTextureType);

private:
	void LoadNodes(const aiScene* const Scene, aiNode* const aiCurrentNode, int32_t ParentNodeIndex, SMESHData* const Model);
	void LoadBones(const aiScene* const Scene, SMESHData* const Model);
	void MatchWeightsAndVertices(SMESHData* const Model);
	void LoadAnimations(const aiScene* const Scene, SMESHData* const Model);

private:
	const aiScene*		m_Scene{};
};