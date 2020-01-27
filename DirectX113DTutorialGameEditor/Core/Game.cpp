#include <thread>
#include <filesystem>

#include "Game.h"
#include "BinaryData.h"
#include "FileDialog.h"

using std::max;
using std::min;
using std::vector;
using std::string;
using std::wstring;
using std::thread;
using std::chrono::steady_clock;
using std::to_string;
using std::to_wstring;
using std::stof;
using std::make_unique;
using std::swap;

void CGame::CreateWin32(WNDPROC const WndProc, const std::string& WindowName, bool bWindowed)
{
	CreateWin32Window(WndProc, WindowName);

	InitializeDirectX(bWindowed);

	InitializeGameData();

	InitializeEditorAssets();

	InitializeImGui("Asset\\D2Coding.ttf", 15.0f);
}

void CGame::CreateSpriteFont(const wstring& FontFileName)
{
	if (!m_Device)
	{
		MB_WARN("아직 Device가 생성되지 않았습니다", "SpriteFont 생성 실패");
		return;
	}

	m_SpriteBatch = make_unique<SpriteBatch>(m_DeviceContext.Get());
	m_SpriteFont = make_unique<SpriteFont>(m_Device.Get(), FontFileName.c_str());
}

void CGame::Destroy()
{
	DestroyWindow(m_hWnd);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_IsDestroyed = true;
}

void CGame::CreateWin32Window(WNDPROC const WndProc, const std::string& WindowName)
{
	if (m_hWnd) return;

	constexpr LPCSTR KClassName{ "GameWindow" };
	constexpr DWORD KWindowStyle{ WS_CAPTION | WS_SYSMENU };

	WNDCLASSEXA WindowClass{};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WindowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	WindowClass.hIcon = WindowClass.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);
	WindowClass.hInstance = m_hInstance;
	WindowClass.lpfnWndProc = WndProc;
	WindowClass.lpszClassName = KClassName;
	WindowClass.lpszMenuName = nullptr;
	WindowClass.style = CS_VREDRAW | CS_HREDRAW;
	RegisterClassExA(&WindowClass);

	RECT WindowRect{};
	WindowRect.right = static_cast<LONG>(m_WindowSize.x);
	WindowRect.bottom = static_cast<LONG>(m_WindowSize.y);
	AdjustWindowRect(&WindowRect, KWindowStyle, false);

	assert(m_hWnd = CreateWindowExA(0, KClassName, WindowName.c_str(), KWindowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top,
		nullptr, nullptr, m_hInstance, nullptr));

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
}

void CGame::InitializeDirectX(bool bWindowed)
{
	_CreateSwapChain(bWindowed);

	_CreateViews();

	_CreateDepthStencilStates();
	_CreateBlendStates();

	_CreateInputDevices();

	_CreateConstantBuffers();
	_CreateBaseShaders();

	_CreateMiniAxes();
	_CreatePickingRay();
	_CreatePickedTriangle();

	SetProjectionMatrices(KDefaultFOV, KDefaultNearZ, KDefaultFarZ);
	_InitializeViewports();

	m_CommonStates = make_unique<CommonStates>(m_Device.Get());
}

void CGame::InitializeGameData()
{
	if (!m_Intelligence)
	{
		m_Intelligence = make_unique<CIntelligence>(m_Device.Get(), m_DeviceContext.Get());
		m_Intelligence->LinkPhysicsEngine(&m_PhysicsEngine);
	}

	if (!m_LightArray[0])
	{
		m_LightArray[0] = make_unique<CPointLight>(m_Device.Get(), m_DeviceContext.Get());
		m_LightArray[1] = make_unique<CSpotLight>(m_Device.Get(), m_DeviceContext.Get());
	}

	if (!m_CascadedShadowMap)
	{
		XMFLOAT2 Size{ 128, 128 };
		XMFLOAT2 PositionBase{ 0, m_WindowSize.y - Size.y };
		vector<CCascadedShadowMap::SLODData> vLODData
		{
			CCascadedShadowMap::SLODData(0,  8.0f, 1, XMFLOAT2(PositionBase.x + Size.x * 0.0f, PositionBase.y), Size),
			CCascadedShadowMap::SLODData(1, 16.0f, 2, XMFLOAT2(PositionBase.x + Size.x * 1.0f, PositionBase.y), Size),
			CCascadedShadowMap::SLODData(2, 24.0f, 8, XMFLOAT2(PositionBase.x + Size.x * 2.0f, PositionBase.y), Size),
			CCascadedShadowMap::SLODData(3, 50.0f, 32, XMFLOAT2(PositionBase.x + Size.x * 3.0f, PositionBase.y), Size)
		};

		m_CascadedShadowMap = make_unique<CCascadedShadowMap>(m_Device.Get(), m_DeviceContext.Get());
		m_CascadedShadowMap->Create(vLODData, XMFLOAT2(1024, 1024));
	}

	if (!m_DirectionalLightFSQ)
	{
		m_DirectionalLightFSQ = make_unique<CFullScreenQuad>(m_Device.Get(), m_DeviceContext.Get());
		m_DirectionalLightFSQ->Create2DDrawer(CFullScreenQuad::EPixelShaderPass::AllChannels);
		m_DirectionalLightFSQ->OverridePixelShader(m_PSDirectionalLight.get());
		//m_DirectionalLightFSQ->OverridePixelShader(m_PSDirectionalLight_NonIBL.get());
	}

	if (!m_SceneMaterial)
	{
		m_SceneMaterial = make_unique<CMaterialData>("SceneMaterial");
	}

	if (!m_SceneMaterialTextureSet)
	{
		m_SceneMaterialTextureSet = make_unique<CMaterialTextureSet>(m_Device.Get(), m_DeviceContext.Get());
	}
}

void CGame::InitializeEditorAssets()
{
	if (!m_Gizmo3D)
	{
		m_Gizmo3D = make_unique<CGizmo3D>(m_Device.Get(), m_DeviceContext.Get());
		m_Gizmo3D->Create();
	}

	if (!m_CameraRep)
	{
		m_CameraRep = make_unique<CObject3D>("CameraRepresentation", m_Device.Get(), m_DeviceContext.Get());
		m_CameraRep->CreateFromFile("Asset\\camera_repr.fbx", false);
	}

	if (!m_BoundingSphereRep)
	{
		m_BoundingSphereRep = make_unique<CObject3D>("BoundingSphere", m_Device.Get(), m_DeviceContext.Get());
		m_BoundingSphereRep->Create(GenerateSphere(16, XMVectorSet(1, 1, 0, 1)));
	}

	if (!m_AxisAlignedBoundingBoxRep)
	{
		m_AxisAlignedBoundingBoxRep = make_unique<CObject3D>("AxisAlignedBoundingBoxRep", m_Device.Get(), m_DeviceContext.Get());
		m_AxisAlignedBoundingBoxRep->Create(GenerateCube(XMVectorSet(1, 0, 1, 1)));
	}

	if (!m_AClosestPointRep)
	{
		m_AClosestPointRep = make_unique<CObject3D>("ClosestPoint", m_Device.Get(), m_DeviceContext.Get());
		m_AClosestPointRep->Create(GenerateSphere(16, XMVectorSet(1, 0, 0, 1)));
		m_AClosestPointRep->ScaleTo(XMVectorSet(0.1f, 0.1f, 0.1f, 0));
	}

	if (!m_BClosestPointRep)
	{
		m_BClosestPointRep = make_unique<CObject3D>("ClosestPoint", m_Device.Get(), m_DeviceContext.Get());
		m_BClosestPointRep->Create(GenerateSphere(16, XMVectorSet(0, 1, 0, 1)));
		m_BClosestPointRep->ScaleTo(XMVectorSet(0.1f, 0.1f, 0.1f, 0));
	}

	if (!m_PickedPointRep)
	{
		m_PickedPointRep = make_unique<CObject3D>("PickedPoint", m_Device.Get(), m_DeviceContext.Get());
		m_PickedPointRep->Create(GenerateSphere(16, XMVectorSet(0, 1, 1, 1)));
		m_PickedPointRep->ScaleTo(XMVectorSet(0.1f, 0.1f, 0.1f, 0));
	}

	if (!m_LightRep)
	{
		m_LightRep = make_unique<CBillboard>("LightRepresentation", m_Device.Get(), m_DeviceContext.Get());
		m_LightRep->CreateWorldSpace("Asset\\light.png", XMFLOAT2(0.5f, 0.5f), m_CBEditorTime.get());
	}

	if (!m_EditorCamera)
	{
		m_EditorCamera = make_unique<CCamera>("EditorCamera");
		m_EditorCamera->SetData(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0)));
		m_EditorCamera->TranslateTo(XMVectorSet(0, 2, 0, 1));
		m_EditorCamera->SetMovementFactor(KEditorCameraDefaultMovementFactor);
		
		UseEditorCamera();
	}

	if (!m_MultipleSelectionRep)
	{
		m_MultipleSelectionRep = make_unique<CObject2D>("RegionSelectionRep", m_Device.Get(), m_DeviceContext.Get());
		CObject2D::SModel2D Model2D{ Generate2DRectangle(XMFLOAT2(1, 1)) };
		for (auto& Vertex : Model2D.vVertices)
		{
			Vertex.Color = XMVectorSet(0, 0.375f, 1.0f, 0.375f);
		}
		m_MultipleSelectionRep->Create(Model2D);
	}

	{
		if (!m_IBLBaker)
		{
			m_IBLBaker = make_unique<CIBLBaker>(m_Device.Get(), m_DeviceContext.Get());
			m_IBLBaker->Create();
		}

		if (!m_EnvironmentTexture)
		{
			// @important: use already mipmapped cubemap texture
			m_EnvironmentTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
			m_EnvironmentTexture->CreateCubeMapFromFile("Asset\\uffizi_environment.dds");
			m_EnvironmentTexture->SetSlot(KEnvironmentTextureSlot);
		}

		if (!m_IrradianceTexture)
		{
			// @important: use already mipmapped cubemap texture
			m_IrradianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
			m_IrradianceTexture->CreateCubeMapFromFile("Asset\\uffizi_irradiance.dds");
			m_IrradianceTexture->SetSlot(KIrradianceTextureSlot);
		}

		if (!m_PrefilteredRadianceTexture)
		{
			// @important: use already mipmapped cubemap texture
			m_PrefilteredRadianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
			m_PrefilteredRadianceTexture->CreateCubeMapFromFile("Asset\\uffizi_prefiltered_radiance.dds");
			m_PrefilteredRadianceTexture->SetSlot(KPrefilteredRadianceTextureSlot);
		}

		if (!m_IntegratedBRDFTexture)
		{
			// @important: this is not cubemap nor mipmapped!
			m_IntegratedBRDFTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
			m_IntegratedBRDFTexture->CreateTextureFromFile("Asset\\integrated_brdf.dds", false);
			m_IntegratedBRDFTexture->SetSlot(KIntegratedBRDFTextureSlot);
		}

		if (!m_EnvironmentCubemapRep)
		{
			m_EnvironmentCubemapRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get());
			m_EnvironmentCubemapRep->Create();
		}

		if (!m_IrradianceCubemapRep)
		{
			m_IrradianceCubemapRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get());
			m_IrradianceCubemapRep->Create();
		}

		if (!m_PrefilteredRadianceCubemapRep)
		{
			m_PrefilteredRadianceCubemapRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get());
			m_PrefilteredRadianceCubemapRep->Create();
		}

		UpdateCBGlobalLightProbeData();
	}
	
	if (!m_Grid)
	{
		m_Grid = make_unique<CObject3DLine>("Grid", m_Device.Get(), m_DeviceContext.Get());
		m_Grid->Create(Generate3DGrid(0));
	}

	if (!m_EdgeDetectorFSQ)
	{
		m_EdgeDetectorFSQ = make_unique<CFullScreenQuad>(m_Device.Get(), m_DeviceContext.Get());
		m_EdgeDetectorFSQ->Create2DDrawer(CFullScreenQuad::EPixelShaderPass::OpaqueSRV);
		m_EdgeDetectorFSQ->OverridePixelShader(m_PSEdgeDetector.get());
	}

	if (!m_TerrainMaterialDefault)
	{
		m_TerrainMaterialDefault = make_unique<CMaterialData>("BrownMudDry");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::DiffuseTexture, "Asset\\brown_mud_dry_base_color.jpg");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::NormalTexture, "Asset\\brown_mud_dry_normal.jpg");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::SpecularIntensityTexture, "Asset\\brown_mud_dry_specular.jpg");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::RoughnessTexture, "Asset\\brown_mud_dry_roughness.jpg");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::MetalnessTexture, "Asset\\brown_mud_dry_specular.jpg");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::AmbientOcclusionTexture, "Asset\\brown_mud_dry_occlusion.jpg");
		m_TerrainMaterialDefault->SetTextureFileName(ETextureType::DisplacementTexture, "Asset\\brown_mud_dry_displacement.jpg");
	}
}

void CGame::InitializeImGui(const std::string& FontFileName, float FontSize)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_hWnd);
	ImGui_ImplDX11_Init(m_Device.Get(), m_DeviceContext.Get());

	ImGuiIO& igIO{ ImGui::GetIO() };
	igIO.Fonts->AddFontDefault();
	
	m_EditorGUIFont = igIO.Fonts->AddFontFromFileTTF(FontFileName.c_str(), FontSize, nullptr, igIO.Fonts->GetGlyphRangesKorean());
}

void CGame::_CreateSwapChain(bool bWindowed)
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc{};
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.Width = static_cast<UINT>(m_WindowSize.x);
	SwapChainDesc.BufferDesc.Height = static_cast<UINT>(m_WindowSize.y);
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.Flags = 0;
	SwapChainDesc.OutputWindow = m_hWnd;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Windowed = bWindowed;

	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
		&SwapChainDesc, m_SwapChain.ReleaseAndGetAddressOf(), m_Device.ReleaseAndGetAddressOf(), nullptr, m_DeviceContext.ReleaseAndGetAddressOf());
}

void CGame::_CreateViews()
{
	// Create depth-stencil DSV + GBuffer #0
	{
		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		Texture2DDesc.ArraySize = 1;
		Texture2DDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // @important
		Texture2DDesc.Width = static_cast<UINT>(m_WindowSize.x);
		Texture2DDesc.Height = static_cast<UINT>(m_WindowSize.y);
		Texture2DDesc.MipLevels = 0;
		Texture2DDesc.MiscFlags = 0;
		Texture2DDesc.SampleDesc.Count = 1;
		Texture2DDesc.SampleDesc.Quality = 0;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_GBuffers.DepthStencilBuffer.ReleaseAndGetAddressOf());

		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
		DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		m_Device->CreateDepthStencilView(m_GBuffers.DepthStencilBuffer.Get(), &DSVDesc, m_GBuffers.DepthStencilDSV.ReleaseAndGetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_Device->CreateShaderResourceView(m_GBuffers.DepthStencilBuffer.Get(), &SRVDesc, m_GBuffers.DepthStencilSRV.ReleaseAndGetAddressOf());
	}

	// Create GBuffer #1 - Base color 24 & Roguhness 8
	{
		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		Texture2DDesc.ArraySize = 1;
		Texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		Texture2DDesc.Width = static_cast<UINT>(m_WindowSize.x);
		Texture2DDesc.Height = static_cast<UINT>(m_WindowSize.y);
		Texture2DDesc.MipLevels = 0;
		Texture2DDesc.MiscFlags = 0;
		Texture2DDesc.SampleDesc.Count = 1;
		Texture2DDesc.SampleDesc.Quality = 0;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_GBuffers.BaseColorRoughBuffer.ReleaseAndGetAddressOf());
		m_Device->CreateRenderTargetView(m_GBuffers.BaseColorRoughBuffer.Get(), nullptr, m_GBuffers.BaseColorRoughRTV.ReleaseAndGetAddressOf());
		m_Device->CreateShaderResourceView(m_GBuffers.BaseColorRoughBuffer.Get(), nullptr, m_GBuffers.BaseColorRoughSRV.ReleaseAndGetAddressOf());
	}

	// Create GBuffer #2 - Normal 32
	{
		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		Texture2DDesc.ArraySize = 1;
		Texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		Texture2DDesc.Width = static_cast<UINT>(m_WindowSize.x);
		Texture2DDesc.Height = static_cast<UINT>(m_WindowSize.y);
		Texture2DDesc.MipLevels = 0;
		Texture2DDesc.MiscFlags = 0;
		Texture2DDesc.SampleDesc.Count = 1;
		Texture2DDesc.SampleDesc.Quality = 0;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_GBuffers.NormalBuffer.ReleaseAndGetAddressOf());
		m_Device->CreateRenderTargetView(m_GBuffers.NormalBuffer.Get(), nullptr, m_GBuffers.NormalRTV.ReleaseAndGetAddressOf());
		m_Device->CreateShaderResourceView(m_GBuffers.NormalBuffer.Get(), nullptr, m_GBuffers.NormalSRV.ReleaseAndGetAddressOf());
	}

	// Create GBuffer #3 - Metalness 8 & Ambient occlusion 8
	{
		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		Texture2DDesc.ArraySize = 1;
		Texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		Texture2DDesc.Width = static_cast<UINT>(m_WindowSize.x);
		Texture2DDesc.Height = static_cast<UINT>(m_WindowSize.y);
		Texture2DDesc.MipLevels = 0;
		Texture2DDesc.MiscFlags = 0;
		Texture2DDesc.SampleDesc.Count = 1;
		Texture2DDesc.SampleDesc.Quality = 0;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_GBuffers.MetalAOBuffer.ReleaseAndGetAddressOf());
		m_Device->CreateRenderTargetView(m_GBuffers.MetalAOBuffer.Get(), nullptr, m_GBuffers.MetalAORTV.ReleaseAndGetAddressOf());
		m_Device->CreateShaderResourceView(m_GBuffers.MetalAOBuffer.Get(), nullptr, m_GBuffers.MetalAOSRV.ReleaseAndGetAddressOf());
	}

	// Create back-buffer RTV
	m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_BackBuffer.ReleaseAndGetAddressOf());
	m_Device->CreateRenderTargetView(m_BackBuffer.Get(), nullptr, m_BackBufferRTV.ReleaseAndGetAddressOf());

	// Create EdgeDetector RTV, SRV
	{
		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		Texture2DDesc.ArraySize = 1;
		Texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		Texture2DDesc.Height = static_cast<UINT>(m_WindowSize.y);
		Texture2DDesc.MipLevels = 1;
		Texture2DDesc.SampleDesc.Count = 1;
		Texture2DDesc.SampleDesc.Quality = 0;
		Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
		Texture2DDesc.Width = static_cast<UINT>(m_WindowSize.x);
		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_EdgeDetectorTexture.ReleaseAndGetAddressOf());

		D3D11_RENDER_TARGET_VIEW_DESC ScreenQuadRTVDesc{};
		ScreenQuadRTVDesc.Format = Texture2DDesc.Format;
		ScreenQuadRTVDesc.Texture2D.MipSlice = 0;
		ScreenQuadRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		m_Device->CreateRenderTargetView(m_EdgeDetectorTexture.Get(), &ScreenQuadRTVDesc, m_EdgeDetectorRTV.ReleaseAndGetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC ScreenQuadSRVDesc{};
		ScreenQuadSRVDesc.Format = Texture2DDesc.Format;
		ScreenQuadSRVDesc.Texture2D.MipLevels = 1;
		ScreenQuadSRVDesc.Texture2D.MostDetailedMip = 0;
		ScreenQuadSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_Device->CreateShaderResourceView(m_EdgeDetectorTexture.Get(), &ScreenQuadSRVDesc, m_EdgeDetectorSRV.ReleaseAndGetAddressOf());
	}
}

void CGame::_InitializeViewports()
{
	// #0
	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = m_WindowSize.x;
		Viewport.Height = m_WindowSize.y;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	// #1
	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 20.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	// #2
	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = m_WindowSize.y * 7.0f / 8.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	// #3
	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = m_WindowSize.x * 1.0f / 8.0f;
		Viewport.TopLeftY = m_WindowSize.y * 7.0f / 8.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	// #4
	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = m_WindowSize.x * 2.0f / 8.0f;
		Viewport.TopLeftY = m_WindowSize.y * 7.0f / 8.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}
}

void CGame::_CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{};
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.StencilEnable = FALSE;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateLessEqualNoWrite.ReleaseAndGetAddressOf())));

	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateGreaterEqual.ReleaseAndGetAddressOf())));

	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateAlways.ReleaseAndGetAddressOf())));
}

void CGame::_CreateBlendStates()
{
	{
		D3D11_BLEND_DESC BlendDesc{};
		BlendDesc.AlphaToCoverageEnable = TRUE;
		BlendDesc.IndependentBlendEnable = FALSE;
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		BlendDesc.RenderTarget[0].BlendOp = BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_Device->CreateBlendState(&BlendDesc, m_BlendAlphaToCoverage.ReleaseAndGetAddressOf());
	}
	
	{
		D3D11_BLEND_DESC BlendDesc{};
		BlendDesc.AlphaToCoverageEnable = FALSE;
		BlendDesc.IndependentBlendEnable = FALSE;
		BlendDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;
		BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		BlendDesc.RenderTarget[0].BlendOp = BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_Device->CreateBlendState(&BlendDesc, m_BlendAdditiveLighting.ReleaseAndGetAddressOf());
	}
}

void CGame::_CreateRasterizerStates()
{
	D3D11_RASTERIZER_DESC RasterizerDesc{};
	RasterizerDesc.AntialiasedLineEnable = FALSE;
	RasterizerDesc.CullMode = D3D11_CULL_FRONT;
	RasterizerDesc.DepthBias = 0;
	RasterizerDesc.DepthBiasClamp = 0.0f;
	RasterizerDesc.DepthClipEnable = FALSE;
	RasterizerDesc.FillMode = D3D11_FILL_SOLID;
	RasterizerDesc.FrontCounterClockwise = FALSE;
	RasterizerDesc.MultisampleEnable = FALSE;
	RasterizerDesc.ScissorEnable = FALSE;
	RasterizerDesc.SlopeScaledDepthBias = 0.0f;
	m_Device->CreateRasterizerState(&RasterizerDesc, m_RSCCWCullFront.ReleaseAndGetAddressOf());
}

void CGame::_CreateInputDevices()
{
	m_Keyboard = make_unique<Keyboard>();

	m_Mouse = make_unique<Mouse>();
	m_Mouse->SetWindow(m_hWnd);
	m_Mouse->SetMode(Mouse::Mode::MODE_ABSOLUTE);
}

void CGame::_CreateConstantBuffers()
{
	m_CBSpace = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBSpaceData, sizeof(m_CBSpaceData));
	m_CBAnimationBones = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBAnimationBonesData, sizeof(m_CBAnimationBonesData));
	m_CBAnimation = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBAnimationData, sizeof(m_CBAnimationData));
	m_CBTerrain = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBTerrainData, sizeof(m_CBTerrainData));
	m_CBWind = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBWindData, sizeof(m_CBWindData));
	m_CBTessFactor = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBTessFactorData, sizeof(m_CBTessFactorData));
	m_CBDisplacement = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBDisplacementData, sizeof(m_CBDisplacementData));
	m_CBGlobalLight = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBGlobalLightData, sizeof(m_CBGlobalLightData));
	m_CBTerrainMaskingSpace = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBTerrainMaskingSpaceData, sizeof(m_CBTerrainMaskingSpaceData));
	m_CBTerrainSelection = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBTerrainSelectionData, sizeof(m_CBTerrainSelectionData));
	m_CBSkyTime = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBSkyTimeData, sizeof(m_CBSkyTimeData));
	m_CBWaterTime = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBWaterTimeData, sizeof(m_CBWaterTimeData));
	m_CBEditorTime = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBEditorTimeData, sizeof(m_CBEditorTimeData));
	m_CBCameraInfo = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBCameraInfoData, sizeof(m_CBCameraInfoData));
	m_CBGBufferUnpacking = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBGBufferUnpackingData, sizeof(m_CBGBufferUnpackingData));
	m_CBShadowMap = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBShadowMapData, sizeof(m_CBShadowMapData));
	m_CBSceneMaterial = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBSceneMaterialData, sizeof(m_CBSceneMaterialData));

	m_CBSpace->Create();
	m_CBAnimationBones->Create();
	m_CBAnimation->Create();
	m_CBTerrain->Create();
	m_CBWind->Create();
	m_CBTessFactor->Create();
	m_CBDisplacement->Create();
	m_CBGlobalLight->Create();
	m_CBTerrainMaskingSpace->Create();
	m_CBTerrainSelection->Create();
	m_CBSkyTime->Create();
	m_CBWaterTime->Create();
	m_CBEditorTime->Create();
	m_CBCameraInfo->Create();
	m_CBGBufferUnpacking->Create();
	m_CBShadowMap->Create();
	m_CBSceneMaterial->Create();
}

