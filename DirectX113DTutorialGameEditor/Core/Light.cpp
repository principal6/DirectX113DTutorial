#include "Light.h"

void CLight::SetPosition(const XMVECTOR& Position)
{
	m_Position = Position;
}

const XMVECTOR& CLight::GetPosition() const
{
	return m_Position;
}

XMVECTOR& CLight::GetPosition()
{
	return m_Position;
}

void CPointLight::Apply()
{

}

void CPointLight::SetColor(const XMVECTOR& Color)
{
	m_Color = Color;
}

const XMVECTOR& CPointLight::GetColor() const
{
	return m_Color;
}

void CPointLight::SetRange(float Range)
{
	m_Range = Range;
}

float CPointLight::GetRange() const
{
	return m_Range;
}

const std::string& CLight::GetName() const
{
	return m_Name;
}

CLight::EType CLight::GetType() const
{
	return m_eType;
}

float CLight::GetBoundingSphereRadius() const
{
	return m_BoundingSphereRadius;
}
