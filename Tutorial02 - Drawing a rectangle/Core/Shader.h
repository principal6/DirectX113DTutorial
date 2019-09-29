#pragma once

#include <d3dcompiler.h>
#include "SharedHeader.h"

#pragma comment(lib, "d3dcompiler.lib")

enum class EShaderType
{
	VertexShader,
	PixelShader,
};

class CShader final
{
public:
	CShader(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice); 
		assert(m_PtrDeviceContext); 
	}
	~CShader() {}

	void Create(EShaderType Type, const wstring& FileName, const string& EntryPoint, 
		const D3D11_INPUT_ELEMENT_DESC* InputElementDescs = nullptr, UINT NumElements = 0);

	void Use();

private:
	ID3D11Device*				m_PtrDevice{};
	ID3D11DeviceContext*		m_PtrDeviceContext{};

private:
	ComPtr<ID3DBlob>			m_Blob{};
	ComPtr<ID3D11VertexShader>	m_VertexShader{};
	ComPtr<ID3D11PixelShader>	m_PixelShader{};
	ComPtr<ID3D11InputLayout>	m_InputLayout{};

	EShaderType					m_ShaderType{};
};