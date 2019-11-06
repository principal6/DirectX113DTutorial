#include "AssimpLoader.h"

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
	LoadMaterialsFromFile(m_Scene, Device, DeviceContext, Model.vMaterials);
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
		Mesh.vVerticesAnimation.resize(Mesh.vVertices.size());
	}

	LoadMaterialsFromFile(m_Scene, Device, DeviceContext, Model.vMaterials);

	// Scene���� ��������� Node�� Tree�� �����.
	LoadNodes(m_Scene, m_Scene->mRootNode, -1, Model);

	// Node�� Name�� ���� Index�� ã�� �� �ֵ��� ���(map)�Ѵ�.
	for (auto& Node : Model.vNodes)
	{
		Model.mapNodeNameToIndex[Node.Name] = Node.Index;
	}

	// Scene���� �� Mesh�� ����� Bone���� �ҷ��´�.
	LoadBones(m_Scene, Model);

	// �� Mesh�� Vertex�� �� Bone�� BlendWeights�� �����Ѵ�.
	MatchWeightsAndVertices(Model);

	// Scene���� Animation�� �ҷ��´�.
	LoadAnimations(m_Scene, Model);

	// Animation�� Name�� ���� Index�� ã�� �� �ֵ��� ���(map)�Ѵ�. 
	for (auto& Animation : Model.vAnimations)
	{
		for (auto& NodeAnimation : Animation.vNodeAnimations)
		{
			Animation.mapNodeAnimationNameToIndex[NodeAnimation.NodeName] = NodeAnimation.Index;
		}
	}

	Model.bIsModelAnimated = true;
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

void CAssimpLoader::LoadMaterialsFromFile(const aiScene* const Scene, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, vector<CMaterial>& vMaterials)
{
	unsigned int MaterialCount{ Scene->mNumMaterials };
	vMaterials.resize(MaterialCount);
	for (unsigned int iMaterial{}; iMaterial < MaterialCount; ++iMaterial)
	{
		const aiMaterial* const _aiMaterial{ Scene->mMaterials[iMaterial] };
		CMaterial& Material{ vMaterials[iMaterial] };

		aiColor4D aiAmbient{}, aiDiffuse{}, aiSpecular{};
		float aiShininess{};
		aiGetMaterialColor(_aiMaterial, AI_MATKEY_COLOR_AMBIENT, &aiAmbient);
		aiGetMaterialColor(_aiMaterial, AI_MATKEY_COLOR_DIFFUSE, &aiDiffuse);
		aiGetMaterialColor(_aiMaterial, AI_MATKEY_COLOR_SPECULAR, &aiSpecular);
		aiGetMaterialFloat(_aiMaterial, AI_MATKEY_SHININESS, &aiShininess);
		
		if (Scene->HasTextures())
		{
			aiString DiffuseTextureFileName{};
			aiString NormalTextureFileName{};
			aiString DisplacementTextureFileName{};
			aiString OpacityTextureFileName{};

			aiGetMaterialTexture(_aiMaterial, aiTextureType_DIFFUSE, 0, &DiffuseTextureFileName);
			aiGetMaterialTexture(_aiMaterial, aiTextureType_NORMALS, 0, &NormalTextureFileName);
			aiGetMaterialTexture(_aiMaterial, aiTextureType_DISPLACEMENT, 0, &DisplacementTextureFileName);
			aiGetMaterialTexture(_aiMaterial, aiTextureType_OPACITY, 0, &OpacityTextureFileName);

			LoadTextureData(Scene, DiffuseTextureFileName, Material, CMaterial::CTexture::EType::DiffuseTexture);
			LoadTextureData(Scene, NormalTextureFileName, Material, CMaterial::CTexture::EType::NormalTexture);
			LoadTextureData(Scene, DisplacementTextureFileName, Material, CMaterial::CTexture::EType::DisplacementTexture);
			LoadTextureData(Scene, OpacityTextureFileName, Material, CMaterial::CTexture::EType::OpacityTexture);
		}

		if (aiAmbient.r == 0.0f && aiAmbient.g == 0.0f && aiAmbient.b == 0.0f)
		{
			aiAmbient = aiDiffuse;
		}

		Material.SetAmbientColor(XMFLOAT3(aiAmbient.r, aiAmbient.g, aiAmbient.b));
		Material.SetDiffuseColor(XMFLOAT3(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b));
		Material.SetSpecularColor(XMFLOAT3(aiSpecular.r, aiSpecular.g, aiSpecular.b));
		Material.SetSpecularExponent(aiShininess);
		Material.SetSpecularIntensity(1.0f);

		// @warning
		Material.ShouldGenerateAutoMipMap(true);
	}
}

void CAssimpLoader::LoadTextureData(const aiScene* const Scene, const aiString& TextureFileName, CMaterial& Material, CMaterial::CTexture::EType eTextureType)
{
	if (TextureFileName.length == 0) return;

	const aiTexture* const _aiTexture{ Scene->GetEmbeddedTexture(TextureFileName.C_Str()) };
	if (_aiTexture)
	{
		unsigned int TexelCount{ _aiTexture->mWidth / 4 };
		if (_aiTexture->mHeight) TexelCount *= _aiTexture->mHeight;

		vector<uint8_t> vEmbeddedTextureRawData{};
		vEmbeddedTextureRawData.reserve(_aiTexture->mWidth);
		for (unsigned int iTexel = 0; iTexel < TexelCount; ++iTexel)
		{
			const aiTexel& _aiTexel{ _aiTexture->pcData[iTexel] };
			vEmbeddedTextureRawData.emplace_back((uint8_t)_aiTexel.b);
			vEmbeddedTextureRawData.emplace_back((uint8_t)_aiTexel.g);
			vEmbeddedTextureRawData.emplace_back((uint8_t)_aiTexel.r);
			vEmbeddedTextureRawData.emplace_back((uint8_t)_aiTexel.a);
		}

		Material.SetTextureRawData(eTextureType, vEmbeddedTextureRawData);
	}
	else
	{
		Material.SetTextureFileName(eTextureType, TextureFileName.C_Str());
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
		// �θ� Node�� ���� Node�� Child�� ����Ѵ�.
		Model.vNodes[Node.ParentNodeIndex].vChildNodeIndices.emplace_back(NodeIndex);
	}

	if (aiCurrentNode->mNumChildren)
	{
		// Child Node�� �ִ� ��� �� Child�� �ҷ��´�.
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

				// BoneCount�� ���� Bone�� ó�� ���� ��쿡�� �������Ѿ� �Ѵ�! (������ Bone�� ���� �ٸ� Mesh���� ������ �� �ֱ� ����)
				if (mapBones[BoneName] == 0)
				{
					BoneNode.BoneIndex = TotalBoneCount;
					++TotalBoneCount;

					BoneNode.bIsBone = true;
					BoneNode.MatrixBoneOffset = ConvertaiMatrix4x4ToXMMATRIX(_aiBone->mOffsetMatrix);
				}

				// ���� Mesh�� ���� Bone�� ������ �� �����Ƿ� �Ʒ� �ڵ�� �Ź� ����Ǿ�� �Ѵ�.
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
		Model.vAnimations.resize(Scene->mNumAnimations);
		for (unsigned int iAnimation = 0; iAnimation < Scene->mNumAnimations; ++iAnimation)
		{
			const aiAnimation* const _aiAnimation{ Scene->mAnimations[iAnimation] };
			const unsigned int& ChannelCount{ _aiAnimation->mNumChannels };

			SModel::SAnimation& Animation{ Model.vAnimations[iAnimation] };
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