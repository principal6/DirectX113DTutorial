#include "Material.h"
#include <wincodec.h>
#include "../DirectXTex/DirectXTex.h"

#pragma comment(lib, "DirectXTex.lib")

using std::vector;
using std::string;
using std::wstring;
using std::make_unique;

bool CTexture::CreateTextureFromFile(const string& FileName, bool bShouldGenerateMipMap)
{
	m_FileName = FileName;

	if (m_FileName.empty())
	{
		MB_WARN(("���� �̸��� �ι����Դϴ�. (" + m_FileName + ")").c_str(), "�ؽ�ó ���� ����");
		return false;
	}

	size_t found{ m_FileName.find_last_of(L'.') };
	string Ext{ m_FileName.substr(found) };
	wstring wFileName{ m_FileName.begin(), m_FileName.end() };
	for (auto& c : Ext)
	{
		c = toupper(c);
	}
	if (Ext == ".DDS")
	{
		CreateDDSTextureFromFile(m_PtrDevice, wFileName.c_str(), 
			(ID3D11Resource**)m_Texture2D.ReleaseAndGetAddressOf(), m_ShaderResourceView.ReleaseAndGetAddressOf());

		if (!m_Texture2D)
		{
			MB_WARN(("�ؽ�ó�� ã�� �� �����ϴ�. (" + m_FileName + ")").c_str(), "�ؽ�ó ���� ����");
			return false;
		}
	}
	else
	{
		if (bShouldGenerateMipMap)
		{
			ComPtr<ID3D11Texture2D> NonMipMappedTexture{};

			CreateWICTextureFromFileEx(m_PtrDevice, m_PtrDeviceContext, wFileName.c_str(), 0i64, D3D11_USAGE_DEFAULT,
				D3D11_BIND_SHADER_RESOURCE, 0, 0, 0, (ID3D11Resource**)NonMipMappedTexture.GetAddressOf(), nullptr);

			if (!NonMipMappedTexture)
			{
				MB_WARN(("�ؽ�ó�� ã�� �� �����ϴ�. (" + m_FileName + ")").c_str(), "�ؽ�ó ���� ����");
				return false;
			}

			NonMipMappedTexture->GetDesc(&m_Texture2DDesc);
			m_Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			m_Texture2DDesc.CPUAccessFlags = 0;
			m_Texture2DDesc.MipLevels = 0;
			m_Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
			m_Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

			assert(SUCCEEDED(m_PtrDevice->CreateTexture2D(&m_Texture2DDesc, nullptr, m_Texture2D.ReleaseAndGetAddressOf())));

			m_PtrDeviceContext->ResolveSubresource(m_Texture2D.Get(), 0, NonMipMappedTexture.Get(), 0, m_Texture2DDesc.Format);

			m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.ReleaseAndGetAddressOf());

			m_PtrDeviceContext->GenerateMips(m_ShaderResourceView.Get());
		}
		else
		{
			assert(SUCCEEDED(CreateWICTextureFromFile(m_PtrDevice, wFileName.c_str(), 
				(ID3D11Resource**)m_Texture2D.ReleaseAndGetAddressOf(), m_ShaderResourceView.ReleaseAndGetAddressOf())));
		}
	}

	UpdateTextureInfo();

	m_bIsCreated = true;

	return true;
}

