#pragma once

#include "SharedHeader.h"

class CTexture
{
	friend class CGame;
	friend class CObject3D;
	friend class CTerrain;

public:
	CTexture(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, const string& Name) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_Name{ Name }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CTexture() {}

public:
	void CreateFromFile(const string& TextureFileName);
	void CreateWICFromMemory(const vector<uint8_t>& RawData);
	void CreateBlankTexture(DXGI_FORMAT Format, const XMFLOAT2& TextureSize);

public:
	void UpdateTextureRawData(const void* PtrData);
	void SetSlot(UINT Slot);

public:
	const string& GetName() { return m_Name; }
	const string& GetFileName() { return m_TextureFileName; }

public:
	ID3D11ShaderResourceView* GetShaderResourceViewPtr() { return m_ShaderResourceView.Get(); }

private:
	void Use(int ForcedSlot = -1) const;

private:
	ID3D11Device*						m_PtrDevice{};
	ID3D11DeviceContext*				m_PtrDeviceContext{};

private:
	string								m_Name{};
	string								m_TextureFileName{};
	XMFLOAT2							m_TextureSize{};
	size_t								m_PixelByteSize{};
	UINT								m_Slot{};

private:
	ComPtr<ID3D11Texture2D>				m_Texture2D{};
	ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
};