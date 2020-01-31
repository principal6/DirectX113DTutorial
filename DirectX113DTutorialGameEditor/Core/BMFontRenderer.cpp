#include "BMFontRenderer.h"
#include "BMFont.h"
#include "Material.h"
#include "Shader.h"
#include "ConstantBuffer.h"
#include "UTF8.h"

using std::make_unique;

#define SAFE_DELETE(a) if (a) { delete a; a = nullptr; }

CBMFontRenderer::CBMFontRenderer(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CBMFontRenderer::~CBMFontRenderer()
{
	ReleaseResources();
}

void CBMFontRenderer::Create(const std::string& FNT_FileName, const DirectX::XMFLOAT2& WindowSize)
{
	m_bHasOwnData = true; // @important

	// These variables needs to be updated every time
	{
		SAFE_DELETE(m_BMFont);
		m_BMFont = new CBMFont();
		m_BMFont->Load(FNT_FileName);

		const auto& FontData{ m_BMFont->GetData() };
		m_TextureSizeDenominator.x = 1.0f / static_cast<float>(FontData.Common.ScaleW);
		m_TextureSizeDenominator.y = 1.0f / static_cast<float>(FontData.Common.ScaleH);

		SAFE_DELETE(m_FontTexture);
		m_FontTexture = new CTexture(m_PtrDevice, m_PtrDeviceContext);
		m_FontTexture->CreateTextureFromFile(m_BMFont->GetData().vPages.front(), false);

		m_WindowSize = WindowSize;
		m_CBSpaceData.ProjectionMatrix = XMMatrixOrthographicLH(m_WindowSize.x, m_WindowSize.y, 0.0f, 1.0f);
	}

	// These variables only needs to be created once
	{
		if (!m_VSFont)
		{
			m_VSFont = new CShader(m_PtrDevice, m_PtrDeviceContext);
			m_VSFont->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, true, L"Shader\\VSFont.hlsl", "main",
				KInputLayout, ARRAYSIZE(KInputLayout));
		}

		if (!m_PSFont)
		{
			m_PSFont = new CShader(m_PtrDevice, m_PtrDeviceContext);
			m_PSFont->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, true, L"Shader\\PSFont.hlsl", "main");
		}

		if (!m_CBSpace)
		{
			m_CBSpace = new CConstantBuffer(m_PtrDevice, m_PtrDeviceContext, &m_CBSpaceData, sizeof(m_CBSpaceData));
			m_CBSpace->Create();
		}

		if (!m_CBColor)
		{
			m_CBColor = new CConstantBuffer(m_PtrDevice, m_PtrDeviceContext, &m_FontColor, sizeof(m_FontColor));
			m_CBColor->Create();
		}

		if (!m_CommonStates)
		{
			m_CommonStates = new CommonStates(m_PtrDevice);
		}
	}

	m_vGlyphs.resize(1);
	m_vGlyphIndices.resize(1);

	CreateBuffers();
}

void CBMFontRenderer::Create(const CBMFontRenderer* const FontRenderer)
{
	ReleaseResources();

	m_bHasOwnData = false; // @important

	m_BMFont = FontRenderer->m_BMFont;

	const auto& FontData{ m_BMFont->GetData() };
	m_TextureSizeDenominator.x = 1.0f / static_cast<float>(FontData.Common.ScaleW);
	m_TextureSizeDenominator.y = 1.0f / static_cast<float>(FontData.Common.ScaleH);

	m_FontTexture = FontRenderer->m_FontTexture;

	m_WindowSize = FontRenderer->m_WindowSize;
	m_CBSpaceData.ProjectionMatrix = XMMatrixOrthographicLH(m_WindowSize.x, m_WindowSize.y, 0.0f, 1.0f);

	m_VSFont = FontRenderer->m_VSFont;
	m_PSFont = FontRenderer->m_PSFont;
	m_CBSpace = FontRenderer->m_CBSpace;
	m_CommonStates = FontRenderer->m_CommonStates;

	m_CBColor = new CConstantBuffer(m_PtrDevice, m_PtrDeviceContext, &m_FontColor, sizeof(m_FontColor));
	m_CBColor->Create();

	m_vGlyphs.resize(1);
	m_vGlyphIndices.resize(1);

	CreateBuffers();
}

void CBMFontRenderer::CreateBuffers()
{
	// Vertex buffer
	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex) * m_StringCapacity * 4);
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &m_vGlyphs[0];

		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_VertexBuffer.ReleaseAndGetAddressOf());
	}

	// Index buffer
	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * m_StringCapacity * 6);
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &m_vGlyphIndices[0];

		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, m_IndexBuffer.ReleaseAndGetAddressOf());
	}
}

