#pragma once

#define NOMINMAX 0

#include <Windows.h>
#include <chrono>

#include "Math.h"
#include "Camera.h"
#include "Shader.h"
#include "ConstantBuffer.h"
#include "Material.h"
#include "Object3D.h"
#include "Object3DLine.h"
#include "Object2D.h"
#include "ParticlePool.h"
#include "PrimitiveGenerator.h"
#include "Terrain.h"
#include "CubemapRep.h"
#include "ModelPorter.h"
#include "Billboard.h"
#include "Light.h"

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
		VSScreenQuad,
		VSBase2D,
		VSBillboard,
		VSLight,

		HSTerrain,
		HSWater,
		HSStatic,
		HSBillboard,
		HSPointLight,

		DSTerrain,
		DSWater,
		DSStatic,
		DSBillboard,
		DSPointLight,

		GSNormal,
		GSParticle,

		PSBase,
		PSBase_GBuffer,
		PSVertexColor,
		PSDynamicSky,
		PSCloud,
		PSLine,
		PSGizmo,
		PSTerrain,
		PSTerrain_GBuffer,
		PSWater,
		PSFoliage,
		PSParticle,
		PSCamera,
		PSScreenQuad,
		PSScreenQuad_Opaque,
		PSEdgeDetector,
		PSSky,
		PSIrradianceGenerator,
		PSFromHDR,
		PSRadiancePrefiltering,
		PSBRDFIntegrator,
		PSBillboard,
		PSDirectionalLight,
		PSPointLight,

		PSBase2D,
		PSMasking2D,
		PSHeightMap2D,
		PSCubemap2D
	};

	enum class EObjectType
	{
		NONE,

		Object3D,
		Object3DLine,
		Object2D,
		Camera,
		Light,

		Object3DInstance
	};

	struct SCBSpaceWVPData
	{
		XMMATRIX	World{};
		XMMATRIX	ViewProjection{};
		XMMATRIX	WVP{};
	};

	struct SCBSpaceVPData
	{
		XMMATRIX	View{};
		XMMATRIX	Projection{};
		XMMATRIX	ViewProjection{};
	};

	struct SCBSpace2DData
	{
		XMMATRIX	World{};
		XMMATRIX	Projection{};
	};

	struct SCBAnimationBonesData
	{
		XMMATRIX	BoneMatrices[CObject3D::KMaxBoneMatrixCount]{};
	};

	struct SCBPSFlagsData
	{
		BOOL		bUseTexture{};
		BOOL		bUseLighting{};
		BOOL		bUsePhysicallyBasedRendering{};
		uint32_t	EnvironmentTextureMipLevels{};

		uint32_t	PrefilteredRadianceTextureMipLevels{};
		float		Pads[3]{};
	};

	struct SCBLightData
	{
		XMVECTOR	DirectionalLightDirection{ XMVectorSet(0, 1, 0, 0) };
		XMFLOAT3	DirectionalLightColor{ 1, 1, 1 };
		float		Exposure{ 1.0 };
		XMFLOAT3	AmbientLightColor{ 1, 1, 1 };
		float		AmbientLightIntensity{ 0.5f };
		XMVECTOR	EyePosition{};
	};

	struct SCBDirectionalLightData
	{
		XMVECTOR	LightDirection{ XMVectorSet(0, 1, 0, 0) };
		XMFLOAT3	LightColor{ 1, 1, 1 };
		float		Exposure{ 1.0 };
		uint32_t	EnvironmentTextureMipLevels{};
		uint32_t	PrefilteredRadianceTextureMipLevels{};
		float		Reserved[2]{};
	};

	struct SCBMaterialData
	{
		XMFLOAT3	AmbientColor{};
		float		SpecularExponent{ 1 };
		XMFLOAT3	DiffuseColor{};
		float		SpecularIntensity{ 0 };
		XMFLOAT3	SpecularColor{};
		float		Roughness{};

		float		Metalness{};
		uint32_t	FlagsHasTexture{};
		uint32_t	FlagsIsTextureSRGB{};
		uint32_t	TotalMaterialCount{};
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
		float		Reserved[2]{};
	};

	struct SCBCameraSelectionData
	{
		uint32_t	SelectedCameraID{};
		uint32_t	CurrentCameraID{};
		float		Pads[2]{};
	};

	struct SCBBillboardSelectionData
	{
		BOOL		bUseBillboardSelection{ FALSE };
		uint32_t	SelectedBillboardID{};
		float		Pads[2]{};
	};

	struct SCBScreenData
	{
		XMFLOAT2	InverseScreenSize{};
		float		Pads[2]{};
	};

	struct SCBRadiancePrefiltering
	{
		float	Roughness{};
		float	RangeFactor{};
		float	Pads[2]{};
	};

	struct SCBIrradianceGenerator
	{
		float RangeFactor{};
		float Pads[3]{};
	};

	struct SCBGBufferUnpackingData
	{
		XMFLOAT4 PerspectiveValues;
		XMMATRIX InverseViewMatrix;
	};

	enum class EFlagsRendering
	{
		None = 0x0000,
		DrawWireFrame = 0x0001,
		DrawNormals = 0x0002,
		Use3DGizmos = 0x0004,
		DrawMiniAxes = 0x0008,
		DrawPickingData = 0x0010,
		DrawBoundingSphere = 0x0020,
		DrawTerrainHeightMapTexture = 0x0040,
		DrawTerrainMaskingTexture = 0x0080,
		DrawTerrainFoliagePlacingTexture = 0x0100,
		DrawGrid = 0x0200,
		TessellateTerrain = 0x0400, // 내부적으로만 사용하는 플래그!
		UseLighting = 0x0800,
		UsePhysicallyBasedRendering = 0x1000
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
		bool bShowPopupMaterialNameChanger{ false };
		bool bShowPopupMaterialTextureExplorer{ false };
	};

	struct SObject3DPickingCandiate
	{
		SObject3DPickingCandiate() {};
		SObject3DPickingCandiate(CObject3D* _PtrObject3D, XMVECTOR _T) : PtrObject3D{ _PtrObject3D }, T{ _T } {}
		SObject3DPickingCandiate(CObject3D* _PtrObject3D, int _InstanceID, XMVECTOR _T) : PtrObject3D{ _PtrObject3D }, InstanceID{ _InstanceID }, T{ _T } {}

		CObject3D* PtrObject3D{};
		int			InstanceID{ -1 };
		XMVECTOR	T{};
		bool		bHasFailedPickingTest{ false };
	};

	struct SScreenQuadVertex
	{
		SScreenQuadVertex(const XMFLOAT4& _Position, const XMFLOAT3& _TexCoord) : Position{ _Position }, TexCoord{ _TexCoord } {}
		XMFLOAT4 Position;
		XMFLOAT3 TexCoord;
	};

	struct SCubemapVertex
	{
		SCubemapVertex(const XMFLOAT4& _Position, const XMFLOAT3& _TexCoord) : Position{ _Position }, TexCoord{ _TexCoord } {}
		XMFLOAT4 Position;
		XMFLOAT3 TexCoord;
	};

	struct SRegionSelectionData
	{
		SRegionSelectionData() {}
		SRegionSelectionData(EObjectType _eObjectType, const std::string& _Name, void* _PtrObject) :
			eObjectType{ _eObjectType }, Name{ _Name }, PtrObject{ _PtrObject } {}

		EObjectType	eObjectType{};
		std::string	Name{};
		void*		PtrObject{};
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
	void InitializeImGui(const std::string& FontFileName, float FontSize);

private:
	void CreateSwapChain(bool bWindowed);
	void CreateViews();
	void InitializeViewports();
	void CreateDepthStencilStates();
	void CreateBlendStates();
	void CreateRasterizerStates();
	void CreateInputDevices();
	void CreateConstantBuffers();
	void CreateBaseShaders();
	void CreateMiniAxes();
	void CreatePickingRay();
	void CreatePickedTriangle();
	void CreateBoundingSphere();
	void Create3DGizmos();
	void CreateScreenQuadVertexBuffer();
	void CreateCubemapVertexBuffer();

public:
	void LoadScene(const std::string& FileName);
	void SaveScene(const std::string& FileName);

	// Advanced settings
public:
	void SetProjectionMatrices(float FOV, float NearZ, float FarZ);
	void SetRenderingFlags(EFlagsRendering Flags);
	void ToggleGameRenderingFlags(EFlagsRendering Flags);
	void Set3DGizmoMode(E3DGizmoMode Mode);
	void SetUniversalRSState();
	void SetUniversalbUseLighiting();
	E3DGizmoMode Get3DGizmoMode() const { return m_e3DGizmoMode; }
	CommonStates* GetCommonStates() const { return m_CommonStates.get(); }

	// Shader-related settings
public:
	void UpdateCBMaterialData(const CMaterialData& MaterialData, uint32_t TotalMaterialCount);

private:
	void UpdateCBSpace(const XMMATRIX& World = KMatrixIdentity);
	void UpdateCBAnimationBoneMatrices(const XMMATRIX* const BoneMatrices);
	void UpdateCBAnimationData(const CObject3D::SCBAnimationData& Data);
	void UpdateCBTerrainData(const CTerrain::SCBTerrainData& Data);
	void UpdateCBWindData(const CTerrain::SCBWindData& Data);

	void UpdateCBTessFactorData(const CObject3D::SCBTessFactorData& Data);
	void UpdateCBDisplacementData(const CObject3D::SCBDisplacementData& Data);

	void UpdateCBTerrainMaskingSpace(const XMMATRIX& Matrix);

	// This function also updates terrain's world matrix by calling UpdateCBSpace() from inside.
	void UpdateCBTerrainSelection(const CTerrain::SCBTerrainSelectionData& Selection);

	void UpdateCBBillboard(const CBillboard::SCBBillboardData& Data);
	void UpdateCBDirectionalLight();

public:
	void CreateDynamicSky(const std::string& SkyDataFileName, float ScalingFactor);
	void CreateStaticSky(float ScalingFactor);

private:
	void LoadSkyObjectData(const tinyxml2::XMLElement* const xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData);

public:
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMFLOAT3& Color);
	void SetDirectionalLightDirection(const XMVECTOR& LightSourcePosition);
	void SetDirectionalLightColor(const XMFLOAT3& Color);
	const XMVECTOR& GetDirectionalLightDirection() const;
	const XMFLOAT3& GetDirectionalLightColor() const;

	void SetAmbientlLight(const XMFLOAT3& Color, float Intensity);
	const XMFLOAT3& GetAmbientLightColor() const;
	float GetAmbientLightIntensity() const;

	void SetExposure(float Value);
	float GetExposure();