void CGame::_CreateBaseShaders()
{
	bool bShouldCompileShaders{ false };

	// VS
	{
		m_CBSpace->Use(EShaderType::VertexShader, 0);

		m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSAnimation->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSAnimation.hlsl", "main",
			CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
		m_VSAnimation->ReserveConstantBufferSlots(KVSSharedCBCount);
		m_VSAnimation->AttachConstantBuffer(m_CBAnimationBones.get());
		m_VSAnimation->AttachConstantBuffer(m_CBAnimation.get());

		m_VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSBase->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSBase.hlsl", "main",
			CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
		m_VSBase->ReserveConstantBufferSlots(KVSSharedCBCount);

		m_VSBase_Instanced = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSBase_Instanced->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSBase.hlsl", "Instanced",
			CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
		m_VSBase_Instanced->ReserveConstantBufferSlots(KVSSharedCBCount);

		m_VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSBase2D->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSBase2D.hlsl", "main",
			CObject2D::KInputLayout, ARRAYSIZE(CObject2D::KInputLayout));
		m_VSBase2D->ReserveConstantBufferSlots(KVSSharedCBCount);

		m_VSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSFoliage->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSFoliage.hlsl", "main",
			CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
		m_VSFoliage->ReserveConstantBufferSlots(KVSSharedCBCount);
		m_VSFoliage->AttachConstantBuffer(m_CBTerrain.get());
		m_VSFoliage->AttachConstantBuffer(m_CBWind.get());

		m_VSLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSLight->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSLight.hlsl", "main",
			CLight::KInputElementDescs, ARRAYSIZE(CLight::KInputElementDescs));
		m_VSLight->ReserveConstantBufferSlots(KVSSharedCBCount);

		m_VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSLine->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSLine.hlsl", "main",
			CObject3DLine::KInputElementDescs, ARRAYSIZE(CObject3DLine::KInputElementDescs));
		m_VSLine->ReserveConstantBufferSlots(KVSSharedCBCount);

		m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSSky->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSSky.hlsl", "main",
			CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
		m_VSSky->ReserveConstantBufferSlots(KVSSharedCBCount);

		m_VSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_VSTerrain->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSTerrain.hlsl", "main",
			CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
		m_VSTerrain->ReserveConstantBufferSlots(KVSSharedCBCount);
		m_VSTerrain->AttachConstantBuffer(m_CBTerrain.get());
	}

	// HS
	{
		m_CBSpace->Use(EShaderType::HullShader, 0);
		m_CBGlobalLight->Use(EShaderType::HullShader, 1);

		m_HSPointLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_HSPointLight->Create(EShaderType::HullShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\HSPointLight.hlsl", "main");
		m_HSPointLight->ReserveConstantBufferSlots(KHSSharedCBCount);

		m_HSSpotLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_HSSpotLight->Create(EShaderType::HullShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\HSSpotLight.hlsl", "main");
		m_HSSpotLight->ReserveConstantBufferSlots(KHSSharedCBCount);

		m_HSStatic = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_HSStatic->Create(EShaderType::HullShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\HSStatic.hlsl", "main");
		m_HSStatic->ReserveConstantBufferSlots(KHSSharedCBCount);
		m_HSStatic->AttachConstantBuffer(m_CBTessFactor.get());

		m_HSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_HSTerrain->Create(EShaderType::HullShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\HSTerrain.hlsl", "main");
		m_HSTerrain->ReserveConstantBufferSlots(KHSSharedCBCount);
		m_HSTerrain->AttachConstantBuffer(m_CBTessFactor.get());

		m_HSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_HSWater->Create(EShaderType::HullShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\HSWater.hlsl", "main");
		m_HSWater->ReserveConstantBufferSlots(KHSSharedCBCount);
		m_HSWater->AttachConstantBuffer(m_CBTessFactor.get());
	}

	// DS
	{
		m_CBSpace->Use(EShaderType::DomainShader, 0);

		m_DSPointLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_DSPointLight->Create(EShaderType::DomainShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\DSPointLight.hlsl", "main");
		m_DSPointLight->ReserveConstantBufferSlots(KDSSharedCBCount);

		m_DSSpotLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_DSSpotLight->Create(EShaderType::DomainShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\DSSpotLight.hlsl", "main");
		m_DSSpotLight->ReserveConstantBufferSlots(KDSSharedCBCount);

		m_DSStatic = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_DSStatic->Create(EShaderType::DomainShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\DSStatic.hlsl", "main");
		m_DSStatic->ReserveConstantBufferSlots(KDSSharedCBCount);
		m_DSStatic->AttachConstantBuffer(m_CBDisplacement.get());

		m_DSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_DSTerrain->Create(EShaderType::DomainShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\DSTerrain.hlsl", "main");
		m_DSTerrain->ReserveConstantBufferSlots(KDSSharedCBCount);
		m_DSTerrain->AttachConstantBuffer(m_CBDisplacement.get());

		m_DSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_DSWater->Create(EShaderType::DomainShader, CShader::EVersion::_5_0, bShouldCompileShaders, L"Shader\\DSWater.hlsl", "main");
		m_DSWater->ReserveConstantBufferSlots(KDSSharedCBCount);
		m_DSWater->AttachConstantBuffer(m_CBWaterTime.get());
	}

	// GS
	{
		m_CBSpace->Use(EShaderType::GeometryShader, 0);

		m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_GSNormal->Create(EShaderType::GeometryShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\GSNormal.hlsl", "main");
		m_GSNormal->ReserveConstantBufferSlots(KGSSharedCBCount);
	}
	
	// PS
	{
		// @important: Slot 0 is CBMaterial in CObject3D
		m_CBGlobalLight->Use(EShaderType::PixelShader, 1);
		m_CBSpace->Use(EShaderType::PixelShader, 2);

		m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase.hlsl", "main");
		m_PSBase->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSBase->AttachConstantBuffer(m_CBSceneMaterial.get());

		m_PSBase_GBuffer = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase_GBuffer->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase.hlsl", "GBuffer");
		m_PSBase_GBuffer->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSBase_GBuffer->AttachConstantBuffer(m_CBSceneMaterial.get());

		m_PSBase_RawVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase_RawVertexColor->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase.hlsl", "RawVertexColor");
		m_PSBase_RawVertexColor->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSBase_RawDiffuseColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase_RawDiffuseColor->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase.hlsl", "RawDiffuseColor");
		m_PSBase_RawDiffuseColor->ReserveConstantBufferSlots(KPSSharedCBCount);
		
		m_PSBase_Void = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase_Void->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase.hlsl", "Void");
		m_PSBase_Void->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase2D->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase2D.hlsl", "main");
		m_PSBase2D->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSBase2D_RawVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSBase2D_RawVertexColor->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSBase2D.hlsl", "RawVertexColor");
		m_PSBase2D_RawVertexColor->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSCamera = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSCamera->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSCamera.hlsl", "main");
		m_PSCamera->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSCamera->AttachConstantBuffer(m_CBEditorTime.get());
		m_PSCamera->AttachConstantBuffer(m_CBCameraInfo.get());

		m_PSCloud = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSCloud->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSCloud.hlsl", "main");
		m_PSCloud->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSCloud->AttachConstantBuffer(m_CBSkyTime.get());

		m_PSDirectionalLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSDirectionalLight->Create(EShaderType::PixelShader, CShader::EVersion::_4_1, bShouldCompileShaders, L"Shader\\PSDirectionalLight.hlsl", "main");
		m_PSDirectionalLight->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSDirectionalLight->AttachConstantBuffer(m_CBGBufferUnpacking.get());
		m_PSDirectionalLight->AttachConstantBuffer(m_CBShadowMap.get());

		m_PSDirectionalLight_NonIBL = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSDirectionalLight_NonIBL->Create(EShaderType::PixelShader, CShader::EVersion::_4_1, bShouldCompileShaders, L"Shader\\PSDirectionalLight.hlsl", "NonIBL");
		m_PSDirectionalLight_NonIBL->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSDirectionalLight_NonIBL->AttachConstantBuffer(m_CBGBufferUnpacking.get());
		m_PSDirectionalLight_NonIBL->AttachConstantBuffer(m_CBShadowMap.get());

		m_PSDynamicSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSDynamicSky->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSDynamicSky.hlsl", "main");
		m_PSDynamicSky->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSDynamicSky->AttachConstantBuffer(m_CBSkyTime.get());

		m_PSEdgeDetector = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSEdgeDetector->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSEdgeDetector.hlsl", "main");
		m_PSEdgeDetector->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSFoliage->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSFoliage.hlsl", "main");
		m_PSFoliage->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSHeightMap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSHeightMap2D->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSHeightMap2D.hlsl", "main");
		m_PSHeightMap2D->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSLine->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSLine.hlsl", "main");
		m_PSLine->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSMasking2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSMasking2D->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSMasking2D.hlsl", "main");
		m_PSMasking2D->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSPointLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSPointLight->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSPointLight.hlsl", "main");
		m_PSPointLight->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSPointLight->AttachConstantBuffer(m_CBGBufferUnpacking.get());

		m_PSPointLight_Volume = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSPointLight_Volume->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSPointLight.hlsl", "Volume");
		m_PSPointLight_Volume->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSPointLight_Volume->AttachConstantBuffer(m_CBGBufferUnpacking.get());

		m_PSSpotLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSSpotLight->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSSpotLight.hlsl", "main");
		m_PSSpotLight->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSSpotLight->AttachConstantBuffer(m_CBGBufferUnpacking.get());

		m_PSSpotLight_Volume = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSSpotLight_Volume->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSSpotLight.hlsl", "Volume");
		m_PSSpotLight_Volume->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSSpotLight_Volume->AttachConstantBuffer(m_CBGBufferUnpacking.get());

		m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSSky->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSSky.hlsl", "main");
		m_PSSky->ReserveConstantBufferSlots(KPSSharedCBCount);

		m_PSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSTerrain->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSTerrain.hlsl", "main");
		m_PSTerrain->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSTerrain->AttachConstantBuffer(m_CBTerrainMaskingSpace.get());
		m_PSTerrain->AttachConstantBuffer(m_CBTerrainSelection.get());
		m_PSTerrain->AttachConstantBuffer(m_CBEditorTime.get());

		m_PSTerrain_gbuffer = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSTerrain_gbuffer->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSTerrain.hlsl", "gbuffer");
		m_PSTerrain_gbuffer->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBTerrainMaskingSpace.get());
		m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBTerrainSelection.get());
		m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBEditorTime.get());

		m_PSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
		m_PSWater->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSWater.hlsl", "main");
		m_PSWater->ReserveConstantBufferSlots(KPSSharedCBCount);
		m_PSWater->AttachConstantBuffer(m_CBWaterTime.get());
	}
}

void CGame::_CreateMiniAxes()
{
	m_vMiniAxes.clear();
	m_vMiniAxes.emplace_back(make_unique<CObject3D>("AxisX", m_Device.Get(), m_DeviceContext.Get()));
	m_vMiniAxes.emplace_back(make_unique<CObject3D>("AxisY", m_Device.Get(), m_DeviceContext.Get()));
	m_vMiniAxes.emplace_back(make_unique<CObject3D>("AxisZ", m_Device.Get(), m_DeviceContext.Get()));

	const SMesh KAxisCone{ GenerateCone(0, 1.0f, 1.0f, 16) };
	vector<CMaterialData> vMaterialData{};
	vMaterialData.resize(3);
	vMaterialData[0].SetUniformColor(XMFLOAT3(1, 0, 0));
	vMaterialData[1].SetUniformColor(XMFLOAT3(0, 1, 0));
	vMaterialData[2].SetUniformColor(XMFLOAT3(0, 0, 1));
	m_vMiniAxes[0]->Create(KAxisCone, vMaterialData[0]);
	m_vMiniAxes[0]->RotateRollTo(-XM_PIDIV2);
	
	m_vMiniAxes[1]->Create(KAxisCone, vMaterialData[1]);
	
	m_vMiniAxes[2]->Create(KAxisCone, vMaterialData[2]);
	m_vMiniAxes[2]->RotateYawTo(-XM_PIDIV2);
	m_vMiniAxes[2]->RotateRollTo(-XM_PIDIV2);
	
	XMVECTOR Scaling{ XMVectorSet(0.1f, 0.8f, 0.1f, 0) };
	m_vMiniAxes[0]->ScaleTo(Scaling);
	m_vMiniAxes[1]->ScaleTo(Scaling);
	m_vMiniAxes[2]->ScaleTo(Scaling);
}

void CGame::_CreatePickingRay()
{
	m_PickingRayRep = make_unique<CObject3DLine>("PickingRay", m_Device.Get(), m_DeviceContext.Get());

	vector<SVertex3DLine> Vertices{};
	Vertices.emplace_back(XMVectorSet(0, 0, 0, 1), XMVectorSet(1, 0, 0, 1));
	Vertices.emplace_back(XMVectorSet(10.0f, 10.0f, 0, 1), XMVectorSet(0, 1, 0, 1));

	m_PickingRayRep->Create(Vertices);
}

void CGame::_CreatePickedTriangle()
{
	m_PickedTriangleRep = make_unique<CObject3D>("PickedTriangle", m_Device.Get(), m_DeviceContext.Get());

	m_PickedTriangleRep->Create(GenerateTriangle(XMVectorSet(0, 0, 1.5f, 1), XMVectorSet(+1.0f, 0, 0, 1), XMVectorSet(-1.0f, 0, 0, 1),
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f)));
}

void CGame::SetAssetDirectory(const std::string& Directory)
{
	m_AssetDirectory = Directory;
}

void CGame::SetSceneDirectory(const std::string& Directory)
{
	m_SceneDirectory = Directory;
}

const std::string& CGame::GetAssetDirectory() const
{
	return m_AssetDirectory;
}

const std::string& CGame::GetSceneDirectory() const
{
	return m_SceneDirectory;
}

void CGame::EmptyScene()
{
	DeselectAll();
	ClearCopyList();
	ClearObject3Ds();
	ClearCameras();
	ClearLights();

	m_PhysicsEngine.ClearData();
	m_Intelligence = make_unique<CIntelligence>(m_Device.Get(), m_DeviceContext.Get());
	m_Intelligence->LinkPhysicsEngine(&m_PhysicsEngine); // @important
	m_PtrPlayerCamera = nullptr;
	m_SceneMaterial->ClearAllTexturesData();
	m_SceneMaterialTextureSet->DestroyAllTextures();

	m_Terrain.reset();

	ClearPatterns();
}

void CGame::LoadScene(const string& FileName, const std::string& SceneContentDirectory)
{
	EmptyScene();

	string ReadString{};
	XMVECTOR ReadXMVECTOR{};

	CBinaryData SceneBinaryData{};
	SceneBinaryData.LoadFromFile(FileName);

	// Scene Intelligence (Patterns)
	{
		size_t PatternCount{ SceneBinaryData.ReadUint32() };
		for (size_t iPattern = 0; iPattern < PatternCount; ++iPattern)
		{
			SceneBinaryData.ReadStringWithPrefixedLength(ReadString);

			InsertPattern(ReadString);
		}
	}

	// Terrain
	{
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		LoadTerrain(ReadString);
	}
	
	// Object3D
	{
		uint32_t Object3DCount{ SceneBinaryData.ReadUint32() };
		CBinaryData Object3DBinary{};
		string Object3DName{};
		bool bIsRigged{};
		EObjectRole eObjectRole{};
		for (uint32_t iObject3D = 0; iObject3D < Object3DCount; ++iObject3D)
		{
			SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
			SceneBinaryData.ReadBool(bIsRigged);
			eObjectRole = (EObjectRole)SceneBinaryData.ReadUint8();

			{
				Object3DBinary.LoadFromFile(ReadString);
				Object3DBinary.ReadSkip(8 + 4); // @important: Signature + Version
				Object3DBinary.ReadStringWithPrefixedLength(Object3DName);
			}

			InsertObject3D(Object3DName);
			CObject3D* const Object3D{ GetObject3D(Object3DName) };
			Object3D->LoadOB3D(ReadString, bIsRigged);

			// physics engine
			m_PhysicsEngine.RegisterObject(Object3D, eObjectRole);

			// instance
			{
				bool bIsInstanced{ SceneBinaryData.ReadBool() };
				if (bIsInstanced)
				{
					for (const auto& InstanceCPUData : Object3D->GetInstanceCPUDataVector())
					{
						bool bHasPattern{ SceneBinaryData.ReadBool() };
						if (bHasPattern)
						{
							SObjectIdentifier Identifier{ Object3D, InstanceCPUData.Name.c_str() };

							SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
							m_Intelligence->RegisterPattern(Identifier, GetPattern(ReadString));
						}
					}
				}
			}
		}
	}

	// Editor camera
	{
		m_EditorCamera->SetType((CCamera::EType)SceneBinaryData.ReadUint32());
		SceneBinaryData.ReadXMVECTOR(ReadXMVECTOR);
		m_EditorCamera->TranslateTo(ReadXMVECTOR);
		m_EditorCamera->SetPitch(SceneBinaryData.ReadFloat());
		m_EditorCamera->SetYaw(SceneBinaryData.ReadFloat());
		m_EditorCamera->SetMovementFactor(SceneBinaryData.ReadFloat());
	}

	// Camera
	{
		uint32_t CameraCount{ SceneBinaryData.ReadUint32() };
		for (uint32_t iCamera = 0; iCamera < CameraCount; ++iCamera)
		{
			SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
			CCamera::EType eType{ (CCamera::EType)SceneBinaryData.ReadUint32() };
			InsertCamera(ReadString, eType);
			
			CCamera* const Camera{ GetCamera(ReadString) };
			SceneBinaryData.ReadXMVECTOR(ReadXMVECTOR);
			Camera->TranslateTo(ReadXMVECTOR);
			Camera->SetPitch(SceneBinaryData.ReadFloat());
			Camera->SetYaw(SceneBinaryData.ReadFloat());
			Camera->SetMovementFactor(SceneBinaryData.ReadFloat());
			Camera->SetZoomDistance(SceneBinaryData.ReadFloat());
			if (SceneBinaryData.ReadBool())
			{
				SetPlayerCamera(Camera);
			}

			m_CameraRep->TranslateInstanceTo(ReadString, Camera->GetEyePosition());
			m_CameraRep->RotateInstancePitchTo(ReadString, Camera->GetPitch());
			m_CameraRep->RotateInstanceYawTo(ReadString, Camera->GetYaw());
			m_CameraRep->UpdateInstanceWorldMatrix(ReadString);
		}
	}
	
	// Light
	{
		for (uint32_t iLightType = 0; iLightType < CLight::KLightTypeCount; ++iLightType)
		{
			uint32_t LightType{ SceneBinaryData.ReadUint32() };

			uint32_t LightCount{ SceneBinaryData.ReadUint32() };

			for (uint32_t iLight = 0; iLight < LightCount; ++iLight)
			{
				CLight::SInstanceCPUData InstanceCPUData{};
				CLight::SInstanceGPUData InstanceGPUData{};

				SceneBinaryData.ReadStringWithPrefixedLength(InstanceCPUData.Name);
				InstanceCPUData.eType = (CLight::EType)SceneBinaryData.ReadUint32();

				SceneBinaryData.ReadXMVECTOR(InstanceGPUData.Position);
				SceneBinaryData.ReadXMVECTOR(InstanceGPUData.Color);
				SceneBinaryData.ReadXMVECTOR(InstanceGPUData.Direction);
				SceneBinaryData.ReadFloat(InstanceGPUData.Range);
				SceneBinaryData.ReadFloat(InstanceGPUData.Theta);

				InsertLight(InstanceCPUData.eType, InstanceCPUData.Name);

				m_LightArray[(uint32_t)InstanceCPUData.eType]->SetInstanceGPUData(InstanceCPUData.Name, InstanceGPUData);
				m_LightRep->SetInstancePosition(InstanceCPUData.Name, InstanceGPUData.Position);
			}
		}
	}

	// Scene material
	{
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_SceneMaterial->SetTextureFileName(ETextureType::BaseColorTexture, ReadString);

		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_SceneMaterial->SetTextureFileName(ETextureType::NormalTexture, ReadString);
		
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_SceneMaterial->SetTextureFileName(ETextureType::OpacityTexture, ReadString);
		
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_SceneMaterial->SetTextureFileName(ETextureType::RoughnessTexture, ReadString);
		
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_SceneMaterial->SetTextureFileName(ETextureType::MetalnessTexture, ReadString);
		
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_SceneMaterial->SetTextureFileName(ETextureType::AmbientOcclusionTexture, ReadString);

		m_SceneMaterialTextureSet->CreateTextures(*m_SceneMaterial);

		UpdateSceneMaterial();
	}

	// Light probe
	{
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_EnvironmentTexture->CreateCubeMapFromFile(ReadString);
		m_EnvironmentTexture->SetSlot(KEnvironmentTextureSlot);
		m_EnvironmentCubemapRep->UnfoldCubemap(m_EnvironmentTexture->GetShaderResourceViewPtr());

		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_IrradianceTexture->CreateCubeMapFromFile(ReadString);
		m_IrradianceTexture->SetSlot(KIrradianceTextureSlot);
		m_IrradianceCubemapRep->UnfoldCubemap(m_IrradianceTexture->GetShaderResourceViewPtr());

		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_PrefilteredRadianceTexture->CreateCubeMapFromFile(ReadString);
		m_PrefilteredRadianceTexture->SetSlot(KPrefilteredRadianceTextureSlot);
		m_PrefilteredRadianceCubemapRep->UnfoldCubemap(m_PrefilteredRadianceTexture->GetShaderResourceViewPtr());

		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		m_IntegratedBRDFTexture->CreateCubeMapFromFile(ReadString);
		m_IntegratedBRDFTexture->SetSlot(KIntegratedBRDFTextureSlot);

		UpdateCBGlobalLightProbeData();
	}

	// Directional & Ambient light
	{
		SceneBinaryData.ReadXMVECTOR(m_CBGlobalLightData.DirectionalLightDirection);
		SceneBinaryData.ReadXMFLOAT3(m_CBGlobalLightData.DirectionalLightColor);
		SceneBinaryData.ReadXMFLOAT3(m_CBGlobalLightData.AmbientLightColor);
		SceneBinaryData.ReadFloat(m_CBGlobalLightData.AmbientLightIntensity);
		SceneBinaryData.ReadFloat(m_CBGlobalLightData.Exposure);

		m_CBGlobalLight->Update();
	}
}

void CGame::SaveScene(const string& FileName, const std::string& SceneContentDirectory)
{
	std::filesystem::remove_all(SceneContentDirectory.c_str());
	std::filesystem::create_directory(SceneContentDirectory.c_str());

	CBinaryData SceneBinaryData{};

	// Scene Intelligence (Patterns)
	{
		SceneBinaryData.WriteUint32((uint32_t)m_vPatterns.size());
		for (const auto& Pattern : m_vPatterns)
		{
			SceneBinaryData.WriteStringWithPrefixedLength(Pattern->GetFileName());
		}
	}
	
	// Terrain
	{
		if (m_Terrain)
		{
			if (m_Terrain->GetFileName().empty())
			{
				m_Terrain->Save(SceneContentDirectory + "terrain.terr");
			}

			SceneBinaryData.WriteStringWithPrefixedLength(m_Terrain->GetFileName());
		}
		else
		{
			SceneBinaryData.WriteUint32(0);
		}
	}

	// Object3D
	{
		uint32_t Object3DCount{ (uint32_t)m_vObject3Ds.size() };
		SceneBinaryData.WriteUint32(Object3DCount);
		if (Object3DCount)
		{
			for (const auto& Object3D : m_vObject3Ds)
			{
				// @important: always save OB3D
				{
					Object3D->SaveOB3D(SceneContentDirectory + Object3D->GetName() + ".ob3d");
				}

				SceneBinaryData.WriteStringWithPrefixedLength(Object3D->GetOB3DFileName());

				// @important
				SceneBinaryData.WriteBool(Object3D->IsRigged());

				EObjectRole eObjectRole{ m_PhysicsEngine.GetObjectRole(Object3D.get()) };
				SceneBinaryData.WriteUint8((uint8_t)eObjectRole);

				// instance
				{
					SceneBinaryData.WriteBool(Object3D->IsInstanced());
					if (Object3D->IsInstanced())
					{
						for (const auto& InstanceCPUData : Object3D->GetInstanceCPUDataVector())
						{
							SObjectIdentifier Identifier{ Object3D.get(), InstanceCPUData.Name.c_str() };

							bool bHasPattern{ m_Intelligence->HasPattern(Identifier) };
							SceneBinaryData.WriteBool(bHasPattern);
							if (bHasPattern)
							{
								CPattern* Pattern{ m_Intelligence->GetPattern(Identifier) };
								SceneBinaryData.WriteStringWithPrefixedLength(Pattern->GetFileName());
							}
						}
					}
				}
			}
		}
	}

	// Editor camera
	{
		SceneBinaryData.WriteUint32((uint32_t)m_EditorCamera->GetType());
		SceneBinaryData.WriteXMVECTOR(m_EditorCamera->GetTranslation());
		SceneBinaryData.WriteFloat(m_EditorCamera->GetPitch());
		SceneBinaryData.WriteFloat(m_EditorCamera->GetYaw());
		SceneBinaryData.WriteFloat(m_EditorCamera->GetMovementFactor());
	}

	// Camera
	{
		uint32_t CameraCount{ (uint32_t)m_vCameras.size() };
		SceneBinaryData.WriteUint32(CameraCount);
		for (const auto& Camera : m_vCameras)
		{
			SceneBinaryData.WriteStringWithPrefixedLength(Camera->GetName());
			SceneBinaryData.WriteUint32((uint32_t)Camera->GetType());
			SceneBinaryData.WriteXMVECTOR(Camera->GetTranslation());
			SceneBinaryData.WriteFloat(Camera->GetPitch());
			SceneBinaryData.WriteFloat(Camera->GetYaw());
			SceneBinaryData.WriteFloat(Camera->GetMovementFactor());
			SceneBinaryData.WriteFloat(Camera->GetZoomDistance());
			SceneBinaryData.WriteBool(IsPlayerCamera(Camera.get()));
		}
	}

	// LightArray
	{
		for (auto& Light : m_LightArray)
		{
			uint32_t LightType{ (uint32_t)Light->GetType() };
			SceneBinaryData.WriteUint32(LightType);

			uint32_t LightCount{ (uint32_t)Light->GetInstanceCount() };
			SceneBinaryData.WriteUint32(LightCount);

			if (LightCount)
			{
				for (const auto& LightPair : Light->GetInstanceNameToIndexMap())
				{
					const auto& InstanceCPUData{ Light->GetInstanceCPUData(LightPair.first) };
					const auto& InstanceGPUData{ Light->GetInstanceGPUData(LightPair.first) };

					SceneBinaryData.WriteStringWithPrefixedLength(InstanceCPUData.Name);
					SceneBinaryData.WriteUint32((uint32_t)InstanceCPUData.eType);

					SceneBinaryData.WriteXMVECTOR(InstanceGPUData.Position);
					SceneBinaryData.WriteXMVECTOR(InstanceGPUData.Color);
					SceneBinaryData.WriteXMVECTOR(InstanceGPUData.Direction);
					SceneBinaryData.WriteFloat(InstanceGPUData.Range);
					SceneBinaryData.WriteFloat(InstanceGPUData.Theta);
				}
			}
		}
	}

	// Scene material
	{
		SceneBinaryData.WriteStringWithPrefixedLength(m_SceneMaterial->GetTextureFileName(ETextureType::BaseColorTexture));
		SceneBinaryData.WriteStringWithPrefixedLength(m_SceneMaterial->GetTextureFileName(ETextureType::NormalTexture));
		SceneBinaryData.WriteStringWithPrefixedLength(m_SceneMaterial->GetTextureFileName(ETextureType::OpacityTexture));
		SceneBinaryData.WriteStringWithPrefixedLength(m_SceneMaterial->GetTextureFileName(ETextureType::RoughnessTexture));
		SceneBinaryData.WriteStringWithPrefixedLength(m_SceneMaterial->GetTextureFileName(ETextureType::MetalnessTexture));
		SceneBinaryData.WriteStringWithPrefixedLength(m_SceneMaterial->GetTextureFileName(ETextureType::AmbientOcclusionTexture));
	}

	// Light probe
	{
		SceneBinaryData.WriteStringWithPrefixedLength(m_EnvironmentTexture->GetFileName());
		SceneBinaryData.WriteStringWithPrefixedLength(m_IrradianceTexture->GetFileName());
		SceneBinaryData.WriteStringWithPrefixedLength(m_PrefilteredRadianceTexture->GetFileName());
		SceneBinaryData.WriteStringWithPrefixedLength(m_IntegratedBRDFTexture->GetFileName());
	}

	// Directional & Ambient light
	{
		SceneBinaryData.WriteXMVECTOR(m_CBGlobalLightData.DirectionalLightDirection);
		SceneBinaryData.WriteXMFLOAT3(m_CBGlobalLightData.DirectionalLightColor);
		SceneBinaryData.WriteXMFLOAT3(m_CBGlobalLightData.AmbientLightColor);
		SceneBinaryData.WriteFloat(m_CBGlobalLightData.AmbientLightIntensity);
		SceneBinaryData.WriteFloat(m_CBGlobalLightData.Exposure);
	}
	
	SceneBinaryData.SaveToFile(FileName);
}

void CGame::SetProjectionMatrices(float FOV, float NearZ, float FarZ)
{
	m_NearZ = NearZ;
	m_FarZ = FarZ;

	m_MatrixProjection = XMMatrixPerspectiveFovLH(FOV, m_WindowSize.x / m_WindowSize.y, m_NearZ, m_FarZ);
	m_MatrixProjection2D = XMMatrixOrthographicLH(m_WindowSize.x, m_WindowSize.y, 0.0f, 1.0f);
}

void CGame::SetRenderingFlags(EFlagsRendering Flags)
{
	m_eFlagsRendering = Flags;
}

void CGame::ToggleRenderingFlag(EFlagsRendering Flags)
{
	m_eFlagsRendering ^= Flags;
}

void CGame::TurnOnRenderingFlag(EFlagsRendering Flags)
{
	if (EFLAG_HAS_NO(m_eFlagsRendering, Flags))
	{
		ToggleRenderingFlag(Flags);
	}
}

void CGame::TurnOffRenderingFlag(EFlagsRendering Flags)
{
	if (EFLAG_HAS(m_eFlagsRendering, Flags))
	{
		ToggleRenderingFlag(Flags);
	}
}

CGame::EFlagsRendering CGame::GetRenderingFlags() const
{
	return m_eFlagsRendering;
}

void CGame::SetUniversalRSState()
{
	switch (m_eRasterizerState)
	{
	case ERasterizerState::CullNone:
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
		break;
	case ERasterizerState::CullClockwise:
		m_DeviceContext->RSSetState(m_CommonStates->CullClockwise());
		break;
	case ERasterizerState::CullCounterClockwise:
		m_DeviceContext->RSSetState(m_CommonStates->CullCounterClockwise());
		break;
	case ERasterizerState::WireFrame:
		m_DeviceContext->RSSetState(m_CommonStates->Wireframe());
		break;
	default:
		break;
	}
}

void CGame::UpdateCBSpace(const XMMATRIX& World)
{
	const XMMATRIX WorldT{ XMMatrixTranspose(World) };
	const XMMATRIX ViewT{ XMMatrixTranspose(m_MatrixView) };
	const XMMATRIX ProjectionT{ XMMatrixTranspose(m_MatrixProjection) };
	const XMMATRIX ViewProjectionT{ XMMatrixTranspose(m_MatrixView * m_MatrixProjection) };
	const XMMATRIX WVPT{ XMMatrixTranspose(World * m_MatrixView * m_MatrixProjection) };

	m_CBSpaceData.World = WorldT;
	m_CBSpaceData.View = ViewT;
	m_CBSpaceData.Projection = ProjectionT;
	m_CBSpaceData.ViewProjection = ViewProjectionT;
	m_CBSpaceData.WVP = WVPT;
	m_CBSpaceData.EyePosition = m_PtrCurrentCamera->GetTranslation();
	m_CBSpace->Update();
}

void CGame::UpdateCBSpace(const XMMATRIX& World, const XMMATRIX& Projection)
{
	const XMMATRIX WorldT{ XMMatrixTranspose(World) };
	const XMMATRIX ViewT{ XMMatrixTranspose(m_MatrixView) };
	const XMMATRIX ProjectionT{ XMMatrixTranspose(Projection) };
	const XMMATRIX ViewProjectionT{ XMMatrixTranspose(m_MatrixView * Projection) };
	const XMMATRIX WVPT{ XMMatrixTranspose(World * m_MatrixView * Projection) };

	m_CBSpaceData.World = WorldT;
	m_CBSpaceData.View = ViewT;
	m_CBSpaceData.Projection = ProjectionT;
	m_CBSpaceData.ViewProjection = ViewProjectionT;
	m_CBSpaceData.WVP = WVPT;
	m_CBSpaceData.EyePosition = m_PtrCurrentCamera->GetTranslation();
	m_CBSpace->Update();
}

void CGame::UpdateCBAnimationBoneMatrices(const XMMATRIX* const BoneMatrices)
{
	memcpy(m_CBAnimationBonesData.BoneMatrices, BoneMatrices, sizeof(SCBAnimationBonesData));
	m_CBAnimationBones->Update();
}

void CGame::UpdateCBAnimationData(const CObject3D::SCBAnimationData& Data)
{
	m_CBAnimationData = Data;
	m_CBAnimation->Update();
}

void CGame::UpdateCBTerrainData(const CTerrain::SCBTerrainData& Data)
{
	m_CBTerrainData = Data;
	m_CBTerrain->Update();
}

void CGame::UpdateCBWindData(const CTerrain::SCBWindData& Data)
{
	m_CBWindData = Data;
	m_CBWind->Update();
}

void CGame::UpdateCBTessFactorData(const CObject3D::SCBTessFactorData& Data)
{
	m_CBTessFactorData = Data;
	m_CBTessFactor->Update();
}

void CGame::UpdateCBDisplacementData(const CObject3D::SCBDisplacementData& Data)
{
	m_CBDisplacementData = Data;
	m_CBDisplacement->Update();
}

void CGame::UpdateCBTerrainMaskingSpace(const XMMATRIX& Matrix)
{
	m_CBTerrainMaskingSpaceData.Matrix = XMMatrixTranspose(Matrix);
	m_CBTerrainMaskingSpace->Update();
}

void CGame::UpdateCBTerrainSelection(const CTerrain::SCBTerrainSelectionData& Selection)
{
	m_CBTerrainSelectionData = Selection;
	m_CBTerrainSelection->Update();

	UpdateCBSpace(m_CBTerrainSelectionData.TerrainWorld);
}

void CGame::UpdateCBGlobalLightProbeData()
{
	if (m_IrradianceTexture) m_CBGlobalLightData.IrradianceTextureMipLevels = m_IrradianceTexture->GetMipLevels();
	if (m_PrefilteredRadianceTexture) m_CBGlobalLightData.PrefilteredRadianceTextureMipLevels = m_PrefilteredRadianceTexture->GetMipLevels();
	m_CBGlobalLight->Update();
}

void CGame::UpdateSceneMaterial()
{
	uint32_t FlagsHasSceneTexture{};
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::DiffuseTexture) ? 0x01 : 0;
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::NormalTexture) ? 0x02 : 0;
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::OpacityTexture) ? 0x04 : 0;
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::SpecularIntensityTexture) ? 0x08 : 0;
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::RoughnessTexture) ? 0x10 : 0;
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::MetalnessTexture) ? 0x20 : 0;
	FlagsHasSceneTexture += m_SceneMaterialTextureSet->HasTexture(ETextureType::AmbientOcclusionTexture) ? 0x40 : 0;

	uint32_t FlagsIsSceneTextureSRGB{};
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::DiffuseTexture) ? 0x01 : 0;
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::NormalTexture) ? 0x02 : 0;
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::OpacityTexture) ? 0x04 : 0;
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::SpecularIntensityTexture) ? 0x08 : 0;
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::RoughnessTexture) ? 0x10 : 0;
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::MetalnessTexture) ? 0x20 : 0;
	FlagsIsSceneTextureSRGB += m_SceneMaterialTextureSet->IssRGB(ETextureType::AmbientOcclusionTexture) ? 0x40 : 0;

	m_SceneMaterialTextureSet->SetSlot(ETextureType::BaseColorTexture, 60);
	m_SceneMaterialTextureSet->SetSlot(ETextureType::NormalTexture, 61);
	m_SceneMaterialTextureSet->SetSlot(ETextureType::OpacityTexture, 62);
	m_SceneMaterialTextureSet->SetSlot(ETextureType::RoughnessTexture, 63);
	m_SceneMaterialTextureSet->SetSlot(ETextureType::MetalnessTexture, 64);
	m_SceneMaterialTextureSet->SetSlot(ETextureType::AmbientOcclusionTexture, 65);
	m_SceneMaterialTextureSet->UseTextures();

	m_CBSceneMaterialData.FlagsHasSceneTexture = FlagsHasSceneTexture;
	m_CBSceneMaterialData.FlagsIsSceneTextureSRGB = FlagsIsSceneTextureSRGB;
	m_CBSceneMaterial->Update();
}

void CGame::CreateDynamicSky(const string& SkyDataFileName, float ScalingFactor)
{
	using namespace tinyxml2;

	m_SkyFileName = SkyDataFileName;
	m_SkyScalingFactor = ScalingFactor;

	size_t Point{ SkyDataFileName.find_last_of('.') };
	string Extension{ SkyDataFileName.substr(Point + 1) };
	for (auto& c : Extension)
	{
		c = toupper(c);
	}
	assert(Extension == "XML");
	
	tinyxml2::XMLDocument xmlDocument{};
	if (xmlDocument.LoadFile(SkyDataFileName.c_str()) != XML_SUCCESS)
	{
		MB_WARN(("Sky 설정 파일을 찾을 수 없습니다. (" + SkyDataFileName + ")").c_str(), "Sky 설정 불러오기 실패");
		return;
	}
	
	XMLElement* xmlRoot{ xmlDocument.FirstChildElement() };
	XMLElement* xmlTexture{ xmlRoot->FirstChildElement() };
	m_SkyData.TextureFileName = xmlTexture->GetText();

	XMLElement* xmlSun{ xmlTexture->NextSiblingElement() };
	{
		LoadSkyObjectData(xmlSun, m_SkyData.Sun);
	}

	XMLElement* xmlMoon{ xmlSun->NextSiblingElement() };
	{
		LoadSkyObjectData(xmlMoon, m_SkyData.Moon);
	}

	XMLElement* xmlCloud{ xmlMoon->NextSiblingElement() };
	{
		LoadSkyObjectData(xmlCloud, m_SkyData.Cloud);
	}

	m_SkyMaterialData.SetTextureFileName(ETextureType::DiffuseTexture, m_SkyData.TextureFileName);

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get());
	m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom), m_SkyMaterialData);
	m_Object3DSkySphere->ScaleTo(XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0));
	m_Object3DSkySphere->IsPickable(false);

	m_Object3DSun = make_unique<CObject3D>("Sun", m_Device.Get(), m_DeviceContext.Get());
	m_Object3DSun->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterialData);
	m_Object3DSun->UpdateQuadUV(m_SkyData.Sun.UVOffset, m_SkyData.Sun.UVSize);
	m_Object3DSun->ScaleTo(XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Sun.WidthHeightRatio, 0));
	m_Object3DSun->IsTransparent(true);
	m_Object3DSun->IsPickable(false);
	
	m_Object3DMoon = make_unique<CObject3D>("Moon", m_Device.Get(), m_DeviceContext.Get());
	m_Object3DMoon->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterialData);
	m_Object3DMoon->UpdateQuadUV(m_SkyData.Moon.UVOffset, m_SkyData.Moon.UVSize);
	m_Object3DMoon->ScaleTo(XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Moon.WidthHeightRatio, 0));
	m_Object3DMoon->IsTransparent(true);
	m_Object3DMoon->IsPickable(false);
	
	m_SkyData.bIsDataSet = true;
	m_SkyData.bIsDynamic = true;

	return;
}

void CGame::CreateStaticSky(float ScalingFactor)
{
	m_SkyScalingFactor = ScalingFactor;

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get());
	//m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom));
	m_Object3DSkySphere->Create(GenerateCubemapSphere(KSkySphereSegmentCount));
	m_Object3DSkySphere->ScaleTo(XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0));
	m_Object3DSkySphere->IsPickable(false);

	m_SkyData.bIsDataSet = true;
	m_SkyData.bIsDynamic = false;
}

void CGame::LoadSkyObjectData(const tinyxml2::XMLElement* const xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData)
{
	using namespace tinyxml2;

	const XMLElement* xmlUVOffset{ xmlSkyObject->FirstChildElement() };
	SkyObjectData.UVOffset.x = xmlUVOffset->FloatAttribute("U");
	SkyObjectData.UVOffset.y = xmlUVOffset->FloatAttribute("V");

	const XMLElement* xmlUVSize{ xmlUVOffset->NextSiblingElement() };
	SkyObjectData.UVSize.x = xmlUVSize->FloatAttribute("U");
	SkyObjectData.UVSize.y = xmlUVSize->FloatAttribute("V");

	const XMLElement* xmlWidthHeightRatio{ xmlUVSize->NextSiblingElement() };
	SkyObjectData.WidthHeightRatio = stof(xmlWidthHeightRatio->GetText());
}

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMFLOAT3& Color)
{
	m_CBGlobalLightData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	m_CBGlobalLightData.DirectionalLightColor = Color;

	m_CBGlobalLight->Update();
}

void CGame::SetDirectionalLightDirection(const XMVECTOR& LightSourcePosition)
{
	m_CBGlobalLightData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);

	m_CBGlobalLight->Update();
}

void CGame::SetDirectionalLightColor(const XMFLOAT3& Color)
{
	m_CBGlobalLightData.DirectionalLightColor = Color;

	m_CBGlobalLight->Update();
}

const XMVECTOR& CGame::GetDirectionalLightDirection() const
{
	return m_CBGlobalLightData.DirectionalLightDirection;
}

const XMFLOAT3& CGame::GetDirectionalLightColor() const
{
	return m_CBGlobalLightData.DirectionalLightColor;
}

void CGame::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	m_CBGlobalLightData.AmbientLightColor = Color;
	m_CBGlobalLightData.AmbientLightIntensity = Intensity;

	m_CBGlobalLight->Update();
}

const XMFLOAT3& CGame::GetAmbientLightColor() const
{
	return m_CBGlobalLightData.AmbientLightColor;
}

float CGame::GetAmbientLightIntensity() const
{
	return m_CBGlobalLightData.AmbientLightIntensity;
}

void CGame::SetExposure(float Value)
{
	m_CBGlobalLightData.Exposure = Value;

	m_CBGlobalLight->Update();
}

float CGame::GetExposure()
{
	return m_CBGlobalLightData.Exposure;
}

void CGame::CreateTerrain(const XMFLOAT2& TerrainSize, uint32_t MaskingDetail, float UniformScaling)
{
	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Create(TerrainSize, *m_TerrainMaterialDefault, MaskingDetail, UniformScaling);
	UpdateCBTerrainData(m_Terrain->GetTerrainData());
	UpdateCBTerrainMaskingSpace(m_Terrain->GetMaskingSpaceData());
	
	ID3D11ShaderResourceView* NullSRVs[20]{};
	m_DeviceContext->DSSetShaderResources(0, 1, NullSRVs);
	m_DeviceContext->PSSetShaderResources(0, 20, NullSRVs);
}

void CGame::LoadTerrain(const string& TerrainFileName)
{
	if (TerrainFileName.empty())
	{
		m_Terrain.release();
		return;
	}

	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Load(TerrainFileName);
	UpdateCBTerrainData(m_Terrain->GetTerrainData());
	UpdateCBTerrainMaskingSpace(m_Terrain->GetMaskingSpaceData());
}

void CGame::SaveTerrain(const string& TerrainFileName)
{
	if (!m_Terrain) return;
	if (TerrainFileName.empty()) return;

	m_Terrain->Save(TerrainFileName);
}

void CGame::ClearCopyList()
{
	m_vCopyObject3Ds.clear();
	m_vCopyObject3DInstances.clear();
	m_vCopyLights.clear();
}

void CGame::CopySelectedObject()
{
	if (IsAnythingSelected())
	{
		ClearCopyList();
		
		for (const auto& SelectionData : m_vSelectionData)
		{
			switch (SelectionData.eObjectType)
			{
			default:
			case CGame::EObjectType::NONE:
			case CGame::EObjectType::EditorCamera:
				break;
			case CGame::EObjectType::Object3D:
			case CGame::EObjectType::Object3DInstance:
			{
				CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
				if (SelectionData.eObjectType == EObjectType::Object3DInstance)
				{
					const CObject3D* const KObject3D{ Object3D };
					const auto& InstanceCPUData{ KObject3D->GetInstanceCPUData(SelectionData.Name) };
					m_vCopyObject3DInstances.emplace_back(InstanceCPUData, Object3D);
				}
				else
				{
					m_vCopyObject3Ds.emplace_back(SelectionData.Name, Object3D->GetModel(),
						Object3D->GetTransform(), Object3D->GetPhysics(), Object3D->GetRender(),
						Object3D->GetInstanceCPUDataVector());
				}
				break;
			}
			case CGame::EObjectType::Object3DLine:
				break;
			case CGame::EObjectType::Object2D:
				break;
			case CGame::EObjectType::Camera:
				break;
			case CGame::EObjectType::Light:
				m_vCopyLights.emplace_back(
					m_LightArray[SelectionData.Extra]->GetInstanceCPUData(SelectionData.Name),
					m_LightArray[SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name));
				break;
			}
		}
	}
}

