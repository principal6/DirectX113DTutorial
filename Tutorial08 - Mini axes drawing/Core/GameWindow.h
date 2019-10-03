#pragma once

#include <Windows.h>
#include "Object3D.h"
#include "Texture.h"
#include "Shader.h"
#include "PrimitiveGenerator.h"
#include "GameObject.h"

enum class EFlagsGameRendering
{
	None		= 0x00,
	DrawNormals	= 0x01,
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

struct SCBVSDefaultSpaceData
{
	XMMATRIX WVP{};
	XMMATRIX World{};
};

struct SCBVSMiniAxisSpaceData
{
	XMMATRIX WVP{};
};

struct SCBPSDefaultTextureData
{
	BOOL bUseTexture{};
	BOOL Pad[3]{};
};

class CGameWindow
{
public:
	CGameWindow(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGameWindow() {}

public:
	void CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed);

	void SetPerspective(float FOV, float NearZ, float FarZ);
	void ToggleGameRenderingFlags(EFlagsGameRendering Flags);

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
	void DrawMiniAxis(CGameObject* PtrGO);

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();
	void CreateInputDevices();
	void CreateGSNormal();
	void CreateVSMiniAxis();
	void CreateCBs();
	void CreateMiniAxis();
	void UpdateCBVSDefaultSpace(const XMMATRIX& MatrixWorld);
	void UpdateCBVSMiniAxisSpace(const XMMATRIX& MatrixWorld);
	void UpdateCBPSDefaultTexture(BOOL UseTexture);

private:
	void CreateCB(ID3D11Buffer** PPBuffer, size_t ByteWidth);
	void UpdateCB(ID3D11Buffer* PPBuffer, void* PtrValue, size_t ValueByteSize);

private:
	static constexpr float KDefaultFOV{ 60.0f / 360.0f * XM_2PI };
	static constexpr float KDefaultNearZ{ 0.1f };
	static constexpr float KDefaultFarZ{ 1000.0f };

private:
	vector<unique_ptr<CShader>>		m_vShaders{};
	vector<unique_ptr<CObject3D>>	m_vObject3Ds{};
	vector<unique_ptr<CTexture>>	m_vTextures{};
	vector<unique_ptr<CGameObject>>	m_vGameObjects{};
	unique_ptr<CShader>				m_GSNormal{};
	unique_ptr<CShader>				m_VSMiniAxis{};

	unique_ptr<CObject3D>			m_Object3DAxisX{};
	unique_ptr<CObject3D>			m_Object3DAxisY{};
	unique_ptr<CObject3D>			m_Object3DAxisZ{};
	unique_ptr<CGameObject>			m_GOAxisX{};
	unique_ptr<CGameObject>			m_GOAxisY{};
	unique_ptr<CGameObject>			m_GOAxisZ{};

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
	vector<D3D11_VIEWPORT>			m_vViewPorts{};

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
	ComPtr<ID3D11Buffer>			m_CBVSDefaultSpace{};
	ComPtr<ID3D11Buffer>			m_CBVSMiniAxisSpace{};
	ComPtr<ID3D11Buffer>			m_CBPSDefaultTexture{};
	SCBVSDefaultSpaceData			m_CBVSDefaultSpaceData{};
	SCBVSMiniAxisSpaceData			m_CBVSMiniAxisSpaceData{};
	SCBPSDefaultTextureData			m_CBPSDefaultTextureData{};
};