#include "Billboard.h"
#include "ConstantBuffer.h"
#include "Material.h"
#include "Shader.h"

using std::string;
using std::make_unique;

CBillboard::CBillboard(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CBillboard::~CBillboard()
{
}

void CBillboard::CreateScreenSpace(const std::string& TextureFileName, const XMFLOAT2& ScreenSize, const CConstantBuffer* const CBEditorTime)
{
	m_Texture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_Texture->CreateTextureFromFile(TextureFileName, false);

	m_CBBillboardData.bIsScreenSpace = TRUE;
	m_CBBillboardData.PixelSize = m_Texture->GetTextureSize();
	m_CBBillboardData.ScreenSize = ScreenSize;

	CreateShaders(CBEditorTime);
}

void CBillboard::CreateWorldSpace(const std::string& TextureFileName, const XMFLOAT2& WorldSpaceSize, const CConstantBuffer* const CBEditorTime)
{
	m_Texture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_Texture->CreateTextureFromFile(TextureFileName, true);

	m_CBBillboardData.bIsScreenSpace = FALSE;
	m_CBBillboardData.PixelSize = m_Texture->GetTextureSize();
	m_CBBillboardData.WorldSpaceSize = WorldSpaceSize;

	CreateShaders(CBEditorTime);
}

void CBillboard::CreateShaders(const CConstantBuffer* const CBEditorTime)
{
	m_CommonStates = make_unique<CommonStates>(m_PtrDevice);

	m_CBBillboard = make_unique<CConstantBuffer>(m_PtrDevice, m_PtrDeviceContext, &m_CBBillboardData, sizeof(m_CBBillboardData));
	m_CBBillboard->Create();

	bool bShouldCompileShaders{ false };

	m_VSBillboard = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_VSBillboard->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSBillboard.hlsl", "main",
		CBillboard::KInputElementDescs, ARRAYSIZE(CBillboard::KInputElementDescs));
	m_VSBillboard->ReserveConstantBufferSlots(KVSSharedCBCount);

	m_HSBillboard = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_HSBillboard->Create(EShaderType::HullShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\HSBillboard.hlsl", "main");
	m_HSBillboard->ReserveConstantBufferSlots(KHSSharedCBCount);

	m_DSBillboard = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_DSBillboard->Create(EShaderType::DomainShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\DSBillboard.hlsl", "main");
	m_DSBillboard->ReserveConstantBufferSlots(KDSSharedCBCount);
	m_DSBillboard->AttachConstantBuffer(m_CBBillboard.get());

	m_PSBillboard = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_PSBillboard->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBillboard.hlsl", "main");
	m_PSBillboard->ReserveConstantBufferSlots(KPSSharedCBCount);
	m_PSBillboard->AttachConstantBuffer(CBEditorTime);
}

void CBillboard::InsertInstance(const std::string& InstanceName)
{
	if (m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end())
	{
		MB_WARN("이미 존재하는 이름입니다.", "빌보드 인스턴스 생성 실패");
		return;
	}
	if (InstanceName.empty())
	{
		MB_WARN("이름은 공백일 수 없습니다.", "빌보드 인스턴스 생성 실패");
		return;
	}

	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	m_vInstanceCPUData.emplace_back(InstanceName); // @important
	m_vInstanceGPUData.emplace_back();
	m_mapInstanceNameToIndex[InstanceName] = m_vInstanceCPUData.size() - 1;

	(bShouldRecreateInstanceBuffer == true) ? CreateInstanceBuffer() : UpdateInstanceBuffer();
}

void CBillboard::DeleteInstance(const std::string& InstanceName)
{
	if (m_vInstanceCPUData.empty()) return;

	if (InstanceName.empty())
	{
		MB_WARN("이름이 잘못되었습니다.", "인스턴스 삭제 실패");
		return;
	}
	if (m_mapInstanceNameToIndex.find(InstanceName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN("해당 인스턴스는 존재하지 않습니다.", "인스턴스 삭제 실패");
		return;
	}

	uint32_t iInstance{ (uint32_t)m_mapInstanceNameToIndex.at(InstanceName) };
	uint32_t iLastInstance{ (uint32_t)(m_vInstanceCPUData.size() - 1) };
	string InstanceNameCopy{ InstanceName };
	if (iInstance == iLastInstance)
	{
		// End instance
		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(InstanceNameCopy);

		// @important
		--iInstance;
	}
	else
	{
		// Non-end instance
		string LastInstanceName{ m_vInstanceCPUData[iLastInstance].Name };

		std::swap(m_vInstanceCPUData[iInstance], m_vInstanceCPUData[iLastInstance]);
		std::swap(m_vInstanceGPUData[iInstance], m_vInstanceGPUData[iLastInstance]);

		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(InstanceNameCopy);
		m_mapInstanceNameToIndex[LastInstanceName] = iInstance;

		UpdateInstanceBuffer();
	}
}

void CBillboard::ClearInstances()
{
	m_vInstanceCPUData.clear();
	m_vInstanceCPUData.shrink_to_fit(); // @important
	m_vInstanceGPUData.clear();
	m_vInstanceGPUData.shrink_to_fit();

	m_mapInstanceNameToIndex.clear();
}

void CBillboard::CreateInstanceBuffer()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SInstanceGPUDataBillboard) * m_vInstanceGPUData.capacity()); // @important
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vInstanceGPUData[0];

	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_InstanceBuffer.Buffer.ReleaseAndGetAddressOf());
}

