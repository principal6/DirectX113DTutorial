#include "MeshPorter.h"
#include "Object3D.h"

using std::vector;
using std::unique_ptr;
using std::make_unique;
using std::string;

void CMeshPorter::ImportMesh(const std::string& FileName, CMeshPorter::SMESHData& MESHFile)
{
	m_BinaryData.Clear();
	
	m_BinaryData.LoadFromFile(FileName);
	ReadMeshData(MESHFile);
}

void CMeshPorter::ExportMesh(const std::string& FileName, const CMeshPorter::SMESHData& MESHFile)
{
	m_BinaryData.Clear();

	WriteMeshData(MESHFile);
	m_BinaryData.SaveToFile(FileName);
}

void CMeshPorter::ImportTerrain(const std::string& FileName, CMeshPorter::STERRData& Data)
{
	uint32_t StringLength{};
	
	m_BinaryData.Clear();

	m_BinaryData.LoadFromFile(FileName);

	// 8B Signature (TERR_KJW)
	m_BinaryData.ReadSkip(8);

	// 4B (float) Terrain size x
	Data.SizeX = m_BinaryData.ReadFloat();

	// 4B (float) Terrain size z
	Data.SizeZ = m_BinaryData.ReadFloat();

	// 4B (float) Terrain height range
	Data.HeightRange = m_BinaryData.ReadFloat();

	// 4B (float) Terrain tessellation factor
	Data.TerrainTessellationFactor = m_BinaryData.ReadFloat();

	// 4B (float) Uniform scaling factor
	Data.UniformScalingFactor = m_BinaryData.ReadFloat();


	// 4B (uint32_t) HeightMap texture raw data size
	Data.vHeightMapTextureRawData.resize(m_BinaryData.ReadUint32());

	// HeightMap texture raw data
	for (SPixel8UInt& Pixel : Data.vHeightMapTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		Pixel.R = m_BinaryData.ReadUint8();
	}
	

	// 1B (bool) bShouldDrawWater
	Data.bShouldDrawWater = m_BinaryData.ReadBool();

	// 4B (float) Water height
	Data.WaterHeight = m_BinaryData.ReadFloat();

	// 4B (float) Water tessellation factor
	Data.WaterTessellationFactor = m_BinaryData.ReadFloat();


	// 4B (float) Masking detail
	Data.MaskingDetail = m_BinaryData.ReadUint32();

	// 4B (uint32_t) Masking texture raw data size
	Data.vMaskingTextureRawData.resize(m_BinaryData.ReadUint32());

	// Masking texture raw data
	for (SPixel32UInt& Pixel : Data.vMaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		Pixel.R = m_BinaryData.ReadUint8();
		Pixel.G = m_BinaryData.ReadUint8();
		Pixel.B = m_BinaryData.ReadUint8();
		Pixel.A = m_BinaryData.ReadUint8();
	}


	// 1B (bool) bHasFoliageCluster
	Data.bHasFoliageCluster = m_BinaryData.ReadBool();

	// 4B (uint32_t) Foliage placing detail
	Data.FoliagePlacingDetail = m_BinaryData.ReadUint32();

	// 4B (float) Foliage density
	Data.FoliageDenstiy = m_BinaryData.ReadFloat();

	// 4B (uint32_t) Foliage placing texture raw data size
	Data.vFoliagePlacingTextureRawData.resize(m_BinaryData.ReadUint32());

	// Foliage placing texture raw data
	for (SPixel8UInt& Pixel : Data.vFoliagePlacingTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		Pixel.R = m_BinaryData.ReadUint8();
	}

	// 1B (uint8_t) Foliage count
	Data.vFoliageData.resize(m_BinaryData.ReadUint8());

	for (auto& FoliageData : Data.vFoliageData)
	{
		// # <@PrefString> File name
		m_BinaryData.ReadStringWithPrefixedLength(StringLength, FoliageData.FileName);

		// 4B (uint32_t) Foliage instance count
		FoliageData.vInstanceData.resize(m_BinaryData.ReadUint32());

		for (auto& InstanceData : FoliageData.vInstanceData)
		{
			// # 32B (string, KInstanceNameMaxLength) Foliage instance name
			m_BinaryData.ReadString(InstanceData.Name, SInstanceCPUData::KMaxNameLengthZeroTerminated);

			// # 16B (XMVECTOR) Foliage instance translation
			m_BinaryData.ReadXMVECTOR(InstanceData.Translation);

			// # 4B (float) Foliage instance pitch
			m_BinaryData.ReadFloat(InstanceData.Pitch);

			// # 4B (float) Foliage instance yaw
			m_BinaryData.ReadFloat(InstanceData.Yaw);

			// # 4B (float) Foliage instance roll
			m_BinaryData.ReadFloat(InstanceData.Roll);

			// # 16B (XMVECTOR) Foliage instance scaling
			m_BinaryData.ReadXMVECTOR(InstanceData.Scaling);
		}
	}


	// 1B (uint8_t) Material count
	Data.vMaterialData.resize(m_BinaryData.ReadUint8());

	// Material data
	ReadModelMaterials(Data.vMaterialData);

	// @important
	for (size_t iMaterial = 0; iMaterial < Data.vMaterialData.size(); ++iMaterial)
	{
		Data.vMaterialData[iMaterial].Index(iMaterial);
	}

	// @important: right after importing the terrain, it doens't need to be saved again!
	Data.bShouldSave = false;
}

