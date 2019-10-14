#pragma once

#define NOMINMAX 0

#include <Windows.h>
#include "Math.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Object3D.h"
#include "Object3DLine.h"
#include "Object2D.h"
#include "PrimitiveGenerator.h"
#include "GameObject3D.h"
#include "GameObject3DLine.h"
#include "GameObject2D.h"
#include "TinyXml2/tinyxml2.h"

enum class EFlagsGameRendering
{
	None				= 0x00,
	DrawWireFrame		= 0x01,
	DrawNormals			= 0x02,
	UseLighting			= 0x04,
	DrawMiniAxes		= 0x08,
	DrawPickingData		= 0x10,
	DrawBoundingSphere	= 0x20,
	Use3DGizmos			= 0x40,
	UseTerrainSelector	= 0x80
};
ENUM_CLASS_FLAG(EFlagsGameRendering)

enum class EBaseShader
{
	VSAnimation,
	VSSky,
	VSLine,
	VSGizmo,
	VSBase2D,

	GSNormal,

	PSBase,
	PSVertexColor,
	PSSky,
	PSLine,
	PSGizmo,
	PSTerrainEdit,
	PSBase2D,
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

enum class ETerrainEditMode
{
	SetHeight,
	DeltaHeight,
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

enum class EFlagPSBase
{
	UseTexture,
	UseLighting
};

struct SCBPSBaseFlagsData
{
	BOOL		bUseTexture{};
	BOOL		bUseLighting{};
	BOOL		Pad[2]{};
};

struct SCBPSBaseLightsData
{
	XMVECTOR	DirectionalLightDirection{ XMVectorSet(0, 1, 0, 0) };
	XMVECTOR	DirectionalColor{ XMVectorSet(1, 1, 1, 1) };
	XMFLOAT3	AmbientLightColor{ 1, 1, 1 };
	float		AmbientLightIntensity{ 0.5f };
};

struct SCBPSBaseMaterialData
{
	XMFLOAT3	MaterialAmbient{};
	float		SpecularExponent{ 1 };
	XMFLOAT3	MaterialDiffuse{};
	float		SpecularIntensity{ 0 };
	XMFLOAT3	MaterialSpecular{};
	float		Pad{};
};

struct SCBPSSkyTimeData
{
	float		SkyTime{};
	float		Pads[3]{};
};

struct SCBPSBaseEyeData
{
	XMVECTOR	EyePosition{};
};

struct SCBPSGizmoColorFactorData
{
	XMVECTOR	ColorFactor{};
};

class CGame
{
public:
	CGame(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGame() {}

public:
	void CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed);
	void Destroy();

private:
	void CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName);
	void InitializeDirectX(const wstring& FontFileName, bool bWindowed);

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();
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
	void SetGameRenderingFlags(EFlagsGameRendering Flags);
	void ToggleGameRenderingFlags(EFlagsGameRendering Flags);
	void Set3DGizmoMode(E3DGizmoMode Mode);
	E3DGizmoMode Get3DGizmoMode() { return m_e3DGizmoMode; }

// Shader-related settings
public:
	void UpdatePSBaseFlagOn(EFlagPSBase Flag);
	void UpdatePSBaseFlagOff(EFlagPSBase Flag);
	void UpdateVSBaseMaterial(const SMaterial& Material);
	void UpdateVSAnimationBoneMatrices(const XMMATRIX* BoneMatrices);

public:
	void SetSky(const string& SkyDataFileName, float ScalingFactor);
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition);
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color);
	void SetAmbientlLight(const XMFLOAT3& Color, float Intensity);
	void SetTerrain(CGameObject3D* Terrain, const XMFLOAT2& TerrainSize);

private:
	void LoadSkyObjectData(tinyxml2::XMLElement* xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData);

// Object pool
public:
	CCamera* AddCamera(const SCameraData& CameraData);
	CCamera* GetCamera(size_t Index);

	CShader* AddShader();
	CShader* GetShader(size_t Index);
	CShader* GetBaseShader(EBaseShader eShader);

	CObject3D* AddObject3D();
	CObject3D* GetObject3D(size_t Index);

	CObject3DLine* AddObject3DLine();
	CObject3DLine* GetObject3DLine(size_t Index);

	CObject2D* AddObject2D();
	CObject2D* GetObject2D(size_t Index);

	CTexture* AddTexture(const string& Name);
	CTexture* GetTexture(const string& Name);
	const map<string, size_t>& GetTextureListMap() { return m_mapTextureNameToIndex; }

	CGameObject3D* AddGameObject3D(const string& Name);
	CGameObject3D* GetGameObject3D(const string& Name);
	CGameObject3D* GetGameObject3D(size_t Index);

	CGameObject3DLine* AddGameObject3DLine(const string& Name);
	CGameObject3DLine* GetGameObject3DLine(const string& Name);
	CGameObject3DLine* GetGameObject3DLine(size_t Index);

	CGameObject2D* AddGameObject2D(const string& Name);
	CGameObject2D* GetGameObject2D(const string& Name);
	CGameObject2D* GetGameObject2D(size_t Index);

public:
	void Pick();
	void ReleasePickedGameObject();

