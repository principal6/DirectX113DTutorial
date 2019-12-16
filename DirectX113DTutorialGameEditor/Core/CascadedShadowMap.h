#pragma once

#include "SharedHeader.h"
#include "ShadowMapFrustum.h"

class CObject3DLine;
class CFullScreenQuad;

static constexpr size_t KCascadedShadowMapLODCountMax{ 5 };

// Cascaded shadow map for directional light (orthogonal projection)
class CCascadedShadowMap final
{
public:
	struct SLODData
	{
		SLODData(size_t _LOD, float _ZFar, size_t _UpdateFrameInterval,
			const XMFLOAT2& _TextureViewportPosition = XMFLOAT2(0, 0), const XMFLOAT2& _TextureViewportSize = XMFLOAT2(0, 0)) :
			LOD{ _LOD }, ZFar{ _ZFar }, UpdateFrameInterval{ _UpdateFrameInterval }, 
			TextureViewportPosition{ _TextureViewportPosition }, TextureViewportSize{ _TextureViewportSize } {}

		size_t			LOD{};
		float			ZFar{ FLT_MAX };
		size_t			UpdateFrameInterval{};
		mutable size_t	FrameCounter{};

		XMFLOAT2		TextureViewportPosition{};
		XMFLOAT2		TextureViewportSize{};
	};

private:
	struct SMapData
	{
		ComPtr<ID3D11Texture2D>				Texture{};
		ComPtr<ID3D11DepthStencilView>		DSV{};
		ComPtr<ID3D11ShaderResourceView>	SRV{};
		D3D11_VIEWPORT						Viewport{};
		XMMATRIX							ViewMatrix{};
		XMMATRIX							ProjectionMatrix{};
		XMMATRIX							VPTransposed{};
	};

public:
	CCascadedShadowMap(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CCascadedShadowMap();

public:
	void Create(const std::vector<SLODData>& vLODData, const XMFLOAT2& MapSize);

public:
	void SetZFar(size_t LOD, float ZFar);
	float GetZNear(size_t LOD) const;
	float GetZFar(size_t LOD) const;

public:
	bool ShouldUpdate(size_t LOD) const;
	void Set(size_t LOD, const XMMATRIX& Projection, const XMVECTOR& EyePosition, const XMVECTOR& ViewDirection, const XMVECTOR& DirectionToLight);

public:
	void CaptureFrustums();
	void DrawFrustums();
	void DrawTextures();

public:
	size_t GetLODCount() const;
	ID3D11ShaderResourceView* GetSRV(size_t LOD) const;
	const XMMATRIX& GetViewMatrix(size_t LOD) const;
	const XMMATRIX& GetProjectionMatrix(size_t LOD) const;
	const XMMATRIX& GetTransposedSpaceMatrix(size_t LOD) const;
	const SFrustumVertices& GetViewFrustumVertices(size_t LOD) const;
	const SFrustumVertices& GetShadowMapFrustumVertices(size_t LOD) const;

private:
	ID3D11Device* const				m_PtrDevice{};
	ID3D11DeviceContext* const		m_PtrDeviceContext{};

private:
	std::vector<SMapData>			m_vShadowMaps{};
	std::vector<SShadowMapFrustum>	m_vShadowMapFrustums{};
	std::vector<SFrustumVertices>	m_vShadowMapFrustumVertices{};
	std::vector<SFrustumVertices>	m_vViewFrustumVertices{};
	std::vector<SLODData>			m_vLODData{};

private:
	std::vector<std::unique_ptr<CObject3DLine>>	m_vViewFrustumReps{};
	std::vector<std::unique_ptr<CObject3DLine>>	m_vShadowMapFrustumReps{};
	std::vector<D3D11_VIEWPORT>					m_vViewports{};
	std::unique_ptr<CFullScreenQuad>			m_FullScreenQuad{};
};
