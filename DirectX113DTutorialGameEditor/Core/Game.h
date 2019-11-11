#pragma once

#define NOMINMAX 0

#include <Windows.h>
#include <chrono>

#include "Math.h"
#include "Camera.h"
#include "Shader.h"
#include "Material.h"
#include "Object3D.h"
#include "Object3DLine.h"
#include "Object2D.h"
#include "ParticlePool.h"
#include "PrimitiveGenerator.h"
#include "Terrain.h"

#include "TinyXml2/tinyxml2.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

class CGame
{
public:
	enum class EBaseShader
	{
		VSBase,
		VSInstance,
		VSAnimation,
		VSSky,
		VSLine,
		VSGizmo,
		VSTerrain,
		VSFoliage,
		VSParticle,
		VSBase2D,

		HSTerrain,
		HSWater,

		DSTerrain,
		DSWater,

		GSNormal,
		GSParticle,

		PSBase,
		PSVertexColor,
		PSDynamicSky,
		PSCloud,
		PSLine,
		PSGizmo,
		PSTerrain,
		PSWater,
		PSFoliage,
		PSParticle,
		PSCamera,

		PSBase2D,
		PSMasking2D,
		PSHeightMap2D
	};

	struct SCBSpaceWVPData
	{
		XMMATRIX	World{};
		XMMATRIX	ViewProjection{};
	};

	struct SCBSpaceVPData
	{
		XMMATRIX	ViewProjection{};
	};

	struct SCBSpace2DData
	{
		XMMATRIX	World{};
		XMMATRIX	Projection{};
	};

	struct SCBAnimationBonesData
	{
		XMMATRIX	BoneMatrices[KMaxBoneMatrixCount]{};
	};

	struct SCBCameraData
	{
		XMVECTOR	EyePosition{};
	};

	struct SCBTessFactorData
	{
		float		TessFactor{};
		float		Pads[3]{};
	};

	struct SCBDisplacementData
	{
		BOOL		bUseDisplacement{};
		float		Pads[3]{};
	};

	struct SCBPSBaseFlagsData
	{
		BOOL		bUseTexture{};
		BOOL		bUseLighting{};
		BOOL		Pads[2]{};
	};

	struct SCBLightData
	{
		XMVECTOR	DirectionalLightDirection{ XMVectorSet(0, 1, 0, 0) };
		XMVECTOR	DirectionalLightColor{ XMVectorSet(1, 1, 1, 1) };
		XMFLOAT3	AmbientLightColor{ 1, 1, 1 };
		float		AmbientLightIntensity{ 0.5f };
		XMVECTOR	EyePosition{};
	};

	struct SCBMaterialData
	{
		XMFLOAT3	MaterialAmbient{};
		float		SpecularExponent{ 1 };
		XMFLOAT3	MaterialDiffuse{};
		float		SpecularIntensity{ 0 };
		XMFLOAT3	MaterialSpecular{};
		BOOL		bHasDiffuseTexture{};

		BOOL		bHasNormalTexture{};
		BOOL		bHasOpacityTexture{};
		BOOL		Pad[2]{};
	};

	struct SCBSkyTimeData
	{
		float		SkyTime{};
		float		Pads[3]{};
	};

	struct SCBGizmoColorFactorData
	{
		XMVECTOR	ColorFactor{};
	};

	struct SCBTerrainMaskingSpaceData
	{
		XMMATRIX	Matrix{};
	};

	struct SCBPS2DFlagsData
	{
		BOOL		bUseTexture{};
		BOOL		Pad[3]{};
	};

	struct SCBWaterTimeData
	{
		float		Time{};
		float		Pads[3]{};
	};

	struct SCBEditorTimeData
	{
		float		NormalizedTime{};
		float		NormalizedTimeHalfSpeed{};
		float		Pads[2]{};
	};

	struct SCBCameraSelectionData
	{
		BOOL		bIsSelected{ false };
		float		Pads[3]{};
	};

