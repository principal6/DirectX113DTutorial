#include "Object3D.h"
#include "Game.h"

void CObject3D::Create(const SMesh& Mesh)
{
	m_Model.vMeshes.clear();
	m_Model.vMeshes.emplace_back(Mesh);

	m_Model.vMaterials.clear();
	m_Model.vMaterials.emplace_back();

	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const SMesh& Mesh, const CMaterial& Material)
{
	m_Model.vMeshes.clear();
	m_Model.vMeshes.emplace_back(Mesh);

	m_Model.vMaterials.clear();
	m_Model.vMaterials.emplace_back(Material);
	
	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const vector<SMesh>& vMeshes, const vector<CMaterial>& vMaterials)
{
	m_Model.vMeshes = vMeshes;
	m_Model.vMaterials = vMaterials;
	
	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const SModel& Model)
{
	m_Model = Model;

	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::CreateFromFile(const string& FileName, bool bIsModelRigged)
{
	m_ModelFileName = FileName;

	ULONGLONG StartTimePoint{ GetTickCount64() };
	if (bIsModelRigged)
	{
		m_AssimpLoader.LoadAnimatedModelFromFile(FileName, m_Model, m_PtrDevice, m_PtrDeviceContext);

		ComponentRender.PtrVS = m_PtrGame->GetBaseShader(EBaseShader::VSAnimation);
	}
	else
	{
		m_AssimpLoader.LoadStaticModelFromFile(FileName, m_Model, m_PtrDevice, m_PtrDeviceContext);
	}
	OutputDebugString(("- Model [" + FileName + "] loaded. [" + to_string(GetTickCount64() - StartTimePoint) + "] elapsed.\n").c_str());

	CreateMeshBuffers();
	CreateMaterialTextures();

	for (const CMaterial& Material : m_Model.vMaterials)
	{
		// @important
		if (Material.HasTexture(CMaterial::CTexture::EType::OpacityTexture))
		{
			ComponentRender.bIsTransparent = true;
			break;
		}
	}

	m_bIsCreated = true;
}

void CObject3D::AddMaterial(const CMaterial& Material)
{
	m_Model.vMaterials.emplace_back(Material);
	m_Model.vMaterials.back().SetIndex(m_Model.vMaterials.size() - 1);

	CreateMaterialTextures();
}

void CObject3D::SetMaterial(size_t Index, const CMaterial& Material)
{
	assert(Index < m_Model.vMaterials.size());

	m_Model.vMaterials[Index] = Material;
	m_Model.vMaterials[Index].SetIndex(Index);

	CreateMaterialTextures();
}

size_t CObject3D::GetMaterialCount() const
{
	return m_Model.vMaterials.size();
}

void CObject3D::CreateInstances(int InstanceCount)
{
	m_vInstanceCPUData.clear();
	m_vInstanceCPUData.resize(InstanceCount);
	for (auto& InstanceCPUData : m_vInstanceCPUData)
	{
		InstanceCPUData.Scaling = ComponentTransform.Scaling;
	}

	m_vInstanceGPUData.clear();
	m_vInstanceGPUData.resize(InstanceCount);

	m_mapInstanceNameToIndex.clear();
	for (int iInstance = 0; iInstance < InstanceCount; ++iInstance)
	{
		string Name{ "instance" + to_string(iInstance) };
		m_mapInstanceNameToIndex[Name] = iInstance;
	}

	ComponentRender.PtrVS = m_PtrGame->GetBaseShader(EBaseShader::VSInstance);

	CreateInstanceBuffers();

	UpdateAllInstancesWorldMatrix();
}

void CObject3D::InsertInstance(bool bShouldCreateInstanceBuffers)
{
	int InstanceCount{ GetInstanceCount() };
	string Name{ "instance" + to_string(InstanceCount) };

	if (m_mapInstanceNameToIndex.find(Name) != m_mapInstanceNameToIndex.end())
	{
		for (int iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			Name = "instance" + to_string(iInstance);
			if (m_mapInstanceNameToIndex.find(Name) == m_mapInstanceNameToIndex.end()) break;
		}
	}

	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	m_vInstanceCPUData.emplace_back();
	m_vInstanceCPUData.back().Name = Name;
	m_vInstanceCPUData.back().Scaling = ComponentTransform.Scaling;
	m_mapInstanceNameToIndex[Name] = m_vInstanceCPUData.size() - 1;

	m_vInstanceGPUData.emplace_back();
	
	if (bShouldCreateInstanceBuffers)
	{
		if (m_vInstanceCPUData.size() == 1) CreateInstanceBuffers();
		if (bShouldRecreateInstanceBuffer) CreateInstanceBuffers();
	}

	ComponentRender.PtrVS = m_PtrGame->GetBaseShader(EBaseShader::VSInstance);

	UpdateInstanceWorldMatrix(static_cast<int>(m_vInstanceCPUData.size() - 1));
}

void CObject3D::InsertInstance(const string& Name)
{
	int InstanceCount{ GetInstanceCount() };
	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	m_vInstanceCPUData.emplace_back();
	m_vInstanceCPUData.back().Name = Name;
	m_vInstanceCPUData.back().Scaling = ComponentTransform.Scaling;
	m_mapInstanceNameToIndex[Name] = m_vInstanceCPUData.size() - 1;

	m_vInstanceGPUData.emplace_back();

	if (m_vInstanceCPUData.size() == 1) CreateInstanceBuffers();
	if (bShouldRecreateInstanceBuffer) CreateInstanceBuffers();

	ComponentRender.PtrVS = m_PtrGame->GetBaseShader(EBaseShader::VSInstance);

	UpdateInstanceWorldMatrix(static_cast<int>(m_vInstanceCPUData.size() - 1));
}

void CObject3D::DeleteInstance(const string& Name)
{
	if (Name.empty()) return;
	if (m_vInstanceCPUData.empty()) return;
	if (m_mapInstanceNameToIndex.find(Name) == m_mapInstanceNameToIndex.end()) return;

	int iInstance{ (int)m_mapInstanceNameToIndex.at(Name) };
	int iLastInstance{ GetInstanceCount() - 1 };
	string InstanceName{ Name };
	if (iInstance == iLastInstance)
	{
		// Last instance
		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(InstanceName);

		// @important
		--iInstance;

		if (m_vInstanceCPUData.empty())
		{
			ComponentRender.PtrVS = m_PtrGame->GetBaseShader(EBaseShader::VSBase);

			// @important
			m_PtrGame->DeselectInstance();
			return;
		}
	}
	else
	{
		// Non-last instance
		string LastInstanceName{ m_vInstanceCPUData[iLastInstance].Name };

		std::swap(m_vInstanceCPUData[iInstance], m_vInstanceCPUData[iLastInstance]);
		std::swap(m_vInstanceGPUData[iInstance], m_vInstanceGPUData[iLastInstance]);

		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(InstanceName);
		m_mapInstanceNameToIndex[LastInstanceName] = iInstance;

		UpdateInstanceBuffers();
	}

	// @important
	m_PtrGame->SelectInstance((int)iInstance);	
}

CObject3D::SInstanceCPUData& CObject3D::GetInstance(int InstanceID)
{
	assert(InstanceID < m_vInstanceCPUData.size());
	return m_vInstanceCPUData[InstanceID];
}

CObject3D::SInstanceCPUData& CObject3D::GetInstance(const string& Name)
{
	assert(m_mapInstanceNameToIndex.find(Name) != m_mapInstanceNameToIndex.end());
	return m_vInstanceCPUData[m_mapInstanceNameToIndex.at(Name)];
}

void CObject3D::CreateMeshBuffers()
{
	m_vMeshBuffers.clear();
	m_vMeshBuffers.resize(m_Model.vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		CreateMeshBuffer(iMesh, m_Model.bIsModelAnimated);
	}
}

void CObject3D::CreateMeshBuffer(size_t MeshIndex, bool IsAnimated)
{
	const SMesh& Mesh{ m_Model.vMeshes[MeshIndex] };

	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex3D) * Mesh.vVertices.size());
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vVertices[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].VertexBuffer);
	}

	if (IsAnimated)
	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertexAnimation) * Mesh.vVerticesAnimation.size());
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vVerticesAnimation[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].VertexBufferAnimation);
	}

	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(STriangle) * Mesh.vTriangles.size());
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vTriangles[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].IndexBuffer);
	}
}

