#include "Shader.h"
#include "ConstantBuffer.h"

using std::string;
using std::wstring;

void CShader::Create(EShaderType Type, const wstring& FileName, const string& EntryPoint,
	const D3D11_INPUT_ELEMENT_DESC* InputElementDescs, UINT NumElements)
{
	if (InputElementDescs) assert(Type == EShaderType::VertexShader);

	m_ShaderType = Type;

	switch (m_ShaderType)
	{
	case EShaderType::VertexShader:
		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			"vs_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);

		m_PtrDevice->CreateVertexShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, &m_VertexShader);

		if (InputElementDescs)
		{
			m_PtrDevice->CreateInputLayout(InputElementDescs, NumElements, m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), &m_InputLayout);
		}
		break;
	case EShaderType::HullShader:
		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			"hs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);

		m_PtrDevice->CreateHullShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, &m_HullShader);
		break;
	case EShaderType::DomainShader:
		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			"ds_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);

		m_PtrDevice->CreateDomainShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, &m_DomainShader);
		break;
	case EShaderType::GeometryShader:
		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			"gs_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);

		m_PtrDevice->CreateGeometryShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, &m_GeometryShader);
		break;
	case EShaderType::PixelShader:
		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			"ps_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);

		m_PtrDevice->CreatePixelShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, &m_PixelShader);
		break;
	default:
		break;
	}
}

void CShader::AttachConstantBuffer(CConstantBuffer* const ConstantBuffer, int32_t Slot)
{
	uint32_t uSlot{ (Slot == -1) ? (uint32_t)m_vAttachedConstantBuffers.size() : (uint32_t)Slot };
	m_vAttachedConstantBuffers.emplace_back(ConstantBuffer, uSlot);
}

void CShader::Use()
{
	switch (m_ShaderType)
	{
	case EShaderType::VertexShader:
		m_PtrDeviceContext->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		m_PtrDeviceContext->IASetInputLayout(m_InputLayout.Get());
		break;
	case EShaderType::HullShader:
		m_PtrDeviceContext->HSSetShader(m_HullShader.Get(), nullptr, 0);
		break;
	case EShaderType::DomainShader:
		m_PtrDeviceContext->DSSetShader(m_DomainShader.Get(), nullptr, 0);
		break;
	case EShaderType::GeometryShader:
		m_PtrDeviceContext->GSSetShader(m_GeometryShader.Get(), nullptr, 0);
		break;
	case EShaderType::PixelShader:
		m_PtrDeviceContext->PSSetShader(m_PixelShader.Get(), nullptr, 0);
		break;
	default:
		break;
	}

	for (const SAttachedConstantBuffer& AttachedConstantBuffer : m_vAttachedConstantBuffers)
	{
		AttachedConstantBuffer.PtrConstantBuffer->Use(m_ShaderType, AttachedConstantBuffer.AttachedSlot);
	}
}