void CGame::PasteCopiedObject()
{
	DeselectAll();

	size_t SelectionCount{};

	for (auto& CopyObject3D : m_vCopyObject3Ds)
	{
		string NewName{ CopyObject3D.Name + "_" + to_string(CopyObject3D.PasteCounter) };
		while (IsObject3DNameInsertable(NewName, false) == false)
		{
			++CopyObject3D.PasteCounter;
			NewName = CopyObject3D.Name + "_" + to_string(CopyObject3D.PasteCounter);
		}

		if (InsertObject3D(NewName))
		{
			CObject3D* const Object3D{ GetObject3D(NewName) };

			Object3D->Create(CopyObject3D.Model);
			Object3D->CreateInstances(CopyObject3D.vInstanceCPUData);
			Object3D->SetTransform(CopyObject3D.ComponentTransform);
			Object3D->SetPhysics(CopyObject3D.ComponentPhysics);
			Object3D->SetRender(CopyObject3D.ComponentRender);

			Object3D->UpdateWorldMatrix();
			Object3D->UpdateAllInstances();

			SelectObject(SSelectionData(EObjectType::Object3D, NewName, Object3D), ESelectionMode::Additive);
			++SelectionCount;

			++CopyObject3D.PasteCounter;
		}
	}

	for (auto& CopyObject3DInstance : m_vCopyObject3DInstances)
	{
		CObject3D* const Object3D{ CopyObject3DInstance.PtrObject3D };

		if (Object3D->InsertInstance())
		{
			auto& InstanceName{ Object3D->GetLastInstanceName() };
			Object3D->SetInstanceCPUData(InstanceName, CopyObject3DInstance.InstanceCPUData);
			Object3D->UpdateInstanceWorldMatrix(InstanceName);
			SelectObject(SSelectionData(EObjectType::Object3DInstance, InstanceName, Object3D), ESelectionMode::Additive);
			++SelectionCount;

			++CopyObject3DInstance.PasteCounter;
		}
	}

	for (auto& CopyLight : m_vCopyLights)
	{
		string NewName{ CopyLight.InstanceCPUData.Name + "_" + to_string(CopyLight.PasteCounter) };
		while ((m_umapLightNames.find(NewName) != m_umapLightNames.end()))
		{
			++m_LightCreationCounter;
			NewName = CopyLight.InstanceCPUData.Name + "_" + to_string(CopyLight.PasteCounter);
		}

		if (InsertLight(CopyLight.InstanceCPUData.eType, NewName))
		{
			m_LightArray[(uint32_t)CopyLight.InstanceCPUData.eType]->SetInstanceGPUData(NewName, CopyLight.InstanceGPUData);
			m_LightRep->SetInstancePosition(NewName, CopyLight.InstanceGPUData.Position);

			SelectObject(SSelectionData(EObjectType::Light, NewName, (uint32_t)CopyLight.InstanceCPUData.eType), ESelectionMode::Additive);
			++SelectionCount;

			++CopyLight.PasteCounter;
		}
	}

	// @important
	m_MultipleSelectionWorldCenter /= (float)SelectionCount;
	
	Capture3DGizmoTranslation();
}

void CGame::DeleteSelectedObjects()
{
	// Object3D
	if (IsAnythingSelected())
	{
		for (const auto& SelectionData : m_vSelectionData)
		{
			switch (SelectionData.eObjectType)
			{
			default:
			case CGame::EObjectType::NONE:
			case CGame::EObjectType::EditorCamera:
				break;
			case CGame::EObjectType::Object3D:
				DeleteObject3D(SelectionData.Name);
				break;
			case CGame::EObjectType::Object3DLine:
				break;
			case CGame::EObjectType::Object2D:
				DeleteObject2D(SelectionData.Name);
				break;
			case CGame::EObjectType::Camera:
				DeleteCamera(SelectionData.Name);
				break;
			case CGame::EObjectType::Light:
				DeleteLight((CLight::EType)SelectionData.Extra, SelectionData.Name);
				break;
			case CGame::EObjectType::Object3DInstance:
			{
				CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
				Object3D->DeleteInstance(SelectionData.Name);
				break;
			}
			}
		}

		DeselectAll();

		ClearCopyList();
	}
}

bool CGame::InsertCamera(const string& Name, CCamera::EType eType)
{
	if (Name == m_EditorCamera->GetName())
	{
		MB_WARN(("고를 수 없는 이름입니다. (" + Name + ")").c_str(), "Camera 생성 실패");
		return false;
	}
	if (m_mapCameraNameToIndex.find(Name) != m_mapCameraNameToIndex.end())
	{
		MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Camera 생성 실패");
		return false;
	}
	if (Name.size() >= KAssetNameMaxLength)
	{
		MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Camera 생성 실패");
		return false;
	}
	else if (Name.size() == 0)
	{
		MB_WARN("이름은 공백일 수 없습니다.", "Camera 생성 실패");
		return false;
	}

	m_vCameras.emplace_back(make_unique<CCamera>(Name));
	m_vCameras.back()->SetType(eType);
	m_vCameras.back()->TranslateTo(XMVectorSet(0, 1.0f, 0, 1));

	m_mapCameraNameToIndex[Name] = m_vCameras.size() - 1;

	m_CameraRep->InsertInstance(Name);

	m_CameraRep->TranslateInstanceTo(Name, m_vCameras.back()->GetEyePosition());
	m_CameraRep->RotateInstancePitchTo(Name, m_vCameras.back()->GetPitch());
	m_CameraRep->RotateInstanceYawTo(Name, m_vCameras.back()->GetYaw());
	m_CameraRep->UpdateInstanceWorldMatrix(Name);

	return true;
}

void CGame::DeleteCamera(const std::string& Name)
{
	if (Name == m_EditorCamera->GetName()) return;
	if (Name.length() == 0) return;
	if (m_mapCameraNameToIndex.find(Name) == m_mapCameraNameToIndex.end())
	{
		MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Camera 삭제 실패");
		return;
	}

	size_t ID{ m_mapCameraNameToIndex.at(Name) };
	if (ID < m_vCameras.size() - 1)
	{
		const string& SwappedName{ m_vCameras.back()->GetName() };
		swap(m_vCameras[ID], m_vCameras.back());

		m_mapCameraNameToIndex[SwappedName] = ID;
	}

	m_vCameras.pop_back();
	m_mapCameraNameToIndex.erase(Name);

	UseEditorCamera();

	m_CameraRep->DeleteInstance(Name);
}

void CGame::ClearCameras()
{
	m_mapCameraNameToIndex.clear();
	m_vCameras.clear();

	m_CameraRep->ClearInstances();

	UseEditorCamera();

	DeselectType(EObjectType::Camera);
}

CCamera* CGame::GetCamera(const string& Name, bool bShowWarning) const
{
	if (m_mapCameraNameToIndex.find(Name) == m_mapCameraNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Camera 얻어오기 실패");
		return nullptr;
	}
	return m_vCameras[GetCameraID(Name)].get();
}

size_t CGame::GetCameraID(const std::string Name) const
{
	return m_mapCameraNameToIndex.at(Name);
}

CShader* CGame::AddCustomShader()
{
	m_vCustomShaders.emplace_back(make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vCustomShaders.back().get();
}

CShader* CGame::GetCustomShader(size_t Index) const
{
	assert(Index < m_vCustomShaders.size());
	return m_vCustomShaders[Index].get();
}

CShader* CGame::GetBaseShader(EBaseShader eShader) const
{
	CShader* Result{};
	switch (eShader)
	{
	case EBaseShader::VSBase:
		Result = m_VSBase.get();
		break;
	case EBaseShader::VSAnimation:
		Result = m_VSAnimation.get();
		break;
	case EBaseShader::VSSky:
		Result = m_VSSky.get();
		break;
	case EBaseShader::VSLine:
		Result = m_VSLine.get();
		break;
	case EBaseShader::VSTerrain:
		Result = m_VSTerrain.get();
		break;
	case EBaseShader::VSFoliage:
		Result = m_VSFoliage.get();
		break;
	case EBaseShader::VSBase2D:
		Result = m_VSBase2D.get();
		break;
	case EBaseShader::VSLight:
		Result = m_VSLight.get();
		break;
	case EBaseShader::HSTerrain:
		Result = m_HSTerrain.get();
		break;
	case EBaseShader::HSWater:
		Result = m_HSWater.get();
		break;
	case EBaseShader::HSStatic:
		Result = m_HSStatic.get();
		break;
	case EBaseShader::HSPointLight:
		Result = m_HSPointLight.get();
		break;
	case EBaseShader::HSSpotLight:
		Result = m_HSSpotLight.get();
		break;
	case EBaseShader::DSTerrain:
		Result = m_DSTerrain.get();
		break;
	case EBaseShader::DSWater:
		Result = m_DSWater.get();
		break;
	case EBaseShader::DSStatic:
		Result = m_DSStatic.get();
		break;
	case EBaseShader::DSPointLight:
		Result = m_DSPointLight.get();
		break;
	case EBaseShader::DSSpotLight:
		Result = m_DSSpotLight.get();
		break;
	case EBaseShader::GSNormal:
		Result = m_GSNormal.get();
		break;
	case EBaseShader::PSBase:
		Result = m_PSBase.get();
		break;
	case EBaseShader::PSBase_RawVertexColor:
		Result = m_PSBase_RawVertexColor.get();
		break;
	case EBaseShader::PSBase_RawDiffuseColor:
		Result = m_PSBase_RawDiffuseColor.get();
		break;
	case EBaseShader::PSDynamicSky:
		Result = m_PSDynamicSky.get();
		break;
	case EBaseShader::PSCloud:
		Result = m_PSCloud.get();
		break;
	case EBaseShader::PSLine:
		Result = m_PSLine.get();
		break;
	case EBaseShader::PSTerrain:
		Result = m_PSTerrain.get();
		break;
	case EBaseShader::PSWater:
		Result = m_PSWater.get();
		break;
	case EBaseShader::PSFoliage:
		Result = m_PSFoliage.get();
		break;
	case EBaseShader::PSCamera:
		Result = m_PSCamera.get();
		break;
	case EBaseShader::PSEdgeDetector:
		Result = m_PSEdgeDetector.get();
		break;
	case EBaseShader::PSSky:
		Result = m_PSSky.get();
		break;
	case EBaseShader::PSBase_GBuffer:
		Result = m_PSBase_GBuffer.get();
		break;
	case EBaseShader::PSBase_Void:
		Result = m_PSBase_Void.get();
		break;
	case EBaseShader::PSDirectionalLight:
		Result = m_PSDirectionalLight.get();
		break;
	case EBaseShader::PSDirectionalLight_NonIBL:
		Result = m_PSDirectionalLight_NonIBL.get();
		break;
	case EBaseShader::PSPointLight:
		Result = m_PSPointLight.get();
		break;
	case EBaseShader::PSPointLight_Volume:
		Result = m_PSPointLight_Volume.get();
		break;
	case EBaseShader::PSSpotLight:
		Result = m_PSSpotLight.get();
		break;
	case EBaseShader::PSSpotLight_Volume:
		Result = m_PSSpotLight_Volume.get();
		break;
	case EBaseShader::PSBase2D:
		Result = m_PSBase2D.get();
		break;
	case EBaseShader::PSBase2D_RawVertexColor:
		Result = m_PSBase2D_RawVertexColor.get();
		break;
	case EBaseShader::PSMasking2D:
		Result = m_PSMasking2D.get();
		break;
	case EBaseShader::PSHeightMap2D:
		Result = m_PSHeightMap2D.get();
		break;
	default:
		assert(Result);
		break;
	}

	return Result;
}

bool CGame::IsObject3DNameInsertable(const std::string& Name, bool bShowWarning)
{
	if (m_mapObject3DNameToIndex.find(Name) != m_mapObject3DNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Object3D 생성 실패");
		return false;
	}

	if (Name.size() >= KAssetNameMaxLength)
	{
		if (bShowWarning) MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Object3D 생성 실패");
		return false;
	}

	if (Name.size() == 0)
	{
		if (bShowWarning) MB_WARN("이름은 공백일 수 없습니다.", "Object3D 생성 실패");
		return false;
	}

	return true;
}

bool CGame::InsertObject3D(const string& Name)
{
	if (IsObject3DNameInsertable(Name, true))
	{
		m_vObject3Ds.emplace_back(make_unique<CObject3D>(Name, m_Device.Get(), m_DeviceContext.Get()));
		m_mapObject3DNameToIndex[Name] = m_vObject3Ds.size() - 1;

		return true;
	}
	return false;
}

bool CGame::ChangeObject3DName(const std::string& OldName, const std::string& NewName)
{
	if (m_mapObject3DNameToIndex.find(OldName) == m_mapObject3DNameToIndex.end())
	{
		MB_WARN(("기존 이름 (" + OldName + ")의 오브젝트가 존재하지 않습니다.").c_str(), "이름 변경 실패");
		return false;
	}
	if (m_mapObject3DNameToIndex.find(NewName) != m_mapObject3DNameToIndex.end())
	{
		MB_WARN(("새 이름 (" + NewName + ")의 오브젝트가 이미 존재합니다.").c_str(), "이름 변경 실패");
		return false;
	}

	string SavedOldName{ OldName };
	GetObject3D(SavedOldName)->SetName(NewName);
	size_t iObject3D{ m_mapObject3DNameToIndex.at(SavedOldName) };
	m_mapObject3DNameToIndex.erase(SavedOldName);
	m_mapObject3DNameToIndex[NewName] = iObject3D;

	return true;
}

void CGame::DeleteObject3D(const string& Name)
{
	if (!m_vObject3Ds.size()) return;
	if (Name.length() == 0) return;
	if (m_mapObject3DNameToIndex.find(Name) == m_mapObject3DNameToIndex.end())
	{
		MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Object3D 삭제 실패");
		return;
	}

	// @important
	{
		CObject3D* const Object3D{ GetObject3D(Name) };
		m_PhysicsEngine.DeregisterObject(Object3D);
	}

	size_t iObject3D{ m_mapObject3DNameToIndex.at(Name) };
	if (iObject3D < m_vObject3Ds.size() - 1)
	{
		const string& SwappedName{ m_vObject3Ds.back()->GetName() };
		swap(m_vObject3Ds[iObject3D], m_vObject3Ds.back());

		m_mapObject3DNameToIndex[SwappedName] = iObject3D;
	}

	string SavedName{ Name };
	m_vObject3Ds.pop_back();
	m_mapObject3DNameToIndex.erase(SavedName);
}

void CGame::ClearObject3Ds()
{
	m_mapObject3DNameToIndex.clear();
	m_vObject3Ds.clear();
	DeselectType(EObjectType::Object3D);
}

CObject3D* CGame::GetObject3D(const string& Name, bool bShowWarning) const
{
	if (m_mapObject3DNameToIndex.find(Name) == m_mapObject3DNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Object3D 얻어오기 실패");
		return nullptr;
	}
	return m_vObject3Ds[m_mapObject3DNameToIndex.at(Name)].get();
}

bool CGame::InsertObject3DLine(const string& Name, bool bShowWarning)
{
	if (m_mapObject3DLineNameToIndex.find(Name) != m_mapObject3DLineNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Object3DLine 생성 실패");
		return false;
	}

	if (Name.size() >= KAssetNameMaxLength)
	{
		if (bShowWarning) MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Object3DLine 생성 실패");
		return false;
	}
	else if (Name.size() == 0)
	{
		if (bShowWarning) MB_WARN("이름은 공백일 수 없습니다.", "Object3DLine 생성 실패");
		return false;
	}

	m_vObject3DLines.emplace_back(make_unique<CObject3DLine>(Name, m_Device.Get(), m_DeviceContext.Get()));
	m_mapObject3DLineNameToIndex[Name] = m_vObject3DLines.size() - 1;

	return true;
}

void CGame::ClearObject3DLines()
{
	m_mapObject3DLineNameToIndex.clear();
	m_vObject3DLines.clear();
	DeselectType(EObjectType::Object3DLine);
}

CObject3DLine* CGame::GetObject3DLine(const string& Name, bool bShowWarning) const
{
	if (m_mapObject3DLineNameToIndex.find(Name) == m_mapObject3DLineNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Object3DLine 얻어오기 실패");
		return nullptr;
	}
	return m_vObject3DLines[m_mapObject3DLineNameToIndex.at(Name)].get();
}

bool CGame::InsertObject2D(const string& Name)
{
	if (m_mapObject2DNameToIndex.find(Name) != m_mapObject2DNameToIndex.end())
	{
		MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Object2D 생성 실패");
		return false;
	}

	if (Name.size() >= KAssetNameMaxLength)
	{
		MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Object2D 생성 실패");
		return false;
	}
	else if (Name.size() == 0)
	{
		MB_WARN("이름은 공백일 수 없습니다.", "Object2D 생성 실패");
		return false;
	}

	m_vObject2Ds.emplace_back(make_unique<CObject2D>(Name, m_Device.Get(), m_DeviceContext.Get()));
	m_mapObject2DNameToIndex[Name] = m_vObject2Ds.size() - 1;

	return true;
}

void CGame::DeleteObject2D(const std::string& Name)
{
	if (!m_vObject2Ds.size()) return;
	if (Name.length() == 0) return;
	if (m_mapObject2DNameToIndex.find(Name) == m_mapObject2DNameToIndex.end())
	{
		MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Object2D 삭제 실패");
		return;
	}

	size_t iObject2D{ m_mapObject2DNameToIndex[Name] };
	if (iObject2D < m_vObject2Ds.size() - 1)
	{
		const string& SwappedName{ m_vObject2Ds.back()->GetName() };
		swap(m_vObject2Ds[iObject2D], m_vObject2Ds.back());

		m_mapObject2DNameToIndex[SwappedName] = iObject2D;
	}

	string SavedName{ Name };
	m_vObject2Ds.pop_back();
	m_mapObject2DNameToIndex.erase(SavedName);
}

void CGame::ClearObject2Ds()
{
	m_mapObject2DNameToIndex.clear();
	m_vObject2Ds.clear();
	DeselectType(EObjectType::Object2D);
}

CObject2D* CGame::GetObject2D(const string& Name, bool bShowWarning) const
{
	if (m_mapObject2DNameToIndex.find(Name) == m_mapObject2DNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Object2D 얻어오기 실패");
		return nullptr;
	}
	return m_vObject2Ds[m_mapObject2DNameToIndex.at(Name)].get();
}

void CGame::WalkPlayerToPickedPoint(float WalkSpeed)
{
	if (GetMode() == EMode::Edit) return;

	if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyWindowFocused() && !ImGui::IsAnyWindowHovered())
	{
		if (m_bLeftButtonUpOnce)
		{
			CastPickingRay();

			if (m_PhysicsEngine.PickObject(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection))
			{
				CObject3D* const PickedObject{ m_PhysicsEngine.GetPickedObject() };
				EObjectRole eObjectRole{ m_PhysicsEngine.GetObjectRole(PickedObject) };
				if (eObjectRole == EObjectRole::Environment)
				{
					CObject3D* const PlayerObject{ m_PhysicsEngine.GetPlayerObject() };

					bool bShouldWalk{ true };
					if (m_Intelligence->HasBehavior(PlayerObject))
					{
						EBehaviorType eBehaviorType{ m_Intelligence->PeekFrontBehavior(PlayerObject).eBehaviorType };
						if (eBehaviorType == EBehaviorType::Jump)
						{
							bShouldWalk = false;
						}
						else if (eBehaviorType == EBehaviorType::WalkTo)
						{
							m_Intelligence->PopFrontBehavior(PlayerObject);
						}
					}

					if (bShouldWalk)
					{
						XMVECTOR DestinationXZ{ XMVectorSetY(m_PhysicsEngine.GetPickedPoint(), 0) };

						SBehaviorData BehaviorData{};
						BehaviorData.eBehaviorType = EBehaviorType::WalkTo;
						BehaviorData.Vector = DestinationXZ;
						BehaviorData.PrevTranslation = PlayerObject->GetTransform().Translation;
						BehaviorData.Scalar = WalkSpeed;
						BehaviorData.bIsPlayer = true;
						BehaviorData.StartTime_ms = m_TimeNow_ms;
						m_Intelligence->PushBackBehavior(PlayerObject, BehaviorData);
					}
				}
				else if (eObjectRole == EObjectRole::Monster)
				{

				}
			}
		}
	}
}

void CGame::JumpPlayer(float JumpSpeed)
{
	if (GetMode() == EMode::Edit) return;

	CObject3D* const PlayerObject{ m_PhysicsEngine.GetPlayerObject() };
	if (m_Intelligence->HasBehavior(PlayerObject))
	{
		const auto& CurrentBehavior{ m_Intelligence->PeekFrontBehavior(PlayerObject) };
		if (CurrentBehavior.eBehaviorType == EBehaviorType::Jump) return;

		m_Intelligence->ClearBehavior(PlayerObject);
	}

	SBehaviorData BehaviorData{};
	BehaviorData.eBehaviorType = EBehaviorType::Jump;
	BehaviorData.Scalar = JumpSpeed;
	BehaviorData.bIsPlayer = true;
	BehaviorData.StartTime_ms = m_TimeNow_ms;
	m_Intelligence->PushFrontBehavior(PlayerObject, BehaviorData);
}

bool CGame::InsertLight(CLight::EType eType, const std::string& Name)
{
	string InstanceName{ Name };
	
	while ((m_umapLightNames.find(InstanceName) != m_umapLightNames.end()) || InstanceName.empty())
	{
		InstanceName = "light" + to_string(m_LightCreationCounter);
		++m_LightCreationCounter;
	}
	m_umapLightNames[InstanceName] = 0xE;

	if (m_LightArray[(uint32_t)eType]->InsertInstance(InstanceName))
	{
		m_LightRep->InsertInstance(InstanceName);
		return true;
	}
	return false;
}

void CGame::DeleteLight(CLight::EType eType, const std::string& Name)
{
	if (m_LightArray[(uint32_t)eType]->DeleteInstance(Name))
	{
		m_umapLightNames.erase(Name);

		m_LightRep->DeleteInstance(Name);
	}
}

void CGame::ClearLights()
{
	m_umapLightNames.clear();
	m_LightCreationCounter = 0;

	for (auto& Light : m_LightArray)
	{
		Light->ClearInstances();
	}
	m_LightRep->ClearInstances();
}

bool CGame::InsertPattern(const std::string& FileName)
{
	if (m_umapPatternFileNameToIndex.find(FileName) != m_umapPatternFileNameToIndex.end()) return false;

	m_vPatterns.emplace_back(make_unique<CPattern>());
	m_vPatterns.back()->Load(FileName.c_str());
	m_umapPatternFileNameToIndex[FileName] = m_vPatterns.size() - 1;

	return false;
}

void CGame::DeletePattern(const std::string& FileName)
{
	if (m_umapPatternFileNameToIndex.find(FileName) != m_umapPatternFileNameToIndex.end())
	{
		size_t At{ m_umapPatternFileNameToIndex.at(FileName) };
		if (At < m_vPatterns.size() - 1)
		{
			swap(m_umapPatternFileNameToIndex[m_vPatterns.back()->GetFileName()], m_umapPatternFileNameToIndex[FileName]);
			swap(m_vPatterns[At], m_vPatterns.back());
		}

		m_vPatterns.pop_back();
		m_umapPatternFileNameToIndex.erase(FileName);
	}
}

void CGame::ClearPatterns()
{
	m_vPatterns.clear();
	m_umapPatternFileNameToIndex.clear();
}

CPattern* CGame::GetPattern(const std::string& FileName)
{
	if (m_umapPatternFileNameToIndex.find(FileName) != m_umapPatternFileNameToIndex.end())
	{
		return m_vPatterns[m_umapPatternFileNameToIndex.at(FileName)].get();
	}
	return nullptr;
}

bool CGame::SetMode(EMode eMode)
{
	if (eMode != EMode::Edit)
	{
		if (!GetPlayerCamera())
		{
			MB_WARN("먼저 플레이어 카메라를 지정해 주세요.", "플레이어 카메라 미지정");
			return false;
		}

		if (!m_PhysicsEngine.GetPlayerObject())
		{
			MB_WARN("먼저 플레이어 오브젝트를 지정해 주세요.", "플레이어 오브젝트 미지정");
			return false;
		}
		
		DeselectAll();

		m_PhysicsEngine.ShouldApplyGravity(true);
	}
	else
	{
		m_PhysicsEngine.ShouldApplyGravity(false);
	}

	m_eMode = eMode;

	if (m_eMode == EMode::Edit)
	{
		SetEditMode(GetEditMode(), true);
	}
	else
	{
		if (m_Terrain) m_Terrain->ShouldShowSelection(FALSE);

		DeselectAll();
	}

	return true;
}

void CGame::SetEditMode(EEditMode eEditMode, bool bForcedSet)
{
	if (!bForcedSet)
	{
		if (m_eEditMode == eEditMode) return;
	}

	m_eEditMode = eEditMode;
	if (m_eEditMode == EEditMode::EditTerrain)
	{
		if (m_Terrain) m_Terrain->ShouldShowSelection(TRUE);
		
		DeselectAll();
	}
	else
	{
		if (m_Terrain) m_Terrain->ShouldShowSelection(FALSE);
	}
}

void CGame::NotifyMouseLeftDown()
{
	m_bLeftButtonPressedOnce = true;
}

void CGame::NotifyMouseLeftUp()
{
	m_bLeftButtonUpOnce = true;
}

void CGame::Select()
{
	if (m_Gizmo3D->IsInAction()) return;

	if (m_eMode == EMode::Edit)
	{	
		ESelectionMode eSelectionMode{ ESelectionMode::Override };
		if (m_CapturedKeyboardState.LeftShift || m_CapturedKeyboardState.RightShift) eSelectionMode = ESelectionMode::Additive;
		else if (m_CapturedKeyboardState.LeftAlt || m_CapturedKeyboardState.RightAlt) eSelectionMode = ESelectionMode::Subtractive;

		// Multiple region-selection
		SelectMultipleObjects(eSelectionMode);

		if (eSelectionMode != ESelectionMode::Override || !IsAnythingSelected())
		{
			// Single ray-selection

			CastPickingRay();

			PickBoundingSphere(eSelectionMode);
			PickObject3DTriangle(eSelectionMode);
		}

		// @important
		{
			m_CameraRep->SetAllInstancesHighlightOff();
			if (IsAnythingSelected())
			{
				for (const auto& SelectionData : m_vSelectionData)
				{
					if (SelectionData.eObjectType == EObjectType::Camera)
					{
						m_CameraRep->SetInstanceHighlight(SelectionData.Name, true);
					}
				}
			}
			
		}
	}
	else
	{
		// TODO

		CastPickingRay();

		PickBoundingSphere(ESelectionMode::Override);
	}
}

void CGame::SelectMultipleObjects(ESelectionMode eSelectionMode)
{
	const XMMATRIX KViewProjection{ m_MatrixView * m_MatrixProjection };
	if (eSelectionMode == ESelectionMode::Override) DeselectAll();

	ESelectionMode eInternalSelectionMode{ (eSelectionMode == ESelectionMode::Subtractive) ? ESelectionMode::Subtractive : ESelectionMode::Additive };
	for (const auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->IsInstanced())
		{
			const auto& vInstanceCPUData{ Object3D->GetInstanceCPUDataVector() };
			for (const auto& Instance : vInstanceCPUData)
			{
				const XMVECTOR KTranslation{ Instance.Transform.Translation + Instance.EditorBoundingSphere.Center };
				const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(KTranslation, KViewProjection) };
				const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };

				if (IsInsideSelectionRegion(KProjectionXY))
				{
					SSelectionData SelectionData{ EObjectType::Object3DInstance, Instance.Name, Object3D.get() };
					SelectObject(SelectionData, eInternalSelectionMode);
				}
			}
		}
		else
		{
			const XMVECTOR KTranslation{ Object3D->GetTransform().Translation + Object3D->GetOuterBoundingSphereCenterOffset() };
			const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(KTranslation, KViewProjection) };
			const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };

			if (IsInsideSelectionRegion(KProjectionXY))
			{
				SSelectionData SelectionData{ EObjectType::Object3D, Object3D->GetName(), Object3D.get() };
				SelectObject(SelectionData, eInternalSelectionMode);
			}
		}
	}

	size_t iCamera{};
	for (auto& Camera : m_vCameras)
	{
		const string& KCameraName{ Camera->GetName() };
		const XMVECTOR& KWorldPosition{ Camera->GetEyePosition() };
		const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(KWorldPosition, KViewProjection) };
		const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };

		if (IsInsideSelectionRegion(KProjectionXY))
		{
			SSelectionData SelectionData{ EObjectType::Camera, KCameraName };
			SelectObject(SelectionData, eInternalSelectionMode);
		}
		++iCamera;
	}
	m_CameraRep->UpdateAllInstances();

	for (const auto& Light : m_LightArray)
	{
		CLight::EType eType{ Light->GetType() };

		for (const auto& LightPair : Light->GetInstanceNameToIndexMap())
		{
			const XMVECTOR& KWorldPosition{ Light->GetInstanceGPUData(LightPair.first).Position };
			const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(KWorldPosition, KViewProjection) };
			const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };

			if (IsInsideSelectionRegion(KProjectionXY))
			{
				SSelectionData SelectionData{ EObjectType::Light, LightPair.first, (uint32_t)eType };
				SelectObject(SelectionData, eInternalSelectionMode);
			}
		}
	}
	m_LightRep->UpdateInstanceBuffer();
}

void CGame::SelectObject(const SSelectionData& SelectionData, ESelectionMode eSelectionMode)
{
	if (eSelectionMode == ESelectionMode::Override) DeselectAll();
	if (eSelectionMode == ESelectionMode::Subtractive)
	{
		DeselectObject(SelectionData);
		return;
	}

	switch (SelectionData.eObjectType)
	{
	case CGame::EObjectType::NONE:
	default:
		break;
	case CGame::EObjectType::Object3D:
		if (m_umapSelectionObject3D.find(SelectionData.Name) != m_umapSelectionObject3D.end()) return;
		break;
	case CGame::EObjectType::Object3DInstance:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		if (m_umapSelectionObject3D.find(Object3D->GetName() + SelectionData.Name) != m_umapSelectionObject3D.end()) return;
		break;
	}
	case CGame::EObjectType::Object3DLine:
		if (m_umapSelectionObject3DLine.find(SelectionData.Name) != m_umapSelectionObject3DLine.end()) return;
		break;
	case CGame::EObjectType::Object2D:
		if (m_umapSelectionObject2D.find(SelectionData.Name) != m_umapSelectionObject2D.end()) return;
		break;
	case CGame::EObjectType::EditorCamera:
		//if (m_EditorCameraSelectionIndex < m_vSelectionData.size()) return;
	case CGame::EObjectType::Camera:
		if (m_umapSelectionCamera.find(SelectionData.Name) != m_umapSelectionCamera.end()) return;
		break;
	case CGame::EObjectType::Light:
		if (m_umapSelectionLight.find(SelectionData.Name) != m_umapSelectionLight.end()) return;
		break;
	}

	m_vSelectionData.emplace_back(SelectionData);
	const size_t KSelectionIndex{ m_vSelectionData.size() - 1 };
	XMVECTOR MultipleSelectionWorldCenter{};
	if (KSelectionIndex > 0)
	{
		m_MultipleSelectionWorldCenter /= XMVectorGetW(m_MultipleSelectionWorldCenter); // @important
		MultipleSelectionWorldCenter = m_MultipleSelectionWorldCenter * static_cast<float>(KSelectionIndex);
	}

	switch (SelectionData.eObjectType)
	{
	default:
	case CGame::EObjectType::NONE:
	case CGame::EObjectType::Object3DLine:
		return;

	case CGame::EObjectType::Object3D:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		const XMVECTOR KTranslation{ Object3D->GetTransform().Translation + Object3D->GetOuterBoundingSphereCenterOffset() };
		
		m_umapSelectionObject3D[SelectionData.Name] = KSelectionIndex;

		MultipleSelectionWorldCenter += XMVectorSetW(KTranslation, 1);
		break;
	}
	case CGame::EObjectType::Object3DInstance:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		const XMVECTOR KTranslation{ 
			Object3D->GetInstanceTransform(SelectionData.Name).Translation + Object3D->GetInstanceOuterBoundingSphere(SelectionData.Name).Center };

		m_umapSelectionObject3D[Object3D->GetName() + SelectionData.Name] = KSelectionIndex;

		MultipleSelectionWorldCenter += XMVectorSetW(KTranslation, 1);
		break;
	}
	case CGame::EObjectType::Object2D:
		break;
	case CGame::EObjectType::EditorCamera:
		m_EditorCameraSelectionIndex = KSelectionIndex;

		MultipleSelectionWorldCenter += XMVectorSetW(m_EditorCamera->GetTranslation(), 1);
		break;
	case CGame::EObjectType::Camera:
		m_umapSelectionCamera[SelectionData.Name] = KSelectionIndex;

		m_CameraRep->SetInstanceHighlight(SelectionData.Name, true);
		m_CameraRep->UpdateAllInstances(false);

		MultipleSelectionWorldCenter += XMVectorSetW(m_vCameras[GetCameraID(SelectionData.Name)]->GetTranslation(), 1);
		break;
	case CGame::EObjectType::Light:
		m_umapSelectionLight[SelectionData.Name] = KSelectionIndex;

		m_LightRep->SetInstanceHighlight(SelectionData.Name, true);
		m_LightRep->UpdateInstanceBuffer();

		MultipleSelectionWorldCenter += XMVectorSetW(m_LightArray[SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name).Position, 1);
		break;
	}
	m_MultipleSelectionWorldCenter = MultipleSelectionWorldCenter / static_cast<float>(m_vSelectionData.size());

	Capture3DGizmoTranslation();
}

std::string CGame::GetSelectionName(const SSelectionData& SelectionData) const
{
	if (SelectionData.eObjectType == EObjectType::Object3DInstance)
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		return Object3D->GetName() + SelectionData.Name;
	}
	return SelectionData.Name;
}

