#pragma once

#include "SharedHeader.h"

struct SVertexLine
{
	SVertexLine() {}
	SVertexLine(const XMVECTOR& _Position, const XMVECTOR& _Color = XMVectorSet(1, 0, 0, 1)) : Position{ _Position }, Color{ _Color } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
};

class CObjectLine
{
public:
	CObjectLine(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) : m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObjectLine() {}

public:
	void Create(const vector<SVertexLine>& _vVertices);
	void Update();
	void Draw() const;

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

private:
	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertexLine) };
	UINT					m_VertexBufferOffset{};

public:
	vector<SVertexLine>		vVertices{};
};