#pragma once

#include "Object3D.h"
#include "GameObject3D.h"

#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

#pragma comment(lib, "assimp-vc142-mtd.lib")

static XMVECTOR ConvertaiVector3DToXMVECTOR(const aiVector3D& Vector, float w);
static XMVECTOR ConvertaiQuaternionToXMVECTOR(const aiQuaternion& Quaternion);
static XMMATRIX ConvertaiMatrix4x4ToXMMATRIX(const aiMatrix4x4& Matrix);

static vector<SMaterial> LoadMaterialsFromFile(const aiScene* Scene);
static vector<SMesh> LoadMeshesFromFile(const aiScene* Scene);

static SModel LoadStaticModelFromFile(const string& FileName);
static SModel LoadAnimatedModelFromFile(const string& FileName);

static void LoadNodes(const aiScene* Scene, aiNode* aiCurrentNode, int32_t ParentNodeIndex, SModel& Model);
static void LoadBones(const aiScene* Scene, SModel& Model);
static void MatchWeightsAndVertices(SModel& Model);
static void LoadAnimations(const aiScene* Scene, SModel& Model);

static void SerializeNodes(const vector<SModelNode>& vNodes, uint32_t NodeIndex, uint32_t Depth, string& SerializedString);

static XMVECTOR ConvertaiVector3DToXMVECTOR(const aiVector3D& Vector, float w)
{
	return XMVectorSet(Vector.x, Vector.y, Vector.z, w);
}

static XMVECTOR ConvertaiQuaternionToXMVECTOR(const aiQuaternion& Quaternion)
{
	return XMVectorSet(Quaternion.x, Quaternion.y, Quaternion.z, Quaternion.w);
}

static XMMATRIX ConvertaiMatrix4x4ToXMMATRIX(const aiMatrix4x4& Matrix)
{
	return XMMatrixTranspose(XMMatrixSet(
		Matrix.a1, Matrix.a2, Matrix.a3, Matrix.a4,
		Matrix.b1, Matrix.b2, Matrix.b3, Matrix.b4,
		Matrix.c1, Matrix.c2, Matrix.c3, Matrix.c4,
		Matrix.d1, Matrix.d2, Matrix.d3, Matrix.d4
	));
}

