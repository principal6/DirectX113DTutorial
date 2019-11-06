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

		if (!NonMipMappedTexture)
		{
			MessageBox(nullptr, ("텍스처를 찾을 수 없습니다." + m_TextureFileName).c_str(), "파일 열기 오류", MB_OK | MB_ICONEXCLAMATION);
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

	UpdateTextureSize();

	m_bIsCreated = true;
}

void CMaterial::CTexture::CreateTextureFromMemory(const vector<uint8_t>& RawData, bool bShouldGenerateMipMap)
{
	if (bShouldGenerateMipMap)
	{
		ComPtr<ID3D11Texture2D> NonMipMappedTexture{};
		CreateWICTextureFromMemory(m_PtrDevice, &RawData[0], RawData.size(), (ID3D11Resource**)NonMipMappedTexture.GetAddressOf(), nullptr);

		if (!NonMipMappedTexture)
		{
			MessageBox(nullptr, "텍스처를 생성할 수 없습니다.", "텍스처 생성 오류", MB_OK | MB_ICONEXCLAMATION);
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

	m_bIsCreated = true;
}

void CMaterial::CTexture::UpdateTextureSize()
{
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	m_Texture2D->GetDesc(&Texture2DDesc);

	m_TextureSize.x = static_cast<float>(Texture2DDesc.Width);
	m_TextureSize.x = static_cast<float>(Texture2DDesc.Height);
}

void CMaterial::CTexture::UpdateTextureRawData(const SPixel8UInt* const PtrData)
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

void CMaterial::CTexture::UpdateTextureRawData(const SPixel32UInt* const PtrData)
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

void CMaterial::SetIndex(size_t Index)
{
	m_Index = Index;
}

void CMaterial::SetTextureRawData(CTexture::EType eType, const vector<uint8_t>& Data)
{
	if (!m_bHasTexture) m_bHasTexture = true;

	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		m_bHasDiffuseTexture = true;
		m_vEmbeddedDiffuseTextureRawData = Data;
		break;
	case CMaterial::CTexture::EType::NormalTexture:
		m_bHasNormalTexture = true;
		m_vEmbeddedNormalTextureRawData = Data;
		break;
	case CMaterial::CTexture::EType::DisplacementTexture:
		m_bHasDisplacementTexture = true;
		m_vEmbeddedDisplacementTextureRawData = Data;
		break;
	case CMaterial::CTexture::EType::OpacityTexture:
		m_bHasOpacityTexture = true;
		m_vEmbeddedOpacityTextureRawData = Data;
		break;
	default:
		break;
	}
}

void CMaterial::SetTextureFileName(CTexture::EType eType, const string& FileName)
{
	if (FileName.size() == 0) return;
	if (!m_bHasTexture) m_bHasTexture = true;

	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		m_bHasDiffuseTexture = true;
		m_DiffuseTextureFileName = FileName;
		m_vEmbeddedDiffuseTextureRawData.clear();
		break;
	case CMaterial::CTexture::EType::NormalTexture:
		m_bHasNormalTexture = true;
		m_NormalTextureFileName = FileName;
		m_vEmbeddedNormalTextureRawData.clear();
		break;
	case CMaterial::CTexture::EType::DisplacementTexture:
		m_bHasDisplacementTexture = true;
		m_DisplacementTextureFileName = FileName;
		m_vEmbeddedDisplacementTextureRawData.clear();
		break;
	case CMaterial::CTexture::EType::OpacityTexture:
		m_bHasOpacityTexture = true;
		m_OpacityTextureFileName = FileName;
		m_vEmbeddedOpacityTextureRawData.clear();
		break;
	default:
		break;
	}
}

void CMaterial::CreateTextures(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext)
{
	CreateTexture(CTexture::EType::DiffuseTexture, PtrDevice, PtrDeviceContext);
	CreateTexture(CTexture::EType::NormalTexture, PtrDevice, PtrDeviceContext);
	CreateTexture(CTexture::EType::DisplacementTexture, PtrDevice, PtrDeviceContext);
	CreateTexture(CTexture::EType::OpacityTexture, PtrDevice, PtrDeviceContext);
}

void CMaterial::UseTextures() const
{
	if (m_DiffuseTexture) m_DiffuseTexture->Use();
	if (m_NormalTexture) m_NormalTexture->Use();
	if (m_DisplacementTexture) m_DisplacementTexture->Use();
	if (m_OpacityTexture) m_OpacityTexture->Use();
}

