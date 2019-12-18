#pragma once

#include "SharedHeader.h"

class CTexture;
class CShader;
class CConstantBuffer;
class CFullScreenQuad;

class CIBLBaker final
{
	struct SCBIrradianceGenerator
	{
		float RangeFactor{};
		float Pads[3]{};
	};

	struct SCBRadiancePrefiltering
	{
		float	Roughness{};
		float	RangeFactor{};
		float	Pads[2]{};
	};

public:
	CIBLBaker(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CIBLBaker();

public:
	void Create();

public:
	void ConvertHDRiToCubemap(const XMFLOAT2& HDRiSize, CTexture* const HDRiTexture);
	void GenerateIrradianceMap(CTexture* const EnvironmentMapTexture, uint32_t MipBias, float RangeFactor);
	void GeneratePrefilteredRadianceMap(CTexture* const EnvironmentMapTexture, uint32_t MipBias, float RangeFactor);
	void GenerateIntegratedBRDF(const XMFLOAT2& ResultTextureSize);

public:
	ID3D11Texture2D* GetBakedTexture() const;

private:
	ID3D11Device* const					m_PtrDevice{};
	ID3D11DeviceContext* const			m_PtrDeviceContext{};

private:
	ComPtr<ID3D11Texture2D>				m_CubemapTexture{};
	ComPtr<ID3D11ShaderResourceView>	m_CubemapSRV{};
	ComPtr<ID3D11RenderTargetView>		m_CubemapRTV[6]{};
	std::unique_ptr<CFullScreenQuad>	m_CubemapFSQ{};

// HDRi to cubemap
private:
	std::unique_ptr<CShader>			m_PSFromHDR{};

// Irradiance
private:
	std::unique_ptr<CShader>			m_PSIrradianceGenerator{};
	std::unique_ptr<CConstantBuffer>	m_CBIrradianceGenerator{};
	SCBIrradianceGenerator				m_CBIrradianceGeneratorData{};

// Prefiltered radiance
private:
	std::unique_ptr<CShader>			m_PSRadiancePrefiltering{};
	std::unique_ptr<CConstantBuffer>	m_CBRadiancePrefiltering{};
	SCBRadiancePrefiltering				m_CBRadiancePrefilteringData{};

// Integrated BRDF
private:
	std::unique_ptr<CShader>			m_PSBRDFIntegrator{};
};