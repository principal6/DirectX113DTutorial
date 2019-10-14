#pragma once

#include "SharedHeader.h"

class CTexture
{
	friend class CGame;
	friend class CObject3D;

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

public:
	const string& GetName() { return m_Name; }

public:
	ID3D11ShaderResourceView* GetShaderResourceViewPtr() { return m_ShaderResourceView.Get(); }

private:
	void Use() const;

private:
	ID3D11Device*						m_PtrDevice{};
	ID3D11DeviceContext*				m_PtrDeviceContext{};

private:
	string								m_Name{};

private:
	ComPtr<ID3D11Texture2D>				m_Texture2D{};
	ComPtr<ID3D11ShaderResourceView>	m_ShaderResourceView{};
};