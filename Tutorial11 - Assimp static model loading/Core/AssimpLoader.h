#pragma once

#include "Object3D.h"
#include "GameObject.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#pragma comment(lib, "assimp-vc142-mtd.lib")

static XMVECTOR ConvertaiVector3DToXMVECTOR(const aiVector3D& Input, float w)
{
	return XMVectorSet(Input.x, Input.y, Input.z, w);
}

static vector<SMaterial> LoadMaterialsFromFile(const string& FileName)
{
	vector<SMaterial> vMaterials{};

	Assimp::Importer AssimpImporter{};
	const aiScene* Scene{ AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded |
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_FixInfacingNormals |
		aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent | aiProcess_GenSmoothNormals) };

	assert(Scene);
	assert(Scene->HasMeshes());
	
	unsigned int MaterialCount{ Scene->mNumMaterials };
	vMaterials.resize(MaterialCount);

	for (unsigned int iMaterial{}; iMaterial < MaterialCount; ++iMaterial)
	{
		aiMaterial* aiCurrentMaterial = Scene->mMaterials[iMaterial];
		SMaterial& CurrentMaterial{ vMaterials[iMaterial] };

		aiColor4D aiAmbient{};
		aiColor4D aiDiffuse{};
		aiColor4D aiSpecular{};
		float aiShininess{};
		aiGetMaterialColor(aiCurrentMaterial, AI_MATKEY_COLOR_AMBIENT, &aiAmbient);
		aiGetMaterialColor(aiCurrentMaterial, AI_MATKEY_COLOR_DIFFUSE, &aiDiffuse);
		aiGetMaterialColor(aiCurrentMaterial, AI_MATKEY_COLOR_SPECULAR, &aiSpecular);
		aiGetMaterialFloat(aiCurrentMaterial, AI_MATKEY_SHININESS, &aiShininess);

		// Load the texture only once per model (not per mesh).
		// i.e. multi-texture not supported.
		if (Scene->HasTextures())
		{
			aiString TextureFileName{};
			aiGetMaterialTexture(aiCurrentMaterial, aiTextureType_DIFFUSE, 0, &TextureFileName);
			
			CurrentMaterial.bHasTexture = true;
			CurrentMaterial.TextureFileName = TextureFileName.C_Str();

			auto aiTexture{ Scene->GetEmbeddedTexture(CurrentMaterial.TextureFileName.c_str()) };
			if (aiTexture)
			{
				unsigned int TexelCount{ aiTexture->mWidth / 4 };
				if (aiTexture->mHeight) TexelCount *= aiTexture->mHeight;

				CurrentMaterial.bHasEmbeddedTexture = true;
				CurrentMaterial.vEmbeddedTextureRawData.reserve(aiTexture->mWidth);
				for (unsigned int iTexel = 0; iTexel < TexelCount; ++iTexel)
				{
					aiTexel& Texel{ aiTexture->pcData[iTexel] };
					CurrentMaterial.vEmbeddedTextureRawData.emplace_back((uint8_t)Texel.b);
					CurrentMaterial.vEmbeddedTextureRawData.emplace_back((uint8_t)Texel.g);
					CurrentMaterial.vEmbeddedTextureRawData.emplace_back((uint8_t)Texel.r);
					CurrentMaterial.vEmbeddedTextureRawData.emplace_back((uint8_t)Texel.a);
				}
			}
		}

		CurrentMaterial.MaterialAmbient.x = aiAmbient.r;
		CurrentMaterial.MaterialAmbient.y = aiAmbient.g;
		CurrentMaterial.MaterialAmbient.z = aiAmbient.b;

		CurrentMaterial.MaterialDiffuse.x = aiDiffuse.r;
		CurrentMaterial.MaterialDiffuse.y = aiDiffuse.g;
		CurrentMaterial.MaterialDiffuse.z = aiDiffuse.b;

		CurrentMaterial.MaterialSpecular.x = aiSpecular.r;
		CurrentMaterial.MaterialSpecular.y = aiSpecular.g;
		CurrentMaterial.MaterialSpecular.z = aiSpecular.b;

		CurrentMaterial.SpecularExponent = aiShininess;
		CurrentMaterial.SpecularIntensity = 1.0f;
	}

	return vMaterials;
}

static vector<SMesh> LoadMeshesFromFile(const string& FileName)
{
	vector<SMesh> vMeshes{};

	Assimp::Importer AssimpImporter{};
	const aiScene* Scene{ AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded |
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_FixInfacingNormals |
		aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent | aiProcess_GenSmoothNormals) };

	assert(Scene);
	assert(Scene->HasMeshes());

	unsigned int MeshCount{ Scene->mNumMeshes };
	vMeshes.resize(MeshCount);

	for (unsigned int iMesh{}; iMesh < MeshCount; ++iMesh)
	{
		aiMesh* aiCurrentMesh{ Scene->mMeshes[iMesh] };
		SMesh& CurrentMesh{ vMeshes[iMesh] };
		CurrentMesh.MaterialID = aiCurrentMesh->mMaterialIndex;

		assert(aiCurrentMesh->HasPositions());
		{
			unsigned int VertexCount{ aiCurrentMesh->mNumVertices };

			XMVECTOR Position{};
			XMVECTOR TexCoord{};
			XMVECTOR Normal{};

			CurrentMesh.vVertices.reserve(VertexCount);
			for (unsigned int iVertex{}; iVertex < VertexCount; ++iVertex)
			{
				Position = ConvertaiVector3DToXMVECTOR(aiCurrentMesh->mVertices[iVertex], 1);

				// Use only one TexCoords
				if (aiCurrentMesh->mTextureCoords[0])
				{
					TexCoord = ConvertaiVector3DToXMVECTOR(aiCurrentMesh->mTextureCoords[0][iVertex], 1);
				}

				Normal = ConvertaiVector3DToXMVECTOR(aiCurrentMesh->mNormals[iVertex], 0);

				CurrentMesh.vVertices.emplace_back();
				CurrentMesh.vVertices.back().Position = Position;
				CurrentMesh.vVertices.back().TexCoord = TexCoord;
				CurrentMesh.vVertices.back().Normal = Normal;
			}
		}

		assert(aiCurrentMesh->HasFaces());
		{
			unsigned int FaceCount{ aiCurrentMesh->mNumFaces };

			CurrentMesh.vTriangles.reserve(FaceCount);
			for (unsigned int iFace{}; iFace < FaceCount; ++iFace)
			{
				assert(aiCurrentMesh->mFaces[iFace].mNumIndices == 3);
				auto& Indices = aiCurrentMesh->mFaces[iFace].mIndices;

				CurrentMesh.vTriangles.emplace_back(Indices[0], Indices[1], Indices[2]);
			}
		}
	}

	return vMeshes;
}

static SModel LoadModelFromFile(const string& FileName)
{
	vector<SMesh> vMesh{ LoadMeshesFromFile((FileName)) };
	vector<SMaterial> vMaterial{ LoadMaterialsFromFile((FileName)) };
	
	return { vMesh, vMaterial };
}