#pragma once

#define NOMINMAX 0

#include <Windows.h>
#include "Camera.h"
#include "Object3D.h"
#include "Object2D.h"
#include "ObjectLine.h"
#include "Shader.h"
#include "PrimitiveGenerator.h"
#include "GameObject.h"
#include "GameObject2D.h"
#include "Math.h"

enum class EFlagsGameRendering
{
	None				= 0x00,
	DrawWireFrame		= 0x01,
	DrawNormals			= 0x02,
	UseLighting			= 0x04,
	DrawMiniAxes		= 0x08,
	DrawPickingData		= 0x10,
	DrawBoundingSphere	= 0x20
};
ENUM_CLASS_FLAG(EFlagsGameRendering)

enum class ERasterizerState
{
	CullNone,
	CullClockwise,
	CullCounterClockwise,
	WireFrame
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

class CGame
{
public:
	CGame(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGame() {}

public:
	void CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed);
	void Destroy();

// Advanced setting
public:
	void SetPerspective(float FOV, float NearZ, float FarZ);
	void SetGameRenderingFlags(EFlagsGameRendering Flags);
	void ToggleGameRenderingFlags(EFlagsGameRendering Flags);

public:
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition);
	void SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color);
	void SetAmbientlLight(const XMFLOAT3& Color, float Intensity);
	void SetGameObjectSky(CGameObject* GameObject);
	void SetGameObjectCloud(CGameObject* GameObject);
	void SetGameObjectSun(CGameObject* GameObject);
	void SetGameObjectMoon(CGameObject* GameObject);

// Object pool
public:
	CCamera* AddCamera(const SCameraData& CameraData);
	CCamera* GetCamera(size_t Index);

	CShader* AddShader();
	CShader* GetShader(size_t Index);

	CObject3D* AddObject3D();
	CObject3D* GetObject3D(size_t Index);

	CObject2D* AddObject2D();
	CObject2D* GetObject2D(size_t Index);

	CTexture* AddTexture();
	CTexture* GetTexture(size_t Index);

	CGameObject* AddGameObject(const string& Name);
	CGameObject* GetGameObject(const string& Name);
	CGameObject* GetGameObject(size_t Index);

	CGameObject2D* AddGameObject2D(const string& Name);
	CGameObject2D* GetGameObject2D(const string& Name);
	CGameObject2D* GetGameObject2D(size_t Index);

public:
	void Pick(int ScreenMousePositionX, int ScreenMousePositionY);
	const char* GetPickedGameObjectName();

public:
	void BeginRendering(const FLOAT* ClearColor);
	void AnimateGameObjects();
	void DrawGameObjects(float DeltaTime);
	void DrawGameObject2Ds(float DeltaTime);
	void EndRendering();

public:
	Keyboard::State GetKeyState();
	Mouse::State GetMouseState();
	SpriteBatch* GetSpriteBatchPtr() { return m_SpriteBatch.get(); }
	SpriteFont* GetSpriteFontPtr() { return m_SpriteFont.get(); }

private:
	void CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName);
	void InitializeDirectX(const wstring& FontFileName, bool bWindowed);

	void UpdateGameObject(CGameObject* PtrGO, float DeltaTime);
	void UpdatePickingRay();

	void DrawGameObject(CGameObject* PtrGO);
	void DrawGameObjectNormals(CGameObject* PtrGO);
	void DrawGameObjectBoundingSphere(CGameObject* PtrGO);
	void DrawMiniAxes();
	void DrawPickingRay();
	void DrawPickedTriangle();

	void SetUniversalRasterizerState();
	void SetUniversalbUseLighiting();

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();
	void CreateInputDevices();
	void CreateShaders();
	void CreateMiniAxes();
	void CreatePickingRay();
	void CreateBoundingSphere();
	void CreatePickedTriangle();

private:
	void PickBoundingSphere();
	void PickTriangle();

