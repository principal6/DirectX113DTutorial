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
	struct SComponentTransform
	{
		XMVECTOR	Translation{};
		XMVECTOR	RotationQuaternion{};
		XMVECTOR	Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX	MatrixWorld{ XMMatrixIdentity() };
	};

public:
	struct SData
	{
		vector<SVertex2D>	vVertices{};
		vector<STriangle>	vTriangles{};
	};

public:
	CObject2D(const string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject2D() {}

public:
	void* operator new(size_t Size)
	{
		return _aligned_malloc(Size, 16);
	}

	void operator delete(void* Pointer)
	{
		_aligned_free(Pointer);
	}

public:
	void CreateStatic(const SData& Data);
	void CreateDynamic(const SData& Data);

	void UpdateVertexBuffer();
	void UpdateWorldMatrix();

	void Draw() const;

public:
	SData& GetData() { return m_Data; }
	const SData& GetData() const { return m_Data; }

private:
	void CreateIndexBuffer();

public:
	static constexpr D3D11_INPUT_ELEMENT_DESC KInputLayout[]
	{
		{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT		, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

public:
	SComponentTransform			ComponentTransform{};
	bool						bIsVisible{ true };

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};

private:
	string						m_Name{};
	SData						m_Data{};

	ComPtr<ID3D11Buffer>		m_VertexBuffer{};
	UINT						m_VertexBufferStride{ sizeof(SVertex2D) };
	UINT						m_VertexBufferOffset{};

	ComPtr<ID3D11Buffer>		m_IndexBuffer{};
};