void CMeshPorter::ExportTerrain(const std::string& FileName, const CMeshPorter::STERRData& Data)
{
	m_BinaryData.Clear();

	// 8B Signature
	m_BinaryData.WriteString("KJW_TERR", 8);


	// 4B (float) Terrain size x
	m_BinaryData.WriteFloat(Data.SizeX);

	// 4B (float) Terrain size z
	m_BinaryData.WriteFloat(Data.SizeZ);

	// 4B (float) Terrain height range
	m_BinaryData.WriteFloat(Data.HeightRange);

	// 4B (float) Terrain tessellation factor
	m_BinaryData.WriteFloat(Data.TerrainTessellationFactor);

	// 4B (float) Uniform scaling factor
	m_BinaryData.WriteFloat(Data.UniformScalingFactor);


	// 4B (uint32_t) HeightMap texture raw data size
	m_BinaryData.WriteUint32((uint32_t)Data.vHeightMapTextureRawData.size());

	// HeightMap texture raw data
	for (const SPixel8UInt& Pixel : Data.vHeightMapTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		m_BinaryData.WriteUint8(Pixel.R);
	}


	// 1B (bool) bShouldDrawWater
	m_BinaryData.WriteBool(Data.bShouldDrawWater);

	// 4B (float) Water height
	m_BinaryData.WriteFloat(Data.WaterHeight);

	// 4B (float) Water tessellation factor
	m_BinaryData.WriteFloat(Data.WaterTessellationFactor);


	// 4B (uint32_t) Masking detail
	m_BinaryData.WriteUint32(Data.MaskingDetail);

	// 4B (uint32_t) Masking texture raw data size
	m_BinaryData.WriteUint32((uint32_t)Data.vMaskingTextureRawData.size());

	// Masking texture raw data
	for (const SPixel32UInt& Pixel : Data.vMaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		m_BinaryData.WriteUint8(Pixel.R);
		m_BinaryData.WriteUint8(Pixel.G);
		m_BinaryData.WriteUint8(Pixel.B);
		m_BinaryData.WriteUint8(Pixel.A);
	}


	// 1B (bool) bHasFoliageCluster
	m_BinaryData.WriteBool(Data.bHasFoliageCluster);

	// 4B (uint32_t) Foliage placing detail
	m_BinaryData.WriteUint32(Data.FoliagePlacingDetail);

	// 4B (float) Foliage density
	m_BinaryData.WriteFloat(Data.FoliageDenstiy);

	// 4B (uint32_t) Foliage placing texture raw data size
	m_BinaryData.WriteUint32((uint32_t)Data.vFoliagePlacingTextureRawData.size());

	// Foliage placing texture raw data
	for (const SPixel8UInt& Pixel : Data.vFoliagePlacingTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		m_BinaryData.WriteUint8(Pixel.R);
	}

	// # 1B (uint8_t) Foliage count
	m_BinaryData.WriteUint8((uint8_t)Data.vFoliageData.size());

	for (const auto& FoliageData : Data.vFoliageData)
	{
		// # <@PrefString> File name
		m_BinaryData.WriteStringWithPrefixedLength(FoliageData.FileName);

		// 4B (uint32_t) Foliage instance count
		m_BinaryData.WriteUint32((uint32_t)FoliageData.vInstanceData.size());

		for (auto& InstanceData : FoliageData.vInstanceData)
		{
			// # 32B (string, KInstanceNameMaxLength) Foliage instance name
			m_BinaryData.WriteString(InstanceData.Name, SInstanceCPUData::KMaxNameLengthZeroTerminated);

			// # 16B (XMVECTOR) Foliage instance translation
			m_BinaryData.WriteXMVECTOR(InstanceData.Translation);

			// # 4B (float) Foliage instance pitch
			m_BinaryData.WriteFloat(InstanceData.Pitch);

			// # 4B (float) Foliage instance yaw
			m_BinaryData.WriteFloat(InstanceData.Yaw);

			// # 4B (float) Foliage instance roll
			m_BinaryData.WriteFloat(InstanceData.Roll);

			// # 16B (XMVECTOR) Foliage instance scaling
			m_BinaryData.WriteXMVECTOR(InstanceData.Scaling);
		}
	}
	

	WriteModelMaterials(Data.vMaterialData);

	m_BinaryData.SaveToFile(FileName);
}

