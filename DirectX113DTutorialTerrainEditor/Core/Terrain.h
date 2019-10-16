#pragma once

#include "Object2D.h"
#include "Object3D.h"
#include "Texture.h"

class CGame;

enum class ETerrainEditMode
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

class CTerrain
{
	friend class CGame;

public:
	CTerrain(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, CGame* PtrGame) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGame{ PtrGame }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CTerrain() {}

public:
	void Create(const XMFLOAT2& TerrainSize, const string& TextureFileName, float MaskingDetail);
	void Load(const string& FileName);
	void Save(const string& FileName);

private:
	void CreateMaskingTexture(bool bShouldClear);

public:
	const XMFLOAT2& GetSize() const;
	int GetTextureCount() const;
	const string& GetTextureFileName(int TextureID) const;
	const XMFLOAT2& GetSelectionRoundUpPosition() const;
	float GetMaskingTextureDetail() const;

public:
	void SetTexture(int TextureID, const string& TextureFileName);
	void AddTexture(const string& TextureFileName);
	
	void UpdateVertexNormals();
	void UpdateHeight(bool bIsLeftButton);

	void SelectTerrain(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, bool bShouldEdit, bool bIsLeftButton);
	void UpdateMaskingTexture();

private:
	void UpdateSelection(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection);
	void ReleaseSelection();
	void UpdateHoverPosition(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection);
	void UpdateMasking(EMaskingLayer eLayer, const XMFLOAT2& Position, float Value, float Radius, bool bForceSet = false);

public:
	void SetSelectionSize(float& Size);
	void SetMaskingLayer(EMaskingLayer eLayer);
	void SetMaskingAttenuation(float Attenuation);
	void SetMaskingRadius(float Radius);
	void SetEditMode(ETerrainEditMode Mode, float Value);

public:
	void Draw(bool bUseTerrainSelector, bool bDrawNormals);
	void DrawMaskingTexture();

private:
	void UpdateVertex(SVertex3D& Vertex, bool bIsLeftButton);

public:
	static constexpr int KTextureMaxCount{ 5 }; // It includes 1 main texture + 4 layer textures

	static constexpr float KHeightUnit{ 0.1f };
	static constexpr float KMaxHeight{ +10.0f };
	static constexpr float KMinHeight{ -10.0f };
	
	static constexpr float KSelectionSizeUnit{ 1.0f };
	static constexpr float KSelectionMinSize{ 1.0f };
	static constexpr float KSelectionMaxSize{ 10.0f };
	
	static constexpr int KMinSize{ 2 };
	static constexpr float KMaskingRatioUnit{ 0.01f };
	static constexpr float KMaskingMinRatio{ 0.0f };
	static constexpr float KMaskingMaxRatio{ 1.0f };

	static constexpr float KMaskingAttenuationUnit{ 0.01f };
	static constexpr float KMaskingAttenuationMin{ 0.0f };
	static constexpr float KMaskingAttenuationMax{ 1.0f };

	static constexpr float KMaskingRadiusUnit{ 0.1f };
	static constexpr float KMaskingMinRadius{ 0.1f };
	static constexpr float KMaskingMaxRadius{ 8.0f };
	static constexpr float KMaskingDefaultRadius{ 1.0f };
	static constexpr float KMaskingMinDetail{ 1.0f };
	static constexpr float KMaskingMaxDetail{ 32.0f };
	static constexpr float KMaskingDefaultDetail{ 16.0f };

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};
	CGame*					m_PtrGame{};

private:
	unique_ptr<CObject2D>	m_Object2DMaskingTexture{};
	unique_ptr<CObject3D>	m_Object3D{};
	XMFLOAT2				m_Size{};
	unique_ptr<CTexture>	m_MaskingTexture{};
	XMFLOAT2				m_MaskingTextureSize{};
	vector<XMFLOAT4>		m_MaskingTextureRawData{};
	XMMATRIX				m_MatrixMaskingSpace{};

private:
	float					m_SelectionHalfSize{ KSelectionMinSize / 2.0f };
	XMFLOAT2				m_SelectionRoundUpPosition{};

	ETerrainEditMode		m_eEditMode{};
	float					m_EditValue{};

	XMFLOAT2				m_HoverPosition{};
	EMaskingLayer			m_eMaskingLayer{};
	float					m_MaskingRadius{ KMaskingDefaultRadius };
	float					m_MaskingAttenuation{ KMaskingAttenuationMin };
	float					m_MaskingTextureDetail{ KMaskingDefaultDetail };
};