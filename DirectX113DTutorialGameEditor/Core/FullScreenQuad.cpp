#include "FullScreenQuad.h"
#include "Shader.h"

using std::make_unique;

CFullScreenQuad::CFullScreenQuad(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CFullScreenQuad::~CFullScreenQuad()
{
}

void CFullScreenQuad::Create2DDrawer(CFullScreenQuad::EPixelShaderPass ePixelShaderPass)
{
	m_vVertices =
	{
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(0, 0, 0) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(1, 0, 0) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(0, 1, 0) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(1, 0, 0) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(1, 1, 0) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(0, 1, 0) }
	};

	CreateVertexBuffer();
	CreateShaders(ePixelShaderPass);
	
	m_CommonStates = make_unique<CommonStates>(m_PtrDevice);
}

void CFullScreenQuad::CreateCubemapDrawer()
{
	m_vVertices =
	{
		// X+
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(+1, +1, +1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, +1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, -1, -1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, +1) },

		// X-
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, +1, -1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, +1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, +1) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(-1, -1, +1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1) },

		// Y+										   
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, +1, -1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, +1, +1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, +1, +1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, +1, +1) },

		// Y-										   
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, -1, +1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, -1, +1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, -1, +1) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, -1, -1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1) },

		// Z+										   
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, +1, +1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, +1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, +1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, +1) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, -1, +1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, +1) },

		// Z-										   
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(+1, +1, -1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, -1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, -1) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, -1) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(-1, -1, -1) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, -1) }
	};

	CreateVertexBuffer();
	CreateShaders(EPixelShaderPass::AllChannels);

	m_CommonStates = make_unique<CommonStates>(m_PtrDevice);
}

void CFullScreenQuad::CreateVertexBuffer()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex) * m_vVertices.size());
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vVertices[0];
	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_VertexBufferBundle.Buffer.ReleaseAndGetAddressOf());
}

void CFullScreenQuad::CreateShaders(CFullScreenQuad::EPixelShaderPass ePixelShaderPass)
{
	bool bShouldCompileShaders{ false };

	m_VSScreenQuad = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_VSScreenQuad->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSScreenQuad.hlsl", "main",
		KInputElementDescs, ARRAYSIZE(KInputElementDescs));

	m_PSScreenQuad = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	switch (ePixelShaderPass)
	{
	case CFullScreenQuad::EPixelShaderPass::AllChannels:
		m_PSScreenQuad->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSScreenQuad.hlsl", "main");
		break;
	case CFullScreenQuad::EPixelShaderPass::OpaqueSRV:
		m_PSScreenQuad->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSScreenQuad.hlsl", "Opaque");
		break;
	case CFullScreenQuad::EPixelShaderPass::MonochromeSRV:
		m_PSScreenQuad->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSScreenQuad.hlsl", "Monochrome");
		break;
	default:
		break;
	}
	
	m_PtrPixelShader = m_PSScreenQuad.get();
}

void CFullScreenQuad::OverridePixelShader(CShader* const PixelShader)
{
	assert(PixelShader);
	m_PtrPixelShader = PixelShader;
}

void CFullScreenQuad::SetShaders() const
{
	m_VSScreenQuad->Use();
	m_PtrPixelShader->Use();
}

void CFullScreenQuad::SetSampler(CFullScreenQuad::ESamplerState eSamplerState)
{
	ID3D11SamplerState* SamplerStates[1]{};

	switch (eSamplerState)
	{
	case CFullScreenQuad::ESamplerState::PointClamp:
		SamplerStates[0] = m_CommonStates->PointClamp();
		break;
	case CFullScreenQuad::ESamplerState::PointWrap:
		SamplerStates[0] = m_CommonStates->PointWrap();
		break;
	case CFullScreenQuad::ESamplerState::LinearClamp:
		SamplerStates[0] = m_CommonStates->LinearClamp();
		break;
	case CFullScreenQuad::ESamplerState::LinearWrap:
		SamplerStates[0] = m_CommonStates->LinearWrap();
		break;
	default:
		break;
	}
	
	m_PtrDeviceContext->PSSetSamplers(0, 1, SamplerStates);
}

void CFullScreenQuad::SetIA()
{
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBufferBundle.Buffer.GetAddressOf(), &m_VertexBufferBundle.Stride, &m_VertexBufferBundle.Offset);
}

void CFullScreenQuad::Draw2D()
{
	m_PtrDeviceContext->Draw(6, 0);
}

void CFullScreenQuad::DrawCubemap(size_t iCubemapFace)
{
	assert(iCubemapFace < 6);

	m_PtrDeviceContext->Draw(6, static_cast<UINT>(iCubemapFace * 6));
}
