#include "ModelPorter.h"

using std::vector;
using std::unique_ptr;
using std::make_unique;

SModel CModelPorter::ImportStaticModel(const std::string& FileName)
{
	m_BinaryFile.OpenToRead(FileName);

	// 8B Signature (SMOD_KJW)
	std::string Signature{ m_BinaryFile.ReadString(8) };

	SModel Model{};
	ReadStaticModelFile(Model);

	m_BinaryFile.Close();

	return Model;
}

void CModelPorter::ExportStaticModel(const SModel& Model, const std::string& FileName)
{
	m_BinaryFile.OpenToWrite(FileName);

	// 8B Signature
	m_BinaryFile.WriteString("SMOD_KJW", 8);

	WriteStaticModelFile(Model);

	m_BinaryFile.Close();
}

void CModelPorter::ImportTerrain(const std::string& FileName, CTerrain::STerrainFileData& Data, vector<unique_ptr<CObject3D>>& vFoliageObject3Ds,
	ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, CGame* const PtrGame)
{
	m_BinaryFile.OpenToRead(FileName);

	// 8B Signature (TERR_KJW)
	std::string Signature{ m_BinaryFile.ReadString(8) };

	// 4B (float) Terrain size x
	Data.SizeX = m_BinaryFile.ReadFloat();

	// 4B (float) Terrain size z
	Data.SizeZ = m_BinaryFile.ReadFloat();

	// 4B (float) Terrain height range
	Data.HeightRange = m_BinaryFile.ReadFloat();

	// 4B (float) Terrain tessellation factor
	Data.TerrainTessellationFactor = m_BinaryFile.ReadFloat();

	// 4B (uint32_t) HeightMap texture raw data size
	Data.vHeightMapTextureRawData.resize(m_BinaryFile.ReadUInt32());

	// HeightMap texture raw data
	for (SPixel8UInt& Pixel : Data.vHeightMapTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		Pixel.R = m_BinaryFile.ReadUInt8();
	}

	// 1B (bool) bShouldDrawWater
	Data.bShouldDrawWater = m_BinaryFile.ReadBool();

	// 4B (float) Water height
	Data.WaterHeight = m_BinaryFile.ReadFloat();

	// 4B (float) Water tessellation factor
	Data.WaterTessellationFactor = m_BinaryFile.ReadFloat();

	// 4B (float) Masking detail
	Data.MaskingDetail = m_BinaryFile.ReadUInt32();

	// 4B (uint32_t) Masking texture raw data size
	Data.vMaskingTextureRawData.resize(m_BinaryFile.ReadUInt32());

	// Masking texture raw data
	for (SPixel32UInt& Pixel : Data.vMaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		Pixel.R = m_BinaryFile.ReadUInt8();
		Pixel.G = m_BinaryFile.ReadUInt8();
		Pixel.B = m_BinaryFile.ReadUInt8();
		Pixel.A = m_BinaryFile.ReadUInt8();
	}

	// 1B (bool) bHasFoliageCluster
	Data.bHasFoliageCluster = m_BinaryFile.ReadBool();

	// 4B (uint32_t) Foliage placing detail
	Data.FoliagePlacingDetail = m_BinaryFile.ReadUInt32();

	// 4B (float) Foliage density
	Data.FoliageDenstiy = m_BinaryFile.ReadFloat();

	// 1B (uint8_t) Foliage file names count
	Data.vFoliageFileNames.resize(m_BinaryFile.ReadUInt8());

	for (auto& FoliageFileName : Data.vFoliageFileNames)
	{
		// 512B (string, file name) Foliage file name
		FoliageFileName = m_BinaryFile.ReadString(512);
	}

	// 4B (uint32_t) Foliage placing texture raw data size
	Data.vFoliagePlacingTextureRawData.resize(m_BinaryFile.ReadUInt32());

	// Foliage placing texture raw data
	for (SPixel8UInt& Pixel : Data.vFoliagePlacingTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		Pixel.R = m_BinaryFile.ReadUInt8();
	}

	// Foliage instance data
	int iFoliage{};
	vFoliageObject3Ds.resize(Data.vFoliageFileNames.size());
	for (auto& FoliageObject3D : vFoliageObject3Ds)
	{
		FoliageObject3D = make_unique<CObject3D>("Foliage", PtrDevice, PtrDeviceContext, PtrGame);
		FoliageObject3D->CreateFromFile(Data.vFoliageFileNames[iFoliage], false);

		// 4B (uint32_t) Foliage instance count
		uint32_t InstanceCount{ m_BinaryFile.ReadUInt32() };
		for (uint32_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			// # 32B (string, KInstanceNameMaxLength) Foliage instance name
			std::string InstanceName{ m_BinaryFile.ReadString(CObject3D::KInstanceNameZeroEndedMaxLength) };

			FoliageObject3D->InsertInstance(InstanceName);
			auto& Instance{ FoliageObject3D->GetInstanceCPUData(InstanceName) };

			// # 16B (XMVECTOR) Foliage instance translation
			Instance.Translation = m_BinaryFile.ReadXMVECTOR();

			// # 4B (float) Foliage instance pitch
			Instance.Pitch = m_BinaryFile.ReadFloat();

			// # 4B (float) Foliage instance yaw
			Instance.Yaw = m_BinaryFile.ReadFloat();

			// # 4B (float) Foliage instance roll
			Instance.Roll = m_BinaryFile.ReadFloat();

			// # 16B (XMVECTOR) Foliage instance scaling
			Instance.Scaling = m_BinaryFile.ReadXMVECTOR();
		}

		FoliageObject3D->UpdateAllInstancesWorldMatrix(); // @important

		++iFoliage;
	}

	// 1B (uint8_t) Material count
	Data.vTerrainMaterialData.resize(m_BinaryFile.ReadUInt8());

	// Material data
	ReadModelMaterials(Data.vTerrainMaterialData);

	// @important
	for (size_t iMaterial = 0; iMaterial < Data.vTerrainMaterialData.size(); ++iMaterial)
	{
		Data.vTerrainMaterialData[iMaterial].Index(iMaterial);
	}

	m_BinaryFile.Close();
}

