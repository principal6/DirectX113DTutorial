#include "CascadedShadowMap.h"
#include "FullScreenQuad.h"
#include "ShadowMapFrustum.h"
#include "../Model/Object3DLine.h"

using std::make_unique;

CCascadedShadowMap::CCascadedShadowMap(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) : 
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CCascadedShadowMap::~CCascadedShadowMap()
{
}

void CCascadedShadowMap::Create(const std::vector<SLODData>& vLODData, const XMFLOAT2& MapSize)
{
	m_vLODData = vLODData;
	size_t LODCount{ m_vLODData.size() };

	assert(LODCount <= KLODCountMax);
	m_vShadowMaps.resize(LODCount);
	m_vShadowMapFrustums.resize(LODCount);
	m_vShadowMapFrustumVertices.resize(LODCount);
	m_vViewFrustumVertices.resize(LODCount);
	m_vShadowMapFrustumReps.resize(LODCount);
	m_vViewFrustumReps.resize(LODCount);
	m_vViewports.resize(LODCount);

	D3D11_TEXTURE2D_DESC Texture2DDesc{};
	Texture2DDesc.ArraySize = 1;
	Texture2DDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	Texture2DDesc.CPUAccessFlags = 0;
	Texture2DDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // @important
	Texture2DDesc.Width = (UINT)MapSize.x;
	Texture2DDesc.Height = (UINT)MapSize.y;
	Texture2DDesc.MipLevels = 1;
	Texture2DDesc.MiscFlags = 0;
	Texture2DDesc.SampleDesc.Count = 1;
	Texture2DDesc.SampleDesc.Quality = 0;
	Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
	DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	
	for (size_t iLOD = 0; iLOD < LODCount; ++iLOD)
	{
		m_vShadowMaps[iLOD].Viewport.TopLeftX = 0;
		m_vShadowMaps[iLOD].Viewport.TopLeftY = 0;
		m_vShadowMaps[iLOD].Viewport.Width = MapSize.x;
		m_vShadowMaps[iLOD].Viewport.Height = MapSize.y;
		m_vShadowMaps[iLOD].Viewport.MinDepth = 0.0f;
		m_vShadowMaps[iLOD].Viewport.MaxDepth = 1.0f;

		m_PtrDevice->CreateTexture2D(&Texture2DDesc, nullptr, m_vShadowMaps[iLOD].Texture.ReleaseAndGetAddressOf());
		m_PtrDevice->CreateDepthStencilView(m_vShadowMaps[iLOD].Texture.Get(), &DSVDesc, m_vShadowMaps[iLOD].DSV.ReleaseAndGetAddressOf());
		m_PtrDevice->CreateShaderResourceView(m_vShadowMaps[iLOD].Texture.Get(), &SRVDesc, m_vShadowMaps[iLOD].SRV.ReleaseAndGetAddressOf());

		m_vShadowMapFrustumReps[iLOD] = make_unique<CObject3DLine>("ShadowFrustumRep", m_PtrDevice, m_PtrDeviceContext);
		m_vShadowMapFrustumReps[iLOD]->Create(12, XMVectorSet(0, 1 - (0.125f * iLOD), 0.25f, 1));

		m_vViewFrustumReps[iLOD] = make_unique<CObject3DLine>("ViewFrustumRep", m_PtrDevice, m_PtrDeviceContext);
		m_vViewFrustumReps[iLOD]->Create(12, XMVectorSet(1 - (0.125f * iLOD), 1 - (0.25f * iLOD), 0, 1));

		m_vViewports[iLOD].MinDepth = 0.0f;
		m_vViewports[iLOD].MaxDepth = 1.0f;
		m_vViewports[iLOD].TopLeftX = m_vLODData[iLOD].TextureViewportPosition.x;
		m_vViewports[iLOD].TopLeftY = m_vLODData[iLOD].TextureViewportPosition.y;
		m_vViewports[iLOD].Width = m_vLODData[iLOD].TextureViewportSize.x;
		m_vViewports[iLOD].Height = m_vLODData[iLOD].TextureViewportSize.y;
	}

	m_FullScreenQuad = make_unique<CFullScreenQuad>(m_PtrDevice, m_PtrDeviceContext);
	m_FullScreenQuad->Create2DDrawer(CFullScreenQuad::EPixelShaderPass::MonochromeSRV);
}

