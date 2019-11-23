#include "CubemapRep.h"
#include "Game.h"

using std::make_unique;

void CCubemapRep::UnfoldCubemap(ID3D11ShaderResourceView* const CubemapSRV, 
	ID3D11RenderTargetView** const CurrentRenderTargetView, ID3D11DepthStencilView* const CurrentDepthStencilView)
{
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
		D3D11_SHADER_RESOURCE_VIEW_DESC ScreenQuadSRVDesc{};
		ScreenQuadSRVDesc.Format = m_TextureDesc.Format;
		ScreenQuadSRVDesc.Texture2D.MipLevels = 1;
		ScreenQuadSRVDesc.Texture2D.MostDetailedMip = 0;
		ScreenQuadSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_PtrDevice->CreateShaderResourceView(m_Texture.Get(), &ScreenQuadSRVDesc, m_SRV.ReleaseAndGetAddressOf());
	}

	if (!m_RTV)
	{
		D3D11_RENDER_TARGET_VIEW_DESC ScreenQuadRTVDesc{};
		ScreenQuadRTVDesc.Format = m_TextureDesc.Format;
		ScreenQuadRTVDesc.Texture2D.MipSlice = 0;
		ScreenQuadRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		m_PtrDevice->CreateRenderTargetView(m_Texture.Get(), &ScreenQuadRTVDesc, m_RTV.ReleaseAndGetAddressOf());
	}

	if (!m_CubemapRepresentation)
	{
		m_CubemapRepresentation = make_unique<CObject2D>("Cubemap", m_PtrDevice, m_PtrDeviceContext);
		m_CubemapRepresentation->Create(Generate2DCubemapRepresentation(XMFLOAT2(KUnitSize, KUnitSize)));
	}

	ID3D11SamplerState* LinearClamp{ m_PtrGame->GetCommonStates()->LinearClamp() };
	m_PtrDeviceContext->PSSetSamplers(0, 1, &LinearClamp);

	D3D11_VIEWPORT Viewport{};
	Viewport.Width = KRepWidth;
	Viewport.Height = KRepHeight;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	m_PtrDeviceContext->RSSetViewports(1, &Viewport);

	m_PtrDeviceContext->OMSetRenderTargets(1, m_RTV.GetAddressOf(), nullptr);
	m_PtrDeviceContext->ClearRenderTargetView(m_RTV.Get(), Colors::Transparent);

	m_PtrGame->GetBaseShader(CGame::EBaseShader::VSBase2D)->Use();
	m_PtrGame->GetBaseShader(CGame::EBaseShader::PSCubemap2D)->Use();
	
	m_PtrDeviceContext->PSSetShaderResources(0, 1, &CubemapSRV);
	m_CubemapRepresentation->Draw(true);

	m_PtrDeviceContext->OMSetRenderTargets(1, CurrentRenderTargetView, CurrentDepthStencilView);
}

ID3D11ShaderResourceView* CCubemapRep::GetSRV() const
{
	return m_SRV.Get();
}
