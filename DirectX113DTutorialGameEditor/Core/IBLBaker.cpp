#include "IBLBaker.h"
#include "Shader.h"
#include "ConstantBuffer.h"
#include "FullScreenQuad.h"
#include "Material.h"

using std::make_unique;

CIBLBaker::CIBLBaker(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CIBLBaker::~CIBLBaker()
{
}

void CIBLBaker::Create()
{
	bool bShouldCompileShaders{ false };

	{
		m_PSFromHDR = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
		m_PSFromHDR->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders,
			L"Shader\\PSFromHDR.hlsl", "main");
	}
	
	{
		m_CBIrradianceGenerator = make_unique<CConstantBuffer>(m_PtrDevice, m_PtrDeviceContext,
			&m_CBIrradianceGeneratorData, sizeof(m_CBIrradianceGeneratorData));
		m_CBIrradianceGenerator->Create();

		m_PSIrradianceGenerator = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
		m_PSIrradianceGenerator->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders,
			L"Shader\\PSIrradianceGenerator.hlsl", "main");
		m_PSIrradianceGenerator->AttachConstantBuffer(m_CBIrradianceGenerator.get(), KPSSharedCBCount + 0);
	}

	{
		m_CBRadiancePrefiltering = make_unique<CConstantBuffer>(m_PtrDevice, m_PtrDeviceContext,
			&m_CBRadiancePrefilteringData, sizeof(m_CBRadiancePrefilteringData));
		m_CBRadiancePrefiltering->Create();

		m_PSRadiancePrefiltering = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
		m_PSRadiancePrefiltering->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders,
			L"Shader\\PSRadiancePrefiltering.hlsl", "main");
		m_PSRadiancePrefiltering->AttachConstantBuffer(m_CBRadiancePrefiltering.get(), KPSSharedCBCount + 0);
	}

	{
		m_PSBRDFIntegrator = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
		m_PSBRDFIntegrator->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, 
			L"Shader\\PSBRDFIntegrator.hlsl", "main");
	}
}

void CIBLBaker::ConvertHDRiToCubemap(const XMFLOAT2& HDRiSize, CTexture* const HDRiTexture)
{
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.Width = static_cast<UINT>(HDRiSize.x * 0.25f);
	Texture2DDesc.Height = static_cast<UINT>(HDRiSize.y * 0.5f);
	Texture2DDesc.ArraySize = 6;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	Texture2DDesc.CPUAccessFlags = 0;
	Texture2DDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
	Texture2DDesc.MipLevels = 0;
	Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_CubemapTexture.ReleaseAndGetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
	RTVDesc.Format = Texture2DDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	RTVDesc.Texture2DArray.ArraySize = 1;

	for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
	{
		RTVDesc.Texture2DArray.FirstArraySlice = iCubeFace;
		m_PtrDevice->CreateRenderTargetView(m_CubemapTexture.Get(), &RTVDesc, m_CubemapRTV[iCubeFace].ReleaseAndGetAddressOf());
	}

	m_PtrDevice->CreateShaderResourceView(m_CubemapTexture.Get(), nullptr, m_CubemapSRV.ReleaseAndGetAddressOf());

	m_CubemapFSQ = make_unique<CFullScreenQuad>(m_PtrDevice, m_PtrDeviceContext);
	m_CubemapFSQ->CreateCubemapDrawer();
	m_CubemapFSQ->OverridePixelShader(m_PSFromHDR.get());

	// Draw!
	{
		m_CubemapFSQ->SetIA();
		m_CubemapFSQ->SetShaders();
		m_CubemapFSQ->SetSampler(CFullScreenQuad::ESamplerState::LinearWrap);

		D3D11_VIEWPORT Viewport{};
		Viewport.Width = static_cast<FLOAT>(Texture2DDesc.Width);
		Viewport.Height = static_cast<FLOAT>(Texture2DDesc.Height);
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		m_PtrDeviceContext->RSSetViewports(1, &Viewport);

		HDRiTexture->Use(0);

		for (auto& RTV : m_CubemapRTV)
		{
			if (RTV) m_PtrDeviceContext->ClearRenderTargetView(RTV.Get(), Colors::Blue);
		}

		for (int iCubemapFace = 0; iCubemapFace < 6; ++iCubemapFace)
		{
			m_PtrDeviceContext->OMSetRenderTargets(1, m_CubemapRTV[iCubemapFace].GetAddressOf(), nullptr);

			m_CubemapFSQ->DrawCubemap(iCubemapFace);
		}
	}

	m_PtrDeviceContext->GenerateMips(m_CubemapSRV.Get());
}