void CMeshPorter::ReadMeshData(CMeshPorter::SMESHData& MESHData)
{
	// 8B Signature (KJW_MESH)
	m_BinaryData.ReadSkip(8);

	// 4B (in total) Version
	uint16_t VersionMajor{ m_BinaryData.ReadUint16() };
	uint8_t VersionMinor{ m_BinaryData.ReadUint8() };
	uint8_t VersionSubminor{ m_BinaryData.ReadUint8() };
	uint32_t VesionCmp{ (uint32_t)(VersionSubminor | (VersionMinor << 8) | (VersionMajor << 16)) };

	// ##### MATERIAL #####
	// 1B (uint8_t) Material count
	MESHData.vMaterialData.resize(m_BinaryData.ReadUint8());

	ReadModelMaterials(MESHData.vMaterialData);

	// ##### MESH #####
	// 1B (uint8_t) Mesh count
	MESHData.vMeshes.resize(m_BinaryData.ReadUint8());
	for (SMesh& Mesh : MESHData.vMeshes)
	{
		// # 1B (uint8_t) Mesh index
		m_BinaryData.ReadUint8();

		// # 1B (uint8_t) Material ID
		m_BinaryData.ReadUint8(Mesh.MaterialID);

		// # ### VERTEX ###
		// 4B (uint32_t) Vertex count
		Mesh.vVertices.resize(m_BinaryData.ReadUint32());
		for (SVertex3D& Vertex : Mesh.vVertices)
		{
			// # 4B (uint32_t) Vertex index
			m_BinaryData.ReadUint32();

			// # 16B (XMVECTOR) Position
			m_BinaryData.ReadXMVECTOR(Vertex.Position);

			// # 16B (XMVECTOR) Color
			m_BinaryData.ReadXMVECTOR(Vertex.Color);

			// # 16B (XMVECTOR) TexCoord
			m_BinaryData.ReadXMVECTOR(Vertex.TexCoord);

			// # 16B (XMVECTOR) Normal
			m_BinaryData.ReadXMVECTOR(Vertex.Normal);

			// 16B (XMVECTOR) Tangent
			m_BinaryData.ReadXMVECTOR(Vertex.Tangent);
		}

		// # ### TRIANGLE ###
		// 4B (uint32_t) Triangle count
		Mesh.vTriangles.resize(m_BinaryData.ReadUint32());
		for (STriangle& Triangle : Mesh.vTriangles)
		{
			// # 4B (uint32_t) Triangle index
			m_BinaryData.ReadUint32();

			// # 4B (uint32_t) Vertex ID 0
			m_BinaryData.ReadUint32(Triangle.I0);

			// # 4B (uint32_t) Vertex ID 1
			m_BinaryData.ReadUint32(Triangle.I1);

			// # 4B (uint32_t) Vertex ID 2
			m_BinaryData.ReadUint32(Triangle.I2);
		}
	}

	if (VesionCmp >= 0x10001)
	{
		// # 16B (XMVECTOR) Bounding sphere center offset
		m_BinaryData.ReadXMVECTOR(MESHData.BoundingSphereData.CenterOffset);

		// # 4B (float) Bounding sphere radius bias
		m_BinaryData.ReadFloat(MESHData.BoundingSphereData.RadiusBias);
	}
}

