#pragma once

#include "Object3D.h"
#include <fstream>

// ###########################
// << SMOD FILE STRUCTURE >>
// 8B SMOD Signature
// ##### MATERIAL #####
// 1B (uint8_t) Material count (559B == Each material)
// # 1B (uint8_t) Material index
// # 512B (string, MAX) Material name
// # 1B (bool) bHasTexture
// # 12B (XMFLOAT3) ambient
// # 12B (XMFLOAT3) diffuse
// # 12B (XMFLOAT3) specular
// # 4B (float) specular exponent
// # 4B (float) specular intensity
// # 1B (bool) bShouldGenerateAutoMipMap
// # 512B (string, MAX) diffuse texture file name
// # 512B (string, MAX) normal texture file name
// ##### MESH #####
// 1B (uint8_t) Mesh count
// # 1B (uint8_t) Mesh index
// # ### MATERIAL ID ###
// # 1B (uint8_t) Material id
// # ### VERTEX ###
// 4B (uint32_t) Vertex count
// # 4B (uint32_t) Vertex index
// # 16B (XMVECTOR) position
// # 16B (XMVECTOR) color
// # 16B (XMVECTOR) texcoord
// # 16B (XMVECTOR) normal
// # ### TRIANGLE ###
// 4B (uint32_t) Triangle count
// # 4B (uint32_t) Triangle index
// # 4B (uint32_t) vid 0
// # 4B (uint32_t) vid 1
// # 4B (uint32_t) vid 2
// ###########################

#define READ_BYTES(ByteCount) memset(ReadBytes, 0, sizeof(ReadBytes)); ifs.read(ReadBytes, ByteCount);
#define READ_BYTES_TO_BOOL GetBoolFromBtyes(ReadBytes)
#define READ_BYTES_TO_UINT8 GetUint8FromBtyes(ReadBytes)
#define READ_BYTES_TO_UINT32 GetUint32FromBtyes(ReadBytes)
#define READ_BYTES_TO_FLOAT GetFloatFromBtyes(ReadBytes)
#define READ_BYTES_TO_XMFLOAT3 GetXMFLOAT3FromBtyes(ReadBytes)
#define READ_BYTES_TO_XMFLOAT4 GetXMFLOAT4FromBtyes(ReadBytes)
#define READ_BYTES_TO_XMVECTOR GetXMVECTORFromBtyes(ReadBytes)

#define WRITE_UINT32_TO_BYTES(Value) GetBytesFromUint32((uint32_t)Value, Uint32Bytes); ofs.write(Uint32Bytes, sizeof(Uint32Bytes))
#define WRITE_FLOAT_TO_BYTES(Value) GetBytesFromFloat((float)Value, FloatBytes); ofs.write(FloatBytes, sizeof(FloatBytes))
#define WRITE_XMFLOAT3_TO_BYTES(Value) GetBytesFromXMFLOAT3(Value, XMFLOAT3Bytes); ofs.write(XMFLOAT3Bytes, sizeof(XMFLOAT3Bytes))
#define WRITE_XMFLOAT4_TO_BYTES(Value) GetBytesFromXMFLOAT4(Value, XMFLOAT4Bytes); ofs.write(XMFLOAT4Bytes, sizeof(XMFLOAT4Bytes))
#define WRITE_XMVECTOR_TO_BYTES(Value) GeBytesFromtXMVECTOR(Value, XMVECTORBytes); ofs.write(XMVECTORBytes, sizeof(XMVECTORBytes))

static void GetBytesFromUint32(uint32_t Value, char(&Bytes)[4])
{
	constexpr size_t KSize{ sizeof(char) * 4 };
	memset(&Bytes[0], 0, KSize);
	memcpy(&Bytes[0], &Value, KSize);
}

static void GetBytesFromFloat(float Value, char (&Bytes)[4])
{
	constexpr size_t KSize{ sizeof(char) * 4 };
	memset(&Bytes[0], 0, KSize);
	memcpy(&Bytes[0], &Value, KSize);
}

