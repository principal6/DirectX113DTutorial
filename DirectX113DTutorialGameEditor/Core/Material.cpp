#include "Material.h"
#include <wincodec.h>

using std::vector;
using std::string;
using std::wstring;
using std::make_unique;

void CTexture::CreateTextureFromFile(const string& FileName, bool bShouldGenerateMipMap)
{
	m_FileName = FileName;

	size_t found{ m_FileName.find_last_of(L'.') };
	string Ext{ m_FileName.substr(found) };
	wstring wFileName{ m_FileName.begin(), m_FileName.end() };
	for (auto& c : Ext)
	{
		c = toupper(c);
	}
	if (Ext == ".DDS")
	{
		assert(SUCCEEDED(CreateDDSTextureFromFile(m_PtrDevice, wFileName.c_str(), (ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}
	else
	{
		if (bShouldGenerateMipMap)
		{
			ComPtr<ID3D11Texture2D> NonMipMappedTexture{};

			// @important: ignore sRGB!!
			CreateWICTextureFromFileEx(m_PtrDevice, m_PtrDeviceContext, wFileName.c_str(), 0i64, D3D11_USAGE_DEFAULT,
				D3D11_BIND_SHADER_RESOURCE, 0, 0, WIC_LOADER_IGNORE_SRGB, (ID3D11Resource**)NonMipMappedTexture.GetAddressOf(), nullptr);

			if (!NonMipMappedTexture)
			{
				MB_WARN(("텍스처를 찾을 수 없습니다." + m_FileName).c_str(), "텍스처 생성 실패");
				return;
			}

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
	}

	UpdateTextureSize();

	m_bIsCreated = true;
}

void CTexture::CreateTextureFromMemory(const vector<uint8_t>& RawData, bool bShouldGenerateMipMap)
{
	if (bShouldGenerateMipMap)
	{
		ComPtr<ID3D11Texture2D> NonMipMappedTexture{};

		// @important: ignore sRGB!!
		CreateWICTextureFromMemoryEx(m_PtrDevice, &RawData[0], RawData.size(), 0i64, D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE, 0, 0, WIC_LOADER_IGNORE_SRGB, (ID3D11Resource**)NonMipMappedTexture.GetAddressOf(), nullptr);

		if (!NonMipMappedTexture)
		{
			MB_WARN("텍스처를 생성할 수 없습니다.", "텍스처 생성 실패");
			return;
		}

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
		assert(SUCCEEDED(CreateWICTextureFromMemory(m_PtrDevice, &RawData[0], RawData.size(),
			(ID3D11Resource**)m_Texture2D.GetAddressOf(), &m_ShaderResourceView)));
	}

	UpdateTextureSize();

	m_bIsCreated = true;
}

void CTexture::CreateBlankTexture(EFormat Format, const XMFLOAT2& TextureSize)
{
	m_TextureSize = TextureSize;

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.ArraySize = 1;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Texture2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Texture2DDesc.Format = (DXGI_FORMAT)Format;
	Texture2DDesc.Height = static_cast<UINT>(m_TextureSize.y);
	Texture2DDesc.MipLevels = 1;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DYNAMIC;
	Texture2DDesc.Width = static_cast<UINT>(m_TextureSize.x);

	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.GetAddressOf());
	m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.GetAddressOf());

	m_bIsCreated = true;
}

void CTexture::SaveToDDSFile(const string& FileName)
{
	wstring wFileName{ FileName.begin(), FileName.end() };
	D3D11_TEXTURE2D_DESC Desc{};
	m_Texture2D->GetDesc(&Desc);
	assert(SUCCEEDED(SaveDDSTextureToFile(m_PtrDeviceContext, (ID3D11Resource*)m_Texture2D.Get(), wFileName.c_str())));
}

void CTexture::UpdateTextureSize()
{
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	m_Texture2D->GetDesc(&Texture2DDesc);

	m_TextureSize.x = static_cast<float>(Texture2DDesc.Width);
	m_TextureSize.x = static_cast<float>(Texture2DDesc.Height);
}

void CTexture::UpdateTextureRawData(const SPixel8UInt* const PtrData)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		size_t SrcRowPixelCount{ (size_t)m_TextureSize.x };
		uint8_t* PtrDest{ (uint8_t*)MappedSubresource.pData };

		UINT RowCount{ (MappedSubresource.DepthPitch) ?
			MappedSubresource.DepthPitch / MappedSubresource.RowPitch :
			static_cast<UINT>(SrcRowPixelCount) };
		for (UINT iRow = 0; iRow < RowCount; ++iRow)
		{
			memcpy(PtrDest + (static_cast<size_t>(iRow)* MappedSubresource.RowPitch),
				PtrData + (static_cast<size_t>(iRow)* SrcRowPixelCount),
				SrcRowPixelCount * sizeof(SPixel8UInt));
		}

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CTexture::UpdateTextureRawData(const SPixel32UInt* const PtrData)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		size_t SrcRowPixelCount{ (size_t)m_TextureSize.x };
		uint8_t* PtrDest{ (uint8_t*)MappedSubresource.pData };

		UINT RowCount{ (MappedSubresource.DepthPitch) ?
			MappedSubresource.DepthPitch / MappedSubresource.RowPitch : 
			static_cast<UINT>(SrcRowPixelCount) };
		for (UINT iRow = 0; iRow < RowCount; ++iRow)
		{
			memcpy(PtrDest + (static_cast<size_t>(iRow) * MappedSubresource.RowPitch), 
				PtrData + (static_cast<size_t>(iRow) * SrcRowPixelCount),
				SrcRowPixelCount * sizeof(SPixel32UInt));
		}

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CTexture::UpdateTextureRawData(const SPixel128Float* const PtrData)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_Texture2D.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		size_t SrcRowPixelCount{ (size_t)m_TextureSize.x };
		size_t SrcRowCount{ (size_t)m_TextureSize.y };
		uint8_t* PtrDest{ (uint8_t*)MappedSubresource.pData };

		UINT RowCount{ (MappedSubresource.DepthPitch) ?
			MappedSubresource.DepthPitch / MappedSubresource.RowPitch :
			static_cast<UINT>(SrcRowCount) };
		for (UINT iRow = 0; iRow < RowCount; ++iRow)
		{
			memcpy(PtrDest + (static_cast<size_t>(iRow) * MappedSubresource.RowPitch),
				PtrData + (static_cast<size_t>(iRow) * SrcRowPixelCount),
				SrcRowPixelCount * sizeof(SPixel128Float));
		}

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CTexture::SetSlot(UINT Slot)
{
	m_Slot = Slot;
}

void CTexture::SetShaderType(EShaderType eShaderType)
{
	m_eShaderType = eShaderType;
}

void CTexture::Use(int ForcedSlot) const
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

void CMaterialTextureSet::CreateTextures(CMaterialData& MaterialData)
{
	UINT SlotOffset{ (UINT)(MaterialData.Index() * KMaxTextureCountPerMaterial) };
	for (int iTexture = 0; iTexture < KMaxTextureCountPerMaterial; ++iTexture)
	{
		STextureData::EType eTextureType{ (STextureData::EType)iTexture };
		if (MaterialData.HasTexture(eTextureType))
		{
			STextureData& TextureData{ MaterialData.GetTextureData(eTextureType) };
			if (TextureData.vRawData.empty())
			{
				m_Textures[iTexture].CreateTextureFromFile(TextureData.FileName, true);
			}
			else
			{
				m_Textures[iTexture].CreateTextureFromMemory(TextureData.vRawData, true);
			}

			// @important
			m_Textures[iTexture].SetSlot(SlotOffset + iTexture);

			// @important
			if (eTextureType == STextureData::EType::DisplacementTexture)
			{
				m_Textures[iTexture].SetShaderType(EShaderType::DomainShader);
				m_Textures[iTexture].SetSlot((UINT)MaterialData.Index()); // @warning
			}

			TextureData.vRawData.clear(); // @important
		}
	}
}

void CMaterialTextureSet::UseTextures() const
{
	for (int iTexture = 0; iTexture < KMaxTextureCountPerMaterial; ++iTexture)
	{
		if (m_Textures[iTexture].IsCreated()) m_Textures[iTexture].Use();
	}
}

ID3D11ShaderResourceView* CMaterialTextureSet::GetTextureSRV(STextureData::EType eType)
{
	return m_Textures[(int)eType].GetShaderResourceViewPtr();
}

void CMaterialData::Name(const string& Name)
{
	m_Name = Name;
}

const string& CMaterialData::Name() const
{
	return m_Name;
}

void CMaterialData::Index(size_t Value)
{
	m_Index = Value;
}

size_t CMaterialData::Index() const
{
	return m_Index;
}

void CMaterialData::AmbientColor(const XMFLOAT3& Color)
{
	m_AmbientColor = Color;
}

const XMFLOAT3& CMaterialData::AmbientColor() const
{
	return m_AmbientColor;
}

void CMaterialData::DiffuseColor(const XMFLOAT3& Color)
{
	m_DiffuseColor = Color;
}

const XMFLOAT3& CMaterialData::DiffuseColor() const
{
	return m_DiffuseColor;
}

void CMaterialData::SpecularColor(const XMFLOAT3& Color)
{
	m_SpecularColor = Color;
}

const XMFLOAT3& CMaterialData::SpecularColor() const
{
	return m_SpecularColor;
}

void CMaterialData::SpecularExponent(float Value)
{
	m_SpecularExponent = Value;
}

float CMaterialData::SpecularExponent() const
{
	return m_SpecularExponent;
}

void CMaterialData::SpecularIntensity(float Value)
{
	m_SpecularIntensity = Value;
}

float CMaterialData::SpecularIntensity() const
{
	return m_SpecularIntensity;
}

STextureData& CMaterialData::GetTextureData(STextureData::EType eType)
{
	return m_TextureData[(int)eType];
}

void CMaterialData::SetTextureFileName(STextureData::EType eType, const string& FileName)
{
	if (FileName.empty()) return;

	m_bHasAnyTexture = true;
	m_TextureData[(int)eType].bHasTexture = true;
	m_TextureData[(int)eType].FileName = FileName;
}

const std::string CMaterialData::GetTextureFileName(STextureData::EType eType) const
{
	return m_TextureData[(int)eType].FileName;
}

void CMaterialData::ShouldGenerateMipMap(bool Value)
{
	m_bShouldGenerateMipMap = Value;
}

bool CMaterialData::ShouldGenerateMipMap() const
{
	return m_bShouldGenerateMipMap;
}

void CMaterialData::HasAnyTexture(bool Value)
{
	m_bHasAnyTexture = Value;
}

bool CMaterialData::HasAnyTexture() const
{
	return m_bHasAnyTexture;
}



void CMaterialData::SetUniformColor(const XMFLOAT3& Color)
{
	m_AmbientColor = m_DiffuseColor = m_SpecularColor = Color;
}

