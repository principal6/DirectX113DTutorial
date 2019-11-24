#include "BinaryFile.h"

using std::string;
using std::ifstream;
using std::ofstream;

CBinaryFile::~CBinaryFile()
{
	Close();
}

void CBinaryFile::OpenToRead(const string FileName)
{
	m_IFStream.close();

	m_IFStream.open(FileName, ofstream::binary);
	if (!m_IFStream.is_open())
	{
		MB_WARN("파일을 열 수 없습니다.", "파일 열기 실패");
		return;
	}
}

void CBinaryFile::OpenToWrite(const string FileName)
{
	m_OFStream.close();

	m_OFStream.open(FileName, ofstream::binary);
	if (!m_OFStream.is_open())
	{
		MB_WARN("파일을 열 수 없습니다.", "파일 열기 실패");
		return;
	}
}

void CBinaryFile::Close()
{
	m_IFStream.close();
	m_OFStream.close();
}

void CBinaryFile::WriteBool(bool Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.put(Value);
}

void CBinaryFile::WriteInt8(int8_t Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.put(Value);
}

void CBinaryFile::WriteInt16(int16_t Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(int16_t));
}

void CBinaryFile::WriteInt32(int32_t Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(int32_t));
}

void CBinaryFile::WriteUInt8(uint8_t Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.put(Value);
}

void CBinaryFile::WriteUInt16(uint16_t Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(uint16_t));
}

void CBinaryFile::WriteUInt32(uint32_t Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(uint32_t));
}

void CBinaryFile::WriteFloat(float Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(float));
}

void CBinaryFile::WriteXMFLOAT2(const XMFLOAT2& Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(XMFLOAT2));
}

void CBinaryFile::WriteXMFLOAT3(const XMFLOAT3& Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(XMFLOAT3));
}

void CBinaryFile::WriteXMFLOAT4(const XMFLOAT4& Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(XMFLOAT4));
}

void CBinaryFile::WriteXMVECTOR(const XMVECTOR& Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write((const char*)&Value, sizeof(XMVECTOR));
}

void CBinaryFile::WriteString(const string& Value)
{
	if (!m_OFStream.is_open()) return;
	m_OFStream.write(Value.c_str(), Value.length());
}

void CBinaryFile::WriteString(const string& Value, int32_t Length)
{
	if (!m_OFStream.is_open()) return;
	if (Length <= 0) return;

	string Resized{ Value };
	Resized.resize(Length);
	m_OFStream.write(Resized.c_str(), Length);
}

bool CBinaryFile::ReadBool()
{
	return (bool)m_IFStream.get();
}

int8_t CBinaryFile::ReadInt8()
{
	return (int8_t)m_IFStream.get();
}

int16_t CBinaryFile::ReadInt16()
{
	constexpr int KByteCount{ 2 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	int16_t Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

int32_t CBinaryFile::ReadInt32()
{
	constexpr int KByteCount{ 4 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	int32_t Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

uint8_t CBinaryFile::ReadUInt8()
{
	return (uint8_t)m_IFStream.get();
}

uint16_t CBinaryFile::ReadUInt16()
{
	constexpr int KByteCount{ 2 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	uint16_t Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

uint32_t CBinaryFile::ReadUInt32()
{
	constexpr int KByteCount{ 4 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	uint32_t Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

float CBinaryFile::ReadFloat()
{
	constexpr int KByteCount{ 4 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	float Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

XMFLOAT2 CBinaryFile::ReadXMFLOAT2()
{
	constexpr int KByteCount{ 4 * 2 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	XMFLOAT2 Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

XMFLOAT3 CBinaryFile::ReadXMFLOAT3()
{
	constexpr int KByteCount{ 4 * 3 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	XMFLOAT3 Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

XMFLOAT4 CBinaryFile::ReadXMFLOAT4()
{
	constexpr int KByteCount{ 4 * 4 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	XMFLOAT4 Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

XMVECTOR CBinaryFile::ReadXMVECTOR()
{
	constexpr int KByteCount{ 4 * 4 };
	char Bytes[KByteCount]{};
	m_IFStream.read(Bytes, KByteCount);

	XMVECTOR Result{};
	memcpy(&Result, Bytes, KByteCount);

	return Result;
}

string CBinaryFile::ReadString(int32_t Length)
{
	// @important: allocation size is (@Length + 1) in order to properly read non-zero-terminated string
	char* DString{ new char[Length + 1]{} };

	m_IFStream.read(DString, Length);

	string Result{ DString };
	
	delete[] DString;
	DString = nullptr;

	return Result;
}
