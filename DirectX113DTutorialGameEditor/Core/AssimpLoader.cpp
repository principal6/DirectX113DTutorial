#include "AssimpLoader.h"
#include "PrimitiveGenerator.h"

using std::vector;
using std::string;
using std::unordered_map;

void CAssimpLoader::LoadStaticModelFromFile(const string& FileName, SModel& Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	m_Scene = m_AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded | 
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_PreTransformVertices |
		aiProcess_SplitLargeMeshes | aiProcess_FixInfacingNormals |
		aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

	assert(m_Scene);
	assert(m_Scene->HasMeshes());
	assert(m_Scene->mRootNode);

	LoadMeshesFromFile(m_Scene, Model.vMeshes);

	for (auto& Mesh : Model.vMeshes)
	{
		CalculateTangents(Mesh);
	}

	LoadMaterialsFromFile(m_Scene, Device, DeviceContext, Model.vMaterialData);
}

void CAssimpLoader::LoadAnimatedModelFromFile(const string& FileName, SModel& Model, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext)
{
	m_AssimpImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
	m_Scene = m_AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded |
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
		aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality | aiProcess_FixInfacingNormals |
		aiProcess_Triangulate | aiProcess_SplitByBoneCount | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent | aiProcess_GenSmoothNormals);

	assert(m_Scene);
	assert(m_Scene->HasMeshes());
	assert(m_Scene->mRootNode);

	LoadMeshesFromFile(m_Scene, Model.vMeshes);
	
	for (auto& Mesh : Model.vMeshes)
	{
		CalculateTangents(Mesh);
	}

	for (auto& Mesh : Model.vMeshes)
	{
		Mesh.vVerticesAnimation.resize(Mesh.vVertices.size());
	}

	LoadMaterialsFromFile(m_Scene, Device, DeviceContext, Model.vMaterialData);

	// Scene에서 재귀적으로 Node의 Tree를 만든다.
	LoadNodes(m_Scene, m_Scene->mRootNode, -1, Model);
	
	// Node의 Name을 통해 Index를 찾을 수 있도록 사상(map)한다.
	for (auto& Node : Model.vNodes)
	{
		Model.mapNodeNameToIndex[Node.Name] = Node.Index;
	}

	// Scene에서 각 Mesh에 연결된 Bone들을 불러온다.
	LoadBones(m_Scene, Model);

	// 각 Mesh의 Vertex에 각 Bone의 BlendWeights를 저장한다.
	MatchWeightsAndVertices(Model);

	// Scene에서 Animation을 불러온다.
	LoadAnimations(m_Scene, Model);

	// NodeAnimation의 Name을 통해 Index를 찾을 수 있도록 사상(map)한다. 
	for (auto& Animation : Model.vAnimations)
	{
		for (auto& NodeAnimation : Animation.vNodeAnimations)
		{
			Animation.mapNodeAnimationNameToIndex[NodeAnimation.NodeName] = NodeAnimation.Index;
		}
	}

	Model.bIsModelRigged = true;
}

void CAssimpLoader::AddAnimationFromFile(const string& FileName, SModel& Model)
{
	m_AssimpImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
	m_Scene = m_AssimpImporter.ReadFile(FileName, aiProcess_ConvertToLeftHanded |
		aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
		aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality | aiProcess_FixInfacingNormals |
		aiProcess_Triangulate | aiProcess_SplitByBoneCount | aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveComponent | aiProcess_GenSmoothNormals);

	assert(m_Scene);
	assert(m_Scene->HasMeshes());
	assert(m_Scene->mRootNode);

	LoadAnimations(m_Scene, Model);

	// NodeAnimation의 Name을 통해 Index를 찾을 수 있도록 사상(map)한다. 
	for (auto& Animation : Model.vAnimations)
	{
		for (auto& NodeAnimation : Animation.vNodeAnimations)
		{
			Animation.mapNodeAnimationNameToIndex[NodeAnimation.NodeName] = NodeAnimation.Index;
		}
	}
}

XMVECTOR CAssimpLoader::ConvertaiVector3DToXMVECTOR(const aiVector3D& Vector, float w)
{
	return XMVectorSet(Vector.x, Vector.y, Vector.z, w);
}

XMVECTOR CAssimpLoader::ConvertaiQuaternionToXMVECTOR(const aiQuaternion& Quaternion)
{
	return XMVectorSet(Quaternion.x, Quaternion.y, Quaternion.z, Quaternion.w);
}

