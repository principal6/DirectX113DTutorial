#pragma once

#include "SharedHeader.h"

struct alignas(4) SPixelUNorm
{
	uint8_t R{};
	uint8_t G{};
	uint8_t B{};
	uint8_t A{};
};

class CMaterialTexture
{
public:
	CMaterialTexture(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CMaterialTexture() {}

public:
	void CreateTextureFromFile(const string& TextureFileName, bool bShouldGenerateMipMap);
	void CreateTextureFromMemory(const vector<uint8_t>& RawData);
	void CreateBlankTexture(DXGI_FORMAT Format, const XMFLOAT2& TextureSize);

private:
	void SetTextureSize();

public:
	void UpdateTextureRawData(const SPixelUNorm* PtrData);
	void SetSlot(UINT Slot);
	void Use(int ForcedSlot = -1) const;

public:
	const string& GetFileName() const { return m_TextureFileName; }
	ID3D11ShaderResourceView* GetShaderResourceViewPtr() { return m_ShaderResourceView.Get(); }

private:
	ID3D11Device*						m_PtrDevice{};
	ID3D11DeviceContext*				m_PtrDeviceContext{};

private:
	string								m_TextureFileName{};
	XMFLOAT2							m_TextureSize{};
	UINT								m_Slot{};

private:
	ComPtr<ID3D11Texture2D>				m_Texture2D{};
	ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
};

class CMaterial
{
public:
	CMaterial() {}
	~CMaterial() {}

public:
	void SetTextureRawData(const vector<uint8_t>& Data);
	void SetTextureFileName(const string& FileName, bool bShouldGenerateAutoMipMap);

public:
	void SetUniformColor(const XMFLOAT3& Color);
	void SetAmbientColor(const XMFLOAT3& Color);
	void SetDiffuseColor(const XMFLOAT3& Color);
	void SetSpecularColor(const XMFLOAT3& Color);
	void SetSpecularExponent(float Exponent);
	void SetSpecularIntensity(float Intensity);

public:
	bool HasTexture() const { return bHasTexture; }
	bool HasEmbededdTexture() const { return (vEmbeddedTextureRawData.size()) ? true : false; }
	const string& GetTextureFileName() const { return TextureFileName; }
	const vector<uint8_t>& GetTextureRawData() const { return vEmbeddedTextureRawData; }
	bool ShouldGenerateAutoMipMap() const { return m_bShouldGenerateAutoMipMap; }

	const XMFLOAT3& GetAmbient() const { return MaterialAmbient; }
	const XMFLOAT3& GetDiffuse() const { return MaterialDiffuse; }
	const XMFLOAT3& GetSpecular() const { return MaterialSpecular; }
	float GetSpecularIntensity() const { return SpecularIntensity; }
	float GetSpecularExponent() const { return SpecularExponent; }

private:
	XMFLOAT3				MaterialAmbient{};
	XMFLOAT3				MaterialDiffuse{};
	XMFLOAT3				MaterialSpecular{};
	float					SpecularExponent{ 1.0f };
	float					SpecularIntensity{ 0.0f };

	bool					bHasTexture{ false };
	string					TextureFileName{};

	// This will be cleared after loading the texture into CMaterialTexture
	vector<uint8_t>			vEmbeddedTextureRawData{};

	bool					m_bShouldGenerateAutoMipMap{};
};