	enum class EFlagsRendering
	{
		None = 0x000,
		DrawWireFrame = 0x001,
		DrawNormals = 0x002,
		UseLighting = 0x004,
		DrawMiniAxes = 0x008,
		DrawPickingData = 0x010,
		DrawBoundingSphere = 0x020,
		Use3DGizmos = 0x040,
		TessellateTerrain = 0x080,
		DrawTerrainHeightMapTexture = 0x100,
		DrawTerrainMaskingTexture = 0x200,
		DrawTerrainFoliagePlacingTexture = 0x400
	};

	enum class ERasterizerState
	{
		CullNone,
		CullClockwise,
		CullCounterClockwise,
		WireFrame
	};

	enum class E3DGizmoMode
	{
		Translation,
		Rotation,
		Scaling
	};

	enum class E3DGizmoAxis
	{
		None,
		AxisX,
		AxisY,
		AxisZ
	};

	enum class EMode
	{
		Play,
		Edit,
		Test
	};

	enum class EEditMode
	{
		EditObject,
		EditTerrain
	};
	
	struct SSkyData
	{
		struct SSkyObjectData
		{
			XMFLOAT2	UVOffset{};
			XMFLOAT2	UVSize{};
			float		WidthHeightRatio{};
		};

		bool			bIsDataSet{ false };
		bool			bIsDynamic{ false };
		std::string		TextureFileName{};
		SSkyObjectData	Sun{};
		SSkyObjectData	Moon{};
		SSkyObjectData	Cloud{};
	};

	struct SEditorGUIBools
	{
		bool bShowWindowPropertyEditor{ true };
		bool bShowWindowSceneEditor{ true };
		bool bShowPopupTerrainGenerator{ false };
		bool bShowPopupObjectAdder{ false };
	};

	struct SObject3DPickingCandiate
	{
		SObject3DPickingCandiate() {};
		SObject3DPickingCandiate(CObject3D* _PtrObject3D, XMVECTOR _T) : PtrObject3D{ _PtrObject3D }, T{ _T } {}
		SObject3DPickingCandiate(CObject3D* _PtrObject3D, int _InstanceID, XMVECTOR _T) : PtrObject3D{ _PtrObject3D }, InstanceID{ _InstanceID }, T{ _T } {}

		CObject3D*	PtrObject3D{};
		int			InstanceID{ -1 };
		XMVECTOR	T{};
		bool		bHasFailedPickingTest{ false };
	};

public:
	CGame(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGame() {}

public:
	void CreateWin32(WNDPROC const WndProc, const std::string& WindowName, bool bWindowed);
	void CreateSpriteFont(const std::wstring& FontFileName);
	void Destroy();

private:
	void CreateWin32Window(WNDPROC const WndProc, const std::string& WindowName);
	void InitializeDirectX(bool bWindowed);
	void InitializeEditorAssets();
	void InitializeImGui();

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();
	void CreateDepthStencilStates();
	void CreateBlendStates();
	void CreateInputDevices();
	void CreateBaseShaders();
	void CreateMiniAxes();
	void CreatePickingRay();
	void CreateBoundingSphere();
	void CreatePickedTriangle();
	void Create3DGizmos();

public:
	void LoadScene(const std::string& FileName);
	void SaveScene(const std::string& FileName);

	// Advanced settings
public:
	void SetPerspective(float FOV, float NearZ, float FarZ);
	void SetRenderingFlags(EFlagsRendering Flags);
	void ToggleGameRenderingFlags(EFlagsRendering Flags);
	void Set3DGizmoMode(E3DGizmoMode Mode);
	void SetUniversalRSState();
	void SetUniversalbUseLighiting();
	E3DGizmoMode Get3DGizmoMode() const { return m_e3DGizmoMode; }
	CommonStates* GetCommonStates() const { return m_CommonStates.get(); }

	// Shader-related settings
public:
	void UpdateCBSpace(const XMMATRIX& World = KMatrixIdentity);
	void UpdateCBAnimationBoneMatrices(const XMMATRIX* const BoneMatrices);
	void UpdateCBAnimationData(const CObject3D::SCBAnimationData& Data);
	void UpdateCBTerrainData(const CTerrain::SCBTerrainData& Data);
	void UpdateCBWindData(const CTerrain::SCBWindData& Data);

	void UpdateCBTessFactor(float TessFactor);
	void UpdateCBDisplacementData(bool bUseDisplacement);

	void UpdateCBMaterial(const CMaterial& Material);
	void UpdateCBTerrainMaskingSpace(const XMMATRIX& Matrix);
	void UpdateCBTerrainSelection(const CTerrain::SCBTerrainSelectionData& Selection);

public:
	void CreateDynamicSky(const std::string& SkyDataFileName, float ScalingFactor);
	void CreateStaticSky(float ScalingFactor);

private:
	void LoadSkyObjectData(const tinyxml2::XMLElement* const xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData);

public:
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color);
	void SetDirectionalLightDirection(const XMVECTOR& LightSourcePosition);
	void SetDirectionalLightColor(const XMVECTOR& Color);
	const XMVECTOR& GetDirectionalLightDirection() const;
	const XMVECTOR& GetDirectionalLightColor() const;

