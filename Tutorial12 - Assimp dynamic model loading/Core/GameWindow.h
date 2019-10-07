#pragma once

#define NOMINMAX 0

#include <Windows.h>
#include "Object3D.h"
#include "Shader.h"
#include "PrimitiveGenerator.h"
#include "GameObject.h"

enum class EFlagsGameRendering
{
	None		= 0x00,
	DrawNormals	= 0x01,
	UseLighting = 0x02,
};
ENUM_CLASS_FLAG(EFlagsGameRendering)

enum class ECameraType
{
	FirstPerson,
	ThirdPerson,
	FreeLook
};

enum class ECameraMovementDirection
{
	Forward,
	Backward,
	Rightward,
	Leftward,
};

enum class ERasterizerState
{
	CullNone,
	CullClockwise,
	CullCounterClockwise,
	WireFrame
};

struct SCameraData
{
	static constexpr float KDefaultDistance{ 10.0f };
	static constexpr float KDefaultMinDistance{ 1.0f };
	static constexpr float KDefaultMaxDistance{ 50.0f };

	SCameraData(ECameraType _CameraType) : CameraType{ _CameraType } {}
	SCameraData(ECameraType _CameraType, XMVECTOR _EyePosition, XMVECTOR _FocusPosition, XMVECTOR _UpDirection, 
		float _Distance = KDefaultDistance, float _MinDistance = KDefaultMinDistance, float _MaxDistance = KDefaultMaxDistance) :
		CameraType{ _CameraType }, EyePosition{ _EyePosition }, FocusPosition{ _FocusPosition }, UpDirection{ _UpDirection },
		Distance{ _Distance }, MinDistance{ _MinDistance }, MaxDistance{ _MaxDistance } {}