XMMATRIX CAssimpLoader::ConvertaiMatrix4x4ToXMMATRIX(const aiMatrix4x4& Matrix)
{
	return XMMatrixTranspose(XMMatrixSet(
		Matrix.a1, Matrix.a2, Matrix.a3, Matrix.a4,
		Matrix.b1, Matrix.b2, Matrix.b3, Matrix.b4,
		Matrix.c1, Matrix.c2, Matrix.c3, Matrix.c4,
		Matrix.d1, Matrix.d2, Matrix.d3, Matrix.d4
	));
}

void CAssimpLoader::LoadMeshesFromFile(const aiScene* const Scene, vector<SMesh>& vMeshes)
{
	unsigned int MeshCount{ Scene->mNumMeshes };
	vMeshes.resize(MeshCount);
	for (unsigned int iMesh{}; iMesh < MeshCount; ++iMesh)
	{
		const aiMesh* const _aiMesh{ Scene->mMeshes[iMesh] };
		SMesh& Mesh{ vMeshes[iMesh] };
		Mesh.MaterialID = _aiMesh->mMaterialIndex;

		assert(_aiMesh->HasPositions());
		{
			unsigned int VertexCount{ _aiMesh->mNumVertices };

			XMVECTOR Position{}, TexCoord{}, Normal{};

			Mesh.vVertices.reserve(VertexCount);
			for (unsigned int iVertex{}; iVertex < VertexCount; ++iVertex)
			{
				Position = ConvertaiVector3DToXMVECTOR(_aiMesh->mVertices[iVertex], 1);

				// Use only one TexCoords system
				if (_aiMesh->mTextureCoords[0])
				{
					TexCoord = ConvertaiVector3DToXMVECTOR(_aiMesh->mTextureCoords[0][iVertex], 1);
				}

				Normal = ConvertaiVector3DToXMVECTOR(_aiMesh->mNormals[iVertex], 0);

				Mesh.vVertices.emplace_back();
				Mesh.vVertices.back().Position = Position;
				Mesh.vVertices.back().TexCoord = TexCoord;
				Mesh.vVertices.back().Normal = Normal;
			}
		}

		assert(_aiMesh->HasFaces());
		{
			unsigned int FaceCount{ _aiMesh->mNumFaces };

			Mesh.vTriangles.reserve(FaceCount);
			for (unsigned int iFace{}; iFace < FaceCount; ++iFace)
			{
				assert(_aiMesh->mFaces[iFace].mNumIndices == 3);
				auto& Indices = _aiMesh->mFaces[iFace].mIndices;

				Mesh.vTriangles.emplace_back(Indices[0], Indices[1], Indices[2]);
			}
		}
	}
}

void CAssimpLoader::LoadMaterialsFromFile(const aiScene* const Scene, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, 
	vector<CMaterialData>& vMaterialData)
{
	unsigned int MaterialCount{ Scene->mNumMaterials };
	vMaterialData.resize(MaterialCount);
	for (unsigned int iMaterial{}; iMaterial < MaterialCount; ++iMaterial)
	{
		const aiMaterial* const _aiMaterial{ Scene->mMaterials[iMaterial] };
		CMaterialData& MaterialData{ vMaterialData[iMaterial] };

		aiString aiMaterialName{};
		aiColor4D aiAmbient{}, aiDiffuse{}, aiSpecular{};
		float aiShininess{};
		float aiShininessStength{};
		aiGetMaterialString(_aiMaterial, AI_MATKEY_NAME, &aiMaterialName);
		aiGetMaterialColor(_aiMaterial, AI_MATKEY_COLOR_AMBIENT, &aiAmbient);
		aiGetMaterialColor(_aiMaterial, AI_MATKEY_COLOR_DIFFUSE, &aiDiffuse);
		aiGetMaterialColor(_aiMaterial, AI_MATKEY_COLOR_SPECULAR, &aiSpecular);
		aiGetMaterialFloat(_aiMaterial, AI_MATKEY_SHININESS, &aiShininess);
		aiGetMaterialFloat(_aiMaterial, AI_MATKEY_SHININESS_STRENGTH, &aiShininessStength);

		if (Scene->HasTextures())
		{
			aiString DiffuseTextureFileName{};
			aiString NormalTextureFileName{};
			aiString OpacityTextureFileName{};
			
			aiString MetalnessTextureFileName{};
			aiString AmbientOcclusionTextureFileName{};

			aiString DisplacementTextureFileName{};

			aiGetMaterialTexture(_aiMaterial, aiTextureType_DIFFUSE, 0, &DiffuseTextureFileName);
			aiGetMaterialTexture(_aiMaterial, aiTextureType_NORMALS, 0, &NormalTextureFileName);
			aiGetMaterialTexture(_aiMaterial, aiTextureType_OPACITY, 0, &OpacityTextureFileName);
			
			aiGetMaterialTexture(_aiMaterial, aiTextureType_METALNESS, 0, &MetalnessTextureFileName);
			aiGetMaterialTexture(_aiMaterial, aiTextureType_AMBIENT_OCCLUSION, 0, &AmbientOcclusionTextureFileName);

			aiGetMaterialTexture(_aiMaterial, aiTextureType_DISPLACEMENT, 0, &DisplacementTextureFileName);

			LoadTextureData(Scene, DiffuseTextureFileName, MaterialData, STextureData::EType::DiffuseTexture);
			LoadTextureData(Scene, NormalTextureFileName, MaterialData, STextureData::EType::NormalTexture);
			LoadTextureData(Scene, OpacityTextureFileName, MaterialData, STextureData::EType::OpacityTexture);
			LoadTextureData(Scene, DisplacementTextureFileName, MaterialData, STextureData::EType::DisplacementTexture);

			MaterialData.HasAnyTexture(true);
		}

		// @temporary?
		if (aiAmbient.r == 0.0f && aiAmbient.g == 0.0f && aiAmbient.b == 0.0f)
		{
			aiAmbient = aiDiffuse;
		}

		MaterialData.Name(aiMaterialName.C_Str());
		MaterialData.AmbientColor(XMFLOAT3(aiAmbient.r, aiAmbient.g, aiAmbient.b));
		MaterialData.DiffuseColor(XMFLOAT3(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b));
		MaterialData.SpecularColor(XMFLOAT3(aiSpecular.r, aiSpecular.g, aiSpecular.b));
		MaterialData.SpecularExponent(aiShininess);
		MaterialData.SpecularIntensity(aiShininessStength);
	}
}

