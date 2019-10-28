#pragma once

#define NOMINMAX 0

#include <Windows.h>
#include "Math.h"
#include "Camera.h"
#include "Shader.h"
#include "Material.h"
#include "Object3D.h"
#include "Object3DLine.h"
#include "Object2D.h"
#include "PrimitiveGenerator.h"
#include "TinyXml2/tinyxml2.h"
#include "Terrain.h"

enum class EBaseShader
{
	VSBase,
	VSAnimation,
	VSSky,
	VSLine,
	VSGizmo,
	VSTerrain,
	VSBase2D,

	HSTerrain,
	HSWater,

	DSTerrain,
	DSWater,

	GSNormal,

	PSBase,
	PSVertexColor,
	PSSky,
	PSLine,
	PSGizmo,
	PSTerrain,
	PSWater,

	PSBase2D,
	PSMasking2D,
	PSHeightMap2D
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
	string			TextureFileName{};
	SSkyObjectData	Sun{};
	SSkyObjectData	Moon{};
	SSkyObjectData	Cloud{};
};

struct SCBVS2DSpaceData
{
	XMMATRIX	Projection{};
	XMMATRIX	World{};
};

struct SCBPS2DFlagsData
{
	BOOL		bUseTexture{};
	BOOL		Pad[3]{};
};

struct SCBVSSpaceData
{
	XMMATRIX	WVP{};
	XMMATRIX	World{};
};

struct SCBVSAnimationBonesData
{
	XMMATRIX	BoneMatrices[KMaxBoneMatrixCount]{};
};

struct SCBHSCameraData
{
	XMVECTOR	EyePosition{};
};

struct SCBHSTessFactorData
{
	float		TessFactor{};
	float		Pads[3]{};
};

struct SCBDSSpaceData
{
	XMMATRIX	VP{};
};

struct SCBDSDisplacementData
{
	BOOL		bUseDisplacement{};
	float		Pads[3]{};
};

struct SCBGSSpaceData
{
	XMMATRIX	VP{};
};

enum class EFlagPSBase
{
	UseTexture,
	UseLighting
};

enum class EFlagPSBase2D
{
	UseTexture
};

struct SCBPSBaseFlagsData
{
	BOOL		bUseTexture{};
	BOOL		bUseLighting{};
	BOOL		Pad[2]{};
};

struct SCBPSLightsData
{
	XMVECTOR	DirectionalLightDirection{ XMVectorSet(0, 1, 0, 0) };
	XMVECTOR	DirectionalLightColor{ XMVectorSet(1, 1, 1, 1) };
	XMFLOAT3	AmbientLightColor{ 1, 1, 1 };
	float		AmbientLightIntensity{ 0.5f };
	XMVECTOR	EyePosition{};
};

struct SCBPSBaseMaterialData
{
	XMFLOAT3	MaterialAmbient{};
	float		SpecularExponent{ 1 };
	XMFLOAT3	MaterialDiffuse{};
	float		SpecularIntensity{ 0 };
	XMFLOAT3	MaterialSpecular{};
	BOOL		bHasTexture{};
};

struct SCBPSSkyTimeData
{
	float		SkyTime{};
	float		Pads[3]{};
};

struct SCBPSGizmoColorFactorData
{
	XMVECTOR	ColorFactor{};
};

struct SCBPSTerrainSpaceData
{
	XMMATRIX	Matrix{};
};

struct SCBWaterTimeData
{
	float		Time{};
	float		Pads[3]{};
};

class CGame
{
public:
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
		DrawTerrainMaskingTexture = 0x200
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

	enum class EEditMode
	{
		EditObject,
		EditTerrain
	};

public:
	CGame(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGame() {}

public:
	void CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed);
	void Destroy();
	bool OpenFileDialog(const char* Filter, const char* Title);
	bool SaveFileDialog(const char* Filter, const char* Title, const char* DefaultExtension);
	const char* GetDialogFileNameWithPath() const;
	const char* GetDialogFileNameWithoutPath() const;

private:
	void CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName);
	void InitializeDirectX(const wstring& FontFileName, bool bWindowed);

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();
	void CreateDepthStencilState();
	void CreateInputDevices();
	void CreateBaseShaders();
	void CreateMiniAxes();
	void CreatePickingRay();
	void CreateBoundingSphere();
	void CreatePickedTriangle();
	void Create3DGizmos();

// Advanced settings
public:
	void SetPerspective(float FOV, float NearZ, float FarZ);
	void SetGameRenderingFlags(EFlagsRendering Flags);
	void ToggleGameRenderingFlags(EFlagsRendering Flags);
	void Set3DGizmoMode(E3DGizmoMode Mode);
	void SetUniversalRasterizerState();
	void SetUniversalbUseLighiting();
	E3DGizmoMode Get3DGizmoMode() { return m_e3DGizmoMode; }
	CommonStates* GetCommonStates() { return m_CommonStates.get(); }

