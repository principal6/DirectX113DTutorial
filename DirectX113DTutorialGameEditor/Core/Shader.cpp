#include "Shader.h"
#include "ConstantBuffer.h"
#include <fstream>

using std::string;
using std::wstring;
using std::vector;

void CShader::Create(EShaderType Type, bool bShouldCompile, const wstring& FileName, const string& EntryPoint,
	const D3D11_INPUT_ELEMENT_DESC* InputElementDescs, UINT NumElements)
{
	static const char KCompileFailureTitle[]{ "Failed to compile shader." };
	static const char KCompileFailureMessage[]{ "Check out shader model/file name/entry point name." };
	
	if (InputElementDescs) assert(Type == EShaderType::VertexShader);

	m_ShaderType = Type;

	auto Dot{ FileName.find_last_of('.') };
	wstring wEntryPoint{ EntryPoint.begin(), EntryPoint.end() };
	wstring CSOFileName{ FileName.substr(0, Dot) + ((EntryPoint == "main") ? L"" : L"_" + wEntryPoint) + L".cso" };

	vector<byte> Buffer{};
	if (bShouldCompile)
	{
		switch (m_ShaderType)
		{
		case EShaderType::VertexShader:
			D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
				"vs_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);
			if (!m_Blob) MB_WARN(KCompileFailureMessage, KCompileFailureTitle);
			break;
		case EShaderType::HullShader:
			D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
				"hs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);
			if (!m_Blob) MB_WARN(KCompileFailureMessage, KCompileFailureTitle);
			break;
		case EShaderType::DomainShader:
			D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
				"ds_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);
			if (!m_Blob) MB_WARN(KCompileFailureMessage, KCompileFailureTitle);
			break;
		case EShaderType::GeometryShader:
			D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
				"gs_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);
			if (!m_Blob) MB_WARN(KCompileFailureMessage, KCompileFailureTitle);
			break;
		case EShaderType::PixelShader:
			D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
				"ps_4_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &m_Blob, nullptr);
			if (!m_Blob) MB_WARN(KCompileFailureMessage, KCompileFailureTitle);
			break;
		default:
			break;
		}
	}
	else
	{
		std::ifstream ifs{};
		ifs.open(CSOFileName, ifs.binary);
		ifs.seekg(0, ifs.end);
		Buffer.resize((size_t)ifs.tellg());
		ifs.seekg(0, ifs.beg);
		ifs.read((char*)&Buffer[0], Buffer.size());
		ifs.close();
	}
	const void* PtrBuffer{ (bShouldCompile) ? m_Blob->GetBufferPointer() : &Buffer[0] };
	size_t BufferSize{ (bShouldCompile) ? m_Blob->GetBufferSize() : Buffer.size() };

	switch (m_ShaderType)
	{
	case EShaderType::VertexShader:
		m_PtrDevice->CreateVertexShader(PtrBuffer, BufferSize, nullptr, &m_VertexShader);
		if (InputElementDescs)
		{
			m_PtrDevice->CreateInputLayout(InputElementDescs, NumElements, PtrBuffer, BufferSize, &m_InputLayout);
		}
		break;
	case EShaderType::HullShader:
		m_PtrDevice->CreateHullShader(PtrBuffer, BufferSize, nullptr, &m_HullShader);
		break;
	case EShaderType::DomainShader:
		m_PtrDevice->CreateDomainShader(PtrBuffer, BufferSize, nullptr, &m_DomainShader);
		break;
	case EShaderType::GeometryShader:
		m_PtrDevice->CreateGeometryShader(PtrBuffer, BufferSize, nullptr, &m_GeometryShader);
		break;
	case EShaderType::PixelShader:
		m_PtrDevice->CreatePixelShader(PtrBuffer, BufferSize, nullptr, &m_PixelShader);
		break;
	default:
		break;
	}

	if (bShouldCompile)
	{
		std::ofstream ofs{};
		ofs.open(CSOFileName, ofs.binary);
		ofs.write((const char*)PtrBuffer, BufferSize);
		ofs.close();
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
		if (m_InputLayout) m_PtrDeviceContext->IASetInputLayout(m_InputLayout.Get());
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