#pragma once

#include "SharedHeader.h"

struct SVertex3DLine
{
	SVertex3DLine() {}
	SVertex3DLine(const XMVECTOR& _Position, const XMVECTOR& _Color = XMVectorSet(1, 0, 0, 1)) : Position{ _Position }, Color{ _Color } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
};

class CObject3DLine
{
public:
	CObject3DLine(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) : m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject3DLine() {}

public:
	void Create(const vector<SVertex3DLine>& _vVertices);
	void Update();
	void Draw() const;

public:
	vector<SVertex3DLine>& GetVertices() { return m_vVertices; }
	const vector<SVertex3DLine>& GetVertices() const { return m_vVertices; }

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

private:
	vector<SVertex3DLine>	m_vVertices{};

	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertex3DLine) };
	UINT					m_VertexBufferOffset{};
};