#include "Object3D.h"
#include "Game.h"

CAssimpLoader CObject3D::ms_AssimpLoader{};

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

void CObject3D::CreateFromFile(const string& FileName)
{
	if (ms_AssimpLoader.IsAnimatedModel(FileName))
	{
		ms_AssimpLoader.LoadAnimatedModelFromFile(FileName, m_Model, m_PtrDevice, m_PtrDeviceContext);

		ComponentRender.PtrVS = m_PtrGame->GetBaseShader(EBaseShader::VSAnimation);
	}
	else
	{
		ms_AssimpLoader.LoadStaticModelFromFile(FileName, m_Model, m_PtrDevice, m_PtrDeviceContext);
	}
	
	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::AddMaterial(const CMaterial& Material)
{
	m_Model.vMaterials.emplace_back(Material);

	CreateMaterialTextures();
}

void CObject3D::SetMaterial(size_t Index, const CMaterial& Material)
{
	assert(Index < m_Model.vMaterials.size());

	m_Model.vMaterials[Index] = Material;

	CreateMaterialTextures();
}

size_t CObject3D::GetMaterialCount() const
{
	return m_Model.vMaterials.size();
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

void CObject3D::CreateMaterialTextures()
{
	m_vDiffuseTextures.clear();
	m_vNormalTextures.clear();
	m_vDisplacementTextures.clear();

	for (CMaterial& Material : m_Model.vMaterials)
	{
		m_vDiffuseTextures.emplace_back();
		m_vNormalTextures.emplace_back();
		m_vDisplacementTextures.emplace_back();

		if (Material.HasTexture())
		{
			if (Material.HasDiffuseTexture())
			{
				m_vDiffuseTextures.back() = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

				if (Material.IsDiffuseTextureEmbedded())
				{
					m_vDiffuseTextures.back()->CreateTextureFromMemory(Material.GetDiffuseTextureRawData());
					Material.ClearEmbeddedDiffuseTextureData();
				}
				else
				{
					m_vDiffuseTextures.back()->CreateTextureFromFile(Material.GetDiffuseTextureFileName(), Material.ShouldGenerateAutoMipMap());
				}

				m_vDiffuseTextures.back()->SetSlot(static_cast<UINT>(KDiffuseTextureSlotOffset + m_vDiffuseTextures.size() - 1));
			}

			if (Material.HasNormalTexture())
			{
				m_vNormalTextures.back() = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

				if (Material.IsDiffuseTextureEmbedded())
				{
					m_vNormalTextures.back()->CreateTextureFromMemory(Material.GetNormalTextureRawData());
					Material.ClearEmbeddedNormalTextureData();
				}
				else
				{
					m_vNormalTextures.back()->CreateTextureFromFile(Material.GetNormalTextureFileName(), Material.ShouldGenerateAutoMipMap());
				}

				m_vNormalTextures.back()->SetSlot(static_cast<UINT>(KNormalTextureSlotOffset + m_vNormalTextures.size() - 1));
			}

			if (Material.HasDisplacementTexture())
			{
				m_vDisplacementTextures.back() = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

				if (Material.IsDiffuseTextureEmbedded())
				{
					m_vDisplacementTextures.back()->CreateTextureFromMemory(Material.GetDisplacementTextureRawData());
					Material.ClearEmbeddedDisplacementTextureData();
				}
				else
				{
					m_vDisplacementTextures.back()->CreateTextureFromFile(Material.GetDisplacementTextureFileName(), Material.ShouldGenerateAutoMipMap());
				}

				m_vDisplacementTextures.back()->SetShaderType(EShaderType::DomainShader); // @important
				m_vDisplacementTextures.back()->SetSlot(KDisplacementTextureSlotOffset);
			}
		}
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

void CObject3D::UpdateWorldMatrix()
{
	if (ComponentTransform.Pitch > CGame::KRotationMaxLimit) ComponentTransform.Pitch = CGame::KRotationMinLimit;
	if (ComponentTransform.Pitch < CGame::KRotationMinLimit) ComponentTransform.Pitch = CGame::KRotationMaxLimit;

	if (ComponentTransform.Yaw > CGame::KRotationMaxLimit) ComponentTransform.Yaw = CGame::KRotationMinLimit;
	if (ComponentTransform.Yaw < CGame::KRotationMinLimit) ComponentTransform.Yaw = CGame::KRotationMaxLimit;

	if (ComponentTransform.Roll > CGame::KRotationMaxLimit) ComponentTransform.Roll = CGame::KRotationMinLimit;
	if (ComponentTransform.Roll < CGame::KRotationMinLimit) ComponentTransform.Roll = CGame::KRotationMaxLimit;

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

	ComponentTransform.MatrixWorld = Scaling * Rotation * Translation;
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
		
		if (Material.HasTexture() && !bIgnoreOwnTexture)
		{
			if (m_Model.bUseMultipleTexturesInSingleMesh)
			{
				for (const auto& DiffuseTexture : m_vDiffuseTextures)
				{
					if (DiffuseTexture) DiffuseTexture->Use();
				}
				for (const auto& NormalTexture : m_vNormalTextures)
				{
					if (NormalTexture) NormalTexture->Use();
				}
			}
			else
			{
				if (m_vDiffuseTextures[Mesh.MaterialID]) m_vDiffuseTextures[Mesh.MaterialID]->Use();
				if (m_vNormalTextures[Mesh.MaterialID]) m_vNormalTextures[Mesh.MaterialID]->Use();
			}

			if (m_vDisplacementTextures.size())
			{
				if (m_vDisplacementTextures.front())
				{
					m_PtrGame->UpdateDSDisplacementData(true);
					m_vDisplacementTextures.front()->Use();
				}
				else
				{
					m_PtrGame->UpdateDSDisplacementData(false);
				}
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

		if (m_bShouldTesselate)
		{
			m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		}
		else
		{
			m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
		

		m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Mesh.vTriangles.size() * 3), 0, 0);
	}
}