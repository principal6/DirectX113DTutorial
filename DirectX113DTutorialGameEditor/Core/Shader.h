#pragma once

#include "SharedHeader.h"

class CConstantBuffer;

class CShader final
{
public:
	enum class EVersion
	{
		_4_0,
		_4_1,
		_5_0,
		_5_1
	};

private:
	struct SAttachedConstantBuffer
	{
		SAttachedConstantBuffer() {}
		SAttachedConstantBuffer(const CConstantBuffer* const _PtrConstantBuffer, uint32_t _AttachedSlot) :
			PtrConstantBuffer{ _PtrConstantBuffer }, AttachedSlot{ _AttachedSlot } 
		{
			assert(PtrConstantBuffer);
		}

		const CConstantBuffer*	PtrConstantBuffer{};
		uint32_t				AttachedSlot{};
	};

public:
	CShader(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice); 
		assert(m_PtrDeviceContext); 
	}
	~CShader() {}

public:
	void Create(EShaderType Type, EVersion eVersion, bool bShouldCompile, const std::wstring& FileName, const std::string& EntryPoint,
		const D3D11_INPUT_ELEMENT_DESC* InputElementDescs = nullptr, UINT NumElements = 0);

	bool CompileCSO(EShaderType Type, EVersion eVersion, const std::wstring& FileName, const std::string& EntryPoint);

public:
	void ReserveConstantBufferSlots(uint32_t Count);
	void AttachConstantBuffer(const CConstantBuffer* const ConstantBuffer, int32_t Slot = -1);

public:
	void Use() const;

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
	uint32_t								m_SlotCounter{};
};