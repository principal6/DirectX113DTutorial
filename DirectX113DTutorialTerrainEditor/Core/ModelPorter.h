#pragma once

#include "Object3D.h"
#include <fstream>

// ###########################
// << SMOD FILE STRUCTURE >>
// 8B SMOD Signature
// ##### MATERIAL #####
// 1B (uint8_t) Material count (558B == Each material)
// # 1B (uint8_t) Material index
// # 1B (bool) has texture
// # 12B (XMFLOAT3) ambient
// # 12B (XMFLOAT3) diffuse
// # 12B (XMFLOAT3) specular
// # 4B (float) specular exponent
// # 4B (float) specular intensity
// # 512B (string, MAX) texture file name
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



     // TODO: SAVE AND LOAD MASKING DATA !!


// ###########################
// << TERR FILE STRUCTURE >> : << SMOD FILE STRUCTURE >>
// 8B TERR Signature
// 4B Terrain X Size
// 4B Terrain Z Size
// << SMOD FILE STRUCTURE>>
// ...
// ###########################

#define READ(ByteCount) memset(ReadBytes, 0, sizeof(ReadBytes)); ifs.read(ReadBytes, ByteCount);
#define GET_BOOL GetBoolFromBtyes(ReadBytes)
#define GET_UINT8 GetUint8FromBtyes(ReadBytes)
#define GET_UINT32 GetUint32FromBtyes(ReadBytes)
#define GET_FLOAT GetFloatFromBtyes(ReadBytes)
#define GET_XMFLOAT3 GetXMFLOAT3FromBtyes(ReadBytes)
#define GET_XMVECTOR GetXMVECTORFromBtyes(ReadBytes)

#define WRITE_UINT32 ofs.write(Uint32Bytes, sizeof(Uint32Bytes))
#define WRITE_FLOAT ofs.write(FloatBytes, sizeof(FloatBytes))
#define WRITE_XMFLOAT3 ofs.write(XMFLOAT3Bytes, sizeof(XMFLOAT3Bytes))
#define WRITE_XMVECTOR ofs.write(XMVECTORBytes, sizeof(XMVECTORBytes))

#define GET_WRITE_UINT32(Value) GetBytesFromUint32((uint32_t)Value, Uint32Bytes); WRITE_UINT32
#define GET_WRITE_FLOAT(Value) GetBytesFromFloat((float)Value, FloatBytes); WRITE_FLOAT
#define GET_WRITE_XMFLOAT3(Value) GetBytesFromXMFLOAT3(Value, XMFLOAT3Bytes); WRITE_XMFLOAT3
#define GET_WRITE_XMVECTOR(Value) GeBytesFromtXMVECTOR(Value, XMVECTORBytes); WRITE_XMVECTOR

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

	// 8B Signature
	READ(8);

	// ##### MATERIAL #####
	// 1B (uint8_t) Material count
	READ(1);
	Model.vMaterials.resize(GET_UINT32);
	for (SMaterial& Material : Model.vMaterials)
	{
		// # 1B (uint8_t) Material index
		READ(1);

		// # 1B (bool) has texture
		READ(1);
		Material.bHasTexture = GET_BOOL;

		// # 12B (XMFLOAT3) ambient
		READ(12);
		Material.MaterialAmbient = GET_XMFLOAT3;

		// # 12B (XMFLOAT3) diffuse
		READ(12);
		Material.MaterialDiffuse = GET_XMFLOAT3;

		// # 12B (XMFLOAT3) specular
		READ(12);
		Material.MaterialSpecular = GET_XMFLOAT3;

		// # 4B (float) specular exponent
		READ(4);
		Material.SpecularExponent = GET_FLOAT;

		// # 4B (float) specular intensity
		READ(4);
		Material.SpecularIntensity = GET_FLOAT;

		// # 512B (string, MAX) texture file name
		READ(512);
		Material.TextureFileName = ReadBytes;
	}

	// ##### MESH #####
	// 1B (uint8_t) Mesh count
	READ(1);
	Model.vMeshes.resize(GET_UINT8);
	for (SMesh& Mesh : Model.vMeshes)
	{
		// # 1B (uint8_t) Mesh index
		READ(1);

		// # 1B (uint8_t) Material id
		READ(1);
		Mesh.MaterialID = GET_UINT8;

		// # ### VERTEX ###
		// 4B (uint32_t) Vertex count
		READ(4);
		Mesh.vVertices.resize(GET_UINT32);
		for (SVertex3D& Vertex : Mesh.vVertices)
		{
			// # 4B (uint32_t) Vertex index
			READ(4);

			// # 16B (XMVECTOR) position
			READ(16);
			Vertex.Position = GET_XMVECTOR;

			// # 16B (XMVECTOR) color
			READ(16);
			Vertex.Color = GET_XMVECTOR;

			// # 16B (XMVECTOR) texcoord
			READ(16);
			Vertex.TexCoord = GET_XMVECTOR;

			// # 16B (XMVECTOR) normal
			READ(16);
			Vertex.Normal = GET_XMVECTOR;
		}

		// # ### TRIANGLE ###
		// 4B (uint32_t) Triangle count
		READ(4);
		Mesh.vTriangles.resize(GET_UINT32);
		for (STriangle& Triangle : Mesh.vTriangles)
		{
			// # 4B (uint32_t) Triangle index
			READ(4);

			// # 4B (uint32_t) vid 0
			READ(4);
			Triangle.I0 = GET_UINT32;

			// # 4B (uint32_t) vid 1
			READ(4);
			Triangle.I1 = GET_UINT32;

			// # 4B (uint32_t) vid 2
			READ(4);
			Triangle.I2 = GET_UINT32;
		}
	}
}