void CGame::DeselectObject(const SSelectionData& SelectionData)
{
	const size_t KSelectionCount{ m_vSelectionData.size() };
	m_MultipleSelectionWorldCenter /= XMVectorGetW(m_MultipleSelectionWorldCenter); // @important
	XMVECTOR MultipleSelectionWorldCenter{ m_MultipleSelectionWorldCenter * static_cast<float>(KSelectionCount) };

	switch (SelectionData.eObjectType)
	{
	case CGame::EObjectType::NONE:
	default:
		break;
	case CGame::EObjectType::Object3D:
		if (m_umapSelectionObject3D.find(SelectionData.Name) != m_umapSelectionObject3D.end())
		{
			CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
			const XMVECTOR KTranslation{ Object3D->GetTransform().Translation + Object3D->GetOuterBoundingSphereCenterOffset() };
			MultipleSelectionWorldCenter -= XMVectorSetW(KTranslation, 1);

			size_t iSelectionData{ m_umapSelectionObject3D.at(SelectionData.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				string BackSelectionName{ GetSelectionName(m_vSelectionData.back()) };
				swap(m_umapSelectionObject3D.at(BackSelectionName), m_umapSelectionObject3D.at(SelectionData.Name)); // @important

				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject3D.erase(SelectionData.Name);
			m_vSelectionData.pop_back();
		}
		break;
	case CGame::EObjectType::Object3DInstance:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		string SelectionName{ GetSelectionName(SelectionData) };
		if (m_umapSelectionObject3D.find(SelectionName) != m_umapSelectionObject3D.end())
		{
			const XMVECTOR KTranslation{ 
				Object3D->GetInstanceTransform(SelectionData.Name).Translation + Object3D->GetInstanceOuterBoundingSphere(SelectionData.Name).Center };
			MultipleSelectionWorldCenter -= XMVectorSetW(KTranslation, 1);

			size_t iSelectionData{ m_umapSelectionObject3D.at(SelectionName) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				string BackSelectionName{ GetSelectionName(m_vSelectionData.back()) };
				swap(m_umapSelectionObject3D.at(BackSelectionName), m_umapSelectionObject3D.at(SelectionName)); // @important

				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject3D.erase(SelectionName);
			m_vSelectionData.pop_back();
		}
		break;
	}
	case CGame::EObjectType::Object3DLine:
		if (m_umapSelectionObject3DLine.find(SelectionData.Name) != m_umapSelectionObject3DLine.end())
		{
			size_t iSelectionData{ m_umapSelectionObject3DLine.at(SelectionData.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				string BackSelectionName{ GetSelectionName(m_vSelectionData.back()) };
				swap(m_umapSelectionObject3DLine.at(BackSelectionName), m_umapSelectionObject3DLine.at(SelectionData.Name)); // @important

				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject3DLine.erase(SelectionData.Name);
			m_vSelectionData.pop_back();
		}
		break;
	case CGame::EObjectType::Object2D:
		if (m_umapSelectionObject2D.find(SelectionData.Name) != m_umapSelectionObject2D.end())
		{
			size_t iSelectionData{ m_umapSelectionObject2D.at(SelectionData.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				string BackSelectionName{ GetSelectionName(m_vSelectionData.back()) };
				swap(m_umapSelectionObject2D.at(BackSelectionName), m_umapSelectionObject2D.at(SelectionData.Name)); // @important

				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject2D.erase(SelectionData.Name);
			m_vSelectionData.pop_back();
		}
		break;
	case CGame::EObjectType::EditorCamera:
		if (m_EditorCameraSelectionIndex < m_vSelectionData.size())
		{
			if (m_EditorCameraSelectionIndex < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[m_EditorCameraSelectionIndex], m_vSelectionData.back());
			}
			m_EditorCameraSelectionIndex = KInvalidIndex;
			m_vSelectionData.pop_back();
		}
		break;
	case CGame::EObjectType::Camera:
		if (m_umapSelectionCamera.find(SelectionData.Name) != m_umapSelectionCamera.end())
		{
			MultipleSelectionWorldCenter -= XMVectorSetW(m_vCameras[GetCameraID(SelectionData.Name)]->GetTranslation(), 1);

			size_t iSelectionData{ m_umapSelectionCamera.at(SelectionData.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				string BackSelectionName{ GetSelectionName(m_vSelectionData.back()) };
				swap(m_umapSelectionCamera.at(BackSelectionName), m_umapSelectionCamera.at(SelectionData.Name)); // @important

				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionCamera.erase(SelectionData.Name);
			m_vSelectionData.pop_back();

			m_CameraRep->SetInstanceHighlight(SelectionData.Name, false);
			m_CameraRep->UpdateAllInstances(false);
		}
		break;
	case CGame::EObjectType::Light:
		if (m_umapSelectionLight.find(SelectionData.Name) != m_umapSelectionLight.end())
		{
			MultipleSelectionWorldCenter -= XMVectorSetW(m_LightArray[SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name).Position, 1);

			size_t iSelectionData{ m_umapSelectionLight.at(SelectionData.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				string BackSelectionName{ GetSelectionName(m_vSelectionData.back()) };
				swap(m_umapSelectionLight.at(BackSelectionName), m_umapSelectionLight.at(SelectionData.Name)); // @important

				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionLight.erase(SelectionData.Name);
			m_vSelectionData.pop_back();

			m_LightRep->SetInstanceHighlight(SelectionData.Name, false);
			m_LightRep->UpdateInstanceBuffer();
		}
		break;
	}

	if (m_vSelectionData.size())
	{
		m_MultipleSelectionWorldCenter = MultipleSelectionWorldCenter / static_cast<float>(m_vSelectionData.size());
	}
	else
	{
		m_MultipleSelectionWorldCenter = KVectorZero;
	}

	Capture3DGizmoTranslation();
}

void CGame::DeselectType(EObjectType eObjectType)
{
	vector<SSelectionData> vNewSelectionData{};
	vNewSelectionData.reserve(m_vSelectionData.size());
	for (const auto& SelectionData : m_vSelectionData)
	{
		if (SelectionData.eObjectType != eObjectType) vNewSelectionData.emplace_back(SelectionData);
	}
	m_vSelectionData = vNewSelectionData;

	// @important
	switch (eObjectType)
	{
	default:
	case CGame::EObjectType::NONE:
		break;
	case CGame::EObjectType::Object3D:
	case CGame::EObjectType::Object3DInstance:
		m_umapSelectionObject3D.clear();
		break;
	case CGame::EObjectType::Object3DLine:
		m_umapSelectionObject3DLine.clear();
		break;
	case CGame::EObjectType::Object2D:
		m_umapSelectionObject2D.clear();
		break;
	case CGame::EObjectType::EditorCamera:
		m_EditorCameraSelectionIndex = KInvalidIndex;
		break;
	case CGame::EObjectType::Camera:
		m_umapSelectionCamera.clear();
		m_CameraRep->SetAllInstancesHighlightOff();
		break;
	case CGame::EObjectType::Light:
		m_umapSelectionLight.clear();
		m_LightRep->SetAllInstancesHighlightOff();
		m_LightRep->UpdateInstanceBuffer();
		break;
	}
}

void CGame::DeselectAll()
{
	m_vSelectionData.clear();

	m_MultipleSelectionWorldCenter = XMVectorSet(0, 0, 0, 0); // @important

	m_umapSelectionObject3D.clear();
	m_umapSelectionObject3DLine.clear();
	m_umapSelectionObject2D.clear();
	m_EditorCameraSelectionIndex = KInvalidIndex;
	m_umapSelectionCamera.clear();
	m_umapSelectionLight.clear();

	m_CameraRep->SetAllInstancesHighlightOff();

	m_LightRep->SetAllInstancesHighlightOff();
	m_LightRep->UpdateInstanceBuffer();
}

bool CGame::IsAnythingSelected() const
{
	return !(m_vSelectionData.empty());
}

void CGame::TranslateSelectionTo(EAxis eAxis, float Prime)
{
	for (auto& SelectionData : m_vSelectionData)
	{
		switch (SelectionData.eObjectType)
		{
		case CGame::EObjectType::NONE:
		case CGame::EObjectType::EditorCamera:
		case CGame::EObjectType::Object3DLine:
		case CGame::EObjectType::Object2D:
			break;
		case CGame::EObjectType::Object3D:
		{
			CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
			Object3D->TranslateTo(
				(eAxis == EAxis::X) ? XMVectorSetX(Object3D->GetTransform().Translation, Prime) :
				(eAxis == EAxis::Y) ? XMVectorSetY(Object3D->GetTransform().Translation, Prime) :
				XMVectorSetZ(Object3D->GetTransform().Translation, Prime));
			Object3D->UpdateWorldMatrix();
			break;
		}
		case CGame::EObjectType::Object3DInstance:
		{
			CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
			Object3D->TranslateInstanceTo(SelectionData.Name,
				(eAxis == EAxis::X) ? XMVectorSetX(Object3D->GetInstanceTransform(SelectionData.Name).Translation, Prime) :
				(eAxis == EAxis::Y) ? XMVectorSetY(Object3D->GetInstanceTransform(SelectionData.Name).Translation, Prime) :
				XMVectorSetZ(Object3D->GetInstanceTransform(SelectionData.Name).Translation, Prime));
			Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
			break;
		}
		case CGame::EObjectType::Camera:
			GetCamera(SelectionData.Name)->TranslateTo(
				(eAxis == EAxis::X) ? XMVectorSetX(GetCamera(SelectionData.Name)->GetTranslation(), Prime) :
				(eAxis == EAxis::Y) ? XMVectorSetY(GetCamera(SelectionData.Name)->GetTranslation(), Prime) :
				XMVectorSetZ(GetCamera(SelectionData.Name)->GetTranslation(), Prime)
			);
			m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name);
			break;
		case CGame::EObjectType::Light:
		{
			auto InstanceGPUData{ m_LightArray[SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name) };
			InstanceGPUData.Position =
				(eAxis == EAxis::X) ? XMVectorSetX(InstanceGPUData.Position, Prime) :
				(eAxis == EAxis::Y) ? XMVectorSetY(InstanceGPUData.Position, Prime) :
				XMVectorSetZ(InstanceGPUData.Position, Prime);
			m_LightArray[SelectionData.Extra]->SetInstanceGPUData(SelectionData.Name, InstanceGPUData);
			m_LightRep->SetInstancePosition(SelectionData.Name, InstanceGPUData.Position);
			break;
		}
		default:
			break;
		}
	}

	Capture3DGizmoTranslation();
}

void CGame::CastPickingRay()
{
	float NormalizedX{ m_CapturedMouseState.x / (m_WindowSize.x / 2.0f) - 1.0f };
	float NormalizedY{ -(m_CapturedMouseState.y / (m_WindowSize.y / 2.0f) - 1.0f) };

	float ViewSpaceRayDirectionX{ NormalizedX / XMVectorGetX(m_MatrixProjection.r[0]) };
	float ViewSpaceRayDirectionY{ NormalizedY / XMVectorGetY(m_MatrixProjection.r[1]) };
	static float ViewSpaceRayDirectionZ{ 1.0f };

	static XMVECTOR ViewSpaceRayOrigin{ XMVectorSet(0, 0, 0, 1) };
	XMVECTOR ViewSpaceRayDirection{ XMVectorSet(ViewSpaceRayDirectionX, ViewSpaceRayDirectionY, ViewSpaceRayDirectionZ, 0) };

	XMMATRIX MatrixViewInverse{ XMMatrixInverse(nullptr, m_MatrixView) };
	m_PickingRayWorldSpaceOrigin = XMVector3TransformCoord(ViewSpaceRayOrigin, MatrixViewInverse);
	m_PickingRayWorldSpaceDirection = XMVector3TransformNormal(ViewSpaceRayDirection, MatrixViewInverse);

	if (m_PickingRayRep)
	{
		m_PickingRayRep->GetVertices().at(0).Position = m_PickingRayWorldSpaceOrigin;
		m_PickingRayRep->GetVertices().at(1).Position = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * KPickingRayLength;
		m_PickingRayRep->UpdateVertexBuffer();
	}	
}

void CGame::PickBoundingSphere(ESelectionMode eSelectionMode)
{
	bool bIsSelectionAdded{ false };

	m_vObject3DPickingCandidates.clear();

	// Pick Camera
	for (int iCamera = 0; iCamera < m_vCameras.size(); ++iCamera)
	{
		CCamera* const Camera{ m_vCameras[iCamera].get() };

		// Must not select current camera!
		if (m_CurrentCameraID != iCamera)
		{
			XMVECTOR NewT{ KVectorGreatest };
			if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
				KCameraRepSelectionRadius, Camera->GetEyePosition(), &NewT))
			{
				SelectObject(SSelectionData(EObjectType::Camera, Camera->GetName()), eSelectionMode);
				bIsSelectionAdded = true;
				break;
			}
		}
	}
	if (bIsSelectionAdded) return;

	// Pick Light
	for (const auto& Light : m_LightArray)
	{
		CLight::EType eType{ Light->GetType() };
		for (const auto& LightPair : Light->GetInstanceNameToIndexMap())
		{
			const auto& Instance{ Light->GetInstanceGPUData(LightPair.first) };

			XMVECTOR NewT{ KVectorGreatest };
			if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
				Light->GetBoundingSphereRadius(), Instance.Position, &NewT))
			{
				SelectObject(SSelectionData(EObjectType::Light, LightPair.first, (uint32_t)eType), eSelectionMode);
				bIsSelectionAdded = true;
				break;
			}
		}
	}
	if (bIsSelectionAdded) return;

	// Pick Object3D
	XMVECTOR T{ KVectorGreatest };
	for (const auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->IsPickable())
		{
			if (Object3D->IsInstanced())
			{
				for (const auto& InstanceCPUData : Object3D->GetInstanceCPUDataVector())
				{
					XMVECTOR NewT{ KVectorGreatest };
					if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
						InstanceCPUData.EditorBoundingSphere.Data.BS.Radius, 
						InstanceCPUData.Transform.Translation + InstanceCPUData.EditorBoundingSphere.Center, &NewT))
					{
						m_vObject3DPickingCandidates.emplace_back(Object3D.get(), InstanceCPUData.Name, NewT);
					}
				}
			}
			else
			{
				XMVECTOR NewT{ KVectorGreatest };
				if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
					Object3D->GetOuterBoundingSphereRadius(), 
					Object3D->GetTransform().Translation + Object3D->GetOuterBoundingSphereCenterOffset(), &NewT))
				{
					m_vObject3DPickingCandidates.emplace_back(Object3D.get(), NewT);
				}
			}
		}
	}
}

bool CGame::PickObject3DTriangle(ESelectionMode eSelectionMode)
{
	XMVECTOR T{ KVectorGreatest };
	if (eSelectionMode != ESelectionMode::Override || !IsAnythingSelected())
	{
		for (SObject3DPickingCandiate& Candidate : m_vObject3DPickingCandidates)
		{
			// Pick only static models' triangle.
			// Rigged models should be selected by its bounding sphere, not triangles!
			if (Candidate.PtrObject3D->GetModel().bIsModelRigged)
			{
				Candidate.bHasFailedPickingTest = false;
				continue;
			}

			Candidate.bHasFailedPickingTest = true;
			XMMATRIX WorldMatrix{ Candidate.PtrObject3D->GetWorldMatrix() };
			if (!Candidate.InstanceName.empty())
			{
				WorldMatrix = Candidate.PtrObject3D->GetInstanceWorldMatrix(Candidate.InstanceName);
			}
			for (const SMesh& Mesh : Candidate.PtrObject3D->GetModel().vMeshes)
			{
				for (const STriangle& Triangle : Mesh.vTriangles)
				{
					XMVECTOR V0{ Mesh.vVertices[Triangle.I0].Position };
					XMVECTOR V1{ Mesh.vVertices[Triangle.I1].Position };
					XMVECTOR V2{ Mesh.vVertices[Triangle.I2].Position };
					V0 = XMVector3TransformCoord(V0, WorldMatrix);
					V1 = XMVector3TransformCoord(V1, WorldMatrix);
					V2 = XMVector3TransformCoord(V2, WorldMatrix);

					XMVECTOR NewT{};
					if (IntersectRayTriangle(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, V0, V1, V2, &NewT))
					{
						if (XMVector3Less(NewT, T))
						{
							T = NewT;

							Candidate.bHasFailedPickingTest = false;
							Candidate.T = NewT;

							XMVECTOR N{ CalculateTriangleNormal(V0, V1, V2) };

							m_PickedTriangleV0 = V0 + N * 0.01f;
							m_PickedTriangleV1 = V1 + N * 0.01f;
							m_PickedTriangleV2 = V2 + N * 0.01f;

							continue;
						}
					}
				}
			}
		}

		vector<SObject3DPickingCandiate> vFilteredCandidates{};
		for (const SObject3DPickingCandiate& Candidate : m_vObject3DPickingCandidates)
		{
			if (Candidate.bHasFailedPickingTest == false) vFilteredCandidates.emplace_back(Candidate);
		}

		XMVECTOR TCmp{ KVectorGreatest };
		const SObject3DPickingCandiate* Closest{};
		for (const SObject3DPickingCandiate& FilteredCandidate : vFilteredCandidates)
		{
			if (XMVector3Less(FilteredCandidate.T, TCmp))
			{
				TCmp = FilteredCandidate.T;
				Closest = &FilteredCandidate;
			}
		}
		if (Closest)
		{
			m_PickedPoint = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * TCmp;

			if (Closest->InstanceName.empty())
			{
				SelectObject(SSelectionData(EObjectType::Object3D, Closest->PtrObject3D->GetName(), Closest->PtrObject3D), eSelectionMode);
			}
			else
			{
				SelectObject(SSelectionData(EObjectType::Object3DInstance, Closest->InstanceName, Closest->PtrObject3D), eSelectionMode);
			}
		}
		return true;
	}
	return false;
}

CCamera* CGame::GetCurrentCamera()
{
	return m_PtrCurrentCamera;
}

void CGame::UseEditorCamera()
{
	m_PtrCurrentCamera = m_EditorCamera.get();
	m_CurrentCameraID = KEditorCameraID;
}

void CGame::UseCamera(CCamera* const Camera)
{
	if (IsEditorCamera(Camera))
	{
		UseEditorCamera();
		return;
	}

	m_PtrCurrentCamera = Camera;
	m_CurrentCameraID = (int)m_mapCameraNameToIndex.at(Camera->GetName());
}

void CGame::SetPlayerCamera(CCamera* const Camera)
{
	m_PtrPlayerCamera = Camera;
}

CCamera* CGame::GetPlayerCamera() const
{
	return m_PtrPlayerCamera;
}

bool CGame::IsEditorCamera(CCamera* const Camera) const
{
	return m_EditorCamera.get() == Camera;
}

bool CGame::IsCurrentCamera(CCamera* const Camera) const
{
	return m_PtrCurrentCamera == Camera;
}

bool CGame::IsPlayerCamera(CCamera* const Camera) const
{
	return m_PtrPlayerCamera == Camera;
}

void CGame::SelectTerrain(bool bShouldEdit, bool bIsLeftButton)
{
	if (!m_Terrain) return;
	if (m_eEditMode != EEditMode::EditTerrain) return;

	CastPickingRay();

	m_Terrain->Select(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, bShouldEdit, bIsLeftButton);
}

bool CGame::IsInsideSelectionRegion(const XMFLOAT2& ProjectionSpacePosition)
{
	// Check if out-of-screen
	if (ProjectionSpacePosition.x <= -1.0f || ProjectionSpacePosition.x >= 1.0f) return false;
	if (ProjectionSpacePosition.y <= -1.0f || ProjectionSpacePosition.y >= 1.0f) return false;

	// Check if in region
	if (ProjectionSpacePosition.x >= m_MultipleSelectionXYNormalized.x &&
		ProjectionSpacePosition.x <= m_MultipleSelectionXYPrimeNormalized.x &&
		ProjectionSpacePosition.y <= m_MultipleSelectionXYNormalized.y &&
		ProjectionSpacePosition.y >= m_MultipleSelectionXYPrimeNormalized.y)
	{
		return true;
	}

	return false;
}

void CGame::Capture3DGizmoTranslation()
{
	if (!IsAnythingSelected()) return;

	if (m_vSelectionData.size() == 1)
	{
		const auto& SelectionData{ m_vSelectionData.back() };
		{
			switch (SelectionData.eObjectType)
			{
			default:
			case CGame::EObjectType::NONE:
			case CGame::EObjectType::EditorCamera:
				break;
			case CGame::EObjectType::Object3DLine:
				break;
			case CGame::EObjectType::Object2D:
				break;
			case CGame::EObjectType::Camera:
			{
				CCamera* const Camera{ GetCamera(SelectionData.Name) };
				m_Gizmo3D->CaptureTranslation(Camera->GetTranslation());
				break;
			}
			case CGame::EObjectType::Light:
			{
				const XMVECTOR& KTranslation{ m_LightArray[(uint32_t)SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name).Position };
				m_Gizmo3D->CaptureTranslation(KTranslation);
				break;
			}
			case CGame::EObjectType::Object3D:
			case CGame::EObjectType::Object3DInstance:
			{
				CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
				if (SelectionData.eObjectType == EObjectType::Object3DInstance)
				{
					m_Gizmo3D->CaptureTranslation(Object3D->GetInstanceTransform(SelectionData.Name).Translation);
				}
				else
				{
					if (Object3D->IsInstanced())
					{
						XMVECTOR Center{};
						const auto& mapNameToIndex{ Object3D->GetInstanceNameToIndexMap() };
						for (const auto& InstancePair : mapNameToIndex)
						{
							const auto& InstanceTransform{ Object3D->GetInstanceTransform(InstancePair.first) };
							XMVECTOR InstanceTranslation{ InstanceTransform.Translation / XMVectorGetW(InstanceTransform.Translation) };
							Center += InstanceTranslation;
						}
						Center /= static_cast<float>(Object3D->GetInstanceCount());
						m_Gizmo3D->CaptureTranslation(Center);
					}
					else
					{
						m_Gizmo3D->CaptureTranslation(Object3D->GetTransform().Translation);
					}
				}

				break;
			}
			}
		}
	}
	else
	{
		m_Gizmo3D->CaptureTranslation(m_MultipleSelectionWorldCenter);
	}

	m_Gizmo3D->UpdateTranslation(m_PtrCurrentCamera->GetTranslation()); // @important
}

void CGame::Update3DGizmos()
{
	if (EFLAG_HAS_NO(m_eFlagsRendering, EFlagsRendering::Use3DGizmos)) return;
	if (m_eMode != EMode::Edit) return;
	
	if (!IsAnythingSelected()) return;

	CastPickingRay();

	if (m_Gizmo3D->Interact(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
		m_PtrCurrentCamera->GetTranslation(), m_PtrCurrentCamera->GetForward(), m_CapturedMouseState.x, m_CapturedMouseState.y, m_CapturedMouseState.leftButton))
	{
		for (const auto& SelectionData : m_vSelectionData)
		{
			switch (SelectionData.eObjectType)
			{
			default:
			case CGame::EObjectType::NONE:
			case CGame::EObjectType::EditorCamera:
				break;
			case CGame::EObjectType::Object3DLine:
				break;
			case CGame::EObjectType::Object2D:
				break;
			case CGame::EObjectType::Camera:
			{
				CCamera* const Camera{ GetCamera(SelectionData.Name) };
				Camera->Translate(m_Gizmo3D->GetDeltaTranslation());
				Camera->Rotate(m_Gizmo3D->GetDeltaPitch(), m_Gizmo3D->GetDeltaYaw());
				Camera->Update();

				m_CameraRep->TranslateInstanceTo(SelectionData.Name, Camera->GetEyePosition());
				m_CameraRep->RotateInstancePitchTo(SelectionData.Name, Camera->GetPitch());
				m_CameraRep->RotateInstanceYawTo(SelectionData.Name, Camera->GetYaw());
				m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name);
				break;
			}
			case CGame::EObjectType::Light:
			{
				const XMVECTOR& KTranslation{ m_LightArray[(uint32_t)SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name).Position };
				m_LightArray[(uint32_t)SelectionData.Extra]->SetInstancePosition(SelectionData.Name, KTranslation + m_Gizmo3D->GetDeltaTranslation());
				m_LightRep->SetInstancePosition(SelectionData.Name, KTranslation);
				break;
			}
			case CGame::EObjectType::Object3D:
			case CGame::EObjectType::Object3DInstance:
			{
				CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
				if (SelectionData.eObjectType == EObjectType::Object3DInstance)
				{
					Object3D->TranslateInstance(SelectionData.Name, m_Gizmo3D->GetDeltaTranslation());
					Object3D->RotateInstancePitch(SelectionData.Name, m_Gizmo3D->GetDeltaPitch());
					Object3D->RotateInstanceYaw(SelectionData.Name, m_Gizmo3D->GetDeltaYaw());
					Object3D->RotateInstanceRoll(SelectionData.Name, m_Gizmo3D->GetDeltaRoll());
					Object3D->ScaleInstance(SelectionData.Name, m_Gizmo3D->GetDeltaScaling());
					Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
				}
				else
				{
					if (Object3D->IsInstanced())
					{
						const auto& mapNameToIndex{ Object3D->GetInstanceNameToIndexMap() };
						for (const auto& InstancePair : mapNameToIndex)
						{
							Object3D->TranslateInstance(SelectionData.Name, m_Gizmo3D->GetDeltaTranslation());
							Object3D->RotateInstancePitch(SelectionData.Name, m_Gizmo3D->GetDeltaPitch());
							Object3D->RotateInstanceYaw(SelectionData.Name, m_Gizmo3D->GetDeltaYaw());
							Object3D->RotateInstanceRoll(SelectionData.Name, m_Gizmo3D->GetDeltaRoll());
							Object3D->ScaleInstance(SelectionData.Name, m_Gizmo3D->GetDeltaScaling());
						}
						Object3D->UpdateAllInstances();
					}
					else
					{
						Object3D->Translate(m_Gizmo3D->GetDeltaTranslation());
						Object3D->RotatePitch(m_Gizmo3D->GetDeltaPitch());
						Object3D->RotateYaw(m_Gizmo3D->GetDeltaYaw());
						Object3D->RotateRoll(m_Gizmo3D->GetDeltaRoll());
						Object3D->Scale(m_Gizmo3D->GetDeltaScaling());

						Object3D->UpdateWorldMatrix();
					}
				}

				break;
			}
			}
		}
	}
}

void CGame::BeginRendering(const FLOAT* ClearColor)
{
	ID3D11SamplerState* LinearWrapSampler{ m_CommonStates->LinearWrap() };
	ID3D11SamplerState* LinearClampSampler{ m_CommonStates->LinearClamp() };
	m_DeviceContext->PSSetSamplers(0, 1, &LinearWrapSampler);
	m_DeviceContext->PSSetSamplers(1, 1, &LinearClampSampler);
	m_DeviceContext->DSSetSamplers(0, 1, &LinearWrapSampler); // @important: in order to use displacement mapping

	SetUniversalRSState();

	m_MatrixView = m_PtrCurrentCamera->GetViewMatrix();

	if (m_EnvironmentTexture) m_EnvironmentTexture->Use();
	if (m_IrradianceTexture) m_IrradianceTexture->Use();
	if (m_PrefilteredRadianceTexture) m_PrefilteredRadianceTexture->Use();
	if (m_IntegratedBRDFTexture) m_IntegratedBRDFTexture->Use();
}

void CGame::Update()
{
	// Calculate time
	{
		m_TimeNow_ms = m_Clock.now().time_since_epoch().count() / 1'000'000;
		if (m_TimePrev_ms == 0) m_TimePrev_ms = m_TimeNow_ms;
		if (m_Timer_Test_ms == 0) m_Timer_Test_ms = m_TimePrev_ms;
		m_DeltaTime_s = static_cast<float>(0.001 * (m_TimeNow_ms - m_TimePrev_ms));
		m_DeltaTime_s = min(m_DeltaTime_s, 0.0625f);

		if (m_bIsTestTimerPaused)
		{
			m_DeltaTime_s = 0;

			if (m_bShouldAdvanceTestTimer)
			{
				m_DeltaTime_s += m_Test_DeltaTime_s;
				m_bShouldAdvanceTestTimer = false;
			}
		}
		else
		{
			m_DeltaTime_s *= m_Test_SlowFactor;
		}

		if (m_TimeNow_ms > m_Timer_Frame_ms + 1'000)
		{
			m_FPS = m_FrameCounter;
			m_FrameCounter = 0;
			m_Timer_Frame_ms = m_TimeNow_ms;
		}

		m_CBEditorTimeData.NormalizedTime += m_DeltaTime_s;
		m_CBEditorTimeData.NormalizedTimeHalfSpeed += m_DeltaTime_s * 0.5f;
		if (m_CBEditorTimeData.NormalizedTime > 1.0f) m_CBEditorTimeData.NormalizedTime = 0.0f;
		if (m_CBEditorTimeData.NormalizedTimeHalfSpeed > 1.0f) m_CBEditorTimeData.NormalizedTimeHalfSpeed = 0.0f;
		m_CBEditorTime->Update();

		m_CBWaterTimeData.Time += m_DeltaTime_s * 0.1f;
		if (m_CBWaterTimeData.Time > 1.0f) m_CBWaterTimeData.Time = 0.0f;
		m_CBWaterTime->Update();

		m_CBTerrainData.Time = m_CBEditorTimeData.NormalizedTime;
	}

	// Capture inputs
	m_CapturedKeyboardState = GetKeyState();
	m_CapturedMouseState = GetMouseState();

	// Engine input processing
	static int PrevMouseX{ m_CapturedMouseState.x };
	static int PrevMouseY{ m_CapturedMouseState.y };
	bool bMouseMoved{ (m_CapturedMouseState.x != PrevMouseX || m_CapturedMouseState.y != PrevMouseY) };
	if (GetMode() == EMode::Edit)
	{
		// Process keyboard inputs

		if (m_CapturedKeyboardState.LeftAlt && m_CapturedKeyboardState.Q)
		{
			Destroy();
			return;
		}

		if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyWindowFocused() && m_CapturedKeyboardState.Escape)
		{
			DeselectAll();
		}
		if (!ImGui::IsAnyItemActive())
		{
			if (m_CapturedKeyboardState.W)
			{
				m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Forward, m_DeltaTime_s);

				if (m_PtrCurrentCamera != m_EditorCamera.get())
					m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
			}
			if (m_CapturedKeyboardState.S)
			{
				m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Backward, m_DeltaTime_s);

				if (m_PtrCurrentCamera != m_EditorCamera.get())
					m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
			}
			if (m_CapturedKeyboardState.A && !m_CapturedKeyboardState.LeftControl)
			{
				m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Leftward, m_DeltaTime_s);

				if (m_PtrCurrentCamera != m_EditorCamera.get())
					m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
			}
			if (m_CapturedKeyboardState.D)
			{
				m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Rightward, m_DeltaTime_s);

				if (m_PtrCurrentCamera != m_EditorCamera.get())
					m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
			}

			if (m_CapturedKeyboardState.F)
			{
				m_CascadedShadowMap->CaptureFrustums();
			}

			if (m_CapturedKeyboardState.D1)
			{
				m_Gizmo3D->SetMode(CGizmo3D::EMode::Translation);
			}
			if (m_CapturedKeyboardState.D2)
			{
				m_Gizmo3D->SetMode(CGizmo3D::EMode::Rotation);
			}
			if (m_CapturedKeyboardState.D3)
			{
				m_Gizmo3D->SetMode(CGizmo3D::EMode::Scaling);
			}
		}

		if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
		{
			if (m_CapturedMouseState.rightButton) ImGui::SetWindowFocus(nullptr);

			if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
			{
				Update3DGizmos();

				if (m_bLeftButtonPressedOnce)
				{
					m_MultipleSelectionTopLeft = XMFLOAT2((float)m_CapturedMouseState.x, (float)m_CapturedMouseState.y);
					m_bIsMultipleSelectionChanging = true;
				}

				if (m_CapturedMouseState.leftButton &&
					(
						m_CapturedKeyboardState.LeftShift || m_CapturedKeyboardState.RightShift ||
						m_CapturedKeyboardState.LeftAlt || m_CapturedKeyboardState.RightAlt
						))
				{
					m_Gizmo3D->QuitAction();
				}
				if (m_Gizmo3D->IsInAction()) m_bIsMultipleSelectionChanging = false;

				if (m_bIsMultipleSelectionChanging)
				{
					m_MultipleSelectionBottomRight = XMFLOAT2((float)m_CapturedMouseState.x, (float)m_CapturedMouseState.y);
				}

				if (m_bLeftButtonUpOnce)
				{
					m_bIsMultipleSelectionChanging = false;

					Select();
				}

				if (!m_CapturedMouseState.leftButton) m_Gizmo3D->QuitAction();

				if (m_CapturedMouseState.rightButton) DeselectAll();

				if (bMouseMoved)
				{
					if (m_CapturedMouseState.leftButton || m_CapturedMouseState.rightButton)
					{
						SelectTerrain(true, m_CapturedMouseState.leftButton);
					}
					else
					{
						SelectTerrain(false, false);
					}
				}
			}
			else
			{
				SelectTerrain(false, false);
			}
		}
	}
	else
	{
		
	}

	if (bMouseMoved)
	{
		if ((GetMode() == EMode::Edit) ? m_CapturedMouseState.middleButton : m_CapturedMouseState.rightButton)
		{
			GetCurrentCamera()->Rotate(m_CapturedMouseState.x - PrevMouseX, m_CapturedMouseState.y - PrevMouseY, 
				(m_DeltaTime_s) ? m_DeltaTime_s : m_Test_DeltaTime_s);

			if (!IsEditorCamera(GetCurrentCamera()))
			{
				m_CameraRep->UpdateInstanceWorldMatrix(GetCurrentCamera()->GetName(), GetCurrentCamera()->GetWorldMatrix());
			}
		}
	}

	if (m_CapturedMouseState.scrollWheelValue)
	{
		GetCurrentCamera()->Zoom(m_CapturedMouseState.scrollWheelValue, 0.01f);

		if (!IsEditorCamera(GetCurrentCamera()))
		{
			m_CameraRep->UpdateInstanceWorldMatrix(GetCurrentCamera()->GetName(), GetCurrentCamera()->GetWorldMatrix());
		}
	}

	if (bMouseMoved)
	{
		PrevMouseX = m_CapturedMouseState.x;
		PrevMouseY = m_CapturedMouseState.y;
	}

	m_TimePrev_ms = m_TimeNow_ms;
	++m_FrameCounter;

	// Intelligence
	if (GetMode() != EMode::Edit)
	{
		m_Intelligence->Execute();
	}
	
	// Physics engine
	if (GetMode() != EMode::Edit)
	{
		CObject3D* const PlayerObject{ m_PhysicsEngine.GetPlayerObject() };
		if (PlayerObject)
		{
			GetCurrentCamera()->TranslateTo(PlayerObject->GetTransform().Translation);
		}

		m_PhysicsEngine.Update(m_DeltaTime_s);
	}
}

