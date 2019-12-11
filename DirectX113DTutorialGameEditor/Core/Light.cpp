#include "Light.h"

using std::string;
using std::to_string;

bool CLight::_InsertInstance(const std::string& InstanceName, EType eType)
{
	static size_t StaticInstanceCounter{};

	if (m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("해당 이름 (" + InstanceName + ") 을 사용할 수 없습니다.").c_str(), "인스턴스 생성 실패");
		return false;
	}

	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	m_vInstanceCPUData.emplace_back(InstanceName, eType); // @important
	m_vInstanceGPUData.emplace_back();
	m_mapInstanceNameToIndex[InstanceName] = m_vInstanceCPUData.size() - 1;

	(bShouldRecreateInstanceBuffer == true) ? CreateInstanceBuffer() : UpdateInstanceBuffer();
	++StaticInstanceCounter;

	return true;
}

bool CLight::DeleteInstance(const std::string& InstanceName)
{
	if (m_vInstanceCPUData.empty()) return false;

	if (InstanceName.empty())
	{
		MB_WARN("이름이 잘못되었습니다.", "인스턴스 삭제 실패");
		return false;
	}
	if (m_mapInstanceNameToIndex.find(InstanceName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN("해당 인스턴스는 존재하지 않습니다.", "인스턴스 삭제 실패");
		return false;
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

	return true;
}

void CLight::ClearInstances()
{
	m_vInstanceCPUData.clear();
	m_vInstanceCPUData.shrink_to_fit();
	m_vInstanceGPUData.clear();
	m_vInstanceGPUData.shrink_to_fit();

	m_mapInstanceNameToIndex.clear();
}

void CLight::CreateInstanceBuffer()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SInstanceGPUData) * m_vInstanceGPUData.capacity()); // @important
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vInstanceGPUData[0];

	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_InstanceBuffer.Buffer.ReleaseAndGetAddressOf());
}

void CLight::UpdateInstanceBuffer()
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_InstanceBuffer.Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vInstanceGPUData[0], sizeof(SInstanceGPUData) * m_vInstanceGPUData.size());

		m_PtrDeviceContext->Unmap(m_InstanceBuffer.Buffer.Get(), 0);
	}
}

void CLight::SetInstanceGPUData(const std::string& InstanceName, const SInstanceGPUData& Data)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)] = Data;

	UpdateInstanceBuffer();
}

void CLight::SetInstancePosition(const std::string& InstanceName, const XMVECTOR& Position)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].Position = Position;

	UpdateInstanceBuffer();
}

void CLight::SetInstanceColor(const std::string& InstanceName, const XMVECTOR& Color)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].Color = Color;

	UpdateInstanceBuffer();
}

void CLight::SetInstanceRange(const std::string& InstanceName, float Range)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].Range = Range;

	UpdateInstanceBuffer();
}

void CLight::Light()
{
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_InstanceBuffer.Buffer.GetAddressOf(), &m_InstanceBuffer.Stride, &m_InstanceBuffer.Offset);
	m_PtrDeviceContext->DrawInstanced(2, (UINT)m_vInstanceCPUData.size(), 0, 0);
}

size_t CLight::GetInstanceCount() const
{
	return m_vInstanceCPUData.size();
}

const CLight::SInstanceCPUData& CLight::GetInstanceCPUData(const std::string& InstanceName) const
{
	return m_vInstanceCPUData[GetInstanceID(InstanceName)];
}

const CLight::SInstanceGPUData& CLight::GetInstanceGPUData(const std::string& InstanceName) const
{
	return m_vInstanceGPUData[GetInstanceID(InstanceName)];
}

size_t CLight::GetInstanceID(const std::string& InstanceName) const
{
	return m_mapInstanceNameToIndex.at(InstanceName);
}

const std::map<std::string, size_t>& CLight::GetInstanceNameToIndexMap() const
{
	return m_mapInstanceNameToIndex;
}

float CLight::GetBoundingSphereRadius() const
{
	return m_BoundingSphereRadius;
}

bool CPointLight::InsertInstance(const std::string& InstanceName)
{
	return _InsertInstance(InstanceName, EType::PointLight);
}

CLight::EType CPointLight::GetType() const
{
	return EType::PointLight;
}

bool CSpotLight::InsertInstance(const std::string& InstanceName)
{
	return _InsertInstance(InstanceName, EType::SpotLight);
}

void CSpotLight::SetInstanceDirection(const std::string& InstanceName, const XMVECTOR& Direction)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].Direction = XMVector3Normalize(Direction);

	UpdateInstanceBuffer();
}

void CSpotLight::SetInstanceTheta(const std::string& InstanceName, float Theta)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].Theta = Theta;

	UpdateInstanceBuffer();
}

CLight::EType CSpotLight::GetType() const
{
	return EType::SpotLight;
}
