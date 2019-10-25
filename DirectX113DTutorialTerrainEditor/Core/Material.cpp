#include "Material.h"

void CMaterial::CTexture::CreateTextureFromFile(const string& TextureFileName, bool bShouldGenerateMipMap)
{
	m_TextureFileName = TextureFileName;

	size_t found{ m_TextureFileName.find_last_of(L'.') };
	string Ext{ m_TextureFileName.substr(found) };
	wstring wFileName{ m_TextureFileName.begin(), m_TextureFileName.end() };
	for (auto& c : Ext)
	{
		c = toupper(c);
	}
	assert(Ext != ".DDS");

	if (bShouldGenerateMipMap)
	{
		wstring wFileName{ m_TextureFileName.begin(), m_TextureFileName.end() };
		ComPtr<ID3D11Texture2D> NonMipMappedTexture{};
		CreateWICTextureFromFile(m_PtrDevice, wFileName.c_str(), (ID3D11Resource**)NonMipMappedTexture.GetAddressOf(), nullptr);

		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		NonMipMappedTexture->GetDesc(&Texture2DDesc);
		Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.MipLevels = 0;
		Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

		assert(SUCCEEDED(m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.GetAddressOf())));

		m_PtrDeviceContext->ResolveSubresource(m_Texture2D.Get(), 0, NonMipMappedTexture.Get(), 0, Texture2DDesc.Format);

		m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.GetAddressOf());

		m_PtrDeviceContext->GenerateMips(m_ShaderResourceView.Get());

	}
	else
	{
		assert(SUCCEEDED(CreateWICTextureFromFile(m_PtrDevice, wFileName.c_str(), (ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}

	SetTextureSize();
}

void CMaterial::CTexture::CreateTextureFromMemory(const vector<uint8_t>& RawData)
{
	assert(SUCCEEDED(CreateWICTextureFromMemory(m_PtrDevice, &RawData[0], RawData.size(), 
		(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));

	SetTextureSize();
}

void CMaterial::CTexture::CreateBlankTexture(DXGI_FORMAT Format, const XMFLOAT2& TextureSize)
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

	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.GetAddressOf());
	m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.GetAddressOf());
}

void CMaterial::CTexture::SetTextureSize()
{
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	m_Texture2D->GetDesc(&Texture2DDesc);

	m_TextureSize.x = static_cast<float>(Texture2DDesc.Width);
	m_TextureSize.x = static_cast<float>(Texture2DDesc.Height);
}

void CMaterial::CTexture::UpdateTextureRawData(const SPixel8UInt* PtrData)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		size_t SrcRowPixelCount{ (size_t)m_TextureSize.x };
		uint8_t* PtrDest{ (uint8_t*)MappedSubresource.pData };

		UINT RowCount{ MappedSubresource.DepthPitch / MappedSubresource.RowPitch };
		for (UINT iRow = 0; iRow < RowCount; ++iRow)
		{
			memcpy(PtrDest + (static_cast<size_t>(iRow)* MappedSubresource.RowPitch),
				PtrData + (static_cast<size_t>(iRow)* SrcRowPixelCount),
				SrcRowPixelCount * sizeof(SPixel8UInt));
		}

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CMaterial::CTexture::UpdateTextureRawData(const SPixel32UInt* PtrData)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		size_t SrcRowPixelCount{ (size_t)m_TextureSize.x };
		uint8_t* PtrDest{ (uint8_t*)MappedSubresource.pData };

		UINT RowCount{ MappedSubresource.DepthPitch / MappedSubresource.RowPitch };
		for (UINT iRow = 0; iRow < RowCount; ++iRow)
		{
			memcpy(PtrDest + (static_cast<size_t>(iRow) * MappedSubresource.RowPitch), 
				PtrData + (static_cast<size_t>(iRow) * SrcRowPixelCount),
				SrcRowPixelCount * sizeof(SPixel32UInt));
		}

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CMaterial::CTexture::SetSlot(UINT Slot)
{
	m_Slot = Slot;
}

void CMaterial::CTexture::SetShaderType(EShaderType eShaderType)
{
	m_eShaderType = eShaderType;
}

void CMaterial::CTexture::Use(int ForcedSlot) const
{
	UINT Slot{ m_Slot };
	if (ForcedSlot != -1) Slot = static_cast<UINT>(ForcedSlot);

	switch (m_eShaderType)
	{
	case EShaderType::VertexShader:
		m_PtrDeviceContext->VSSetShaderResources(Slot, 1, m_ShaderResourceView.GetAddressOf());
		break;
	case EShaderType::HullShader:
		m_PtrDeviceContext->HSSetShaderResources(Slot, 1, m_ShaderResourceView.GetAddressOf());
		break;
	case EShaderType::DomainShader:
		m_PtrDeviceContext->DSSetShaderResources(Slot, 1, m_ShaderResourceView.GetAddressOf());
		break;
	case EShaderType::GeometryShader:
		m_PtrDeviceContext->GSSetShaderResources(Slot, 1, m_ShaderResourceView.GetAddressOf());
		break;
	case EShaderType::PixelShader:
		m_PtrDeviceContext->PSSetShaderResources(Slot, 1, m_ShaderResourceView.GetAddressOf());
		break;
	default:
		break;
	}
}

void CMaterial::SetName(const string& Name)
{
	m_Name = Name;
}

void CMaterial::SetDiffuseTextureRawData(const vector<uint8_t>& Data)
{
	bHasTexture = true;
	bHasDiffuseTexture = true;

	vEmbeddedDiffuseTextureRawData = Data;
}

void CMaterial::SetDiffuseTextureFileName(const string& FileName)
{
	bHasTexture = true;
	bHasDiffuseTexture = true;

	DiffuseTextureFileName = FileName;

	vEmbeddedDiffuseTextureRawData.clear();
}

void CMaterial::SetNormalTextureRawData(const vector<uint8_t>& Data)
{
	bHasTexture = true;
	bHasNormalTexture = true;

	vEmbeddedNormalTextureRawData = Data;
}

void CMaterial::SetNormalTextureFileName(const string& FileName)
{
	bHasTexture = true;
	bHasNormalTexture = true;

	NormalTextureFileName = FileName;

	vEmbeddedNormalTextureRawData.clear();
}

void CMaterial::SetUniformColor(const XMFLOAT3& Color)
{
	SetAmbientColor(Color);
	SetDiffuseColor(Color);
	SetSpecularColor(Color);
}

void CMaterial::SetAmbientColor(const XMFLOAT3& Color)
{
	MaterialAmbient = Color;
}

void CMaterial::SetDiffuseColor(const XMFLOAT3& Color)
{
	MaterialDiffuse = Color;
}

void CMaterial::SetSpecularColor(const XMFLOAT3& Color)
{
	MaterialSpecular = Color;
}

void CMaterial::SetSpecularExponent(float Exponent)
{
	SpecularExponent = Exponent;
}

void CMaterial::SetSpecularIntensity(float Intensity)
{
	SpecularIntensity = Intensity;
}