public:
	void CreateTerrain(const XMFLOAT2& TerrainSize, uint32_t MaskingDetail, float UniformScaling);
	void LoadTerrain(const std::string& TerrainFileName);
	void SaveTerrain(const std::string& TerrainFileName);
	CTerrain* GetTerrain() const { return m_Terrain.get(); }

// Object pool
public:
	void CopySelectedObject();
	void PasteCopiedObject();
	void DeleteSelectedObject();

public:
	bool InsertCamera(const std::string& Name);
	void DeleteCamera(const std::string& Name);
	void ClearCameras();
	CCamera* GetCamera(const std::string& Name, bool bShowWarning = true);
	const std::map<std::string, size_t>& GetCameraMap() const { return m_mapCameraNameToIndex; }

public:
	CShader* AddCustomShader();
	CShader* GetCustomShader(size_t Index) const;
	CShader* GetBaseShader(EBaseShader eShader) const;

	bool InsertObject3D(const std::string& Name);
	void DeleteObject3D(const std::string& Name);
	void ClearObject3Ds();
	CObject3D* GetObject3D(const std::string& Name, bool bShowWarning = true) const;
	const std::map<std::string, size_t>& GetObject3DMap() const { return m_mapObject3DNameToIndex; }

	bool InsertObject3DLine(const std::string& Name, bool bShowWarning = true);
	void ClearObject3DLines();
	CObject3DLine* GetObject3DLine(const std::string& Name, bool bShowWarning = true) const;

	bool InsertObject2D(const std::string& Name);
	void DeleteObject2D(const std::string& Name);
	void ClearObject2Ds();
	CObject2D* GetObject2D(const std::string& Name, bool bShowWarning = true) const;
	const std::map<std::string, size_t>& GetObject2DMap() const { return m_mapObject2DNameToIndex; }

	bool InsertMaterial(const std::string& Name, bool bShowWarning = true);
	bool InsertMaterialCreateTextures(const CMaterialData& MaterialData, bool bShowWarning = true);
	void DeleteMaterial(const std::string& Name);
	void CreateMaterialTextures(CMaterialData& MaterialData);
	void ClearMaterials();
	CMaterialData* GetMaterial(const std::string& Name, bool bShowWarning = true);
	CMaterialTextureSet* GetMaterialTextureSet(const std::string& Name, bool bShowWarning = true);
	size_t GetMaterialCount() const;
	bool ChangeMaterialName(const std::string& OldName, const std::string& NewName);
	const std::map<std::string, size_t>& GetMaterialMap() const { return m_mapMaterialNameToIndex; }
	ID3D11ShaderResourceView* GetMaterialTextureSRV(STextureData::EType eType, const std::string& Name) const;