void CObject3D::CreateInstanceBuffers()
{
	if (m_vInstanceCPUData.empty()) return;

	m_vInstanceBuffers.clear();
	m_vInstanceBuffers.resize(m_Model.vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		CreateInstanceBuffer(iMesh);
	}
}

void CObject3D::CreateInstanceBuffer(size_t MeshIndex)
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SInstanceGPUData) * m_vInstanceGPUData.capacity()); // @important
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vInstanceGPUData[0];
	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vInstanceBuffers[MeshIndex].Buffer);
}

void CObject3D::CreateMaterialTextures()
{
	for (CMaterial& Material : m_Model.vMaterials)
	{
		Material.CreateTextures(m_PtrDevice, m_PtrDeviceContext);
	}
}

void CObject3D::UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize)
{
	float U0{ UVOffset.x };
	float V0{ UVOffset.y };
	float U1{ U0 + UVSize.x };
	float V1{ V0 + UVSize.y };

	m_Model.vMeshes[0].vVertices[0].TexCoord = XMVectorSet(U0, V0, 0, 0);
	m_Model.vMeshes[0].vVertices[1].TexCoord = XMVectorSet(U1, V0, 0, 0);
	m_Model.vMeshes[0].vVertices[2].TexCoord = XMVectorSet(U0, V1, 0, 0);
	m_Model.vMeshes[0].vVertices[3].TexCoord = XMVectorSet(U1, V1, 0, 0);

	UpdateMeshBuffer();
}

