#include "Shader.h"

void CShader::Create(EShaderType Type, const wstring& FileName, const string& EntryPoint,
	const D3D11_INPUT_ELEMENT_DESC* InputElementDescs, UINT NumElements)
{
	if (InputElementDescs) assert(Type == EShaderType::VertexShader);

	m_ShaderType = Type;

	switch (m_ShaderType)
	{
	case EShaderType::VertexShader:
		assert(InputElementDescs);

		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			"vs_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);

		m_PtrDevice->CreateVertexShader(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), nullptr, &m_VertexShader);

		m_PtrDevice->CreateInputLayout(InputElementDescs, NumElements,
			m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(), &m_InputLayout);
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

void CShader::Use()
{
	switch (m_ShaderType)
	{
	case EShaderType::VertexShader:
		m_PtrDeviceContext->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		m_PtrDeviceContext->IASetInputLayout(m_InputLayout.Get());
		break;
	case EShaderType::PixelShader:
		m_PtrDeviceContext->PSSetShader(m_PixelShader.Get(), nullptr, 0);
		break;
	default:
		break;
	}
}