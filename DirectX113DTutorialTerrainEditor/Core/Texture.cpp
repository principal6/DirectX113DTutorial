#include "Texture.h"

void CTexture::CreateFromFile(const string& TextureFileName)
{
	m_TextureFileName = TextureFileName;

	size_t found{ m_TextureFileName.find_last_of(L'.') };
	string Ext{ m_TextureFileName.substr(found) };
	wstring wFileName{ m_TextureFileName.begin(), m_TextureFileName.end() };
	for (auto& c : Ext)
	{
		c = toupper(c);
	}
	if (Ext == ".DDS")
	{
		assert(SUCCEEDED(CreateDDSTextureFromFile(m_PtrDevice, wFileName.c_str(),
			(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}
	else
	{
		assert(SUCCEEDED(CreateWICTextureFromFile(m_PtrDevice, wFileName.c_str(),
			(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}
}

void CTexture::CreateWICFromMemory(const vector<uint8_t>& RawData)
{
	assert(SUCCEEDED(CreateWICTextureFromMemory(m_PtrDevice, &RawData[0], RawData.size(), 
		(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
}

void CTexture::CreateBlankTexture(DXGI_FORMAT Format, const XMFLOAT2& TextureSize)
{
	m_TextureSize = TextureSize;

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.ArraySize = 1;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Texture2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Texture2DDesc.Format = Format;
	Texture2DDesc.Height = static_cast<UINT>(m_TextureSize.y);
	Texture2DDesc.MipLevels = 1;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DYNAMIC;
	Texture2DDesc.Width = static_cast<UINT>(m_TextureSize.x);

	switch (Format)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		m_PixelByteSize = sizeof(float) * 4;
		break;
	case DXGI_FORMAT_R32G32B32_FLOAT:
		m_PixelByteSize = sizeof(float) * 3;
		break;
	case DXGI_FORMAT_R32G32_FLOAT:
		m_PixelByteSize = sizeof(float) * 2;
		break;
	case DXGI_FORMAT_R32_FLOAT:
		m_PixelByteSize = sizeof(float) * 1;
		break;
	case DXGI_FORMAT_R8G8B8A8_UINT:
		m_PixelByteSize = sizeof(uint8_t) * 4;
		break;
	default:
		assert(true);
		break;
	}

	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.GetAddressOf());
	m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.GetAddressOf());
}

void CTexture::UpdateTextureRawData(const void* PtrData)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		MappedSubresource.RowPitch = static_cast<UINT>(m_TextureSize.x * m_PixelByteSize);
		memcpy(MappedSubresource.pData, PtrData, MappedSubresource.RowPitch * (int)m_TextureSize.y);

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CTexture::SetSlot(UINT Slot)
{
	m_Slot = Slot;
}

void CTexture::Use(int ForcedSlot) const
{
	if (ForcedSlot != -1)
	{
		m_PtrDeviceContext->PSSetShaderResources(static_cast<UINT>(ForcedSlot), 1, m_ShaderResourceView.GetAddressOf());
	}
	else
	{
		m_PtrDeviceContext->PSSetShaderResources(m_Slot, 1, m_ShaderResourceView.GetAddressOf());
	}
}