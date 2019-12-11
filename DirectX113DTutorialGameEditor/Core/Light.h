#pragma once

#include "SharedHeader.h"

// Base light class for deferred shading
class CLight
{
public:
	enum class EType
	{
		PointLight,
		SpotLight, // += Direction, Theta

		COUNT,
	};

	static constexpr uint32_t KLightTypeCount{ (uint32_t)EType::COUNT };

	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "DIRECTION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "RANGE"		, 0, DXGI_FORMAT_R32_FLOAT			, 0, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "THETA"		, 0, DXGI_FORMAT_R32_FLOAT			, 0, 52, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	struct SInstanceCPUData
	{
		SInstanceCPUData() {}
		SInstanceCPUData(const std::string& _Name, EType _eType) : Name{ _Name }, eType{ _eType } {}

		std::string Name{};
		EType		eType{};
	};

	struct alignas(16) SInstanceGPUData
	{
		XMVECTOR	Position{ 0, 0, 0, 1 };
		XMVECTOR	Color{ 1, 1, 1, 1 };
		XMVECTOR	Direction{ 0, -1, 0, 0 };
		float		Range{ 1.0f };
		float		Theta{ XM_PIDIV4 };
	};

protected:
	struct SInstanceBuffer
	{
		ComPtr<ID3D11Buffer>	Buffer{};
		UINT					Stride{ sizeof(SInstanceGPUData) };
		UINT					Offset{};
	};

public:
	CLight(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	virtual ~CLight() {}

public:
	virtual bool InsertInstance(const std::string& InstanceName) abstract;

protected:
	bool _InsertInstance(const std::string& InstanceName, EType eType);

public:
	bool DeleteInstance(const std::string& InstanceName);
	void ClearInstances();

protected:
	void CreateInstanceBuffer();
	void UpdateInstanceBuffer();

public:
	void SetInstanceGPUData(const std::string& InstanceName, const SInstanceGPUData& Data);
	void SetInstancePosition(const std::string& InstanceName, const XMVECTOR& Position);
	void SetInstanceColor(const std::string& InstanceName, const XMVECTOR& Color);
	void SetInstanceRange(const std::string& InstanceName, float Range);

public:
	void Light();

public:
	virtual EType GetType() const abstract;
	size_t GetInstanceCount() const;
	const SInstanceCPUData& GetInstanceCPUData(const std::string& InstanceName) const;
	const SInstanceGPUData& GetInstanceGPUData(const std::string& InstanceName) const;
	const std::map<std::string, size_t>& GetInstanceNameToIndexMap() const;
	float GetBoundingSphereRadius() const;

protected:
	size_t GetInstanceID(const std::string& InstanceName) const;

protected:
	static constexpr float KBoundingSphereRadiusDefault{ 0.25f };

protected:
	ID3D11Device* const				m_PtrDevice{};
	ID3D11DeviceContext* const		m_PtrDeviceContext{};

protected:
	std::vector<SInstanceCPUData>	m_vInstanceCPUData{};
	std::vector<SInstanceGPUData>	m_vInstanceGPUData{};
	SInstanceBuffer					m_InstanceBuffer{};
	std::map<std::string, size_t>	m_mapInstanceNameToIndex{};
	float							m_BoundingSphereRadius{ KBoundingSphereRadiusDefault };
};

// PointLight class derived from CLight class
class CPointLight final : public CLight
{
public:
	CPointLight(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) : CLight(PtrDevice, PtrDeviceContext) {}
	~CPointLight() {}

public:
	bool InsertInstance(const std::string& InstanceName) override;

public:
	EType GetType() const override;
};

// SpotLight class derived from CLight class.
// It requires additional [Direction] and [Theta]
class CSpotLight final : public CLight
{
public:
	CSpotLight(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) : CLight(PtrDevice, PtrDeviceContext) {}
	~CSpotLight() {}

public:
	bool InsertInstance(const std::string& InstanceName) override;

public:
	void SetInstanceDirection(const std::string& InstanceName, const XMVECTOR& Direction);
	void SetInstanceTheta(const std::string& InstanceName, float Theta);

public:
	EType GetType() const override;
};