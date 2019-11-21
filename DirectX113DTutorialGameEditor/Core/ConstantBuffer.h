#pragma once

#include "SharedHeader.h"

class CConstantBuffer
{
public:
	CConstantBuffer(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, const void* const PtrData, size_t DataByteWidth) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrData{ PtrData }, m_DataByteWidth{ DataByteWidth }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
		assert(m_PtrData);
		assert(m_DataByteWidth);
	}
	~CConstantBuffer() {}

public:
	void Create();
	void Update();
	void Use(EShaderType eShaderType, uint32_t Slot) const;

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};

private:
	const size_t				m_DataByteWidth{};
	const void*	const			m_PtrData{};

private:
	ComPtr<ID3D11Buffer>		m_ConstantBuffer{};
};