static void GetBytesFromXMFLOAT3(const XMFLOAT3& Value, char(&Bytes)[12])
{
	constexpr size_t KSize{ sizeof(char) * 12 };
	memset(&Bytes[0], 0, KSize);
	memcpy(&Bytes[0], &Value.x, KSize);
}

static void GetBytesFromXMFLOAT4(const XMFLOAT4& Value, char(&Bytes)[16])
{
	constexpr size_t KSize{ sizeof(char) * 16 };
	memset(&Bytes[0], 0, KSize);
	memcpy(&Bytes[0], &Value.x, KSize);
}

static void GeBytesFromtXMVECTOR(const XMVECTOR& Value, char(&Bytes)[16])
{
	constexpr size_t KSizePerDigit{ sizeof(char) * 4 };
	float X{ XMVectorGetX(Value) };
	float Y{ XMVectorGetY(Value) };
	float Z{ XMVectorGetZ(Value) };
	float W{ XMVectorGetW(Value) };

	memset(&Bytes[0], 0, KSizePerDigit * 4);
	
	memcpy(&Bytes[4 * 0], &X, KSizePerDigit);
	memcpy(&Bytes[4 * 1], &Y, KSizePerDigit);
	memcpy(&Bytes[4 * 2], &Z, KSizePerDigit);
	memcpy(&Bytes[4 * 3], &W, KSizePerDigit);
}

static bool GetBoolFromBtyes(char(&Bytes)[512])
{
	return (bool)Bytes[0];
}

static uint8_t GetUint8FromBtyes(char(&Bytes)[512])
{
	return Bytes[0];
}

static uint32_t GetUint32FromBtyes(char(&Bytes)[512])
{
	uint32_t Result{};
	memcpy(&Result, &Bytes[0], sizeof(uint32_t));
	return Result;
}

static float GetFloatFromBtyes(char(&Bytes)[512])
{
	float Result{};
	memcpy(&Result, &Bytes[0], sizeof(float));
	return Result;
}

static XMFLOAT3 GetXMFLOAT3FromBtyes(char(&Bytes)[512])
{
	XMFLOAT3 Result{};
	memcpy(&Result, &Bytes[0], sizeof(XMFLOAT3));
	return Result;
}

static XMFLOAT4 GetXMFLOAT4FromBtyes(char(&Bytes)[512])
{
	XMFLOAT4 Result{};
	memcpy(&Result, &Bytes[0], sizeof(XMFLOAT4));
	return Result;
}

static XMVECTOR GetXMVECTORFromBtyes(char(&Bytes)[512])
{
	float X{}, Y{}, Z{}, W{};

	memcpy(&X, &Bytes[4 * 0], sizeof(float));
	memcpy(&Y, &Bytes[4 * 1], sizeof(float));
	memcpy(&Z, &Bytes[4 * 2], sizeof(float));
	memcpy(&W, &Bytes[4 * 3], sizeof(float));

	return XMVectorSet(X, Y, Z, W);
}