public:
	bool InsertLight(CLight::EType eType, const std::string& Name);
	void DeleteLight(const std::string& Name);

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

private:
	void CastPickingRay();
	void UpdatePickingRay();
	void PickBoundingSphere();
	bool PickObject3DTriangle();

private:
	bool IsAnythingPicked() const;
	bool IsAnyObject3DPicked() const;
	bool IsAnyInstancePicked() const;
	bool IsAnyCameraPicked() const;
	bool IsAnyLightPicked() const;
	const std::string& GetPickedObject3DName() const;
	int GetPickedInstanceID() const;

	bool IsAnythingSelected() const;
	bool IsAnyObject3DSelected() const;
	bool IsAnyInstanceSelected() const;
	bool IsAnyObject2DSelected() const;
	bool IsAnyCameraSelected() const;
	bool IsAnyLightSelected() const;

	void SelectObject3D(const std::string& Name);
	void SelectPickedObject3D();
	void DeselectObject3D();
	CObject3D* GetSelectedObject3D();
	const std::string& GetSelectedObject3DName() const;

	void SelectObject2D(const std::string& Name);
	void DeselectObject2D();
	CObject2D* GetSelectedObject2D();
	const std::string& GetSelectedObject2DName() const;

	void SelectCamera(int CameraID);
	void SelectCamera(const std::string& Name);
	void SelectPickedCamera();
	void DeselectCamera();
	CCamera* GetSelectedCamera();
	const std::string& GetSelectedCameraName() const;
	CCamera* GetCurrentCamera();
	void UseEditorCamera();
	void UseCamera(CCamera* const Camera);

	void SelectLight(int LightID);
	void SelectLight(const std::string& Name);
	void SelectPickedLight();
	void DeselectLight();

	void DeselectAll();

public:
	void Select3DGizmos();
	void Deselect3DGizmos();
	bool IsGizmoHovered() const;
	bool IsGizmoSelected() const;
	bool ShouldSelectRotationGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis, XMVECTOR* const OutPtrT);
	bool ShouldSelectTranslationScalingGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis);

public:
	void SelectInstance(int InstanceID);
	void SelectPickedInstance();
	void DeselectInstance();
	int GetSelectedInstanceID() const;

private:
	void SelectTerrain(bool bShouldEdit, bool bIsLeftButton);

public:
	void CaptureCurrentProjectionSpace();

public:
	void BeginRendering(const FLOAT* ClearColor);
	void Update();
	void Draw();
	void EndRendering();