void CCascadedShadowMap::SetZFar(size_t LOD, float ZFar)
{
	m_vLODData[LOD].ZFar = ZFar;
}

float CCascadedShadowMap::GetZNear(size_t LOD) const
{
	return (LOD == 0) ? 0.1f : m_vLODData[LOD - 1].ZFar;
}

float CCascadedShadowMap::GetZFar(size_t LOD) const
{
	return m_vLODData[LOD].ZFar;
}

bool CCascadedShadowMap::ShouldUpdate(size_t LOD) const
{
	++m_vLODData[LOD].FrameCounter;

	if (m_vLODData[LOD].FrameCounter >= m_vLODData[LOD].UpdateFrameInterval)
	{
		m_vLODData[LOD].FrameCounter = 0;
		return true;
	}
	return false;
}

void CCascadedShadowMap::Set(size_t LOD, const XMMATRIX& Projection, const XMVECTOR& EyePosition,
	const XMVECTOR& ViewDirection, const XMVECTOR& DirectionToLight)
{
	float ZNear{ (LOD == 0) ? 0.1f : m_vLODData[LOD - 1].ZFar };
	float ZFar{ m_vLODData[LOD].ZFar };

	m_PtrDeviceContext->RSSetViewports(1, &m_vShadowMaps[LOD].Viewport);

	m_PtrDeviceContext->OMSetRenderTargets(0, nullptr, m_vShadowMaps[LOD].DSV.Get());
	m_PtrDeviceContext->ClearDepthStencilView(m_vShadowMaps[LOD].DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_vShadowMapFrustums[LOD] = CalculateShadowMapFrustum(Projection, EyePosition, ViewDirection, DirectionToLight, ZNear, ZFar);
	m_vShadowMapFrustumVertices[LOD] = CalculateShadowMapFrustumVertices(DirectionToLight, m_vShadowMapFrustums[LOD]);
	m_vViewFrustumVertices[LOD] = CalculateViewFrustumVertices(Projection, EyePosition, ViewDirection, DirectionToLight, ZNear, ZFar);

	m_vShadowMaps[LOD].ViewMatrix = XMMatrixLookToLH(m_vShadowMapFrustums[LOD].LightPosition, m_vShadowMapFrustums[LOD].LightForward, m_vShadowMapFrustums[LOD].LightUp);
	m_vShadowMaps[LOD].ProjectionMatrix = XMMatrixOrthographicOffCenterLH(
		-m_vShadowMapFrustums[LOD].HalfSize.x, +m_vShadowMapFrustums[LOD].HalfSize.x, 
		-m_vShadowMapFrustums[LOD].HalfSize.y, +m_vShadowMapFrustums[LOD].HalfSize.y,
		0.0f, m_vShadowMapFrustums[LOD].HalfSize.z * 2.0f);

	m_vShadowMaps[LOD].VPTransposed = XMMatrixTranspose(m_vShadowMaps[LOD].ViewMatrix * m_vShadowMaps[LOD].ProjectionMatrix);
}

void CCascadedShadowMap::CaptureFrustums()
{
	size_t LODCount{ GetLODCount() };

	// View frustums
	for (size_t iLOD = 0; iLOD < LODCount; ++iLOD)
	{
		const auto& Vertices{ GetViewFrustumVertices(iLOD).Vertices };

		// Near plane
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(0, Vertices[0], Vertices[1]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(1, Vertices[1], Vertices[3]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(2, Vertices[3], Vertices[2]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(3, Vertices[2], Vertices[0]);

		// Far plane
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(4, Vertices[4], Vertices[5]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(5, Vertices[5], Vertices[7]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(6, Vertices[7], Vertices[6]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(7, Vertices[6], Vertices[4]);

		// Side
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(8, Vertices[0], Vertices[4]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(9, Vertices[1], Vertices[5]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(10, Vertices[2], Vertices[6]);
		m_vViewFrustumReps[iLOD]->UpdateLinePosition(11, Vertices[3], Vertices[7]);

		m_vViewFrustumReps[iLOD]->UpdateVertexBuffer();
	}

	// Shadow map frustums
	for (size_t iLOD = 0; iLOD < LODCount; ++iLOD)
	{
		const auto& Vertices{ GetShadowMapFrustumVertices(iLOD).Vertices };

		// Near plane
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(0, Vertices[0], Vertices[1]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(1, Vertices[1], Vertices[3]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(2, Vertices[3], Vertices[2]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(3, Vertices[2], Vertices[0]);

		// Far plane
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(4, Vertices[4], Vertices[5]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(5, Vertices[5], Vertices[7]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(6, Vertices[7], Vertices[6]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(7, Vertices[6], Vertices[4]);

		// Side
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(8, Vertices[0], Vertices[4]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(9, Vertices[1], Vertices[5]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(10, Vertices[2], Vertices[6]);
		m_vShadowMapFrustumReps[iLOD]->UpdateLinePosition(11, Vertices[3], Vertices[7]);

		m_vShadowMapFrustumReps[iLOD]->UpdateVertexBuffer();
	}
}

void CCascadedShadowMap::DrawFrustums()
{
	for (const auto& ViewFrustum : m_vViewFrustumReps)
	{
		if (ViewFrustum) ViewFrustum->Draw();
	}

	for (const auto& ShadowMapFrustum : m_vShadowMapFrustumReps)
	{
		if (ShadowMapFrustum) ShadowMapFrustum->Draw();
	}
}

void CCascadedShadowMap::DrawTextures()
{
	m_FullScreenQuad->SetIA();
	m_FullScreenQuad->SetShaders();
	m_FullScreenQuad->SetSampler(CFullScreenQuad::ESamplerState::PointClamp);

	for (size_t iLOD = 0; iLOD < GetLODCount(); ++iLOD)
	{
		m_PtrDeviceContext->RSSetViewports(1, &m_vViewports[iLOD]);

		m_PtrDeviceContext->PSSetShaderResources(0, 1, m_vShadowMaps[iLOD].SRV.GetAddressOf());

		m_FullScreenQuad->Draw2D();
	}
}

size_t CCascadedShadowMap::GetLODCount() const
{
	return m_vLODData.size();
}

ID3D11ShaderResourceView* CCascadedShadowMap::GetSRV(size_t LOD) const
{
	if (m_vShadowMaps.size() <= LOD) return nullptr;
	return m_vShadowMaps[LOD].SRV.Get();
}

const XMMATRIX& CCascadedShadowMap::GetViewMatrix(size_t LOD) const
{
	return m_vShadowMaps[LOD].ViewMatrix;
}

const XMMATRIX& CCascadedShadowMap::GetProjectionMatrix(size_t LOD) const
{
	return m_vShadowMaps[LOD].ProjectionMatrix;
}

const XMMATRIX& CCascadedShadowMap::GetTransposedSpaceMatrix(size_t LOD) const
{
	return m_vShadowMaps[LOD].VPTransposed;
}

const SFrustumVertices& CCascadedShadowMap::GetViewFrustumVertices(size_t LOD) const
{
	return m_vViewFrustumVertices[LOD];
}

const SFrustumVertices& CCascadedShadowMap::GetShadowMapFrustumVertices(size_t LOD) const
{
	return m_vShadowMapFrustumVertices[LOD];
}