void CModelPorter::ExportTerrain(const std::string& FileName, const CTerrain::STerrainFileData& Data, const vector<unique_ptr<CObject3D>>& vFoliageObject3Ds)
{
	m_BinaryFile.OpenToWrite(FileName);

	// 8B Signature
	m_BinaryFile.WriteString("TERR_KJW", 8);

	// 4B (float) Terrain size x
	m_BinaryFile.WriteFloat(Data.SizeX);

	// 4B (float) Terrain size z
	m_BinaryFile.WriteFloat(Data.SizeZ);

	// 4B (float) Terrain height range
	m_BinaryFile.WriteFloat(Data.HeightRange);

	// 4B (float) Terrain tessellation factor
	m_BinaryFile.WriteFloat(Data.TerrainTessellationFactor);

	// 4B (uint32_t) HeightMap texture raw data size
	m_BinaryFile.WriteUInt32((uint32_t)Data.vHeightMapTextureRawData.size());

	// HeightMap texture raw data
	for (const SPixel8UInt& Pixel : Data.vHeightMapTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		m_BinaryFile.WriteUInt8(Pixel.R);
	}

	// 1B (bool) bShouldDrawWater
	m_BinaryFile.WriteBool(Data.bShouldDrawWater);

	// 4B (float) Water height
	m_BinaryFile.WriteFloat(Data.WaterHeight);

	// 4B (float) Water tessellation factor
	m_BinaryFile.WriteFloat(Data.WaterTessellationFactor);

	// 4B (uint32_t) Masking detail
	m_BinaryFile.WriteUInt32(Data.MaskingDetail);

	// 4B (uint32_t) Masking texture raw data size
	m_BinaryFile.WriteUInt32((uint32_t)Data.vMaskingTextureRawData.size());

	// Masking texture raw data
	for (const SPixel32UInt& Pixel : Data.vMaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		m_BinaryFile.WriteUInt8(Pixel.R);
		m_BinaryFile.WriteUInt8(Pixel.G);
		m_BinaryFile.WriteUInt8(Pixel.B);
		m_BinaryFile.WriteUInt8(Pixel.A);
	}

	// 1B (bool) bHasFoliageCluster
	m_BinaryFile.WriteBool(Data.bHasFoliageCluster);

	// 4B (uint32_t) Foliage placing detail
	m_BinaryFile.WriteUInt32(Data.FoliagePlacingDetail);

	// 4B (float) Foliage density
	m_BinaryFile.WriteFloat(Data.FoliageDenstiy);

	// 1B (uint8_t) Foliage file names count
	m_BinaryFile.WriteUInt8((uint8_t)Data.vFoliageFileNames.size());

	for (const auto& FoliageFileName : Data.vFoliageFileNames)
	{
		// 512B (string, file name) Foliage file name
		m_BinaryFile.WriteString(FoliageFileName, 512);
	}

	// 4B (uint32_t) Foliage placing texture raw data size
	m_BinaryFile.WriteUInt32((uint32_t)Data.vFoliagePlacingTextureRawData.size());

	// Foliage placing texture raw data
	for (const SPixel8UInt& Pixel : Data.vFoliagePlacingTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		m_BinaryFile.WriteUInt8(Pixel.R);
	}

	// Foliage instance data
	for (const auto& FoliageObject3D : vFoliageObject3Ds)
	{
		uint32_t InstanceCount{ FoliageObject3D->GetInstanceCount() };

		// 4B (uint32_t) Foliage instance count
		m_BinaryFile.WriteUInt32(InstanceCount);

		for (uint32_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			const auto& Instance{ FoliageObject3D->GetInstanceCPUData(iInstance) };

			// # 32B (string, KInstanceNameMaxLength) Foliage instance name
			m_BinaryFile.WriteString(Instance.Name, CObject3D::KInstanceNameZeroEndedMaxLength);

			// # 16B (XMVECTOR) Foliage instance translation
			m_BinaryFile.WriteXMVECTOR(Instance.Translation);
			
			// # 4B (float) Foliage instance pitch
			m_BinaryFile.WriteFloat(Instance.Pitch);

			// # 4B (float) Foliage instance yaw
			m_BinaryFile.WriteFloat(Instance.Yaw);

			// # 4B (float) Foliage instance roll
			m_BinaryFile.WriteFloat(Instance.Roll);

			// # 16B (XMVECTOR) Foliage instance scaling
			m_BinaryFile.WriteXMVECTOR(Instance.Scaling);
		}
	}

	// 1B (uint8_t) Material count
	m_BinaryFile.WriteUInt8((uint8_t)Data.vTerrainMaterialData.size());

	WriteModelMaterials(Data.vTerrainMaterialData);

	m_BinaryFile.Close();
}

