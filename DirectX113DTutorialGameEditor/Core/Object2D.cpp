#include "Object2D.h"

using std::make_unique;

void CObject2D::Create(const SModel2D& Model2D, bool bIsDynamic)
{
	m_Model2D = Model2D;
	m_bIsDynamic = bIsDynamic;

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex) * m_Model2D.vVertices.size());
	BufferDesc.CPUAccessFlags = (m_bIsDynamic) ? D3D11_CPU_ACCESS_WRITE : 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = (m_bIsDynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_Model2D.vVertices[0];

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_VertexBuffer)));

	CreateIndexBuffer();
}

void CObject2D::CreateTexture(const std::string& FileName)
{
	m_Texture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_Texture->CreateTextureFromFile(FileName, false);
}

void CObject2D::CreateIndexBuffer()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(STriangle) * m_Model2D.vTriangles.size());
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_Model2D.vTriangles[0];

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_IndexBuffer)));
}

void CObject2D::Translate(const XMFLOAT2& DeltaPosition)
{
	m_ComponentTransform.Translation.x += DeltaPosition.x;
	m_ComponentTransform.Translation.y += DeltaPosition.y;
	m_ComponentTransform.MatrixTranslation = XMMatrixTranslation(m_ComponentTransform.Translation.x, m_ComponentTransform.Translation.y, 0);

	UpdateWorldMatrix();
}

void CObject2D::Rotate(float DeltaAngle)
{
	m_ComponentTransform.RotationAngle += DeltaAngle;
	if (m_ComponentTransform.RotationAngle > XM_2PI) m_ComponentTransform.RotationAngle = 0.0f;
	if (m_ComponentTransform.RotationAngle < -XM_2PI) m_ComponentTransform.RotationAngle = 0.0f;
	m_ComponentTransform.MatrixRotation = XMMatrixRotationZ(m_ComponentTransform.RotationAngle);

	UpdateWorldMatrix();
}

void CObject2D::Scale(const XMFLOAT2& DeltaScalar)
{
	m_ComponentTransform.Scaling.x += DeltaScalar.x;
	m_ComponentTransform.Scaling.y += DeltaScalar.y;
	if (m_ComponentTransform.Scaling.x < 0.0f) m_ComponentTransform.Scaling.x = 0.0f;
	if (m_ComponentTransform.Scaling.y < 0.0f) m_ComponentTransform.Scaling.y = 0.0f;
	m_ComponentTransform.MatrixScaling = XMMatrixScaling(m_ComponentTransform.Scaling.x, m_ComponentTransform.Scaling.y, 1);

	UpdateWorldMatrix();
}

void CObject2D::TranslateTo(const XMFLOAT2& NewPosition)
{
	m_ComponentTransform.Translation = NewPosition;
	m_ComponentTransform.MatrixTranslation = XMMatrixTranslation(m_ComponentTransform.Translation.x, m_ComponentTransform.Translation.y, 0);

	UpdateWorldMatrix();
}

void CObject2D::RotateTo(float NewAngle)
{
	m_ComponentTransform.RotationAngle = NewAngle;
	if (m_ComponentTransform.RotationAngle > XM_2PI) m_ComponentTransform.RotationAngle = 0.0f;
	if (m_ComponentTransform.RotationAngle < -XM_2PI) m_ComponentTransform.RotationAngle = 0.0f;
	m_ComponentTransform.MatrixRotation = XMMatrixRotationZ(m_ComponentTransform.RotationAngle);

	UpdateWorldMatrix();
}

void CObject2D::ScaleTo(const XMFLOAT2& NewScalar)
{
	m_ComponentTransform.Scaling = NewScalar;
	if (m_ComponentTransform.Scaling.x < 0.0f) m_ComponentTransform.Scaling.x = 0.0f;
	if (m_ComponentTransform.Scaling.y < 0.0f) m_ComponentTransform.Scaling.y = 0.0f;
	m_ComponentTransform.MatrixScaling = XMMatrixScaling(m_ComponentTransform.Scaling.x, m_ComponentTransform.Scaling.y, 1);

	UpdateWorldMatrix();
}

void CObject2D::UpdateWorldMatrix()
{
	m_ComponentTransform.MatrixWorld = m_ComponentTransform.MatrixScaling * m_ComponentTransform.MatrixRotation * m_ComponentTransform.MatrixTranslation;
}

void CObject2D::IsVisible(bool bIsVisible)
{
	m_bIsVisible = bIsVisible;
}

auto CObject2D::IsVisible() const->bool
{
	return m_bIsVisible;
}

auto CObject2D::GetWorldMatrix() const->const XMMATRIX&
{
	return m_ComponentTransform.MatrixWorld;
}

auto CObject2D::GetName() const->const std::string&
{
	return m_Name;
}

auto CObject2D::GetData()->SModel2D&
{
	return m_Model2D;
}

auto CObject2D::GetData() const->const SModel2D&
{
	return m_Model2D;
}

auto CObject2D::HasTexture() const->bool
{
	return (m_Texture) ? true : false;
}

void CObject2D::UpdateVertexBuffer()
{
	if (!m_bIsDynamic) return;

	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_Model2D.vVertices[0], sizeof(SVertex) * m_Model2D.vVertices.size());
		m_PtrDeviceContext->Unmap(m_VertexBuffer.Get(), 0);
	}
}

void CObject2D::Draw(bool bIgnoreOwnTexture) const
{
	if (m_Texture && !bIgnoreOwnTexture) m_Texture->Use();

	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_PtrDeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(m_Model2D.vTriangles.size() * 3), 0, 0);
}