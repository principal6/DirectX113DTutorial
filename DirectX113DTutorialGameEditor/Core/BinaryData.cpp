#include "BinaryData.h"
#include <fstream>

void CBinaryData::Clear()
{
	m_vBytes.clear();
	m_ReadByteOffset = 0;
}

bool CBinaryData::LoadFromFile(const std::string FileName)
{
	m_ReadByteOffset = 0;

	std::ifstream ifs{ FileName, std::ios::binary };
	if (ifs.is_open())
	{
		ifs.seekg(0, ifs.end);
		size_t ByteCount{ (size_t)ifs.tellg() };
		ifs.seekg(0, ifs.beg);

		m_vBytes.resize(ByteCount);
		ifs.read((char*)&m_vBytes[0], ByteCount);

		ifs.close();
		return true;
	}
	return false;
}

bool CBinaryData::SaveToFile(const std::string FileName)
{
	m_ReadByteOffset = 0;

	std::ofstream ofs{ FileName, std::ios::binary };
	if (ofs.is_open())
	{
		ofs.write((const char*)&m_vBytes[0], m_vBytes.size());

		ofs.close();
		return true;
	}
	return false;
}

void CBinaryData::WriteBool(bool Value)
{
	m_vBytes.emplace_back((Value == true) ? 0xBB : 0x00);
}

void CBinaryData::WriteInt8(int8_t Value)
{
	byte Byte{};
	memcpy(&Byte, &Value, KInt8ByteCount);
	m_vBytes.emplace_back(Byte);
}

