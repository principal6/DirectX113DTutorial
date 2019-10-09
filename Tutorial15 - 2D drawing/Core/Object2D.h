#pragma once

#include "SharedHeader.h"

struct SVertex2D
{
	SVertex2D() {}
	SVertex2D(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord) :
		Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
	XMVECTOR TexCoord{};
};

struct SObject2DData
{
	vector<SVertex2D>	vVertices{};
	vector<STriangle>	vTriangles{};
};

class CObject2D
{
	friend class CGame;

public:
	CObject2D(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) : 
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject2D() {}

public:
	void CreateStatic(const SObject2DData& _Object2DData);
	void CreateDynamic(const SObject2DData& _Object2DData);

	void UpdateVertexBuffer();

	void Draw();

private:
	void CreateIndexBuffer();

public:
	SObject2DData			Object2DData{};

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

private:
	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertex2D) };
	UINT					m_VertexBufferOffset{};

	ComPtr<ID3D11Buffer>	m_IndexBuffer{};
};