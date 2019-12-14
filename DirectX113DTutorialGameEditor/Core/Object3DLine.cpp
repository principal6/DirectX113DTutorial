#include "Object3DLine.h"

using std::vector;
using std::string;

void CObject3DLine::Create(const vector<SVertex3DLine>& vVertices)
{
	m_vVertices = vVertices;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex3DLine) * m_vVertices.size());
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vVertices[0];

	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_VertexBuffer.GetAddressOf());
}

void CObject3DLine::Create(size_t LineCount, const XMVECTOR& UniformColor)
{
	m_vVertices.resize(LineCount * 2);

	for (auto& Vertex : m_vVertices)
	{
		Vertex.Color = UniformColor;
	}

	Create(m_vVertices);
}

void CObject3DLine::UpdateLinePosition(size_t LineIndex, const XMVECTOR& PositionA, const XMVECTOR& PositionB)
{
	if (LineIndex >= m_vVertices.size() * 2) return;

	size_t iVertexA{ LineIndex * 2 };
	size_t iVertexB{ iVertexA + 1 };

	m_vVertices[iVertexA].Position = PositionA;
	m_vVertices[iVertexB].Position = PositionB;
}

void CObject3DLine::UpdateLineColor(size_t LineIndex, const XMVECTOR& ColorA, const XMVECTOR& ColorB)
{
	if (LineIndex >= m_vVertices.size() * 2) return;

	size_t iVertexA{ LineIndex * 2 };
	size_t iVertexB{ iVertexA + 1 };

	m_vVertices[iVertexA].Color = ColorA;
	m_vVertices[iVertexB].Color = ColorB;
}

void CObject3DLine::UpdateVertexBuffer()
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vVertices[0], sizeof(SVertex3DLine) * m_vVertices.size());

		m_PtrDeviceContext->Unmap(m_VertexBuffer.Get(), 0);
	}
}

void CObject3DLine::UpdateWorldMatrix()
{
	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationQuaternion(ComponentTransform.RotationQuaternion) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	ComponentTransform.MatrixWorld = Scaling * Rotation * Translation;
}

void CObject3DLine::Draw() const
{
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	m_PtrDeviceContext->Draw(static_cast<UINT>(m_vVertices.size()), 0);
}