void CBinaryData::WriteInt16(int16_t Value)
{
	byte Bytes[KInt16ByteCount]{};
	memcpy(Bytes, &Value, KInt16ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteInt32(int32_t Value)
{
	byte Bytes[KInt32ByteCount]{};
	memcpy(Bytes, &Value, KInt32ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteUint8(uint8_t Value)
{
	m_vBytes.emplace_back(Value);
}

void CBinaryData::WriteUint16(uint16_t Value)
{
	byte Bytes[KUint16ByteCount]{};
	memcpy(Bytes, &Value, KUint16ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteUint32(uint32_t Value)
{
	byte Bytes[KUint32ByteCount]{};
	memcpy(Bytes, &Value, KUint32ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteFloat(float Value)
{
	byte Bytes[KFloatByteCount]{};
	memcpy(Bytes, &Value, KFloatByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteXMFLOAT2(const XMFLOAT2& Value)
{
	byte Bytes[KXMFLOAT2ByteCount]{};
	memcpy(Bytes, &Value, KXMFLOAT2ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteXMFLOAT3(const XMFLOAT3& Value)
{
	byte Bytes[KXMFLOAT3ByteCount]{};
	memcpy(Bytes, &Value, KXMFLOAT3ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteXMFLOAT4(const XMFLOAT4& Value)
{
	byte Bytes[KXMFLOAT4ByteCount]{};
	memcpy(Bytes, &Value, KXMFLOAT4ByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteXMVECTOR(const XMVECTOR& Value)
{
	byte Bytes[KXMVECTORByteCount]{};
	memcpy(Bytes, &Value, KXMVECTORByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteXMMATRIX(const XMMATRIX& Value)
{
	byte Bytes[KXMMATRIXByteCount]{};
	memcpy(Bytes, &Value, KXMMATRIXByteCount);
	for (const byte& Byte : Bytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

void CBinaryData::WriteString(const std::string& String)
{
	m_vBytes.reserve(m_vBytes.size() + String.size());
	for (const auto& Character : String)
	{
		m_vBytes.emplace_back(Character);
	}
}

void CBinaryData::WriteString(const std::string& String, size_t FixedLength)
{
	m_vBytes.reserve(m_vBytes.size() + FixedLength);
	for (size_t iPosition = 0; iPosition < FixedLength; ++iPosition)
	{
		if (iPosition < String.size())
		{
			m_vBytes.emplace_back(String[iPosition]);
		}
		else
		{
			m_vBytes.emplace_back(0);
		}
	}
}

void CBinaryData::WriteStringWithPrefixedLength(const std::string& String)
{
	WriteUint32((uint32_t)String.size());
	WriteString(String);
}

bool CBinaryData::ReadSkip(size_t SkippingByteCount)
{
	if (m_ReadByteOffset + SkippingByteCount - 1 >= m_vBytes.size()) return false;

	m_ReadByteOffset += SkippingByteCount;
	return true;
}

bool CBinaryData::ReadBytes(size_t ByteCount, std::vector<byte>& Out)
{
	if (m_ReadByteOffset + ByteCount - 1 >= m_vBytes.size()) return false;

	Out.clear();
	Out.reserve(ByteCount);
	for (size_t iByte = 0; iByte < ByteCount; ++iByte)
	{
		Out.emplace_back(m_vBytes[m_ReadByteOffset + iByte]);
	}
	m_ReadByteOffset += ByteCount;

	return true;
}

bool CBinaryData::ReadBool(bool& Out)
{
	if (m_ReadByteOffset >= m_vBytes.size()) return false;

	Out = (m_vBytes[m_ReadByteOffset] == 0) ? false : true;
	m_ReadByteOffset += KBoolByteCount;
	return true;
}

bool CBinaryData::ReadInt8(int8_t& Out)
{
	if (m_ReadByteOffset >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KInt8ByteCount);

	m_ReadByteOffset += KInt8ByteCount;
	return true;
}

bool CBinaryData::ReadInt16(int16_t& Out)
{
	if (m_ReadByteOffset + KInt16ByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KInt16ByteCount);

	m_ReadByteOffset += KInt16ByteCount;
	return true;
}

bool CBinaryData::ReadInt32(int32_t& Out)
{
	if (m_ReadByteOffset + KInt32ByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KInt32ByteCount);

	m_ReadByteOffset += KInt32ByteCount;
	return true;
}

bool CBinaryData::ReadUint8(uint8_t& Out)
{
	if (m_ReadByteOffset >= m_vBytes.size()) return false;

	Out = m_vBytes[m_ReadByteOffset];
	m_ReadByteOffset += KUint8ByteCount;
	return true;
}

bool CBinaryData::ReadUint16(uint16_t& Out)
{
	if (m_ReadByteOffset + KUint16ByteCount - 1 >= m_vBytes.size()) return false;
	
	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KUint16ByteCount);
	
	m_ReadByteOffset += KUint16ByteCount;
	return true;
}

bool CBinaryData::ReadUint32(uint32_t& Out)
{
	if (m_ReadByteOffset + KUint32ByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KUint32ByteCount);

	m_ReadByteOffset += KUint32ByteCount;
	return true;
}

bool CBinaryData::ReadFloat(float& Out)
{
	if (m_ReadByteOffset + KFloatByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KFloatByteCount);

	m_ReadByteOffset += KFloatByteCount;
	return true;
}

bool CBinaryData::ReadXMFLOAT2(XMFLOAT2& Out)
{
	if (m_ReadByteOffset + KXMFLOAT2ByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KXMFLOAT2ByteCount);

	m_ReadByteOffset += KXMFLOAT2ByteCount;
	return true;
}

bool CBinaryData::ReadXMFLOAT3(XMFLOAT3& Out)
{
	if (m_ReadByteOffset + KXMFLOAT3ByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KXMFLOAT3ByteCount);

	m_ReadByteOffset += KXMFLOAT3ByteCount;
	return true;
}

bool CBinaryData::ReadXMFLOAT4(XMFLOAT4& Out)
{
	if (m_ReadByteOffset + KXMFLOAT4ByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KXMFLOAT4ByteCount);

	m_ReadByteOffset += KXMFLOAT4ByteCount;
	return true;
}

bool CBinaryData::ReadXMVECTOR(XMVECTOR& Out)
{
	if (m_ReadByteOffset + KXMVECTORByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KXMVECTORByteCount);

	m_ReadByteOffset += KXMVECTORByteCount;
	return true;
}

bool CBinaryData::ReadXMMATRIX(XMMATRIX& Out)
{
	if (m_ReadByteOffset + KXMMATRIXByteCount - 1 >= m_vBytes.size()) return false;

	memcpy(&Out, &m_vBytes[m_ReadByteOffset], KXMMATRIXByteCount);

	m_ReadByteOffset += KXMMATRIXByteCount;
	return true;
}

bool CBinaryData::ReadString(std::string& Out, uint32_t Length)
{
	if (Length == 0) return false;
	if (m_ReadByteOffset + Length - 1 >= m_vBytes.size()) return false;

	Out.clear();
	Out.reserve(Length);
	for (uint32_t iPosition = 0; iPosition < Length; ++iPosition)
	{
		Out += m_vBytes[m_ReadByteOffset + iPosition];
	}

	m_ReadByteOffset += Length;
	return true;
}

bool CBinaryData::ReadStringWithPrefixedLength(std::string& OutString)
{
	uint32_t ReadStringLength{};
	if (ReadUint32(ReadStringLength))
	{
		return ReadString(OutString, ReadStringLength);
	}
	return false;
}

bool CBinaryData::ReadStringWithPrefixedLength(uint32_t& OutLength, std::string& OutString)
{
	if (ReadUint32(OutLength))
	{
		return ReadString(OutString, OutLength);
	}
	return false;
}

bool CBinaryData::ReadBool()
{
	bool Value{};
	ReadBool(Value);
	return Value;
}

int8_t CBinaryData::ReadInt8()
{
	int8_t Value{};
	ReadInt8(Value);
	return Value;
}

int16_t CBinaryData::ReadInt16()
{
	int16_t Value{};
	ReadInt16(Value);
	return Value;
}

int32_t CBinaryData::ReadInt32()
{
	int32_t Value{};
	ReadInt32(Value);
	return Value;
}

uint8_t CBinaryData::ReadUint8()
{
	uint8_t Value{};
	ReadUint8(Value);
	return Value;
}

uint16_t CBinaryData::ReadUint16()
{
	uint16_t Value{};
	ReadUint16(Value);
	return Value;
}

uint32_t CBinaryData::ReadUint32()
{
	uint32_t Value{};
	ReadUint32(Value);
	return Value;
}

float CBinaryData::ReadFloat()
{
	float Value{};
	ReadFloat(Value);
	return Value;
}

void CBinaryData::AppendBytes(const std::vector<byte>& SrcBytes)
{
	m_vBytes.reserve(m_vBytes.size() + SrcBytes.size());
	for (const byte& Byte : SrcBytes)
	{
		m_vBytes.emplace_back(Byte);
	}
}

const std::vector<byte> CBinaryData::GetBytes() const
{
	return m_vBytes;
}
