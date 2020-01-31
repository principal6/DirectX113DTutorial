#include "BMFont.h"
#include "BinaryData.h"
#include "UTF8.h"

using std::string;

CBMFont::CBMFont()
{
}

CBMFont::~CBMFont()
{
}

void CBMFont::Load(const std::string& FNT_FileName)
{
	string FileName{ FNT_FileName };
	for (auto& Char : FileName)
	{
		if (Char == '/') Char = '\\';
	}
	std::string Directory{};
	if (FNT_FileName.find('\\') != string::npos)
	{
		size_t At{ FNT_FileName.find_last_of('\\') };
		Directory = FNT_FileName.substr(0, At + 1);
	}

	m_FontFace = SBMFontData();
	m_mapIDToCharsIndex.clear();

	CBinaryData BinaryData{};
	BinaryData.LoadFromFile(FNT_FileName);
	
	string ReadString{};
	BinaryData.ReadString(ReadString, 3);
	assert(ReadString == "BMF");

	byte Version{ BinaryData.ReadByte() };
	assert(Version == 3);

	// Block #0 font info
	{
		byte BlockID{ BinaryData.ReadByte() };
		uint32_t BlockByteSize{ BinaryData.ReadUint32() };
		assert(BlockByteSize);

		BinaryData.ReadUint16(m_FontFace.Info.FontSize);
		BinaryData.ReadByte(m_FontFace.Info.BitField);

		BinaryData.ReadUint8(m_FontFace.Info.CharSet);
		BinaryData.ReadUint16(m_FontFace.Info.StretchH);
		BinaryData.ReadUint8(m_FontFace.Info.AA);

		BinaryData.ReadUint8(m_FontFace.Info.PaddingUp);
		BinaryData.ReadUint8(m_FontFace.Info.PaddingRight);
		BinaryData.ReadUint8(m_FontFace.Info.PaddingDown);
		BinaryData.ReadUint8(m_FontFace.Info.PaddingLeft);

		BinaryData.ReadUint8(m_FontFace.Info.SpacingHorz);
		BinaryData.ReadUint8(m_FontFace.Info.SpacingVert);

		BinaryData.ReadUint8(m_FontFace.Info.Outline);

		BinaryData.ReadNullTerminatedString(m_FontFace.Info.FontName);
	}

	// Block #1 common
	{
		byte BlockID{ BinaryData.ReadByte() };
		uint32_t BlockByteSize{ BinaryData.ReadUint32() };
		assert(BlockByteSize);

		BinaryData.ReadUint16(m_FontFace.Common.LineHeight);
		BinaryData.ReadUint16(m_FontFace.Common.Base);
		BinaryData.ReadUint16(m_FontFace.Common.ScaleW);
		BinaryData.ReadUint16(m_FontFace.Common.ScaleH);
		BinaryData.ReadUint16(m_FontFace.Common.Pages);

		BinaryData.ReadByte(m_FontFace.Common.BitField);

		BinaryData.ReadUint8(m_FontFace.Common.AlphaChannel);
		BinaryData.ReadUint8(m_FontFace.Common.RedChannel);
		BinaryData.ReadUint8(m_FontFace.Common.GreenChannel);
		BinaryData.ReadUint8(m_FontFace.Common.BlueChannel);
	}

	// Block #2 pages
	{
		byte BlockID{ BinaryData.ReadByte() };
		uint32_t BlockByteSize{ BinaryData.ReadUint32() };
		assert(BlockByteSize);

		for (uint16_t iPage = 0; iPage < m_FontFace.Common.Pages; ++iPage)
		{
			BinaryData.ReadNullTerminatedString(ReadString);
			ReadString = Directory + ReadString; // @important
			m_FontFace.vPages.emplace_back(ReadString);
		}
	}

	// Block #3 chars
	{
		byte BlockID{ BinaryData.ReadByte() };
		uint32_t BlockByteSize{ BinaryData.ReadUint32() };
		assert(BlockByteSize);

		uint32_t CharCount{ BlockByteSize / 20 };

		for (uint32_t iChar = 0; iChar < CharCount; ++iChar)
		{
			SBMFontData::SChar Char{};
			BinaryData.ReadUint32(Char.ID);
			BinaryData.ReadUint16(Char.X);
			BinaryData.ReadUint16(Char.Y);
			BinaryData.ReadUint16(Char.Width);
			BinaryData.ReadUint16(Char.Height);
			BinaryData.ReadInt16(Char.XOffset);
			BinaryData.ReadInt16(Char.YOffset);
			BinaryData.ReadInt16(Char.XAdvance);
			BinaryData.ReadUint8(Char.Page);
			BinaryData.ReadUint8(Char.Channel);

			m_FontFace.vChars.emplace_back(Char);
		}
	}

	// Block #4 kerning pairs
	{
		byte BlockID{ BinaryData.ReadByte() };
		uint32_t BlockByteSize{ BinaryData.ReadUint32() };

		if (BlockByteSize)
		{
			uint32_t KerningPairCount{ BlockByteSize / 10 };
			for (uint32_t iKerningPair = 0; iKerningPair < KerningPairCount; ++iKerningPair)
			{
				SBMFontData::SKerningPair KerningPair{};
				BinaryData.ReadUint32(KerningPair.FirstID);
				BinaryData.ReadUint32(KerningPair.SecondID);
				BinaryData.ReadInt16(KerningPair.Amount);
				m_FontFace.vKerningPairs.emplace_back(KerningPair);
			}
		}
	}

	size_t iChar{};
	for (auto& Char : m_FontFace.vChars)
	{
		Char.ID = ConvertToUTF8((wchar_t)Char.ID);

		m_mapIDToCharsIndex[Char.ID] = iChar;
		++iChar;
	}
}

const SBMFontData& CBMFont::GetData() const
{
	return m_FontFace;
}

const std::map<uint32_t, size_t>& CBMFont::GetCharIndexMap() const
{
	return m_mapIDToCharsIndex;
}
