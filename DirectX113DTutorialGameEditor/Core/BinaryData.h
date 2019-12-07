#pragma once

#include "SharedHeader.h"

class CBinaryData
{
public:
	CBinaryData() {}
	CBinaryData(const std::vector<byte>& vBytes) : m_vBytes{ vBytes } {}
	~CBinaryData() {}

public:
	void Clear();
	bool LoadFromFile(const std::string FileName);
	bool SaveToFile(const std::string FileName);

public:
	void WriteBool(bool Value);
	void WriteInt8(int8_t Value);
	void WriteInt16(int16_t Value);
	void WriteInt32(int32_t Value);
	void WriteUint8(uint8_t Value);
	void WriteUint16(uint16_t Value);
	void WriteUint32(uint32_t Value);
	void WriteFloat(float Value);
	void WriteXMFLOAT2(const XMFLOAT2& Value);
	void WriteXMFLOAT3(const XMFLOAT3& Value);
	void WriteXMFLOAT4(const XMFLOAT4& Value);
	void WriteXMVECTOR(const XMVECTOR& Value);
	void WriteXMMATRIX(const XMMATRIX& Value);
	void WriteString(const std::string& String);
	void WriteString(const std::string& String, size_t FixedLength);
	// @important: string length is written in <uint32_t>
	void WriteStringWithPrefixedLength(const std::string& String);

public:
	bool ReadSkip(size_t SkippingByteCount);
	bool ReadBytes(size_t ByteCount, std::vector<byte>& Out);
	bool ReadBool(bool& Out);
	bool ReadInt8(int8_t& Out);
	bool ReadInt16(int16_t& Out);
	bool ReadInt32(int32_t& Out);
	bool ReadUint8(uint8_t& Out);
	bool ReadUint16(uint16_t& Out);
	bool ReadUint32(uint32_t& Out);
	bool ReadFloat(float& Out);
	bool ReadXMFLOAT2(XMFLOAT2& Out);
	bool ReadXMFLOAT3(XMFLOAT3& Out);
	bool ReadXMFLOAT4(XMFLOAT4& Out);
	bool ReadXMVECTOR(XMVECTOR& Out);
	bool ReadXMMATRIX(XMMATRIX& Out);
	bool ReadString(std::string& Out, uint32_t Length);
	bool ReadStringWithPrefixedLength(std::string& OutString);
	bool ReadStringWithPrefixedLength(uint32_t& OutLength, std::string& OutString);

// @important: below are for more convenient reading of built-in types, though less safer
	bool ReadBool();
	int8_t ReadInt8();
	int16_t ReadInt16();
	int32_t ReadInt32();
	uint8_t ReadUint8();
	uint16_t ReadUint16();
	uint32_t ReadUint32();
	float ReadFloat();

public:
	void AppendBytes(const std::vector<byte>& SrcBytes);
	const std::vector<byte> GetBytes() const;

private:
	static constexpr size_t KBoolByteCount{ 1 };
	static constexpr size_t KInt8ByteCount{ 1 };
	static constexpr size_t KInt16ByteCount{ 2 };
	static constexpr size_t KInt32ByteCount{ 4 };
	static constexpr size_t KUint8ByteCount{ 1 };
	static constexpr size_t KUint16ByteCount{ 2 };
	static constexpr size_t KUint32ByteCount{ 4 };
	static constexpr size_t KFloatByteCount{ 4 };
	static constexpr size_t KXMFLOAT2ByteCount{ 4 * 2 };
	static constexpr size_t KXMFLOAT3ByteCount{ 4 * 3 };
	static constexpr size_t KXMFLOAT4ByteCount{ 4 * 4 };
	static constexpr size_t KXMVECTORByteCount{ 4 * 4 };
	static constexpr size_t KXMMATRIXByteCount{ 4 * 16 };

private:
	std::vector<byte>	m_vBytes{};
	size_t				m_ReadByteOffset{};
};