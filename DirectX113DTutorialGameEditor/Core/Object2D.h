#pragma once

#include "SharedHeader.h"
#include "Material.h"

class CObject2D
{
public:
	struct SVertex
	{
		SVertex() {}
		SVertex(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord) :
			Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

		XMVECTOR Position{};
		XMVECTOR Color{};
		XMVECTOR TexCoord{};
	};

	struct SModel2D
	{
		std::vector<SVertex>	vVertices{};
		std::vector<STriangle>	vTriangles{};
	};

	struct SComponentTransform
	{
		XMMATRIX	MatrixWorld{ XMMatrixIdentity() };
		XMMATRIX	MatrixTranslation{};
		XMMATRIX	MatrixRotation{};
		XMMATRIX	MatrixScaling{};
		XMFLOAT2	Translation{};
		XMFLOAT2	Scaling{ 1.0f, 1.0f };
		float		RotationAngle{};
	};

public:
	CObject2D(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CObject2D() {}

public:
	void* operator new(size_t Size)
	{
		return _aligned_malloc(Size, KAlignmentBytes);
	}

	void operator delete(void* Pointer)
	{
		_aligned_free(Pointer);
	}

public:
	void Create(const SModel2D& Model2D, bool bIsDynamic = false);
	void CreateTexture(const std::string& FileName);

private:
	void CreateIndexBuffer();

public:
	void Translate(const XMFLOAT2& DeltaPosition);
	void Rotate(float DeltaAngle);
	void Scale(const XMFLOAT2& DeltaScalar);

	void TranslateTo(const XMFLOAT2& NewPosition);
	void RotateTo(float NewAngle);
	void ScaleTo(const XMFLOAT2& NewScalar);

private:
	void UpdateWorldMatrix();

public:
	void UpdateVertexBuffer();
	void Draw(bool bIgnoreOwnTexture = false) const;

public:
	void IsVisible(bool bIsVisible);
	auto IsVisible() const->bool;
	auto GetWorldMatrix() const->const XMMATRIX&;
	auto GetName() const->const std::string&;
	auto GetData()->SModel2D&;
	auto GetData() const->const SModel2D&;
	auto HasTexture() const->bool;
	auto IsInstanced() const->bool { return false; } // @TEMPRORATY

public:
	static constexpr D3D11_INPUT_ELEMENT_DESC KInputLayout[]
	{
		{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	static constexpr size_t		KAlignmentBytes{ 16 };

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};

private:
	std::string					m_Name{};
	SModel2D					m_Model2D{};
	bool						m_bIsDynamic{};
	bool						m_bIsVisible{ true };
	SComponentTransform			m_ComponentTransform{};

private:
	ComPtr<ID3D11Buffer>		m_VertexBuffer{};
	UINT						m_VertexBufferStride{ sizeof(SVertex) };
	UINT						m_VertexBufferOffset{};
	ComPtr<ID3D11Buffer>		m_IndexBuffer{};

	std::unique_ptr<CTexture>	m_Texture{};
};