void CGame::Draw()
{
	if (m_IsDestroyed) return;

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	bool bShouldDrawNormals{ m_eMode == EMode::Edit && EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals) };
	if (bShouldDrawNormals)
	{
		if (EFLAG_HAS_NO(m_eFlagsRendering, EFlagsRendering::DrawWireFrame)) m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRSState();
	}

	// Deferred shading
	{
		// @important
		SetForwardRenderTargets(true); // @important: just for clearing...
		SetDeferredRenderTargets(true);

		m_DeviceContext->OMSetBlendState(m_CommonStates->Opaque(), nullptr, 0xFFFFFFFF);

		if (bShouldDrawNormals)
		{
			UpdateCBSpace();
			m_GSNormal->Use();
		}

		// Terrain
		DrawTerrainOpaqueParts(m_DeltaTime_s);

		// Opaque Object3Ds
		DrawOpaqueObject3Ds();

		if (bShouldDrawNormals)
		{
			m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
		}

		// Directional light shadow map
		{
			m_bIsDeferredRenderTargetsSet = false;

			XMMATRIX SavedViewMatrix{ m_MatrixView };
			XMMATRIX SavedProjectionMatrix{ m_MatrixProjection };

			const XMVECTOR& EyePosition{ m_PtrCurrentCamera->GetEyePosition() };
			const XMVECTOR& ViewDirection{ m_PtrCurrentCamera->GetForward() };

			size_t LODCount{ m_CascadedShadowMap->GetLODCount() };
			m_CBShadowMapData.LODCount = static_cast<uint32_t>(LODCount);
			for (size_t iLOD = 0; iLOD < LODCount; ++iLOD)
			{
				if (m_CascadedShadowMap->ShouldUpdate(iLOD))
				{
					m_CascadedShadowMap->Set(iLOD, SavedProjectionMatrix, EyePosition, ViewDirection, m_CBGlobalLightData.DirectionalLightDirection);

					// @important
					m_MatrixView = m_CascadedShadowMap->GetViewMatrix(iLOD);
					m_MatrixProjection = m_CascadedShadowMap->GetProjectionMatrix(iLOD);
					m_CBShadowMapData.ShadowMapSpaceMatrix[iLOD] = m_CascadedShadowMap->GetTransposedSpaceMatrix(iLOD);
					if (iLOD < CCascadedShadowMap::KLODCountMax) m_CBShadowMapData.ShadowMapZFars[iLOD] = m_CascadedShadowMap->GetZFar(iLOD);

					DrawOpaqueObject3Ds(true, true);
				}
			}

			m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
			m_MatrixView = SavedViewMatrix;
			m_MatrixProjection = SavedProjectionMatrix;
		}

		// @important
		m_CBGBufferUnpackingData.PerspectiveValues =
			XMFLOAT4(1.0f / XMVectorGetX(m_MatrixProjection.r[0]), 1.0f / XMVectorGetY(m_MatrixProjection.r[1]), 
				XMVectorGetZ(m_MatrixProjection.r[3]), -XMVectorGetZ(m_MatrixProjection.r[2]));
		m_CBGBufferUnpackingData.InverseViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m_MatrixView));
		m_CBGBufferUnpackingData.ScreenSize = m_WindowSize;
		m_CBGBufferUnpacking->Update();

		// Directional light
		{
			m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
			m_DeviceContext->OMSetRenderTargets(1, m_BackBufferRTV.GetAddressOf(), nullptr);

			m_CBGlobalLight->Update();
			m_CBShadowMap->Update();

			// @important
			vector<ID3D11ShaderResourceView*> vSRVs
			{
				m_GBuffers.DepthStencilSRV.Get(), m_GBuffers.BaseColorRoughSRV.Get(), m_GBuffers.NormalSRV.Get(), m_GBuffers.MetalAOSRV.Get()
			};
			size_t LODCount{ m_CascadedShadowMap->GetLODCount() };
			for (size_t iLOD = 0; iLOD < LODCount; ++iLOD)
			{
				vSRVs.emplace_back(m_CascadedShadowMap->GetSRV(iLOD));
			}

			m_DeviceContext->PSSetShaderResources(0, (UINT)vSRVs.size(), &vSRVs[0]);

			m_DirectionalLightFSQ->SetIA();

			// @important
			ID3D11SamplerState* const SamplerStates[]{ m_CommonStates->PointClamp(), m_CommonStates->LinearWrap(), m_CommonStates->LinearClamp() };
			m_DeviceContext->PSSetSamplers(0, ARRAYSIZE(SamplerStates), SamplerStates);
			
			m_DirectionalLightFSQ->SetShaders();

			m_DirectionalLightFSQ->Draw2D();
		}

		// LightArray
		{
			m_DeviceContext->OMSetBlendState(m_BlendAdditiveLighting.Get(), nullptr, 0xFFFFFFFF);
			m_DeviceContext->RSSetState(m_CommonStates->CullCounterClockwise());
			m_DeviceContext->OMSetDepthStencilState(m_DepthStencilStateGreaterEqual.Get(), 0); // @important??

			UpdateCBSpace();

			m_VSLight->Use();

			ID3D11SamplerState* const Samplers[]{ m_CommonStates->PointClamp() };
			m_DeviceContext->PSSetSamplers(0, ARRAYSIZE(Samplers), Samplers);

			// PointLight
			if (m_LightArray[0]->GetInstanceCount() > 0)
			{
				m_HSPointLight->Use();
				m_DSPointLight->Use();
				m_PSPointLight->Use();

				m_LightArray[0]->Light();
			}

			// SpotLight
			if (m_LightArray[1]->GetInstanceCount() > 0)
			{
				m_HSSpotLight->Use();
				m_DSSpotLight->Use();
				m_PSSpotLight->Use();

				m_LightArray[1]->Light();
			}

			m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
			m_DeviceContext->DSSetShader(nullptr, nullptr, 0);

			m_DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
			SetUniversalRSState();
		}

		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
	}

	// @important
	// Forward shading
	{
		SetForwardRenderTargets();

		if (m_eMode != EMode::Play)
		{
			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawWireFrame))
			{
				m_eRasterizerState = ERasterizerState::WireFrame;
			}
			else
			{
				m_eRasterizerState = ERasterizerState::CullCounterClockwise;
			}

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawMiniAxes))
			{
				DrawMiniAxes();
			}

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawPickingData))
			{
				DrawPickingRay();

				DrawPickedTriangle();
			}

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawGrid))
			{
				DrawGrid();
			}

			DrawCameraRep();

			DrawObject3DLines();

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawDirectionalLightShadowMap))
			{
				DrawShadowMapReps();

				m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
			}

			if (m_Terrain)
			{
				if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainHeightMapTexture))
				{
					m_DeviceContext->RSSetViewports(1, &m_vViewports[2]);
					UpdateCBSpace();
					m_Terrain->DrawHeightMapTexture();
				}

				if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainMaskingTexture))
				{
					m_DeviceContext->RSSetViewports(1, &m_vViewports[3]);
					UpdateCBSpace();
					m_Terrain->DrawMaskingTexture();
				}

				if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainFoliagePlacingTexture))
				{
					m_DeviceContext->RSSetViewports(1, &m_vViewports[4]);
					UpdateCBSpace();
					m_Terrain->DrawFoliagePlacingTexture();
				}

				m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
			}

			// Point representations
			{
				m_VSBase->Use();
				m_PSBase_RawVertexColor->Use();

				m_AClosestPointRep->TranslateTo(m_PhysicsEngine.GetDynamicClosestPoint());
				m_AClosestPointRep->UpdateWorldMatrix();

				m_BClosestPointRep->TranslateTo(m_PhysicsEngine.GetStaticClosestPoint());
				m_BClosestPointRep->UpdateWorldMatrix();

				m_PickedPointRep->TranslateTo(m_PhysicsEngine.GetPickedPoint());
				m_PickedPointRep->UpdateWorldMatrix();

				UpdateCBSpace(m_AClosestPointRep->GetWorldMatrix());
				m_AClosestPointRep->Draw();

				UpdateCBSpace(m_BClosestPointRep->GetWorldMatrix());
				m_BClosestPointRep->Draw();

				UpdateCBSpace(m_PickedPointRep->GetWorldMatrix());
				m_PickedPointRep->Draw();
			}
		}

		if (m_SkyData.bIsDataSet)
		{
			DrawSky(m_DeltaTime_s);
		}

		if (bShouldDrawNormals)
		{
			UpdateCBSpace();
			m_GSNormal->Use();
		}

		// Terrain
		DrawTerrainTransparentParts();

		m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

		// Transparent Object3Ds
		for (auto& Object3D : m_vObject3Ds)
		{
			if (!Object3D->IsTransparent()) continue;

			Object3D->Animate(m_DeltaTime_s);
			Object3D->UpdateWorldMatrix();
			DrawObject3D(Object3D.get());

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingVolumes))
			{
				DrawBoundingSphereRep(Object3D->GetTransform().Translation + Object3D->GetOuterBoundingSphereCenterOffset(),
					Object3D->GetOuterBoundingSphereRadius());
			}
		}

		if (bShouldDrawNormals)
		{
			m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
		}
	}

	// Drawing light volumes
	if (m_eMode == EMode::Edit && EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawLightVolumes))
	{
		m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

		UpdateCBSpace();

		m_VSLight->Use();

		// PointLight
		if (m_LightArray[0]->GetInstanceCount() > 0)
		{
			m_HSPointLight->Use();
			m_DSPointLight->Use();
			m_PSPointLight_Volume->Use();

			m_LightArray[0]->Light();
		}

		// SpotLight
		if (m_LightArray[1]->GetInstanceCount() > 0)
		{
			m_HSSpotLight->Use();
			m_DSSpotLight->Use();
			m_PSSpotLight_Volume->Use();

			m_LightArray[1]->Light();
		}

		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);

		SetUniversalRSState();
	}

	// 2D & Billboard
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
		m_DeviceContext->GSSetShader(nullptr, nullptr, 0);

		DrawObject2Ds();

		if (m_eMode == EMode::Edit)
		{
			DrawMultipleSelectionRep();

			m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);

			// This is not 2D object, but Billboard
			DrawLightRep();
		}
		else
		{
			m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
		}
	}
}

void CGame::DrawOpaqueObject3Ds(bool bIgnoreOwnTexture, bool bUseVoidPS)
{
	// Opaque Object3Ds
	for (auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->IsTransparent()) continue;
		if (Object3D->IsRigged())
		{
			UpdateCBAnimationData(Object3D->GetAnimationData());

			if (!Object3D->HasBakedAnimationTexture())
			{
				UpdateCBAnimationBoneMatrices(Object3D->GetAnimationBoneMatrices());
			}

			Object3D->Animate(m_DeltaTime_s);
		}

		Object3D->UpdateWorldMatrix();
		EFlagsObject3DRendering eFlagsRendering{};
		if (bIgnoreOwnTexture) eFlagsRendering |= EFlagsObject3DRendering::IgnoreOwnTextures;
		if (bUseVoidPS) eFlagsRendering |= EFlagsObject3DRendering::UseVoidPS;
		DrawObject3D(Object3D.get(), eFlagsRendering);

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingVolumes))
		{
			DrawBoundingSphereRep(Object3D->GetTransform().Translation + Object3D->GetOuterBoundingSphereCenterOffset(),
				Object3D->GetOuterBoundingSphereRadius());

			if (Object3D->HasInnerBoundingVolumes())
			{
				for (const auto& BoundingVolume : Object3D->GetInnerBoundingVolumeVector())
				{
					if (BoundingVolume.eType == EBoundingVolumeType::BoundingSphere)
					{
						DrawBoundingSphereRep(Object3D->GetTransform().Translation + BoundingVolume.Center, 
							BoundingVolume.Data.BS.Radius);
					}
					else
					{
						DrawAxisAlignedBoundingBoxRep(Object3D->GetTransform().Translation + BoundingVolume.Center,
							BoundingVolume.Data.AABBHalfSizes.x, BoundingVolume.Data.AABBHalfSizes.y, BoundingVolume.Data.AABBHalfSizes.z);
					}
				}
			}
		}
	}
}

void CGame::DrawObject3D(CObject3D* const PtrObject3D, EFlagsObject3DRendering eFlagsRendering, size_t OneInstanceIndex)
{
	if (!PtrObject3D) return;

	UpdateCBSpace(PtrObject3D->GetWorldMatrix());

	if (EFLAG_HAS(PtrObject3D->GetRenderingFlags(), CObject3D::EFlagsRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}

	if (PtrObject3D->IsRigged())
	{
		m_VSAnimation->Use();
	}
	else
	{
		if (PtrObject3D->IsInstanced())
		{
			m_VSBase_Instanced->Use();
		}
		else
		{
			m_VSBase->Use();
		}
	}
	
	if (PtrObject3D->ShouldTessellate())
	{
		UpdateCBTessFactorData(PtrObject3D->GetTessFactorData());
		UpdateCBDisplacementData(PtrObject3D->GetDisplacementData());

		m_HSStatic->Use();
		m_DSStatic->Use();
	}

	if (EFLAG_HAS(eFlagsRendering, EFlagsObject3DRendering::UseVoidPS))
	{
		m_PSBase_Void->Use();
	}
	else
	{
		(m_bIsDeferredRenderTargetsSet) ? m_PSBase_GBuffer->Use() : m_PSBase->Use();
	}

	PtrObject3D->Draw(eFlagsRendering, OneInstanceIndex);

	if (PtrObject3D->ShouldTessellate())
	{
		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	}

	if (EFLAG_HAS(PtrObject3D->GetRenderingFlags(), CObject3D::EFlagsRendering::NoCulling))
	{
		SetUniversalRSState();
	}
}

void CGame::DrawBoundingSphereRep(const XMVECTOR& Center, float Radius)
{
	m_VSBase->Use();
	m_PSBase_RawVertexColor->Use();

	XMMATRIX Translation{ XMMatrixTranslationFromVector(Center) };
	XMMATRIX Scaling{ XMMatrixScaling(Radius, Radius, Radius) };
	UpdateCBSpace(Scaling * Translation);

	m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

	m_BoundingSphereRep->Draw();

	SetUniversalRSState();
}

void CGame::DrawAxisAlignedBoundingBoxRep(const XMVECTOR& Center, float HalfSizeX, float HalfSizeY, float HalfSizeZ)
{
	m_VSBase->Use();
	m_PSBase_RawVertexColor->Use();

	XMMATRIX Translation{ XMMatrixTranslationFromVector(Center) };
	XMMATRIX Scaling{ XMMatrixScaling(HalfSizeX * 2.0f, HalfSizeY * 2.0f, HalfSizeZ * 2.0f) };
	UpdateCBSpace(Scaling * Translation);

	m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

	m_AxisAlignedBoundingBoxRep->Draw();

	SetUniversalRSState();
}

void CGame::DrawObject3DLines()
{
	m_VSLine->Use();
	m_PSLine->Use();

	for (auto& Object3DLine : m_vObject3DLines)
	{
		if (Object3DLine)
		{
			if (!Object3DLine->bIsVisible) continue;

			Object3DLine->UpdateWorldMatrix();

			UpdateCBSpace(Object3DLine->ComponentTransform.MatrixWorld);
			
			Object3DLine->Draw();
		}
	}
}

void CGame::DrawObject2Ds()
{
	m_VSBase2D->Use();
	
	for (auto& Object2D : m_vObject2Ds)
	{
		if (!Object2D->IsVisible()) continue;

		// @important
		UpdateCBSpace(Object2D->GetWorldMatrix(), m_MatrixProjection2D);
		
		(Object2D->HasTexture()) ? m_PSBase2D->Use() : m_PSBase2D_RawVertexColor->Use();

		Object2D->Draw();
	}
}

void CGame::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	m_VSBase->Use();
	m_PSBase_RawDiffuseColor->Use();

	for (auto& Object3D : m_vMiniAxes)
	{
		UpdateCBSpace(Object3D->GetWorldMatrix());

		Object3D->Draw();

		Object3D->TranslateTo(m_PtrCurrentCamera->GetTranslation() + m_PtrCurrentCamera->GetForward());

		Object3D->UpdateWorldMatrix();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGame::DrawPickingRay()
{
	m_VSLine->Use();

	UpdateCBSpace();
	
	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSLine->Use();

	m_PickingRayRep->Draw();
}

void CGame::DrawPickedTriangle()
{
	m_VSBase->Use();

	UpdateCBSpace();
	
	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSBase_RawVertexColor->Use();

	m_PickedTriangleRep->GetModel().vMeshes[0].vVertices[0].Position = m_PickedTriangleV0;
	m_PickedTriangleRep->GetModel().vMeshes[0].vVertices[1].Position = m_PickedTriangleV1;
	m_PickedTriangleRep->GetModel().vMeshes[0].vVertices[2].Position = m_PickedTriangleV2;
	m_PickedTriangleRep->UpdateMeshBuffer();

	m_PickedTriangleRep->Draw();
}

void CGame::DrawGrid()
{
	m_VSLine->Use();
	m_PSLine->Use();

	m_Grid->UpdateWorldMatrix();

	UpdateCBSpace(m_Grid->ComponentTransform.MatrixWorld);

	m_Grid->Draw();
}

void CGame::DrawShadowMapReps()
{
	m_VSLine->Use();
	m_PSLine->Use();

	UpdateCBSpace();

	m_CascadedShadowMap->DrawFrustums();

	m_CascadedShadowMap->DrawTextures();
}

void CGame::DrawSky(float DeltaTime)
{
	m_DeviceContext->RSSetState(m_CommonStates->CullNone());

	if (m_SkyData.bIsDynamic)
	{
		// Elapse SkyTime [0.0f, 1.0f]
		m_CBSkyTimeData.SkyTime += KSkyTimeFactorAbsolute * DeltaTime;
		if (m_CBSkyTimeData.SkyTime > 1.0f) m_CBSkyTimeData.SkyTime = 0.0f;

		// Update directional light source position
		static float DirectionalLightRoll{};
		if (m_CBSkyTimeData.SkyTime <= 0.25f)
		{
			DirectionalLightRoll = XM_2PI * (m_CBSkyTimeData.SkyTime + 0.25f);
		}
		else if (m_CBSkyTimeData.SkyTime > 0.25f && m_CBSkyTimeData.SkyTime <= 0.75f)
		{
			DirectionalLightRoll = XM_2PI * (m_CBSkyTimeData.SkyTime - 0.25f);
		}
		else
		{
			DirectionalLightRoll = XM_2PI * (m_CBSkyTimeData.SkyTime - 0.75f);
		}
		XMVECTOR DirectionalLightSourcePosition{ XMVector3TransformCoord(XMVectorSet(1, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, DirectionalLightRoll)) };
		SetDirectionalLightDirection(DirectionalLightSourcePosition);

		m_VSSky->Use();
		m_PSDynamicSky->Use();

		// SkySphere
		{
			m_Object3DSkySphere->TranslateTo(m_PtrCurrentCamera->GetTranslation());
			m_Object3DSkySphere->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DSkySphere->GetWorldMatrix());

			m_Object3DSkySphere->Draw();
		}

		m_PSBase->Use();

		// Sun
		{
			float SunRoll{ XM_2PI * m_CBSkyTimeData.SkyTime - XM_PIDIV2 };
			XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, SunRoll)) };
			m_Object3DSun->TranslateTo(m_PtrCurrentCamera->GetTranslation() + Offset);
			m_Object3DSun->RotateRollTo(SunRoll);
			m_Object3DSun->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DSun->GetWorldMatrix());
			
			m_Object3DSun->Draw();
		}

		// Moon
		{
			float MoonRoll{ XM_2PI * m_CBSkyTimeData.SkyTime + XM_PIDIV2 };
			XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, MoonRoll)) };
			m_Object3DMoon->TranslateTo(m_PtrCurrentCamera->GetTranslation() + Offset);
			m_Object3DMoon->RotateRollTo((MoonRoll > XM_2PI) ? (MoonRoll - XM_2PI) : MoonRoll);
			m_Object3DMoon->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DMoon->GetWorldMatrix());

			m_Object3DMoon->Draw();
		}
	}
	else
	{
		// SkySphere
		{
			m_Object3DSkySphere->TranslateTo(m_PtrCurrentCamera->GetTranslation());
			m_Object3DSkySphere->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DSkySphere->GetWorldMatrix());

			m_VSSky->Use();
			m_PSSky->Use();

			m_Object3DSkySphere->Draw();
		}
	}

	m_CBSkyTime->Update();
}

void CGame::DrawTerrainOpaqueParts(float DeltaTime)
{
	if (!m_Terrain) return;
	
	SetUniversalRSState();

	UpdateCBSpace(m_Terrain->GetSelectionData().TerrainWorld);

	m_Terrain->UpdateWind(DeltaTime);
	UpdateCBWindData(m_Terrain->GetWindData());

	UpdateCBTerrainSelection(m_Terrain->GetSelectionData());

	m_Terrain->ShouldTessellate(EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::TessellateTerrain));

	if (m_Terrain->ShouldTessellate())
	{
		m_HSTerrain->Use();
		m_DSTerrain->Use();

		UpdateCBTessFactorData(m_Terrain->GetTerrainTessFactorData());
		UpdateCBDisplacementData(m_Terrain->GetTerrainDisplacementData());
	}

	m_VSTerrain->Use();

	if (m_bIsDeferredRenderTargetsSet)
	{
		m_PSTerrain_gbuffer->Use();
	}
	else
	{
		m_PSTerrain->Use();
	}
	
	m_Terrain->DrawTerrain(EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals) && (m_eMode == EMode::Edit));

	if (m_Terrain->ShouldTessellate())
	{
		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	}

	if (false)
	{
		UpdateCBSpace(m_Terrain->GetWindRepresentationWorldMatrix());
		m_Terrain->DrawWindRepresentation();
	}
}

void CGame::DrawTerrainTransparentParts()
{
	if (!m_Terrain) return;

	if (m_Terrain->ShouldDrawWater())
	{
		UpdateCBSpace(m_Terrain->GetWaterWorldMatrix());
		UpdateCBTessFactorData(m_Terrain->GetWaterTessFactorData());
		UpdateCBDisplacementData(m_Terrain->GetWaterDisplacementData());

		m_Terrain->DrawWater();
	}

	if (m_Terrain->HasFoliageCluster())
	{
		UpdateCBSpace();

		m_Terrain->DrawFoliageCluster();
	}
}

void CGame::Draw3DGizmos()
{
	if (!IsAnythingSelected()) return;

	if (m_vSelectionData.size() >= 2)
	{
		// Multiple selection


	}
	else
	{
		// Single selection

		if (m_vSelectionData.back().eObjectType == EObjectType::EditorCamera)
		{
			return;
		}

		if (m_vSelectionData.back().eObjectType == EObjectType::Camera)
		{
			if (GetCamera(m_vSelectionData.back().Name) == GetCurrentCamera()) return;
		}
	}

	m_Gizmo3D->Draw(m_MatrixView * m_MatrixProjection);
}

void CGame::DrawCameraRep()
{
	if (!m_vCameras.size()) return;

	m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	
	UpdateCBSpace();

	m_CBCameraInfoData.CurrentCameraID = m_CurrentCameraID;
	m_CBCameraInfo->Update();

	m_VSBase_Instanced->Use();
	m_PSCamera->Use();

	m_CameraRep->Draw();
	
	SetUniversalRSState();
}

void CGame::DrawLightRep()
{
	if (!m_LightRep) return;

	UpdateCBSpace();

	m_LightRep->Draw();
}

void CGame::DrawMultipleSelectionRep()
{
	if (!m_bIsMultipleSelectionChanging) return;
	if (m_eEditMode == EEditMode::EditTerrain) return;

	m_VSBase2D->Use();
	m_PSBase2D_RawVertexColor->Use();

	m_MultipleSelectionBottomRight.x = min(m_MultipleSelectionBottomRight.x, m_WindowSize.x);
	m_MultipleSelectionBottomRight.y = min(m_MultipleSelectionBottomRight.y, m_WindowSize.y);

	XMFLOAT2 SelectionSize{ m_MultipleSelectionBottomRight.x - m_MultipleSelectionTopLeft.x, m_MultipleSelectionBottomRight.y - m_MultipleSelectionTopLeft.y };
	XMFLOAT2 SelectionTranslation{ 
		m_MultipleSelectionTopLeft.x - m_WindowSize.x * 0.5f + SelectionSize.x * 0.5f,
		m_WindowSize.y * 0.5f - m_MultipleSelectionTopLeft.y - SelectionSize.y * 0.5f };
	m_MultipleSelectionXYNormalized = XMFLOAT2(
		(m_MultipleSelectionTopLeft.x / m_WindowSize.x) * 2.0f - 1.0f, (m_MultipleSelectionTopLeft.y / m_WindowSize.y) * -2.0f + 1.0f);
	m_MultipleSelectionSizeNormalized = XMFLOAT2((SelectionSize.x / m_WindowSize.x) * 2.0f, (SelectionSize.y / m_WindowSize.y) * 2.0f);
	m_MultipleSelectionXYPrimeNormalized = XMFLOAT2(
		m_MultipleSelectionXYNormalized.x + m_MultipleSelectionSizeNormalized.x, m_MultipleSelectionXYNormalized.y - m_MultipleSelectionSizeNormalized.y);

	m_MultipleSelectionRep->TranslateTo(SelectionTranslation);
	m_MultipleSelectionRep->ScaleTo(SelectionSize);

	UpdateCBSpace(m_MultipleSelectionRep->GetWorldMatrix(), m_MatrixProjection2D);

	m_MultipleSelectionRep->Draw();
}

void CGame::DrawEditorGUI()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(m_EditorGUIFont);

	if (m_eMode == EMode::Edit)
	{
		DrawEditorGUIMenuBar();

		DrawEditorGUIPopupTerrainGenerator();
		DrawEditorGUIPopupObjectAdder();
		
		DrawEditorGUIWindowPropertyEditor();

		DrawEditorGUIWindowSceneEditor();
		
		static constexpr float KTestWindowSizeX{ 110.0f };
		ImGui::SetNextWindowPos(ImVec2((m_WindowSize.x - KTestWindowSizeX) * 0.5f, 21), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(KTestWindowSizeX, 0), ImGuiCond_Always);
		if (ImGui::Begin(u8"테스트 윈도우", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button(u8"테스트 시작"))
			{
				if (SetMode(EMode::Test))
				{
					m_SavedPlayerTransform = m_PhysicsEngine.GetPlayerObject()->GetTransform();
					m_SavedPlayerPhysics = m_PhysicsEngine.GetPlayerObject()->GetPhysics();

					m_SavedCurrentCamera = m_PtrCurrentCamera;

					UseCamera(m_PtrPlayerCamera);

					m_SavedRenderingFlags = GetRenderingFlags();

					TurnOnRenderingFlag(CGame::EFlagsRendering::DrawPickingData);
				}
			}
		}
		ImGui::End();
	}
	else if (m_eMode == EMode::Test)
	{
		static constexpr float KTestWindowSizeX{ 110.0f };
		ImGui::SetNextWindowPos(ImVec2((m_WindowSize.x - KTestWindowSizeX) * 0.5f, 21), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(KTestWindowSizeX, 0), ImGuiCond_Always);
		if (ImGui::Begin(u8"테스트 윈도우", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button(u8"테스트 종료"))
			{
				if (SetMode(EMode::Edit))
				{
					m_PhysicsEngine.GetPlayerObject()->SetTransform(m_SavedPlayerTransform);
					m_PhysicsEngine.GetPlayerObject()->SetPhysics(m_SavedPlayerPhysics);
					
					m_Intelligence->ClearBehaviors();

					UseCamera(m_SavedCurrentCamera);

					SetRenderingFlags(m_SavedRenderingFlags);
				}
			}
		}
		ImGui::End();

		if (ImGui::Begin(u8"제어판", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
		{
			static constexpr float KLabelWidth{ 160.0f };

			// 렌더링
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"FPS");
				ImGui::SameLine(KLabelWidth);
				ImGui::Text(to_string(m_FPS).c_str());
			}


			// 물리 엔진
			if (ImGui::TreeNodeEx(u8"물리 엔진", ImGuiTreeNodeFlags_DefaultOpen))
			{
				static int iButtonState{};
				static const char* KButtonTexts[2]{ u8"갱신 중지", u8"갱신 재개" };
				ImGui::SetCursorPosX(KLabelWidth);
				if (ImGui::Button(KButtonTexts[iButtonState]))
				{
					iButtonState = (iButtonState == 0) ? 1 : 0;
					m_bIsTestTimerPaused = !m_bIsTestTimerPaused;
				}

				if (iButtonState == 0)
				{
					ImGui::AlignTextToFramePadding();
					ImGui::Text(u8"갱신 속도");
					ImGui::SameLine(KLabelWidth);
					ImGui::SliderFloat(u8"##갱신 속도", &m_Test_SlowFactor, 0.1f, 1.0f, "%.2f");
				}
				else
				{
					ImGui::SameLine();
					if (ImGui::Button(u8"다음 프레임으로"))
					{
						m_bShouldAdvanceTestTimer = true;
					}
					ImGui::AlignTextToFramePadding();
					ImGui::Text(u8"Delta time (s)");
					ImGui::SameLine(KLabelWidth);
					ImGui::SliderFloat(u8"##Delta time (s)", &m_Test_DeltaTime_s, 0.01f, 0.1f, "%.2f");
				}

				ImGui::TreePop();
			}

			ImGui::Separator();

			// 플레이어
			if (ImGui::TreeNodeEx(u8"플레이어", ImGuiTreeNodeFlags_DefaultOpen))
			{
				CObject3D* const PlayerObject{ m_PhysicsEngine.GetPlayerObject() };
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"위치");
				ImGui::SameLine(KLabelWidth);
				ImGui::DragFloat3(u8"##위치", (float*)&PlayerObject->GetTransform().Translation, 0.1f);
				
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"속도");
				ImGui::SameLine(KLabelWidth);
				ImGui::DragFloat3(u8"##속도", (float*)&PlayerObject->GetPhysics().LinearVelocity, 0.1f);

				ImGui::TreePop();
			}
		}
		ImGui::End();
	}

	ImGui::PopFont();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CGame::DrawEditorGUIMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		bool bShowDialogLoadTerrain{};
		bool bShowDialogSaveTerrain{};

		if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.A) m_EditorGUIBools.bShowPopupObjectAdder = true;
		if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.N) m_EditorGUIBools.bShowPopupTerrainGenerator = true;
		if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.O) bShowDialogLoadTerrain = true;
		if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.S) bShowDialogSaveTerrain = true;

		if (ImGui::BeginMenu(u8"지형"))
		{
			if (ImGui::MenuItem(u8"만들기", "Ctrl+N", nullptr)) m_EditorGUIBools.bShowPopupTerrainGenerator = true;
			if (ImGui::MenuItem(u8"불러오기", "Ctrl+O", nullptr)) bShowDialogLoadTerrain = true;
			if (ImGui::MenuItem(u8"내보내기", "Ctrl+S", nullptr)) bShowDialogSaveTerrain = true;

			ImGui::EndMenu();
		}

		if (bShowDialogLoadTerrain)
		{
			static CFileDialog FileDialog{ GetAssetDirectory() };
			if (FileDialog.OpenFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 불러오기"))
			{
				LoadTerrain(FileDialog.GetRelativeFileName());
			}
		}

		if (bShowDialogSaveTerrain)
		{
			if (GetTerrain())
			{
				static CFileDialog FileDialog{ GetAssetDirectory() };
				if (FileDialog.SaveFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 내보내기", ".terr"))
				{
					SaveTerrain(FileDialog.GetRelativeFileName());
				}
			}
		}

		if (ImGui::BeginMenu(u8"창"))
		{
			ImGui::MenuItem(u8"속성 편집기", nullptr, &m_EditorGUIBools.bShowWindowPropertyEditor);
			ImGui::MenuItem(u8"장면 편집기", nullptr, &m_EditorGUIBools.bShowWindowSceneEditor);

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem(u8"종료", "Alt+Q"))
		{
			Destroy();
			return;
		}

		ImGui::EndMainMenuBar();
	}
}