private:
	static constexpr float KDefaultFOV{ 50.0f / 360.0f * XM_2PI };
	static constexpr float KDefaultNearZ{ 0.1f };
	static constexpr float KDefaultFarZ{ 1000.0f };
	static constexpr float KSkyDistance{ 30.0f };
	static constexpr float KSkyTimeFactorAbsolute{ 0.1f };
	static constexpr float KPickingRayLength{ 1000.0f };

public:
	unique_ptr<CShader>				VSBase{};
	unique_ptr<CShader>				VSAnimation{};
	unique_ptr<CShader>				VSSky{};
	unique_ptr<CShader>				VSLine{};
	unique_ptr<CShader>				VSBase2D{};

	unique_ptr<CShader>				GSNormal{};

	unique_ptr<CShader>				PSBase{};
	unique_ptr<CShader>				PSVertexColor{};
	unique_ptr<CShader>				PSSky{};
	unique_ptr<CShader>				PSLine{};
	unique_ptr<CShader>				PSBase2D{};

public:
	SCBVSSpaceData					cbVSSpaceData{};
	SCBVSAnimationBonesData			cbVSAnimationBonesData{};
	SCBVS2DSpaceData				cbVS2DSpaceData{};

	SCBPSBaseFlagsData				cbPSBaseFlagsData{};
	SCBPSBaseLightsData				cbPSBaseLightsData{};
	SCBPSBaseMaterialData			cbPSBaseMaterialData{};
	SCBPSBaseEyeData				cbPSBaseEyeData{};
	SCBPSSkyTimeData				cbPSSkyTimeData{};
	SCBPS2DFlagsData				cbPS2DFlagsData{};

private:
	vector<unique_ptr<CShader>>			m_vShaders{};
	vector<unique_ptr<CObject3D>>		m_vObject3Ds{};
	vector<unique_ptr<CObject2D>>		m_vObject2Ds{};
	vector<unique_ptr<CTexture>>		m_vTextures{};
	vector<unique_ptr<CGameObject>>		m_vGameObjects{};
	vector<unique_ptr<CGameObject2D>>	m_vGameObject2Ds{};

	unique_ptr<CObjectLine>				m_ObjectLinePickingRay{};
	unique_ptr<CObject3D>				m_Object3DPickedTriangle{};

	unique_ptr<CObject3D>				m_Object3DBoundingSphere{};

	vector<unique_ptr<CObject3D>>		m_vMiniAxisObject3Ds{};
	vector<unique_ptr<CGameObject>>		m_vMiniAxisGameObjects{};

	CGameObject*						m_PtrSky{};
	CGameObject*						m_PtrCloud{};
	CGameObject*						m_PtrSun{};
	CGameObject*						m_PtrMoon{};

	unordered_map<string, size_t>		m_mapGameObjectNameToIndex{};
	unordered_map<string, size_t>		m_mapGameObject2DNameToIndex{};

private:
	vector<D3D11_VIEWPORT>			m_vViewports{};

private:
	HWND							m_hWnd{};
	HINSTANCE						m_hInstance{};
	XMFLOAT2						m_WindowSize{};

private:
	XMMATRIX						m_MatrixProjection{};
	XMMATRIX						m_MatrixProjection2D{};
	float							m_NearZ{};
	float							m_FarZ{};

	XMMATRIX						m_MatrixView{};
	vector<CCamera>					m_vCameras{};
	size_t							m_CurrentCameraIndex{};
	
private:
	XMVECTOR						m_PickingRayWorldSpaceOrigin{};
	XMVECTOR						m_PickingRayWorldSpaceDirection{};
	CGameObject*					m_PtrPickedGameObject{};
	XMVECTOR						m_PickedTriangleV0{};
	XMVECTOR						m_PickedTriangleV1{};
	XMVECTOR						m_PickedTriangleV2{};

private:
	ERasterizerState				m_eRasterizerState{ ERasterizerState::CullCounterClockwise };
	EFlagsGameRendering				m_eFlagsGameRendering{};

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