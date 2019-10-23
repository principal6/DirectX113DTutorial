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

class CObject2D
{
public:
	struct SData
	{
		vector<SVertex2D>	vVertices{};
		vector<STriangle>	vTriangles{};
	};

public:
	CObject2D(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) : 
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject2D() {}

public:
	void CreateStatic(const SData& Data);
	void CreateDynamic(const SData& Data);

	void UpdateVertexBuffer();

	void Draw();

public:
	SData& GetData() { return m_Data; }
	const SData& GetData() const { return m_Data; }

private:
	void CreateIndexBuffer();

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

private:
	SData					m_Data{};

	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertex2D) };
	UINT					m_VertexBufferOffset{};

	ComPtr<ID3D11Buffer>	m_IndexBuffer{};
};