	void SetAmbientlLight(const XMFLOAT3& Color, float Intensity);
	const XMFLOAT3& GetAmbientLightColor() const;
	float GetAmbientLightIntensity() const;

public:
	void CreateTerrain(const XMFLOAT2& TerrainSize, const CMaterial& Material, uint32_t MaskingDetail, float UniformScaling);
	void LoadTerrain(const std::string& TerrainFileName);
	void SaveTerrain(const std::string& TerrainFileName);
	void AddTerrainMaterial(const CMaterial& Material);
	void SetTerrainMaterial(int MaterialID, const CMaterial& Material);
	CTerrain* GetTerrain() const { return m_Terrain.get(); }

// Object pool
public:	
	void InsertCamera(const std::string& Name);
	void DeleteCamera(const std::string& Name);
	void ClearCameras();
	CCamera* GetCamera(const std::string& Name, bool bShowWarning = true);
	const std::map<std::string, size_t>& GetCameraMap() const { return m_mapCameraNameToIndex; }

private:
	void CreateEditorCamera();
	CCamera* GetEditorCamera();

public:
	CShader* AddCustomShader();
	CShader* GetCustomShader(size_t Index) const;
	CShader* GetBaseShader(EBaseShader eShader) const;

	void InsertObject3D(const std::string& Name);
	void DeleteObject3D(const std::string& Name);
	void ClearObject3Ds();
	CObject3D* GetObject3D(const std::string& Name, bool bShowWarning = true) const;
	const std::map<std::string, size_t>& GetObject3DMap() const { return m_mapObject3DNameToIndex; }

	void InsertObject3DLine(const std::string& Name);
	CObject3DLine* GetObject3DLine(const std::string& Name) const;

	void InsertObject2D(const std::string& Name);
	void DeleteObject2D(const std::string& Name);
	void ClearObject2Ds();
	CObject2D* GetObject2D(const std::string& Name, bool bShowWarning = true) const;
	const std::map<std::string, size_t>& GetObject2DMap() const { return m_mapObject2DNameToIndex; }

	CMaterial* AddMaterial(const CMaterial& Material);
	CMaterial* GetMaterial(const std::string& Name) const;
	void ClearMaterials();
	size_t GetMaterialCount() const;
	void ChangeMaterialName(const std::string& OldName, const std::string& NewName);
	void ReloadMaterial(const std::string& Name);
	const std::map<std::string, size_t>& GetMaterialMap() const { return m_mapMaterialNameToIndex; }
	CMaterial::CTexture* GetMaterialTexture(CMaterial::CTexture::EType eType, const std::string& Name) const;

private:
	void CreateMaterialTexture(CMaterial::CTexture::EType eType, CMaterial& Material);

public:
	void SetMode(EMode eMode);
	EMode GetMode() const { return m_eMode; }

	void SetEditMode(EEditMode eEditMode, bool bForcedSet = false);
	EEditMode GetEditMode() const { return m_eEditMode; }

public:
	void NotifyMouseLeftDown();
	void NotifyMouseLeftUp();

private:
	bool Pick();
	const std::string& GetPickedObject3DName() const;
	int GetPickedInstanceID() const;

