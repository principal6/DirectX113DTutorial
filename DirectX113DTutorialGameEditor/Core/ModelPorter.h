#pragma once

#include "Object3D.h"
#include "Terrain.h"
#include "BinaryFile.h"

class CModelPorter
{
public:
	CModelPorter() {}
	~CModelPorter() {}

public:
	SModel ImportStaticModel(const std::string& FileName);
	void ExportStaticModel(const SModel& Model, const std::string& FileName);

	void ImportTerrain(const std::string& FileName, CTerrain::STerrainFileData& Data, std::vector<std::unique_ptr<CObject3D>>& vFoliageObject3Ds,
		ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, CGame* const PtrGame);
	void ExportTerrain(const std::string& FileName, const CTerrain::STerrainFileData& Data, const std::vector<std::unique_ptr<CObject3D>>& vFoliageObject3Ds);

private:
	void ReadStaticModelFile(SModel& Model);
	void ReadModelMaterials(std::vector<CMaterial>& vMaterials);
	void WriteStaticModelFile(const SModel& Model);
	void WriteModelMaterials(const std::vector<CMaterial>& vMaterials);

private:
	CBinaryFile m_BinaryFile{};
};