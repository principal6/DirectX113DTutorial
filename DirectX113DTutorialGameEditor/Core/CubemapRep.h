#pragma once

#include "SharedHeader.h"
#include "Object2D.h"

class CGame;

// 2D representation of cube-map texture
class CCubemapRep final
{
public:
	CCubemapRep(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, CGame* const PtrGame) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGame{ PtrGame }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
		assert(m_PtrGame);
	}
	~CCubemapRep() {}

public:
	void UnfoldCubemap(ID3D11ShaderResourceView* const CubemapSRV);

	ID3D11ShaderResourceView* GetSRV() const;

public:
	static constexpr float KUnitSize{ 75.0f };
	static constexpr float KRepWidth{ KUnitSize * 4 };
	static constexpr float KRepHeight{ KUnitSize * 3 };

private:
	ID3D11Device* const					m_PtrDevice{};
	ID3D11DeviceContext* const			m_PtrDeviceContext{};
	CGame* const						m_PtrGame{};

private:
	std::unique_ptr<CObject2D>			m_CubemapRepresentation{};

private:
	ComPtr<ID3D11RenderTargetView>		m_RTV{};
	ComPtr<ID3D11ShaderResourceView>	m_SRV{};
	ComPtr<ID3D11Texture2D>				m_Texture{};
	D3D11_TEXTURE2D_DESC				m_TextureDesc{};
};