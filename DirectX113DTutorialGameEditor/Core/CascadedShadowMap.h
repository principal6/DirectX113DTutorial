#pragma once

#include "SharedHeader.h"
#include "ShadowMapFrustum.h"

static constexpr size_t KCascadedShadowMapLODCountMax{ 5 };

// Cascaded shadow map for directional light (orthogonal projection)
class CCascadedShadowMap final
{
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
	CCascadedShadowMap(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CCascadedShadowMap() {}

public:
	void Create(size_t LODCount, const XMFLOAT2& MapSize);

public:
	void SetZFar(size_t LOD, float ZFar);
	float GetZNear(size_t LOD) const;
	float GetZFar(size_t LOD) const;

public:
	void Set(size_t LOD, const XMMATRIX& Projection, const XMVECTOR& EyePosition, const XMVECTOR& ViewDirection, const XMVECTOR& DirectionToLight);

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
	std::vector<float>				m_vZFars{};
};