void CBMFontRenderer::UpdateBuffers()
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};

	// Vertex buffer
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vGlyphs[0], sizeof(SVertex) * 4 * m_vGlyphs.size());
		m_PtrDeviceContext->Unmap(m_VertexBuffer.Get(), 0);
	}

	// Index buffer
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_IndexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vGlyphIndices[0], sizeof(UINT) * 6 * m_vGlyphs.size());
		m_PtrDeviceContext->Unmap(m_IndexBuffer.Get(), 0);
	}
}

void CBMFontRenderer::ReleaseResources()
{
	if (m_bHasOwnData)
	{
		SAFE_DELETE(m_BMFont);
		SAFE_DELETE(m_FontTexture);
		SAFE_DELETE(m_VSFont);
		SAFE_DELETE(m_PSFont);
		SAFE_DELETE(m_CBSpace);
		SAFE_DELETE(m_CommonStates);
	}

	SAFE_DELETE(m_CBColor);
}

void CBMFontRenderer::SetVSConstantBufferSlot(UINT Slot)
{
	m_CBSpaceSlot = Slot;
}

void CBMFontRenderer::SetPSConstantBufferSlot(UINT Slot)
{
	m_CBColorSlot = Slot;
}

void CBMFontRenderer::SetFontColor(const DirectX::XMFLOAT4& Color)
{
	m_FontColor = Color;
	m_CBColor->Update();
}

CBMFontRenderer::SGlyph CBMFontRenderer::GetGlyph(size_t GlyphIndex, size_t CharIndex, int32_t& CursorX, int32_t CursorY)
{
	const auto& FontData{ m_BMFont->GetData() };
	const auto& Char{ FontData.vChars[CharIndex] };

	CBMFontRenderer::SGlyph Glyph{};

	float U0{ static_cast<float>(Char.X) * m_TextureSizeDenominator.x };
	float V0{ static_cast<float>(Char.Y) * m_TextureSizeDenominator.y };
	float U1{ U0 + static_cast<float>(Char.Width) * m_TextureSizeDenominator.x };
	float V1{ V0 + static_cast<float>(Char.Height) * m_TextureSizeDenominator.y };

	Glyph.Vertices[0].Position.x = static_cast<float>(CursorX + Char.XOffset);
	Glyph.Vertices[0].Position.y = static_cast<float>(CursorY - Char.YOffset);
	Glyph.Vertices[0].TexCoord.x = U0;
	Glyph.Vertices[0].TexCoord.y = V0;

	Glyph.Vertices[1].Position.x = Glyph.Vertices[0].Position.x + static_cast<float>(Char.Width);
	Glyph.Vertices[1].Position.y = Glyph.Vertices[0].Position.y;
	Glyph.Vertices[1].TexCoord.x = U1;
	Glyph.Vertices[1].TexCoord.y = V0;

	Glyph.Vertices[2].Position.x = Glyph.Vertices[0].Position.x;
	Glyph.Vertices[2].Position.y = Glyph.Vertices[0].Position.y - static_cast<float>(Char.Height);
	Glyph.Vertices[2].TexCoord.x = U0;
	Glyph.Vertices[2].TexCoord.y = V1;

	Glyph.Vertices[3].Position.x = Glyph.Vertices[1].Position.x;
	Glyph.Vertices[3].Position.y = Glyph.Vertices[2].Position.y;
	Glyph.Vertices[3].TexCoord.x = U1;
	Glyph.Vertices[3].TexCoord.y = V1;

	CursorX += Char.XAdvance; // @important

	return Glyph;
}

