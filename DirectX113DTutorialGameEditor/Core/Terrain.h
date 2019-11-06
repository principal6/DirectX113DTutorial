#pragma once

#include "Object2D.h"
#include "Object3D.h"
#include "Material.h"

class CGame;

class CTerrain
{
public:
	enum class EEditMode
	{
		SetHeight,
		DeltaHeight,
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

	struct SCBVSTerrainData
	{
		float TerrainSizeX{};
		float TerrainSizeZ{};
		float TerrainHeightRange{};
		float Pad{};
	};

	struct SCBVSFoliageData
	{
		float	Density{};
		int		CountX{};
		int		CountZ{};
		float	Reserved{};
	};

	struct SCBPSTerrainSelectionData
	{
		BOOL		bShowSelection{};
		float		SelectionHalfSize{ KSelectionMinSize / 2.0f };
		XMFLOAT2	DigitalPosition{};

		BOOL		bUseCircularSelection{};
		float		SelectionRadius{};
		XMFLOAT2	AnaloguePosition{};
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
	void Create(const XMFLOAT2& TerrainSize, const CMaterial& Material, float MaskingDetail);
	void Load(const string& FileName);
	void Save(const string& FileName);

	void CreateFoliageCluster(const vector<string>& vFoliageFileNames, int PlacingDetail);
	void SetFoliageDensity(float Density);
	float GetFoliageDenstiy() const;

private:
	void CreateTerrainObject3D(vector<CMaterial>& vMaterialsl);
	void CreateHeightMapTexture(bool bShouldClear);
	void CreateMaskingTexture(bool bShouldClear);
	void CreateWater();
	void CreateFoliagePlaceTexutre();

public:
	void AddMaterial(const CMaterial& Material);
	void SetMaterial(int MaterialID, const CMaterial& NewMaterial);
	const CMaterial& GetMaterial(int Index) const;
	
	void Select(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, bool bShouldEdit, bool bIsLeftButton);

private:
	void UpdateSelection(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection);
	void ReleaseSelection();
	void UpdateHeights(bool bIsLeftButton);
	void UpdateHeight(size_t iPixel, bool bIsLeftButton);
	void UpdateHeightMapTexture();

	void UpdateMasking(EMaskingLayer eLayer, const XMFLOAT2& Position, float Value, float Radius, bool bForceSet = false);
	void UpdateMaskingTexture();

	void UpdateFoliagePlacing(float Radius, const XMFLOAT2& Position, bool bErase);
	void UpdateFoliagePlaceTexture();

public:
	void SetSelectionSize(float& Size);
	float GetSelectionSize() const;
	void SetMaskingLayer(EMaskingLayer eLayer);
	CTerrain::EMaskingLayer GetMaskingLayer() const;
	void SetMaskingAttenuation(float Attenuation);
	float GetMaskingAttenuation() const;
	void SetMaskingRadius(float Radius);
	float GetMaskingRadius() const;
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
	void SetWaterTessFactor(float Value);
	float GetWaterTessFactor() const;

	const XMFLOAT2& GetSize() const;
	int GetMaterialCount() const;
	const XMFLOAT2& GetSelectionPosition() const;
	float GetMaskingDetail() const;

	const string& GetFileName() const;

public:
	void Draw(bool bDrawNormals);
	void DrawHeightMapTexture();
	void DrawMaskingTexture();
	void DrawFoliagePlacingTexture();

private:
	void DrawWater();
	void DrawFoliageCluster();

public:
	static constexpr int KMaterialMaxCount{ 5 }; // It includes 1 main texture + 4 layer textures

	static constexpr float KHeightUnit{ 0.01f };
	static constexpr float KMaxHeight{ +5.0f };
	static constexpr float KMinHeight{ -5.0f };
	static constexpr float KHeightRange{ KMaxHeight - KMinHeight };
	static constexpr float KHeightRangeHalf{ KHeightRange / 2.0f };

	static constexpr float KTessFactorUnit{ 0.1f };
	static constexpr float KTessFactorMin{ 2.0f };
	static constexpr float KTessFactorMax{ 64.0f };

	static constexpr float KSelectionSizeUnit{ 1.0f };
	static constexpr float KSelectionMinSize{ 1.0f };
	static constexpr float KSelectionMaxSize{ 10.0f };
	