void CMeshPorter::ReadModelMaterials(std::vector<CMaterialData>& vMaterialData)
{
	string ReadString{};
	uint32_t ReadStringLength{};

	for (CMaterialData& MaterialData : vMaterialData)
	{
		// # 1B (uint8_t) Material index
		m_BinaryData.ReadUint8();

		// # <@PrefString> Material name
		m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString);
		MaterialData.Name(ReadString);

		// # 1B (bool) bHasTexture ( automatically set by SetTextureFileName() )
		m_BinaryData.ReadBool();

		// # 12B (XMFLOAT3) Diffuse color (Classical) == Base color (PBR)
		XMFLOAT3 DiffuseColor{};
		m_BinaryData.ReadXMFLOAT3(DiffuseColor);
		MaterialData.DiffuseColor(DiffuseColor);

		// # 12B (XMFLOAT3) Ambient color (Classical only)
		XMFLOAT3 AmbientColor{};
		m_BinaryData.ReadXMFLOAT3(AmbientColor);
		MaterialData.AmbientColor(AmbientColor);

		// # 12B (XMFLOAT3) Specular color (Classical only)
		XMFLOAT3 SpecularColor{};
		m_BinaryData.ReadXMFLOAT3(SpecularColor);
		MaterialData.SpecularColor(SpecularColor);

		// # 4B (float) Specular exponent (Classical only)
		MaterialData.SpecularExponent(m_BinaryData.ReadFloat());

		// # 4B (float) Specular intensity
		MaterialData.SpecularIntensity(m_BinaryData.ReadFloat());

		// # 4B (float) Roughness (PBR only)
		MaterialData.Roughness(m_BinaryData.ReadFloat());

		// # 4B (float) Metalness (PBR only)
		MaterialData.Metalness(m_BinaryData.ReadFloat());

		// # 1B (bool) bShouldGenerateAutoMipMap
		MaterialData.ShouldGenerateMipMap(m_BinaryData.ReadBool());

		// # <@PrefString> Diffuse texture file name (Classical) // BaseColor texture file name (PBR)
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, ReadString);

		// # <@PrefString> Normal texture file name
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, ReadString);

		// # <@PrefString> Opacity texture file name
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::OpacityTexture, ReadString);

		// # <@PrefString> Specular intensity texture file name
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::SpecularIntensityTexture, ReadString);

		// # <@PrefString> Roughness texture file name (PBR only)
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::RoughnessTexture, ReadString);

		// # <@PrefString> Metalness texture file name (PBR only)
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::MetalnessTexture, ReadString);

		// # <@PrefString> Ambient occlusion texture file name (PBR only)
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::AmbientOcclusionTexture, ReadString);

		// # <@PrefString> Displacement texture file name
		if (m_BinaryData.ReadStringWithPrefixedLength(ReadStringLength, ReadString))
			MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, ReadString);
	}
}

void CMeshPorter::WriteMeshData(const CMeshPorter::SMESHData& MESHData)
{
	static uint16_t KVersionMajor{ 0x0001 };
	static uint8_t KVersionMinor{ 0x00 };
	static uint8_t KVersionSubminor{ 0x01 };

	// 8B Signature
	m_BinaryData.WriteString("KJW_MESH", 8);

	// 4B (in total) Version
	m_BinaryData.WriteUint16(KVersionMajor);
	m_BinaryData.WriteUint8(KVersionMinor);
	m_BinaryData.WriteUint8(KVersionSubminor);

	WriteModelMaterials(MESHData.vMaterialData);

	// 1B (uint8_t) Mesh count
	m_BinaryData.WriteUint8((uint8_t)MESHData.vMeshes.size());

	for (uint8_t iMesh = 0; iMesh < (uint8_t)MESHData.vMeshes.size(); ++iMesh)
	{
		// 1B (uint8_t) Mesh index
		m_BinaryData.WriteUint8(iMesh);

		const SMesh& Mesh{ MESHData.vMeshes[iMesh] };

		// 1B (uint8_t) Material ID
		m_BinaryData.WriteUint8((uint8_t)Mesh.MaterialID);

		// 4B (uint32_t) Vertex count
		m_BinaryData.WriteUint32((uint32_t)Mesh.vVertices.size());

		for (uint32_t iVertex = 0; iVertex < (uint32_t)Mesh.vVertices.size(); ++iVertex)
		{
			// 4B (uint32_t) Vertex index
			m_BinaryData.WriteUint32(iVertex);

			const SVertex3D& Vertex{ Mesh.vVertices[iVertex] };

			// 16B (XMVECTOR) Position
			m_BinaryData.WriteXMVECTOR(Vertex.Position);

			// 16B (XMVECTOR) Color
			m_BinaryData.WriteXMVECTOR(Vertex.Color);

			// 16B (XMVECTOR) TexCoord
			m_BinaryData.WriteXMVECTOR(Vertex.TexCoord);

			// 16B (XMVECTOR) Normal
			m_BinaryData.WriteXMVECTOR(Vertex.Normal);

			// 16B (XMVECTOR) Tangent
			m_BinaryData.WriteXMVECTOR(Vertex.Tangent);
		}

		// 4B (uint32_t) Triangle count
		m_BinaryData.WriteUint32((uint32_t)Mesh.vTriangles.size());
		for (uint32_t iTriangle = 0; iTriangle < (uint32_t)Mesh.vTriangles.size(); ++iTriangle)
		{
			// 4B (uint32_t) Triangle index
			m_BinaryData.WriteUint32(iTriangle);

			const STriangle& Triangle{ Mesh.vTriangles[iTriangle] };

			// 4B (uint32_t) Vertex ID 0
			m_BinaryData.WriteUint32(Triangle.I0);

			// 4B (uint32_t) Vertex ID 1
			m_BinaryData.WriteUint32(Triangle.I1);

			// 4B (uint32_t) Vertex ID 2
			m_BinaryData.WriteUint32(Triangle.I2);
		}
	}

	// # 16B (XMVECTOR) Bounding sphere center offset
	m_BinaryData.WriteXMVECTOR(MESHData.BoundingSphereData.CenterOffset);
	
	// # 4B (float) Bounding sphere radius bias
	m_BinaryData.WriteFloat(MESHData.BoundingSphereData.RadiusBias);
}