void CModelPorter::ReadStaticModelFile(SModel& Model)
{
	// ##### MATERIAL #####
	// 1B (uint8_t) Material count
	Model.vMaterialData.resize(m_BinaryFile.ReadUInt8());

	ReadModelMaterials(Model.vMaterialData);

	// ##### MESH #####
	// 1B (uint8_t) Mesh count
	Model.vMeshes.resize(m_BinaryFile.ReadUInt8());
	for (SMesh& Mesh : Model.vMeshes)
	{
		// # 1B (uint8_t) Mesh index
		m_BinaryFile.ReadUInt8();

		// # 1B (uint8_t) Material ID
		Mesh.MaterialID = m_BinaryFile.ReadUInt8();

		// # ### VERTEX ###
		// 4B (uint32_t) Vertex count
		Mesh.vVertices.resize(m_BinaryFile.ReadUInt32());
		for (SVertex3D& Vertex : Mesh.vVertices)
		{
			// # 4B (uint32_t) Vertex index
			m_BinaryFile.ReadUInt32();

			// # 16B (XMVECTOR) Position
			Vertex.Position = m_BinaryFile.ReadXMVECTOR();

			// # 16B (XMVECTOR) Color
			Vertex.Color = m_BinaryFile.ReadXMVECTOR();

			// # 16B (XMVECTOR) TexCoord
			Vertex.TexCoord = m_BinaryFile.ReadXMVECTOR();

			// # 16B (XMVECTOR) Normal
			Vertex.Normal = m_BinaryFile.ReadXMVECTOR();
		}

		// # ### TRIANGLE ###
		// 4B (uint32_t) Triangle count
		Mesh.vTriangles.resize(m_BinaryFile.ReadUInt32());
		for (STriangle& Triangle : Mesh.vTriangles)
		{
			// # 4B (uint32_t) Triangle index
			m_BinaryFile.ReadUInt32();

			// # 4B (uint32_t) Vertex ID 0
			Triangle.I0 = m_BinaryFile.ReadUInt32();

			// # 4B (uint32_t) Vertex ID 1
			Triangle.I1 = m_BinaryFile.ReadUInt32();

			// # 4B (uint32_t) Vertex ID 2
			Triangle.I2 = m_BinaryFile.ReadUInt32();
		}
	}
}

