#include "Object3D.h"

void CObject3D::Create(const SObject3DData& Object3DData)
{
	m_Object3DData = Object3DData;
	
	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex3D) * m_Object3DData.vVertices.size());
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &m_Object3DData.vVertices[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_VertexBuffer);
	}
	
	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(STriangle) * m_Object3DData.vTriangles.size());
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &m_Object3DData.vTriangles[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_IndexBuffer);
	}
}

void CObject3D::Draw()
{
	m_PtrDeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(m_Object3DData.vTriangles.size() * 3), 0, 0);
}