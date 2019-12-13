#pragma once

#include "SharedHeader.h"
#include "BinaryData.h"
#include "Material.h"

struct SMESHData
{
	struct STreeNode
	{
		struct SBlendWeight
		{
			uint32_t	MeshIndex{};
			uint32_t	VertexID{};
			float		Weight{};
		};

		int32_t						Index{};
		std::string					Name{};

		int32_t						ParentNodeIndex{};
		std::vector<int32_t>		vChildNodeIndices{};
		XMMATRIX					MatrixTransformation{};

		bool						bIsBone{ false };
		uint32_t					BoneIndex{};
		std::vector<SBlendWeight>	vBlendWeights{};
		XMMATRIX					MatrixBoneOffset{};
	};

	struct SAnimation
	{
		struct SNodeAnimation
		{
			struct SKey
			{
				float		Time{};
				XMVECTOR	Value{};
			};

			uint32_t			Index{};
			std::string			Name{};
			std::vector<SKey>	vPositionKeys{};
			std::vector<SKey>	vRotationKeys{};
			std::vector<SKey>	vScalingKeys{};
		};

		std::vector<SNodeAnimation>				vNodeAnimations{};

		float									Duration{};
		float									TicksPerSecond{};

		std::unordered_map<std::string, size_t>	umapNodeAnimationNameToIndex{};

		std::string								Name{};
	};

	std::vector<SMesh>						vMeshes{};
	std::vector<CMaterialData>				vMaterialData{};
	SEditorBoundingSphere					EditorBoundingSphereData{};

	bool									bIsModelRigged{};
	std::vector<STreeNode>					vTreeNodes{};
	std::unordered_map<std::string, size_t>	umapTreeNodeNameToIndex{};
	uint32_t								ModelBoneCount{};
	std::vector<SAnimation>					vAnimations{};

	bool									bUseMultipleTexturesInSingleMesh{ false };
};

class CMeshPorter
{
public:
	struct STERRData
	{
	public:
		struct SFoliageData
		{
			std::string FileName{};
			std::vector<SObject3DInstanceCPUData> vInstanceData{};
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
	void ImportMESH(const std::string& FileName, SMESHData& MESHFile);
	void ExportMESH(const std::string& FileName, const SMESHData& MESHFile);

	void ImportTerrain(const std::string& FileName, STERRData& Data);
	void ExportTerrain(const std::string& FileName, const STERRData& Data);

public:
	void ReadMESHData(SMESHData& MESHData);
	void WriteMESHData(const SMESHData& MESHData);

private:
	void ReadModelMaterials(std::vector<CMaterialData>& vMaterialData);
	void WriteModelMaterials(const std::vector<CMaterialData>& vMaterialData);

public:
	const std::vector<byte> GetBytes() const;

private:
	CBinaryData m_BinaryData{};
};