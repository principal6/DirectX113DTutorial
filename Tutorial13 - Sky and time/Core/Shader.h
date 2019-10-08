#pragma once

#include <d3dcompiler.h>
#include "SharedHeader.h"

#pragma comment(lib, "d3dcompiler.lib")

class CShader final
{
	class CConstantBuffer
	{
	public:
		CConstantBuffer(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
			m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
		{
			assert(m_PtrDevice);
			assert(m_PtrDeviceContext);
		}
		~CConstantBuffer() {}

	public:
		void Create(EShaderType ShaderType, const void* PtrData, size_t DataByteWidth);
		void Update();
		void Use(UINT Slot) const;

	private:
		ID3D11Device* m_PtrDevice{};
		ID3D11DeviceContext* m_PtrDeviceContext{};

	private:
		ComPtr<ID3D11Buffer>	m_ConstantBuffer{};

		EShaderType				m_eShaderType{};
		size_t					m_DataByteWidth{};
		const void* m_PtrData{};
	};

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

	void AddConstantBuffer(const void* PtrData, size_t DataByteWidth);
	void UpdateConstantBuffer(size_t ConstantBufferIndex);
	void UpdateAllConstantBuffers();

	void Use();

private:
	ID3D11Device*						m_PtrDevice{};
	ID3D11DeviceContext*				m_PtrDeviceContext{};

private:
	ComPtr<ID3DBlob>					m_Blob{};
	ComPtr<ID3D11VertexShader>			m_VertexShader{};
	ComPtr<ID3D11PixelShader>			m_PixelShader{};
	ComPtr<ID3D11GeometryShader>		m_GeometryShader{};
	ComPtr<ID3D11InputLayout>			m_InputLayout{};

	EShaderType							m_ShaderType{};

private:
	vector<unique_ptr<CConstantBuffer>>	m_vConstantBuffers{};
};