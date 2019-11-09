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
	struct SComponentTransform
	{
		XMVECTOR		Translation{};
		XMVECTOR		RotationQuaternion{};
		XMVECTOR		Scaling{ XMVectorSet(1, 1, 1, 0) };
		XMMATRIX		MatrixWorld{ XMMatrixIdentity() };
	};

public:
	CObject3DLine(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject3DLine() {}

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
	void Create(const std::vector<SVertex3DLine>& vVertices);
	void UpdateVertexBuffer();
	void UpdateWorldMatrix();
	void Draw() const;

public:
	std::vector<SVertex3DLine>& GetVertices() { return m_vVertices; }
	const std::vector<SVertex3DLine>& GetVertices() const { return m_vVertices; }

public:
	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

public:
	SComponentTransform			ComponentTransform{};
	bool						bIsVisible{ true };

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};

private:
	std::string						m_Name{};
	std::vector<SVertex3DLine>		m_vVertices{};

	ComPtr<ID3D11Buffer>		m_VertexBuffer{};
	UINT						m_VertexBufferStride{ sizeof(SVertex3DLine) };
	UINT						m_VertexBufferOffset{};
};