void CMaterial::CreateTexture(CMaterial::CTexture::EType eType, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext)
{
	if (HasTexture(eType))
	{
		CMaterial::CTexture* PtrTexture{};

		switch (eType)
		{
		case CMaterial::CTexture::EType::DiffuseTexture:
			m_DiffuseTexture = make_unique<CMaterial::CTexture>(PtrDevice, PtrDeviceContext);
			PtrTexture = m_DiffuseTexture.get();
			break;
		case CMaterial::CTexture::EType::NormalTexture:
			m_NormalTexture = make_unique<CMaterial::CTexture>(PtrDevice, PtrDeviceContext);
			PtrTexture = m_NormalTexture.get();
			break;
		case CMaterial::CTexture::EType::DisplacementTexture:
			m_DisplacementTexture = make_unique<CMaterial::CTexture>(PtrDevice, PtrDeviceContext);
			PtrTexture = m_DisplacementTexture.get();
			break;
		case CMaterial::CTexture::EType::OpacityTexture:
			m_OpacityTexture = make_unique<CMaterial::CTexture>(PtrDevice, PtrDeviceContext);
			PtrTexture = m_OpacityTexture.get();
			break;
		default:
			break;
		}

		if (IsTextureEmbedded(eType))
		{
			PtrTexture->CreateTextureFromMemory(GetTextureRawData(eType), ShouldGenerateAutoMipMap());
			ClearEmbeddedTextureData(eType);
		}
		else
		{
			PtrTexture->CreateTextureFromFile(GetTextureFileName(eType), ShouldGenerateAutoMipMap());
		}

		UINT Offset{};
		switch (eType)
		{
		case CMaterial::CTexture::EType::DiffuseTexture:
			Offset = KDiffuseTextureSlotOffset;
			break;
		case CMaterial::CTexture::EType::NormalTexture:
			Offset = KNormalTextureSlotOffset;
			break;
		case CMaterial::CTexture::EType::DisplacementTexture:
			Offset = KDisplacementTextureSlotOffset;
			PtrTexture->SetShaderType(EShaderType::DomainShader);
			break;
		case CMaterial::CTexture::EType::OpacityTexture:
			Offset = KOpacityTextureSlotOffset;
			break;
		default:
			break;
		}

		PtrTexture->SetSlot(static_cast<UINT>(Offset + m_Index));
	}
}

void CMaterial::SetUniformColor(const XMFLOAT3& Color)
{
	SetAmbientColor(Color);
	SetDiffuseColor(Color);
	SetSpecularColor(Color);
}

void CMaterial::SetAmbientColor(const XMFLOAT3& Color)
{
	m_MaterialAmbient = Color;
}

void CMaterial::SetDiffuseColor(const XMFLOAT3& Color)
{
	m_MaterialDiffuse = Color;
}

void CMaterial::SetSpecularColor(const XMFLOAT3& Color)
{
	m_MaterialSpecular = Color;
}

void CMaterial::SetSpecularExponent(float Exponent)
{
	m_SpecularExponent = Exponent;
}

void CMaterial::SetSpecularIntensity(float Intensity)
{
	m_SpecularIntensity = Intensity;
}

void CMaterial::ClearEmbeddedTextureData(CTexture::EType eType)
{
	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		m_vEmbeddedDiffuseTextureRawData.clear();
		break;
	case CMaterial::CTexture::EType::NormalTexture:
		m_vEmbeddedNormalTextureRawData.clear();
		break;
	case CMaterial::CTexture::EType::DisplacementTexture:
		m_vEmbeddedDisplacementTextureRawData.clear();
		break;
	case CMaterial::CTexture::EType::OpacityTexture:
		m_vEmbeddedOpacityTextureRawData.clear();
		break;
	default:
		break;
	}
}

bool CMaterial::HasTexture(CTexture::EType eType) const
{
	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		return m_bHasDiffuseTexture;
	case CMaterial::CTexture::EType::NormalTexture:
		return m_bHasNormalTexture;
	case CMaterial::CTexture::EType::DisplacementTexture:
		return m_bHasDisplacementTexture;
	case CMaterial::CTexture::EType::OpacityTexture:
		return m_bHasOpacityTexture;
	default:
		return false;
	}
}

bool CMaterial::IsTextureEmbedded(CTexture::EType eType) const
{
	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		return (m_vEmbeddedDiffuseTextureRawData.size()) ? true : false;
	case CMaterial::CTexture::EType::NormalTexture:
		return (m_vEmbeddedNormalTextureRawData.size()) ? true : false;
	case CMaterial::CTexture::EType::DisplacementTexture:
		return (m_vEmbeddedDisplacementTextureRawData.size()) ? true : false;
	case CMaterial::CTexture::EType::OpacityTexture:
		return (m_vEmbeddedOpacityTextureRawData.size()) ? true : false;
	default:
		return false;
	}
}

const string& CMaterial::GetTextureFileName(CTexture::EType eType) const
{
	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		return m_DiffuseTextureFileName;
	case CMaterial::CTexture::EType::NormalTexture:
		return m_NormalTextureFileName;
	case CMaterial::CTexture::EType::DisplacementTexture:
		return m_DisplacementTextureFileName;
	case CMaterial::CTexture::EType::OpacityTexture:
		return m_OpacityTextureFileName;
	default:
		return m_DiffuseTextureFileName;
	}
}

const vector<uint8_t>& CMaterial::GetTextureRawData(CTexture::EType eType) const
{
	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		return m_vEmbeddedDiffuseTextureRawData;
	case CMaterial::CTexture::EType::NormalTexture:
		return m_vEmbeddedNormalTextureRawData;
	case CMaterial::CTexture::EType::DisplacementTexture:
		return m_vEmbeddedDisplacementTextureRawData;
	case CMaterial::CTexture::EType::OpacityTexture:
		return m_vEmbeddedOpacityTextureRawData;
	default:
		return m_vEmbeddedDiffuseTextureRawData;
	}
}