static void _ReadStaticModelFile(std::ifstream& ifs, SModel& Model)
{
	char ReadBytes[512]{};

	// ##### MATERIAL #####
	// 1B (uint8_t) Material count
	READ_BYTES(1);
	Model.vMaterials.resize(READ_BYTES_TO_UINT32);
	for (CMaterial& Material : Model.vMaterials)
	{
		// # 1B (uint8_t) Material index
		READ_BYTES(1);

		// # 512B (string, MAX) Material name
		READ_BYTES(512);
		Material.SetName(ReadBytes);

		// # 1B (bool) has texture ( automatically set by SetTextureFileName() )
		READ_BYTES(1);
		READ_BYTES_TO_BOOL;

		// # 12B (XMFLOAT3) ambient
		READ_BYTES(12);
		Material.SetAmbientColor(READ_BYTES_TO_XMFLOAT3);

		// # 12B (XMFLOAT3) diffuse
		READ_BYTES(12);
		Material.SetDiffuseColor(READ_BYTES_TO_XMFLOAT3);

		// # 12B (XMFLOAT3) specular
		READ_BYTES(12);
		Material.SetSpecularColor(READ_BYTES_TO_XMFLOAT3);

		// # 4B (float) specular exponent
		READ_BYTES(4);
		Material.SetSpecularExponent(READ_BYTES_TO_FLOAT);

		// # 4B (float) specular intensity
		READ_BYTES(4);
		Material.SetSpecularIntensity(READ_BYTES_TO_FLOAT);

		// # 1B (bool) bShouldGenerateAutoMipMap
		READ_BYTES(1);
		Material.SetbShouldGenerateAutoMipMap(READ_BYTES_TO_BOOL);

		// # 512B (string, MAX) diffuse texture file name
		READ_BYTES(512);
		Material.SetDiffuseTextureFileName(ReadBytes);

		// # 512B (string, MAX) normal texture file name
		READ_BYTES(512);
		Material.SetNormalTextureFileName(ReadBytes);
	}

	// ##### MESH #####
	// 1B (uint8_t) Mesh count
	READ_BYTES(1);
	Model.vMeshes.resize(READ_BYTES_TO_UINT8);
	for (SMesh& Mesh : Model.vMeshes)
	{
		// # 1B (uint8_t) Mesh index
		READ_BYTES(1);

		// # 1B (uint8_t) Material id
		READ_BYTES(1);
		Mesh.MaterialID = READ_BYTES_TO_UINT8;

		// # ### VERTEX ###
		// 4B (uint32_t) Vertex count
		READ_BYTES(4);
		Mesh.vVertices.resize(READ_BYTES_TO_UINT32);
		for (SVertex3D& Vertex : Mesh.vVertices)
		{
			// # 4B (uint32_t) Vertex index
			READ_BYTES(4);

			// # 16B (XMVECTOR) position
			READ_BYTES(16);
			Vertex.Position = READ_BYTES_TO_XMVECTOR;

			// # 16B (XMVECTOR) color
			READ_BYTES(16);
			Vertex.Color = READ_BYTES_TO_XMVECTOR;

			// # 16B (XMVECTOR) texcoord
			READ_BYTES(16);
			Vertex.TexCoord = READ_BYTES_TO_XMVECTOR;

			// # 16B (XMVECTOR) normal
			READ_BYTES(16);
			Vertex.Normal = READ_BYTES_TO_XMVECTOR;
		}

		// # ### TRIANGLE ###
		// 4B (uint32_t) Triangle count
		READ_BYTES(4);
		Mesh.vTriangles.resize(READ_BYTES_TO_UINT32);
		for (STriangle& Triangle : Mesh.vTriangles)
		{
			// # 4B (uint32_t) Triangle index
			READ_BYTES(4);

			// # 4B (uint32_t) vid 0
			READ_BYTES(4);
			Triangle.I0 = READ_BYTES_TO_UINT32;

			// # 4B (uint32_t) vid 1
			READ_BYTES(4);
			Triangle.I1 = READ_BYTES_TO_UINT32;

			// # 4B (uint32_t) vid 2
			READ_BYTES(4);
			Triangle.I2 = READ_BYTES_TO_UINT32;
		}
	}
}

static SModel ImportStaticModel(const string& FileName)
{
	std::ifstream ifs{};
	ifs.open(FileName, std::ofstream::binary);
	assert(ifs.is_open());

	char ReadBytes[512]{};

	// 8B Signature
	READ_BYTES(8);

	SModel Model{};
	_ReadStaticModelFile(ifs, Model);

	return Model;
}