void CIBLBaker::GenerateIrradianceMap(CTexture* const EnvironmentMapTexture, uint32_t MipBias, float RangeFactor)
{
	assert(EnvironmentMapTexture);
	XMFLOAT2 Texture2DSize{ EnvironmentMapTexture->GetTextureSize() };

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.Width = static_cast<uint32_t>(Texture2DSize.x / pow(2.0f, static_cast<float>(MipBias)));
	Texture2DDesc.Height = static_cast<uint32_t>(Texture2DSize.y / pow(2.0f, static_cast<float>(MipBias)));
	Texture2DDesc.ArraySize = 6;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	Texture2DDesc.CPUAccessFlags = 0;
	Texture2DDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
	Texture2DDesc.MipLevels = 0;
	Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_CubemapTexture.ReleaseAndGetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
	RTVDesc.Format = Texture2DDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	RTVDesc.Texture2DArray.ArraySize = 1;

	for (int iCubemapFace = 0; iCubemapFace < 6; ++iCubemapFace)
	{
		RTVDesc.Texture2DArray.FirstArraySlice = iCubemapFace;
		m_PtrDevice->CreateRenderTargetView(m_CubemapTexture.Get(), &RTVDesc, m_CubemapRTV[iCubemapFace].ReleaseAndGetAddressOf());
	}

	m_PtrDevice->CreateShaderResourceView(m_CubemapTexture.Get(), nullptr, m_CubemapSRV.ReleaseAndGetAddressOf());

	m_CubemapFSQ = make_unique<CFullScreenQuad>(m_PtrDevice, m_PtrDeviceContext);
	m_CubemapFSQ->CreateCubemapDrawer();
	m_CubemapFSQ->OverridePixelShader(m_PSIrradianceGenerator.get());

	// Draw!
	{
		m_CubemapFSQ->SetIA();
		m_CubemapFSQ->SetSampler(CFullScreenQuad::ESamplerState::LinearWrap);
		m_CubemapFSQ->SetShaders();

		D3D11_VIEWPORT Viewport{};
		Viewport.Width = static_cast<FLOAT>(Texture2DDesc.Width);
		Viewport.Height = static_cast<FLOAT>(Texture2DDesc.Height);
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		m_PtrDeviceContext->RSSetViewports(1, &Viewport);

		EnvironmentMapTexture->Use(0);

		// @important
		m_CBIrradianceGeneratorData.RangeFactor = RangeFactor;
		m_CBIrradianceGenerator->Update();

		for (auto& RTV : m_CubemapRTV)
		{
			if (RTV) m_PtrDeviceContext->ClearRenderTargetView(RTV.Get(), Colors::Blue);
		}

		for (int iCubemapFace = 0; iCubemapFace < 6; ++iCubemapFace)
		{
			m_PtrDeviceContext->OMSetRenderTargets(1, m_CubemapRTV[iCubemapFace].GetAddressOf(), nullptr);

			m_CubemapFSQ->DrawCubemap(iCubemapFace);
		}
	}

	m_PtrDeviceContext->GenerateMips(m_CubemapSRV.Get());
}

