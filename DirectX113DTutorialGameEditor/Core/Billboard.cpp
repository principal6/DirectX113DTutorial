#include "Billboard.h"

using std::string;

void CBillboard::CreateScreenSpace(const std::string& TextureFileName)
{
	m_Texture.CreateTextureFromFile(TextureFileName, false);

	m_CBBillboardData.bIsScreenSpace = TRUE;
	m_CBBillboardData.PixelSize = m_Texture.GetTextureSize();
}

void CBillboard::CreateWorldSpace(const std::string& TextureFileName, const XMFLOAT2& WorldSpaceSize)
{
	m_Texture.CreateTextureFromFile(TextureFileName, true);

	m_CBBillboardData.bIsScreenSpace = FALSE;
	m_CBBillboardData.PixelSize = m_Texture.GetTextureSize();
	m_CBBillboardData.WorldSpaceSize = WorldSpaceSize;
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

	SetInstancePosition(m_mapInstanceNameToIndex.at(InstanceName), Position);
}

void CBillboard::SetInstancePosition(size_t InstanceID, const XMVECTOR& Position)
{
	if (InstanceID >= m_vInstanceGPUData.size()) return;

	m_vInstanceGPUData[InstanceID].Position = Position;

	UpdateInstanceBuffer();
}

const XMVECTOR& CBillboard::GetInstancePosition(size_t InstanceID) const
{
	return m_vInstanceGPUData[InstanceID].Position;
}

const CBillboard::SCBBillboardData& CBillboard::GetCBBillboard() const
{
	return m_CBBillboardData;
}

void CBillboard::Draw()
{
	m_Texture.Use();

	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_InstanceBuffer.Buffer.GetAddressOf(), &m_InstanceBuffer.Stride, &m_InstanceBuffer.Offset);
	m_PtrDeviceContext->DrawInstanced(1, (UINT)m_vInstanceCPUData.size(), 0, 0);
}