	ECameraType CameraType{};
	XMVECTOR EyePosition{ XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) };
	XMVECTOR FocusPosition{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
	XMVECTOR UpDirection{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

public:
	float Distance{ KDefaultDistance };
	float MinDistance{ KDefaultMinDistance };
	float MaxDistance{ KDefaultMaxDistance };

public:
	float Pitch{};
	float Yaw{};
	XMVECTOR Forward{};
};

struct SCBVSBaseSpaceData
{
	XMMATRIX WVP{};
	XMMATRIX World{};
};

struct SCBVSAnimationBonesData
{
	XMMATRIX BoneMatrices[KMaxBoneMatrixCount]{};
};

struct SCBPSBaseFlagsData
{
	BOOL bUseTexture{};
	BOOL bUseLighting{};
	BOOL Pad[2]{};
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

struct SCBPSBaseEyeData
{
	XMVECTOR EyePosition{};
};

class CGameWindow
{
	friend class CObject3D;

public:
	CGameWindow(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGameWindow() {}

public:
	void CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed);

	void SetPerspective(float FOV, float NearZ, float FarZ);

	void SetGameRenderingFlags(EFlagsGameRendering Flags);
	void ToggleGameRenderingFlags(EFlagsGameRendering Flags);

	void SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color);
	void SetAmbientlLight(const XMFLOAT3& Color, float Intensity);

	void AddCamera(const SCameraData& CameraData);
	void SetCamera(size_t Index);
	void MoveCamera(ECameraMovementDirection Direction, float StrideFactor = 1.0f);
	void RotateCamera(int DeltaX, int DeltaY, float RotationFactor = 1.0f);
	void ZoomCamera(int DeltaWheel, float ZoomFactor = 1.0f);

	CShader* AddShader();
	CShader* GetShader(size_t Index);

	CObject3D* AddObject3D();
	CObject3D* GetObject3D(size_t Index);

	CTexture* AddTexture();
	CTexture* GetTexture(size_t Index);

	CGameObject* AddGameObject();
	CGameObject* GetGameObject(size_t Index);

	void SetRasterizerState(ERasterizerState State);

	void BeginRendering(const FLOAT* ClearColor);
	void AnimateGameObjects();
	void DrawGameObjects();
	void DrawMiniAxes();
	void EndRendering();

public:
	HWND GetHWND() { return m_hWnd; }
	Keyboard::State GetKeyState();
	Mouse::State GetMouseState();
	SpriteBatch* GetSpriteBatchPtr() { return m_SpriteBatch.get(); }
	SpriteFont* GetSpriteFontPtr() { return m_SpriteFont.get(); }

private:
	void CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName);
	void InitializeDirectX(const wstring& FontFileName, bool bWindowed);
	void DrawGameObject(CGameObject* PtrGO);
	void DrawGameObjectNormals(CGameObject* PtrGO);

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();
	void CreateInputDevices();
	void CreateBaseShaders();
	void CreateCBs();
	void CreateMiniAxes();

private:
	void UpdateCBVSBaseSpace(const XMMATRIX& MatrixWorld);
	void UpdateCBVSAnimationBones(const XMMATRIX* PtrBoneMatrices);

	void UpdateCBPSBaseFlags();
	void UpdateCBPSBaseLights();
	void UpdateCBPSBaseEye();
	void UpdateCBPSBaseMaterial(const SMaterial& Material);

private:
	void CreateCB(size_t ByteWidth, ID3D11Buffer** ppBuffer);
	void UpdateCB(size_t ByteWidth, ID3D11Buffer* pBuffer, const void* pValue);

private:
	static constexpr float KDefaultFOV{ 60.0f / 360.0f * XM_2PI };
	static constexpr float KDefaultNearZ{ 0.1f };
	static constexpr float KDefaultFarZ{ 1000.0f };

private:
	vector<unique_ptr<CShader>>		m_vShaders{};
	vector<unique_ptr<CObject3D>>	m_vObject3Ds{};
	vector<unique_ptr<CTexture>>	m_vTextures{};
	vector<unique_ptr<CGameObject>>	m_vGameObjects{};

	vector<unique_ptr<CObject3D>>	m_vMiniAxisObject3Ds{};
	vector<unique_ptr<CGameObject>>	m_vMiniAxisGameObjects{};

	unique_ptr<CShader>				m_VSBase{};
	unique_ptr<CShader>				m_VSAnimation{};
	unique_ptr<CShader>				m_GSNormal{};
	unique_ptr<CShader>				m_PSBase{};
	unique_ptr<CShader>				m_PSNormal{};

	vector<D3D11_VIEWPORT>			m_vViewports{};

private:
	HWND							m_hWnd{};
	HINSTANCE						m_hInstance{};
	XMFLOAT2						m_WindowSize{};

private:
	XMMATRIX						m_MatrixProjection{};
	XMMATRIX						m_MatrixView{};
	vector<SCameraData>				m_vCameras{};
	SCameraData*					m_PtrCurrentCamera{};
	XMVECTOR						m_BaseForward{};
	XMVECTOR						m_BaseUp{};

private:
	ERasterizerState				m_eRasterizerState{ ERasterizerState::CullCounterClockwise };
	EFlagsGameRendering				m_eFlagsGamerendering{};

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

private:
	ComPtr<ID3D11Buffer>			m_cbVSBaseSpace{};
	ComPtr<ID3D11Buffer>			m_cbVSAnimationBones{};

	SCBVSBaseSpaceData				m_cbVSBaseSpaceData{};
	SCBVSAnimationBonesData			m_cbVSAnimationBonesData{};

private:
	ComPtr<ID3D11Buffer>			m_cbPSBaseFlags{};
	ComPtr<ID3D11Buffer>			m_cbPSBaseLights{};
	ComPtr<ID3D11Buffer>			m_cbPSBaseMaterial{};
	ComPtr<ID3D11Buffer>			m_cbPSBaseEye{};

	SCBPSBaseFlagsData				m_cbPSBaseFlagsData{};
	SCBPSBaseLightsData				m_cbPSBaseLightsData{};
	SCBPSBaseMaterialData			m_cbPSBaseMaterialData{};
	SCBPSBaseEyeData				m_cbPSBaseEyeData{};
};