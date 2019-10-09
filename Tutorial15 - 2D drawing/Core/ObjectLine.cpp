#include "ObjectLine.h"

void CObjectLine::Create(const vector<SVertexLine>& _vVertices)
{
	vVertices = _vVertices;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertexLine) * vVertices.size());
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &vVertices[0];

	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_VertexBuffer.GetAddressOf());
}

void CObjectLine::Update()
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &vVertices[0], sizeof(SVertexLine) * vVertices.size());

		m_PtrDeviceContext->Unmap(m_VertexBuffer.Get(), 0);
	}
}

void CObjectLine::Draw() const
{
	//m_PtrDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	m_PtrDeviceContext->Draw(static_cast<UINT>(vVertices.size()), 0);
	
}