	void SelectObject3D(const std::string& Name);
	void DeselectObject3D();
	bool IsAnyObject3DSelected() const;
	CObject3D* GetSelectedObject3D();
	const std::string& GetSelectedObject3DName() const;

	void SelectObject2D(const std::string& Name);
	void DeselectObject2D();
	bool IsAnyObject2DSelected() const;
	CObject2D* GetSelectedObject2D();
	const std::string& GetSelectedObject2DName() const;

	void SelectCamera(const std::string& Name);
	void DeselectCamera();
	bool IsAnyCameraSelected() const;
	CCamera* GetSelectedCamera();
	const std::string& GetSelectedCameraName() const;
	CCamera* GetCurrentCamera();

	void Select3DGizmos();
	void Deselect3DGizmos();
	bool IsGizmoHovered() const;
	bool IsGizmoSelected() const;
	bool ShouldSelectRotationGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis);
	bool ShouldSelectTranslationScalingGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis);

	void DeselectAll();

public:
	void SelectInstance(int InstanceID);
	void DeselectInstance();
	bool IsAnyInstanceSelected() const;
	int GetSelectedInstanceID() const;

private:
	void SelectTerrain(bool bShouldEdit, bool bIsLeftButton);

private:
	void CastPickingRay();
	void PickBoundingSphere();
	bool PickTriangle();

public:
	void BeginRendering(const FLOAT* ClearColor);
	void Update();
	void Draw();
	void EndRendering();

public:
	auto GethWnd() const->HWND { return m_hWnd; }
	auto GetDevicePtr() const->ID3D11Device* { return m_Device.Get(); }
	auto GetDeviceContextPtr() const->ID3D11DeviceContext* { return m_DeviceContext.Get(); }
	auto GetKeyState() const->Keyboard::State;
	auto GetMouseState() const->Mouse::State;
	auto GetSpriteBatchPtr() const->SpriteBatch* { return m_SpriteBatch.get(); }
	auto GetSpriteFontPtr() const->SpriteFont* { return m_SpriteFont.get(); }
	auto GetWindowSize() const->const XMFLOAT2&;
	auto GetSkyTime() const->float;
	auto GetTransposedViewProjectionMatrix() const->XMMATRIX;
	auto GetDepthStencilStateLessEqualNoWrite() const->ID3D11DepthStencilState* { return m_DepthStencilStateLessEqualNoWrite.Get(); }
	auto GetBlendStateAlphaToCoverage() const->ID3D11BlendState* { return m_BlendAlphaToCoverage.Get(); }
	auto GetWorkingDirectory() const->const char* { return m_WorkingDirectory; }
	auto GetDeltaTime() const->float { return m_DeltaTimeF; }

private:
	void UpdateObject3D(CObject3D* const PtrObject3D);
	void DrawObject3D(const CObject3D* const PtrObject3D);
	void DrawObject3DBoundingSphere(const CObject3D* const PtrObject3D);

	void DrawObject3DLines();

	void DrawObject2Ds();

	void DrawMiniAxes();

	void UpdatePickingRay();
	void DrawPickingRay();
	void DrawPickedTriangle();

	void DrawSky(float DeltaTime);
	void DrawTerrain(float DeltaTime);

	void Draw3DGizmos();
	void Draw3DGizmoTranslations(E3DGizmoAxis Axis);
	void Draw3DGizmoRotations(E3DGizmoAxis Axis);
	void Draw3DGizmoScalings(E3DGizmoAxis Axis);
	void Draw3DGizmo(CObject3D* const Gizmo, bool bShouldHighlight);

	void DrawCameraRepresentations();

	void DrawEditorGUI();
	void DrawEditorGUIMenuBar();
	void DrawEditorGUIPopups();
	void DrawEditorGUIPopupTerrainGenerator();
	void DrawEditorGUIPopupObjectAdder();
	void DrawEditorGUIWindowPropertyEditor();
	void DrawEditorGUIWindowSceneEditor();

