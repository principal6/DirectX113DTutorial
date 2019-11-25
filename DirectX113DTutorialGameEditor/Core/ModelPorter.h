#pragma once

#include "BinaryFile.h"
#include "Material.h"

class CModelPorter
{
public:
	struct SSMODData
	{
		SSMODData() {}
		SSMODData(const std::vector<SMesh>& _vMeshes, const std::vector<CMaterialData>& _vMaterialData) :
			vMeshes{ _vMeshes }, vMaterialData{ _vMaterialData} {}

		std::vector<SMesh>			vMeshes{};
		std::vector<CMaterialData>	vMaterialData{};
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
	CModelPorter() {}
	~CModelPorter() {}

public:
	void ImportStaticModel(const std::string& FileName, SSMODData& SMODFile);
	void ExportStaticModel(const std::string& FileName, const SSMODData& SMODFile);

	void ImportTerrain(const std::string& FileName, STERRData& Data);
	void ExportTerrain(const std::string& FileName, const STERRData& Data);

private:
	void ReadStaticModelFile(SSMODData& SMODFile);
	void ReadModelMaterials(std::vector<CMaterialData>& vMaterialData);

	void WriteStaticModelFile(const SSMODData& SMODFile);
	void WriteModelMaterials(const std::vector<CMaterialData>& vMaterialData);

private:
	CBinaryFile m_BinaryFile{};
};