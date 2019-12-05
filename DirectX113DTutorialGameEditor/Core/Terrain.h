#pragma once

#include "Object2D.h"
#include "Object3D.h"
#include "Material.h"
#include "MeshPorter.h"

class CGame;

class CTerrain
{
public:
	enum class EEditMode
	{
		SetHeight,
		DeltaHeight,
		AverageHeight,
		Masking,
		FoliagePlacing
	};

	enum class EMaskingLayer
	{
		LayerR,
		LayerG,
		LayerB,
		LayerA,
	};

	struct SCBTerrainData
	{
		float TerrainSizeX{};
		float TerrainSizeZ{};
		float TerrainHeightRange{};
		float Time{};
	};

	// TODO: Bake wind data into a fetch texture, so that in shader we could use multiple wind displacement!
	struct SCBWindData
	{
		XMVECTOR	Velocity{};
		XMFLOAT3	Position{};
		float		Radius{ 1.0f };
	};

	struct SCBTerrainSelectionData
	{
		BOOL		bShowSelection{};
		float		SelectionRadius{ CTerrain::KDefaultSelectionRadius };
		XMFLOAT2	Position{};

		XMMATRIX	TerrainWorld{};
		XMMATRIX	InverseTerrainWorld{};
	};

public:
	CTerrain(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext, CGame* const PtrGame) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGame{ PtrGame }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CTerrain() {}

public:
	void Create(const XMFLOAT2& TerrainSize, const CMaterialData& MaterialData, uint32_t MaskingDetail, float UniformScaling = 1.0f);
	void Load(const std::string& FileName);
	bool Save(const std::string& FileName);

private:
	void Scale(const XMVECTOR& Scaling);
	void RegisterChange();
	bool ShouldSave() const;

public:
	void CreateFoliageCluster();
	void CreateFoliageCluster(const std::vector<std::string>& vFoliageFileNames, uint32_t PlacingDetail);
	void SetFoliageDensity(float Density);
	float GetFoliageDenstiy() const;

public:
	void SetWindVelocity(const XMFLOAT3& Velocity);
	void SetWindVelocity(const XMVECTOR& Velocity);
	const SCBWindData& GetWindData() const;
	const XMVECTOR& GetWindVelocity() const;
	void SetWindRadius(float Radius);
	float GetWindRadius() const;

private:
	void CreateTerrainObject3D(const std::vector<CMaterialData>& vMaterialData);
	void CreateHeightMapTexture(bool bShouldClear);
	void CreateMaskingTexture(bool bShouldClear);
	void CreateWater();
	void CreateFoliagePlacingTexutre(bool bShouldClear);
	void CreateWindRepresentation();

public:
	void AddMaterial(const CMaterialData& MaterialData);
	void SetMaterial(int MaterialID, const CMaterialData& NewMaterialData);
	const CMaterialData& GetMaterial(int Index) const;
	
public:
	void Select(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, bool bShouldEdit, bool bIsLeftButton);

private:
	void UpdateSelectionPosition(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection);
	void UpdateHeights(bool bIsLeftButton);
	void UpdateMasking(EMaskingLayer eLayer, float Value, bool bForceSet = false);
	void UpdateFoliagePlacing(bool bErase);

public:
	void SetMaskingLayer(EMaskingLayer eLayer);
	CTerrain::EMaskingLayer GetMaskingLayer() const;
	void SetMaskingAttenuation(float Attenuation);
	float GetMaskingAttenuation() const;
	void SetSelectionRadius(float Radius);
	float GetSelectionRadius() const;
	void SetSetHeightValue(float Value);
	float GetSetHeightValue() const;
	void SetDeltaHeightValue(float Value);
	float GetDeltaHeightValue() const;
	void SetMaskingRatio(float Value);
	float GetMaskingRatio() const;

	void ShouldTessellate(bool Value);
	bool ShouldTessellate() const;

	void ShouldDrawWater(bool Value);
	bool ShouldDrawWater() const;

	void ShouldShowSelection(bool Value);

	void SetEditMode(EEditMode Mode);
	CTerrain::EEditMode GetEditMode();
	void SetWaterHeight(float Value);
	float GetWaterHeight() const;
	void SetTerrainTessFactor(float Value);
	float GetTerrainTessFactor() const;
	const CObject3D::SCBTessFactorData& GetTerrainTessFactorData() const;
	const CObject3D::SCBDisplacementData& GetTerrainDisplacementData() const;
	void SetWaterTessFactor(float Value);
	float GetWaterTessFactor() const;
	const CObject3D::SCBTessFactorData& GetWaterTessFactorData() const;
	const CObject3D::SCBDisplacementData& GetWaterDisplacementData() const;


	XMFLOAT2 GetSize() const;
	int GetMaterialCount() const;
	int GetMaskingDetail() const;

	bool HasFoliageCluster() const;

	const std::string& GetFileName() const;

	const XMFLOAT2& GetSelectionPosition() const;
	uint32_t GetFoliagePlacingDetail() const;

	float GetTerrainHeightAt(float X, float Z);
	float GetTerrainHeightAt(int iX, int iZ);