	static constexpr int KMinSize{ 2 };
	static constexpr int KDefaultSize{ 10 };

	static constexpr size_t KMaskingTextureSlot{ 10 };
	static constexpr float KMaskingRatioUnit{ 0.01f };
	static constexpr float KMaskingMinRatio{ 0.0f };
	static constexpr float KMaskingMaxRatio{ 1.0f };
	static constexpr float KMaskingDefaultRatio{ KMaskingMaxRatio };

	static constexpr float KMaskingAttenuationUnit{ 0.01f };
	static constexpr float KMaskingMinAttenuation{ 0.0f };
	static constexpr float KMaskingMaxAttenuation{ 1.0f };
	static constexpr float KMaskingDefaultAttenuation{ KMaskingMinAttenuation };

	static constexpr float KMaskingRadiusUnit{ 0.1f };
	static constexpr float KMaskingMinRadius{ 0.1f };
	static constexpr float KMaskingMaxRadius{ 8.0f };
	static constexpr float KMaskingDefaultRadius{ 1.0f };

	static constexpr float KMaskingMinDetail{ 1.0f };
	static constexpr float KMaskingMaxDetail{ 16.0f };
	static constexpr float KMaskingDefaultDetail{ 8.0f };

	static constexpr float KWaterHeightUnit{ 0.1f };
	static constexpr float KWaterMinHeight{ KMinHeight };
	static constexpr float KWaterMaxHeight{ KMaxHeight };

	static constexpr float KMaxFoliageInterval{ 1.0f };
	static constexpr float KMinFoliageInterval{ 0.1f };

private:
	ID3D11Device* const			m_PtrDevice{};
	ID3D11DeviceContext* const	m_PtrDeviceContext{};
	CGame* const				m_PtrGame{};

private:
	unique_ptr<CObject2D>			m_Object2DTextureRepresentation{};
	unique_ptr<CObject3D>			m_Object3DTerrain{};
	XMFLOAT2						m_Size{};

	XMFLOAT2						m_HeightMapTextureSize{};
	unique_ptr<CMaterial::CTexture>	m_HeightMapTexture{};
	vector<SPixel8UInt>				m_HeightMapTextureRawData{};
	SCBVSTerrainData				m_cbTerrainData{};
	float							m_TerrainTessFactor{ KTessFactorMin };

	XMFLOAT2						m_MaskingTextureSize{};
	unique_ptr<CMaterial::CTexture>	m_MaskingTexture{};
	vector<SPixel32UInt>			m_MaskingTextureRawData{};
	XMMATRIX						m_MatrixMaskingSpace{};

private:
	unique_ptr<CObject3D>			m_Object3DWater{};
	unique_ptr<CMaterial::CTexture>	m_WaterNormalTexture{};
	unique_ptr<CMaterial::CTexture>	m_WaterDisplacementTexture{};
	float							m_WaterHeight{};
	float							m_WaterTessFactor{ KTessFactorMin };
	bool							m_bShouldDrawWater{ true };

private:
	vector<unique_ptr<CObject3D>>	m_vFoliages{};
	bool							m_bHasFoliageCluster{ false };
	SCBVSFoliageData				m_cbVSFoliageData{};
	unique_ptr<CMaterial::CTexture>	m_PerlinNoiseTexture{};
	XMFLOAT2						m_FoliagePlaceTextureSize{};
	unique_ptr<CMaterial::CTexture>	m_FoliagePlaceTexture{};
	vector<SPixel8UInt>				m_FoliagePlaceTextureRawData{};
	int								m_FoliagePlacingDetail{};

private:
	SCBPSTerrainSelectionData	m_cbPSTerrainSelectionData{};

	EEditMode		m_eEditMode{};
	float			m_SetHeightValue{};
	float			m_DeltaHeightValue{ KHeightUnit * KHeightRange };
	float			m_MaskingRatio{ KMaskingDefaultRatio };

	EMaskingLayer	m_eMaskingLayer{};
	float			m_MaskingRadius{ KMaskingDefaultRadius };
	float			m_MaskingAttenuation{ KMaskingMinAttenuation };
	float			m_MaskingTextureDetail{ KMaskingDefaultDetail };

	string			m_FileName{};
};