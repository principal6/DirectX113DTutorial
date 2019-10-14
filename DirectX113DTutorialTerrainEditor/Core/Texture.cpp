#include "Texture.h"

void CTexture::CreateFromFile(const string& TextureFileName)
{
	size_t found{ TextureFileName.find_last_of(L'.') };
	string Ext{ TextureFileName.substr(found) };

	wstring wFileName{ TextureFileName.begin(), TextureFileName.end() };

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

void CTexture::Use() const
{
	m_PtrDeviceContext->PSSetShaderResources(0, 1, m_ShaderResourceView.GetAddressOf());
}