public:
	static constexpr float KTranslationMinLimit{ -1000.0f };
	static constexpr float KTranslationMaxLimit{ +1000.0f };
	static constexpr float KTranslationUnit{ +0.01f };
	static constexpr float KRotationMaxLimit{ +XM_2PI };
	static constexpr float KRotationMinLimit{ -XM_2PI };
	static constexpr int KRotation360MaxLimit{ 360 };
	static constexpr int KRotation360MinLimit{ 360 };
	static constexpr int KRotation360Unit{ 1 };
	static constexpr float KRotation360To2PI{ 1.0f / 360.0f * XM_2PI };
	static constexpr float KRotation2PITo360{ 1.0f / XM_2PI * 360.0f };
	static constexpr float KScalingMaxLimit{ +100.0f };
	static constexpr float KScalingMinLimit{ +0.001f };
	static constexpr float KScalingUnit{ +0.001f };
	static constexpr float KBSCenterOffsetUnit{ +0.01f };
	static constexpr float KBSCenterOffsetMinLimit{ -10.0f };
	static constexpr float KBSCenterOffsetMaxLimit{ +10.0f };
	static constexpr float KBSRadiusUnit{ +0.01f };
	static constexpr float KBSRadiusMinLimit{ 0.001f };
	static constexpr float KBSRadiusMaxLimit{ 10.0f };
	static constexpr int KObjectNameMaxLength{ 100 };