private:
	void SetForwardRenderTargets(bool bClearViews = false);
	void SetDeferredRenderTargets(bool bClearViews = false);
	void DrawFullScreenQuad(CShader* const PixelShader, ID3D11ShaderResourceView** const SRVs, UINT NumSRVs);

public:
	auto GethWnd() const->HWND { return m_hWnd; }
	auto GetDevicePtr() const->ID3D11Device* { return m_Device.Get(); }
	auto GetDeviceContextPtr() const->ID3D11DeviceContext* { return m_DeviceContext.Get(); }
	auto GetKeyState() const->Keyboard::State;
	auto GetMouseState() const->Mouse::State;
	auto GetSpriteBatchPtr() const->SpriteBatch* { return m_SpriteBatch.get(); }
	auto GetSpriteFontPtr() const->SpriteFont* { return m_SpriteFont.get(); }
	auto GetWindowSize() const->const XMFLOAT2&;
	auto GetDepthStencilStateLessEqualNoWrite() const->ID3D11DepthStencilState* { return m_DepthStencilStateLessEqualNoWrite.Get(); }
	auto GetBlendStateAlphaToCoverage() const->ID3D11BlendState* { return m_BlendAlphaToCoverage.Get(); }
	auto GetWorkingDirectory() const->const char* { return m_WorkingDirectory; }
	auto GetDeltaTime() const->float { return m_DeltaTimeF; }

private:
	void DrawObject3D(CObject3D* const PtrObject3D, bool bIgnoreInstances = false, bool bIgnoreOwnTexture = false);
	void DrawObject3DBoundingSphere(const CObject3D* const PtrObject3D);

	void DrawObject3DLines();

	void DrawObject2Ds();

	void DrawMiniAxes();

	void DrawPickingRay();
	void DrawPickedTriangle();

	void DrawGrid();

	void DrawSky(float DeltaTime);
	void DrawTerrain(float DeltaTime);

	void Draw3DGizmos();
	void Draw3DGizmoTranslations(E3DGizmoAxis Axis);
	void Draw3DGizmoRotations(E3DGizmoAxis Axis);
	void Draw3DGizmoScalings(E3DGizmoAxis Axis);
	void Draw3DGizmo(CObject3D* const Gizmo, bool bShouldHighlight);

	void DrawCameraRep();
	void DrawLightRep();
	void DrawRegionSelectionRep();

	void DrawEditorGUI();
	void DrawEditorGUIMenuBar();
	void DrawEditorGUIPopupTerrainGenerator();
	void DrawEditorGUIPopupObjectAdder();
	void DrawEditorGUIWindowPropertyEditor();

	// return true if any interaction is required
	bool DrawEditorGUIWindowPropertyEditor_MaterialData(CMaterialData& MaterialData, CMaterialTextureSet* const TextureSet,
		STextureData::EType& eSeletedTextureType, float ItemsOffsetX);
	void DrawEditorGUIPopupMaterialNameChanger(CMaterialData*& capturedMaterialData, bool bIsEditorMaterial);
	void DrawEditorGUIPopupMaterialTextureExplorer(CMaterialData* const capturedMaterialData, CMaterialTextureSet* const capturedMaterialTextureSet,
		STextureData::EType eSelectedTextureType);
	void DrawEditorGUIWindowSceneEditor();

private:
	void GenerateCubemapFromHDRi();
	void GenerateIrradianceMap(float RangeFactor);
	void GeneratePrefilteredRadianceMap(float RangeFactor);
	void GenerateIntegratedBRDFMap();

public:
	static constexpr float KTranslationMinLimit{ -1000.0f };
	static constexpr float KTranslationMaxLimit{ +1000.0f };
	static constexpr float KTranslationDelta{ +0.0078125f };
	static constexpr float KRotationMaxLimit{ +XM_2PI };
	static constexpr float KRotationMinLimit{ -XM_2PI };
	static constexpr float KRotationDelta{ +0.25f };
	static constexpr int KRotation360MaxLimit{ 360 };
	static constexpr int KRotation360MinLimit{ 360 };
	static constexpr int KRotation360Unit{ 1 };
	static constexpr float KRotation360To2PI{ 1.0f / 360.0f * XM_2PI };
	static constexpr float KRotation2PITo360{ 1.0f / XM_2PI * 360.0f };
	static constexpr float KScalingMaxLimit{ +100.0f };
	static constexpr float KScalingMinLimit{ +0.001f };
	static constexpr float KScalingDelta{ +0.0078125f };
	static constexpr float KBSCenterOffsetMinLimit{ -10.0f };
	static constexpr float KBSCenterOffsetMaxLimit{ +10.0f };
	static constexpr float KBSCenterOffsetDelta{ +0.01f };
	static constexpr float KBSRadiusMinLimit{ 0.001f };
	static constexpr float KBSRadiusMaxLimit{ 10.0f };
	static constexpr float KBSRadiusDelta{ +0.01f };
	static constexpr float KBSRadiusBiasMinLimit{ 0.001f };
	static constexpr float KBSRadiusBiasMaxLimit{ 1000.0f };
	static constexpr float KBSRadiusBiasDelta{ +0.01f };
	static constexpr float KCameraRepSelectionRadius{ 0.25f };
	static constexpr int KAssetNameMaxLength{ 100 };

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
	static constexpr float K3DGizmoRadius{ 0.05f };
	static constexpr float K3DGizmoSelectionRadius{ 1.1f };
	static constexpr float K3DGizmoSelectionLowBoundary{ 0.8f };
	static constexpr float K3DGizmoSelectionHighBoundary{ 1.2f };
	static constexpr float K3DGizmoMovementFactor{ 0.01f };
	static constexpr float K3DGizmoCameraDistanceThreshold{ 0.03125f };
	static constexpr float K3DGizmoDistanceFactorExponent{ 0.75f };
	static constexpr int KRotationGizmoRingSegmentCount{ 36 };
	static constexpr int KEnvironmentTextureSlot{ 50 };
	static constexpr int KIrradianceTextureSlot{ 51 };
	static constexpr int KPrefilteredRadianceTextureSlot{ 52 };
	static constexpr int KIntegratedBRDFTextureSlot{ 53 };
	static constexpr int KEditorCameraID{ -999 };
	static constexpr float KEditorCameraDefaultMovementFactor{ 3.0f };

	static constexpr char KTextureDialogFilter[45]{ "JPG 파일\0*.jpg\0PNG 파일\0*.png\0모든 파일\0*.*\0" };
	static constexpr char KTextureDialogTitle[16]{ "텍스쳐 불러오기" };

