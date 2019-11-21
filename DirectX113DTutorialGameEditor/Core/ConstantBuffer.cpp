#include "ConstantBuffer.h"

void CConstantBuffer::Create()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(m_DataByteWidth);
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = m_PtrData;

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_ConstantBuffer.GetAddressOf())));
}

void CConstantBuffer::Update()
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, m_PtrData, m_DataByteWidth);

		m_PtrDeviceContext->Unmap(m_ConstantBuffer.Get(), 0);
	}
}

void CConstantBuffer::Use(EShaderType eShaderType, uint32_t Slot) const
{
	switch (eShaderType)
	{
	case EShaderType::VertexShader:
		m_PtrDeviceContext->VSSetConstantBuffers(Slot, 1, m_ConstantBuffer.GetAddressOf());
		break;
	case EShaderType::HullShader:
		m_PtrDeviceContext->HSSetConstantBuffers(Slot, 1, m_ConstantBuffer.GetAddressOf());
		break;
	case EShaderType::DomainShader:
		m_PtrDeviceContext->DSSetConstantBuffers(Slot, 1, m_ConstantBuffer.GetAddressOf());
		break;
	case EShaderType::GeometryShader:
		m_PtrDeviceContext->GSSetConstantBuffers(Slot, 1, m_ConstantBuffer.GetAddressOf());
		break;
	case EShaderType::PixelShader:
		m_PtrDeviceContext->PSSetConstantBuffers(Slot, 1, m_ConstantBuffer.GetAddressOf());
		break;
	default:
		break;
	}
}
