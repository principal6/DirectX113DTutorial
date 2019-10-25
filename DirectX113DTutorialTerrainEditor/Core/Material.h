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
		CTexture(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
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
		void UpdateTextureRawData(const SPixel8UInt* PtrData);
		void UpdateTextureRawData(const SPixel32UInt* PtrData);
		void SetSlot(UINT Slot);
		void SetShaderType(EShaderType eShaderType);
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
		EShaderType							m_eShaderType{ EShaderType::PixelShader };

	private:
		ComPtr<ID3D11Texture2D>				m_Texture2D{};
		ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
	};

public:
	CMaterial() {}
	~CMaterial() {}

public:
	void SetName(const string& Name);

	void SetDiffuseTextureRawData(const vector<uint8_t>& Data);
	void SetDiffuseTextureFileName(const string& FileName);

	void SetNormalTextureRawData(const vector<uint8_t>& Data);
	void SetNormalTextureFileName(const string& FileName);

public:
	void SetUniformColor(const XMFLOAT3& Color);
	void SetAmbientColor(const XMFLOAT3& Color);
	void SetDiffuseColor(const XMFLOAT3& Color);
	void SetSpecularColor(const XMFLOAT3& Color);
	void SetSpecularExponent(float Exponent);
	void SetSpecularIntensity(float Intensity);

public:
	const string& GetName() const { return m_Name; }
	bool HasTexture() const { return bHasTexture; }

	bool HasDiffuseTexture() const { return bHasDiffuseTexture; }
	bool IsDiffuseTextureEmbedded() const { return (vEmbeddedDiffuseTextureRawData.size()) ? true : false; }
	const string& GetDiffuseTextureFileName() const { return DiffuseTextureFileName; }
	const vector<uint8_t>& GetDiffuseTextureRawData() const { return vEmbeddedDiffuseTextureRawData; }

	bool HasNormalTexture() const { return bHasNormalTexture; }
	bool IsNormalTextureEmbedded() const { return (vEmbeddedNormalTextureRawData.size()) ? true : false; }
	const string& GetNormalTextureFileName() const { return NormalTextureFileName; }
	const vector<uint8_t>& GetNormalTextureRawData() const { return vEmbeddedNormalTextureRawData; }
	
	void ShouldGenerateAutoMipMap(bool Value) { m_bShouldGenerateAutoMipMap = Value; }
	bool ShouldGenerateAutoMipMap() const { return m_bShouldGenerateAutoMipMap; }

	const XMFLOAT3& GetAmbientColor() const { return MaterialAmbient; }
	const XMFLOAT3& GetDiffuseColor() const { return MaterialDiffuse; }
	const XMFLOAT3& GetSpecularColor() const { return MaterialSpecular; }
	float GetSpecularIntensity() const { return SpecularIntensity; }
	float GetSpecularExponent() const { return SpecularExponent; }

private:
	string			m_Name{};

private:
	XMFLOAT3		MaterialAmbient{};
	XMFLOAT3		MaterialDiffuse{};
	XMFLOAT3		MaterialSpecular{};
	float			SpecularExponent{ 1.0f };
	float			SpecularIntensity{ 0.0f };

	bool			bHasTexture{ false };

	bool			bHasDiffuseTexture{ false };
	string			DiffuseTextureFileName{};
	vector<uint8_t>	vEmbeddedDiffuseTextureRawData{};

	bool			bHasNormalTexture{ false };
	string			NormalTextureFileName{};
	vector<uint8_t>	vEmbeddedNormalTextureRawData{};
	
	bool			m_bShouldGenerateAutoMipMap{};
};