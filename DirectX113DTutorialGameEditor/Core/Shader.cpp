#include "Shader.h"
#include "ConstantBuffer.h"
#include <d3dcompiler.h>
#include <fstream>

#pragma comment(lib, "d3dcompiler.lib")

using std::string;
using std::wstring;
using std::vector;

void CShader::Create(EShaderType Type, EVersion eVersion, bool bShouldCompile, const wstring& FileName, const string& EntryPoint,
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

	string VersionSuffix{};
	switch (eVersion)
	{
	case CShader::EVersion::_4_0:
		VersionSuffix = "_4_0";
		break;
	case CShader::EVersion::_4_1:
		VersionSuffix = "_4_1";
		break;
	case CShader::EVersion::_5_0:
		VersionSuffix = "_5_0";
		break;
	case CShader::EVersion::_5_1:
		VersionSuffix = "_5_1";
		break;
	default:
		break;
	}

	string ShaderPrefix{};
	switch (m_ShaderType)
	{
	case EShaderType::VertexShader:
		ShaderPrefix = "vs";
		break;
	case EShaderType::HullShader:
		ShaderPrefix = "hs";
		break;
	case EShaderType::DomainShader:
		ShaderPrefix = "ds";
		break;
	case EShaderType::GeometryShader:
		ShaderPrefix = "gs";
		break;
	case EShaderType::PixelShader:
		ShaderPrefix = "ps";
		break;
	default:
		break;
	}

	if (bShouldCompile)
	{
		D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
			(ShaderPrefix + VersionSuffix).c_str(), D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, m_Blob.ReleaseAndGetAddressOf(), nullptr);
		if (!m_Blob) MB_WARN(KCompileFailureMessage, KCompileFailureTitle);
	}
	else
	{
		std::ifstream ifs{};
		ifs.open(CSOFileName, ifs.binary);
		if (!ifs.is_open())
		{
			MB_WARN("해당 cso파일을 찾을 수 없습니다.", "셰이더 생성 실패");
			return;
		}
		ifs.seekg(0, ifs.end);
		size_t BufferSize{ (size_t)ifs.tellg() };
		Buffer.resize(BufferSize);
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

bool CShader::CompileCSO(EShaderType Type, EVersion eVersion, const std::wstring& FileName, const std::string& EntryPoint)
{
	ComPtr<ID3DBlob> Blob{};

	string VersionSuffix{};
	switch (eVersion)
	{
	case CShader::EVersion::_4_0:
		VersionSuffix = "_4_0";
		break;
	case CShader::EVersion::_4_1:
		VersionSuffix = "_4_1";
		break;
	case CShader::EVersion::_5_0:
		VersionSuffix = "_5_0";
		break;
	case CShader::EVersion::_5_1:
		VersionSuffix = "_5_1";
		break;
	default:
		break;
	}

	string ShaderPrefix{};
	switch (Type)
	{
	case EShaderType::VertexShader:
		ShaderPrefix = "vs";
		break;
	case EShaderType::HullShader:
		ShaderPrefix = "hs";
		break;
	case EShaderType::DomainShader:
		ShaderPrefix = "ds";
		break;
	case EShaderType::GeometryShader:
		ShaderPrefix = "gs";
		break;
	case EShaderType::PixelShader:
		ShaderPrefix = "ps";
		break;
	default:
		break;
	}

	if (SUCCEEDED(D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(),
		(ShaderPrefix + VersionSuffix).c_str(), D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, Blob.ReleaseAndGetAddressOf(), nullptr)))
	{
		const void* PtrBuffer{ Blob->GetBufferPointer() };
		size_t BufferSize{ Blob->GetBufferSize() };

		auto Dot{ FileName.find_last_of('.') };
		wstring wEntryPoint{ EntryPoint.begin(), EntryPoint.end() };
		wstring CSOFileName{ FileName.substr(0, Dot) + ((EntryPoint == "main") ? L"" : L"_" + wEntryPoint) + L".cso" };

		std::ofstream ofs{};
		ofs.open(CSOFileName, ofs.binary);
		ofs.write((const char*)PtrBuffer, BufferSize);
		ofs.close();

		return true;
	}
	return false;
}

void CShader::ReserveConstantBufferSlots(uint32_t Count)
{
	m_SlotCounter = Count;
}

void CShader::AttachConstantBuffer(const CConstantBuffer* const ConstantBuffer, int32_t Slot)
{
	uint32_t uSlot{ (Slot == -1) ? m_SlotCounter : (uint32_t)Slot };
	m_vAttachedConstantBuffers.emplace_back(ConstantBuffer, uSlot);

	++m_SlotCounter;
}

void CShader::Use() const
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