void CTexture::CreateTextureFromMemory(const vector<uint8_t>& RawData, bool bShouldGenerateMipMap)
{
	if (bShouldGenerateMipMap)
	{
		ComPtr<ID3D11Texture2D> NonMipMappedTexture{};

		CreateWICTextureFromMemoryEx(m_PtrDevice, &RawData[0], RawData.size(), 0i64, D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE, 0, 0, 0, (ID3D11Resource**)NonMipMappedTexture.GetAddressOf(), nullptr);

		if (!NonMipMappedTexture)
		{
			MB_WARN("�ؽ�ó�� ������ �� �����ϴ�.", "�ؽ�ó ���� ����");
			return;
		}

		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		NonMipMappedTexture->GetDesc(&Texture2DDesc);
		Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.MipLevels = 0;
		Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

		assert(SUCCEEDED(m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.ReleaseAndGetAddressOf())));

		m_PtrDeviceContext->ResolveSubresource(m_Texture2D.Get(), 0, NonMipMappedTexture.Get(), 0, Texture2DDesc.Format);
		
		m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.ReleaseAndGetAddressOf());

		m_PtrDeviceContext->GenerateMips(m_ShaderResourceView.Get());
	}
	else
	{
		assert(SUCCEEDED(CreateWICTextureFromMemory(m_PtrDevice, &RawData[0], RawData.size(),
			(ID3D11Resource**)m_Texture2D.ReleaseAndGetAddressOf(), m_ShaderResourceView.ReleaseAndGetAddressOf())));
	}

	UpdateTextureInfo();

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
	Texture2DDesc.MiscFlags = 0;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DYNAMIC;
	Texture2DDesc.Width = static_cast<UINT>(m_TextureSize.x);

	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.ReleaseAndGetAddressOf());
	m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.ReleaseAndGetAddressOf());

	m_bIsCreated = true;
}

void CTexture::CreateCubeMapFromFile(const std::string& FileName)
{
	m_FileName = FileName;

	if (m_FileName.empty())
	{
		MB_WARN(("���� �̸��� �ι����Դϴ�. (" + m_FileName + ")").c_str(), "�ؽ�ó ���� ����");
		return;
	}

	size_t found{ m_FileName.find_last_of(L'.') };
	string Ext{ m_FileName.substr(found) };
	wstring wFileName{ m_FileName.begin(), m_FileName.end() };
	for (auto& c : Ext)
	{
		c = toupper(c);
	}
	if (Ext == ".DDS")
	{
		CreateDDSTextureFromFileEx(m_PtrDevice, wFileName.c_str(), 0i64,
			D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, 
			false, (ID3D11Resource**)m_Texture2D.ReleaseAndGetAddressOf(), m_ShaderResourceView.ReleaseAndGetAddressOf());

		if (!m_Texture2D)
		{
			MB_WARN(("�ؽ�ó�� ã�� �� �����ϴ�. (" + m_FileName + ")").c_str(), "�ؽ�ó ���� ����");
			return;
		}
	}
	else if (Ext == ".HDR")
	{
		ScratchImage _ScratchImage{};
		LoadFromHDRFile(wFileName.c_str(), nullptr, _ScratchImage);

		CreateTexture(m_PtrDevice, _ScratchImage.GetImages(), _ScratchImage.GetImageCount(), _ScratchImage.GetMetadata(),
			(ID3D11Resource**)m_Texture2D.ReleaseAndGetAddressOf());

		if (!m_Texture2D)
		{
			MB_WARN(("�ؽ�ó�� ã�� �� �����ϴ�. (" + m_FileName + ")").c_str(), "�ؽ�ó ���� ����");
			return;
		}

		m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.ReleaseAndGetAddressOf());

		m_bIsHDR = true;
	}
	else
	{
		assert(true);
	}

	UpdateTextureInfo();

	m_bIsCreated = true;
}

void CTexture::CopyTexture(ID3D11Texture2D* const Texture)
{
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture->GetDesc(&Texture2DDesc);

	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_Texture2D.ReleaseAndGetAddressOf());
	m_PtrDeviceContext->CopyResource(m_Texture2D.Get(), Texture);

	m_PtrDevice->CreateShaderResourceView(m_Texture2D.Get(), nullptr, m_ShaderResourceView.ReleaseAndGetAddressOf());

	UpdateTextureInfo();
}

void CTexture::ReleaseResources()
{
	m_ShaderResourceView.Reset();
	m_Texture2D.Reset();
	m_bIsCreated = false;
}