void CGame::DrawEditorGUIPopupTerrainGenerator()
{
	// ### 지형 생성기 윈도우 ###
	if (m_EditorGUIBools.bShowPopupTerrainGenerator) ImGui::OpenPopup(u8"지형 생성기");
	if (ImGui::BeginPopupModal(u8"지형 생성기", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		static int SizeX{ CTerrain::KDefaultSize };
		static int SizeZ{ CTerrain::KDefaultSize };
		static int MaskingDetail{ CTerrain::KMaskingDefaultDetail };
		static float UniformScaling{ CTerrain::KMinUniformScaling };

		ImGui::Text(u8"가로:");
		ImGui::SameLine(120);
		ImGui::SetNextItemWidth(80);
		ImGui::DragInt("##0", &SizeX, 2.0f, 2, 1000);
		if (SizeX % 2) ++SizeX;

		ImGui::Text(u8"세로:");
		ImGui::SameLine(120);
		ImGui::SetNextItemWidth(80);
		ImGui::DragInt("##1", &SizeZ, 2.0f, 2, 1000);
		if (SizeZ % 2) ++SizeZ;

		ImGui::Text(u8"마스킹 디테일:");
		ImGui::SameLine(120);
		ImGui::SetNextItemWidth(80);
		ImGui::SliderInt("##2", &MaskingDetail, CTerrain::KMaskingMinDetail, CTerrain::KMaskingMaxDetail);

		ImGui::Text(u8"스케일링:");
		ImGui::SameLine(120);
		ImGui::SetNextItemWidth(80);
		ImGui::DragFloat("##3", &UniformScaling, 0.1f, CTerrain::KMinUniformScaling, CTerrain::KMaxUniformScaling, "%.1f");

		ImGui::Separator();

		if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
		{
			CTerrain::EEditMode eTerrainEditMode{};
			if (GetTerrain()) eTerrainEditMode = GetTerrain()->GetEditMode();

			XMFLOAT2 TerrainSize{ (float)SizeX, (float)SizeZ };
			CreateTerrain(TerrainSize, MaskingDetail, UniformScaling);

			CTerrain* const Terrain{ GetTerrain() };
			SetEditMode(GetEditMode(), true);
			Terrain->SetEditMode(eTerrainEditMode);

			SizeX = CTerrain::KDefaultSize;
			SizeZ = CTerrain::KDefaultSize;
			m_EditorGUIBools.bShowPopupTerrainGenerator = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
		{
			SizeX = CTerrain::KDefaultSize;
			SizeZ = CTerrain::KDefaultSize;

			m_EditorGUIBools.bShowPopupTerrainGenerator = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void CGame::DrawEditorGUIPopupObjectAdder()
{
	// ### 오브젝트 추가 윈도우 ###
	if (m_EditorGUIBools.bShowPopupObjectAdder) ImGui::OpenPopup(u8"오브젝트 추가기");
	ImGui::SetNextWindowPosCenter();
	if (ImGui::BeginPopupModal(u8"오브젝트 추가기", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		static char NewObejctName[KAssetNameMaxLength]{};
		static char ModelFileNameWithPath[MAX_PATH]{};
		static char ModelFileNameWithoutPath[MAX_PATH]{};
		static char ModelFileNameExtension[MAX_PATH]{};
		static bool bIsModelRigged{ false };

		static constexpr float KIndentPerDepth{ 12.0f };
		static constexpr float KItemsOffetX{ 150.0f };
		static constexpr float KItemsWidth{ 150.0f };
		static const char* const KOptions[5]{ u8"3D 도형", u8"3D 모델 파일", u8"2D 도형", u8"카메라", u8"광원" };
		static int iSelectedOption{};

		static const char* const K3DPrimitiveTypes[9]{ u8"정사각형(XY)", u8"정사각형(XZ)", u8"정사각형(YZ)",
				u8"원", u8"정육면체", u8"각뿔", u8"각기둥", u8"구", u8"도넛(Torus)" };
		static int iSelected3DPrimitiveType{};
		static uint32_t SideCount{ KDefaultPrimitiveDetail };
		static uint32_t SegmentCount{ KDefaultPrimitiveDetail };
		static float RadiusFactor{ 0.0f };
		static float InnerRadius{ 0.5f };
		static float WidthScalar3D{ 1.0f };
		static float HeightScalar3D{ 1.0f };
		static float PixelWidth{ 50.0f };
		static float PixelHeight{ 50.0f };

		static const char* const K2DPrimitiveTypes[1]{ u8"정사각형" };
		static int iSelected2DPrimitiveType{};

		static const char* const KCameraTypes[3]{ u8"1인칭", u8"3인칭", u8"자유 시점" };
		static int iSelectedCameraType{};

		static const char* const KLightTypes[2]{ u8"점 광원", u8"SpotLight" };
		static int iSelectedLightType{};

		static XMFLOAT4 MaterialUniformColor{ 1.0f, 1.0f, 1.0f, 1.0f };

		bool bShowDialogLoad3DModel{};

		ImGui::SetNextItemWidth(140);
		if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
		ImGui::InputText(u8"오브젝트 이름", NewObejctName, KAssetNameMaxLength);

		for (int iOption = 0; iOption < ARRAYSIZE(KOptions); ++iOption)
		{
			if (ImGui::Selectable(KOptions[iOption], (iSelectedOption == iOption)))
			{
				iSelectedOption = iOption;
			}

			if (iSelectedOption == iOption)
			{
				if (iSelectedOption == 0)
				{
					ImGui::Indent(KIndentPerDepth);

					for (int i3DPrimitiveType = 0; i3DPrimitiveType < ARRAYSIZE(K3DPrimitiveTypes); ++i3DPrimitiveType)
					{
						if (ImGui::Selectable(K3DPrimitiveTypes[i3DPrimitiveType], (iSelected3DPrimitiveType == i3DPrimitiveType)))
						{
							iSelected3DPrimitiveType = i3DPrimitiveType;
						}
						if (i3DPrimitiveType == iSelected3DPrimitiveType)
						{
							ImGui::Indent(KIndentPerDepth);

							ImGui::PushItemWidth(KItemsWidth);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"- 색상");
							ImGui::SameLine(KItemsOffetX);
							ImGui::ColorEdit3(u8"##- 색상", (float*)&MaterialUniformColor.x, ImGuiColorEditFlags_RGB);

							// Quasi-2D primitives scalars
							if (iSelected3DPrimitiveType >= 0 && iSelected3DPrimitiveType <= 3)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"- 가로 크기");
								ImGui::SameLine(KItemsOffetX);
								ImGui::SliderFloat(u8"##- 가로 크기", &WidthScalar3D, 0.01f, 100.0f);

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"- 세로 크기");
								ImGui::SameLine(KItemsOffetX);
								ImGui::SliderFloat(u8"##- 세로 크기", &HeightScalar3D, 0.01f, 100.0f);
							}

							// 3D primitives that require SideCount
							if (iSelected3DPrimitiveType == 3 || (iSelected3DPrimitiveType >= 5 && iSelected3DPrimitiveType <= 6) || iSelected3DPrimitiveType == 8)
							{
								if (iSelected3DPrimitiveType == 5)
								{
									// Cone
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"- 반지름 인수");
									ImGui::SameLine(KItemsOffetX);
									ImGui::SliderFloat(u8"##- 반지름 인수", &RadiusFactor, 0.0f, 1.0f);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"- 옆면 수");
								ImGui::SameLine(KItemsOffetX);
								ImGui::SliderInt(u8"##- 옆면 수", (int*)&SideCount, KMinPrimitiveDetail, KMaxPrimitiveDetail);
							}

							// 3D primitives that require SegmentCount
							if (iSelected3DPrimitiveType == 7 || iSelected3DPrimitiveType == 8)
							{
								if (iSelected3DPrimitiveType == 8)
								{
									// Torus
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"- 띠 반지름");
									ImGui::SameLine(KItemsOffetX);
									ImGui::SliderFloat(u8"##- 띠 반지름", &InnerRadius, 0.0f, 1.0f);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"- Segment 수");
								ImGui::SameLine(KItemsOffetX);
								ImGui::SliderInt(u8"##- Segment 수", (int*)&SegmentCount, KMinPrimitiveDetail, KMaxPrimitiveDetail);
							}

							ImGui::PopItemWidth();

							ImGui::Unindent(KIndentPerDepth);
						}
					}

					ImGui::Unindent(KIndentPerDepth);
				}
				else if (iSelectedOption == 1)
				{
					if (ImGui::Button(u8"모델 불러오기")) bShowDialogLoad3DModel = true;

					ImGui::SameLine();

					ImGui::Text(ModelFileNameWithoutPath);

					ImGui::SameLine();

					ImGui::Checkbox(u8"리깅 여부", &bIsModelRigged);
				}
				else if (iSelectedOption == 2)
				{
					ImGui::Indent(KIndentPerDepth);

					for (int i2DPrimitiveType = 0; i2DPrimitiveType < ARRAYSIZE(K2DPrimitiveTypes); ++i2DPrimitiveType)
					{
						if (ImGui::Selectable(K2DPrimitiveTypes[i2DPrimitiveType], (iSelected2DPrimitiveType == i2DPrimitiveType)))
						{
							iSelected2DPrimitiveType = i2DPrimitiveType;
						}
						if (i2DPrimitiveType == iSelected2DPrimitiveType)
						{
							ImGui::Indent(KIndentPerDepth);

							ImGui::PushItemWidth(KItemsWidth);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"- 가로 크기(픽셀)");
							ImGui::SameLine(KItemsOffetX);
							ImGui::DragFloat(u8"##- 가로 크기(픽셀)", &PixelWidth, 1.0f, 0.0f, 4096.0f, "%.0f");

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"- 세로 크기(픽셀)");
							ImGui::SameLine(KItemsOffetX);
							ImGui::DragFloat(u8"##- 세로 크기(픽셀)", &PixelHeight, 1.0f, 0.0f, 4096.0f, "%.0f");

							ImGui::PopItemWidth();

							ImGui::Unindent(KIndentPerDepth);
						}
					}

					ImGui::Unindent(KIndentPerDepth);
				}
				else if (iSelectedOption == 3)
				{
					static constexpr float KOffetX{ 150.0f };
					static constexpr float KItemsWidth{ 150.0f };
					ImGui::Indent(20.0f);

					ImGui::AlignTextToFramePadding();
					ImGui::Text(u8"카메라 종류");
					ImGui::SameLine(KOffetX);
					if (ImGui::BeginCombo(u8"##카메라 종류", KCameraTypes[iSelectedCameraType]))
					{
						for (int iCameraType = 0; iCameraType < ARRAYSIZE(KCameraTypes); ++iCameraType)
						{
							if (ImGui::Selectable(KCameraTypes[iCameraType], (iSelectedCameraType == iCameraType)))
							{
								iSelectedCameraType = iCameraType;
							}
							if (iSelectedCameraType == iCameraType)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::Unindent();
				}
				else if (iSelectedOption == 4)
				{
				static constexpr float KOffetX{ 150.0f };
				static constexpr float KItemsWidth{ 150.0f };
				ImGui::Indent(20.0f);

				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"광원 종류");
				ImGui::SameLine(KOffetX);
				if (ImGui::BeginCombo(u8"##광원 종류", KLightTypes[iSelectedLightType]))
				{
					for (int iLightType = 0; iLightType < ARRAYSIZE(KLightTypes); ++iLightType)
					{
						if (ImGui::Selectable(KLightTypes[iLightType], (iSelectedLightType == iLightType)))
						{
							iSelectedLightType = iLightType;
						}
						if (iSelectedLightType == iLightType)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				ImGui::Unindent();
				}
			}
		}
		
		if (ImGui::Button(u8"결정") || m_CapturedKeyboardState.Enter)
		{
			if (iSelectedOption == 0 && strlen(NewObejctName) == 0)
			{
				strcpy_s(NewObejctName, ("primitive" + to_string(m_PrimitiveCreationCounter)).c_str());
			}

			if (iSelectedOption == 1 && ModelFileNameWithPath[0] == 0)
			{
				MB_WARN("모델을 불러오세요.", "오류");
			}
			else if (strlen(NewObejctName) == 0 && iSelectedOption != 4)
			{
				MB_WARN("이름을 정하세요.", "오류");
			}
			else
			{
				bool IsObjectCreated{ true };
				if (iSelectedOption == 0)
				{
					if (GetObject3D(NewObejctName, false))
					{
						MB_WARN("이미 존재하는 이름입니다.", "3D 오브젝트 생성 실패");
						IsObjectCreated = false;
					}
					else
					{
						InsertObject3D(NewObejctName);
						CObject3D* const Object3D{ GetObject3D(NewObejctName) };

						SMesh Mesh{};
						CMaterialData MaterialData{};
						MaterialData.SetUniformColor(XMFLOAT3(MaterialUniformColor.x, MaterialUniformColor.y, MaterialUniformColor.z));
						switch (iSelected3DPrimitiveType)
						{
						case 0:
							Mesh = GenerateSquareXYPlane();
							ScaleMesh(Mesh, XMVectorSet(WidthScalar3D, HeightScalar3D, 1.0f, 0));
							Object3D->SetOuterBoundingSphereRadiusBias(sqrt(2.0f));
							break;
						case 1:
							Mesh = GenerateSquareXZPlane();
							ScaleMesh(Mesh, XMVectorSet(WidthScalar3D, 1.0f, HeightScalar3D, 0));
							Object3D->SetOuterBoundingSphereRadiusBias(sqrt(2.0f));
							break;
						case 2:
							Mesh = GenerateSquareYZPlane();
							ScaleMesh(Mesh, XMVectorSet(1.0f, WidthScalar3D, HeightScalar3D, 0));
							Object3D->SetOuterBoundingSphereRadiusBias(sqrt(2.0f));
							break;
						case 3:
							Mesh = GenerateCircleXZPlane(SideCount);
							ScaleMesh(Mesh, XMVectorSet(WidthScalar3D, 1.0f, HeightScalar3D, 0));
							break;
						case 4:
							Mesh = GenerateCube();
							break;
						case 5:
							Mesh = GenerateCone(RadiusFactor, 1.0f, 1.0f, SideCount);
							Object3D->SetOuterBoundingSphereCenterOffset(XMVectorSet(0, -0.5f, 0, 0));
							break;
						case 6:
							Mesh = GenerateCylinder(1.0f, 1.0f, SideCount);
							Object3D->SetOuterBoundingSphereRadiusBias(sqrt(1.5f));
							break;
						case 7:
							Mesh = GenerateSphere(SegmentCount);
							break;
						case 8:
							Mesh = GenerateTorus(InnerRadius, SideCount, SegmentCount);
							Object3D->SetOuterBoundingSphereRadiusBias(1.0f + InnerRadius);
							break;
						default:
							break;
						}
						Object3D->Create(Mesh, MaterialData);
						
						WidthScalar3D = 1.0f;
						HeightScalar3D = 1.0f;

						++m_PrimitiveCreationCounter;
					}
				}
				else if (iSelectedOption == 1)
				{
					if (GetObject3D(NewObejctName, false))
					{
						MB_WARN("이미 존재하는 이름입니다.", "3D 오브젝트 생성 실패");
						IsObjectCreated = false;
					}
					else
					{
						InsertObject3D(NewObejctName);
						CObject3D* const Object3D{ GetObject3D(NewObejctName) };

						string Extension{ ModelFileNameExtension };
						if (Extension == "OB3D")
						{
							Object3D->LoadOB3D(ModelFileNameWithPath, bIsModelRigged);
						}
						else
						{
							Object3D->CreateFromFile(ModelFileNameWithPath, bIsModelRigged);
						}

						memset(ModelFileNameWithPath, 0, MAX_PATH);
						memset(ModelFileNameWithoutPath, 0, MAX_PATH);
						memset(ModelFileNameExtension, 0, MAX_PATH);
					}
				}
				else if (iSelectedOption == 2)
				{
					if (GetObject2D(NewObejctName, false))
					{
						MB_WARN("이미 존재하는 이름입니다.", "2D 오브젝트 생성 실패");
						IsObjectCreated = false;
					}
					else
					{
						InsertObject2D(NewObejctName);
						CObject2D* const Object2D{ GetObject2D(NewObejctName) };
						switch (iSelected2DPrimitiveType)
						{
						case 0:
							Object2D->Create(Generate2DRectangle(XMFLOAT2(PixelWidth, PixelHeight)));
							break;
						default:
							break;
						}
					}
				}
				else if (iSelectedOption == 3)
				{
					if (GetCamera(NewObejctName, false))
					{
						MB_WARN("이미 존재하는 이름입니다.", "카메라 생성 실패");
						IsObjectCreated = false;
					}
					else
					{
						InsertCamera(NewObejctName, (CCamera::EType)iSelectedCameraType);
					}
				}
				else if (iSelectedOption == 4)
				{
					if (!InsertLight((CLight::EType)iSelectedLightType, NewObejctName))
					{
						IsObjectCreated = false;
					}
				}

				if (IsObjectCreated)
				{
					m_EditorGUIBools.bShowPopupObjectAdder = false;
					memset(NewObejctName, 0, KAssetNameMaxLength);

					ImGui::CloseCurrentPopup();
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"취소") || ImGui::IsKeyDown(VK_ESCAPE))
		{
			m_EditorGUIBools.bShowPopupObjectAdder = false;
			memset(ModelFileNameWithPath, 0, MAX_PATH);
			memset(ModelFileNameWithoutPath, 0, MAX_PATH);
			memset(ModelFileNameExtension, 0, MAX_PATH);
			memset(NewObejctName, 0, KAssetNameMaxLength);
			ImGui::CloseCurrentPopup();
		}

		if (bShowDialogLoad3DModel)
		{
			static CFileDialog FileDialog{ GetAssetDirectory() };
			if (FileDialog.OpenFileDialog("FBX 파일\0*.fbx\0MESH 파일\0*.mesh\0OB3D 파일\0*.ob3d\0모든 파일\0*.*\0", "모델 불러오기"))
			{
				strcpy_s(ModelFileNameWithPath, FileDialog.GetRelativeFileName().c_str());
				strcpy_s(ModelFileNameWithoutPath, FileDialog.GetFileNameWithoutPath().c_str());
				strcpy_s(ModelFileNameExtension, FileDialog.GetCapitalExtension().c_str());
			}
		}

		ImGui::EndPopup();
	}
}

void CGame::DrawEditorGUIWindowPropertyEditor()
{
	// ### 속성 편집기 윈도우 ###
	if (m_EditorGUIBools.bShowWindowPropertyEditor)
	{
		static constexpr float KInitialWindowWidth{ 400 };
		ImGui::SetNextWindowPos(ImVec2(m_WindowSize.x - KInitialWindowWidth + 6, 21), ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(ImVec2(KInitialWindowWidth, 0), ImGuiCond_Appearing);
		ImGui::SetNextWindowSizeConstraints(
			ImVec2(m_WindowSize.x * 0.25f, m_WindowSize.y), ImVec2(m_WindowSize.x * 0.5f, m_WindowSize.y));

		if (ImGui::Begin(u8"속성 편집기", &m_EditorGUIBools.bShowWindowPropertyEditor, 
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
		{
			float WindowWidth{ ImGui::GetWindowWidth() };

			if (ImGui::BeginTabBar(u8"탭바", ImGuiTabBarFlags_None))
			{
				// 오브젝트 탭
				if (ImGui::BeginTabItem(u8"오브젝트"))
				{
					static bool bShowAnimationAdder{ false };
					static bool bShowAnimationEditor{ false };
					static int iSelectedAnimationID{};

					SetEditMode(EEditMode::EditObject);

					static constexpr float KLabelsWidth{ 240 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };

					// TODO
					// Instance culling data
					if (false)
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text((u8"Total Instance Count: " + to_string(m_Object3DTotalInstanceCount)).c_str());

						ImGui::AlignTextToFramePadding();
						ImGui::Text((u8"Culled Instance Count: " + to_string(m_Object3DCulledInstanceCount)).c_str());

						ImGui::Separator();
					}

					if (m_vSelectionData.size() == 1)
					{
						const SSelectionData& SelectionData{ m_vSelectionData.back() };

						switch (SelectionData.eObjectType)
						{
						default:
						case CGame::EObjectType::NONE:
						case CGame::EObjectType::Object3DLine:
						case CGame::EObjectType::Object2D:
							ImGui::Text(u8"<먼저 오브젝트를 선택하세요.>");
							break;
						case CGame::EObjectType::Object3D:
						case CGame::EObjectType::Object3DInstance:
						{
							ImGui::PushItemWidth(ItemsWidth);
							{
								CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"선택된 오브젝트:");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"<%s>", Object3D->GetName().c_str());

								ImGui::Separator();

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"정점 개수");
								ImGui::SameLine(ItemsOffsetX);
								int VertexCount{};
								for (const SMesh& Mesh : Object3D->GetModel().vMeshes)
								{
									VertexCount += (int)Mesh.vVertices.size();
								}
								ImGui::Text(u8"%d", VertexCount);

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"삼각형 개수");
								ImGui::SameLine(ItemsOffsetX);
								int TriangleCount{};
								for (const SMesh& Mesh : Object3D->GetModel().vMeshes)
								{
									TriangleCount += (int)Mesh.vTriangles.size();
								}
								ImGui::Text(u8"%d", TriangleCount);


								ImGui::Separator();

								if (SelectionData.eObjectType == EObjectType::Object3DInstance)
								{
									auto& InstanceTransform{ Object3D->GetInstanceTransform(SelectionData.Name) };

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"선택된 인스턴스:");
									ImGui::SameLine(ItemsOffsetX);
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"<%s>", SelectionData.Name.c_str());

									ImGui::Separator();

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"위치");
									ImGui::SameLine(ItemsOffsetX);
									XMVECTOR Translation{ InstanceTransform.Translation };
									if (ImGui::DragFloat3(u8"##위치", Translation.m128_f32, KTranslationDelta,
										KTranslationMinLimit, KTranslationMaxLimit, "%.2f"))
									{
										Object3D->TranslateInstanceTo(SelectionData.Name, Translation);
										Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);

										Capture3DGizmoTranslation();
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"회전");
									ImGui::SameLine(ItemsOffsetX);
									int PitchYawRoll360[3]{ 
										(int)(InstanceTransform.Pitch * KRotation2PITo360),
										(int)(InstanceTransform.Yaw * KRotation2PITo360),
										(int)(InstanceTransform.Roll * KRotation2PITo360) };
									if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, KRotation360Unit,
										KRotation360MinLimit, KRotation360MaxLimit))
									{
										Object3D->RotateInstancePitchTo(SelectionData.Name, PitchYawRoll360[0] * KRotation360To2PI);
										Object3D->RotateInstanceYawTo(SelectionData.Name, PitchYawRoll360[1] * KRotation360To2PI);
										Object3D->RotateInstanceRollTo(SelectionData.Name, PitchYawRoll360[2] * KRotation360To2PI);
										Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"크기");
									ImGui::SameLine(ItemsOffsetX);
									XMVECTOR Scaling{ InstanceTransform.Scaling };
									if (ImGui::DragFloat3(u8"##크기", Scaling.m128_f32, KScalingDelta,
										CObject3D::KScalingMinLimit, CObject3D::KScalingMaxLimit, "%.3f"))
									{
										Object3D->ScaleInstanceTo(SelectionData.Name, Scaling);
										Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);

										Update3DGizmos();
									}

									ImGui::Separator();
								}
								else
								{
									if (Object3D->IsInstanced())
									{
										ImGui::Text(u8"<인스턴스를 선택해 주세요.>");

										ImGui::Separator();
									}
									
									// Non-instanced Object3D
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"위치");
										ImGui::SameLine(ItemsOffsetX);
										XMVECTOR Translation{ Object3D->GetTransform().Translation };
										if (ImGui::DragFloat3(u8"##위치", Translation.m128_f32, KTranslationDelta,
											KTranslationMinLimit, KTranslationMaxLimit, "%.2f"))
										{
											Object3D->TranslateTo(Translation);
											Object3D->UpdateWorldMatrix();

											Capture3DGizmoTranslation();
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"회전");
										ImGui::SameLine(ItemsOffsetX);
										int PitchYawRoll360[3]{ 
											(int)(Object3D->GetTransform().Pitch * KRotation2PITo360),
											(int)(Object3D->GetTransform().Yaw * KRotation2PITo360),
											(int)(Object3D->GetTransform().Roll * KRotation2PITo360) };
										if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, KRotation360Unit,
											KRotation360MinLimit, KRotation360MaxLimit))
										{
											Object3D->RotatePitchTo(PitchYawRoll360[0] * KRotation360To2PI);
											Object3D->RotateYawTo(PitchYawRoll360[1] * KRotation360To2PI);
											Object3D->RotateRollTo(PitchYawRoll360[2] * KRotation360To2PI);
											Object3D->UpdateWorldMatrix();

											Update3DGizmos();
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"크기");
										ImGui::SameLine(ItemsOffsetX);
										XMVECTOR Scaling{ Object3D->GetTransform().Scaling };
										if (ImGui::DragFloat3(u8"##크기", Scaling.m128_f32, KScalingDelta,
											CObject3D::KScalingMinLimit, CObject3D::KScalingMaxLimit, "%.3f"))
										{
											Object3D->ScaleTo(Scaling);
											Object3D->UpdateWorldMatrix();

											Update3DGizmos();
										}
									}
								}

								ImGui::Separator();

								if (ImGui::TreeNode(u8"Physics properties"))
								{
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"오브젝트 역할: ");

									ImGui::SameLine(ItemsOffsetX);
									EObjectRole ObjectRole{ m_PhysicsEngine.GetObjectRole(Object3D) };
									switch (ObjectRole)
									{
									case EObjectRole::None:
										ImGui::Text(u8"미지정");
										break;
									case EObjectRole::Player:
										ImGui::Text(u8"<플레이어>");
										break;
									case EObjectRole::Environment:
										ImGui::Text(u8"<환경>");
										break;
									case EObjectRole::Monster:
										ImGui::Text(u8"<몬스터>");
										break;
									default:
										break;
									}

									if (ImGui::Button(u8"플레이어로"))
									{
										m_PhysicsEngine.RegisterObject(Object3D, EObjectRole::Player);
										m_Intelligence->RegisterPriority(Object3D, EObjectPriority::A_Crucial, true); // @important
									}
									ImGui::SameLine();
									if (ImGui::Button(u8"환경으로"))
									{
										m_PhysicsEngine.RegisterObject(Object3D, EObjectRole::Environment);
									}
									ImGui::SameLine();
									if (ImGui::Button(u8"몬스터로"))
									{
										m_PhysicsEngine.RegisterObject(Object3D, EObjectRole::Monster);
									}

									XMFLOAT3 LinearVelocity{};
									XMStoreFloat3(&LinearVelocity, Object3D->GetPhysics().LinearVelocity);
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"속도");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::DragFloat3(u8"##속도", &LinearVelocity.x, 0.1f))
									{
										Object3D->SetLinearVelocity(XMLoadFloat3(&LinearVelocity));
									}

									ImGui::TreePop();
								}

								ImGui::Separator();

								// Bounding volume
								if (ImGui::TreeNode(u8"Bounding volume"))
								{
									static const char* const KTypes[2]{ u8"BoundingSphere", u8"AxisAlignedBoundingBox" };
									static constexpr int KTypeCount{ ARRAYSIZE(KTypes) };
									static int SelectedBoundingVolumeIndex{};

									if (ImGui::TreeNodeEx(u8"Outer Bounding sphere", ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Outer BS center");
										ImGui::SameLine(ItemsOffsetX);
										float BSCenterOffset[3]{
											XMVectorGetX(Object3D->GetOuterBoundingSphereCenterOffset()),
											XMVectorGetY(Object3D->GetOuterBoundingSphereCenterOffset()),
											XMVectorGetZ(Object3D->GetOuterBoundingSphereCenterOffset()) };
										if (ImGui::DragFloat3(u8"##Outer BS center", BSCenterOffset, KBSCenterOffsetDelta,
											KBSCenterOffsetMinLimit, KBSCenterOffsetMaxLimit, "%.2f"))
										{
											Object3D->SetOuterBoundingSphereCenterOffset(XMVectorSet(BSCenterOffset[0], BSCenterOffset[1], BSCenterOffset[2], 1.0f));
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Outer BS radius bias");
										ImGui::SameLine(ItemsOffsetX);
										float BSRadiusBias{ Object3D->GetOuterBoundingSphereRadiusBias() };
										if (ImGui::DragFloat(u8"##Outer BS radius bias", &BSRadiusBias, KBSRadiusBiasDelta,
											KBSRadiusBiasMinLimit, KBSRadiusBiasMaxLimit, "%.2f"))
										{
											Object3D->SetOuterBoundingSphereRadiusBias(BSRadiusBias);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Outer BS radius (auto)");
										ImGui::SameLine(ItemsOffsetX);
										float BSRadius{ Object3D->GetOuterBoundingSphereRadius() };
										ImGui::DragFloat(u8"##Outer BS radius (auto)", &BSRadius, KBSRadiusDelta,
											KBSRadiusMinLimit, KBSRadiusMaxLimit, "%.2f");

										ImGui::TreePop();
									}

									if (ImGui::TreeNodeEx(u8"Inner bounding volumes", ImGuiTreeNodeFlags_DefaultOpen))
									{
										if (ImGui::Button(u8"추가"))
										{
											Object3D->GetInnerBoundingVolumeVector().emplace_back();
										}

										ImGui::SameLine();
										if (ImGui::Button(u8"제거") && Object3D->HasInnerBoundingVolumes())
										{
											auto& vInnerBoundingVolumes{ Object3D->GetInnerBoundingVolumeVector() };
											if (SelectedBoundingVolumeIndex != (int)(vInnerBoundingVolumes.size() - 1))
											{
												swap(vInnerBoundingVolumes[SelectedBoundingVolumeIndex], vInnerBoundingVolumes.back());
												vInnerBoundingVolumes.pop_back();
											}
											else
											{
												vInnerBoundingVolumes.pop_back();
											}
										}

										int BoundingVolumeIndex{};
										for (const auto& BoundingVolume : Object3D->GetInnerBoundingVolumeVector())
										{
											int Type{ (int)BoundingVolume.eType };
											if (ImGui::Selectable((to_string(BoundingVolumeIndex) + KTypes[Type]).c_str(),
												SelectedBoundingVolumeIndex == BoundingVolumeIndex))
											{
												SelectedBoundingVolumeIndex = BoundingVolumeIndex;
											}
											++BoundingVolumeIndex;
										}

										if (Object3D->HasInnerBoundingVolumes())
										{
											if (ImGui::TreeNode(u8"세부사항 편집"))
											{
												auto& vInnerBoundingVolumes{ Object3D->GetInnerBoundingVolumeVector() };
												if (SelectedBoundingVolumeIndex >= vInnerBoundingVolumes.size())
												{
													SelectedBoundingVolumeIndex = (int)(vInnerBoundingVolumes.size() - 1);
												}
												auto& SelectedBoundingVolume{ vInnerBoundingVolumes[SelectedBoundingVolumeIndex] };

												ImGui::PushItemWidth(300);
												{
													ImGui::AlignTextToFramePadding();
													ImGui::Text(u8"Bounding volume 유형");
													if (ImGui::BeginCombo(u8"##Bounding volume 유형", KTypes[(int)SelectedBoundingVolume.eType]))
													{
														for (int iType = 0; iType < KTypeCount; ++iType)
														{
															if (ImGui::Selectable(KTypes[iType], ((int)SelectedBoundingVolume.eType == iType)))
															{
																SelectedBoundingVolume.eType = (EBoundingVolumeType)iType;
															}
														}
														ImGui::EndCombo();
													}

													XMFLOAT3 Center{};
													XMStoreFloat3(&Center, SelectedBoundingVolume.Center);
													ImGui::AlignTextToFramePadding();
													ImGui::Text(u8"중심");
													if (ImGui::DragFloat3(u8"##Center", &Center.x, 0.1f))
													{
														SelectedBoundingVolume.Center = XMLoadFloat3(&Center);
													}

													if (SelectedBoundingVolume.eType == EBoundingVolumeType::BoundingSphere)
													{
														ImGui::AlignTextToFramePadding();
														ImGui::Text(u8"반지름");
														ImGui::DragFloat(u8"##Radius", &SelectedBoundingVolume.Data.BS.Radius, 0.1f);
													}
													else
													{
														ImGui::AlignTextToFramePadding();
														ImGui::Text(u8"X 절반 크기");
														ImGui::DragFloat(u8"##HalfSizeX", &SelectedBoundingVolume.Data.AABBHalfSizes.x, 0.1f);

														ImGui::AlignTextToFramePadding();
														ImGui::Text(u8"Y 절반 크기");
														ImGui::DragFloat(u8"##HalfSizeY", &SelectedBoundingVolume.Data.AABBHalfSizes.y, 0.1f);

														ImGui::AlignTextToFramePadding();
														ImGui::Text(u8"Z 절반 크기");
														ImGui::DragFloat(u8"##HalfSizeZ", &SelectedBoundingVolume.Data.AABBHalfSizes.z, 0.1f);
													}
												}
												ImGui::PopItemWidth();

												ImGui::TreePop();
											}
										}

										ImGui::TreePop();
									}

									ImGui::TreePop();
								}

								ImGui::Separator();

								// Tessellation data
								if (ImGui::TreeNode(u8"테셀레이션"))
								{
									bool bShouldTessellate{ Object3D->ShouldTessellate() };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"테셀레이션 사용 여부");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::Checkbox(u8"##테셀레이션 사용 여부", &bShouldTessellate))
									{
										Object3D->ShouldTessellate(bShouldTessellate);
									}

									CObject3D::SCBTessFactorData TessFactorData{ Object3D->GetTessFactorData() };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"테셀레이션 변 계수");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::SliderFloat(u8"##테셀레이션 변 계수", &TessFactorData.EdgeTessFactor, 0.0f, 64.0f, "%.2f"))
									{
										Object3D->SetTessFactorData(TessFactorData);
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"테셀레이션 내부 계수");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::SliderFloat(u8"##테셀레이션 내부 계수", &TessFactorData.InsideTessFactor, 0.0f, 64.0f, "%.2f"))
									{
										Object3D->SetTessFactorData(TessFactorData);
									}

									CObject3D::SCBDisplacementData DisplacementData{ Object3D->GetDisplacementData() };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"변위 계수");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::SliderFloat(u8"##변위 계수", &DisplacementData.DisplacementFactor, 0.0f, 1.0f, "%.2f"))
									{
										Object3D->SetDisplacementData(DisplacementData);
									}

									ImGui::TreePop();
								}

								ImGui::Separator();

								// Material data
								if (ImGui::TreeNode(u8"재질"))
								{
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"오브젝트 재질");

									bool bIgnoreSceneMaterial{ Object3D->ShouldIgnoreSceneMaterial() };
									if (ImGui::Checkbox(u8"장면 재질 무시하기", &bIgnoreSceneMaterial))
									{
										Object3D->ShouldIgnoreSceneMaterial(bIgnoreSceneMaterial);
									}

									if (Object3D->GetMaterialCount() > 0)
									{
										static CMaterialData* capturedMaterialData{};
										static CMaterialTextureSet* capturedMaterialTextureSet{};
										static ETextureType ecapturedTextureType{};
										if (!ImGui::IsPopupOpen(u8"텍스처탐색기")) m_EditorGUIBools.bShowPopupMaterialTextureExplorer = false;

										for (size_t iMaterial = 0; iMaterial < Object3D->GetMaterialCount(); ++iMaterial)
										{
											CMaterialData& MaterialData{ Object3D->GetModel().vMaterialData[iMaterial] };
											CMaterialTextureSet* const MaterialTextureSet{ Object3D->GetMaterialTextureSet(iMaterial) };

											ImGui::PushID((int)iMaterial);

											if (DrawEditorGUIWindowPropertyEditor_MaterialData(MaterialData, MaterialTextureSet, ecapturedTextureType, ItemsOffsetX))
											{
												capturedMaterialData = &MaterialData;
												capturedMaterialTextureSet = MaterialTextureSet;
											}

											ImGui::PopID();
										}

										DrawEditorGUIPopupMaterialTextureExplorer(capturedMaterialData, capturedMaterialTextureSet, ecapturedTextureType);
										DrawEditorGUIPopupMaterialNameChanger(capturedMaterialData);
									}

									ImGui::TreePop();
								}

								// Rigged model
								if (Object3D->IsRigged())
								{
									ImGui::Separator();

									if (ImGui::TreeNode(u8"애니메이션"))
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"GPU 스키닝 사용 여부:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::Text((Object3D->IsGPUSkinned() ? u8"예" : u8"아니오"));

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"애니메이션 개수:");
										ImGui::SameLine(ItemsOffsetX);
										int AnimationCount{ static_cast<int>(Object3D->GetAnimationCount()) };
										ImGui::Text(u8"%d", AnimationCount);

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"애니메이션 목록");

										if (ImGui::Button(u8"추가")) bShowAnimationAdder = true;

										ImGui::SameLine();

										if (ImGui::Button(u8"수정")) bShowAnimationEditor = true;

										if (Object3D->CanBakeAnimationTexture())
										{
											ImGui::SameLine();

											if (ImGui::Button(u8"저장"))
											{
												static CFileDialog FileDialog{ GetAssetDirectory() };
												if (FileDialog.SaveFileDialog("애니메이션 텍스처 파일(*.dds)\0*.dds\0", "애니메이션 텍스처 저장", ".dds"))
												{
													Object3D->BakeAnimationTexture();
													Object3D->SaveBakedAnimationTexture(FileDialog.GetRelativeFileName());
												}
											}
										}

										ImGui::SameLine();

										if (ImGui::Button(u8"열기"))
										{
											static CFileDialog FileDialog{ GetAssetDirectory() };
											if (FileDialog.OpenFileDialog("애니메이션 텍스처 파일(*.dds)\0*.dds\0", "애니메이션 텍스처 불러오기"))
											{
												Object3D->LoadBakedAnimationTexture(FileDialog.GetRelativeFileName());
											}
										}

										static constexpr float KAnimationNameWidth{ 200.0f };
										static constexpr float KRegistrationWidth{ 100.0f };
										if (ImGui::ListBoxHeader(u8"##애니메이션 목록", ImVec2(WindowWidth, 0)))
										{
											for (int iAnimation = 0; iAnimation < AnimationCount; ++iAnimation)
											{
												ImGui::PushID(iAnimation);
												{
													if (ImGui::Selectable(Object3D->GetAnimationName(iAnimation).c_str(),
														(iAnimation == iSelectedAnimationID)))
													{
														iSelectedAnimationID = iAnimation;
													}

													ImGui::SameLine(KAnimationNameWidth);
													int RegistrationData{ static_cast<int>(Object3D->GetRegisteredAnimationType(iAnimation)) };
													if (ImGui::Selectable(KRegisteredAnimationTypeNames[RegistrationData],
														(iAnimation == iSelectedAnimationID)))
													{
														iSelectedAnimationID = iAnimation;
													}

													ImGui::SameLine(KAnimationNameWidth + KRegistrationWidth);
													if (ImGui::Selectable(
														(to_string(static_cast<int>(Object3D->GetAnimationTicksPerSecond(iAnimation))) + " TPS").c_str(),
														(iAnimation == iSelectedAnimationID)))
													{
														iSelectedAnimationID = iAnimation;
													}
												}
												ImGui::PopID();
											}

											ImGui::ListBoxFooter();
										}

										if (Object3D->IsInstanced() && (SelectionData.eObjectType == EObjectType::Object3DInstance))
										{
											SObjectIdentifier Identifier{ Object3D, SelectionData.Name.c_str() };
											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"인스턴스 애니메이션 ID");
											ImGui::SameLine(ItemsOffsetX);
											int AnimationID{ (int)Object3D->GetAnimationID(Identifier) };
											if (ImGui::SliderInt(u8"##인스턴스 애니메이션 ID", &AnimationID, 0, AnimationCount - 1))
											{
												Object3D->SetAnimation(SObjectIdentifier(Object3D, SelectionData.Name.c_str()), AnimationID);
											}
										}
										else
										{
											SObjectIdentifier Identifier{ Object3D };
											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"오브젝트 애니메이션 ID");
											ImGui::SameLine(ItemsOffsetX);
											int AnimationID{ (int)Object3D->GetAnimationID(Identifier) };
											if (ImGui::SliderInt(u8"##오브젝트 애니메이션 ID", &AnimationID, 0, AnimationCount - 1))
											{
												Object3D->SetAnimation(SObjectIdentifier(Object3D), AnimationID);
											}
										}

										ImGui::TreePop();
									}
								}
							}
							ImGui::PopItemWidth();


							if (bShowAnimationAdder) ImGui::OpenPopup(u8"애니메이션 추가");
							if (ImGui::BeginPopupModal(u8"애니메이션 추가", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"애니메이션 이름");
								ImGui::SameLine(150);
								static char Name[16]{};
								ImGui::InputText(u8"##애니메이션 이름", Name, 16, ImGuiInputTextFlags_EnterReturnsTrue);

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"파일 이름");
								ImGui::SameLine(150);
								static char FileName[MAX_PATH]{};
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"%s", FileName);

								if (ImGui::Button(u8"불러오기"))
								{
									static CFileDialog FileDialog{ GetAssetDirectory() };
									if (FileDialog.OpenFileDialog("모델 파일(*.fbx)\0*.fbx\0", "애니메이션 불러오기"))
									{
										strcpy_s(FileName, FileDialog.GetRelativeFileName().c_str());
									}
								}

								ImGui::SameLine();

								if (ImGui::Button(u8"결정") || m_CapturedKeyboardState.Enter)
								{
									if (FileName[0] == '\0')
									{
										MB_WARN("파일을 먼저 불러오세요.", "오류");
									}
									else
									{
										CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
										Object3D->AddAnimationFromFile(FileName, Name);

										bShowAnimationAdder = false;
										ImGui::CloseCurrentPopup();
									}
								}

								ImGui::SameLine();

								if (ImGui::Button(u8"취소") || m_CapturedKeyboardState.Escape)
								{
									bShowAnimationAdder = false;
									ImGui::CloseCurrentPopup();
								}

								ImGui::EndPopup();
							}

							if (bShowAnimationEditor) ImGui::OpenPopup(u8"애니메이션 수정");
							if (ImGui::BeginPopupModal(u8"애니메이션 수정", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
							{
								static constexpr float KLabelWidth{ 150.0f };
								static bool bFirstTime{ true };
								CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };

								static char AnimationName[CObject3D::KMaxAnimationNameLength]{};
								static size_t iSelectedRegistration{};
								static float TPS{};
								static float BehaviorStartTick{};
								{
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"애니메이션 이름");
									ImGui::SameLine(KLabelWidth);
									if (bFirstTime)
									{
										strcpy_s(AnimationName, Object3D->GetAnimationName(iSelectedAnimationID).c_str());
										iSelectedRegistration = (size_t)Object3D->GetRegisteredAnimationType(iSelectedAnimationID);
										TPS = Object3D->GetAnimationTicksPerSecond(iSelectedAnimationID);
										BehaviorStartTick = Object3D->GetAnimationBehaviorStartTick(iSelectedAnimationID);

										bFirstTime = false;
									}
									ImGui::InputText(u8"##애니메이션 이름", AnimationName,
										CObject3D::KMaxAnimationNameLength, ImGuiInputTextFlags_EnterReturnsTrue);

									const char* CurrentRegistration{ KRegisteredAnimationTypeNames[iSelectedRegistration] };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"등록 정보");
									ImGui::SameLine(KLabelWidth);
									if (ImGui::BeginCombo(u8"##등록 정보", CurrentRegistration))
									{
										for (size_t iRegistration = 0; iRegistration < ARRAYSIZE(KRegisteredAnimationTypeNames); ++iRegistration)
										{
											if (ImGui::Selectable(KRegisteredAnimationTypeNames[iRegistration], (iRegistration == iSelectedRegistration)))
											{
												iSelectedRegistration = iRegistration;
												CurrentRegistration = KRegisteredAnimationTypeNames[iSelectedRegistration];
											}
										}
										ImGui::EndCombo();
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"TPS");
									ImGui::SameLine(KLabelWidth);
									ImGui::DragFloat(u8"##TPS", &TPS);

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"행동 개시 Tick");
									ImGui::SameLine(KLabelWidth);
									ImGui::DragFloat(u8"##행동 개시 Tick", &BehaviorStartTick);
								}
								
								if (ImGui::Button(u8"결정") || m_CapturedKeyboardState.Enter)
								{
									Object3D->SetAnimationName(iSelectedAnimationID, AnimationName);
									Object3D->SetAnimationTicksPerSecond(iSelectedAnimationID, TPS);
									Object3D->RegisterAnimation(iSelectedAnimationID, (EAnimationRegistrationType)iSelectedRegistration);
									Object3D->SetAnimationBehaviorStartTick(iSelectedAnimationID, BehaviorStartTick);

									bFirstTime = true;
									bShowAnimationEditor = false;
									ImGui::CloseCurrentPopup();
								}

								ImGui::SameLine();

								if (ImGui::Button(u8"취소") || m_CapturedKeyboardState.Escape)
								{
									bFirstTime = true;
									bShowAnimationEditor = false;
									ImGui::CloseCurrentPopup();
								}

								ImGui::EndPopup();
							}

							ImGui::Separator();

							// 인스턴스 인공지능 패턴
							if (SelectionData.eObjectType == EObjectType::Object3DInstance)
							{
								if (ImGui::TreeNodeEx(u8"인공지능"))
								{
									CObject3D* Object3D{ (CObject3D*)SelectionData.PtrObject };
									SObjectIdentifier Identifier{ SObjectIdentifier(Object3D, 
										((const CObject3D*)Object3D)->GetInstanceCPUData(SelectionData.Name).Name.c_str()) };

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"연결된 패턴:");
									ImGui::SameLine(ItemsOffsetX);
									ImGui::AlignTextToFramePadding();
									if (m_Intelligence->HasPattern(Identifier))
									{
										ImGui::Text(m_Intelligence->GetPattern(Identifier)->GetFileName().c_str());
									}
									else
									{
										ImGui::Text(u8"없음");
									}

									// Button
									if (ImGui::Button(u8"패턴 연결")) ImGui::OpenPopup(u8"패턴 목록");

									ImGui::SameLine();

									// Button
									if (ImGui::Button(u8"패턴 연결 해제")) m_Intelligence->DeregisterPattern(Identifier);

									ImGui::SetNextWindowSize(ImVec2(240, 160), ImGuiCond_Appearing);
									if (ImGui::BeginPopupModal(u8"패턴 목록", nullptr))
									{
										static size_t iSelectedPattern{};
										bool bClosing{ false };

										if (ImGui::Button(u8"닫기")) bClosing = true;

										if (m_vPatterns.size())
										{
											ImGui::SameLine();

											if (ImGui::Button(u8"결정"))
											{
												m_Intelligence->RegisterPattern(Identifier, m_vPatterns[iSelectedPattern].get());

												bClosing = true;
											}
										}
										else
										{
											ImGui::Text(u8"패턴이 존재하지 않습니다.");
										}

										size_t iPattern{};
										for (const auto& Pattern : m_vPatterns)
										{
											if (ImGui::Selectable(Pattern->GetFileName().c_str(), iSelectedPattern == iPattern, 
												ImGuiSelectableFlags_DontClosePopups))
											{
												iSelectedPattern = iPattern;
											}
										}

										if (bClosing)
										{
											iSelectedPattern = 0;
											ImGui::CloseCurrentPopup();
										}

										ImGui::EndPopup();
									}

									ImGui::TreePop();
								}
							}

							break;
						}
						case CGame::EObjectType::EditorCamera:
						case CGame::EObjectType::Camera:
						{
							ImGui::PushItemWidth(ItemsWidth);
							{
								CCamera* const SelectedCamera{ 
									(SelectionData.eObjectType == EObjectType::EditorCamera) ? m_EditorCamera.get() : GetCamera(SelectionData.Name) };
								const XMVECTOR& KTranslation{ SelectedCamera->GetTranslation() };
								float Translation[3]{ XMVectorGetX(KTranslation), XMVectorGetY(KTranslation), XMVectorGetZ(KTranslation) };
								float Pitch{ SelectedCamera->GetPitch() };
								float Yaw{ SelectedCamera->GetYaw() };

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"현재 화면 카메라:");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::AlignTextToFramePadding();
								CCamera* const CurrentCamera{ GetCurrentCamera() };
								ImGui::Text(u8"<%s>", CurrentCamera->GetName().c_str());

								if (m_CurrentCameraID != KEditorCameraID)
								{
									ImGui::SetCursorPosX(ItemsOffsetX);
									if (ImGui::Button(u8"에디터 카메라로 돌아가기", ImVec2(ItemsWidth, 0)))
									{
										UseEditorCamera();

										Update3DGizmos();
									}
								}

								ImGui::Separator();

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"선택된 카메라:");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"<%s> %s", SelectedCamera->GetName().c_str(), (IsPlayerCamera(SelectedCamera) ? u8"[P]" : ""));

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"카메라 종류:");
								ImGui::SameLine(ItemsOffsetX);
								CCamera::EType eCameraType{ SelectedCamera->GetType() };
								switch (eCameraType)
								{
								case CCamera::EType::FirstPerson:
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"1인칭 카메라");
									break;
								case CCamera::EType::ThirdPerson:
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"3인칭 카메라");
									break;
								case CCamera::EType::FreeLook:
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"자유 시점 카메라");
									break;
								default:
									break;
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"위치");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat3(u8"##위치", Translation, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									SelectedCamera->TranslateTo(XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f));

									m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());

									Capture3DGizmoTranslation();
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"회전 Pitch");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##회전 Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									SelectedCamera->SetPitch(Pitch);

									m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());

									Capture3DGizmoTranslation();
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"회전 Yaw");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##회전 Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									SelectedCamera->SetYaw(Yaw);

									m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());

									Capture3DGizmoTranslation();
								}

								if (eCameraType == CCamera::EType::ThirdPerson)
								{
									float ZoomDistance{ SelectedCamera->GetZoomDistance() };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"줌 거리");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::DragFloat(u8"##줌 거리", &ZoomDistance, 0.01f, 
										SelectedCamera->GetZoomDistanceMin(), SelectedCamera->GetZoomDistanceMax()))
									{
										SelectedCamera->SetZoomDistance(ZoomDistance);

										m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());
									}
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"카메라 이동 속도");
								ImGui::SameLine(ItemsOffsetX);
								float MovementFactor{ SelectedCamera->GetMovementFactor() };
								if (ImGui::SliderFloat(u8"##카메라 이동 속도", &MovementFactor, 1.0f, 100.0f, "%.0f"))
								{
									SelectedCamera->SetMovementFactor(MovementFactor);
								}

								if (!IsPlayerCamera(SelectedCamera) && !IsEditorCamera(SelectedCamera))
								{
									ImGui::SetCursorPosX(ItemsOffsetX);
									if (ImGui::Button(u8"플레이어 카메라로", ImVec2(ItemsWidth, 0)))
									{
										SetPlayerCamera(SelectedCamera);
									}
								}

								if (!IsCurrentCamera(SelectedCamera))
								{
									ImGui::SetCursorPosX(ItemsOffsetX);
									if (ImGui::Button(u8"현재 화면 카메라로", ImVec2(ItemsWidth, 0)))
									{
										UseCamera(SelectedCamera);

										Update3DGizmos();
									}
								}
							}
							ImGui::PopItemWidth();
							break;
						}
						case CGame::EObjectType::Light:
						{
							ImGui::PushItemWidth(ItemsWidth);
							{
								const auto& SelectedLightCPU{ m_LightArray[SelectionData.Extra]->GetInstanceCPUData(SelectionData.Name) };
								const auto& SelectedLightGPU{ m_LightArray[SelectionData.Extra]->GetInstanceGPUData(SelectionData.Name) };

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"선택된 광원:");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"<%s>", SelectedLightCPU.Name.c_str());

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"광원 종류:");
								ImGui::SameLine(ItemsOffsetX);
								switch (SelectedLightCPU.eType)
								{
								case CLight::EType::PointLight:
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"점 광원");
									break;
								case CLight::EType::SpotLight:
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"SpotLight");
									break;
								default:
									break;
								}

								float Position[3]{ XMVectorGetX(SelectedLightGPU.Position), XMVectorGetY(SelectedLightGPU.Position),
									XMVectorGetZ(SelectedLightGPU.Position) };
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"위치");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat3(u8"##위치", Position, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									m_LightArray[SelectionData.Extra]->SetInstancePosition(SelectionData.Name,
										XMVectorSet(Position[0], Position[1], Position[2], 1.0f));
									m_LightRep->SetInstancePosition(SelectionData.Name, SelectedLightGPU.Position);

									Capture3DGizmoTranslation();
								}

								float Color[3]{ XMVectorGetX(SelectedLightGPU.Color), XMVectorGetY(SelectedLightGPU.Color), XMVectorGetZ(SelectedLightGPU.Color) };
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"색상");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::ColorEdit3(u8"##색상", Color, ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
								{
									m_LightArray[SelectionData.Extra]->SetInstanceColor(SelectionData.Name, 
										XMVectorSet(Color[0], Color[1], Color[2], 1.0f));
								}

								if (SelectedLightCPU.eType == CLight::EType::SpotLight)
								{
									CSpotLight* const SpotLight{ dynamic_cast<CSpotLight*>(m_LightArray[SelectionData.Extra].get()) };

									float Direction[3]{ 
										XMVectorGetX(SelectedLightGPU.Direction), XMVectorGetY(SelectedLightGPU.Direction),
										XMVectorGetZ(SelectedLightGPU.Direction) };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"방향");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::DragFloat3(u8"##방향", Direction, 0.01f, -1.0f, +1.0f, "%.3f"))
									{
										SpotLight->SetInstanceDirection(SelectionData.Name,
											XMVectorSet(Direction[0], Direction[1], Direction[2], 0.0f));
									}
								}

								float Range{ SelectedLightGPU.Range };
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"범위");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##범위", &Range, 0.1f, 1.0f, 20.0f, "%.1f"))
								{
									m_LightArray[SelectionData.Extra]->SetInstanceRange(SelectionData.Name, Range);
								}

								if (SelectedLightCPU.eType == CLight::EType::SpotLight)
								{
									CSpotLight* const SpotLight{ dynamic_cast<CSpotLight*>(m_LightArray[SelectionData.Extra].get()) };

									float Theta{ SelectedLightGPU.Theta };
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"Theta");
									ImGui::SameLine(ItemsOffsetX);
									if (ImGui::DragFloat(u8"##Theta", &Theta, 0.03125f, 0.03125f, XM_PIDIV2, "%.5f"))
									{
										SpotLight->SetInstanceTheta(SelectionData.Name, Theta);
									}
								}
							}
							ImGui::PopItemWidth();
							break;
						}
						}
					}
					else if (m_vSelectionData.size() >= 2)
					{
						ImGui::PushItemWidth(ItemsWidth);
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"위치");
							ImGui::SameLine(ItemsOffsetX);
							XMVECTOR MultipleTranslation{};
							if (ImGui::DragFloat3(u8"##위치", MultipleTranslation.m128_f32))
							{
								if (XMVectorGetX(MultipleTranslation))
								{
									TranslateSelectionTo(EAxis::X, XMVectorGetX(MultipleTranslation));
								}
								else if (XMVectorGetY(MultipleTranslation))
								{
									TranslateSelectionTo(EAxis::Y, XMVectorGetY(MultipleTranslation));
								}
								else
								{
									TranslateSelectionTo(EAxis::Z, XMVectorGetZ(MultipleTranslation));
								}
							}
						}
						ImGui::PopItemWidth();
					}

					ImGui::EndTabItem();
				}

				// 지형 탭
				static bool bShowFoliageClusterGenerator{ false };
				if (ImGui::BeginTabItem(u8"지형"))
				{
					static constexpr float KLabelsWidth{ 220 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };

					SetEditMode(EEditMode::EditTerrain);

					CTerrain* const Terrain{ GetTerrain() };
					if (Terrain)
					{
						static const char* const KModeLabelList[5]{
							u8"<높이 지정 모드>", u8"<높이 변경 모드>", u8"<높이 평균 모드>", u8"<마스킹 모드>", u8"<초목 배치 모드>" };
						static int iSelectedMode{};

						const XMFLOAT2& KTerrainSize{ Terrain->GetSize() };

						ImGui::PushItemWidth(ItemsWidth);
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"가로 x 세로:");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"%d x %d", (int)KTerrainSize.x, (int)KTerrainSize.y);

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"마스킹 디테일:");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"%d", GetTerrain()->GetMaskingDetail());

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"지형 테셀레이션 계수");
							ImGui::SameLine(ItemsOffsetX);
							float TerrainTessFactor{ Terrain->GetTerrainTessFactor() };
							if (ImGui::SliderFloat(u8"##지형 테셀레이션 계수", &TerrainTessFactor,
								CTerrain::KTessFactorMin, CTerrain::KTessFactorMax, "%.1f"))
							{
								Terrain->SetTerrainTessFactor(TerrainTessFactor);
							}

							ImGui::Separator();

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"물 그리기");
							ImGui::SameLine(ItemsOffsetX);
							bool bDrawWater{ Terrain->ShouldDrawWater() };
							if (ImGui::Checkbox(u8"##물 그리기", &bDrawWater))
							{
								Terrain->ShouldDrawWater(bDrawWater);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"물 높이");
							ImGui::SameLine(ItemsOffsetX);
							float WaterHeight{ Terrain->GetWaterHeight() };
							if (ImGui::DragFloat(u8"##물 높이", &WaterHeight, CTerrain::KWaterHeightUnit,
								CTerrain::KWaterMinHeight, CTerrain::KWaterMaxHeight, "%.1f"))
							{
								Terrain->SetWaterHeight(WaterHeight);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"물 테셀레이션 계수");
							ImGui::SameLine(ItemsOffsetX);
							float WaterTessFactor{ Terrain->GetWaterTessFactor() };
							if (ImGui::SliderFloat(u8"##물 테셀레이션 계수", &WaterTessFactor,
								CTerrain::KTessFactorMin, CTerrain::KTessFactorMax, "%.1f"))
							{
								Terrain->SetWaterTessFactor(WaterTessFactor);
							}

							ImGui::Separator();

							for (int iListItem = 0; iListItem < ARRAYSIZE(KModeLabelList); ++iListItem)
							{
								if (ImGui::Selectable(KModeLabelList[iListItem], (iListItem == iSelectedMode) ? true : false))
								{
									iSelectedMode = iListItem;

									switch (iSelectedMode)
									{
									case 0:
										Terrain->SetEditMode(CTerrain::EEditMode::SetHeight);
										break;
									case 1:
										Terrain->SetEditMode(CTerrain::EEditMode::DeltaHeight);
										break;
									case 2:
										Terrain->SetEditMode(CTerrain::EEditMode::AverageHeight);
										break;
									case 3:
										Terrain->SetEditMode(CTerrain::EEditMode::Masking);
										break;
									case 4:
										Terrain->SetEditMode(CTerrain::EEditMode::FoliagePlacing);
										break;
									default:
										break;
									}
								}
							}

							if (iSelectedMode == 0)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"지정할 높이");
								ImGui::SameLine(ItemsOffsetX);
								float TerrainSetHeightValue{ Terrain->GetSetHeightValue() };
								if (ImGui::SliderFloat(u8"##지정할 높이", &TerrainSetHeightValue,
									CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
								{
									Terrain->SetSetHeightValue(TerrainSetHeightValue);
								}
							}
							else if (iSelectedMode == 3)
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"마스킹 레이어");
								ImGui::SameLine(ItemsOffsetX);
								int TerrainMaskingLayer{ (int)Terrain->GetMaskingLayer() };
								if (ImGui::SliderInt(u8"##마스킹 레이어", &TerrainMaskingLayer, 0, CTerrain::KMaterialMaxCount - 2))
								{
									switch (TerrainMaskingLayer)
									{
									case 0:
										Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerR);
										break;
									case 1:
										Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerG);
										break;
									case 2:
										Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerB);
										break;
									case 3:
										Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerA);
										break;
									default:
										break;
									}
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"마스킹 배합 비율");
								ImGui::SameLine(ItemsOffsetX);
								float TerrainMaskingRatio{ Terrain->GetMaskingRatio() };
								if (ImGui::SliderFloat(u8"##마스킹 배합 비율", &TerrainMaskingRatio,
									CTerrain::KMaskingMinRatio, CTerrain::KMaskingMaxRatio, "%.3f"))
								{
									Terrain->SetMaskingRatio(TerrainMaskingRatio);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"마스킹 감쇠");
								ImGui::SameLine(ItemsOffsetX);
								float TerrainMaskingAttenuation{ Terrain->GetMaskingAttenuation() };
								if (ImGui::SliderFloat(u8"##마스킹 감쇠", &TerrainMaskingAttenuation,
									CTerrain::KMaskingMinAttenuation, CTerrain::KMaskingMaxAttenuation, "%.3f"))
								{
									Terrain->SetMaskingAttenuation(TerrainMaskingAttenuation);
								}
							}
							else if (iSelectedMode == 4)
							{
								if (Terrain->HasFoliageCluster())
								{
									ImGui::SetCursorPosX(ItemsOffsetX);
									if (ImGui::Button(u8"초목 클러스터 재생성", ImVec2(ItemsWidth, 0)))
									{
										bShowFoliageClusterGenerator = true;
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"초목 배치 디테일");
									ImGui::SameLine(ItemsOffsetX);
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"%d", Terrain->GetFoliagePlacingDetail());

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"초목 밀도");
									ImGui::SameLine(ItemsOffsetX);
									float FoliageDenstiy{ Terrain->GetFoliageDenstiy() };
									if (ImGui::SliderFloat(u8"##초목밀도", &FoliageDenstiy, 0.0f, 1.0f, "%.2f"))
									{
										Terrain->SetFoliageDensity(FoliageDenstiy);
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"바람 속도");
									ImGui::SameLine(ItemsOffsetX);
									XMFLOAT3 WindVelocity{}; DirectX::XMStoreFloat3(&WindVelocity, Terrain->GetWindVelocity());
									if (ImGui::SliderFloat3(u8"##바람속도", &WindVelocity.x,
										CTerrain::KMinWindVelocityElement, CTerrain::KMaxWindVelocityElement, "%.2f"))
									{
										Terrain->SetWindVelocity(WindVelocity);
										UpdateCBWindData(Terrain->GetWindData());
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"바람 반지름");
									ImGui::SameLine(ItemsOffsetX);
									float WindRadius{ Terrain->GetWindRadius() };
									if (ImGui::SliderFloat(u8"##바람반지름", &WindRadius,
										CTerrain::KMinWindRadius, CTerrain::KMaxWindRadius, "%.2f"))
									{
										Terrain->SetWindRadius(WindRadius);
									}
								}
								else
								{
									ImGui::SetCursorPosX(ItemsOffsetX);
									if (ImGui::Button(u8"초목 클러스터 생성", ImVec2(ItemsWidth, 0)))
									{
										bShowFoliageClusterGenerator = true;
									}
								}
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"선택 반지름");
							ImGui::SameLine(ItemsOffsetX);
							float TerrainMaskingRadius{ Terrain->GetSelectionRadius() };
							if (ImGui::SliderFloat(u8"##선택 반지름", &TerrainMaskingRadius,
								CTerrain::KMinSelectionRadius, CTerrain::KMaxSelectionRadius, "%.1f"))
							{
								Terrain->SetSelectionRadius(TerrainMaskingRadius);
							}
							if (m_CapturedMouseState.scrollWheelValue)
							{
								if (m_CapturedMouseState.scrollWheelValue > 0) TerrainMaskingRadius += CTerrain::KSelectionRadiusUnit;
								if (m_CapturedMouseState.scrollWheelValue < 0) TerrainMaskingRadius -= CTerrain::KSelectionRadiusUnit;
								Terrain->SetSelectionRadius(TerrainMaskingRadius);
							}

							ImGui::Separator();

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 선택 위치:");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::Text(u8"(%.2f, %.2f)", Terrain->GetSelectionPosition().x, Terrain->GetSelectionPosition().y);

							ImGui::Separator();
						}
						
						if (Terrain)
						{
							if (ImGui::Button(u8"재질 추가"))
							{
								Terrain->AddMaterial(CMaterialData());
							}

							static CMaterialData* capturedMaterialData{};
							static CMaterialTextureSet* capturedMaterialTextureSet{};
							static ETextureType ecapturedTextureType{};
							if (!ImGui::IsPopupOpen(u8"텍스처탐색기")) m_EditorGUIBools.bShowPopupMaterialTextureExplorer = false;

							for (size_t iMaterial = 0; iMaterial < Terrain->GetMaterialCount(); ++iMaterial)
							{
								CMaterialData& MaterialData{ Terrain->GetMaterial(iMaterial) };
								CMaterialTextureSet* const MaterialTextureSet{ Terrain->GetMaterialTextureSet(iMaterial) };

								ImGui::PushID((int)iMaterial);

								if (DrawEditorGUIWindowPropertyEditor_MaterialData(MaterialData, MaterialTextureSet, ecapturedTextureType, ItemsOffsetX))
								{
									capturedMaterialData = &MaterialData;
									capturedMaterialTextureSet = MaterialTextureSet;
								}

								ImGui::PopID();
							}

							DrawEditorGUIPopupMaterialTextureExplorer(capturedMaterialData, capturedMaterialTextureSet, ecapturedTextureType);
							DrawEditorGUIPopupMaterialNameChanger(capturedMaterialData);
						}
						else
						{
							ImGui::Text(u8"텍스쳐: 없음");
						}

						ImGui::PopItemWidth();
					}
					else
					{
						ImGui::Text(u8"<먼저 지형을 만들거나 불러오세요.>");
					}

					ImGui::EndTabItem();
				}

				// ### 초목 클러스터 생성기 윈도우 ###
				if (bShowFoliageClusterGenerator) ImGui::OpenPopup(u8"초목 생성기");
				if (ImGui::BeginPopupModal(u8"초목 생성기", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
				{
					static const vector<string> KDefaultFoliages{
						{ "Asset\\basic_grass0.fbx" }, { "Asset\\basic_grass2.fbx" }, { "Asset\\basic_grass3.fbx" }
					};
					static vector<string> vFoliageFileNames{};
					static uint32_t PlacingDetail{ CTerrain::KDefaultFoliagePlacingDetail };
					static bool bUseDefaultFoliages{ false };

					const float KItemsWidth{ 240 };
					ImGui::PushItemWidth(KItemsWidth);
					{
						if (!bUseDefaultFoliages)
						{
							if (ImGui::Button(u8"추가"))
							{
								static CFileDialog FileDialog{ GetAssetDirectory() };
								if (FileDialog.OpenFileDialog("모델 파일(*.fbx)\0*.fbx\0", "초목 오브젝트 불러오기"))
								{
									vFoliageFileNames.emplace_back(FileDialog.GetRelativeFileName());
								}
							}
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"배치 디테일");
						ImGui::SameLine(100);
						ImGui::SetNextItemWidth(KItemsWidth - 100);
						ImGui::SliderInt(u8"##배치 디테일", (int*)&PlacingDetail,
							CTerrain::KMinFoliagePlacingDetail, CTerrain::KMaxFoliagePlacingDetail);

						ImGui::Separator();

						if (ImGui::ListBoxHeader(u8"##lb"))
						{
							if (bUseDefaultFoliages)
							{
								for (const auto& FoliageFileName : KDefaultFoliages)
								{
									ImGui::Text(FoliageFileName.data());
								}
							}
							else
							{
								for (const auto& FoliageFileName : vFoliageFileNames)
								{
									ImGui::Text(FoliageFileName.data());
								}
							}

							ImGui::ListBoxFooter();
						}

						ImGui::Separator();

						if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
						{
							CTerrain* const Terrain{ GetTerrain() };

							if (bUseDefaultFoliages)
							{
								Terrain->CreateFoliageCluster(KDefaultFoliages, PlacingDetail);
							}
							else
							{
								Terrain->CreateFoliageCluster(vFoliageFileNames, PlacingDetail);
							}

							vFoliageFileNames.clear();
							bShowFoliageClusterGenerator = false;
							ImGui::CloseCurrentPopup();
						}

						ImGui::SameLine();

						if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
						{
							vFoliageFileNames.clear();
							bShowFoliageClusterGenerator = false;
							ImGui::CloseCurrentPopup();
						}

						ImGui::SameLine();

						ImGui::Checkbox(u8"기본값으로 생성", &bUseDefaultFoliages);
					}
					ImGui::PopItemWidth();

					ImGui::EndPopup();
				}

				// 장면 탭
				if (ImGui::BeginTabItem(u8"장면"))
				{
					static constexpr float KLabelsWidth{ 240 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };

					ImGui::PushItemWidth(ItemsWidth);
					{

						float FloorHeight{ m_PhysicsEngine.GetWorldFloorHeight() };
						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"World 바닥 높이");
						ImGui::SameLine(ItemsOffsetX);
						if (ImGui::DragFloat(u8"##World 바닥 높이", &FloorHeight, 0.1f))
						{
							m_PhysicsEngine.SetWorldFloorHeight(FloorHeight);
						}

						ImGui::Separator();

						// 장면 재질
						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"장면 재질");
						{
							static CMaterialData* capturedMaterialData{};
							static CMaterialTextureSet* capturedMaterialTextureSet{};
							static ETextureType eCapturedTextureType{};
							if (!ImGui::IsPopupOpen(u8"텍스처탐색기")) m_EditorGUIBools.bShowPopupMaterialTextureExplorer = false;

							if (DrawEditorGUIWindowPropertyEditor_MaterialData(*m_SceneMaterial, m_SceneMaterialTextureSet.get(),
								eCapturedTextureType, ItemsWidth, true))
							{
								capturedMaterialData = m_SceneMaterial.get();
								capturedMaterialTextureSet = m_SceneMaterialTextureSet.get();

								UpdateSceneMaterial();
							}

							if (DrawEditorGUIPopupMaterialTextureExplorer(capturedMaterialData, capturedMaterialTextureSet, eCapturedTextureType))
							{
								UpdateSceneMaterial();
							}

							ImGui::EndTabItem();
						}

						ImGui::Separator();

						// 인공지능 패턴
						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"인공지능 패턴");
						if (ImGui::TreeNodeEx(u8"패턴 목록", ImGuiTreeNodeFlags_DefaultOpen))
						{
							static size_t iSelectedPattern{};

							if (ImGui::Button(u8"불러오기"))
							{
								static CFileDialog FileDialog{ GetAssetDirectory() };
								if (FileDialog.OpenFileDialog("패턴 파일(*.ptrn)\0*.ptrn\0", "인공지능 패턴 파일 불러오기"))
								{
									InsertPattern(FileDialog.GetRelativeFileName());
								}
							}
							
							ImGui::SameLine();

							if (m_vPatterns.size())
							{
								if (ImGui::Button(u8"내용 보기")) ImGui::OpenPopup(u8"패턴 파일 보기");
							}

							ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);
							if (ImGui::BeginPopupModal(u8"패턴 파일 보기", nullptr, ImGuiWindowFlags_NoScrollbar))
							{
								const ImVec2 WindowSize{ ImGui::GetWindowSize() };
								ImVec2 ChildSizeMax{ WindowSize };
								ChildSizeMax.x -= 15;
								ChildSizeMax.y -= 50;

								ImGui::BeginChild(u8"메뉴", ImVec2(ChildSizeMax.x, 30));
								{
									if (ImGui::Button(u8"닫기"))
									{
										ImGui::CloseCurrentPopup();
									}
								}
								ImGui::EndChild();

								ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0, 0, 0, 1.0));
								ImGui::BeginChild(u8"내용", ImVec2(ChildSizeMax.x, ChildSizeMax.y - 30));
								{
									ImGui::Text(m_vPatterns[iSelectedPattern]->GetFileContent().c_str());
								}
								ImGui::EndChild();
								ImGui::PopStyleColor();
								
								ImGui::EndPopup();
							}

							size_t iPattern{};
							for (const auto& Pattern : m_vPatterns)
							{
								if (ImGui::Selectable(Pattern->GetFileName().c_str(), (iSelectedPattern == iPattern)))
								{
									iSelectedPattern = iPattern;
								}
								++iPattern;
							}
						}

						ImGui::TreePop();
					}
					ImGui::PopItemWidth();
				}

				// IBL 탭
				if (ImGui::BeginTabItem(u8"IBL"))
				{
					static constexpr float KLabelsWidth{ 220 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };

					if (ImGui::TreeNodeEx(u8"Environment map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetAssetDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0HDR 파일\0*.hdr\0", "Environment map 불러오기"))
							{
								m_EnvironmentTexture->CreateCubeMapFromFile(FileDialog.GetRelativeFileName());

								if (m_EnvironmentTexture->IsHDRi())
								{
									GenerateEnvironmentCubemapFromHDRi();
								}
							}
						}

						if (m_EnvironmentTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Environment map");

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8" : %s", m_EnvironmentTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetAssetDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.DDS\0", "Environment map 내보내기", ".DDS"))
								{
									m_EnvironmentTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							UpdateCBSpace(KMatrixIdentity, KMatrixIdentity);

							m_EnvironmentCubemapRep->UnfoldCubemap(m_EnvironmentTexture->GetShaderResourceViewPtr());
							SetForwardRenderTargets(); // @important

							ImGui::Image(m_EnvironmentCubemapRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx(u8"Irradiance map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetAssetDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0", "Irradiance map 불러오기"))
							{
								m_IrradianceTexture->CreateCubeMapFromFile(FileDialog.GetRelativeFileName());

								UpdateCBGlobalLightProbeData();
							}
						}

						if (m_EnvironmentTexture)
						{
							static float RangeFactor{ 1.0f };

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"RangeFactor: ");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(60);
							ImGui::DragFloat(u8"##RangeFactor", &RangeFactor, 0.01f, 0.0f, 1.0f, "%.2f");

							ImGui::SameLine();

							if (ImGui::Button(u8"Environment map에서 생성하기"))
							{
								GenerateIrradianceMap(RangeFactor);

								UpdateCBGlobalLightProbeData();
							}
						}

						if (m_IrradianceTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Irradiance map");

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8" : %s", m_IrradianceTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetAssetDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.dds\0", "Irradiance map 내보내기", ".dds"))
								{
									m_IrradianceTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							UpdateCBSpace(KMatrixIdentity, KMatrixIdentity);

							m_IrradianceCubemapRep->UnfoldCubemap(m_IrradianceTexture->GetShaderResourceViewPtr());
							SetForwardRenderTargets(); // @important

							ImGui::Image(m_IrradianceCubemapRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (ImGui::TreeNodeEx(u8"Prefiltered radiance map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetAssetDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0", "Prefiltered radiance map 불러오기"))
							{
								m_PrefilteredRadianceTexture->CreateCubeMapFromFile(FileDialog.GetRelativeFileName());

								UpdateCBGlobalLightProbeData();
							}
						}

						if (m_EnvironmentTexture)
						{
							static float RangeFactor{ 1.0f };

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"RangeFactor: ");
							ImGui::SameLine();
							ImGui::SetNextItemWidth(60);
							ImGui::DragFloat(u8"##RangeFactor", &RangeFactor, 0.01f, 0.1f, 1.0f, "%.2f");

							if (ImGui::Button(u8"Prefiltered radiance map 생성하기"))
							{
								GeneratePrefilteredRadianceMap(RangeFactor);

								UpdateCBGlobalLightProbeData();
							}
						}

						if (m_PrefilteredRadianceTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Prefiltered radiance map");

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8" : %s", m_PrefilteredRadianceTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetAssetDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.DDS\0", "Prefiltered radiance map 내보내기", ".DDS"))
								{
									m_PrefilteredRadianceTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							UpdateCBSpace(KMatrixIdentity, KMatrixIdentity);

							m_PrefilteredRadianceCubemapRep->UnfoldCubemap(m_PrefilteredRadianceTexture->GetShaderResourceViewPtr());
							SetForwardRenderTargets(); // @important

							ImGui::Image(m_PrefilteredRadianceCubemapRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx(u8"Integrated BRDF map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetAssetDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0", "Integrated BRDF map 불러오기"))
							{
								m_IntegratedBRDFTexture->CreateTextureFromFile(FileDialog.GetRelativeFileName(), false);
							}
						}

						ImGui::SameLine();

						if (ImGui::Button(u8"Integrated BRDF map 생성하기"))
						{
							GenerateIntegratedBRDFMap();
						}

						if (m_IntegratedBRDFTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Integrated BRDF map");

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8" : %s", m_IntegratedBRDFTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetAssetDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.DDS\0", "Integrated BRDF map 내보내기", ".DDS"))
								{
									m_IntegratedBRDFTexture->SaveDDSFile(FileDialog.GetRelativeFileName(), true);
								}
							}

							ImGui::Image(m_IntegratedBRDFTexture->GetShaderResourceViewPtr(), ImVec2(256, 256));
						}

						ImGui::TreePop();
					}

					ImGui::EndTabItem();
				}

				// 기타 탭
				if (ImGui::BeginTabItem(u8"기타"))
				{
					const XMVECTOR& KDirectionalLightDirection{ GetDirectionalLightDirection() };
					float DirectionalLightDirection[3]{ XMVectorGetX(KDirectionalLightDirection), XMVectorGetY(KDirectionalLightDirection),
						XMVectorGetZ(KDirectionalLightDirection) };

					static constexpr float KLabelsWidth{ 240 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };
					ImGui::PushItemWidth(ItemsWidth);
					{
						if (ImGui::TreeNodeEx(u8"조명", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"Directional Light 위치");
							ImGui::SameLine(ItemsOffsetX);
							if (ImGui::DragFloat3(u8"##Directional Light 위치", DirectionalLightDirection, 0.02f, -1.0f, +1.0f, "%.2f"))
							{
								SetDirectionalLightDirection(XMVectorSet(DirectionalLightDirection[0], DirectionalLightDirection[1],
									DirectionalLightDirection[2], 0.0f));
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"Directional Light 색상");
							ImGui::SameLine(ItemsOffsetX);
							XMFLOAT3 DirectionalLightColor{ GetDirectionalLightColor() };
							if (ImGui::ColorEdit3(u8"##Directional Light 색상 (HDR)", &DirectionalLightColor.x,
								ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
							{
								SetDirectionalLightColor(DirectionalLightColor);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"Ambient Light 색상");
							ImGui::SameLine(ItemsOffsetX);
							XMFLOAT3 AmbientLightColor{ GetAmbientLightColor() };
							if (ImGui::ColorEdit3(u8"##Ambient Light 색상", &AmbientLightColor.x, ImGuiColorEditFlags_RGB))
							{
								SetAmbientlLight(AmbientLightColor, GetAmbientLightIntensity());
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"Ambient Light 강도");
							ImGui::SameLine(ItemsOffsetX);
							float AmbientLightIntensity{ GetAmbientLightIntensity() };
							if (ImGui::DragFloat(u8"##Ambient Light 강도", &AmbientLightIntensity, 0.02f, 0.0f, +1.0f, "%.2f"))
							{
								SetAmbientlLight(GetAmbientLightColor(), AmbientLightIntensity);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"노출 (HDR)");
							ImGui::SameLine(ItemsOffsetX);
							float Exposure{ GetExposure() };
							if (ImGui::DragFloat(u8"##노출", &Exposure, 0.02f, 0.1f, +10.0f, "%.2f"))
							{
								SetExposure(Exposure);
							}

							ImGui::TreePop();
						}

						ImGui::Separator();

						if (ImGui::TreeNodeEx(u8"FPS", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"Frames per second:");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"%d", m_FPS);

							ImGui::TreePop();
						}

						ImGui::Separator();
						ImGui::Separator();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"에디터 플래그");

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"와이어 프레임");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawWireFrame{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawWireFrame) };
						if (ImGui::Checkbox(u8"##와이어 프레임", &bDrawWireFrame))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawWireFrame);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"법선 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawNormals{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals) };
						if (ImGui::Checkbox(u8"##법선 표시", &bDrawNormals))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawNormals);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"화면 상단에 좌표축 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawMiniAxes{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawMiniAxes) };
						if (ImGui::Checkbox(u8"##화면 상단에 좌표축 표시", &bDrawMiniAxes))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawMiniAxes);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"Bounding Volume 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawBoundingVolumes{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingVolumes) };
						if (ImGui::Checkbox(u8"##Bounding Volume 표시", &bDrawBoundingVolumes))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawBoundingVolumes);
						}


						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"지형 높이맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawTerrainHeightMapTexture{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainHeightMapTexture) };
						if (ImGui::Checkbox(u8"##지형 높이맵 표시", &bDrawTerrainHeightMapTexture))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawTerrainHeightMapTexture);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"지형 마스킹맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawTerrainMaskingTexture{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainMaskingTexture) };
						if (ImGui::Checkbox(u8"##지형 마스킹맵 표시", &bDrawTerrainMaskingTexture))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawTerrainMaskingTexture);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"지형 초목맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawTerrainFoliagePlacingTexture{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainFoliagePlacingTexture) };
						if (ImGui::Checkbox(u8"##지형 초목맵 표시", &bDrawTerrainFoliagePlacingTexture))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawTerrainFoliagePlacingTexture);
						}

						ImGui::Separator();
						ImGui::Separator();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"엔진 플래그");

						/*
						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"조명 적용");
						ImGui::SameLine(ItemsOffsetX);
						bool bUseLighting{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UseLighting) };
						if (ImGui::Checkbox(u8"##조명 적용", &bUseLighting))
						{
							ToggleGameRenderingFlags(EFlagsRendering::UseLighting);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"물리 기반 렌더링 사용");
						ImGui::SameLine(ItemsOffsetX);
						bool bUsePhysicallyBasedRendering{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UsePhysicallyBasedRendering) };
						if (ImGui::Checkbox(u8"##물리 기반 렌더링 사용", &bUsePhysicallyBasedRendering))
						{
							ToggleGameRenderingFlags(EFlagsRendering::UsePhysicallyBasedRendering);
						}
						*/

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"광원 볼륨 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawLightVolumes{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawLightVolumes) };
						if (ImGui::Checkbox(u8"##광원 볼륨 표시", &bDrawLightVolumes))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawLightVolumes);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"3D Gizmo 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bUse3DGizmos{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::Use3DGizmos) };
						if (ImGui::Checkbox(u8"##3D Gizmo 표시", &bUse3DGizmos))
						{
							ToggleRenderingFlag(EFlagsRendering::Use3DGizmos);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"피킹 데이터 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawPickingData{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawPickingData) };
						if (ImGui::Checkbox(u8"##피킹 데이터 표시", &bDrawPickingData))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawPickingData);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"그리드 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawGrid{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawGrid) };
						if (ImGui::Checkbox(u8"##그리드 표시", &bDrawGrid))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawGrid);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"DirectionalLight 그림자맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawDirectionalLightShadowMap{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawDirectionalLightShadowMap) };
						if (ImGui::Checkbox(u8"##DirectionalLight 그림자맵 표시", &bDrawDirectionalLightShadowMap))
						{
							ToggleRenderingFlag(EFlagsRendering::DrawDirectionalLightShadowMap);
						}
					}
					ImGui::PopItemWidth();

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}

		ImGui::End();
	}
}