void CAssimpLoader::LoadTextureData(const aiScene* const Scene, const aiString& TextureFileName, CMaterialData& MaterialData,
	STextureData::EType eTextureType)
{
	const aiTexture* const _aiTexture{ Scene->GetEmbeddedTexture(TextureFileName.C_Str()) };
	if (TextureFileName.length == 0 && _aiTexture == nullptr) return; // No texture
	STextureData& TextureData{ MaterialData.GetTextureData(eTextureType) };
	if (_aiTexture)
	{
		unsigned int TexelCount{ _aiTexture->mWidth / 4 };
		if (_aiTexture->mHeight) TexelCount *= _aiTexture->mHeight;

		TextureData.bHasTexture = true;
		TextureData.vRawData.reserve(_aiTexture->mWidth);
		for (unsigned int iTexel = 0; iTexel < TexelCount; ++iTexel)
		{
			const aiTexel& _aiTexel{ _aiTexture->pcData[iTexel] };
			TextureData.vRawData.emplace_back((uint8_t)_aiTexel.b);
			TextureData.vRawData.emplace_back((uint8_t)_aiTexel.g);
			TextureData.vRawData.emplace_back((uint8_t)_aiTexel.r);
			TextureData.vRawData.emplace_back((uint8_t)_aiTexel.a);
		}
	}
	else
	{
		TextureData.bHasTexture = true;
		TextureData.FileName = TextureFileName.C_Str();
	}
}

