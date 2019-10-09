#include "Object2D.h"

void CObject2D::CreateStatic(const SObject2DData& _Object2DData)
{
	assert(!m_VertexBuffer);
	assert(!m_IndexBuffer);

	Object2DData = _Object2DData;
	
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(SVertex2D) * Object2DData.vVertices.size());
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = &Object2DData.vVertices[0];

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&buffer_desc, &subresource_data, &m_VertexBuffer)));

	CreateIndexBuffer();
}

void CObject2D::CreateDynamic(const SObject2DData& _Object2DData)
{
	assert(!m_VertexBuffer);
	assert(!m_IndexBuffer);

	Object2DData = _Object2DData;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(SVertex2D) * Object2DData.vVertices.size());
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = &Object2DData.vVertices[0];

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&buffer_desc, &subresource_data, &m_VertexBuffer)));

	CreateIndexBuffer();
}

void CObject2D::CreateIndexBuffer()
{
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(STriangle) * Object2DData.vTriangles.size());
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = &Object2DData.vTriangles[0];

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&buffer_desc, &subresource_data, &m_IndexBuffer)));
}

void CObject2D::UpdateVertexBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource)))
	{
		memcpy(mapped_subresource.pData, &Object2DData.vVertices[0], sizeof(SVertex2D) * Object2DData.vVertices.size());
		m_PtrDeviceContext->Unmap(m_VertexBuffer.Get(), 0);
	}
}

void CObject2D::Draw()
{
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_PtrDeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Object2DData.vTriangles.size() * 3), 0, 0);
}