void CBillboard::UpdateInstanceBuffer()
{
	if (m_vInstanceGPUData.empty()) return;
	if (!m_InstanceBuffer.Buffer) return;

	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_InstanceBuffer.Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vInstanceGPUData[0], sizeof(SInstanceGPUDataBillboard) * m_vInstanceGPUData.size());

		m_PtrDeviceContext->Unmap(m_InstanceBuffer.Buffer.Get(), 0);
	}
}

void CBillboard::SetInstancePosition(const std::string& InstanceName, const XMVECTOR& Position)
{
	if (m_mapInstanceNameToIndex.find(InstanceName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN("해당 인스턴스를 찾을 수 없습니다.", "인스턴스 이동 실패");
		return;
	}

	m_vInstanceGPUData[GetInstanceID(InstanceName)].Position = Position;

	UpdateInstanceBuffer();
}

const XMVECTOR& CBillboard::GetInstancePosition(const std::string& InstanceName) const
{
	return m_vInstanceGPUData[GetInstanceID(InstanceName)].Position;
}

void CBillboard::SetInstanceHighlight(const std::string& InstanceName, bool bShouldHighlight)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].IsHighlighted = (bShouldHighlight) ? 1.0f : 0.0f;
}

void CBillboard::SetAllInstancesHighlightOff()
{
	for (auto& InstanceGPUData : m_vInstanceGPUData)
	{
		InstanceGPUData.IsHighlighted = 0.0f;
	}
}

size_t CBillboard::GetInstanceCount() const
{
	return m_vInstanceCPUData.size();
}

size_t CBillboard::GetInstanceID(const std::string& InstanceName) const
{
	return m_mapInstanceNameToIndex.at(InstanceName);
}

void CBillboard::Draw()
{
	ID3D11SamplerState* const SamplerStates[]{ m_CommonStates->LinearClamp(), m_CommonStates->PointClamp() };
	m_PtrDeviceContext->PSSetSamplers(0, 2, SamplerStates);

	m_Texture->Use();

	m_VSBillboard->Use();
	m_HSBillboard->Use();
	m_DSBillboard->Use();
	m_PSBillboard->Use();

	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_InstanceBuffer.Buffer.GetAddressOf(), &m_InstanceBuffer.Stride, &m_InstanceBuffer.Offset);
	m_PtrDeviceContext->DrawInstanced(1, (UINT)m_vInstanceCPUData.size(), 0, 0);

	m_PtrDeviceContext->HSSetShader(nullptr, nullptr, 0);
	m_PtrDeviceContext->DSSetShader(nullptr, nullptr, 0);
}