private:
	static constexpr float KDefaultFOV{ 50.0f / 360.0f * XM_2PI };
	static constexpr float KDefaultNearZ{ 0.1f };
	static constexpr float KDefaultFarZ{ 1000.0f };
	static constexpr float KSkyDistance{ 100.0f };
	static constexpr float KSkyTimeFactorAbsolute{ 0.04f };
	static constexpr float KPickingRayLength{ 1000.0f };
	static constexpr uint32_t KSkySphereSegmentCount{ 32 };
	static constexpr XMVECTOR KColorWhite{ 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr XMVECTOR KSkySphereColorUp{ 0.1f, 0.5f, 1.0f, 1.0f };
	static constexpr XMVECTOR KSkySphereColorBottom{ 1.2f, 1.2f, 1.2f, 1.0f };
	static constexpr float K3DGizmoSelectionRadius{ 1.1f };
	static constexpr float K3DGizmoSelectionLowBoundary{ 0.8f };
	static constexpr float K3DGizmoSelectionHighBoundary{ 1.2f };
	static constexpr float K3DGizmoMovementFactor{ 0.01f };

private:
	std::unique_ptr<CShader>	m_VSBase{};
	std::unique_ptr<CShader>	m_VSInstance{};
	std::unique_ptr<CShader>	m_VSAnimation{};
	std::unique_ptr<CShader>	m_VSSky{};
	std::unique_ptr<CShader>	m_VSLine{};
	std::unique_ptr<CShader>	m_VSGizmo{};
	std::unique_ptr<CShader>	m_VSTerrain{};
	std::unique_ptr<CShader>	m_VSFoliage{};
	std::unique_ptr<CShader>	m_VSParticle{};

	std::unique_ptr<CShader>	m_VSBase2D{};

	std::unique_ptr<CShader>	m_HSTerrain{};
	std::unique_ptr<CShader>	m_HSWater{};

	std::unique_ptr<CShader>	m_DSTerrain{};
	std::unique_ptr<CShader>	m_DSWater{};

	std::unique_ptr<CShader>	m_GSNormal{};
	std::unique_ptr<CShader>	m_GSParticle{};

	std::unique_ptr<CShader>	m_PSBase{};
	std::unique_ptr<CShader>	m_PSVertexColor{};
	std::unique_ptr<CShader>	m_PSDynamicSky{};
	std::unique_ptr<CShader>	m_PSCloud{};
	std::unique_ptr<CShader>	m_PSLine{};
	std::unique_ptr<CShader>	m_PSGizmo{};
	std::unique_ptr<CShader>	m_PSTerrain{};
	std::unique_ptr<CShader>	m_PSWater{};
	std::unique_ptr<CShader>	m_PSFoliage{};
	std::unique_ptr<CShader>	m_PSParticle{};
	std::unique_ptr<CShader>	m_PSCamera{};

	std::unique_ptr<CShader>	m_PSBase2D{};
	std::unique_ptr<CShader>	m_PSMasking2D{};
	std::unique_ptr<CShader>	m_PSHeightMap2D{};

private:
	SCBSpaceWVPData				m_CBSpaceWVPData{};
	SCBSpaceVPData				m_CBSpaceVPData{};
	SCBSpace2DData				m_CBSpace2DData{};
	SCBAnimationBonesData		m_CBAnimationBonesData{};
	CObject3D::SCBAnimationData	m_CBAnimationData{};
	CTerrain::SCBTerrainData	m_CBTerrainData{};
	CTerrain::SCBWindData		m_CBWindData{};

	SCBCameraData				m_CBCameraData{};
	SCBTessFactorData			m_CBTessFactorData{};
	SCBDisplacementData			m_CBDisplacementData{};

	SCBLightData				m_CBLightData{};
	SCBMaterialData				m_CBMaterialData{};
	SCBPSBaseFlagsData			m_cbPSBaseFlagsData{};
	SCBGizmoColorFactorData		m_CBGizmoColorFactorData{};

	SCBPS2DFlagsData					m_cbPS2DFlagsData{};
	SCBTerrainMaskingSpaceData			m_CBTerrainMaskingSpaceData{};
	CTerrain::SCBTerrainSelectionData	m_CBTerrainSelectionData{};
	SCBSkyTimeData						m_CBSkyTimeData{};
	SCBWaterTimeData					m_CBWaterTimeData{};
	SCBEditorTimeData					m_CBEditorTimeData{};
	SCBCameraSelectionData				m_CBCameraSelectionData{};

private:
	std::vector<std::unique_ptr<CShader>>				m_vShaders{};
	std::vector<std::unique_ptr<CObject3D>>				m_vObject3Ds{};
	std::vector<std::unique_ptr<CObject3DLine>>			m_vObject3DLines{};
	std::vector<std::unique_ptr<CObject2D>>				m_vObject2Ds{};
	std::vector<std::unique_ptr<CMaterial>>				m_vMaterials{};
	std::vector<std::unique_ptr<CMaterial::CTexture>>	m_vMaterialDiffuseTextures{};
	std::vector<std::unique_ptr<CMaterial::CTexture>>	m_vMaterialNormalTextures{};
	std::vector<std::unique_ptr<CMaterial::CTexture>>	m_vMaterialDisplacementTextures{};
	std::vector<std::unique_ptr<CMaterial::CTexture>>	m_vMaterialOpacityTextures{};

	std::unique_ptr<CObject3DLine>				m_Object3DLinePickingRay{};
	std::unique_ptr<CObject3D>					m_Object3DPickedTriangle{};

	std::unique_ptr<CObject3D>					m_Object3DBoundingSphere{};

	std::vector<std::unique_ptr<CObject3D>>		m_vObject3DMiniAxes{};

	std::string								m_SkyFileName{};
	float									m_SkyScalingFactor{};
	SSkyData								m_SkyData{};
	CMaterial								m_SkyMaterial{};
	std::unique_ptr<CObject3D>				m_Object3DSkySphere{};
	std::unique_ptr<CObject3D>				m_Object3DSun{};
	std::unique_ptr<CObject3D>				m_Object3DMoon{};
	std::unique_ptr<CObject3D>				m_Object3DCloud{};

	std::map<std::string, size_t>	m_mapMaterialNameToIndex{};
	std::map<std::string, size_t>	m_mapCameraNameToIndex{};
	std::map<std::string, size_t>	m_mapObject3DNameToIndex{};
	std::map<std::string, size_t>	m_mapObject3DLineNameToIndex{};
	std::map<std::string, size_t>	m_mapObject2DNameToIndex{};

private:
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationPitch{};
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationYaw{};
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationRoll{};

	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationX{};
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationY{};
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationZ{};

	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingX{};
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingY{};
	std::unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingZ{};

	bool						m_bIsGizmoHovered{ false };
	bool						m_bIsGizmoSelected{ false };
	E3DGizmoAxis				m_e3DGizmoSelectedAxis{};
	E3DGizmoMode				m_e3DGizmoMode{};
	float						m_3DGizmoDistanceScalar{};

private:
	std::vector<D3D11_VIEWPORT>	m_vViewports{};

private:
	HWND			m_hWnd{};
	HINSTANCE		m_hInstance{};
	XMFLOAT2		m_WindowSize{};
	char			m_WorkingDirectory[MAX_PATH]{};

private:
	XMMATRIX		m_MatrixProjection{};
	XMMATRIX		m_MatrixProjection2D{};
	float			m_NearZ{};
	float			m_FarZ{};

	XMMATRIX								m_MatrixView{};
	std::vector<std::unique_ptr<CCamera>>	m_vCameras{};
	CCamera*								m_PtrCurrentCamera{};
	CCamera*								m_PtrSelectedCamera{};
	float									m_CameraMovementFactor{ 10.0f };
	std::unique_ptr<CObject3D>				m_Object3D_CameraRepresentation{};

private:
	XMVECTOR	m_PickingRayWorldSpaceOrigin{};
	XMVECTOR	m_PickingRayWorldSpaceDirection{};
	std::vector<SObject3DPickingCandiate>	m_vObject3DPickingCandidates{};
	CObject3D*	m_PtrPickedObject3D{};
	CObject3D*	m_PtrSelectedObject3D{};
	CObject2D*	m_PtrSelectedObject2D{};
	std::string	m_NullString{};
	int			m_PickedInstanceID{ -1 };
	int			m_SelectedInstanceID{ -1 };
	XMVECTOR	m_PickedTriangleV0{};
	XMVECTOR	m_PickedTriangleV1{};
	XMVECTOR	m_PickedTriangleV2{};
	EMode		m_eMode{};
	EEditMode	m_eEditMode{};

private:
	std::unique_ptr<CTerrain>	m_Terrain{};

private:
	ImFont*			m_EditorGUIFont{};
	SEditorGUIBools	m_EditorGUIBools{};

private:
	std::chrono::steady_clock	m_Clock{};
	long long					m_TimeNow{};
	long long					m_TimePrev{};
	long long					m_PreviousFrameTime{};
	long long					m_FPS{};
	long long					m_FrameCount{};
	float						m_DeltaTimeF{};

private:
	ERasterizerState	m_eRasterizerState{ ERasterizerState::CullCounterClockwise };
	EFlagsRendering		m_eFlagsRendering{};

private:
	ComPtr<IDXGISwapChain>			m_SwapChain{};
	ComPtr<ID3D11Device>			m_Device{};
	ComPtr<ID3D11DeviceContext>		m_DeviceContext{};
	ComPtr<ID3D11RenderTargetView>	m_RenderTargetView{};
	ComPtr<ID3D11DepthStencilView>	m_DepthStencilView{};
	ComPtr<ID3D11Texture2D>			m_DepthStencilBuffer{};
	ComPtr<ID3D11DepthStencilState>	m_DepthStencilStateLessEqualNoWrite{};
	ComPtr<ID3D11DepthStencilState>	m_DepthStencilStateAlways{};
	ComPtr<ID3D11BlendState>		m_BlendAlphaToCoverage{};

	std::unique_ptr<Keyboard>			m_Keyboard{};
	std::unique_ptr<Mouse>				m_Mouse{};
	Keyboard::State						m_CapturedKeyboardState{};
	Mouse::State						m_CapturedMouseState{};
	bool								m_bLeftButtonPressedOnce{ false };
	int									m_PrevCapturedMouseX{};
	int									m_PrevCapturedMouseY{};
	std::unique_ptr<SpriteBatch>		m_SpriteBatch{};
	std::unique_ptr<SpriteFont>			m_SpriteFont{};
	std::unique_ptr<CommonStates>		m_CommonStates{};
	bool								m_IsDestroyed{ false };
};

ENUM_CLASS_FLAG(CGame::EFlagsRendering)