void CObject3D::UpdateMeshBuffer(size_t MeshIndex)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vMeshBuffers[MeshIndex].VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_Model.vMeshes[MeshIndex].vVertices[0], sizeof(SVertex3D) * m_Model.vMeshes[MeshIndex].vVertices.size());

		m_PtrDeviceContext->Unmap(m_vMeshBuffers[MeshIndex].VertexBuffer.Get(), 0);
	}
}

void CObject3D::UpdateInstanceBuffers()
{
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		UpdateInstanceBuffer(iMesh);
	}
}

void CObject3D::UpdateInstanceBuffer(size_t MeshIndex)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vInstanceGPUData[0], sizeof(SInstanceGPUData) * m_vInstanceGPUData.size());

		m_PtrDeviceContext->Unmap(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0);
	}
}

void CObject3D::LimitFloatRotation(float& Value, const float Min, const float Max)
{
	if (Value > Max) Value = Min;
	if (Value < Min) Value = Max;
}

void CObject3D::UpdateWorldMatrix()
{
	LimitFloatRotation(ComponentTransform.Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(ComponentTransform.Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(ComponentTransform.Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

	if (XMVectorGetX(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetX(ComponentTransform.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetY(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetY(ComponentTransform.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetZ(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetZ(ComponentTransform.Scaling, CGame::KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(ComponentTransform.Pitch,
		ComponentTransform.Yaw, ComponentTransform.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };

	ComponentTransform.MatrixWorld = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;
}

void CObject3D::UpdateInstanceWorldMatrix(int InstanceID)
{
	if (InstanceID >= m_vInstanceCPUData.size()) return;

	// Update CPU data
	LimitFloatRotation(m_vInstanceCPUData[InstanceID].Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(m_vInstanceCPUData[InstanceID].Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(m_vInstanceCPUData[InstanceID].Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

	if (XMVectorGetX(m_vInstanceCPUData[InstanceID].Scaling) < CGame::KScalingMinLimit)
		m_vInstanceCPUData[InstanceID].Scaling = XMVectorSetX(m_vInstanceCPUData[InstanceID].Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetY(m_vInstanceCPUData[InstanceID].Scaling) < CGame::KScalingMinLimit)
		m_vInstanceCPUData[InstanceID].Scaling = XMVectorSetY(m_vInstanceCPUData[InstanceID].Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetZ(m_vInstanceCPUData[InstanceID].Scaling) < CGame::KScalingMinLimit)
		m_vInstanceCPUData[InstanceID].Scaling = XMVectorSetZ(m_vInstanceCPUData[InstanceID].Scaling, CGame::KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(m_vInstanceCPUData[InstanceID].Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(m_vInstanceCPUData[InstanceID].Pitch,
		m_vInstanceCPUData[InstanceID].Yaw, m_vInstanceCPUData[InstanceID].Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(m_vInstanceCPUData[InstanceID].Scaling) };

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };
	
	// Update GPU data
	m_vInstanceGPUData[InstanceID].InstanceWorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;

	UpdateInstanceBuffers();
}

void CObject3D::UpdateAllInstancesWorldMatrix()
{
	// Update CPU data
	for (int iInstance = 0; iInstance < static_cast<int>(m_vInstanceCPUData.size()); ++iInstance)
	{
		LimitFloatRotation(m_vInstanceCPUData[iInstance].Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
		LimitFloatRotation(m_vInstanceCPUData[iInstance].Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
		LimitFloatRotation(m_vInstanceCPUData[iInstance].Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

		if (XMVectorGetX(m_vInstanceCPUData[iInstance].Scaling) < CGame::KScalingMinLimit)
			m_vInstanceCPUData[iInstance].Scaling = XMVectorSetX(m_vInstanceCPUData[iInstance].Scaling, CGame::KScalingMinLimit);
		if (XMVectorGetY(m_vInstanceCPUData[iInstance].Scaling) < CGame::KScalingMinLimit)
			m_vInstanceCPUData[iInstance].Scaling = XMVectorSetY(m_vInstanceCPUData[iInstance].Scaling, CGame::KScalingMinLimit);
		if (XMVectorGetZ(m_vInstanceCPUData[iInstance].Scaling) < CGame::KScalingMinLimit)
			m_vInstanceCPUData[iInstance].Scaling = XMVectorSetZ(m_vInstanceCPUData[iInstance].Scaling, CGame::KScalingMinLimit);

		XMMATRIX Translation{ XMMatrixTranslationFromVector(m_vInstanceCPUData[iInstance].Translation) };
		XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(m_vInstanceCPUData[iInstance].Pitch,
			m_vInstanceCPUData[iInstance].Yaw, m_vInstanceCPUData[iInstance].Roll) };
		XMMATRIX Scaling{ XMMatrixScalingFromVector(m_vInstanceCPUData[iInstance].Scaling) };

		XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
		XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };

		// Update GPU data
		m_vInstanceGPUData[iInstance].InstanceWorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;
	}
	
	UpdateInstanceBuffers();
}

void CObject3D::Animate()
{
	if (!m_Model.vAnimations.size()) return;

	SModel::SAnimation& CurrentAnimation{ m_Model.vAnimations[m_CurrentAnimationIndex] };
	
	m_CurrentAnimationTick += CurrentAnimation.TicksPerSecond * 0.001f; // TEMPORARY SLOW DOWN!!
	if (m_CurrentAnimationTick >= CurrentAnimation.Duration)
	{
		m_CurrentAnimationTick = 0.0f;
	}

	CalculateAnimatedBoneMatrices(m_Model.vNodes[0], XMMatrixIdentity());

	m_PtrGame->UpdateVSAnimationBoneMatrices(m_AnimatedBoneMatrices);
}

void CObject3D::ShouldTessellate(bool Value)
{
	m_bShouldTesselate = Value;
}

void CObject3D::CalculateAnimatedBoneMatrices(const SModel::SNode& Node, XMMATRIX ParentTransform)
{
	XMMATRIX MatrixTransformation{ Node.MatrixTransformation * ParentTransform };

	if (Node.bIsBone)
	{
		const SModel::SAnimation& CurrentAnimation{ m_Model.vAnimations[m_CurrentAnimationIndex] };
		if (CurrentAnimation.vNodeAnimations.size())
		{
			if (CurrentAnimation.mapNodeAnimationNameToIndex.find(Node.Name) != CurrentAnimation.mapNodeAnimationNameToIndex.end())
			{
				size_t NodeAnimationIndex{ CurrentAnimation.mapNodeAnimationNameToIndex.at(Node.Name) };

				const SModel::SAnimation::SNodeAnimation& NodeAnimation{ CurrentAnimation.vNodeAnimations[NodeAnimationIndex] };

				XMMATRIX MatrixPosition{ XMMatrixIdentity() };
				XMMATRIX MatrixRotation{ XMMatrixIdentity() };
				XMMATRIX MatrixScaling{ XMMatrixIdentity() };

				{
					const vector<SModel::SAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vPositionKeys };
					SModel::SAnimation::SNodeAnimation::SKey KeyA{};
					SModel::SAnimation::SNodeAnimation::SKey KeyB{};
					for (uint32_t iKey = 0; iKey < (uint32_t)vKeys.size(); ++iKey)
					{
						if (vKeys[iKey].Time <= m_CurrentAnimationTick)
						{
							KeyA = vKeys[iKey];
							KeyB = vKeys[(iKey < (uint32_t)vKeys.size() - 1) ? iKey + 1 : 0];
						}
					}

					MatrixPosition = XMMatrixTranslationFromVector(KeyA.Value);
				}

				{
					const vector<SModel::SAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vRotationKeys };
					SModel::SAnimation::SNodeAnimation::SKey KeyA{};
					SModel::SAnimation::SNodeAnimation::SKey KeyB{};
					for (uint32_t iKey = 0; iKey < (uint32_t)vKeys.size(); ++iKey)
					{
						if (vKeys[iKey].Time <= m_CurrentAnimationTick)
						{
							KeyA = vKeys[iKey];
							KeyB = vKeys[(iKey < (uint32_t)vKeys.size() - 1) ? iKey + 1 : 0];
						}
					}

					MatrixRotation = XMMatrixRotationQuaternion(KeyA.Value);
				}

				{
					const vector<SModel::SAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vScalingKeys };
					SModel::SAnimation::SNodeAnimation::SKey KeyA{};
					SModel::SAnimation::SNodeAnimation::SKey KeyB{};
					for (uint32_t iKey = 0; iKey < (uint32_t)vKeys.size(); ++iKey)
					{
						if (vKeys[iKey].Time <= m_CurrentAnimationTick)
						{
							KeyA = vKeys[iKey];
							KeyB = vKeys[(iKey < (uint32_t)vKeys.size() - 1) ? iKey + 1 : 0];
						}
					}

					MatrixScaling = XMMatrixScalingFromVector(KeyA.Value);
				}

				MatrixTransformation = MatrixScaling * MatrixRotation * MatrixPosition * ParentTransform;
			}
		}

		// Transpose at the last moment!
		m_AnimatedBoneMatrices[Node.BoneIndex] = XMMatrixTranspose(Node.MatrixBoneOffset * MatrixTransformation);
	}
	
	if (Node.vChildNodeIndices.size())
	{
		for (auto iChild : Node.vChildNodeIndices)
		{
			CalculateAnimatedBoneMatrices(m_Model.vNodes[iChild], MatrixTransformation);
		}
	}
}

void CObject3D::Draw(bool bIgnoreOwnTexture) const
{
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_Model.vMeshes[iMesh] };
		const CMaterial& Material{ m_Model.vMaterials[Mesh.MaterialID] };

		m_PtrGame->UpdatePSBaseMaterial(Material);
		m_PtrGame->UpdateDSDisplacementData(false);

		if (m_Model.bUseMultipleTexturesInSingleMesh) // This bool is for CTerrain
		{
			for (const CMaterial& Material : m_Model.vMaterials)
			{
				if (Material.HasTexture() && !bIgnoreOwnTexture)
				{
					if (Material.HasTexture(CMaterial::CTexture::EType::DisplacementTexture))
					{
						m_PtrGame->UpdateDSDisplacementData(true);
					}
					Material.UseTextures();
				}
			}
		}
		else
		{
			m_PtrGame->UpdatePSBaseMaterial(Material);
			if (Material.HasTexture() && !bIgnoreOwnTexture)
			{
				if (Material.HasTexture(CMaterial::CTexture::EType::DisplacementTexture))
				{
					m_PtrGame->UpdateDSDisplacementData(true);
				}
				Material.UseTextures();
			}
		}

		m_PtrDeviceContext->IASetIndexBuffer(m_vMeshBuffers[iMesh].IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_vMeshBuffers[iMesh].VertexBuffer.GetAddressOf(), 
			&m_vMeshBuffers[iMesh].VertexBufferStride, &m_vMeshBuffers[iMesh].VertexBufferOffset);

		if (m_Model.bIsModelAnimated)
		{
			m_PtrDeviceContext->IASetVertexBuffers(1, 1, m_vMeshBuffers[iMesh].VertexBufferAnimation.GetAddressOf(),
				&m_vMeshBuffers[iMesh].VertexBufferAnimationStride, &m_vMeshBuffers[iMesh].VertexBufferAnimationOffset);
		}

		if (IsInstanced())
		{
			m_PtrDeviceContext->IASetVertexBuffers(2, 1, m_vInstanceBuffers[iMesh].Buffer.GetAddressOf(),
				&m_vInstanceBuffers[iMesh].Stride, &m_vInstanceBuffers[iMesh].Offset);
		}

		if (m_bShouldTesselate)
		{
			m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		}
		else
		{
			m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
		
		if (m_vInstanceCPUData.size())
		{
			m_PtrDeviceContext->DrawIndexedInstanced(static_cast<UINT>(Mesh.vTriangles.size() * 3), static_cast<UINT>(m_vInstanceCPUData.size()), 0, 0, 0);
		}
		else
		{
			m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Mesh.vTriangles.size() * 3), 0, 0);
		}
	}
}