void CIBLBaker::GeneratePrefilteredRadianceMap(CTexture* const EnvironmentMapTexture, uint32_t MipBias, float RangeFactor)
{
	assert(EnvironmentMapTexture);
	XMFLOAT2 Texture2DSize{ EnvironmentMapTexture->GetTextureSize() };
	uint32_t Texture2DMipLevels{ EnvironmentMapTexture->GetMipLevels() };
	uint32_t BiasedMipLevelCount{ Texture2DMipLevels - MipBias };

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.Width = static_cast<uint32_t>(Texture2DSize.x / pow(2.0f, static_cast<float>(MipBias)));
	Texture2DDesc.Height = static_cast<uint32_t>(Texture2DSize.y / pow(2.0f, static_cast<float>(MipBias)));
	Texture2DDesc.ArraySize = 6;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	Texture2DDesc.CPUAccessFlags = 0;
	Texture2DDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
	Texture2DDesc.MipLevels = BiasedMipLevelCount;
	Texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_CubemapTexture.ReleaseAndGetAddressOf());

	m_PtrDevice->CreateShaderResourceView(m_CubemapTexture.Get(), nullptr, m_CubemapSRV.ReleaseAndGetAddressOf());

	m_CubemapFSQ = make_unique<CFullScreenQuad>(m_PtrDevice, m_PtrDeviceContext);
	m_CubemapFSQ->CreateCubemapDrawer();
	m_CubemapFSQ->OverridePixelShader(m_PSRadiancePrefiltering.get());

	m_CubemapFSQ->SetIA();
	m_CubemapFSQ->SetSampler(CFullScreenQuad::ESamplerState::LinearWrap);
	m_CubemapFSQ->SetShaders();

	EnvironmentMapTexture->Use(0);

	// Draw
	{
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
		RTVDesc.Format = Texture2DDesc.Format;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.ArraySize = 1;

		for (uint32_t iMipLevel = 0; iMipLevel < BiasedMipLevelCount; ++iMipLevel)
		{
			// @important
			float Roughness{ (float)(iMipLevel) / (float)(BiasedMipLevelCount - 1) };
			m_CBRadiancePrefilteringData.Roughness = Roughness;
			m_CBRadiancePrefilteringData.RangeFactor = RangeFactor;
			m_CBRadiancePrefiltering->Update();

			D3D11_VIEWPORT Viewport{};
			Viewport.Width = static_cast<FLOAT>(Texture2DDesc.Width / (uint32_t)pow(2.0f, (float)iMipLevel));
			Viewport.Height = static_cast<FLOAT>(Texture2DDesc.Height / (uint32_t)pow(2.0f, (float)iMipLevel));
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.MaxDepth = 1.0f;
			m_PtrDeviceContext->RSSetViewports(1, &Viewport);

			for (int iCubemapFace = 0; iCubemapFace < 6; ++iCubemapFace)
			{
				RTVDesc.Texture2DArray.MipSlice = iMipLevel;
				RTVDesc.Texture2DArray.FirstArraySlice = iCubemapFace;
				m_PtrDevice->CreateRenderTargetView(m_CubemapTexture.Get(), &RTVDesc, m_CubemapRTV[iCubemapFace].ReleaseAndGetAddressOf());
			}

			for (auto& RTV : m_CubemapRTV)
			{
				if (RTV) m_PtrDeviceContext->ClearRenderTargetView(RTV.Get(), Colors::Blue);
			}

			for (int iCubemapFace = 0; iCubemapFace < 6; ++iCubemapFace)
			{
				m_PtrDeviceContext->OMSetRenderTargets(1, m_CubemapRTV[iCubemapFace].GetAddressOf(), nullptr);

				m_CubemapFSQ->DrawCubemap(iCubemapFace);
			}
		}
	}
}

void CIBLBaker::GenerateIntegratedBRDF(const XMFLOAT2& ResultTextureSize)
{
	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.Width = static_cast<UINT>(ResultTextureSize.x);
	Texture2DDesc.Height = static_cast<UINT>(ResultTextureSize.y);
	Texture2DDesc.ArraySize = 1;
	Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	Texture2DDesc.CPUAccessFlags = 0;
	Texture2DDesc.Format = DXGI_FORMAT_R16G16_UNORM;
	Texture2DDesc.MipLevels = 0;
	Texture2DDesc.MiscFlags = 0;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
	m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_CubemapTexture.ReleaseAndGetAddressOf());

	m_PtrDevice->CreateShaderResourceView(m_CubemapTexture.Get(), nullptr, m_CubemapSRV.ReleaseAndGetAddressOf());

	m_CubemapFSQ = make_unique<CFullScreenQuad>(m_PtrDevice, m_PtrDeviceContext);
	m_CubemapFSQ->Create2DDrawer(CFullScreenQuad::EPixelShaderPass::AllChannels);
	m_CubemapFSQ->OverridePixelShader(m_PSBRDFIntegrator.get());

	{
		m_CubemapFSQ->SetIA();
		m_CubemapFSQ->SetSampler(CFullScreenQuad::ESamplerState::LinearWrap);
		m_CubemapFSQ->SetShaders();

		D3D11_VIEWPORT Viewport{};
		Viewport.Width = ResultTextureSize.x;
		Viewport.Height = ResultTextureSize.y;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		m_PtrDeviceContext->RSSetViewports(1, &Viewport);

		m_PtrDevice->CreateRenderTargetView(m_CubemapTexture.Get(), nullptr, m_CubemapRTV[0].ReleaseAndGetAddressOf());
		m_PtrDeviceContext->ClearRenderTargetView(m_CubemapRTV[0].Get(), Colors::Blue);

		m_PtrDeviceContext->OMSetRenderTargets(1, m_CubemapRTV[0].GetAddressOf(), nullptr);

		m_CubemapFSQ->Draw2D();
	}
}

ID3D11Texture2D* CIBLBaker::GetBakedTexture() const
{
	return m_CubemapTexture.Get();
}
