#pragma once

#include "SharedHeader.h"
#include "BinaryData.h"
#include "Material.h"

class CMeshPorter
{
public:
	struct SMESHData
	{
		SMESHData() {}
		SMESHData(const std::vector<SMesh>& _vMeshes, const std::vector<CMaterialData>& _vMaterialData) :
			vMeshes{ _vMeshes }, vMaterialData{ _vMaterialData} {}
		SMESHData(const std::vector<SMesh>& _vMeshes, const std::vector<CMaterialData>& _vMaterialData,
			const SBoundingSphere& _BoundingSphereData) :
			vMeshes{ _vMeshes }, vMaterialData{ _vMaterialData }, BoundingSphereData{ _BoundingSphereData } {}

		std::vector<SMesh>			vMeshes{};
		std::vector<CMaterialData>	vMaterialData{};
		SBoundingSphere				BoundingSphereData{};
	};

	struct STERRData
	{
	public:
		struct SFoliageData
		{
			std::string FileName{};
			std::vector<SInstanceCPUData> vInstanceData{};
		};

	public:
		STERRData() {}
		STERRData(float _TerrainTessellationFactor, float _WaterTessellationFactor, uint32_t _MaskingDetail, uint32_t _FoliagePlacingDetail) :
			TerrainTessellationFactor{ _TerrainTessellationFactor }, WaterTessellationFactor{ _WaterTessellationFactor },
			MaskingDetail{ _MaskingDetail }, FoliagePlacingDetail{ _FoliagePlacingDetail } {}

	public:
		std::string FileName{};

		float SizeX{};
		float SizeZ{};
		float HeightRange{};
		float TerrainTessellationFactor{};
		float UniformScalingFactor{};

		std::vector<SPixel8UInt> vHeightMapTextureRawData{};

		bool bShouldDrawWater{ false };
		float WaterHeight{};
		float WaterTessellationFactor{};

		uint32_t MaskingDetail{};
		std::vector<SPixel32UInt> vMaskingTextureRawData{};

		bool bHasFoliageCluster{ false };
		uint32_t FoliagePlacingDetail{};
		float FoliageDenstiy{};
		std::vector<SPixel8UInt> vFoliagePlacingTextureRawData{};
		std::vector<SFoliageData> vFoliageData{};

		std::vector<CMaterialData> vMaterialData{};

		bool bShouldSave{ true };
	};

public:
	CMeshPorter() {}
	CMeshPorter(const std::vector<byte>& vBytes) : m_BinaryData{ vBytes } {}
	~CMeshPorter() {}

public:
	void ImportMesh(const std::string& FileName, SMESHData& MESHFile);
	void ExportMesh(const std::string& FileName, const SMESHData& MESHFile);

	void ImportTerrain(const std::string& FileName, STERRData& Data);
	void ExportTerrain(const std::string& FileName, const STERRData& Data);

public:
	void ReadMeshData(SMESHData& MESHData);
	void WriteMeshData(const SMESHData& MESHData);

private:
	void ReadModelMaterials(std::vector<CMaterialData>& vMaterialData);
	void WriteModelMaterials(const std::vector<CMaterialData>& vMaterialData);

public:
	const std::vector<byte> GetBytes() const;

private:
	CBinaryData m_BinaryData{};
};