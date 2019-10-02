#pragma once

#include "SharedHeader.h"

struct SVertex3D
{
	SVertex3D() {}
	SVertex3D(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord = XMVectorSet(0, 0, 0, 0)) :
		Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
	XMVECTOR TexCoord{};
	XMVECTOR Normal{};
};

struct SObject3DData
{
	vector<SVertex3D>	vVertices{};
	vector<STriangle>	vTriangles{};
};

class CObject3D
{
	friend class CGameWindow;

public:
	CObject3D(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject3D() {}

	void Create(const SObject3DData& Object3DData);

private:
	void Draw();

	void DrawNormals();

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

private:
	SObject3DData			m_Object3DData{};

	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertex3D) };
	UINT					m_VertexBufferOffset{};

	ComPtr<ID3D11Buffer>	m_IndexBuffer{};
};