// Shader-related settings
public:
	void UpdateVSSpace(const XMMATRIX& World);
	void UpdateVS2DSpace(const XMMATRIX& World);
	void UpdateVSAnimationBoneMatrices(const XMMATRIX* BoneMatrices);
	void UpdateVSTerrainData(const SCBVSTerrainData& Data);

	void UpdateDSDisplacementData(bool bUseDisplacement);

	void UpdateGSSpace();

	void UpdatePSBaseMaterial(const CMaterial& Material);
	void UpdatePSTerrainSpace(const XMMATRIX& Matrix);
	void UpdatePSBase2DFlagOn(EFlagPSBase2D Flag);
	void UpdatePSBase2DFlagOff(EFlagPSBase2D Flag);

public:
	void SetSky(const string& SkyDataFileName, float ScalingFactor);
	
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition);
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color);
	const XMVECTOR& GetDirectionalLightDirection() const;

	void SetAmbientlLight(const XMFLOAT3& Color, float Intensity);
	const XMFLOAT3& GetAmbientLightColor() const;
	float GetAmbientLightIntensity() const;

public:
	void CreateTerrain(const XMFLOAT2& TerrainSize, const CMaterial& Material, float MaskingDetail);
	void LoadTerrain(const string& TerrainFileName);
	void SaveTerrain(const string& TerrainFileName);
	void AddTerrainMaterial(const CMaterial& Material);
	void SetTerrainMaterial(int MaterialID, const CMaterial& Material);
	CTerrain* GetTerrain() { return m_Terrain.get(); }
	void SetTerrainSelectionSize(float& Size);

private:
	void LoadSkyObjectData(tinyxml2::XMLElement* xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData);

// Object pool
public:
	CCamera* AddCamera(const CCamera::SCameraData& CameraData);
	CCamera* GetCamera(size_t Index);

	CShader* AddShader();
	CShader* GetShader(size_t Index);
	CShader* GetBaseShader(EBaseShader eShader);

	void InsertObject3D(const string& Name);
	void EraseObject3D(const string& Name);
	CObject3D* GetObject3D(const string& Name);
	const map<string, size_t>& GetObject3DMap() { return m_mapObject3DNameToIndex; }

	void InsertObject3DLine(const string& Name);
	CObject3DLine* GetObject3DLine(const string& Name);

	void InsertObject2D(const string& Name);
	CObject2D* GetObject2D(const string& Name);

	CMaterial* AddMaterial(const CMaterial& Material);
	CMaterial* GetMaterial(const string& Name);
	void ClearMaterials();
	size_t GetMaterialCount();
	void ChangeMaterialName(const string& OldName, const string& NewName);
	void UpdateMaterial(const string& Name);
	const map<string, size_t>& GetMaterialMap() { return m_mapMaterialNameToIndex; }

	CMaterial::CTexture* AddMaterialDiffuseTexture(const string& Name);
	CMaterial::CTexture* GetMaterialDiffuseTexture(const string& Name);
	
	CMaterial::CTexture* AddMaterialNormalTexture(const string& Name);
	CMaterial::CTexture* GetMaterialNormalTexture(const string& Name);

public:
	void SetEditMode(EEditMode Mode);
	void Pick();
	void PickObject3D(const string& Name);
	void ReleaseCapturedObject3D();
	const char* GetPickedObject3DName() const;
	const char* GetCapturedObject3DName() const;
	bool IsObjectPicked() const { return m_PtrPickedObject3D; };
	bool IsObjectCaptured() const { return m_PtrCapturedPickedObject3D; }

private:
	void CastPickingRay();
	void PickBoundingSphere();
	bool PickTriangle();

public:
	void SelectTerrain(bool bShouldEdit, bool bIsLeftButton);
	void SetTerrainEditMode(CTerrain::EEditMode Mode);
	void SetTerrainMaskingLayer(CTerrain::EMaskingLayer eLayer);
	void SetTerrainMaskingAttenuation(float Attenuation);
	void SetTerrainMaskingSize(float Size);

public:
	void BeginRendering(const FLOAT* ClearColor);
	void Animate();
	void Draw(float DeltaTime);
	void EndRendering();

public:
	HWND GethWnd() const { return m_hWnd; }
	ID3D11Device* GetDevicePtr() { return m_Device.Get(); }
	ID3D11DeviceContext* GetDeviceContextPtr() { return m_DeviceContext.Get(); }
	Keyboard::State GetKeyState();
	Mouse::State GetMouseState();
	SpriteBatch* GetSpriteBatchPtr() { return m_SpriteBatch.get(); }
	SpriteFont* GetSpriteFontPtr() { return m_SpriteFont.get(); }
	const XMFLOAT2& GetWindowSize() const;
	const XMFLOAT2& GetTerrainSelectionPosition() const;
	float GetSkyTime() const;
	XMMATRIX GetTransposedVPMatrix() const;
	ID3D11DepthStencilState* GetDepthStencilStateLessEqualNoWrite() { return m_DepthStencilStateLessEqualNoWrite.Get(); }

