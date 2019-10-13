#include "Object3D.h"
#include "Game.h"

void CObject3D::Create(const SMesh& Mesh, const SMaterial& Material)
{
	vector<SMesh> vMeshes{ Mesh };
	vector<SMaterial> vMaterials{ Material };

	Create(vMeshes, vMaterials);
}

void CObject3D::Create(const vector<SMesh>& vMeshes, const vector<SMaterial>& vMaterials)
{
	SModel Model{ vMeshes, vMaterials };
	
	Create(Model);
}

void CObject3D::Create(const SModel& Model)
{
	m_Model = Model;

	m_vMeshBuffers.resize(m_Model.vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		CreateMeshBuffers(iMesh, m_Model.bIsModelAnimated);
	}

	m_vEmbeddedTextures.reserve(m_Model.vMaterials.size());
	for (SMaterial& Material : m_Model.vMaterials)
	{
		if (Material.bHasTexture && Material.vEmbeddedTextureRawData.size())
		{
			m_vEmbeddedTextures.emplace_back(make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext));

			m_vEmbeddedTextures.back()->CreateWICFromMemory(Material.vEmbeddedTextureRawData);

			Material.vEmbeddedTextureRawData.clear();
		}
	}
}

void CObject3D::CreateMeshBuffers(size_t MeshIndex, bool IsAnimated)
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

void CObject3D::Animate()
{
	if (!m_Model.vAnimations.size()) return;

	SModelAnimation& CurrentAnimation{ m_Model.vAnimations[m_CurrentAnimationIndex] };
	
	m_CurrentAnimationTick += CurrentAnimation.TicksPerSecond * 0.001f; // TEMPORARY SLOW DOWN!!
	if (m_CurrentAnimationTick >= CurrentAnimation.Duration)
	{
		m_CurrentAnimationTick = 0.0f;
	}

	CalculateAnimatedBoneMatrices(m_Model.vNodes[0], XMMatrixIdentity());

	m_PtrGame->UpdateVSAnimationBoneMatrices(m_AnimatedBoneMatrices);
}

void CObject3D::CalculateAnimatedBoneMatrices(const SModelNode& Node, XMMATRIX ParentTransform)
{
	XMMATRIX MatrixTransformation{ Node.MatrixTransformation * ParentTransform };

	if (Node.bIsBone)
	{
		SModelAnimation& CurrentAnimation{ m_Model.vAnimations[m_CurrentAnimationIndex] };
		if (CurrentAnimation.vNodeAnimations.size())
		{
			if (CurrentAnimation.mapNodeAnimationNameToIndex.find(Node.Name) != CurrentAnimation.mapNodeAnimationNameToIndex.end())
			{
				size_t NodeAnimationIndex{ CurrentAnimation.mapNodeAnimationNameToIndex[Node.Name] };

				SModelNodeAnimation& NodeAnimation{ CurrentAnimation.vNodeAnimations[NodeAnimationIndex] };

				XMMATRIX MatrixPosition{ XMMatrixIdentity() };
				XMMATRIX MatrixRotation{ XMMatrixIdentity() };
				XMMATRIX MatrixScaling{ XMMatrixIdentity() };

				{
					vector<SModelNodeAnimation::SKey>& vKeys{ NodeAnimation.vPositionKeys };
					SModelNodeAnimation::SKey KeyA{};
					SModelNodeAnimation::SKey KeyB{};
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
					vector<SModelNodeAnimation::SKey>& vKeys{ NodeAnimation.vRotationKeys };
					SModelNodeAnimation::SKey KeyA{};
					SModelNodeAnimation::SKey KeyB{};
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
					vector<SModelNodeAnimation::SKey>& vKeys{ NodeAnimation.vScalingKeys };
					SModelNodeAnimation::SKey KeyA{};
					SModelNodeAnimation::SKey KeyB{};
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

void CObject3D::Draw() const
{
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_Model.vMeshes[iMesh] };
		const SMaterial& Material{ m_Model.vMaterials[Mesh.MaterialID] };

		m_PtrGame->UpdateVSBaseMaterial(Material);
		
		if (Material.bHasTexture && Material.bHasEmbeddedTexture)
		{
			m_PtrGame->UpdatePSBaseFlagOn(EFlagPSBase::UseTexture);

			m_vEmbeddedTextures[Mesh.MaterialID]->Use();
		}

		m_PtrDeviceContext->IASetIndexBuffer(m_vMeshBuffers[iMesh].IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_vMeshBuffers[iMesh].VertexBuffer.GetAddressOf(), 
			&m_vMeshBuffers[iMesh].VertexBufferStride, &m_vMeshBuffers[iMesh].VertexBufferOffset);

		if (m_Model.bIsModelAnimated)
		{
			m_PtrDeviceContext->IASetVertexBuffers(1, 1, m_vMeshBuffers[iMesh].VertexBufferAnimation.GetAddressOf(),
				&m_vMeshBuffers[iMesh].VertexBufferAnimationStride, &m_vMeshBuffers[iMesh].VertexBufferAnimationOffset);
		}

		m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Mesh.vTriangles.size() * 3), 0, 0);
	}
}

void CObject3D::DrawNormals() const
{
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_Model.vMeshes[iMesh] };

		m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_vMeshBuffers[iMesh].VertexBuffer.GetAddressOf(), 
			&m_vMeshBuffers[iMesh].VertexBufferStride, &m_vMeshBuffers[iMesh].VertexBufferOffset);

		if (m_Model.bIsModelAnimated)
		{
			m_PtrDeviceContext->IASetVertexBuffers(1, 1, m_vMeshBuffers[iMesh].VertexBufferAnimation.GetAddressOf(),
				&m_vMeshBuffers[iMesh].VertexBufferAnimationStride, &m_vMeshBuffers[iMesh].VertexBufferAnimationOffset);
		}

		m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		m_PtrDeviceContext->Draw(static_cast<UINT>(Mesh.vVertices.size() * 2), 0);
	}
}