void CModelPorter::ReadModelMaterials(std::vector<CMaterialData>& vMaterialData)
{
	for (CMaterialData& MaterialData : vMaterialData)
	{
		// # 1B (uint8_t) Material index
		m_BinaryFile.ReadUInt8();

		// # 512B (string) Material name
		MaterialData.Name(m_BinaryFile.ReadString(512));

		// # 1B (bool) bHasTexture ( automatically set by SetTextureFileName() )
		m_BinaryFile.ReadBool();

		// # 12B (XMFLOAT3) Ambient color
		MaterialData.AmbientColor(m_BinaryFile.ReadXMFLOAT3());

		// # 12B (XMFLOAT3) Diffuse color
		MaterialData.DiffuseColor(m_BinaryFile.ReadXMFLOAT3());

		// # 12B (XMFLOAT3) Specular color
		MaterialData.SpecularColor(m_BinaryFile.ReadXMFLOAT3());

		// # 4B (float) Specular exponent
		MaterialData.SpecularExponent(m_BinaryFile.ReadFloat());

		// # 4B (float) Specular intensity
		MaterialData.SpecularIntensity(m_BinaryFile.ReadFloat());

		// # 1B (bool) bShouldGenerateAutoMipMap
		MaterialData.ShouldGenerateMipMap(m_BinaryFile.ReadBool());

		// # 512B (string, File name) Diffuse texture file name
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, m_BinaryFile.ReadString(512));

		// # 512B (string, File name) Normal texture file name
		MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, m_BinaryFile.ReadString(512));

		// # 512B (string, File name) Displacement texture file name
		MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, m_BinaryFile.ReadString(512));

		// # 512B (string, File name) Opacity texture file name
		MaterialData.SetTextureFileName(STextureData::EType::OpacityTexture, m_BinaryFile.ReadString(512));
	}
}