// Shader
private:
	std::vector<std::unique_ptr<CShader>>	m_vCustomShaders{};

	std::unique_ptr<CShader>	m_VSBase{};
	std::unique_ptr<CShader>	m_VSInstance{};
	std::unique_ptr<CShader>	m_VSAnimation{};
	std::unique_ptr<CShader>	m_VSSky{};
	std::unique_ptr<CShader>	m_VSLine{};
	std::unique_ptr<CShader>	m_VSGizmo{};
	std::unique_ptr<CShader>	m_VSTerrain{};
	std::unique_ptr<CShader>	m_VSFoliage{};
	std::unique_ptr<CShader>	m_VSParticle{};
	std::unique_ptr<CShader>	m_VSScreenQuad{};
	std::unique_ptr<CShader>	m_VSBase2D{};
	std::unique_ptr<CShader>	m_VSBillboard{};
	std::unique_ptr<CShader>	m_VSLight{};

	std::unique_ptr<CShader>	m_HSTerrain{};
	std::unique_ptr<CShader>	m_HSWater{};
	std::unique_ptr<CShader>	m_HSStatic{};
	std::unique_ptr<CShader>	m_HSBillboard{};
	std::unique_ptr<CShader>	m_HSPointLight{};

	std::unique_ptr<CShader>	m_DSTerrain{};
	std::unique_ptr<CShader>	m_DSWater{};
	std::unique_ptr<CShader>	m_DSStatic{};
	std::unique_ptr<CShader>	m_DSBillboard{};
	std::unique_ptr<CShader>	m_DSPointLight{};

	std::unique_ptr<CShader>	m_GSNormal{};
	std::unique_ptr<CShader>	m_GSParticle{};

	std::unique_ptr<CShader>	m_PSBase{};
	std::unique_ptr<CShader>	m_PSBase_gbuffer{};
	std::unique_ptr<CShader>	m_PSVertexColor{};
	std::unique_ptr<CShader>	m_PSDynamicSky{};
	std::unique_ptr<CShader>	m_PSCloud{};
	std::unique_ptr<CShader>	m_PSLine{};
	std::unique_ptr<CShader>	m_PSGizmo{};
	std::unique_ptr<CShader>	m_PSTerrain{};
	std::unique_ptr<CShader>	m_PSTerrain_gbuffer{};
	std::unique_ptr<CShader>	m_PSWater{};
	std::unique_ptr<CShader>	m_PSFoliage{};
	std::unique_ptr<CShader>	m_PSParticle{};
	std::unique_ptr<CShader>	m_PSCamera{};
	std::unique_ptr<CShader>	m_PSScreenQuad{};
	std::unique_ptr<CShader>	m_PSScreenQuad_opaque{};
	std::unique_ptr<CShader>	m_PSEdgeDetector{};
	std::unique_ptr<CShader>	m_PSSky{};
	std::unique_ptr<CShader>	m_PSIrradianceGenerator{};
	std::unique_ptr<CShader>	m_PSFromHDR{};
	std::unique_ptr<CShader>	m_PSRadiancePrefiltering{};
	std::unique_ptr<CShader>	m_PSBRDFIntegrator{};
	std::unique_ptr<CShader>	m_PSBillboard{};
	std::unique_ptr<CShader>	m_PSDirectionalLight{};
	std::unique_ptr<CShader>	m_PSPointLight{};

	std::unique_ptr<CShader>	m_PSBase2D{};
	std::unique_ptr<CShader>	m_PSMasking2D{};
	std::unique_ptr<CShader>	m_PSHeightMap2D{};
	std::unique_ptr<CShader>	m_PSCubemap2D{};

