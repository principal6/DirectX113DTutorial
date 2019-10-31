#pragma once

#include "SharedHeader.h"

struct SPixel8UInt
{
	uint8_t R{};
};

struct alignas(4) SPixel32UInt
{
	uint8_t R{};
	uint8_t G{};
	uint8_t B{};
	uint8_t A{};
};

class CMaterial
{
public:
	class CTexture
	{
	public:
		enum class EType
		{
			DiffuseTexture,
			NormalTexture,
			DisplacementTexture,
			OpacityTexture
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
		void CreateTextureFromFile(const string& TextureFileName, bool bShouldGenerateMipMap);
		void CreateTextureFromMemory(const vector<uint8_t>& RawData);
		void CreateBlankTexture(DXGI_FORMAT Format, const XMFLOAT2& TextureSize);

	private:
		void SetTextureSize();

	public:
		void UpdateTextureRawData(const SPixel8UInt* const PtrData);
		void UpdateTextureRawData(const SPixel32UInt* const PtrData);
		void SetSlot(UINT Slot);
		void SetShaderType(EShaderType eShaderType);
		void Use(int ForcedSlot = -1) const;

	public:
		const string& GetFileName() const { return m_TextureFileName; }
		ID3D11ShaderResourceView* GetShaderResourceViewPtr() { return m_ShaderResourceView.Get(); }

	private:
		ID3D11Device* const					m_PtrDevice{};
		ID3D11DeviceContext* const			m_PtrDeviceContext{};

	private:
		string								m_TextureFileName{};
		XMFLOAT2							m_TextureSize{};
		UINT								m_Slot{};
		EShaderType							m_eShaderType{ EShaderType::PixelShader };

	private:
		ComPtr<ID3D11Texture2D>				m_Texture2D{};
		ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
	};

public:
	CMaterial() {}
	CMaterial(const CMaterial& b)
	{
		*this = b;
	}
	~CMaterial() {}
	
public:
	CMaterial& operator=(const CMaterial& b)
	{
		m_Name = b.m_Name;
		m_Index = b.m_Index;

		m_MaterialAmbient = b.m_MaterialAmbient;
		m_MaterialDiffuse = b.m_MaterialDiffuse;
		m_MaterialSpecular = b.m_MaterialSpecular;
		m_SpecularExponent = b.m_SpecularExponent;
		m_SpecularIntensity = b.m_SpecularIntensity;

		m_bHasTexture = b.m_bHasTexture;
		
		m_bHasDiffuseTexture = b.m_bHasDiffuseTexture;
		m_DiffuseTextureFileName = b.m_DiffuseTextureFileName;
		m_vEmbeddedDiffuseTextureRawData = b.m_vEmbeddedDiffuseTextureRawData;
		
		m_bHasNormalTexture = b.m_bHasNormalTexture;
		m_NormalTextureFileName = b.m_NormalTextureFileName;
		m_vEmbeddedNormalTextureRawData = b.m_vEmbeddedNormalTextureRawData;
		
		m_bHasDisplacementTexture = b.m_bHasDisplacementTexture;
		m_DisplacementTextureFileName = b.m_DisplacementTextureFileName;
		m_vEmbeddedDisplacementTextureRawData = b.m_vEmbeddedDisplacementTextureRawData;

		m_bHasOpacityTexture = b.m_bHasOpacityTexture;
		m_OpacityTextureFileName = b.m_OpacityTextureFileName;
		m_vEmbeddedOpacityTextureRawData = b.m_vEmbeddedOpacityTextureRawData;

		m_bShouldGenerateAutoMipMap = b.m_bShouldGenerateAutoMipMap;

		return *this;
	}

public:
	void SetName(const string& Name);
	void SetIndex(size_t Index);

	void SetTextureRawData(CTexture::EType eType, const vector<uint8_t>& Data);
	void SetTextureFileName(CTexture::EType eType, const string& FileName);

	void CreateTextures(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	void UseTextures() const;

private:
	void CreateTexture(CMaterial::CTexture::EType eType, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);

public:
	void SetUniformColor(const XMFLOAT3& Color);
	void SetAmbientColor(const XMFLOAT3& Color);
	void SetDiffuseColor(const XMFLOAT3& Color);
	void SetSpecularColor(const XMFLOAT3& Color);
	void SetSpecularExponent(float Exponent);
	void SetSpecularIntensity(float Intensity);

public:
	const string& GetName() const { return m_Name; }
	bool HasTexture() const { return m_bHasTexture; }

	void ClearEmbeddedTextureData(CTexture::EType eType);
	bool HasTexture(CTexture::EType eType) const;
	bool IsTextureEmbedded(CTexture::EType eType) const;
	const string& GetTextureFileName(CTexture::EType eType) const;
	const vector<uint8_t>& GetTextureRawData(CTexture::EType eType) const;

	void ShouldGenerateAutoMipMap(bool Value) { m_bShouldGenerateAutoMipMap = Value; }
	bool ShouldGenerateAutoMipMap() const { return m_bShouldGenerateAutoMipMap; }

	const XMFLOAT3& GetAmbientColor() const { return m_MaterialAmbient; }
	const XMFLOAT3& GetDiffuseColor() const { return m_MaterialDiffuse; }
	const XMFLOAT3& GetSpecularColor() const { return m_MaterialSpecular; }
	float GetSpecularIntensity() const { return m_SpecularIntensity; }
	float GetSpecularExponent() const { return m_SpecularExponent; }

public:
	static constexpr UINT	KDiffuseTextureSlotOffset{ 0 }; // PS
	static constexpr UINT	KNormalTextureSlotOffset{ 5 }; // PS
	static constexpr UINT	KOpacityTextureSlotOffset{ 10 }; // PS
	static constexpr UINT	KDisplacementTextureSlotOffset{ 0 }; // DS

private:
	string			m_Name{};
	size_t			m_Index{};

private:
	XMFLOAT3		m_MaterialAmbient{};
	XMFLOAT3		m_MaterialDiffuse{};
	XMFLOAT3		m_MaterialSpecular{};
	float			m_SpecularExponent{ 1.0f };
	float			m_SpecularIntensity{ 0.0f };

	bool			m_bHasTexture{ false };

	bool			m_bHasDiffuseTexture{ false };
	string			m_DiffuseTextureFileName{};
	vector<uint8_t>	m_vEmbeddedDiffuseTextureRawData{};
	unique_ptr<CMaterial::CTexture>	m_DiffuseTexture{};

	bool			m_bHasNormalTexture{ false };
	string			m_NormalTextureFileName{};
	vector<uint8_t>	m_vEmbeddedNormalTextureRawData{};
	unique_ptr<CMaterial::CTexture>	m_NormalTexture{};

	bool			m_bHasDisplacementTexture{ false };
	string			m_DisplacementTextureFileName{};
	vector<uint8_t>	m_vEmbeddedDisplacementTextureRawData{};
	unique_ptr<CMaterial::CTexture>	m_DisplacementTexture{};

	bool			m_bHasOpacityTexture{ false };
	string			m_OpacityTextureFileName{};
	vector<uint8_t>	m_vEmbeddedOpacityTextureRawData{};
	unique_ptr<CMaterial::CTexture>	m_OpacityTexture{};
	
	bool			m_bShouldGenerateAutoMipMap{};
};