private:
	void CastPickingRay();
	void PickBoundingSphere();
	bool PickTriangle();

public:
	void SelectTerrain(bool bShouldEdit, bool bIsLeftButton);
	void SetTerrainEditMode(ETerrainEditMode Mode, float Value);
	void SetTerrainSelectionSize(float Size);

private:
	void UpdateTerrainSelection();
	void UpdateTerrainHeight(bool bIsLeftButton);
	void UpdateTerrainVertexPosition(XMVECTOR& Position, bool bIsLeftButton);

public:
	void BeginRendering(const FLOAT* ClearColor);
	void Animate();
	void Draw(float DeltaTime);
	void EndRendering();

public:
	HWND GethWnd() { return m_hWnd; }
	ID3D11Device* GetDevicePtr() { return m_Device.Get(); }
	ID3D11DeviceContext* GetDeviceContextPtr() { return m_DeviceContext.Get(); }
	Keyboard::State GetKeyState();
	Mouse::State GetMouseState();
	SpriteBatch* GetSpriteBatchPtr() { return m_SpriteBatch.get(); }
	SpriteFont* GetSpriteFontPtr() { return m_SpriteFont.get(); }
	const char* GetPickedGameObject3DName();
	const char* GetCapturedPickedGameObject3DName();
	float GetSkyTime();

private:
	void UpdateGameObject3D(CGameObject3D* PtrGO);
	void DrawGameObject3D(CGameObject3D* PtrGO);
	void DrawGameObject3DNormal(CGameObject3D* PtrGO);
	void DrawGameObject3DBoundingSphere(CGameObject3D* PtrGO);

	void DrawGameObject3DLines();

	void DrawGameObject2Ds();

	void DrawMiniAxes();

	void UpdatePickingRay();
	void DrawPickingRay();
	void DrawPickedTriangle();

	void DrawSky(float DeltaTime);

	bool ShouldSelectRotationGizmo(CGameObject3D* Gizmo, E3DGizmoAxis Axis);
	bool ShouldSelectTranslationScalingGizmo(CGameObject3D* Gizmo, E3DGizmoAxis Axis);
	void Draw3DGizmos();
	void Draw3DGizmoRotations(E3DGizmoAxis Axis);
	void Draw3DGizmoTranslations(E3DGizmoAxis Axis);
	void Draw3DGizmoScalings(E3DGizmoAxis Axis);
	void Draw3DGizmo(CGameObject3D* Gizmo, bool bShouldHighlight);

private:
	void SetUniversalRasterizerState();
	void SetUniversalbUseLighiting();

public:
	static constexpr float KTranslationMinLimit{ -1000.0f };
	static constexpr float KTranslationMaxLimit{ +1000.0f };
	static constexpr float KTranslationUnit{ +0.1f };
	static constexpr float KRotationMaxLimit{ +XM_2PI };
	static constexpr float KRotationMinLimit{ -XM_2PI };
	static constexpr float KRotation360MaxLimit{ +360.0f };
	static constexpr float KRotation360MinLimit{ -360.0f };
	static constexpr float KRotation360Unit{ 1.0f };
	static constexpr float KRotation360To2PI{ 1.0f / 360.0f * XM_2PI };
	static constexpr float KRotation2PITo360{ 1.0f / XM_2PI * 360.0f };
	static constexpr float KScalingMaxLimit{ +100.0f };
	static constexpr float KScalingMinLimit{ +0.01f };
	static constexpr float KScalingUnit{ +0.1f };
	static constexpr float KTerrainMaxHeight{ +10.0f };
	static constexpr float KTerrainMinHeight{ -10.0f };
	static constexpr float KTerrainHeightUnit{ 0.1f };
	static constexpr int KTerrainMinSize{ 2 };
	
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
	unique_ptr<CShader>	m_VSBase2D{};

	unique_ptr<CShader>	m_GSNormal{};

	unique_ptr<CShader>	m_PSBase{};
	unique_ptr<CShader>	m_PSVertexColor{};
	unique_ptr<CShader>	m_PSSky{};
	unique_ptr<CShader>	m_PSLine{};
	unique_ptr<CShader>	m_PSGizmo{};
	unique_ptr<CShader>	m_PSTerrainEdit{};
	unique_ptr<CShader>	m_PSBase2D{};

private:
	SCBVSSpaceData				m_cbVSSpaceData{};
	SCBVSAnimationBonesData		m_cbVSAnimationBonesData{};
	SCBVS2DSpaceData			m_cbVS2DSpaceData{};

	SCBPSBaseFlagsData			m_cbPSBaseFlagsData{};
	SCBPSBaseLightsData			m_cbPSBaseLightsData{};
	SCBPSBaseMaterialData		m_cbPSBaseMaterialData{};
	SCBPSBaseEyeData			m_cbPSBaseEyeData{};
	SCBPSGizmoColorFactorData	m_cbPSGizmoColorFactorData{};
	SCBPSSkyTimeData			m_cbPSSkyTimeData{};
	SCBPS2DFlagsData			m_cbPS2DFlagsData{};