void CTexture::SaveWICFile(const std::string& FileName) const
{
	m_FileName = FileName;
	size_t ExtPos{ m_FileName.find('.') };
	string Ext{ m_FileName.substr(ExtPos + 1) };
	for (auto& ch : Ext) ch = toupper(ch);

	wstring wFileName{ FileName.begin(), FileName.end() };

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	m_Texture2D->GetDesc(&Texture2DDesc);

	DirectX::ScratchImage _ScratchImage{};
	CaptureTexture(m_PtrDevice, m_PtrDeviceContext, m_Texture2D.Get(), _ScratchImage);
	
	if (Ext == "PNG")
	{
		assert(SUCCEEDED(SaveToWICFile(*_ScratchImage.GetImage(0, 0, 0), WIC_FLAGS_NONE, GUID_ContainerFormatPng, wFileName.c_str())));
	}
	else if (Ext == "JPG" || Ext == "JPEG")
	{
		assert(SUCCEEDED(SaveToWICFile(*_ScratchImage.GetImage(0, 0, 0), WIC_FLAGS_NONE, GUID_ContainerFormatJpeg, wFileName.c_str())));
	}
	else if (Ext == "TIF" || Ext == "TIFF")
	{
		assert(SUCCEEDED(SaveToWICFile(*_ScratchImage.GetImage(0, 0, 0), WIC_FLAGS_NONE, GUID_ContainerFormatTiff, wFileName.c_str())));
	}
	else if (Ext == "GIF")
	{
		assert(SUCCEEDED(SaveToWICFile(*_ScratchImage.GetImage(0, 0, 0), WIC_FLAGS_NONE, GUID_ContainerFormatGif, wFileName.c_str())));
	}
}

void CTexture::SaveDDSFile(const string& FileName, bool bIsLookUpTexture) const
{
	m_FileName = FileName;

	wstring wFileName{ FileName.begin(), FileName.end() };

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	m_Texture2D->GetDesc(&Texture2DDesc);

	DirectX::ScratchImage _ScratchImage{};
	CaptureTexture(m_PtrDevice, m_PtrDeviceContext, m_Texture2D.Get(), _ScratchImage);

	if (bIsLookUpTexture)
	{
		assert(SUCCEEDED(SaveToDDSFile(*_ScratchImage.GetImage(0, 0, 0), DDS_FLAGS_NONE, wFileName.c_str())));
	}
	else
	{
		assert(SUCCEEDED(SaveToDDSFile(_ScratchImage.GetImages(), _ScratchImage.GetImageCount(), _ScratchImage.GetMetadata(), DDS_FLAGS_NONE, wFileName.c_str())));
	}
}

void CTexture::UpdateTextureInfo()
{
	m_Texture2D->GetDesc(&m_Texture2DDesc);

	// @important: check if sRGB
	if (m_Texture2DDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || 
		m_Texture2DDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB || 
		m_Texture2DDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB ||
		m_Texture2DDesc.Format == DXGI_FORMAT_BC1_UNORM_SRGB ||
		m_Texture2DDesc.Format == DXGI_FORMAT_BC2_UNORM_SRGB ||
		m_Texture2DDesc.Format == DXGI_FORMAT_BC3_UNORM_SRGB ||
		m_Texture2DDesc.Format == DXGI_FORMAT_BC7_UNORM_SRGB)
	{
		m_bIssRGB = true;
	}

	m_TextureSize.x = static_cast<float>(m_Texture2DDesc.Width);
	m_TextureSize.y = static_cast<float>(m_Texture2DDesc.Height);
}

void CTexture::UpdateTextureRawData(const SPixel8Uint* const PtrData)
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
				SrcRowPixelCount * sizeof(SPixel8Uint));
		}

		m_PtrDeviceContext->Unmap(m_Texture2D.Get(), 0);
	}
}

void CTexture::UpdateTextureRawData(const SPixel32Uint* const PtrData)
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
				SrcRowPixelCount * sizeof(SPixel32Uint));
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
	for (int iTexture = 0; iTexture < KMaxTextureCountPerMaterial; ++iTexture)
	{
		ETextureType eTextureType{ (ETextureType)iTexture };
		CreateTexture(eTextureType, MaterialData);
	}
}