void CModelPorter::WriteStaticModelFile(const SModel& Model)
{
	// 1B (uint8_t) Material count
	m_BinaryFile.WriteUInt8((uint8_t)Model.vMaterialData.size());

	WriteModelMaterials(Model.vMaterialData);

	// 1B (uint8_t) Mesh count
	m_BinaryFile.WriteUInt8((uint8_t)Model.vMeshes.size());

	for (uint8_t iMesh = 0; iMesh < (uint8_t)Model.vMeshes.size(); ++iMesh)
	{
		// 1B (uint8_t) Mesh index
		m_BinaryFile.WriteUInt8(iMesh);

		const SMesh& Mesh{ Model.vMeshes[iMesh] };

		// 1B (uint8_t) Material ID
		m_BinaryFile.WriteUInt8((uint8_t)Mesh.MaterialID);

		// 4B (uint32_t) Vertex count
		m_BinaryFile.WriteUInt32((uint32_t)Mesh.vVertices.size());

		for (uint32_t iVertex = 0; iVertex < (uint32_t)Mesh.vVertices.size(); ++iVertex)
		{
			// 4B (uint32_t) Vertex index
			m_BinaryFile.WriteUInt32(iVertex);

			const SVertex3D& Vertex{ Mesh.vVertices[iVertex] };

			// 16B (XMVECTOR) Position
			m_BinaryFile.WriteXMVECTOR(Vertex.Position);

			// 16B (XMVECTOR) Color
			m_BinaryFile.WriteXMVECTOR(Vertex.Color);

			// 16B (XMVECTOR) TexCoord
			m_BinaryFile.WriteXMVECTOR(Vertex.TexCoord);

			// 16B (XMVECTOR) Normal
			m_BinaryFile.WriteXMVECTOR(Vertex.Normal);
		}

		// 4B (uint32_t) Triangle count
		m_BinaryFile.WriteUInt32((uint32_t)Mesh.vTriangles.size());
		for (uint32_t iTriangle = 0; iTriangle < (uint32_t)Mesh.vTriangles.size(); ++iTriangle)
		{
			// 4B (uint32_t) Triangle index
			m_BinaryFile.WriteUInt32(iTriangle);

			const STriangle& Triangle{ Mesh.vTriangles[iTriangle] };

			// 4B (uint32_t) Vertex ID 0
			m_BinaryFile.WriteUInt32(Triangle.I0);

			// 4B (uint32_t) Vertex ID 1
			m_BinaryFile.WriteUInt32(Triangle.I1);

			// 4B (uint32_t) Vertex ID 2
			m_BinaryFile.WriteUInt32(Triangle.I2);
		}
	}
}

void CModelPorter::WriteModelMaterials(const std::vector<CMaterialData>& vMaterialData)
{
	uint8_t iMaterial{};

	for (const CMaterialData& MaterialData : vMaterialData)
	{
		// 1B (uint8_t) Material index
		m_BinaryFile.WriteUInt8(iMaterial);

		// 512B (string) Material name
		m_BinaryFile.WriteString(MaterialData.Name(), 512);
		
		// 1B (bool) bHasTexture
		m_BinaryFile.WriteBool(MaterialData.HasAnyTexture());

		// 12B (XMFLOAT3) Ambient color
		m_BinaryFile.WriteXMFLOAT3(MaterialData.AmbientColor());

		// 12B (XMFLOAT3) Diffuse color
		m_BinaryFile.WriteXMFLOAT3(MaterialData.DiffuseColor());

		// 12B (XMFLOAT3) Specular color
		m_BinaryFile.WriteXMFLOAT3(MaterialData.SpecularColor());

		// 4B (float) Specular exponent
		m_BinaryFile.WriteFloat(MaterialData.SpecularExponent());

		// 4B (float) Specular intensity
		m_BinaryFile.WriteFloat(MaterialData.SpecularIntensity());

		// 1B (bool) bShouldGenerateAutoMipMap
		m_BinaryFile.WriteBool(MaterialData.ShouldGenerateMipMap());

		// 512B (string, File name) Diffuse texture file name
		m_BinaryFile.WriteString(MaterialData.GetTextureFileName(STextureData::EType::DiffuseTexture), 512);
		
		// 512B (string, File name) Normal texture file name
		m_BinaryFile.WriteString(MaterialData.GetTextureFileName(STextureData::EType::NormalTexture), 512);

		// 512B (string, File name) Displacement texture file name
		m_BinaryFile.WriteString(MaterialData.GetTextureFileName(STextureData::EType::DisplacementTexture), 512);

		// 512B (string, File name) Opacity texture file name
		m_BinaryFile.WriteString(MaterialData.GetTextureFileName(STextureData::EType::OpacityTexture), 512);

		++iMaterial;
	}
}