bool CGame::DrawEditorGUIWindowPropertyEditor_MaterialData(CMaterialData& MaterialData, CMaterialTextureSet* const TextureSet, 
	ETextureType& eSeletedTextureType, float ItemsOffsetX, bool bShowOnlyTextureData)
{
	bool Result{ false };
	bool bUsePhysicallyBasedRendering{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UsePhysicallyBasedRendering) };

	if (ImGui::TreeNodeEx(MaterialData.Name().c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
	{
		if (!bShowOnlyTextureData)
		{
			if (ImGui::Button(u8"재질 이름 변경"))
			{
				m_EditorGUIBools.bShowPopupMaterialNameChanger = true;
				Result = true;
			}

			if (bUsePhysicallyBasedRendering)
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Base color 색상");
			}
			else
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Diffuse 색상");
			}
			ImGui::SameLine(ItemsOffsetX);
			XMFLOAT3 DiffuseColor{ MaterialData.DiffuseColor() };
			if (ImGui::ColorEdit3(u8"##Diffuse 색상", &DiffuseColor.x, ImGuiColorEditFlags_RGB))
			{
				MaterialData.DiffuseColor(DiffuseColor);
			}

			if (!bUsePhysicallyBasedRendering)
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Ambient 색상");
				ImGui::SameLine(ItemsOffsetX);
				XMFLOAT3 AmbientColor{ MaterialData.AmbientColor() };
				if (ImGui::ColorEdit3(u8"##Ambient 색상", &AmbientColor.x, ImGuiColorEditFlags_RGB))
				{
					MaterialData.AmbientColor(AmbientColor);
				}
			}

			if (!bUsePhysicallyBasedRendering)
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Specular 색상");
				ImGui::SameLine(ItemsOffsetX);
				XMFLOAT3 SpecularColor{ MaterialData.SpecularColor() };
				if (ImGui::ColorEdit3(u8"##Specular 색상", &SpecularColor.x, ImGuiColorEditFlags_RGB))
				{
					MaterialData.SpecularColor(SpecularColor);
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Specular 지수");
				ImGui::SameLine(ItemsOffsetX);
				float SpecularExponent{ MaterialData.SpecularExponent() };
				if (ImGui::DragFloat(u8"##Specular 지수", &SpecularExponent, 0.1f, CMaterialData::KSpecularMinExponent, CMaterialData::KSpecularMaxExponent, "%.1f"))
				{
					MaterialData.SpecularExponent(SpecularExponent);
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Specular 강도");
				ImGui::SameLine(ItemsOffsetX);
				float SpecularIntensity{ MaterialData.SpecularIntensity() };
				if (ImGui::DragFloat(u8"##Specular 강도", &SpecularIntensity, 0.01f, 0.0f, 1.0f, "%.2f"))
				{
					MaterialData.SpecularIntensity(SpecularIntensity);
				}
			}

			if (bUsePhysicallyBasedRendering)
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Roughness");
				ImGui::SameLine(ItemsOffsetX);
				float Roughness{ MaterialData.Roughness() };
				if (ImGui::DragFloat(u8"##Roughness", &Roughness, 0.01f, 0.0f, 1.0f, "%.2f"))
				{
					MaterialData.Roughness(Roughness);
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"Metalness");
				ImGui::SameLine(ItemsOffsetX);
				float Metalness{ MaterialData.Metalness() };
				if (ImGui::DragFloat(u8"##Metalness", &Metalness, 0.01f, 0.0f, 1.0f, "%.2f"))
				{
					MaterialData.Metalness(Metalness);
				}
			}

			ImGui::Separator();
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"텍스처");

		if (ImGui::Button(u8"모든 텍스처 해제"))
		{
			MaterialData.ClearAllTexturesData();
			TextureSet->DestroyAllTextures();
			Result = true;
		}

		static const ImVec2 KTextureSmallViewSize{ 60.0f, 60.0f };
		ImGui::PushID(0);
		if (bUsePhysicallyBasedRendering)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Base color");
		}
		else
		{
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Diffuse");
		}
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::DiffuseTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = ETextureType::DiffuseTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::PushID(1);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Normal");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::NormalTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = ETextureType::NormalTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::PushID(2);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Opacity");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::OpacityTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = ETextureType::OpacityTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::PushID(3);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Specular Intensity");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::SpecularIntensityTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = ETextureType::SpecularIntensityTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		if (bUsePhysicallyBasedRendering)
		{
			ImGui::PushID(4);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Roughness");
			ImGui::SameLine(ItemsOffsetX);
			if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::RoughnessTexture) : nullptr, KTextureSmallViewSize))
			{
				eSeletedTextureType = ETextureType::RoughnessTexture;
				m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
				Result = true;
			}
			ImGui::PopID();

			ImGui::PushID(5);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Metalness");
			ImGui::SameLine(ItemsOffsetX);
			if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::MetalnessTexture) : nullptr, KTextureSmallViewSize))
			{
				eSeletedTextureType = ETextureType::MetalnessTexture;
				m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
				Result = true;
			}
			ImGui::PopID();

			ImGui::PushID(6);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Ambient Occlusion");
			ImGui::SameLine(ItemsOffsetX);
			if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::AmbientOcclusionTexture) : nullptr, KTextureSmallViewSize))
			{
				eSeletedTextureType = ETextureType::AmbientOcclusionTexture;
				m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
				Result = true;
			}
			ImGui::PopID();
		}

		ImGui::PushID(7);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Displacement");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(ETextureType::DisplacementTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = ETextureType::DisplacementTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::TreePop();
	}

	return Result;
}