// Constant buffer
private:
	std::unique_ptr<CConstantBuffer>	m_CBSpaceWVP{};
	std::unique_ptr<CConstantBuffer>	m_CBSpaceVP{};
	std::unique_ptr<CConstantBuffer>	m_CBSpace2D{};
	std::unique_ptr<CConstantBuffer>	m_CBAnimationBones{};
	std::unique_ptr<CConstantBuffer>	m_CBAnimation{};
	std::unique_ptr<CConstantBuffer>	m_CBTerrain{};
	std::unique_ptr<CConstantBuffer>	m_CBWind{};
	std::unique_ptr<CConstantBuffer>	m_CBTessFactor{};
	std::unique_ptr<CConstantBuffer>	m_CBDisplacement{};
	std::unique_ptr<CConstantBuffer>	m_CBLight{};
	std::unique_ptr<CConstantBuffer>	m_CBDirectionalLight{};
	std::unique_ptr<CConstantBuffer>	m_CBMaterial{};
	std::unique_ptr<CConstantBuffer>	m_CBPSFlags{}; // ...
	std::unique_ptr<CConstantBuffer>	m_CBGizmoColorFactor{};
	std::unique_ptr<CConstantBuffer>	m_CBPS2DFlags{}; // ...
	std::unique_ptr<CConstantBuffer>	m_CBTerrainMaskingSpace{};
	std::unique_ptr<CConstantBuffer>	m_CBTerrainSelection{};
	std::unique_ptr<CConstantBuffer>	m_CBSkyTime{};
	std::unique_ptr<CConstantBuffer>	m_CBWaterTime{};
	std::unique_ptr<CConstantBuffer>	m_CBEditorTime{};
	std::unique_ptr<CConstantBuffer>	m_CBCameraSelection{};
	std::unique_ptr<CConstantBuffer>	m_CBScreen{};
	std::unique_ptr<CConstantBuffer>	m_CBRadiancePrefiltering{};
	std::unique_ptr<CConstantBuffer>	m_CBIrradianceGenerator{};
	std::unique_ptr<CConstantBuffer>	m_CBBillboard{};
	std::unique_ptr<CConstantBuffer>	m_CBBillboardSelection{};
	std::unique_ptr<CConstantBuffer>	m_CBGBufferUnpacking{};

	SCBSpaceWVPData						m_CBSpaceWVPData{};
	SCBSpaceVPData						m_CBSpaceVPData{};
	SCBSpace2DData						m_CBSpace2DData{};
	SCBAnimationBonesData				m_CBAnimationBonesData{};
	CObject3D::SCBAnimationData			m_CBAnimationData{};
	CTerrain::SCBTerrainData			m_CBTerrainData{};
	CTerrain::SCBWindData				m_CBWindData{};
	CObject3D::SCBTessFactorData		m_CBTessFactorData{};
	CObject3D::SCBDisplacementData		m_CBDisplacementData{};
	SCBLightData						m_CBLightData{};
	SCBDirectionalLightData				m_CBDirectionalLightData{};
	SCBMaterialData						m_CBMaterialData{};
	SCBPSFlagsData						m_CBPSFlagsData{};
	SCBGizmoColorFactorData				m_CBGizmoColorFactorData{};
	SCBPS2DFlagsData					m_CBPS2DFlagsData{};
	SCBTerrainMaskingSpaceData			m_CBTerrainMaskingSpaceData{};
	CTerrain::SCBTerrainSelectionData	m_CBTerrainSelectionData{};
	SCBSkyTimeData						m_CBSkyTimeData{};
	SCBWaterTimeData					m_CBWaterTimeData{};
	SCBEditorTimeData					m_CBEditorTimeData{};
	SCBCameraSelectionData				m_CBCameraSelectionData{};
	SCBScreenData						m_CBScreenData{};
	SCBRadiancePrefiltering				m_CBRadiancePrefilteringData{};
	SCBIrradianceGenerator				m_CBIrradianceGeneratorData{};
	CBillboard::SCBBillboardData		m_CBBillboardData{};
	SCBBillboardSelectionData			m_CBBillboardSelectionData{};
	SCBGBufferUnpackingData				m_CBGBufferUnpackingData{};

// Object pool
private:
	std::vector<std::unique_ptr<CObject3D>>		m_vObject3Ds{};
	std::vector<std::unique_ptr<CObject3DLine>>	m_vObject3DLines{};
	std::vector<std::unique_ptr<CObject2D>>		m_vObject2Ds{};

	std::vector<std::unique_ptr<CObject3D>>		m_vMiniAxes{};
	std::unique_ptr<CObject3DLine>				m_Grid{};

	std::unique_ptr<CObject3D>					m_BoundingSphereRep{};

	std::map<std::string, size_t>				m_mapObject3DNameToIndex{};
	std::map<std::string, size_t>				m_mapObject3DLineNameToIndex{};
	std::map<std::string, size_t>				m_mapObject2DNameToIndex{};
	std::map<std::string, size_t>				m_mapCameraNameToIndex{};
	size_t										m_PrimitiveCreationCounter{};

// Light
private:
	std::unique_ptr<CLight>			m_Light{};
	std::unique_ptr<CBillboard>		m_LightRep{};
	int								m_PickedLightID{ -1 };
	int								m_SelectedLightID{ -1 };

// Sky
private:
	std::string						m_SkyFileName{};
	float							m_SkyScalingFactor{};
	SSkyData						m_SkyData{};
	CMaterialData					m_SkyMaterialData{};
	std::unique_ptr<CObject3D>		m_Object3DSkySphere{};
	std::unique_ptr<CObject3D>		m_Object3DSun{};
	std::unique_ptr<CObject3D>		m_Object3DMoon{};
	std::unique_ptr<CObject3D>		m_Object3DCloud{};