void CMeshPorter::WriteModelMaterials(const std::vector<CMaterialData>& vMaterialData)
{
	uint32_t StringLength{};
	uint8_t iMaterial{};

	// 1B (uint8_t) Material count
	m_BinaryData.WriteUint8((uint8_t)vMaterialData.size());

	for (const CMaterialData& MaterialData : vMaterialData)
	{
		// # 1B (uint8_t) Material index
		m_BinaryData.WriteUint8(iMaterial);

		// # <@PrefString> Material name
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.Name());
		
		// # 1B (bool) bHasTexture
		m_BinaryData.WriteBool(MaterialData.HasAnyTexture());

		// # 12B (XMFLOAT3) Diffuse color (Classical) == Base color (PBR)
		m_BinaryData.WriteXMFLOAT3(MaterialData.DiffuseColor());

		// # 12B (XMFLOAT3) Ambient color (Classical only)
		m_BinaryData.WriteXMFLOAT3(MaterialData.AmbientColor());

		// # 12B (XMFLOAT3) Specular color (Classical only)
		m_BinaryData.WriteXMFLOAT3(MaterialData.SpecularColor());

		// # 4B (float) Specular exponent (Classical)
		m_BinaryData.WriteFloat(MaterialData.SpecularExponent());

		// # 4B (float) Specular intensity
		m_BinaryData.WriteFloat(MaterialData.SpecularIntensity());

		// # 4B (float) Roughness (PBR only)
		m_BinaryData.WriteFloat(MaterialData.Roughness());

		// # 4B (float) Metalness (PBR only)
		m_BinaryData.WriteFloat(MaterialData.Metalness());

		// # 1B (bool) bShouldGenerateAutoMipMap
		m_BinaryData.WriteBool(MaterialData.ShouldGenerateMipMap());

		// # <@PrefString> Diffuse texture file name (Classical) // BaseColor texture file name (PBR)
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::DiffuseTexture));
		
		// # <@PrefString> Normal texture file name
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::NormalTexture));

		// # <@PrefString> Opacity texture file name
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::OpacityTexture));

		// # <@PrefString> Specular intensity texture file name
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::SpecularIntensityTexture));

		// # <@PrefString> Roughness texture file name (PBR only)
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::RoughnessTexture));

		// # <@PrefString> Metalness texture file name (PBR only)
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::MetalnessTexture));

		// # <@PrefString> Ambient occlusion texture file name (PBR only)
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::AmbientOcclusionTexture));

		// # <@PrefString> Displacement texture file name
		m_BinaryData.WriteStringWithPrefixedLength(MaterialData.GetTextureFileName(STextureData::EType::DisplacementTexture));

		++iMaterial;
	}
}

const std::vector<byte> CMeshPorter::GetBytes() const
{
	return m_BinaryData.GetBytes();
}