void CGame::DrawEditorGUIPopupMaterialNameChanger(CMaterialData*& capturedMaterialData)
{
	static char OldName[KAssetNameMaxLength]{};
	static char NewName[KAssetNameMaxLength]{};

	// ### 재질 이름 변경 윈도우 ###
	if (m_EditorGUIBools.bShowPopupMaterialNameChanger) ImGui::OpenPopup(u8"재질 이름 변경");

	ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_Always);
	if (ImGui::BeginPopupModal(u8"재질 이름 변경", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::SetNextItemWidth(160);
		bool bOK{ ImGui::InputText(u8"새 이름", NewName, KAssetNameMaxLength, ImGuiInputTextFlags_EnterReturnsTrue) };

		ImGui::Separator();

		if (ImGui::Button(u8"결정") || bOK)
		{
			strcpy_s(OldName, capturedMaterialData->Name().c_str());

			capturedMaterialData->Name(NewName);

			ImGui::CloseCurrentPopup();
			m_EditorGUIBools.bShowPopupMaterialNameChanger = false;
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
		{
			ImGui::CloseCurrentPopup();
			m_EditorGUIBools.bShowPopupMaterialNameChanger = false;
			capturedMaterialData = nullptr;
		}

		ImGui::EndPopup();
	}
}

bool CGame::DrawEditorGUIPopupMaterialTextureExplorer(CMaterialData* const capturedMaterialData, CMaterialTextureSet* const capturedMaterialTextureSet,
	ETextureType eSelectedTextureType)
{
	bool bResult{ false };

	// ### 텍스처 탐색기 윈도우 ###
	if (m_EditorGUIBools.bShowPopupMaterialTextureExplorer) ImGui::OpenPopup(u8"텍스처탐색기");
	if (ImGui::BeginPopup(u8"텍스처탐색기", ImGuiWindowFlags_AlwaysAutoResize))
	{
		ID3D11ShaderResourceView* SRV{};
		if (capturedMaterialTextureSet) SRV = capturedMaterialTextureSet->GetTextureSRV(eSelectedTextureType);

		if (ImGui::Button(u8"파일에서 텍스처 불러오기"))
		{
			static CFileDialog FileDialog{ GetAssetDirectory() };
			if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
			{
				capturedMaterialData->SetTextureFileName(eSelectedTextureType, FileDialog.GetRelativeFileName());
				capturedMaterialTextureSet->CreateTexture(eSelectedTextureType, *capturedMaterialData);

				bResult = true;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"텍스처 해제하기"))
		{
			capturedMaterialData->ClearTextureData(eSelectedTextureType);
			capturedMaterialTextureSet->DestroyTexture(eSelectedTextureType);

			bResult = true;
		}

		ImGui::Image(SRV, ImVec2(600, 600));

		ImGui::EndPopup();
	}

	return bResult;
}

void CGame::DrawEditorGUIWindowSceneEditor()
{
	// ### 장면 편집기 윈도우 ###
	if (m_EditorGUIBools.bShowWindowSceneEditor)
	{
		const auto& mapObject3D{ GetObject3DMap() };
		const auto& mapObject2D{ GetObject2DMap() };
		const auto& mapCamera{ GetCameraMap() };

		ImGui::SetNextWindowPos(ImVec2(0, 122), ImGuiCond_Appearing);
		ImGui::SetNextWindowSizeConstraints(ImVec2(400, 60), ImVec2(500, 600));
		if (ImGui::Begin(u8"장면 편집기", &m_EditorGUIBools.bShowWindowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
		{
			// 장면 내보내기
			if (ImGui::Button(u8"장면 비우기"))
			{
				EmptyScene();
			}

			ImGui::SameLine();

			// 장면 내보내기
			if (ImGui::Button(u8"장면 내보내기"))
			{
				static CFileDialog FileDialog{ GetSceneDirectory() };
				if (FileDialog.SaveFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 내보내기", ".scene"))
				{
					SaveScene(FileDialog.GetRelativeFileName(), "Scene\\" + FileDialog.GetFileNameWithoutExt() + '\\');
				}
			}

			ImGui::SameLine();

			// 장면 불러오기
			if (ImGui::Button(u8"장면 불러오기"))
			{
				static CFileDialog FileDialog{ GetSceneDirectory() };
				
				if (FileDialog.OpenFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 불러오기"))
				{
					LoadScene(FileDialog.GetRelativeFileName(), "Scene\\" + FileDialog.GetFileNameWithoutExt() + '\\');
				}
			}

			ImGui::Separator();

			// 오브젝트 추가
			if (ImGui::Button(u8"오브젝트 추가"))
			{
				m_EditorGUIBools.bShowPopupObjectAdder = true;
			}

			// 오브젝트 저장
			if (ImGui::Button(u8"오브젝트 저장"))
			{
				static CFileDialog FileDialog{ GetAssetDirectory() };
				if (m_vSelectionData.size() == 1)
				{
					const SSelectionData& SelectionData{ m_vSelectionData.back() };
					if (SelectionData.eObjectType == EObjectType::Object3D)
					{
						if (FileDialog.SaveFileDialog("MESH 파일(*.mesh)\0*.mesh\0OB3D 파일(*.ob3d)\0*.ob3d\0", "3D 오브젝트 저장하기", ".mesh"))
						{
							CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
							if (FileDialog.GetCapitalExtension() == "MESH")
							{
								Object3D->ExportEmbeddedTextures("Asset\\");
								m_MeshPorter.ExportMESH(FileDialog.GetFileName(), Object3D->GetModel());
								Object3D->SetModelFileName(FileDialog.GetRelativeFileName());
							}
							else
							{
								Object3D->SaveOB3D(FileDialog.GetFileName());
							}
						}
					}
				}
			}

			// 오브젝트 제거
			if (ImGui::Button(u8"오브젝트 제거"))
			{
				DeleteSelectedObjects();
			}

			ImGui::Separator();

			ImGui::Columns(2);
			ImGui::Text(u8"오브젝트 및 인스턴스"); ImGui::NextColumn();
			ImGui::Text(u8"할일"); ImGui::NextColumn();
			ImGui::Separator();

			static CObject3D* CapturedObject3D{};
			static string CapturedInstanceName{};
			if (ImGui::TreeNodeEx(u8"3D 오브젝트", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 3D 오브젝트 목록
				int iObject3DPair{};
				for (const auto& Object3DPair : mapObject3D)
				{
					CObject3D* const Object3D{ GetObject3D(Object3DPair.first) };
					bool bIsThisObject3DSelected{ false };
					if (m_umapSelectionObject3D.find(Object3DPair.first) != m_umapSelectionObject3D.end())
					{
						bIsThisObject3DSelected = true;
					}

					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth };
					if (bIsThisObject3DSelected) Flags |= ImGuiTreeNodeFlags_Selected;
					if (!Object3D->IsInstanced()) Flags |= ImGuiTreeNodeFlags_Leaf;

					if (!Object3D->IsInstanced()) ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

					string suffix{};
					EObjectRole eRole{ m_PhysicsEngine.GetObjectRole(Object3D) };
					if (eRole == EObjectRole::Player)
					{
						suffix = " [P]";
					}
					else if (eRole == EObjectRole::Environment)
					{
						suffix = " [E]";
					}
					else if (eRole == EObjectRole::Monster)
					{
						suffix = " [M]";
					}

					bool bIsNodeOpen{ ImGui::TreeNodeEx((Object3DPair.first + suffix).c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectObject(SSelectionData(EObjectType::Object3D, Object3DPair.first, Object3D));
					}

					if (bIsNodeOpen)
					{
						// 인스턴스 목록
						if (Object3D->IsInstanced())
						{
							for (const auto& InstancePair : Object3D->GetInstanceNameToIndexMap())
							{
								bool bSelected{ false };
								if (m_umapSelectionObject3D.find(Object3D->GetName() + InstancePair.first) != m_umapSelectionObject3D.end())
								{
									bSelected = true;
								}

								if (ImGui::Selectable(InstancePair.first.c_str(), bSelected))
								{
									SelectObject(SSelectionData(EObjectType::Object3DInstance, InstancePair.first, Object3D));
								}
							}
						}

						ImGui::TreePop();
					}

					++iObject3DPair;
					if (!Object3D->IsInstanced()) ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
				}

				ImGui::NextColumn();

				if (m_vSelectionData.size() == 1)
				{
					const SSelectionData& SelectionData{ m_vSelectionData.back() };
					CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };

					if ((SelectionData.eObjectType == EObjectType::Object3D || SelectionData.eObjectType == EObjectType::Object3DInstance))
					{
						// 인스턴스 추가

						ImGui::PushID(iObject3DPair * 2 + 0);
						if (ImGui::Button(u8"+Inst"))
						{
							Object3D->InsertInstance();

							SelectObject(SSelectionData(EObjectType::Object3DInstance, Object3D->GetLastInstanceName(), Object3D));
						}
						ImGui::PopID();
					}

					if (SelectionData.eObjectType == EObjectType::Object3DInstance)
					{
						// 인스턴스 제거

						ImGui::SameLine();
						ImGui::PushID(iObject3DPair * 2 + 1);
						if (ImGui::Button(u8"-Inst"))
						{
							Object3D->DeleteInstance(SelectionData.Name);
							if (Object3D->GetInstanceCount() > 0)
							{
								SelectObject(SSelectionData(EObjectType::Object3DInstance, Object3D->GetLastInstanceName(), Object3D));
							}
							else
							{
								DeselectAll();
							}
						}
						ImGui::PopID();
					}

					ImGui::SameLine();
					ImGui::PushID(iObject3DPair * 2 + 2);
					if (ImGui::Button(u8"이름 변경"))
					{
						if (SelectionData.eObjectType == EObjectType::Object3DInstance)
						{
							CapturedInstanceName = SelectionData.Name;
						}
						else
						{
							CapturedInstanceName.clear();
						}

						m_EditorGUIBools.bShowPopupObjectRenamer = true;
						CapturedObject3D = Object3D;
					}
					ImGui::PopID();
				}

				ImGui::NextColumn();

				ImGui::TreePop();
			}

			if (m_EditorGUIBools.bShowPopupObjectRenamer) ImGui::OpenPopup(u8"오브젝트/인스턴스 이름 변경");
			ImGui::SetNextWindowSize(ImVec2(240, 60), ImGuiCond_Always);
			if (ImGui::BeginPopupModal(u8"오브젝트/인스턴스 이름 변경", nullptr, ImGuiWindowFlags_NoResize))
			{
				static char NewName[KAssetNameMaxLength]{};
				static bool bOK{ false };

				ImGui::AlignTextToFramePadding();
				ImGui::Text(u8"새 이름: ");
				ImGui::SameLine(80);
				if (!ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
				bOK = ImGui::InputText(u8"##이름", NewName, KAssetNameMaxLength, ImGuiInputTextFlags_EnterReturnsTrue);
				
				if (bOK && NewName[0] != '\0')
				{
					if (CapturedInstanceName.empty())
					{
						if (ChangeObject3DName(CapturedObject3D->GetName(), NewName))
						{
							DeselectAll();

							m_EditorGUIBools.bShowPopupObjectRenamer = false;
							memset(NewName, 0, KAssetNameMaxLength);
							ImGui::CloseCurrentPopup();
						}
					}
					else
					{
						if (CapturedObject3D->ChangeInstanceName(CapturedInstanceName, NewName))
						{
							DeselectAll();

							m_EditorGUIBools.bShowPopupObjectRenamer = false;
							memset(NewName, 0, KAssetNameMaxLength);
							ImGui::CloseCurrentPopup();
						}
					}
					
					bOK = false;
				}
				
				if (m_CapturedKeyboardState.Escape)
				{
					m_EditorGUIBools.bShowPopupObjectRenamer = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
			
			if (ImGui::TreeNodeEx(u8"2D 오브젝트", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 2D 오브젝트 목록
				int iObject2DPair{};
				for (const auto& Object2DPair : mapObject2D)
				{
					CObject2D* const Object2D{ GetObject2D(Object2DPair.first) };
					bool bIsThisObject2DSelected{ false };
					if (m_umapSelectionObject2D.find(Object2DPair.first) != m_umapSelectionObject2D.end())
					{
						bIsThisObject2DSelected = true;
					}
					
					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth };
					if (bIsThisObject2DSelected) Flags |= ImGuiTreeNodeFlags_Selected;
					if (!Object2D->IsInstanced()) Flags |= ImGuiTreeNodeFlags_Leaf;

					if (!Object2D->IsInstanced()) ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

					bool bIsNodeOpen{ ImGui::TreeNodeEx(Object2DPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectObject(SSelectionData(EObjectType::Object2D, Object2DPair.first, Object2D));
					}
					if (bIsNodeOpen)
					{
						ImGui::TreePop();
					}

					++iObject2DPair;
					if (!Object2D->IsInstanced()) ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx(u8"카메라", ImGuiTreeNodeFlags_DefaultOpen))
			{
				// 카메라 목록
				for (const auto& CameraPair : mapCamera)
				{
					CCamera* const Camera{ GetCamera(CameraPair.first) };
					bool bIsThisCameraSelected{ false };
					if (m_umapSelectionCamera.find(CameraPair.first) != m_umapSelectionCamera.end())
					{
						bIsThisCameraSelected = true;
					}

					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf };
					if (bIsThisCameraSelected) Flags |= ImGuiTreeNodeFlags_Selected;

					ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
					string NodeName{ CameraPair.first };
					if (IsPlayerCamera(Camera))
					{
						NodeName += " [P]";
					}
					bool bIsNodeOpen{ ImGui::TreeNodeEx(NodeName.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectObject(SSelectionData(EObjectType::Camera, CameraPair.first));
					}
					if (bIsNodeOpen)
					{
						ImGui::TreePop();
					}
					ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
				}

				// Editor camera
				ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
				if (ImGui::TreeNodeEx(m_EditorCamera->GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf |
					((m_EditorCameraSelectionIndex < m_vSelectionData.size()) ? ImGuiTreeNodeFlags_Selected : 0)))
				{
					if (ImGui::IsItemClicked())
					{
						SelectObject(SSelectionData(EObjectType::EditorCamera));
					}
					ImGui::TreePop();
				}
				ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx(u8"광원", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (const auto& Light : m_LightArray)
				{
					CLight::EType eType{ Light->GetType() };
					for (const auto& LightPair : Light->GetInstanceNameToIndexMap())
					{
						bool bIsThisLightSelected{ false };
						if (m_umapSelectionLight.find(LightPair.first) != m_umapSelectionLight.end())
						{
							bIsThisLightSelected = true;
						}

						ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf };
						if (bIsThisLightSelected) Flags |= ImGuiTreeNodeFlags_Selected;

						ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
						bool bIsNodeOpen{ ImGui::TreeNodeEx(LightPair.first.c_str(), Flags) };
						if (ImGui::IsItemClicked())
						{
							SelectObject(SSelectionData(EObjectType::Light, LightPair.first, (uint32_t)eType));
						}
						if (bIsNodeOpen)
						{
							ImGui::TreePop();
						}
						ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}
}

void CGame::GenerateEnvironmentCubemapFromHDRi()
{
	D3D11_TEXTURE2D_DESC HDRiDesc{};
	m_EnvironmentTexture->GetTexture2DPtr()->GetDesc(&HDRiDesc);

	m_IBLBaker->ConvertHDRiToCubemap(XMFLOAT2(static_cast<FLOAT>(HDRiDesc.Width), static_cast<FLOAT>(HDRiDesc.Height)), m_EnvironmentTexture.get());

	m_EnvironmentTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_EnvironmentTexture->CopyTexture(m_IBLBaker->GetBakedTexture());
	m_EnvironmentTexture->SetSlot(KEnvironmentTextureSlot);

	SetForwardRenderTargets();
}

void CGame::GenerateIrradianceMap(float RangeFactor)
{
	m_IBLBaker->GenerateIrradianceMap(m_EnvironmentTexture.get(), 3, RangeFactor);
	
	m_IrradianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_IrradianceTexture->CopyTexture(m_IBLBaker->GetBakedTexture());
	m_IrradianceTexture->SetSlot(KIrradianceTextureSlot);

	SetForwardRenderTargets();
}

void CGame::GeneratePrefilteredRadianceMap(float RangeFactor)
{
	m_IBLBaker->GeneratePrefilteredRadianceMap(m_EnvironmentTexture.get(), 3, RangeFactor);

	m_PrefilteredRadianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_PrefilteredRadianceTexture->CopyTexture(m_IBLBaker->GetBakedTexture());
	m_PrefilteredRadianceTexture->SetSlot(KPrefilteredRadianceTextureSlot);

	SetForwardRenderTargets();
}

void CGame::GenerateIntegratedBRDFMap()
{
	m_IBLBaker->GenerateIntegratedBRDF(XMFLOAT2(512, 512));

	m_IntegratedBRDFTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_IntegratedBRDFTexture->CopyTexture(m_IBLBaker->GetBakedTexture());
	m_IntegratedBRDFTexture->SetSlot(KIntegratedBRDFTextureSlot);

	SetForwardRenderTargets();
}

void CGame::EndRendering()
{
	if (m_IsDestroyed) return;

	// Edge detection
	if (m_eMode == EMode::Edit)
	{
		m_bIsDeferredRenderTargetsSet = false;

		m_DeviceContext->OMSetRenderTargets(1, m_EdgeDetectorRTV.GetAddressOf(), m_GBuffers.DepthStencilDSV.Get());
		m_DeviceContext->ClearRenderTargetView(m_EdgeDetectorRTV.Get(), Colors::Transparent);
		m_DeviceContext->ClearDepthStencilView(m_GBuffers.DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (IsAnythingSelected())
		{
			for (const auto& SelectionData : m_vSelectionData)
			{
				if (SelectionData.eObjectType == EObjectType::Object3D || SelectionData.eObjectType == EObjectType::Object3DInstance)
				{
					CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };

					if (Object3D->IsRigged())
					{
						Object3D->Animate(0);
						UpdateCBAnimationBoneMatrices(Object3D->GetAnimationBoneMatrices());
						UpdateCBAnimationData(Object3D->GetAnimationData());
					}

					if (SelectionData.eObjectType == EObjectType::Object3DInstance)
					{
						size_t InstanceIndex{ Object3D->GetInstanceIndex(SelectionData.Name) };
						DrawObject3D(Object3D, EFlagsObject3DRendering::DrawOneInstance, InstanceIndex);
					}
					else
					{
						Object3D->UpdateWorldMatrix();
						DrawObject3D(Object3D);
					}
				}
			}
		}

		SetForwardRenderTargets();

		m_DeviceContext->PSSetShaderResources(0, 1, m_EdgeDetectorSRV.GetAddressOf());

		m_EdgeDetectorFSQ->SetIA();
		m_EdgeDetectorFSQ->SetShaders();
		m_EdgeDetectorFSQ->SetSampler(CFullScreenQuad::ESamplerState::PointClamp);
		m_EdgeDetectorFSQ->Draw2D();
	}

	if (m_eMode != EMode::Play)
	{
		m_DeviceContext->ClearDepthStencilView(m_GBuffers.DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (m_eMode == EMode::Edit)
		{
			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::Use3DGizmos))
			{
				Draw3DGizmos();
			}
		}

		DrawEditorGUI();
	}

	m_SwapChain->Present(0, 0);

	m_bLeftButtonPressedOnce = false;
	m_bLeftButtonUpOnce = false;
}

void CGame::SetForwardRenderTargets(bool bClearViews)
{
	m_DeviceContext->OMSetRenderTargets(1, m_BackBufferRTV.GetAddressOf(), m_GBuffers.DepthStencilDSV.Get());

	if (bClearViews)
	{
		m_DeviceContext->ClearRenderTargetView(m_BackBufferRTV.Get(), Colors::CornflowerBlue);
		m_DeviceContext->ClearDepthStencilView(m_GBuffers.DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	m_bIsDeferredRenderTargetsSet = false;
}

void CGame::SetDeferredRenderTargets(bool bClearViews)
{
	ID3D11RenderTargetView* RTVs[]{ m_GBuffers.BaseColorRoughRTV.Get(), m_GBuffers.NormalRTV.Get(), m_GBuffers.MetalAORTV.Get() };

	m_DeviceContext->OMSetRenderTargets(ARRAYSIZE(RTVs), RTVs, m_GBuffers.DepthStencilDSV.Get());

	if (bClearViews)
	{
		for (auto& RTV : RTVs)
		{
			m_DeviceContext->ClearRenderTargetView(RTV, Colors::Transparent); // @important
		}
		m_DeviceContext->ClearDepthStencilView(m_GBuffers.DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	m_bIsDeferredRenderTargetsSet = true;
}

auto CGame::GethWnd() const -> HWND
{
	return m_hWnd;
}

auto CGame::GetDevicePtr() const -> ID3D11Device*
{
	return m_Device.Get();
}

auto CGame::GetDeviceContextPtr() const -> ID3D11DeviceContext*
{
	return m_DeviceContext.Get();
}

Keyboard::State CGame::GetKeyState() const
{
	return m_Keyboard->GetState();
}

Mouse::State CGame::GetMouseState() const
{ 
	Mouse::State ResultState{ m_Mouse->GetState() };
	
	m_Mouse->ResetScrollWheelValue();

	return ResultState;
}

auto CGame::GetSpriteBatchPtr() const -> SpriteBatch*
{
	return m_SpriteBatch.get();
}

auto CGame::GetSpriteFontPtr() const -> SpriteFont*
{
	return m_SpriteFont.get();
}

const XMFLOAT2& CGame::GetWindowSize() const
{
	return m_WindowSize;
}

auto CGame::GetDepthStencilStateLessEqualNoWrite() const -> ID3D11DepthStencilState*
{
	return m_DepthStencilStateLessEqualNoWrite.Get();
}

auto CGame::GetBlendStateAlphaToCoverage() const -> ID3D11BlendState*
{
	return m_BlendAlphaToCoverage.Get();
}

auto CGame::GetDeltaTime() const -> float
{
	return m_DeltaTime_s;
}
