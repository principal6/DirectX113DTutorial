#pragma once

#include <cstdint>

union UUTF8
{
	uint32_t	UInt32{};
	char		Chars[4];
};

uint32_t ConvertToUTF8(wchar_t Char);
uint32_t GetUTF8CharacterByteCount(char FirstCharacter);
size_t GetUTF8StringLength(const char* UTF8String);