void CBMFontRenderer::RenderMutableString(const char* UTF8String, const DirectX::XMFLOAT2& Position)
{
	assert(UTF8String);

	size_t Length{ GetUTF8StringLength(UTF8String) };
	const auto& CharIndexMap{ m_BMFont->GetCharIndexMap() };
	m_vGlyphs.clear();
	m_vGlyphs.reserve(Length * 4);
	m_vGlyphIndices.clear();
	m_vGlyphIndices.reserve(Length * 6);

	UINT GlyphIndex{};
	size_t BufferAt{};
	size_t BufferSize{ strlen(UTF8String) };
	int32_t CursorX{ static_cast<int32_t>(-m_WindowSize.x * 0.5f) };
	int32_t CursorY{ static_cast<int32_t>(+m_WindowSize.y * 0.5f) };
	while (BufferAt < BufferSize)
	{
		size_t ByteCount{ GetUTF8CharacterByteCount(UTF8String[BufferAt]) };

		UUTF8 UTF8{};
		memcpy(UTF8.Chars, &UTF8String[BufferAt], ByteCount);

		size_t CharIndex{ (CharIndexMap.find(UTF8.UInt32) != CharIndexMap.end()) ? CharIndexMap.at(UTF8.UInt32) : 0 };
		m_vGlyphs.emplace_back(GetGlyph(GlyphIndex, CharIndex, CursorX, CursorY));

		UINT VertexBase{ GlyphIndex * 4 };
		m_vGlyphIndices.emplace_back(VertexBase + 0);
		m_vGlyphIndices.emplace_back(VertexBase + 1);
		m_vGlyphIndices.emplace_back(VertexBase + 2);
		m_vGlyphIndices.emplace_back(VertexBase + 1);
		m_vGlyphIndices.emplace_back(VertexBase + 3);
		m_vGlyphIndices.emplace_back(VertexBase + 2);

		++GlyphIndex;
		BufferAt += ByteCount;
	}

	bool bCapacityChanged{ false };
	while (m_vGlyphs.size() > m_StringCapacity)
	{
		m_StringCapacity *= 2;
		bCapacityChanged = true;
	}

	if (bCapacityChanged)
	{
		CreateBuffers();
	}
	else
	{
		UpdateBuffers();
	}

	m_CBSpaceData.Position.x = Position.x;
	m_CBSpaceData.Position.y = Position.y;
	m_CBSpace->Update();

	Render();
}

void CBMFontRenderer::RenderConstantString(const char* UTF8String, const DirectX::XMFLOAT2& Position)
{
	assert(UTF8String);
	if (m_PtrConstantUTF8String != UTF8String)
	{
		m_PtrConstantUTF8String = UTF8String;

		size_t Length{ GetUTF8StringLength(m_PtrConstantUTF8String) };
		const auto& CharIndexMap{ m_BMFont->GetCharIndexMap() };
		m_vGlyphs.clear();
		m_vGlyphs.reserve(Length * 4);
		m_vGlyphIndices.clear();
		m_vGlyphIndices.reserve(Length * 6);

		UINT GlyphIndex{};
		size_t BufferAt{};
		size_t BufferSize{ strlen(m_PtrConstantUTF8String) };
		int32_t CursorX{ static_cast<int32_t>(-m_WindowSize.x * 0.5f) };
		int32_t CursorY{ static_cast<int32_t>(+m_WindowSize.y * 0.5f) };
		while (BufferAt < BufferSize)
		{
			size_t ByteCount{ GetUTF8CharacterByteCount(m_PtrConstantUTF8String[BufferAt]) };

			UUTF8 UTF8{};
			memcpy(UTF8.Chars, &m_PtrConstantUTF8String[BufferAt], ByteCount);

			size_t CharIndex{ (CharIndexMap.find(UTF8.UInt32) != CharIndexMap.end()) ? CharIndexMap.at(UTF8.UInt32) : 0 };
			m_vGlyphs.emplace_back(GetGlyph(GlyphIndex, CharIndex, CursorX, CursorY));

			UINT VertexBase{ GlyphIndex * 4 };
			m_vGlyphIndices.emplace_back(VertexBase + 0);
			m_vGlyphIndices.emplace_back(VertexBase + 1);
			m_vGlyphIndices.emplace_back(VertexBase + 2);
			m_vGlyphIndices.emplace_back(VertexBase + 1);
			m_vGlyphIndices.emplace_back(VertexBase + 3);
			m_vGlyphIndices.emplace_back(VertexBase + 2);

			++GlyphIndex;
			BufferAt += ByteCount;
		}

		bool bCapacityChanged{ false };
		while (m_vGlyphs.size() > m_StringCapacity)
		{
			m_StringCapacity *= 2;
			bCapacityChanged = true;
		}

		if (bCapacityChanged)
		{
			CreateBuffers();
		}
		else
		{
			UpdateBuffers();
		}
	}

	m_CBSpaceData.Position.x = Position.x;
	m_CBSpaceData.Position.y = Position.y;
	m_CBSpace->Update();

	Render();
}

void CBMFontRenderer::Render()
{
	m_CBSpace->Use(EShaderType::VertexShader, m_CBSpaceSlot);
	m_CBColor->Use(EShaderType::PixelShader, m_CBColorSlot);

	m_FontTexture->Use();

	m_VSFont->Use();
	m_PSFont->Use();

	ID3D11SamplerState* Samplers[]{ m_CommonStates->PointClamp() };
	m_PtrDeviceContext->PSSetSamplers(0, 1, Samplers);

	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	m_PtrDeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(m_vGlyphs.size() * 6), 0, 0);
}
