#include "CubemapRep.h"
#include "Object2D.h"
#include "Shader.h"
#include "PrimitiveGenerator.h"

using std::make_unique;

CCubemapRep::CCubemapRep(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CCubemapRep::~CCubemapRep()
{
}

void CCubemapRep::Create(uint32_t VSSharedCBCount, uint32_t PSSharedCBCount)
{
	m_Viewport.Width = KRepWidth;
	m_Viewport.Height = KRepHeight;
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	if (!m_Texture)
	{
		m_TextureDesc.ArraySize = 1;
		m_TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		m_TextureDesc.CPUAccessFlags = 0;
		m_TextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
		m_TextureDesc.Height = static_cast<UINT>(KRepHeight);
		m_TextureDesc.MipLevels = 1;
		m_TextureDesc.SampleDesc.Count = 1;
		m_TextureDesc.SampleDesc.Quality = 0;
		m_TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		m_TextureDesc.Width = static_cast<UINT>(KRepWidth);
		m_PtrDevice->CreateTexture2D(&m_TextureDesc, nullptr, m_Texture.ReleaseAndGetAddressOf());
	}

	if (!m_SRV)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = m_TextureDesc.Format;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_PtrDevice->CreateShaderResourceView(m_Texture.Get(), &SRVDesc, m_SRV.ReleaseAndGetAddressOf());
	}

	if (!m_RTV)
	{
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
		RTVDesc.Format = m_TextureDesc.Format;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		m_PtrDevice->CreateRenderTargetView(m_Texture.Get(), &RTVDesc, m_RTV.ReleaseAndGetAddressOf());
	}

	if (!m_CubemapRepresentation)
	{
		m_CubemapRepresentation = make_unique<CObject2D>("Cubemap", m_PtrDevice, m_PtrDeviceContext);
		m_CubemapRepresentation->Create(Generate2DCubemapRepresentation(XMFLOAT2(KUnitSize, KUnitSize)));
	}

	if (!m_CommonStates)
	{
		m_CommonStates = make_unique<CommonStates>(m_PtrDevice);
	}

	CreateShaders(VSSharedCBCount, PSSharedCBCount);
}

void CCubemapRep::CreateShaders(uint32_t VSSharedCBCount, uint32_t PSSharedCBCount)
{
	bool bShouldCompileShaders{ false };

	if (!m_VSBase2D)
	{
		m_VSBase2D = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
		m_VSBase2D->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSBase2D.hlsl", "main",
			CObject2D::KInputLayout, ARRAYSIZE(CObject2D::KInputLayout));
	}
	
	if (!m_PSCubemap2D)
	{
		m_PSCubemap2D = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
		m_PSCubemap2D->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSCubemap2D.hlsl", "main");
	}
}

void CCubemapRep::UnfoldCubemap(ID3D11ShaderResourceView* const CubemapSRV)
{
	ID3D11SamplerState* SamplerStates{ m_CommonStates->LinearClamp() };
	m_PtrDeviceContext->PSSetSamplers(0, 1, &SamplerStates);

	m_PtrDeviceContext->RSSetViewports(1, &m_Viewport);

	m_PtrDeviceContext->OMSetRenderTargets(1, m_RTV.GetAddressOf(), nullptr);
	m_PtrDeviceContext->ClearRenderTargetView(m_RTV.Get(), Colors::Transparent);

	m_VSBase2D->Use();
	m_PSCubemap2D->Use();
	
	m_PtrDeviceContext->PSSetShaderResources(0, 1, &CubemapSRV);
	m_CubemapRepresentation->Draw(true);
}

ID3D11ShaderResourceView* CCubemapRep::GetSRV() const
{
	return m_SRV.Get();
}
