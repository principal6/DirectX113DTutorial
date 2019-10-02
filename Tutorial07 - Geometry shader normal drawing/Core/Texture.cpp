#include "Texture.h"

void CTexture::CreateFromFile(const wstring& TextureFileName)
{
	size_t found{ TextureFileName.find_last_of(L'.') };
	wstring Ext{ TextureFileName.substr(found) };

	for (auto& c : Ext)
	{
		c = toupper(c);
	}
	if (Ext == L".DDS")
	{
		assert(SUCCEEDED(CreateDDSTextureFromFile(m_PtrDevice, TextureFileName.c_str(),
			(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}
	else
	{
		assert(SUCCEEDED(CreateWICTextureFromFile(m_PtrDevice, TextureFileName.c_str(), 
			(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}

}

void CTexture::Use()
{
	m_PtrDeviceContext->PSSetShaderResources(0, 1, m_ShaderResourceView.GetAddressOf());
}