static SModel ImportStaticModel(const string& FileName)
{
	std::ifstream ifs{};
	ifs.open(FileName, std::ofstream::binary);
	assert(ifs.is_open());

	SModel Model{};
	_ReadStaticModelFile(ifs, Model);

	return Model;
}

static void ImportTerrain(const string& FileName, SModel& Model, XMFLOAT2& TerrainSize)
{
	std::ifstream ifs{};
	ifs.open(FileName, std::ofstream::binary);
	assert(ifs.is_open());

	char ReadBytes[512]{};

	// 8B Signature
	READ(8);

	// 4B Terrain X Size
	READ(4);
	TerrainSize.x = GET_FLOAT;
	
	// 4B Terrain Z Size
	READ(4);
	TerrainSize.y = GET_FLOAT;

	_ReadStaticModelFile(ifs, Model);
}

static void _WriteStaticModelFile(std::ofstream& ofs, const SModel& Model)
{
	// 8B Signature
	ofs.write("SMOD_KJW", 8);

	char TextureFileNameBytes[512]{};
	char Uint32Bytes[4]{};
	char FloatBytes[4]{};
	char XMFLOAT3Bytes[12]{};
	char XMVECTORBytes[16]{};
	uint8_t iMaterial{};

	// 1B (uint8_t) Material count
	ofs.put((uint8_t)Model.vMaterials.size());

	// 559B == Each material
	for (const SMaterial& Material : Model.vMaterials)
	{
		// 1B (uint8_t) Material index
		ofs.put(iMaterial);

		// 1B (bool) has texture
		ofs.put(Material.bHasTexture);

		// 12B (XMFLOAT3) ambient
		GET_WRITE_XMFLOAT3(Material.MaterialAmbient);

		// 12B (XMFLOAT3) diffuse
		GET_WRITE_XMFLOAT3(Material.MaterialDiffuse);

		// 12B (XMFLOAT3) specular
		GET_WRITE_XMFLOAT3(Material.MaterialSpecular);

		// 4B (float) specular exponent
		GET_WRITE_FLOAT(Material.SpecularExponent);

		// 4B (float) specular intensity
		GET_WRITE_FLOAT(Material.SpecularIntensity);

		// 512B (string, MAX) texture file name
		memset(TextureFileNameBytes, 0, 512);
		memcpy(TextureFileNameBytes, Material.TextureFileName.data(), min(Material.TextureFileName.size(), (size_t)512));
		ofs.write(TextureFileNameBytes, 512);

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
		GET_WRITE_UINT32(Mesh.vVertices.size());

		for (uint32_t iVertex = 0; iVertex < (uint32_t)Mesh.vVertices.size(); ++iVertex)
		{
			// 4B (uint32_t) Vertex index
			GET_WRITE_UINT32(iVertex);

			const SVertex3D& Vertex{ Mesh.vVertices[iVertex] };

			// 16B (XMVECTOR) position
			GET_WRITE_XMVECTOR(Vertex.Position);

			// 16B (XMVECTOR) color
			GET_WRITE_XMVECTOR(Vertex.Color);

			// 16B (XMVECTOR) texcoord
			GET_WRITE_XMVECTOR(Vertex.TexCoord);

			// 16B (XMVECTOR) normal
			GET_WRITE_XMVECTOR(Vertex.Normal);
		}

		// 4B (uint32_t) Triangle count
		GET_WRITE_UINT32(Mesh.vTriangles.size());
		for (uint32_t iTriangle = 0; iTriangle < (uint32_t)Mesh.vTriangles.size(); ++iTriangle)
		{
			// 4B (uint32_t) Triangle index
			GET_WRITE_UINT32(iTriangle);

			const STriangle& Triangle{ Mesh.vTriangles[iTriangle] };

			// 4B (uint32_t) vid 0
			GET_WRITE_UINT32(Triangle.I0);

			// 4B (uint32_t) vid 1
			GET_WRITE_UINT32(Triangle.I1);

			// 4B (uint32_t) vid 2
			GET_WRITE_UINT32(Triangle.I2);
		}
	}
}

static void ExportStaticModel(const SModel& Model, const string& FileName)
{
	std::ofstream ofs{};
	ofs.open(FileName, std::ofstream::binary);
	assert(ofs.is_open());

	_WriteStaticModelFile(ofs, Model);

	ofs.close();
}

static void ExportTerrain(const SModel& Model, const XMFLOAT2& TerrainSize, const string& FileName)
{
	std::ofstream ofs{};
	ofs.open(FileName, std::ofstream::binary);
	assert(ofs.is_open());

	char FloatBytes[4]{};

	// 8B Signature
	ofs.write("TERR_KJW", 8);

	// 4B Terrain X Size
	GET_WRITE_FLOAT(TerrainSize.x);
	
	// 4B Terrain Z Size
	GET_WRITE_FLOAT(TerrainSize.y);
	
	_WriteStaticModelFile(ofs, Model);

	ofs.close();
}
