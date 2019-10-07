#pragma once

#include "SharedHeader.h"

class CTexture
{
	friend class CGameWindow;
	friend class CObject3D;

public:
	CTexture(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) : 
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CTexture() {}

	void CreateFromFile(const wstring& TextureFileName);
	void CreateWICFromMemory(const vector<uint8_t>& RawData);

private:
	void Use() const;

private:
	ID3D11Device*						m_PtrDevice{};
	ID3D11DeviceContext*				m_PtrDeviceContext{};

private:
	ComPtr<ID3D11Texture2D>				m_Texture2D{};
	ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
};