static void _WriteStaticModelFile(std::ofstream& ofs, const SModel& Model)
{
	char StringBytes[512]{};
	char Uint32Bytes[4]{};
	char FloatBytes[4]{};
	char XMFLOAT3Bytes[12]{};
	char XMVECTORBytes[16]{};
	uint8_t iMaterial{};

	// 1B (uint8_t) Material count
	ofs.put((uint8_t)Model.vMaterials.size());

	// 559B == Each material
	for (const CMaterial& Material : Model.vMaterials)
	{
		// 1B (uint8_t) Material index
		ofs.put(iMaterial);

		// 512B (string, MAX) Material name
		memset(StringBytes, 0, 512);
		memcpy(StringBytes, Material.GetName().data(), min(Material.GetName().size(), (size_t)512));
		ofs.write(StringBytes, 512);

		// 1B (bool) has texture
		ofs.put(Material.HasTexture());

		// 12B (XMFLOAT3) ambient
		WRITE_XMFLOAT3_TO_BYTES(Material.GetAmbientColor());

		// 12B (XMFLOAT3) diffuse
		WRITE_XMFLOAT3_TO_BYTES(Material.GetDiffuseColor());

		// 12B (XMFLOAT3) specular
		WRITE_XMFLOAT3_TO_BYTES(Material.GetSpecularColor());

		// 4B (float) specular exponent
		WRITE_FLOAT_TO_BYTES(Material.GetSpecularExponent());

		// 4B (float) specular intensity
		WRITE_FLOAT_TO_BYTES(Material.GetSpecularIntensity());

		// 1B (bool) bShouldGenerateAutoMipMap
		ofs.put(Material.ShouldGenerateAutoMipMap());

		// 512B (string, MAX) diffuse texture file name
		memset(StringBytes, 0, 512);
		memcpy(StringBytes, Material.GetDiffuseTextureFileName().data(), min(Material.GetDiffuseTextureFileName().size(), (size_t)512));
		ofs.write(StringBytes, 512);

		// 512B (string, MAX) normal texture file name
		memset(StringBytes, 0, 512);
		memcpy(StringBytes, Material.GetNormalTextureFileName().data(), min(Material.GetNormalTextureFileName().size(), (size_t)512));
		ofs.write(StringBytes, 512);

		++iMaterial;
	}

	// 1B (uint8_t) Mesh count
	ofs.put((uint8_t)Model.vMeshes.size());

	for (uint8_t iMesh = 0; iMesh < (uint8_t)Model.vMeshes.size(); ++iMesh)
	{
		// 1B (uint8_t) Mesh index
		ofs.put(iMesh);

		const SMesh& Mesh{ Model.vMeshes[iMesh] };

		// 1B (uint8_t) Material id
		ofs.put((uint8_t)Mesh.MaterialID);

		// 4B (uint32_t) Vertex count
		WRITE_UINT32_TO_BYTES(Mesh.vVertices.size());

		for (uint32_t iVertex = 0; iVertex < (uint32_t)Mesh.vVertices.size(); ++iVertex)
		{
			// 4B (uint32_t) Vertex index
			WRITE_UINT32_TO_BYTES(iVertex);

			const SVertex3D& Vertex{ Mesh.vVertices[iVertex] };

			// 16B (XMVECTOR) position
			WRITE_XMVECTOR_TO_BYTES(Vertex.Position);

			// 16B (XMVECTOR) color
			WRITE_XMVECTOR_TO_BYTES(Vertex.Color);

			// 16B (XMVECTOR) texcoord
			WRITE_XMVECTOR_TO_BYTES(Vertex.TexCoord);

			// 16B (XMVECTOR) normal
			WRITE_XMVECTOR_TO_BYTES(Vertex.Normal);
		}

		// 4B (uint32_t) Triangle count
		WRITE_UINT32_TO_BYTES(Mesh.vTriangles.size());
		for (uint32_t iTriangle = 0; iTriangle < (uint32_t)Mesh.vTriangles.size(); ++iTriangle)
		{
			// 4B (uint32_t) Triangle index
			WRITE_UINT32_TO_BYTES(iTriangle);

			const STriangle& Triangle{ Mesh.vTriangles[iTriangle] };

			// 4B (uint32_t) vid 0
			WRITE_UINT32_TO_BYTES(Triangle.I0);

			// 4B (uint32_t) vid 1
			WRITE_UINT32_TO_BYTES(Triangle.I1);

			// 4B (uint32_t) vid 2
			WRITE_UINT32_TO_BYTES(Triangle.I2);
		}
	}
}

static void ExportStaticModel(const SModel& Model, const string& FileName)
{
	std::ofstream ofs{};
	ofs.open(FileName, std::ofstream::binary);
	assert(ofs.is_open());

	// 8B Signature
	ofs.write("SMOD_KJW", 8);

	_WriteStaticModelFile(ofs, Model);

	ofs.close();
}
