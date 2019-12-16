#pragma once

#include "SharedHeader.h"

class CObject2D;
class CShader;

// 2D representation of cube-map texture
class CCubemapRep final
{
public:
	CCubemapRep(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CCubemapRep();

public:
	void Create(uint32_t VSSharedCBCount, uint32_t PSSharedCBCount);

private:
	void CreateShaders(uint32_t VSSharedCBCount, uint32_t PSSharedCBCount);

public:
	void UnfoldCubemap(ID3D11ShaderResourceView* const CubemapSRV);

public:
	ID3D11ShaderResourceView* GetSRV() const;

public:
	static constexpr float KUnitSize{ 75.0f };
	static constexpr float KRepWidth{ KUnitSize * 4 };
	static constexpr float KRepHeight{ KUnitSize * 3 };

private:
	ID3D11Device* const					m_PtrDevice{};
	ID3D11DeviceContext* const			m_PtrDeviceContext{};

private:
	std::unique_ptr<CObject2D>			m_CubemapRepresentation{};
	std::unique_ptr<CommonStates>		m_CommonStates{};
	std::unique_ptr<CShader>			m_VSBase2D{};
	std::unique_ptr<CShader>			m_PSCubemap2D{};

private:
	ComPtr<ID3D11RenderTargetView>		m_RTV{};
	ComPtr<ID3D11ShaderResourceView>	m_SRV{};
	ComPtr<ID3D11Texture2D>				m_Texture{};
	D3D11_TEXTURE2D_DESC				m_TextureDesc{};
	D3D11_VIEWPORT						m_Viewport{};
};