void CAssimpLoader::LoadNodes(const aiScene* const Scene, aiNode* const aiCurrentNode, int32_t ParentNodeIndex, SModel& Model)
{
	Model.vNodes.emplace_back();
	SModel::SNode& Node{ Model.vNodes.back() };

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

void CAssimpLoader::LoadBones(const aiScene* const Scene, SModel& Model)
{
	unordered_map<string, uint32_t> mapBones{};

	int TotalBoneCount{};
	for (unsigned int iMesh = 0; iMesh < Model.vMeshes.size(); ++iMesh)
	{
		const aiMesh* const _aiMesh{ Scene->mMeshes[iMesh] };
		if (_aiMesh->HasBones())
		{
			for (unsigned int iBone = 0; iBone < _aiMesh->mNumBones; ++iBone)
			{
				const aiBone* const _aiBone{ _aiMesh->mBones[iBone] };

				string BoneName{ _aiBone->mName.C_Str() };
				size_t BoneNodeIndex{ Model.mapNodeNameToIndex[BoneName] };
				SModel::SNode& BoneNode{ Model.vNodes[BoneNodeIndex] };

				// BoneCount는 현재 Bone이 처음 나온 경우에만 증가시켜야 한다! (동일한 Bone이 서로 다른 Mesh에서 참조될 수 있기 때문)
				if (mapBones[BoneName] == 0)
				{
					BoneNode.BoneIndex = TotalBoneCount;
					++TotalBoneCount;

					BoneNode.bIsBone = true;
					BoneNode.MatrixBoneOffset = ConvertaiMatrix4x4ToXMMATRIX(_aiBone->mOffsetMatrix);
				}

				// 여러 Mesh가 같은 Bone을 참조할 수 있으므로 아래 코드는 매번 실행되어야 한다.
				for (unsigned int iWeight = 0; iWeight < _aiBone->mNumWeights; ++iWeight)
				{
					SModel::SNode::SBlendWeight BlendWeight{ iMesh, _aiBone->mWeights[iWeight].mVertexId, _aiBone->mWeights[iWeight].mWeight };

					BoneNode.vBlendWeights.emplace_back(BlendWeight);
				}

				++mapBones[BoneName];
			}
		}
	}

	Model.ModelBoneCount = TotalBoneCount;
}

void CAssimpLoader::MatchWeightsAndVertices(SModel& Model)
{
	vector<unordered_map<uint32_t, uint32_t>> mapBlendCountPerVertexPerMesh{};
	mapBlendCountPerVertexPerMesh.resize(Model.vMeshes.size());

	for (auto& BoneNode : Model.vNodes)
	{
		for (size_t iBlendWeight = 0; iBlendWeight < BoneNode.vBlendWeights.size(); ++iBlendWeight)
		{
			const SModel::SNode::SBlendWeight& BlendWeight{ BoneNode.vBlendWeights[iBlendWeight] };
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

void CAssimpLoader::LoadAnimations(const aiScene* const Scene, SModel& Model)
{
	if (Scene->mNumAnimations)
	{
		unsigned int aiAnimationCount{ Scene->mNumAnimations };
		for (unsigned int iaiAnimation = 0; iaiAnimation < aiAnimationCount; ++iaiAnimation)
		{
			const aiAnimation* const _aiAnimation{ Scene->mAnimations[iaiAnimation] };
			const unsigned int& ChannelCount{ _aiAnimation->mNumChannels };

			Model.vAnimations.emplace_back();
			SModel::SAnimation& Animation{ Model.vAnimations.back() };
			Animation.TicksPerSecond = static_cast<float>(_aiAnimation->mTicksPerSecond);
			Animation.Duration = static_cast<float>(_aiAnimation->mDuration);
			Animation.vNodeAnimations.resize(ChannelCount);
			for (unsigned int iChannel = 0; iChannel < ChannelCount; ++iChannel)
			{
				const aiNodeAnim* const aiChannel{ _aiAnimation->mChannels[iChannel] };
				SModel::SAnimation::SNodeAnimation& NodeAnimation{ Animation.vNodeAnimations[iChannel] };

				NodeAnimation.Index = iChannel;
				NodeAnimation.NodeName = aiChannel->mNodeName.C_Str();

				unsigned int PositionKeyCount{ aiChannel->mNumPositionKeys };
				NodeAnimation.vPositionKeys.resize(PositionKeyCount);
				for (unsigned int iPositionKey = 0; iPositionKey < PositionKeyCount; ++iPositionKey)
				{
					const aiVectorKey& aiKey{ aiChannel->mPositionKeys[iPositionKey] };
					SModel::SAnimation::SNodeAnimation::SKey& Key{ NodeAnimation.vPositionKeys[iPositionKey] };
					Key.Time = static_cast<float>(aiKey.mTime);
					Key.Value = ConvertaiVector3DToXMVECTOR(aiKey.mValue, 0);
				}

				unsigned int RotationKeyCount{ aiChannel->mNumRotationKeys };
				NodeAnimation.vRotationKeys.resize(RotationKeyCount);
				for (unsigned int iRotationKey = 0; iRotationKey < RotationKeyCount; ++iRotationKey)
				{
					const aiQuatKey& aiKey{ aiChannel->mRotationKeys[iRotationKey] };
					SModel::SAnimation::SNodeAnimation::SKey& Key{ NodeAnimation.vRotationKeys[iRotationKey] };
					Key.Time = static_cast<float>(aiKey.mTime);
					Key.Value = ConvertaiQuaternionToXMVECTOR(aiKey.mValue);
				}

				unsigned int ScalingKeyCount{ aiChannel->mNumScalingKeys };
				NodeAnimation.vScalingKeys.resize(ScalingKeyCount);
				for (unsigned int iScalingKey = 0; iScalingKey < ScalingKeyCount; ++iScalingKey)
				{
					const aiVectorKey& aiKey{ aiChannel->mScalingKeys[iScalingKey] };
					SModel::SAnimation::SNodeAnimation::SKey& Key{ NodeAnimation.vScalingKeys[iScalingKey] };
					Key.Time = static_cast<float>(aiKey.mTime);
					Key.Value = ConvertaiVector3DToXMVECTOR(aiKey.mValue, 0);
				}
			}
		}
	}
}