// 3D Gizmo
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

	bool							m_bIsGizmoHovered{ false };
	bool							m_bIsGizmoSelected{ false };
	E3DGizmoAxis					m_e3DGizmoSelectedAxis{};
	E3DGizmoMode					m_e3DGizmoMode{};
	float							m_3DGizmoDistanceScalar{};
	XMVECTOR						m_CapturedGizmoTranslation{};
	XMVECTOR						m_GizmoRecentTranslation{};

// Camera
private:
	std::unique_ptr<CCamera>				m_EditorCamera{};
	std::vector<std::unique_ptr<CCamera>>	m_vCameras{};
	CCamera*								m_PtrCurrentCamera{};
	int										m_CurrentCameraID{ -1 };
	int										m_PickedCameraID{ -1 };
	int										m_SelectedCameraID{ -1 };
	std::unique_ptr<CObject3D>				m_CameraRep{};

// Picking
private:
	std::unique_ptr<CObject3DLine>			m_PickingRayRep{};
	std::unique_ptr<CObject3D>				m_PickedTriangleRep{};
	XMVECTOR								m_PickingRayWorldSpaceOrigin{};
	XMVECTOR								m_PickingRayWorldSpaceDirection{};
	std::vector<SObject3DPickingCandiate>	m_vObject3DPickingCandidates{};
	CObject3D*								m_PtrPickedObject3D{};
	CObject3D*								m_PtrSelectedObject3D{};
	CObject2D*								m_PtrSelectedObject2D{};
	int										m_PickedInstanceID{ -1 };
	int										m_SelectedInstanceID{ -1 };
	XMVECTOR								m_PickedTriangleV0{};
	XMVECTOR								m_PickedTriangleV1{};
	XMVECTOR								m_PickedTriangleV2{};

	// Region selection
	std::unique_ptr<CObject2D>				m_RegionSelectionRep{};
	XMFLOAT2								m_RegionSelectionTopLeft{};
	XMFLOAT2								m_RegionSelectionBottomRight{};
	bool									m_RegionSelectionChanging{ false };
	XMFLOAT2								m_RegionSelectionXYNormalized{};
	XMFLOAT2								m_RegionSelectionXYPrimeNormalized{};
	XMFLOAT2								m_RegionSelectionSizeNormalized{};
	std::vector<SRegionSelectionData>		m_vRegionSelectionData{};
	XMVECTOR								m_RegionSelectionWorldCenter{};

// Copy & paste
private:
	CObject3D*						m_PtrInstanceCopiedObject3D{};
	SInstanceCPUData				m_CopiedObject3DInstanceData{};
	size_t							m_CopyCounterObject3DInstance{};

	CLight::SLightInstanceCPUData	m_CopiedLightCPUData{};
	CLight::SLightInstanceGPUData	m_CopiedLightGPUData{};
	size_t							m_CopyCounterLight{};

	EObjectType						m_CopiedObjectType{};

// Terrain
private:
	std::unique_ptr<CTerrain>							m_Terrain{};
	std::vector<CMaterialData>							m_vMaterialData{};
	std::vector<std::unique_ptr<CMaterialTextureSet>>	m_vMaterialTextureSets{};
	std::map<std::string, size_t>						m_mapMaterialNameToIndex{};

// Time
private:
	std::chrono::steady_clock	m_Clock{};
	long long					m_TimeNow{};
	long long					m_TimePrev{};
	long long					m_PreviousFrameTime{};
	long long					m_FPS{};
	long long					m_FrameCount{};
	float						m_DeltaTimeF{};

// Editor
private:
	ERasterizerState			m_eRasterizerState{ ERasterizerState::CullCounterClockwise };
	EFlagsRendering				m_eFlagsRendering{};
	bool						m_bIsDeferredRenderTargetsSet{ false };
	bool						m_IsDestroyed{ false };
	CModelPorter				m_ModelPorter{};
	ImFont*						m_EditorGUIFont{};
	SEditorGUIBools				m_EditorGUIBools{};
	EMode						m_eMode{};
	EEditMode					m_eEditMode{};

// Full-screen quad
private:
	std::vector<SScreenQuadVertex>		m_vScreenQuadVertices
	{
		{ XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(0, 0, 0) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(1, 0, 0) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(0, 1, 0) },
		{ XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(1, 0, 0) },
		{ XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(1, 1, 0) },
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(0, 1, 0) },
	};
	ComPtr<ID3D11Buffer>				m_ScreenQuadVertexBuffer{};
	UINT								m_ScreenQuadVertexBufferStride{ sizeof(SScreenQuadVertex) };
	UINT								m_ScreenQuadVertexBufferOffset{};

