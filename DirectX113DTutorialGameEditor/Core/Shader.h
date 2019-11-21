#pragma once

#include <d3dcompiler.h>
#include "SharedHeader.h"

#pragma comment(lib, "d3dcompiler.lib")

class CConstantBuffer;

class CShader final
{
	struct SAttachedConstantBuffer
	{
		SAttachedConstantBuffer() {}
		SAttachedConstantBuffer(CConstantBuffer* const _PtrConstantBuffer, uint32_t _AttachedSlot) :
			PtrConstantBuffer{ _PtrConstantBuffer }, AttachedSlot{ _AttachedSlot } 
		{
			assert(PtrConstantBuffer);
		}

		CConstantBuffer*	PtrConstantBuffer{};
		uint32_t			AttachedSlot{};
	};

public:
	CShader(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice); 
		assert(m_PtrDeviceContext); 
	}
	~CShader() {}

	void Create(EShaderType Type, const std::wstring& FileName, const std::string& EntryPoint,
		const D3D11_INPUT_ELEMENT_DESC* InputElementDescs = nullptr, UINT NumElements = 0);

	void AttachConstantBuffer(CConstantBuffer* const ConstantBuffer, int32_t Slot = -1);

	void Use();

private:
	ID3D11Device* const						m_PtrDevice{};
	ID3D11DeviceContext* const				m_PtrDeviceContext{};

private:
	ComPtr<ID3DBlob>						m_Blob{};

	ComPtr<ID3D11VertexShader>				m_VertexShader{};
	ComPtr<ID3D11HullShader>				m_HullShader{};
	ComPtr<ID3D11DomainShader>				m_DomainShader{};
	ComPtr<ID3D11GeometryShader>			m_GeometryShader{};
	ComPtr<ID3D11PixelShader>				m_PixelShader{};

	ComPtr<ID3D11InputLayout>				m_InputLayout{};

	EShaderType								m_ShaderType{};

private:
	std::vector<SAttachedConstantBuffer>	m_vAttachedConstantBuffers{};
};