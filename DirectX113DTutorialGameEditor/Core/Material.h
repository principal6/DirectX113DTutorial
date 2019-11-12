#pragma once

#include "SharedHeader.h"

struct SPixel8UInt
{
	uint8_t R{};
};

struct SPixel32UInt
{
	uint8_t R{};
	uint8_t G{};
	uint8_t B{};
	uint8_t A{};
};

struct alignas(4) SPixel128Float
{
	float R{};
	float G{};
	float B{};
	float A{};
};

static constexpr int KMaxTextureCountPerMaterial{ 4 };

struct STextureData
{
	enum class EType
	{
		DiffuseTexture,
		NormalTexture,
		OpacityTexture,
		DisplacementTexture
	};

	bool					bHasTexture{ false };
	std::string				FileName{};
	std::vector<uint8_t>	vRawData{};
};

class CMaterialData
{
public:
	CMaterialData() {}
	~CMaterialData() {}

	void Name(const std::string& Name);
	const std::string& Name() const;

	void Index(size_t Value);
	size_t Index() const;

	void AmbientColor(const XMFLOAT3& Color);
	const XMFLOAT3& AmbientColor() const;

	void DiffuseColor(const XMFLOAT3& Color);
	const XMFLOAT3& DiffuseColor() const;

	void SpecularColor(const XMFLOAT3& Color);
	const XMFLOAT3& SpecularColor() const;

	void SpecularExponent(float Value);
	float SpecularExponent() const;

	void SpecularIntensity(float Value);
	float SpecularIntensity() const;

	STextureData& GetTextureData(STextureData::EType eType);
	void SetTextureFileName(STextureData::EType eType, const std::string& FileName);
	const std::string GetTextureFileName(STextureData::EType eType) const;

	void ShouldGenerateMipMap(bool Value);
	bool ShouldGenerateMipMap() const;

public:
	void HasAnyTexture(bool Value);
	bool HasAnyTexture() const;
	bool HasTexture(STextureData::EType eType) const { return m_TextureData[(int)eType].bHasTexture; }

public:
	void SetUniformColor(const XMFLOAT3& Color);

private:
	XMFLOAT3		m_AmbientColor{};
	XMFLOAT3		m_DiffuseColor{};
	XMFLOAT3		m_SpecularColor{};
	float			m_SpecularExponent{ 1.0f }; // Shininess
	float			m_SpecularIntensity{ 0.2f }; // Shininess strength

	bool			m_bHasAnyTexture{ false };
	STextureData	m_TextureData[KMaxTextureCountPerMaterial]{};

	bool			m_bShouldGenerateMipMap{ true };

private:
	std::string		m_Name{};
	size_t			m_Index{};
};

class CTexture
{
public:
	enum class EFormat
	{
		Pixel8Int = DXGI_FORMAT_R8_UNORM,
		Pixel32Int = DXGI_FORMAT_R8G8B8A8_UNORM,
		Pixel128Float = DXGI_FORMAT_R32G32B32A32_FLOAT
	};

public:
	CTexture(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CTexture() {}

public:
	void CreateTextureFromFile(const std::string& FileName, bool bShouldGenerateMipMap);
	void CreateTextureFromMemory(const std::vector<uint8_t>& RawData, bool bShouldGenerateMipMap);
	void CreateBlankTexture(EFormat Format, const XMFLOAT2& TextureSize);

	void SaveToDDSFile(const std::string& FileName);

private:
	void UpdateTextureSize();

public:
	void UpdateTextureRawData(const SPixel8UInt* const PtrData);
	void UpdateTextureRawData(const SPixel32UInt* const PtrData);
	void UpdateTextureRawData(const SPixel128Float* const PtrData);
	void SetSlot(UINT Slot);
	void SetShaderType(EShaderType eShaderType);
	void Use(int ForcedSlot = -1) const;

public:
	bool IsCreated() const { return m_bIsCreated; }
	const std::string& GetFileName() const { return m_FileName; }
	const XMFLOAT2& GetTextureSize() const { return m_TextureSize; }
	ID3D11Texture2D* GetTexture2DPtr() const { return m_Texture2D.Get(); }
	ID3D11ShaderResourceView* GetShaderResourceViewPtr() { return m_ShaderResourceView.Get(); }

private:
	ID3D11Device* const					m_PtrDevice{};
	ID3D11DeviceContext* const			m_PtrDeviceContext{};

private:
	std::string							m_FileName{};
	XMFLOAT2							m_TextureSize{};
	UINT								m_Slot{};
	EShaderType							m_eShaderType{ EShaderType::PixelShader };
	bool								m_bIsCreated{ false };

private:
	ComPtr<ID3D11Texture2D>				m_Texture2D{};
	ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
};

class CMaterialTextureSet
{
public:
	CMaterialTextureSet(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CMaterialTextureSet() {}

public:
	void CreateTextures(CMaterialData& MaterialData);
	void UseTextures() const;

public:
	ID3D11ShaderResourceView* GetTextureSRV(STextureData::EType eType);

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};

private:
	CTexture					m_Textures[KMaxTextureCountPerMaterial]
	{
		{ m_PtrDevice, m_PtrDeviceContext },
		{ m_PtrDevice, m_PtrDeviceContext },
		{ m_PtrDevice, m_PtrDeviceContext },
		{ m_PtrDevice, m_PtrDeviceContext }
	};
};