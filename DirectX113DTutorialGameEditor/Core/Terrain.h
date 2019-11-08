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
	void Create(const XMFLOAT2& TerrainSize, const CMaterial& Material, int MaskingDetail, float UniformScaling = 1.0f);
	void Load(const string& FileName);
	void Save(const string& FileName);

private:
	void Scale(const XMVECTOR& Scaling);

public:
	void CreateFoliageCluster(const vector<string>& vFoliageFileNames, int PlacingDetail);
	void SetFoliageDensity(float Density);
	float GetFoliageDenstiy() const;

public:
	void SetWindVelocity(const XMFLOAT3& Velocity);
	void SetWindVelocity(const XMVECTOR& Velocity);
	const XMVECTOR& GetWindVelocity() const;
	void SetWindRadius(float Radius);
	float GetWindRadius() const;

private:
	void CreateTerrainObject3D(vector<CMaterial>& vMaterialsl);
	void CreateHeightMapTexture(bool bShouldClear);
	void CreateMaskingTexture(bool bShouldClear);
	void CreateWater();
	void CreateFoliagePlaceTexutre();
	void CreateWindRepresentation();

public:
	void AddMaterial(const CMaterial& Material);
	void SetMaterial(int MaterialID, const CMaterial& NewMaterial);
	const CMaterial& GetMaterial(int Index) const;
	
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
	void SetWaterTessFactor(float Value);
	float GetWaterTessFactor() const;

	const XMFLOAT2& GetSize() const;
	int GetMaterialCount() const;
	int GetMaskingDetail() const;

	bool HasFoliageCluster() const;

	const string& GetFileName() const;

	const XMFLOAT2& GetSelectionPosition() const;
	int GetFoliagePlacingDetail() const;

	float GetTerrainHeightAt(float X, float Z);
	float GetTerrainHeightAt(int iX, int iZ);

public:
	void UpdateWind(float DeltaTime);

	void Draw(bool bDrawNormals);
	void DrawHeightMapTexture();
	void DrawMaskingTexture();
	void DrawFoliagePlacingTexture();

private:
	void DrawWater();
	void DrawFoliageCluster();

public:
	static constexpr int KMaterialMaxCount{ 5 }; // It includes 1 main texture + 4 layer textures

	static constexpr float KMinUniformScaling{ 1.0f };
	static constexpr float KMaxUniformScaling{ 16.0f };

	static constexpr float KHeightUnit{ 0.01f };
	static constexpr float KMaxHeight{ +5.0f };
	static constexpr float KMinHeight{ -5.0f };
	static constexpr float KHeightRange{ KMaxHeight - KMinHeight };
	static constexpr float KHeightRangeHalf{ KHeightRange / 2.0f };

	static constexpr float KTessFactorUnit{ 0.1f };
	static constexpr float KTessFactorMin{ 2.0f };
	static constexpr float KTessFactorMax{ 64.0f };

	static constexpr float KSelectionRadiusUnit{ 0.1f };
	static constexpr float KMinSelectionRadius{ 0.1f };
	static constexpr float KMaxSelectionRadius{ 8.0f };
	static constexpr float KDefaultSelectionRadius{ 1.0f };
	
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

	static constexpr int KMaskingMinDetail{ 1 };
	static constexpr int KMaskingMaxDetail{ 16 };
	static constexpr int KMaskingDefaultDetail{ 8 };

	static constexpr float KWaterHeightUnit{ 0.1f };
	static constexpr float KWaterMinHeight{ KMinHeight };
	static constexpr float KWaterMaxHeight{ KMaxHeight };

	static constexpr float KMaxFoliageInterval{ 1.0f };
	static constexpr float KMinFoliageInterval{ 0.1f };
	static constexpr int KDefaultFoliagePlacingDetail{ 7 };
	static constexpr int KMaxFoliagePlacingDetail{ 16 };
	static constexpr int KMinFoliagePlacingDetail{ 1 };

	static constexpr float KMinWindRadius{ 0.125f };
	static constexpr float KMinWindVelocityElement{ -8.0f };
	static constexpr float KMaxWindVelocityElement{ +8.0f };

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
	SCBTerrainData					m_CBTerrainData{};
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
	unique_ptr<CMaterial::CTexture>	m_PerlinNoiseTexture{};
	XMFLOAT2						m_FoliagePlaceTextureSize{};
	unique_ptr<CMaterial::CTexture>	m_FoliagePlaceTexture{};
	vector<SPixel8UInt>				m_FoliagePlaceTextureRawData{};
	int								m_FoliagePlacingDetail{ KDefaultFoliagePlacingDetail };
	float							m_FoliageDenstiy{};

	SCBWindData						m_CBWindData{};
	unique_ptr<CObject3D>			m_Object3DWindRepresentation{};

private:
	SCBTerrainSelectionData			m_CBTerrainSelectionData{};

	EEditMode		m_eEditMode{};
	float			m_SetHeightValue{};
	float			m_DeltaHeightValue{ KHeightUnit * KHeightRange };
	float			m_MaskingRatio{ KMaskingDefaultRatio };

	EMaskingLayer	m_eMaskingLayer{};
	float			m_MaskingAttenuation{ KMaskingMinAttenuation };
	int				m_MaskingTextureDetail{ KMaskingDefaultDetail };

	string			m_FileName{};
};