private:
	void UpdateObject3D(CObject3D* PtrObject3D);
	void DrawObject3D(CObject3D* PtrObject3D);
	void DrawObject3DBoundingSphere(CObject3D* PtrObject3D);

	void DrawObject3DLines();

	void DrawObject2Ds();

	void DrawMiniAxes();

	void UpdatePickingRay();
	void DrawPickingRay();
	void DrawPickedTriangle();

	void DrawSky(float DeltaTime);
	void DrawTerrain();

	bool ShouldSelectRotationGizmo(CObject3D* Gizmo, E3DGizmoAxis Axis);
	bool ShouldSelectTranslationScalingGizmo(CObject3D* Gizmo, E3DGizmoAxis Axis);
	void Draw3DGizmos();
	void Draw3DGizmoRotations(E3DGizmoAxis Axis);
	void Draw3DGizmoTranslations(E3DGizmoAxis Axis);
	void Draw3DGizmoScalings(E3DGizmoAxis Axis);
	void Draw3DGizmo(CObject3D* Gizmo, bool bShouldHighlight);

public:
	static constexpr float KTranslationMinLimit{ -1000.0f };
	static constexpr float KTranslationMaxLimit{ +1000.0f };
	static constexpr float KTranslationUnit{ +0.1f };
	static constexpr float KRotationMaxLimit{ +XM_2PI };
	static constexpr float KRotationMinLimit{ -XM_2PI };
	static constexpr int KRotation360MaxLimit{ 360 };
	static constexpr int KRotation360MinLimit{ 360 };
	static constexpr int KRotation360Unit{ 1 };
	static constexpr float KRotation360To2PI{ 1.0f / 360.0f * XM_2PI };
	static constexpr float KRotation2PITo360{ 1.0f / XM_2PI * 360.0f };
	static constexpr float KScalingMaxLimit{ +100.0f };
	static constexpr float KScalingMinLimit{ +0.01f };
	static constexpr float KScalingUnit{ +0.01f };
	static constexpr int KObject3DNameMaxLength{ 100 };
	