	const SCBTerrainData& GetTerrainData() const;
	const DirectX::XMMATRIX& GetMaskingSpaceData() const;
	const SCBTerrainSelectionData& GetSelectionData();
	const DirectX::XMMATRIX& GetWaterWorldMatrix() const;
	const DirectX::XMMATRIX& GetWindRepresentationWorldMatrix() const;

public:
	void UpdateWind(float DeltaTime);

	void DrawTerrain(bool bDrawNormals);
	void DrawWater();
	void DrawFoliageCluster();
	void DrawWindRepresentation();

	void DrawHeightMapTexture();
	void DrawMaskingTexture();
	void DrawFoliagePlacingTexture();

public:
	static constexpr int KMaterialMaxCount{ 4 }; // It includes 1 main texture + 3 layer textures
	static constexpr int KTextureSubdivisionDetail{ 4 };

	static constexpr float KMinUniformScaling{ 1.0f };
	static constexpr float KMaxUniformScaling{ 16.0f };

	static constexpr float KHeightUnit{ 0.01f };
	static constexpr float KMaxHeight{ +8.0f };
	static constexpr float KMinHeight{ -KMaxHeight };
	static constexpr float KHeightRange{ KMaxHeight - KMinHeight };
	static constexpr float KHeightRangeHalf{ KHeightRange / 2.0f };

	static constexpr float KTessFactorMin{ 1.0f };
	static constexpr float KTessFactorMax{ 64.0f };

	static constexpr float KSelectionRadiusUnit{ 0.2f };
	static constexpr float KMinSelectionRadius{ 0.1f };
	static constexpr float KMaxSelectionRadius{ 16.0f };
	static constexpr float KDefaultSelectionRadius{ 1.0f };
	
	static constexpr int KMinSize{ 2 };
	static constexpr int KDefaultSize{ 10 };

	static constexpr size_t KMaskingTextureSlot{ 40 }; // @important
	static constexpr float KMaskingMinRatio{ 0.0f };
	static constexpr float KMaskingMaxRatio{ 1.0f };
	static constexpr float KMaskingDefaultRatio{ KMaskingMaxRatio };

	static constexpr float KMaskingMinAttenuation{ 0.0f };
	static constexpr float KMaskingMaxAttenuation{ 1.0f };
	static constexpr float KMaskingDefaultAttenuation{ KMaskingMinAttenuation };

	static constexpr uint32_t KMaskingMinDetail{ 1 };
	static constexpr uint32_t KMaskingMaxDetail{ 16 };
	static constexpr uint32_t KMaskingDefaultDetail{ 8 };

	static constexpr float KWaterHeightUnit{ 0.1f };
	static constexpr float KWaterMinHeight{ KMinHeight };
	static constexpr float KWaterMaxHeight{ KMaxHeight };

	static constexpr float KMaxFoliageInterval{ 1.0f };
	static constexpr float KMinFoliageInterval{ 0.1f };
	static constexpr uint32_t KDefaultFoliagePlacingDetail{ 7 };
	static constexpr uint32_t KMaxFoliagePlacingDetail{ 16 };
	static constexpr uint32_t KMinFoliagePlacingDetail{ 1 };

	static constexpr float KMinWindRadius{ 0.125f };
	static constexpr float KMaxWindRadius{ 16.0f };
	static constexpr float KMinWindVelocityElement{ -8.0f };
	static constexpr float KMaxWindVelocityElement{ +8.0f };

private:
	ID3D11Device* const				m_PtrDevice{};
	ID3D11DeviceContext* const		m_PtrDeviceContext{};
	CGame* const					m_PtrGame{};

private:
	std::unique_ptr<CObject2D>		m_Object2DTextureRep{};
	std::unique_ptr<CObject3D>		m_Object3DTerrain{};

private:
	XMFLOAT2						m_HeightMapTextureSize{};
	std::unique_ptr<CTexture>		m_HeightMapTexture{};

private:
	XMFLOAT2						m_MaskingTextureSize{};
	std::unique_ptr<CTexture>		m_MaskingTexture{};
	XMMATRIX						m_MatrixMaskingSpace{};

private:
	std::unique_ptr<CObject3D>		m_Object3DWater{};

private:
	std::vector<std::unique_ptr<CObject3D>>	m_vFoliages{};
	XMFLOAT2								m_FoliagePlacingTextureSize{};
	std::unique_ptr<CTexture>				m_FoliagePlacingTexture{};

	SCBWindData							m_CBWindData{};
	std::unique_ptr<CObject3D>			m_Object3DWindRep{};

private:
	SCBTerrainSelectionData	m_CBTerrainSelectionData{};

	EEditMode				m_eEditMode{};
	float					m_SetHeightValue{};
	float					m_DeltaHeightValue{ KHeightUnit * KHeightRange };
	float					m_MaskingRatio{ KMaskingDefaultRatio };

	EMaskingLayer			m_eMaskingLayer{};
	float					m_MaskingAttenuation{ KMaskingMinAttenuation };

	CMeshPorter::STERRData	m_TerrainFileData{ KTessFactorMin, KTessFactorMin, KMaskingDefaultDetail, KDefaultFoliagePlacingDetail };
	SCBTerrainData					m_CBTerrainData{};
};