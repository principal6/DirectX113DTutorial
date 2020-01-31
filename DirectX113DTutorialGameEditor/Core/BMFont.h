#pragma once

#include "SharedHeader.h"

// BMFont from [www.angelcode.com]

struct SBMFontData
{
	// BMFont information
	struct SInfo
	{
		uint16_t	FontSize{};		// The size of the true type font
		byte		BitField{};		// 0: smooth, 1: unicode, 2: italic, 3: bold, 4: fixedHeigth, 5-7: reserved
		uint8_t		CharSet{};		// The name of the OEM charset used(when not unicode)
		uint16_t	StretchH{};		// The font height stretch in percentage. 100% means no stretch
		uint8_t		AA{};			// The supersampling level used. 1 means no supersampling was used
		uint8_t		PaddingUp{};	// The padding for each character
		uint8_t		PaddingRight{};	// The padding for each character
		uint8_t		PaddingDown{};	// The padding for each character
		uint8_t		PaddingLeft{};	// The padding for each character
		uint8_t		SpacingHorz{};	// The spacing for each character
		uint8_t		SpacingVert{};	// The spacing for each character
		uint8_t		Outline{};		// The outline thickness for the characters
		std::string	FontName{};		// name of the font face
	};

	// Data shared by all the glyphs
	struct SCommon
	{
		uint16_t	LineHeight{};	// current face's line height (this value moves the CURSOR VERTICALLY)
		uint16_t	Base{};			// vertical offset of the base line from the font face's top line
		uint16_t	ScaleW{};		// The width of the texture, normally used to scale the x pos of the character image
		uint16_t	ScaleH{};		// The height of the texture, normally used to scale the y pos of the character image
		uint16_t	Pages{};		// total number of pages
		byte		BitField{};		// 0-6: reserved, 7: packed
		uint8_t		AlphaChannel{}; // 0 (glyph data) 1 (outline) 2 (glyph and outline) 3 (set to zero), and 4 (set to one)
		uint8_t		RedChannel{};	// 0 (glyph data) 1 (outline) 2 (glyph and outline) 3 (set to zero), and 4 (set to one)
		uint8_t		GreenChannel{};	// 0 (glyph data) 1 (outline) 2 (glyph and outline) 3 (set to zero), and 4 (set to one)
		uint8_t		BlueChannel{};	// 0 (glyph data) 1 (outline) 2 (glyph and outline) 3 (set to zero), and 4 (set to one)
	};

	// Glyph data for each encoded character
	// ID will be UTF-8 encoded
	struct SChar
	{
		uint32_t	ID{};			// glyph's ID (UTF-8 encoded)
		uint16_t	X{};			// glyph's horizontal position in texture
		uint16_t	Y{};			// glyph's vertical position in texture
		uint16_t	Width{};		// glyph's width in texture
		uint16_t	Height{};		// glyph's height in texture
		int16_t		XOffset{};		// horizontal offset of the left side of the glyph, from the current cursor's x position
		int16_t		YOffset{};		// vertical offset of the upper side of the glyph, from the current face's top line
		int16_t		XAdvance{};		// this is the amount that moves the CURSOR HORIZONTALLY to make it get to its next position
		uint8_t		Page{};			// index of page the glyph is on
		uint8_t		Channel{};		// texture channel where the character image is found (1 = blue, 2 = green, 4 = red, 8 = alpha, 15 = all channels).
	};

	// When specific pair of two glyphs gathers,
	// the second glyph needs to be repositioned horizontally
	// its amount is stores in this structure
	struct SKerningPair
	{
		uint32_t	FirstID{};		// first glyph's ID
		uint32_t	SecondID{};		// second glyph's ID
		int16_t		Amount{};		// the amount the second glyph needs to be moved horizontally
	};

	SInfo						Info{};
	SCommon						Common{};
	std::vector<std::string>	vPages{};

	// UTF-8 encoded characters
	std::vector<SChar>			vChars{};

	std::vector<SKerningPair>	vKerningPairs{};
};

class CBMFont
{
public:
	CBMFont();
	~CBMFont();

public:
	void Load(const std::string& FNT_FileName);
	const SBMFontData& GetData() const;
	const std::map<uint32_t, size_t>& GetCharIndexMap() const;
private:
	SBMFontData					m_FontFace{};
	std::map<uint32_t, size_t>	m_mapIDToCharsIndex{};
};