// IBL
private:
	std::vector<SCubemapVertex>					m_vCubemapVertices{};
	ComPtr<ID3D11Buffer>						m_CubemapVertexBuffer{};
	UINT										m_CubemapVertexBufferStride{ sizeof(SCubemapVertex) };
	UINT										m_CubemapVertexBufferOffset{};

	std::unique_ptr<CTexture>					m_EnvironmentTexture{};
	std::unique_ptr<CTexture>					m_IrradianceTexture{};
	std::unique_ptr<CTexture>					m_PrefilteredRadianceTexture{};
	std::unique_ptr<CTexture>					m_IntegratedBRDFTexture{};

	std::unique_ptr<CCubemapRep>				m_EnvironmentRep{};
	std::unique_ptr<CCubemapRep>				m_IrradianceRep{};
	std::unique_ptr<CCubemapRep>				m_PrefilteredRadianceRep{};

	D3D11_TEXTURE2D_DESC						m_GeneratedIrradianceMapTextureDesc{};
	ComPtr<ID3D11Texture2D>						m_GeneratedIrradianceMapTexture{};
	ComPtr<ID3D11ShaderResourceView>			m_GeneratedIrradianceMapSRV{};
	std::vector<ComPtr<ID3D11RenderTargetView>>	m_vGeneratedIrradianceMapRTV{};

	D3D11_TEXTURE2D_DESC						m_GeneratedEnvironmentMapTextureDesc{};
	ComPtr<ID3D11Texture2D>						m_GeneratedEnvironmentMapTexture{};
	ComPtr<ID3D11ShaderResourceView>			m_GeneratedEnvironmentMapSRV{};
	std::vector<ComPtr<ID3D11RenderTargetView>>	m_vGeneratedEnvironmentMapRTV{};

	D3D11_TEXTURE2D_DESC						m_PrefilteredRadianceMapTextureDesc{};
	ComPtr<ID3D11Texture2D>						m_PrefilteredRadianceMapTexture{};
	ComPtr<ID3D11ShaderResourceView>			m_PrefilteredRadianceMapSRV{};
	std::vector<ComPtr<ID3D11RenderTargetView>>	m_vPrefilteredRadianceMapRTV{};

	ComPtr<ID3D11Texture2D>						m_IntegratedBRDFTextureRaw{};
	ComPtr<ID3D11RenderTargetView>				m_IntegratedBRDFRTV{};
	ComPtr<ID3D11ShaderResourceView>			m_IntegratedBRDFSRV{};
	D3D11_TEXTURE2D_DESC						m_IntegratedBRDFTextureDesc{};

// DirectX
private:
	ComPtr<IDXGISwapChain>				m_SwapChain{};
	ComPtr<ID3D11Device>				m_Device{};
	ComPtr<ID3D11DeviceContext>			m_DeviceContext{};

	ComPtr<ID3D11Texture2D>				m_BackBuffer{};
	ComPtr<ID3D11RenderTargetView>		m_BackBufferRTV{};

	ComPtr<ID3D11Texture2D>				m_ScreenQuadTexture{};
	ComPtr<ID3D11RenderTargetView>		m_ScreenQuadRTV{};
	ComPtr<ID3D11ShaderResourceView>	m_ScreenQuadSRV{};

	// GBuffer #0 - Depth 24 Stencil 8
	ComPtr<ID3D11Texture2D>				m_DepthStencilBuffer{};
	ComPtr<ID3D11DepthStencilView>		m_DepthStencilDSV{};
	ComPtr<ID3D11ShaderResourceView>	m_DepthStencilSRV{};

	// GBuffer #1 - Base color 24 & Roughness 8
	ComPtr<ID3D11Texture2D>				m_BaseColorRoughBuffer{};
	ComPtr<ID3D11RenderTargetView>		m_BaseColorRoughRTV{};
	ComPtr<ID3D11ShaderResourceView>	m_BaseColorRoughSRV{};

	// GBuffer #2 - Normal 32
	ComPtr<ID3D11Texture2D>				m_NormalBuffer{};
	ComPtr<ID3D11RenderTargetView>		m_NormalRTV{};
	ComPtr<ID3D11ShaderResourceView>	m_NormalSRV{};

	// GBuffer #3 - Metalness 8 & AmbientOcclusion 8
	ComPtr<ID3D11Texture2D>				m_MetalAOBuffer{};
	ComPtr<ID3D11RenderTargetView>		m_MetalAORTV{};
	ComPtr<ID3D11ShaderResourceView>	m_MetalAOSRV{};

	ComPtr<ID3D11DepthStencilState>		m_DepthStencilStateLessEqualNoWrite{};
	ComPtr<ID3D11DepthStencilState>		m_DepthStencilStateAlways{};

	ComPtr<ID3D11BlendState>			m_BlendAlphaToCoverage{};
	ComPtr<ID3D11BlendState>			m_BlendAdditiveLighting{};

	ComPtr<ID3D11RasterizerState>		m_RSCCWCullFront{};

	std::unique_ptr<SpriteBatch>		m_SpriteBatch{};
	std::unique_ptr<SpriteFont>			m_SpriteFont{};
	std::unique_ptr<CommonStates>		m_CommonStates{};

	std::vector<D3D11_VIEWPORT>			m_vViewports{};
	XMMATRIX							m_MatrixProjection{};
	XMMATRIX							m_MatrixProjection2D{};
	float								m_NearZ{};
	float								m_FarZ{};
	XMMATRIX							m_MatrixView{};

// Input
private:
	std::unique_ptr<Keyboard>		m_Keyboard{};
	std::unique_ptr<Mouse>			m_Mouse{};
	Keyboard::State					m_CapturedKeyboardState{};
	Mouse::State					m_CapturedMouseState{};
	bool							m_bLeftButtonPressedOnce{ false };
	bool							m_bLeftButtonUpOnce{ false };
	int								m_PrevCapturedMouseX{};
	int								m_PrevCapturedMouseY{};

// Windows
private:
	HWND			m_hWnd{};
	HINSTANCE		m_hInstance{};
	XMFLOAT2		m_WindowSize{};
	char			m_WorkingDirectory[MAX_PATH]{};
};

ENUM_CLASS_FLAG(CGame::EFlagsRendering)