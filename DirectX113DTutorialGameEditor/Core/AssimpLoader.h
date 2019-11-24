#pragma once

#include "SharedHeader.h"
#include "Material.h"

struct aiScene;
struct aiString;
struct aiNode;
struct SModel;

class CAssimpLoader
{
public:
	CAssimpLoader() {}
	~CAssimpLoader() {}

	void LoadStaticModelFromFile(const std::string& FileName, SModel* const Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void LoadAnimatedModelFromFile(const std::string& FileName, SModel* const Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void AddAnimationFromFile(const std::string& FileName, SModel* const Model);

private:
	void LoadMeshesFromFile(const aiScene* const Scene, std::vector<SMesh>& vMeshes);
	void LoadMaterialsFromFile(const aiScene* const Scene, ID3D11Device* const Device, ID3D11DeviceContext* const DeviceContext, std::vector<CMaterialData>& vMaterialData);

private:
	void LoadTextureData(const aiScene* const Scene, const aiString* const TextureFileName, CMaterialData& MaterialData, STextureData::EType eTextureType);

private:
	void LoadNodes(const aiScene* const Scene, aiNode* const aiCurrentNode, int32_t ParentNodeIndex, SModel* const Model);
	void LoadBones(const aiScene* const Scene, SModel* const Model);
	void MatchWeightsAndVertices(SModel* const Model);
	void LoadAnimations(const aiScene* const Scene, SModel* const Model);

private:
	const aiScene*		m_Scene{};
};