static vector<SMaterial> LoadMaterialsFromFile(const aiScene* Scene)
{
	vector<SMaterial> vMaterials{};
	unsigned int MaterialCount{ Scene->mNumMaterials };
	vMaterials.resize(MaterialCount);

	for (unsigned int iMaterial{}; iMaterial < MaterialCount; ++iMaterial)
	{
		aiMaterial* aiCurrentMaterial{ Scene->mMaterials[iMaterial] };
		SMaterial& CurrentMaterial{ vMaterials[iMaterial] };

		aiColor4D aiAmbient{}, aiDiffuse{}, aiSpecular{};
		float aiShininess{};
		aiGetMaterialColor(aiCurrentMaterial, AI_MATKEY_COLOR_AMBIENT, &aiAmbient);
		aiGetMaterialColor(aiCurrentMaterial, AI_MATKEY_COLOR_DIFFUSE, &aiDiffuse);
		aiGetMaterialColor(aiCurrentMaterial, AI_MATKEY_COLOR_SPECULAR, &aiSpecular);
		aiGetMaterialFloat(aiCurrentMaterial, AI_MATKEY_SHININESS, &aiShininess);

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

static vector<SMesh> LoadMeshesFromFile(const aiScene* Scene)
{
	vector<SMesh> vMeshes{};
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

			XMVECTOR Position{}, TexCoord{}, Normal{};

			CurrentMesh.vVertices.reserve(VertexCount);
			for (unsigned int iVertex{}; iVertex < VertexCount; ++iVertex)
			{
				Position = ConvertaiVector3DToXMVECTOR(aiCurrentMesh->mVertices[iVertex], 1);

				// Use only one TexCoords system
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

static SModel LoadStaticModelFromFile(const string& FileName)
{
	SModel StaticModel{};

	Assimp::Importer AssimpImporter{};
	AssimpImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
	const aiScene* Scene{ AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded | aiProcess_PreTransformVertices |
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes |
		aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality | aiProcess_FixInfacingNormals |
		aiProcess_Triangulate | aiProcess_SplitByBoneCount | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent | aiProcess_GenSmoothNormals) };

	assert(Scene);
	assert(Scene->HasMeshes());
	assert(Scene->mRootNode);

	StaticModel.vMeshes = LoadMeshesFromFile(Scene);
	StaticModel.vMaterials = LoadMaterialsFromFile(Scene);
	
	return StaticModel;
}

static SModel LoadAnimatedModelFromFile(const string& FileName)
{
	SModel AnimatedModel{};

	Assimp::Importer AssimpImporter{};
	AssimpImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
	const aiScene* Scene{ AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded |
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
		aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality | aiProcess_FixInfacingNormals |
		aiProcess_Triangulate | aiProcess_SplitByBoneCount | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent | aiProcess_GenSmoothNormals) };

	assert(Scene);
	assert(Scene->HasMeshes());
	assert(Scene->mRootNode);

	AnimatedModel.vMeshes = LoadMeshesFromFile(Scene);
	for (auto& Mesh : AnimatedModel.vMeshes)
	{
		Mesh.vVerticesAnimation.resize(Mesh.vVertices.size());
	}

	AnimatedModel.vMaterials = LoadMaterialsFromFile(Scene);

	// Scene에서 재귀적으로 Node의 Tree를 만든다.
	LoadNodes(Scene, Scene->mRootNode, -1, AnimatedModel);

	// Node의 Name을 통해 Index를 찾을 수 있도록 사상(map)한다.
	for (auto& Node : AnimatedModel.vNodes)
	{
		AnimatedModel.mapNodeNameToIndex[Node.Name] = Node.Index;
	}

	// Scene에서 각 Mesh에 연결된 Bone들을 불러온다.
	LoadBones(Scene, AnimatedModel);

	// 각 Mesh의 Vertex에 각 Bone의 BlendWeights를 저장한다.
	MatchWeightsAndVertices(AnimatedModel);

	// Scene에서 Animation을 불러온다.
	LoadAnimations(Scene, AnimatedModel);

	// Animation의 Name을 통해 Index를 찾을 수 있도록 사상(map)한다. 
	for (auto& Animation : AnimatedModel.vAnimations)
	{
		for (auto& NodeAnimation : Animation.vNodeAnimations)
		{
			Animation.mapNodeAnimationNameToIndex[NodeAnimation.NodeName] = NodeAnimation.Index;
		}
	}

	AnimatedModel.bIsModelAnimated = true;

	return AnimatedModel;
}

static void LoadNodes(const aiScene* Scene, aiNode* aiCurrentNode, int32_t ParentNodeIndex, SModel& Model)
{
	Model.vNodes.emplace_back();
	SModelNode& Node{ Model.vNodes.back() };

	int32_t NodeIndex{ static_cast<int32_t>(Model.vNodes.size() - 1) };

	Node.Index = NodeIndex;
	Node.Name = aiCurrentNode->mName.C_Str();
	Node.ParentNodeIndex = ParentNodeIndex;
	Node.MatrixTransformation = ConvertaiMatrix4x4ToXMMATRIX(aiCurrentNode->mTransformation);

	if (Node.ParentNodeIndex != -1)
	{
		// 부모 Node에 현재 Node를 Child로 등록한다.
		Model.vNodes[Node.ParentNodeIndex].vChildNodeIndices.emplace_back(NodeIndex);
	}

	if (aiCurrentNode->mNumChildren)
	{
		// Child Node가 있는 경우 각 Child를 불러온다.
		for (unsigned int iChild = 0; iChild < aiCurrentNode->mNumChildren; ++iChild)
		{
			LoadNodes(Scene, aiCurrentNode->mChildren[iChild], NodeIndex, Model);
		}
	}
}

static void LoadBones(const aiScene* Scene, SModel& Model)
{
	unordered_map<string, uint32_t> mapBones{};

	int TotalBoneCount{};
	for (unsigned int iMesh = 0; iMesh < Model.vMeshes.size(); ++iMesh)
	{
		const aiMesh* aiMesh{ Scene->mMeshes[iMesh] };
		if (aiMesh->HasBones())
		{
			for (unsigned int iBone = 0; iBone < aiMesh->mNumBones; ++iBone)
			{
				const aiBone* aiBone{ aiMesh->mBones[iBone] };

				string BoneName{ aiBone->mName.C_Str() };
				size_t BoneNodeIndex{ Model.mapNodeNameToIndex[BoneName] };
				SModelNode& BoneNode{ Model.vNodes[BoneNodeIndex] };

				// BoneCount는 현재 Bone이 처음 나온 경우에만 증가시켜야 한다! (동일한 Bone이 서로 다른 Mesh에서 참조될 수 있기 때문)
				if (mapBones[BoneName] == 0)
				{
					BoneNode.BoneIndex = TotalBoneCount;
					++TotalBoneCount;

					BoneNode.bIsBone = true;
					BoneNode.MatrixBoneOffset = ConvertaiMatrix4x4ToXMMATRIX(aiBone->mOffsetMatrix);
				}

				// 여러 Mesh가 같은 Bone을 참조할 수 있으므로 아래 코드는 매번 실행되어야 한다.
				for (unsigned int iWeight = 0; iWeight < aiBone->mNumWeights; ++iWeight)
				{
					SModelNode::SBlendWeight BlendWeight{ iMesh, aiBone->mWeights[iWeight].mVertexId, aiBone->mWeights[iWeight].mWeight };

					BoneNode.vBlendWeights.emplace_back(BlendWeight);
				}

				++mapBones[BoneName];
			}
		}
	}

	Model.ModelBoneCount = TotalBoneCount;
}

static void MatchWeightsAndVertices(SModel& Model)
{
	vector<unordered_map<uint32_t, uint32_t>> mapBlendCountPerVertexPerMesh{};
	mapBlendCountPerVertexPerMesh.resize(Model.vMeshes.size());

	for (auto& BoneNode : Model.vNodes)
	{
		for (size_t iBlendWeight = 0; iBlendWeight < BoneNode.vBlendWeights.size(); ++iBlendWeight)
		{
			const SModelNode::SBlendWeight& BlendWeight{ BoneNode.vBlendWeights[iBlendWeight] };
			const uint32_t& MeshID{ BlendWeight.MeshIndex };
			const uint32_t& VertexID{ BlendWeight.VertexID };
			const uint32_t& BlendIndexPerVertex{ mapBlendCountPerVertexPerMesh[MeshID][VertexID] };

			if (VertexID == 64)
			{
				int deb{};
			}

			if (mapBlendCountPerVertexPerMesh[MeshID][VertexID] >= KMaxWeightCount) continue;

			SMesh& Mesh{ Model.vMeshes[BlendWeight.MeshIndex] };
			Mesh.vVerticesAnimation[VertexID].BoneIDs[BlendIndexPerVertex] = BoneNode.BoneIndex;
			Mesh.vVerticesAnimation[VertexID].Weights[BlendIndexPerVertex] = BlendWeight.Weight;

			++mapBlendCountPerVertexPerMesh[MeshID][VertexID];
		}
	}
}

static void LoadAnimations(const aiScene* Scene, SModel& Model)
{
	if (Scene->mNumAnimations)
	{
		Model.vAnimations.resize(Scene->mNumAnimations);
		for (unsigned int iAnimation = 0; iAnimation < Scene->mNumAnimations; ++iAnimation)
		{
			const aiAnimation* aiAnimation{ Scene->mAnimations[iAnimation] };
			const unsigned int& ChannelCount{ aiAnimation->mNumChannels };

			SModelAnimation& Animation{ Model.vAnimations[iAnimation] };
			Animation.TicksPerSecond = static_cast<float>(aiAnimation->mTicksPerSecond);
			Animation.Duration = static_cast<float>(aiAnimation->mDuration);
			Animation.vNodeAnimations.resize(ChannelCount);
			for (unsigned int iChannel = 0; iChannel < ChannelCount; ++iChannel)
			{
				const aiNodeAnim* Channel{ aiAnimation->mChannels[iChannel] };
				SModelNodeAnimation& NodeAnimation{ Animation.vNodeAnimations[iChannel] };

				NodeAnimation.Index = iChannel;
				NodeAnimation.NodeName = Channel->mNodeName.C_Str();
				
				unsigned int PositionKeyCount{ Channel->mNumPositionKeys };
				NodeAnimation.vPositionKeys.resize(PositionKeyCount);
				for (unsigned int iPositionKey = 0; iPositionKey < PositionKeyCount; ++iPositionKey)
				{
					const aiVectorKey& aiKey{ Channel->mPositionKeys[iPositionKey] };
					SModelNodeAnimation::SKey& Key{ NodeAnimation.vPositionKeys[iPositionKey] };
					Key.Time = static_cast<float>(aiKey.mTime);
					Key.Value = ConvertaiVector3DToXMVECTOR(aiKey.mValue, 0);
				}

				unsigned int RotationKeyCount{ Channel->mNumRotationKeys };
				NodeAnimation.vRotationKeys.resize(RotationKeyCount);
				for (unsigned int iRotationKey = 0; iRotationKey < RotationKeyCount; ++iRotationKey)
				{
					const aiQuatKey& aiKey{ Channel->mRotationKeys[iRotationKey] };
					SModelNodeAnimation::SKey& Key{ NodeAnimation.vRotationKeys[iRotationKey] };
					Key.Time = static_cast<float>(aiKey.mTime);
					Key.Value = ConvertaiQuaternionToXMVECTOR(aiKey.mValue);
				}

				unsigned int ScalingKeyCount{ Channel->mNumScalingKeys };
				NodeAnimation.vScalingKeys.resize(ScalingKeyCount);
				for (unsigned int iScalingKey = 0; iScalingKey < ScalingKeyCount; ++iScalingKey)
				{
					const aiVectorKey& aiKey{ Channel->mScalingKeys[iScalingKey] };
					SModelNodeAnimation::SKey& Key{ NodeAnimation.vScalingKeys[iScalingKey] };
					Key.Time = static_cast<float>(aiKey.mTime);
					Key.Value = ConvertaiVector3DToXMVECTOR(aiKey.mValue, 0);
				}
			}
		}
	}
}