private:
	static constexpr float KDefaultFOV{ 50.0f / 360.0f * XM_2PI };
	static constexpr float KDefaultNearZ{ 0.1f };
	static constexpr float KDefaultFarZ{ 1000.0f };
	static constexpr float KSkyDistance{ 30.0f };
	static constexpr float KSkyTimeFactorAbsolute{ 0.04f };
	static constexpr float KPickingRayLength{ 1000.0f };
	static constexpr uint32_t KSkySphereSegmentCount{ 32 };
	static constexpr XMVECTOR KColorWhite{ 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr XMVECTOR KSkySphereColorUp{ 0.1f, 0.5f, 1.0f, 1.0f };
	static constexpr XMVECTOR KSkySphereColorBottom{ 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr float K3DGizmoSelectionRadius{ 1.1f };
	static constexpr float K3DGizmoSelectionLowBoundary{ 0.8f };
	static constexpr float K3DGizmoSelectionHighBoundary{ 1.2f };
	static constexpr float K3DGizmoMovementFactor{ 0.01f };

private:
	unique_ptr<CShader>	m_VSBase{};
	unique_ptr<CShader>	m_VSAnimation{};
	unique_ptr<CShader>	m_VSSky{};
	unique_ptr<CShader>	m_VSLine{};
	unique_ptr<CShader>	m_VSGizmo{};
	unique_ptr<CShader>	m_VSTerrain{};

	unique_ptr<CShader>	m_VSBase2D{};

	unique_ptr<CShader>	m_HSTerrain{};
	unique_ptr<CShader>	m_HSWater{};
	
	unique_ptr<CShader>	m_DSTerrain{};
	unique_ptr<CShader>	m_DSWater{};

	unique_ptr<CShader>	m_GSNormal{};

	unique_ptr<CShader>	m_PSBase{};
	unique_ptr<CShader>	m_PSVertexColor{};
	unique_ptr<CShader>	m_PSSky{};
	unique_ptr<CShader>	m_PSLine{};
	unique_ptr<CShader>	m_PSGizmo{};
	unique_ptr<CShader>	m_PSTerrain{};
	unique_ptr<CShader>	m_PSWater{};

	unique_ptr<CShader>	m_PSBase2D{};
	unique_ptr<CShader>	m_PSMasking2D{};
	unique_ptr<CShader>	m_PSHeightMap2D{};

private:
	SCBVSSpaceData				m_cbVSSpaceData{};
	SCBVSAnimationBonesData		m_cbVSAnimationBonesData{};
	SCBVSTerrainData			m_cbVSTerrainData{};

	SCBVS2DSpaceData			m_cbVS2DSpaceData{};

	SCBHSCameraData				m_cbHSCameraData{};
	SCBHSTessFactorData			m_cbHSTessFactorData{};

	SCBDSSpaceData				m_cbDSSpaceData{};
	SCBDSDisplacementData		m_cbDSDisplacementData{};

	SCBGSSpaceData				m_cbGSSpaceData{};

	SCBPSLightsData				m_cbPSLightsData{};
	SCBPSBaseFlagsData			m_cbPSBaseFlagsData{};
	SCBPSBaseMaterialData		m_cbPSBaseMaterialData{};
	SCBPSGizmoColorFactorData	m_cbPSGizmoColorFactorData{};
	SCBPSSkyTimeData			m_cbPSSkyTimeData{};
	SCBWaterTimeData			m_cbWaterTimeData{};

	SCBPS2DFlagsData			m_cbPS2DFlagsData{};
	SCBPSTerrainSpaceData		m_cbPSTerrainSpaceData{};

private:
	vector<unique_ptr<CShader>>				m_vShaders{};
	vector<unique_ptr<CObject3D>>			m_vObject3Ds{};
	vector<unique_ptr<CObject3DLine>>		m_vObject3DLines{};
	vector<unique_ptr<CObject2D>>			m_vObject2Ds{};
	vector<unique_ptr<CMaterial>>			m_vMaterials{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vMaterialDiffuseTextures{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vMaterialNormalTextures{};
	vector<unique_ptr<CMaterial::CTexture>>	m_vMaterialDisplacementTextures{};

	unique_ptr<CObject3DLine>				m_Object3DLinePickingRay{};
	unique_ptr<CObject3D>					m_Object3DPickedTriangle{};

	unique_ptr<CObject3D>					m_Object3DBoundingSphere{};

	vector<unique_ptr<CObject3D>>			m_vObject3DMiniAxes{};

	SSkyData								m_SkyData{};
	CMaterial								m_SkyMaterial{};
	unique_ptr<CObject3D>					m_Object3DSkySphere{};
	unique_ptr<CObject3D>					m_Object3DSun{};
	unique_ptr<CObject3D>					m_Object3DMoon{};
	unique_ptr<CObject3D>					m_Object3DCloud{};

	map<string, size_t>						m_mapMaterialNameToIndex{};
	map<string, size_t>						m_mapObject3DNameToIndex{};
	unordered_map<string, size_t>			m_umapObject3DLineNameToIndex{};
	unordered_map<string, size_t>			m_umapObject2DNameToIndex{};

private:
	unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationPitch{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationYaw{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationRoll{};

	unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationX{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationY{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationZ{};

	unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingX{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingY{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingZ{};

	bool						m_bIsGizmoSelected{ false };
	E3DGizmoAxis				m_e3DGizmoSelectedAxis{};
	E3DGizmoMode				m_e3DGizmoMode{};
	float						m_3DGizmoDistanceScalar{};

private:
	vector<D3D11_VIEWPORT>	m_vViewports{};

private:
	HWND			m_hWnd{};
	HINSTANCE		m_hInstance{};
	XMFLOAT2		m_WindowSize{};
	char			m_WorkingDirectory[MAX_PATH]{};
	char			m_DialogFileName[MAX_PATH]{};
	string			m_DialogFileNameWithoutPath{};
	OPENFILENAME	m_OpenFileName{};

private:
	XMMATRIX		m_MatrixProjection{};
	XMMATRIX		m_MatrixProjection2D{};
	float			m_NearZ{};
	float			m_FarZ{};

	XMMATRIX		m_MatrixView{};
	vector<CCamera>	m_vCameras{};
	size_t			m_CurrentCameraIndex{};
	
private:
	XMVECTOR	m_PickingRayWorldSpaceOrigin{};
	XMVECTOR	m_PickingRayWorldSpaceDirection{};
	CObject3D*	m_PtrPickedObject3D{};
	CObject3D*	m_PtrCapturedPickedObject3D{};
	XMVECTOR	m_PickedTriangleV0{};
	XMVECTOR	m_PickedTriangleV1{};
	XMVECTOR	m_PickedTriangleV2{};
	EEditMode	m_eEditMode{};

private:
	unique_ptr<CTerrain>	m_Terrain{};

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

	unique_ptr<Keyboard>			m_Keyboard{};
	unique_ptr<Mouse>				m_Mouse{};
	unique_ptr<SpriteBatch>			m_SpriteBatch{};
	unique_ptr<SpriteFont>			m_SpriteFont{};
	unique_ptr<CommonStates>		m_CommonStates{};
};

ENUM_CLASS_FLAG(CGame::EFlagsRendering)