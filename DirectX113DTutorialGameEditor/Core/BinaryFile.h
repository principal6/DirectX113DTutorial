#pragma once

#include "SharedHeader.h"
#include <fstream>

class CBinaryFile
{
public:
	CBinaryFile() {}
	~CBinaryFile();

public:
	void OpenToRead(const std::string FileName);
	void OpenToWrite(const std::string FileName);
	void Close();

public:
	void WriteBool(bool Value);
	void WriteInt8(int8_t Value);
	void WriteInt16(int16_t Value);
	void WriteInt32(int32_t Value);
	void WriteUInt8(uint8_t Value);
	void WriteUInt16(uint16_t Value);
	void WriteUInt32(uint32_t Value);
	void WriteFloat(float Value);
	void WriteXMFLOAT2(const XMFLOAT2& Value);
	void WriteXMFLOAT3(const XMFLOAT3& Value);
	void WriteXMFLOAT4(const XMFLOAT4& Value);
	void WriteXMVECTOR(const XMVECTOR& Value);
	void WriteString(const std::string& Value);

	// if (@Length <= 0), this function doesn't write anything to the file.
	void WriteString(const std::string& Value, int32_t Length);

public:
	bool ReadBool();
	int8_t ReadInt8();
	int16_t ReadInt16();
	int32_t ReadInt32();
	uint8_t ReadUInt8();
	uint16_t ReadUInt16();
	uint32_t ReadUInt32();
	float ReadFloat();
	XMFLOAT2 ReadXMFLOAT2();
	XMFLOAT3 ReadXMFLOAT3();
	XMFLOAT4 ReadXMFLOAT4();
	XMVECTOR ReadXMVECTOR();
	std::string ReadString(int32_t Length);
	
private:
	std::ofstream m_OFStream{};
	std::ifstream m_IFStream{};
};