private:
	vector<unique_ptr<CShader>>				m_vShaders{};
	vector<unique_ptr<CObject3D>>			m_vObject3Ds{};
	vector<unique_ptr<CObject3DLine>>		m_vObject3DLines{};
	vector<unique_ptr<CObject2D>>			m_vObject2Ds{};
	vector<unique_ptr<CTexture>>			m_vTextures{};
	vector<unique_ptr<CGameObject3D>>		m_vGameObject3Ds{};
	vector<unique_ptr<CGameObject3DLine>>	m_vGameObject3DLines{};
	vector<unique_ptr<CGameObject2D>>		m_vGameObject2Ds{};

	unique_ptr<CObject3DLine>				m_Object3DLinePickingRay{};
	unique_ptr<CObject3D>					m_Object3DPickedTriangle{};

	unique_ptr<CObject3D>					m_Object3DBoundingSphere{};

	vector<unique_ptr<CObject3D>>			m_vObject3DMiniAxes{};
	vector<unique_ptr<CGameObject3D>>		m_vGameObject3DMiniAxes{};

	SSkyData								m_SkyData{};
	unique_ptr<CTexture>					m_SkyTexture{};
	unique_ptr<CObject3D>					m_Object3DSkySphere{};
	unique_ptr<CObject3D>					m_Object3DSun{};
	unique_ptr<CObject3D>					m_Object3DMoon{};
	unique_ptr<CObject3D>					m_Object3DCloud{};
	unique_ptr<CGameObject3D>				m_GameObject3DSkySphere{};
	unique_ptr<CGameObject3D>				m_GameObject3DSun{};
	unique_ptr<CGameObject3D>				m_GameObject3DMoon{};
	unique_ptr<CGameObject3D>				m_GameObject3DCloud{};

	map<string, size_t>						m_mapTextureNameToIndex{};
	unordered_map<string, size_t>			m_umapGameObject3DNameToIndex{};
	unordered_map<string, size_t>			m_umapGameObject3DLineNameToIndex{};
	unordered_map<string, size_t>			m_umapGameObject2DNameToIndex{};

private:
	unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationPitch{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationYaw{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoRotationRoll{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoRotationPitch{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoRotationYaw{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoRotationRoll{};

	unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationX{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationY{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoTranslationZ{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoTranslationX{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoTranslationY{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoTranslationZ{};

	unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingX{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingY{};
	unique_ptr<CObject3D>		m_Object3D_3DGizmoScalingZ{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoScalingX{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoScalingY{};
	unique_ptr<CGameObject3D>	m_GameObject3D_3DGizmoScalingZ{};

	bool						m_bIsGizmoSelected{ false };
	E3DGizmoAxis				m_e3DGizmoSelectedAxis{};
	E3DGizmoMode				m_e3DGizmoMode{};
	float						m_3DGizmoDistanceScalar{};

private:
	vector<D3D11_VIEWPORT>	m_vViewports{};

private:
	HWND		m_hWnd{};
	HINSTANCE	m_hInstance{};
	XMFLOAT2	m_WindowSize{};

private:
	XMMATRIX		m_MatrixProjection{};
	XMMATRIX		m_MatrixProjection2D{};
	float			m_NearZ{};
	float			m_FarZ{};

	XMMATRIX		m_MatrixView{};
	vector<CCamera>	m_vCameras{};
	size_t			m_CurrentCameraIndex{};
	
private:
	XMVECTOR		m_PickingRayWorldSpaceOrigin{};
	XMVECTOR		m_PickingRayWorldSpaceDirection{};
	CGameObject3D*	m_PtrPickedGameObject3D{};
	CGameObject3D*	m_PtrCapturedPickedGameObject3D{};
	XMVECTOR		m_PickedTriangleV0{};
	XMVECTOR		m_PickedTriangleV1{};
	XMVECTOR		m_PickedTriangleV2{};

private:
	CGameObject3D*		m_PtrGameObject3DTerrain{};
	XMFLOAT2			m_TerrainSize{};
	ETerrainEditMode	m_eTerrainEditMode{};
	float				m_TerrainEditValue{};
	float				m_TerrainSelectionHalfSize{ 0.5f };
	XMFLOAT2			m_TerrainSelectionPosition{};

private:
	ERasterizerState	m_eRasterizerState{ ERasterizerState::CullCounterClockwise };
	EFlagsGameRendering	m_eFlagsGameRendering{};

private:
	ComPtr<IDXGISwapChain>			m_SwapChain{};
	ComPtr<ID3D11Device>			m_Device{};
	ComPtr<ID3D11DeviceContext>		m_DeviceContext{};
	ComPtr<ID3D11RenderTargetView>	m_RenderTargetView{};
	ComPtr<ID3D11DepthStencilView>	m_DepthStencilView{};
	ComPtr<ID3D11Texture2D>			m_DepthStencilBuffer{};

	unique_ptr<Keyboard>			m_Keyboard{};
	unique_ptr<Mouse>				m_Mouse{};
	unique_ptr<SpriteBatch>			m_SpriteBatch{};
	unique_ptr<SpriteFont>			m_SpriteFont{};
	unique_ptr<CommonStates>		m_CommonStates{};
};