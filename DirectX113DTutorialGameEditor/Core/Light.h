#pragma once

#include "SharedHeader.h"

// Light class for deferred shading
class CLight final
{
public:
	enum class EType
	{
		PointLight
	};

	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "COLOR"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "RANGE"	, 0, DXGI_FORMAT_R32_FLOAT			, 0, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	struct SLightInstanceCPUData
	{
		SLightInstanceCPUData() {}
		SLightInstanceCPUData(const std::string& _Name, EType _eType) : Name{ _Name }, eType{ _eType } {}

		std::string Name{};
		EType		eType{};
	};

	struct alignas(16) SLightInstanceGPUData
	{
		XMVECTOR	Position{ 0, 0, 0, 1 };
		XMVECTOR	Color{ 1, 1, 1, 1 };
		float		Range{ 1.0f };
	};

	struct SInstanceBuffer
	{
		ComPtr<ID3D11Buffer>	Buffer{};
		UINT					Stride{ sizeof(SLightInstanceGPUData) }; // @important
		UINT					Offset{};
	};

public:
	CLight(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CLight() {}

public:
	bool InsertInstance(const std::string& InstanceName, EType eType);
	bool DeleteInstance(const std::string& InstanceName);

	size_t GetInstanceCount() const;
	const SLightInstanceCPUData& GetInstanceCPUData(size_t InstanceID) const;
	const SLightInstanceGPUData& GetInstanceGPUData(size_t InstanceID) const;
	SLightInstanceGPUData& GetInstanceGPUData(size_t InstanceID);
	std::string GetInstanceName(size_t InstanceID) const;
	size_t GetInstanceID(const std::string& InstanceName) const;
	
	void SetInstancePosition(size_t InstanceID, const XMVECTOR& Position);
	void SetInstanceColor(size_t InstanceID, const XMVECTOR& Color);
	void SetInstanceRange(size_t InstanceID, float Range);

private:
	void CreateInstanceBuffer();
	void UpdateInstanceBuffer();

public:
	void Light();

public:
	float GetBoundingSphereRadius() const;

private:
	ID3D11Device* const					m_PtrDevice{};
	ID3D11DeviceContext* const			m_PtrDeviceContext{};

private:
	SInstanceBuffer						m_InstanceBuffer{};
	std::vector<SLightInstanceCPUData>	m_vInstanceCPUData{};
	std::vector<SLightInstanceGPUData>	m_vInstanceGPUData{};
	std::map<std::string, size_t>		m_mapInstanceNameToIndex{};

	float								m_BoundingSphereRadius{ 1.0f };
};