void CMaterialTextureSet::CreateTexture(ETextureType eType, CMaterialData& MaterialData)
{
	UINT SlotOffset{ (UINT)(MaterialData.Index() * KMaxTextureCountPerMaterial) };
	if (MaterialData.HasTexture(eType))
	{
		size_t iTexture{ (size_t)eType };
		STextureData& TextureData{ MaterialData.GetTextureData(eType) };
		if (TextureData.vRawData.empty())
		{
			if (!m_Textures[iTexture].CreateTextureFromFile(TextureData.FileName, true))
			{
				// @important: failed to load texture

				TextureData.bHasTexture = false;
				TextureData.FileName.clear();
			}
		}
		else
		{
			m_Textures[iTexture].CreateTextureFromMemory(TextureData.vRawData, true);
			TextureData.vRawData.clear(); // @important
		}

		// @important
		m_Textures[iTexture].SetSlot(static_cast<UINT>(SlotOffset + iTexture));

		// @important
		if (eType == ETextureType::DisplacementTexture)
		{
			m_Textures[iTexture].SetShaderType(EShaderType::DomainShader);
			m_Textures[iTexture].SetSlot((UINT)MaterialData.Index()); // @warning
		}

		// @important
		TextureData.bIsSRGB = m_Textures[iTexture].IssRGB();
	}
}

void CMaterialTextureSet::DestroyAllTextures()
{
	for (int iTexture = 0; iTexture < KMaxTextureCountPerMaterial; ++iTexture)
	{
		DestroyTexture((ETextureType)iTexture);
	}
}

void CMaterialTextureSet::DestroyTexture(ETextureType eType)
{
	size_t iTexture{ (size_t)eType };
	m_Textures[iTexture].ReleaseResources();
}

void CMaterialTextureSet::SetSlot(ETextureType eType, uint32_t Slot)
{
	m_Textures[(size_t)eType].SetSlot(Slot);
}

void CMaterialTextureSet::UseTextures() const
{
	for (int iTexture = 0; iTexture < KMaxTextureCountPerMaterial; ++iTexture)
	{
		if (m_Textures[iTexture].IsCreated()) m_Textures[iTexture].Use();
	}
}

bool CMaterialTextureSet::HasTexture(ETextureType eType) const
{
	return m_Textures[(size_t)eType].IsCreated();
}

bool CMaterialTextureSet::IssRGB(ETextureType eType) const
{
	return m_Textures[(size_t)eType].IssRGB();
}

const CTexture& CMaterialTextureSet::GetTexture(ETextureType eType) const
{
	return m_Textures[(int)eType];
}

ID3D11ShaderResourceView* CMaterialTextureSet::GetTextureSRV(ETextureType eType)
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

void CMaterialData::Roughness(float Value)
{
	m_Roughness = Value;
}

float CMaterialData::Roughness() const
{
	return m_Roughness;
}

void CMaterialData::Metalness(float Value)
{
	m_Metalness = Value;
}

float CMaterialData::Metalness() const
{
	return m_Metalness;
}

void CMaterialData::ClearTextureData(ETextureType eType)
{
	if (m_bHasAnyTexture)
	{
		m_TextureData[(int)eType].bHasTexture = false;
		m_TextureData[(int)eType].FileName.clear();
		m_TextureData[(int)eType].vRawData.clear();

		m_bHasAnyTexture = false;
		for (const auto& TextureData : m_TextureData)
		{
			if (TextureData.bHasTexture) m_bHasAnyTexture = true;
		}
	}
}

void CMaterialData::ClearAllTexturesData()
{
	if (!m_bHasAnyTexture) return;
	for (int iTexture = 0; iTexture < KMaxTextureCountPerMaterial; ++iTexture)
	{
		m_TextureData[iTexture].bHasTexture = false;
		m_TextureData[iTexture].FileName.clear();
		m_TextureData[iTexture].vRawData.clear();
	}
	m_bHasAnyTexture = false;
}

STextureData& CMaterialData::GetTextureData(ETextureType eType)
{
	return m_TextureData[(int)eType];
}

void CMaterialData::SetTextureFileName(ETextureType eType, const string& FileName)
{
	if (FileName.empty()) return;

	m_bHasAnyTexture = true;
	m_TextureData[(int)eType].bHasTexture = true;
	m_TextureData[(int)eType].FileName = FileName;
}

const std::string CMaterialData::GetTextureFileName(ETextureType eType) const
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

