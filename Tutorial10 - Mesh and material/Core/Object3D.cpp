#include "Object3D.h"
#include "GameWindow.h"

void CObject3D::Create(const SMesh& Mesh, const SMaterial& Material)
{
	vector<SMesh> vMeshes{ Mesh };
	vector<SMaterial> vMaterials{ Material };

	Create(vMeshes, vMaterials);
}

void CObject3D::Create(const vector<SMesh>& vMeshes, const vector<SMaterial>& vMaterials)
{
	m_vMeshes = vMeshes;
	m_vMaterials = vMaterials;

	m_vMeshBuffers.resize(m_vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_vMeshes.size(); ++iMesh)
	{
		CreateMeshBuffers(iMesh);
	}
}

void CObject3D::CreateMeshBuffers(size_t MeshIndex)
{
	const SMesh& Mesh{ m_vMeshes[MeshIndex] };

	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex3D) * Mesh.vVertices.size());
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vVertices[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].VertexBuffer);
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

void CObject3D::Draw() const
{
	for (size_t iMesh = 0; iMesh < m_vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_vMeshes[iMesh] };

		m_PtrGameWindow->UpdateCBPSBaseMaterial(m_vMaterials[Mesh.MaterialID]);

		m_PtrDeviceContext->IASetIndexBuffer(m_vMeshBuffers[iMesh].IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_vMeshBuffers[iMesh].VertexBuffer.GetAddressOf(), 
			&m_vMeshBuffers[iMesh].VertexBufferStride, &m_vMeshBuffers[iMesh].VertexBufferOffset);
		m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Mesh.vTriangles.size() * 3), 0, 0);
	}
}

void CObject3D::DrawNormals() const
{
	for (size_t iMesh = 0; iMesh < m_vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_vMeshes[iMesh] };

		m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_vMeshBuffers[iMesh].VertexBuffer.GetAddressOf(), 
			&m_vMeshBuffers[iMesh].VertexBufferStride, &m_vMeshBuffers[iMesh].VertexBufferOffset);
		m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

		m_PtrDeviceContext->Draw(static_cast<UINT>(Mesh.vVertices.size() * 2), 0);
	}
}