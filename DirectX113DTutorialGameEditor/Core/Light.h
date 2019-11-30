#pragma once

#include "SharedHeader.h"

// Base class for any deferred light class
class CLight
{
public:
	enum class EType
	{
		PointLight
	};

public:
	CLight(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CLight() {}

public:
	void SetPosition(const XMVECTOR& Position);
	const XMVECTOR& GetPosition() const;
	XMVECTOR& GetPosition();

public:
	const std::string& GetName() const;
	EType GetType() const;
	float GetBoundingSphereRadius() const;

protected:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};

protected:
	std::string					m_Name{};
	EType						m_eType{};

	float						m_BoundingSphereRadius{ 1.0f };
	XMVECTOR					m_Position{ 0, 0, 0, 1 };
};


// PointLight for deferred shading
// Position, Color, Range
class CPointLight final : public CLight
{
public:
	CPointLight(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		CLight(Name, PtrDevice, PtrDeviceContext)
	{
		m_eType = EType::PointLight;
	}
	~CPointLight() {}

public:
	void Apply();

public:
	void SetColor(const XMVECTOR& Color);
	const XMVECTOR& GetColor() const;

	void SetRange(float Range);
	float GetRange() const;

private:
	XMVECTOR					m_Color{ 1, 1, 1, 1 };
	float						m_Range{ 1.0f };
};