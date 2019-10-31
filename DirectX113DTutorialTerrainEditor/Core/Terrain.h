#pragma once

#include "Object2D.h"
#include "Object3D.h"
#include "Material.h"

class CGame;

struct SCBVSTerrainData
{
	float TerrainSizeX{};
	float TerrainSizeZ{};
	float TerrainHeightRange{};
	float Pad{};
};

class CTerrain
{
public:
	enum class EEditMode
	{
		SetHeight,
		DeltaHeight,
		Masking
	};

	enum class EMaskingLayer
	{
		LayerR,
		LayerG,
		LayerB,
		LayerA,
	};

public:
	CTerrain(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, CGame* PtrGame) :
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

private:
	void CreateTerrainObject3D(vector<CMaterial>& vMaterialsl);
	void CreateHeightMapTexture(bool bShouldClear);
	void CreateMaskingTexture(bool bShouldClear);
	void CreateWater();

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

	void UpdateHoverPosition(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection);
	void UpdateMasking(EMaskingLayer eLayer, const XMFLOAT2& Position, float Value, float Radius, bool bForceSet = false);
	void UpdateMaskingTexture();

public:
	void SetSelectionSize(float& Size);
	void SetMaskingLayer(EMaskingLayer eLayer);
	void SetMaskingAttenuation(float Attenuation);
	void SetMaskingRadius(float Radius);
	void SetSetHeightValue(float Value);
	void SetDeltaHeightValue(float Value);
	void SetMaskingValue(float Value);

	void ShouldTessellate(bool Value);
	bool ShouldTessellate() const;

	void ShouldDrawWater(bool Value);
	bool ShouldDrawWater() const;

	void SetEditMode(EEditMode Mode);
	CTerrain::EEditMode GetEditMode();
	void SetWaterHeight(float Value);
	float GetWaterHeight() const;
	void SetTessFactor(float Value);
	float GetTessFactor() const;

	const XMFLOAT2& GetSize() const;
	int GetMaterialCount() const;
	const XMFLOAT2& GetSelectionPosition() const;
	float GetMaskingDetail() const;

	const string& GetFileName() const;

public:
	void Draw(bool bDrawNormals);
	void DrawHeightMapTexture();
	void DrawMaskingTexture();

private:
	void DrawWater();

public:
	static constexpr int KMaterialMaxCount{ 5 }; // It includes 1 main texture + 4 layer textures

	static constexpr float KHeightUnit{ 0.01f };
	static constexpr float KMaxHeight{ +5.0f };
	static constexpr float KMinHeight{ -5.0f };
	static constexpr float KHeightRange{ KMaxHeight - KMinHeight };
	static constexpr float KHeightRangeHalf{ KHeightRange / 2.0f };

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

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};
	CGame*					m_PtrGame{};

private:
	unique_ptr<CObject2D>			m_Object2DTextureRepresentation{};
	unique_ptr<CObject3D>			m_Object3DTerrain{};
	XMFLOAT2						m_Size{};

	XMFLOAT2						m_HeightMapTextureSize{};
	unique_ptr<CMaterial::CTexture>	m_HeightMapTexture{};
	vector<SPixel8UInt>				m_HeightMapTextureRawData{};
	SCBVSTerrainData				m_cbTerrainData{};
	float							m_TessFactor{ KTessFactorMin };

	XMFLOAT2						m_MaskingTextureSize{};
	unique_ptr<CMaterial::CTexture>	m_MaskingTexture{};
	vector<SPixel32UInt>			m_MaskingTextureRawData{};
	XMMATRIX						m_MatrixMaskingSpace{};

private:
	unique_ptr<CObject3D>			m_Object3DWater{};
	unique_ptr<CMaterial::CTexture>	m_WaterNormalTexture{};
	unique_ptr<CMaterial::CTexture>	m_WaterDisplacementTexture{};
	float							m_WaterHeight{};
	bool							m_bShouldDrawWater{ true };

private:
	float			m_SelectionHalfSize{ KSelectionMinSize / 2.0f };
	XMFLOAT2		m_SelectionPosition{};

	EEditMode		m_eEditMode{};
	float			m_SetHeightValue{};
	float			m_DeltaHeightValue{ KHeightUnit * KHeightRange };
	float			m_MaskingRatio{ KMaskingDefaultRatio };

	XMFLOAT2		m_HoverPosition{};
	EMaskingLayer	m_eMaskingLayer{};
	float			m_MaskingRadius{ KMaskingDefaultRadius };
	float			m_MaskingAttenuation{ KMaskingMinAttenuation };
	float			m_MaskingTextureDetail{ KMaskingDefaultDetail };

	string			m_FileName{};
};