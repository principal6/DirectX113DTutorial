#pragma once

#include "SharedHeader.h"

class CShader;

class CFullScreenQuad
{
public:
	enum class EPixelShaderPass
	{
		AllChannels,
		OpaqueSRV,
		MonochromeSRV,
	};

private:
	struct SVertex
	{
		SVertex(const XMFLOAT4& _Position, const XMFLOAT3& _TexCoord) : Position{ _Position }, TexCoord{ _TexCoord } {}

		XMFLOAT4 Position;
		XMFLOAT3 TexCoord;
	};

	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

public:
	CFullScreenQuad(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CFullScreenQuad();

public:
	void Create2DDrawer(CFullScreenQuad::EPixelShaderPass PixelShaderPass = CFullScreenQuad::EPixelShaderPass::AllChannels);
	void CreateCubemapDrawer();

private:
	void CreateVertexBuffer();
	void CreateShaders(CFullScreenQuad::EPixelShaderPass PixelShaderPass);

public:
	void OverridePixelShader(CShader* const PixelShader);

public:
	void SetShaders() const;

	void SetPointClampSampler();

	void SetIA();

public:
	void Draw2D();

	void DrawCubemap(size_t iCubemapFace);

private:
	ID3D11Device* const				m_PtrDevice{};
	ID3D11DeviceContext* const		m_PtrDeviceContext{};

private:
	std::vector<SVertex>			m_vVertices{};
	SVertexBufferBundle				m_VertexBufferBundle{ sizeof(SVertex) };

private:
	std::unique_ptr<CShader>		m_VSScreenQuad{};
	std::unique_ptr<CShader>		m_PSScreenQuad{};
	CShader*						m_PtrPixelShader{};

	std::unique_ptr<CommonStates>	m_CommonStates{};
};