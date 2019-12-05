#include <thread>

#include "Game.h"
#include "Core/FileDialog.h"

using std::max;
using std::min;
using std::vector;
using std::string;
using std::wstring;
using std::thread;
using std::chrono::steady_clock;
using std::to_string;
using std::stof;
using std::make_unique;
using std::swap;

static constexpr D3D11_INPUT_ELEMENT_DESC KBaseInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "BLEND_INDICES"	, 0, DXGI_FORMAT_R32G32B32A32_UINT	, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLEND_WEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 1, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "INSTANCE_WORLD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCE_WORLD"	, 1, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCE_WORLD"	, 2, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCE_WORLD"	, 3, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "IS_HIGHLIGHTED"	, 0, DXGI_FORMAT_R32_FLOAT			, 2, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

// FOR DEBUGGING SHADER...
static constexpr D3D11_INPUT_ELEMENT_DESC KScreenQuadInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void CGame::CreateWin32(WNDPROC const WndProc, const std::string& WindowName, bool bWindowed)
{
	GetCurrentDirectoryA(MAX_PATH, m_WorkingDirectory);

	CreateWin32Window(WndProc, WindowName);

	InitializeDirectX(bWindowed);

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
	CreateSwapChain(bWindowed);

	CreateViews();

	CreateDepthStencilStates();
	CreateBlendStates();

	CreateInputDevices();

	CreateConstantBuffers();
	CreateBaseShaders();

	CreateMiniAxes();
	CreatePickingRay();
	CreatePickedTriangle();
	CreateBoundingSphere();
	Create3DGizmos();

	CreateScreenQuadVertexBuffer();
	CreateCubemapVertexBuffer();

	SetProjectionMatrices(KDefaultFOV, KDefaultNearZ, KDefaultFarZ);
	InitializeViewports();

	m_CommonStates = make_unique<CommonStates>(m_Device.Get());
}

void CGame::InitializeEditorAssets()
{
	if (!m_CameraRep)
	{
		m_CameraRep = make_unique<CObject3D>("CameraRepresentation", m_Device.Get(), m_DeviceContext.Get(), this);
		m_CameraRep->CreateFromFile("Asset\\camera_repr.fbx", false);
	}

	if (!m_Light)
	{
		m_Light = make_unique<CLight>(m_Device.Get(), m_DeviceContext.Get());
	}

	if (!m_LightRep)
	{
		m_LightRep = make_unique<CBillboard>("LightRepresentation", m_Device.Get(), m_DeviceContext.Get());
		m_LightRep->CreateWorldSpace("Asset\\light.png", XMFLOAT2(0.5f, 0.5f));
	}

	if (!m_EditorCamera)
	{
		m_EditorCamera = make_unique<CCamera>("EditorCamera");
		m_EditorCamera->SetData(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0)));
		m_EditorCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));
		m_EditorCamera->SetMovementFactor(KEditorCameraDefaultMovementFactor);
		
		UseEditorCamera();
	}

	if (!m_MultipleSelectionRep)
	{
		m_MultipleSelectionRep = make_unique<CObject2D>("RegionSelectionRep", m_Device.Get(), m_DeviceContext.Get());
		CObject2D::SModel2D Model2D{ Generate2DRectangle(XMFLOAT2(1, 1)) };
		for (auto& Vertex : Model2D.vVertices)
		{
			Vertex.Color = XMVectorSet(0, 0.25f, 1.0f, 0.5f);
		}
		m_MultipleSelectionRep->Create(Model2D);
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

	if (!m_EnvironmentRep) m_EnvironmentRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get(), this);
	if (!m_IrradianceRep) m_IrradianceRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get(), this);
	if (!m_PrefilteredRadianceRep) m_PrefilteredRadianceRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get(), this);
	
	if (!m_Grid)
	{
		m_Grid = make_unique<CObject3DLine>("Grid", m_Device.Get(), m_DeviceContext.Get());
		m_Grid->Create(Generate3DGrid(0));
	}

	{
		CMaterialData MaterialData{ "test" };
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\test_diffuse.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, "Asset\\test_normal.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, "Asset\\test_displacement.jpg");

		InsertMaterialCreateTextures(MaterialData);
	}

	{
		CMaterialData MaterialData{ "cobblestone_large" };
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\cobblestone_large_base_color.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, "Asset\\cobblestone_large_normal.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::RoughnessTexture, "Asset\\cobblestone_large_roughness.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::AmbientOcclusionTexture, "Asset\\cobblestone_large_occlusion.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, "Asset\\cobblestone_large_displacement.jpg");

		InsertMaterialCreateTextures(MaterialData);
	}

	{
		CMaterialData MaterialData{ "burned_ground" };
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\burned_ground_base_color.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, "Asset\\burned_ground_normal.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::SpecularIntensityTexture, "Asset\\burned_ground_specular.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::RoughnessTexture, "Asset\\burned_ground_roughness.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::MetalnessTexture, "Asset\\burned_ground_specular.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::AmbientOcclusionTexture, "Asset\\burned_ground_occlusion.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, "Asset\\burned_ground_displacement.jpg");

		InsertMaterialCreateTextures(MaterialData);
	}

	{
		CMaterialData MaterialData{ "brown_mud_dry" };
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\brown_mud_dry_base_color.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, "Asset\\brown_mud_dry_normal.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::SpecularIntensityTexture, "Asset\\brown_mud_dry_specular.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::RoughnessTexture, "Asset\\brown_mud_dry_roughness.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::MetalnessTexture, "Asset\\brown_mud_dry_specular.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::AmbientOcclusionTexture, "Asset\\brown_mud_dry_occlusion.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, "Asset\\brown_mud_dry_displacement.jpg");

		InsertMaterialCreateTextures(MaterialData);
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

void CGame::CreateSwapChain(bool bWindowed)
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

void CGame::CreateViews()
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

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_DepthStencilBuffer.ReleaseAndGetAddressOf());

		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc{};
		DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &DSVDesc, m_DepthStencilDSV.ReleaseAndGetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_Device->CreateShaderResourceView(m_DepthStencilBuffer.Get(), &SRVDesc, m_DepthStencilSRV.ReleaseAndGetAddressOf());
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

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_BaseColorRoughBuffer.ReleaseAndGetAddressOf());
		m_Device->CreateRenderTargetView(m_BaseColorRoughBuffer.Get(), nullptr, m_BaseColorRoughRTV.ReleaseAndGetAddressOf());
		m_Device->CreateShaderResourceView(m_BaseColorRoughBuffer.Get(), nullptr, m_BaseColorRoughSRV.ReleaseAndGetAddressOf());
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

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_NormalBuffer.ReleaseAndGetAddressOf());
		m_Device->CreateRenderTargetView(m_NormalBuffer.Get(), nullptr, m_NormalRTV.ReleaseAndGetAddressOf());
		m_Device->CreateShaderResourceView(m_NormalBuffer.Get(), nullptr, m_NormalSRV.ReleaseAndGetAddressOf());
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

		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_MetalAOBuffer.ReleaseAndGetAddressOf());
		m_Device->CreateRenderTargetView(m_MetalAOBuffer.Get(), nullptr, m_MetalAORTV.ReleaseAndGetAddressOf());
		m_Device->CreateShaderResourceView(m_MetalAOBuffer.Get(), nullptr, m_MetalAOSRV.ReleaseAndGetAddressOf());
	}

	// Create back-buffer RTV
	m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_BackBuffer.ReleaseAndGetAddressOf());
	m_Device->CreateRenderTargetView(m_BackBuffer.Get(), nullptr, m_BackBufferRTV.ReleaseAndGetAddressOf());

	// Create deferred RTV, SRV
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
		m_Device->CreateTexture2D(&Texture2DDesc, nullptr, m_ScreenQuadTexture.ReleaseAndGetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC ScreenQuadSRVDesc{};
		ScreenQuadSRVDesc.Format = Texture2DDesc.Format;
		ScreenQuadSRVDesc.Texture2D.MipLevels = 1;
		ScreenQuadSRVDesc.Texture2D.MostDetailedMip = 0;
		ScreenQuadSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		m_Device->CreateShaderResourceView(m_ScreenQuadTexture.Get(), &ScreenQuadSRVDesc, m_ScreenQuadSRV.ReleaseAndGetAddressOf());

		D3D11_RENDER_TARGET_VIEW_DESC ScreenQuadRTVDesc{};
		ScreenQuadRTVDesc.Format = Texture2DDesc.Format;
		ScreenQuadRTVDesc.Texture2D.MipSlice = 0;
		ScreenQuadRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		m_Device->CreateRenderTargetView(m_ScreenQuadTexture.Get(), &ScreenQuadRTVDesc, m_ScreenQuadRTV.ReleaseAndGetAddressOf());
	}
}

void CGame::InitializeViewports()
{
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

void CGame::CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{};
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.StencilEnable = FALSE;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateLessEqualNoWrite.ReleaseAndGetAddressOf())));

	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateAlways.ReleaseAndGetAddressOf())));
}

void CGame::CreateBlendStates()
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

void CGame::CreateRasterizerStates()
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

void CGame::CreateInputDevices()
{
	m_Keyboard = make_unique<Keyboard>();

	m_Mouse = make_unique<Mouse>();
	m_Mouse->SetWindow(m_hWnd);
	m_Mouse->SetMode(Mouse::Mode::MODE_ABSOLUTE);
}

void CGame::CreateConstantBuffers()
{
	m_CBSpaceWVP = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBSpaceWVPData, sizeof(m_CBSpaceWVPData));
	m_CBSpaceVP = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBSpaceVPData, sizeof(m_CBSpaceVPData));
	m_CBSpace2D = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBSpace2DData, sizeof(m_CBSpace2DData));
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
	m_CBLight = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBLightData, sizeof(m_CBLightData));
	m_CBDirectionalLight = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBDirectionalLightData, sizeof(m_CBDirectionalLightData));
	m_CBMaterial = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBMaterialData, sizeof(m_CBMaterialData));
	m_CBPSFlags = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBPSFlagsData, sizeof(m_CBPSFlagsData));
	m_CBGizmoColorFactor = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBGizmoColorFactorData, sizeof(m_CBGizmoColorFactorData));
	m_CBPS2DFlags = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBPS2DFlagsData, sizeof(m_CBPS2DFlagsData));
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
	m_CBCamera = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBCameraData, sizeof(m_CBCameraData));
	m_CBScreen = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBScreenData, sizeof(m_CBScreenData));
	m_CBRadiancePrefiltering = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBRadiancePrefilteringData, sizeof(m_CBRadiancePrefilteringData));
	m_CBIrradianceGenerator = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBIrradianceGeneratorData, sizeof(m_CBIrradianceGeneratorData));
	m_CBBillboard = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBBillboardData, sizeof(m_CBBillboardData));
	m_CBGBufferUnpacking = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBGBufferUnpackingData, sizeof(m_CBGBufferUnpackingData));

	m_CBSpaceWVP->Create();
	m_CBSpaceVP->Create();
	m_CBSpace2D->Create();
	m_CBAnimationBones->Create();
	m_CBAnimation->Create();
	m_CBTerrain->Create();
	m_CBWind->Create();
	m_CBTessFactor->Create();
	m_CBDisplacement->Create();
	m_CBLight->Create();
	m_CBDirectionalLight->Create();
	m_CBMaterial->Create();
	m_CBPSFlags->Create();
	m_CBGizmoColorFactor->Create();
	m_CBPS2DFlags->Create();
	m_CBTerrainMaskingSpace->Create();
	m_CBTerrainSelection->Create();
	m_CBSkyTime->Create();
	m_CBWaterTime->Create();
	m_CBEditorTime->Create();
	m_CBCamera->Create();
	m_CBScreen->Create();
	m_CBRadiancePrefiltering->Create();
	m_CBIrradianceGenerator->Create();
	m_CBBillboard->Create();
	m_CBGBufferUnpacking->Create();
}

void CGame::CreateBaseShaders()
{
	bool bShouldCompile{ false };

	m_VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSBase.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSBase->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSInstance = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSInstance->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSInstance.hlsl", "main", 
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSInstance->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSAnimation->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSAnimation.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSAnimation->AttachConstantBuffer(m_CBSpaceWVP.get());
	m_VSAnimation->AttachConstantBuffer(m_CBAnimationBones.get());
	m_VSAnimation->AttachConstantBuffer(m_CBAnimation.get());

	m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSSky->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSSky.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSSky->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSLine->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSLine.hlsl", "main", 
		CObject3DLine::KInputElementDescs, ARRAYSIZE(CObject3DLine::KInputElementDescs));
	m_VSLine->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSGizmo->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSGizmo.hlsl", "main", 
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSGizmo->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSTerrain->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSTerrain.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSTerrain->AttachConstantBuffer(m_CBSpaceWVP.get());
	m_VSTerrain->AttachConstantBuffer(m_CBTerrain.get());

	m_VSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSFoliage->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSFoliage.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSFoliage->AttachConstantBuffer(m_CBSpaceWVP.get());
	m_VSFoliage->AttachConstantBuffer(m_CBTerrain.get());
	m_VSFoliage->AttachConstantBuffer(m_CBWind.get());

	m_VSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSParticle->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSParticle.hlsl", "main",
		CParticlePool::KInputElementDescs, ARRAYSIZE(CParticlePool::KInputElementDescs));

	m_VSScreenQuad = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	//m_VSScreenQuad->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSScreenQuad.hlsl", "main");
	m_VSScreenQuad->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSScreenQuad.hlsl", "main", 
		KScreenQuadInputElementDescs, ARRAYSIZE(KScreenQuadInputElementDescs));

	m_VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase2D->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSBase2D.hlsl", "main",
		CObject2D::KInputLayout, ARRAYSIZE(CObject2D::KInputLayout));
	m_VSBase2D->AttachConstantBuffer(m_CBSpace2D.get());

	m_VSBillboard = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBillboard->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSBillboard.hlsl", "main",
		CBillboard::KInputElementDescs, ARRAYSIZE(CBillboard::KInputElementDescs));

	m_VSLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSLight->Create(EShaderType::VertexShader, bShouldCompile, L"Shader\\VSLight.hlsl", "main",
		CLight::KInputElementDescs, ARRAYSIZE(CLight::KInputElementDescs));

	m_HSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSTerrain->Create(EShaderType::HullShader, bShouldCompile, L"Shader\\HSTerrain.hlsl", "main");
	m_HSTerrain->AttachConstantBuffer(m_CBLight.get());
	m_HSTerrain->AttachConstantBuffer(m_CBTessFactor.get());

	m_HSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSWater->Create(EShaderType::HullShader, bShouldCompile, L"Shader\\HSWater.hlsl", "main");
	m_HSWater->AttachConstantBuffer(m_CBLight.get());
	m_HSWater->AttachConstantBuffer(m_CBTessFactor.get());

	m_HSStatic = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSStatic->Create(EShaderType::HullShader, bShouldCompile, L"Shader\\HSStatic.hlsl", "main");
	m_HSStatic->AttachConstantBuffer(m_CBTessFactor.get());

	m_HSBillboard = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSBillboard->Create(EShaderType::HullShader, bShouldCompile, L"Shader\\HSBillboard.hlsl", "main");

	m_HSPointLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSPointLight->Create(EShaderType::HullShader, bShouldCompile, L"Shader\\HSPointLight.hlsl", "main");

	m_DSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSTerrain->Create(EShaderType::DomainShader, bShouldCompile, L"Shader\\DSTerrain.hlsl", "main");
	m_DSTerrain->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSTerrain->AttachConstantBuffer(m_CBDisplacement.get());

	m_DSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSWater->Create(EShaderType::DomainShader, bShouldCompile, L"Shader\\DSWater.hlsl", "main");
	m_DSWater->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSWater->AttachConstantBuffer(m_CBWaterTime.get());

	m_DSStatic = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSStatic->Create(EShaderType::DomainShader, bShouldCompile, L"Shader\\DSStatic.hlsl", "main");
	m_DSStatic->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSStatic->AttachConstantBuffer(m_CBDisplacement.get());

	m_DSBillboard = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSBillboard->Create(EShaderType::DomainShader, bShouldCompile, L"Shader\\DSBillboard.hlsl", "main");
	m_DSBillboard->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSBillboard->AttachConstantBuffer(m_CBBillboard.get());

	m_DSPointLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSPointLight->Create(EShaderType::DomainShader, bShouldCompile, L"Shader\\DSPointLight.hlsl", "main");
	m_DSPointLight->AttachConstantBuffer(m_CBSpaceVP.get());

	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, bShouldCompile, L"Shader\\GSNormal.hlsl", "main");
	m_GSNormal->AttachConstantBuffer(m_CBSpaceVP.get());

	m_GSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSParticle->Create(EShaderType::GeometryShader, bShouldCompile, L"Shader\\GSParticle.hlsl", "main");
	m_GSParticle->AttachConstantBuffer(m_CBSpaceVP.get());

	m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSBase.hlsl", "main");
	m_PSBase->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSBase->AttachConstantBuffer(m_CBLight.get());
	m_PSBase->AttachConstantBuffer(m_CBMaterial.get());

	m_PSBase_gbuffer = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase_gbuffer->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSBase.hlsl", "gbuffer");
	m_PSBase_gbuffer->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSBase_gbuffer->AttachConstantBuffer(m_CBLight.get());
	m_PSBase_gbuffer->AttachConstantBuffer(m_CBMaterial.get());

	m_PSVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSVertexColor->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSVertexColor.hlsl", "main");

	m_PSDynamicSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSDynamicSky->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSDynamicSky.hlsl", "main");
	m_PSDynamicSky->AttachConstantBuffer(m_CBSkyTime.get());

	m_PSCloud = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCloud->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSCloud.hlsl", "main");
	m_PSCloud->AttachConstantBuffer(m_CBSkyTime.get());

	m_PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSLine->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSLine.hlsl", "main");

	m_PSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSGizmo->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSGizmo.hlsl", "main");
	m_PSGizmo->AttachConstantBuffer(m_CBGizmoColorFactor.get());

	m_PSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSTerrain->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSTerrain.hlsl", "main");
	m_PSTerrain->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSTerrain->AttachConstantBuffer(m_CBTerrainMaskingSpace.get());
	m_PSTerrain->AttachConstantBuffer(m_CBLight.get());
	m_PSTerrain->AttachConstantBuffer(m_CBTerrainSelection.get());
	m_PSTerrain->AttachConstantBuffer(m_CBEditorTime.get());
	m_PSTerrain->AttachConstantBuffer(m_CBMaterial.get());

	m_PSTerrain_gbuffer = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSTerrain_gbuffer->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSTerrain.hlsl", "gbuffer");
	m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBTerrainMaskingSpace.get());
	m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBLight.get());
	m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBTerrainSelection.get());
	m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBEditorTime.get());
	m_PSTerrain_gbuffer->AttachConstantBuffer(m_CBMaterial.get());

	m_PSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSWater->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSWater.hlsl", "main");
	m_PSWater->AttachConstantBuffer(m_CBWaterTime.get());
	m_PSWater->AttachConstantBuffer(m_CBLight.get());

	m_PSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSFoliage->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSFoliage.hlsl", "main");
	m_PSFoliage->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSFoliage->AttachConstantBuffer(m_CBLight.get());
	m_PSFoliage->AttachConstantBuffer(m_CBMaterial.get());

	m_PSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSParticle->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSParticle.hlsl", "main");

	m_PSCamera = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCamera->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSCamera.hlsl", "main");
	m_PSCamera->AttachConstantBuffer(m_CBMaterial.get());
	m_PSCamera->AttachConstantBuffer(m_CBEditorTime.get());
	m_PSCamera->AttachConstantBuffer(m_CBCamera.get());

	m_PSScreenQuad = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSScreenQuad->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSScreenQuad.hlsl", "main");

	m_PSScreenQuad_opaque = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSScreenQuad_opaque->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSScreenQuad.hlsl", "opaque");

	m_CBScreenData.InverseScreenSize = XMFLOAT2(1.0f / m_WindowSize.x, 1.0f / m_WindowSize.y);
	m_CBScreen->Update();

	m_PSEdgeDetector = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSEdgeDetector->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSEdgeDetector.hlsl", "main");
	m_PSEdgeDetector->AttachConstantBuffer(m_CBScreen.get());

	m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSSky->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSSky.hlsl", "main");

	m_PSIrradianceGenerator = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSIrradianceGenerator->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSIrradianceGenerator.hlsl", "main");
	m_PSIrradianceGenerator->AttachConstantBuffer(m_CBIrradianceGenerator.get());

	m_PSFromHDR = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSFromHDR->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSFromHDR.hlsl", "main");

	m_PSRadiancePrefiltering = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSRadiancePrefiltering->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSRadiancePrefiltering.hlsl", "main");
	m_PSRadiancePrefiltering->AttachConstantBuffer(m_CBRadiancePrefiltering.get());

	m_PSBRDFIntegrator = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBRDFIntegrator->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSBRDFIntegrator.hlsl", "main");

	m_PSBillboard = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBillboard->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSBillboard.hlsl", "main");
	m_PSBillboard->AttachConstantBuffer(m_CBEditorTime.get());

	m_PSDirectionalLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSDirectionalLight->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSDirectionalLight.hlsl", "main");
	m_PSDirectionalLight->AttachConstantBuffer(m_CBGBufferUnpacking.get());
	m_PSDirectionalLight->AttachConstantBuffer(m_CBDirectionalLight.get());
	
	m_PSPointLight = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSPointLight->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSPointLight.hlsl", "main");
	m_PSPointLight->AttachConstantBuffer(m_CBGBufferUnpacking.get());

	m_PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase2D->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSBase2D.hlsl", "main");
	m_PSBase2D->AttachConstantBuffer(m_CBPS2DFlags.get());

	m_PSMasking2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSMasking2D->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSMasking2D.hlsl", "main");

	m_PSHeightMap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSHeightMap2D->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSHeightMap2D.hlsl", "main");
	
	m_PSCubemap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCubemap2D->Create(EShaderType::PixelShader, bShouldCompile, L"Shader\\PSCubemap2D.hlsl", "main");
}

void CGame::CreateMiniAxes()
{
	m_vMiniAxes.clear();
	m_vMiniAxes.emplace_back(make_unique<CObject3D>("AxisX", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vMiniAxes.emplace_back(make_unique<CObject3D>("AxisY", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vMiniAxes.emplace_back(make_unique<CObject3D>("AxisZ", m_Device.Get(), m_DeviceContext.Get(), this));

	const SMesh KAxisCone{ GenerateCone(0, 1.0f, 1.0f, 16) };
	vector<CMaterialData> vMaterialData{};
	vMaterialData.resize(3);
	vMaterialData[0].SetUniformColor(XMFLOAT3(1, 0, 0));
	vMaterialData[1].SetUniformColor(XMFLOAT3(0, 1, 0));
	vMaterialData[2].SetUniformColor(XMFLOAT3(0, 0, 1));
	m_vMiniAxes[0]->Create(KAxisCone, vMaterialData[0]);
	m_vMiniAxes[0]->ComponentTransform.Roll = -XM_PIDIV2;
	
	m_vMiniAxes[1]->Create(KAxisCone, vMaterialData[1]);
	
	m_vMiniAxes[2]->Create(KAxisCone, vMaterialData[2]);
	m_vMiniAxes[2]->ComponentTransform.Yaw = -XM_PIDIV2;
	m_vMiniAxes[2]->ComponentTransform.Roll = -XM_PIDIV2;
	
	m_vMiniAxes[0]->ComponentTransform.Scaling =
		m_vMiniAxes[1]->ComponentTransform.Scaling =
		m_vMiniAxes[2]->ComponentTransform.Scaling = XMVectorSet(0.1f, 0.8f, 0.1f, 0);
}

void CGame::CreatePickingRay()
{
	m_PickingRayRep = make_unique<CObject3DLine>("PickingRay", m_Device.Get(), m_DeviceContext.Get());

	vector<SVertex3DLine> Vertices{};
	Vertices.emplace_back(XMVectorSet(0, 0, 0, 1), XMVectorSet(1, 0, 0, 1));
	Vertices.emplace_back(XMVectorSet(10.0f, 10.0f, 0, 1), XMVectorSet(0, 1, 0, 1));

	m_PickingRayRep->Create(Vertices);
}

void CGame::CreatePickedTriangle()
{
	m_PickedTriangleRep = make_unique<CObject3D>("PickedTriangle", m_Device.Get(), m_DeviceContext.Get(), this);

	m_PickedTriangleRep->Create(GenerateTriangle(XMVectorSet(0, 0, 1.5f, 1), XMVectorSet(+1.0f, 0, 0, 1), XMVectorSet(-1.0f, 0, 0, 1),
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f)));
}

void CGame::CreateBoundingSphere()
{
	m_BoundingSphereRep = make_unique<CObject3D>("BoundingSphere", m_Device.Get(), m_DeviceContext.Get(), this);

	m_BoundingSphereRep->Create(GenerateSphere(16));
}

void CGame::Create3DGizmos()
{
	static constexpr XMVECTOR KColorX{ 1.00f, 0.25f, 0.25f, 1.0f };
	static constexpr XMVECTOR KColorY{ 0.25f, 1.00f, 0.25f, 1.0f };
	static constexpr XMVECTOR KColorZ{ 0.25f, 0.25f, 1.00f, 1.0f };

	if (!m_Object3D_3DGizmoRotationPitch)
	{
		m_Object3D_3DGizmoRotationPitch = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, KColorX) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorX) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoRotationPitch->Create(MergeStaticMeshes(MeshRing, MeshAxis));
		m_Object3D_3DGizmoRotationPitch->ComponentTransform.Roll = -XM_PIDIV2;
	}
	
	if (!m_Object3D_3DGizmoRotationYaw)
	{
		m_Object3D_3DGizmoRotationYaw = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, KColorY) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorY) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoRotationYaw->Create(MergeStaticMeshes(MeshRing, MeshAxis));
	}

	if (!m_Object3D_3DGizmoRotationRoll)
	{
		m_Object3D_3DGizmoRotationRoll = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, KColorZ) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorZ) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoRotationRoll->Create(MergeStaticMeshes(MeshRing, MeshAxis));
		m_Object3D_3DGizmoRotationRoll->ComponentTransform.Pitch = XM_PIDIV2;
	}
	
	if (!m_Object3D_3DGizmoTranslationX)
	{
		m_Object3D_3DGizmoTranslationX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorX) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, KColorX) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationX->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationX->ComponentTransform.Roll = -XM_PIDIV2;
	}
	
	if (!m_Object3D_3DGizmoTranslationY)
	{
		m_Object3D_3DGizmoTranslationY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorY) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, KColorY) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationY->Create(MeshAxis);
	}
	
	if (!m_Object3D_3DGizmoTranslationZ)
	{
		m_Object3D_3DGizmoTranslationZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorZ) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, KColorZ) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationZ->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationZ->ComponentTransform.Pitch = XM_PIDIV2;
	}

	if (!m_Object3D_3DGizmoScalingX)
	{
		m_Object3D_3DGizmoScalingX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorX) };
		SMesh MeshCube{ GenerateCube(KColorX) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingX->Create(MeshAxis);
		m_Object3D_3DGizmoScalingX->ComponentTransform.Roll = -XM_PIDIV2;
	}

	if (!m_Object3D_3DGizmoScalingY)
	{
		m_Object3D_3DGizmoScalingY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorY) };
		SMesh MeshCube{ GenerateCube(KColorY) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingY->Create(MeshAxis);
	}

	if (!m_Object3D_3DGizmoScalingZ)
	{
		m_Object3D_3DGizmoScalingZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorZ) };
		SMesh MeshCube{ GenerateCube(KColorZ) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingZ->Create(MeshAxis);
		m_Object3D_3DGizmoScalingZ->ComponentTransform.Pitch = XM_PIDIV2;
	}
}

void CGame::CreateScreenQuadVertexBuffer()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SScreenQuadVertex) * m_vScreenQuadVertices.size());
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vScreenQuadVertices[0];
	m_Device->CreateBuffer(&BufferDesc, &SubresourceData, &m_ScreenQuadVertexBuffer);
}

void CGame::CreateCubemapVertexBuffer()
{
	m_vCubemapVertices.clear();

	// x+
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(+1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, -1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, +1));

	// x-
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(-1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1));

	// y+
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, +1, +1));

	// y-
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, -1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, -1));

	// z+
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(-1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(+1, +1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(+1, -1, +1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(-1, -1, +1));

	// z-
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, +1, 0, 1), XMFLOAT3(+1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, +1, 0, 1), XMFLOAT3(-1, +1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(+1, -1, 0, 1), XMFLOAT3(-1, -1, -1));
	m_vCubemapVertices.emplace_back(XMFLOAT4(-1, -1, 0, 1), XMFLOAT3(+1, -1, -1));

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SCubemapVertex) * m_vCubemapVertices.size());
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vCubemapVertices[0];
	m_Device->CreateBuffer(&BufferDesc, &SubresourceData, &m_CubemapVertexBuffer);
}

void CGame::LoadScene(const string& FileName, const std::string& SceneDirectory)
{
	CBinaryData SceneBinaryData{};
	string ReadString{};

	SceneBinaryData.LoadFromFile(FileName);

	// Terrain
	{
		SceneBinaryData.ReadStringWithPrefixedLength(ReadString);
		LoadTerrain(ReadString);
	}
	
	// Object3D
	{
		ClearObject3Ds();

		uint32_t Object3DCount{ SceneBinaryData.ReadUint32() };
		CBinaryData Object3DBinary{};
		string Object3DName{};
		for (uint32_t iObject3D = 0; iObject3D < Object3DCount; ++iObject3D)
		{
			SceneBinaryData.ReadStringWithPrefixedLength(ReadString);

			Object3DBinary.LoadFromFile(ReadString);
			Object3DBinary.ReadSkip(8);
			Object3DBinary.ReadStringWithPrefixedLength(Object3DName);

			InsertObject3D(Object3DName);
			CObject3D* const Object3D{ GetObject3D(Object3DName) };
			Object3D->LoadOB3D(ReadString);
		}
	}

	// Camera
	{
		ClearCameras();

		uint32_t CameraCount{ SceneBinaryData.ReadUint32() };
		for (uint32_t iCamera = 0; iCamera < CameraCount; ++iCamera)
		{
			/*
			SceneBinaryData.ReadStringWithPrefixedLength(ReadString);

			InsertCamera(ReadString);
			CCamera* Camera{ GetCamera(ReadString) };
			Camera->SetData;
			*/
		}
	}

	// Light
	{
		ClearLights();

		uint32_t LightCount{ SceneBinaryData.ReadUint32() };
		for (uint32_t iLight = 0; iLight < LightCount; ++iLight)
		{
			CLight::SLightInstanceCPUData InstanceCPUData{};
			CLight::SLightInstanceGPUData InstanceGPUData{};

			SceneBinaryData.ReadStringWithPrefixedLength(InstanceCPUData.Name);
			InstanceCPUData.eType = (CLight::EType)SceneBinaryData.ReadUint32();

			SceneBinaryData.ReadXMVECTOR(InstanceGPUData.Position);
			SceneBinaryData.ReadXMVECTOR(InstanceGPUData.Color);
			SceneBinaryData.ReadFloat(InstanceGPUData.Range);

			InsertLight(InstanceCPUData.eType, InstanceCPUData.Name);

			m_Light->SetInstanceGPUData(InstanceCPUData.Name, InstanceGPUData);
			m_LightRep->SetInstancePosition(InstanceCPUData.Name, InstanceGPUData.Position);
		}
	}
}

void CGame::SaveScene(const string& FileName, const std::string& SceneDirectory)
{
	CBinaryData SceneBinaryData{};

	// Terrain
	if (m_Terrain)
	{
		if (m_Terrain->GetFileName().empty())
		{
			m_Terrain->Save(SceneDirectory + "terrain.terr");
		}

		SceneBinaryData.WriteStringWithPrefixedLength(m_Terrain->GetFileName());
	}
	else
	{
		SceneBinaryData.WriteUint32(0);
	}

	// Object3D
	uint32_t Object3DCount{ (uint32_t)m_vObject3Ds.size() };
	SceneBinaryData.WriteUint32(Object3DCount);
	if (Object3DCount)
	{
		for (const auto& Object3D : m_vObject3Ds)
		{
			if (Object3D->GetOB3DFileName().empty())
			{
				Object3D->SaveOB3D(SceneDirectory + Object3D->GetName() + ".ob3d");
			}

			SceneBinaryData.WriteStringWithPrefixedLength(Object3D->GetOB3DFileName());
		}
	}

	// Camera
	uint32_t CameraCount{ (uint32_t)m_vCameras.size() };
	SceneBinaryData.WriteUint32(CameraCount);
	if (CameraCount)
	{
		
	}

	// Light
	uint32_t LightCount{ (uint32_t)m_Light->GetInstanceCount() };
	SceneBinaryData.WriteUint32(LightCount);
	if (LightCount)
	{
		for (const auto& LightPair : m_Light->GetInstanceNameToIndexMap())
		{
			const auto& InstanceCPUData{ m_Light->GetInstanceCPUData(LightPair.first) };
			const auto& InstanceGPUData{ m_Light->GetInstanceGPUData(LightPair.first) };

			SceneBinaryData.WriteStringWithPrefixedLength(InstanceCPUData.Name);
			SceneBinaryData.WriteUint32((uint32_t)InstanceCPUData.eType);

			SceneBinaryData.WriteXMVECTOR(InstanceGPUData.Position);
			SceneBinaryData.WriteXMVECTOR(InstanceGPUData.Color);
			SceneBinaryData.WriteFloat(InstanceGPUData.Range);
		}	
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

void CGame::ToggleGameRenderingFlags(EFlagsRendering Flags)
{
	m_eFlagsRendering ^= Flags;
}

void CGame::Set3DGizmoMode(E3DGizmoMode Mode)
{
	m_e3DGizmoMode = Mode;
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

void CGame::SetUniversalbUseLighiting()
{
	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UseLighting))
	{
		m_CBPSFlagsData.bUseLighting = TRUE;
	}
	m_CBPSFlags->Update();
}

void CGame::UpdateCBSpace(const XMMATRIX& World)
{
	const XMMATRIX WorldT{ XMMatrixTranspose(World) };
	const XMMATRIX ViewT{ XMMatrixTranspose(m_MatrixView) };
	const XMMATRIX ProjectionT{ XMMatrixTranspose(m_MatrixProjection) };
	const XMMATRIX ViewProjectionT{ XMMatrixTranspose(m_MatrixView * m_MatrixProjection) };
	const XMMATRIX WVPT{ XMMatrixTranspose(World * m_MatrixView * m_MatrixProjection) };

	m_CBSpaceWVPData.World = WorldT;
	m_CBSpaceWVPData.ViewProjection = ViewProjectionT;
	m_CBSpaceWVPData.WVP = WVPT;

	m_CBSpaceVPData.View = ViewT;
	m_CBSpaceVPData.Projection = ProjectionT;
	m_CBSpaceVPData.ViewProjection = ViewProjectionT;

	m_CBSpace2DData.World = WorldT;

	m_CBSpaceWVP->Update();
	m_CBSpaceVP->Update();

	m_CBSpace2DData.Projection = XMMatrixTranspose(m_MatrixProjection2D);
	m_CBSpace2D->Update();
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

void CGame::UpdateCBMaterialData(const CMaterialData& MaterialData, uint32_t TotalMaterialCount)
{
	m_CBMaterialData.AmbientColor = MaterialData.AmbientColor();
	m_CBMaterialData.DiffuseColor = MaterialData.DiffuseColor();
	m_CBMaterialData.SpecularColor = MaterialData.SpecularColor();
	m_CBMaterialData.SpecularExponent = MaterialData.SpecularExponent();
	m_CBMaterialData.SpecularIntensity = MaterialData.SpecularIntensity();
	m_CBMaterialData.Roughness = MaterialData.Roughness();
	m_CBMaterialData.Metalness = MaterialData.Metalness();

	uint32_t FlagsHasTexture{};
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::DiffuseTexture) ? 0x01 : 0;
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::NormalTexture) ? 0x02 : 0;
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::OpacityTexture) ? 0x04 : 0;
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::SpecularIntensityTexture) ? 0x08 : 0;
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::RoughnessTexture) ? 0x10 : 0;
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::MetalnessTexture) ? 0x20 : 0;
	FlagsHasTexture += MaterialData.HasTexture(STextureData::EType::AmbientOcclusionTexture) ? 0x40 : 0;
	// Displacement texture is usually not used in PS
	m_CBMaterialData.FlagsHasTexture = FlagsHasTexture;

	uint32_t FlagsIsTextureSRGB{};
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::DiffuseTexture) ? 0x01 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::NormalTexture) ? 0x02 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::OpacityTexture) ? 0x04 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::SpecularIntensityTexture) ? 0x08 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::RoughnessTexture) ? 0x10 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::MetalnessTexture) ? 0x20 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(STextureData::EType::AmbientOcclusionTexture) ? 0x40 : 0;
	// Displacement texture is usually not used in PS
	if (m_EnvironmentTexture) FlagsIsTextureSRGB += m_EnvironmentTexture->IssRGB() ? 0x4000 : 0;
	if (m_IrradianceTexture) FlagsIsTextureSRGB += m_IrradianceTexture->IssRGB() ? 0x8000 : 0;

	m_CBMaterialData.FlagsIsTextureSRGB = FlagsIsTextureSRGB;

	m_CBMaterialData.TotalMaterialCount = TotalMaterialCount;

	m_CBMaterial->Update();
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

void CGame::UpdateCBBillboard(const CBillboard::SCBBillboardData& Data)
{
	m_CBBillboardData = Data;
	m_CBBillboardData.ScreenSize = m_WindowSize; // @important
	m_CBBillboard->Update();
}

void CGame::UpdateCBDirectionalLight()
{
	m_CBDirectionalLightData.LightDirection = m_CBLightData.DirectionalLightDirection;
	m_CBDirectionalLightData.LightColor = m_CBLightData.DirectionalLightColor;
	m_CBDirectionalLightData.Exposure = m_CBLightData.Exposure;
	m_CBDirectionalLightData.EnvironmentTextureMipLevels = m_CBPSFlagsData.EnvironmentTextureMipLevels;
	m_CBDirectionalLightData.PrefilteredRadianceTextureMipLevels = m_CBPSFlagsData.PrefilteredRadianceTextureMipLevels;

	m_CBDirectionalLight->Update();
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

	m_SkyMaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, m_SkyData.TextureFileName);

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom), m_SkyMaterialData);
	m_Object3DSkySphere->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
	m_Object3DSkySphere->ComponentPhysics.bIsPickable = false;

	m_Object3DSun = make_unique<CObject3D>("Sun", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSun->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterialData);
	m_Object3DSun->UpdateQuadUV(m_SkyData.Sun.UVOffset, m_SkyData.Sun.UVSize);
	m_Object3DSun->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Sun.WidthHeightRatio, 0);
	m_Object3DSun->ComponentRender.bIsTransparent = true;
	m_Object3DSun->ComponentPhysics.bIsPickable = false;
	
	m_Object3DMoon = make_unique<CObject3D>("Moon", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DMoon->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterialData);
	m_Object3DMoon->UpdateQuadUV(m_SkyData.Moon.UVOffset, m_SkyData.Moon.UVSize);
	m_Object3DMoon->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Moon.WidthHeightRatio, 0);
	m_Object3DMoon->ComponentRender.bIsTransparent = true;
	m_Object3DMoon->ComponentPhysics.bIsPickable = false;
	
	m_SkyData.bIsDataSet = true;
	m_SkyData.bIsDynamic = true;

	return;
}

void CGame::CreateStaticSky(float ScalingFactor)
{
	m_SkyScalingFactor = ScalingFactor;

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get(), this);
	//m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom));
	m_Object3DSkySphere->Create(GenerateCubemapSphere(KSkySphereSegmentCount));
	m_Object3DSkySphere->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
	m_Object3DSkySphere->ComponentPhysics.bIsPickable = false;

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
	m_CBLightData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	m_CBLightData.DirectionalLightColor = Color;

	m_CBLight->Update();
}

void CGame::SetDirectionalLightDirection(const XMVECTOR& LightSourcePosition)
{
	m_CBLightData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);

	m_CBLight->Update();
}

void CGame::SetDirectionalLightColor(const XMFLOAT3& Color)
{
	m_CBLightData.DirectionalLightColor = Color;

	m_CBLight->Update();
}

const XMVECTOR& CGame::GetDirectionalLightDirection() const
{
	return m_CBLightData.DirectionalLightDirection;
}

const XMFLOAT3& CGame::GetDirectionalLightColor() const
{
	return m_CBLightData.DirectionalLightColor;
}

void CGame::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	m_CBLightData.AmbientLightColor = Color;
	m_CBLightData.AmbientLightIntensity = Intensity;

	m_CBLight->Update();
}

const XMFLOAT3& CGame::GetAmbientLightColor() const
{
	return m_CBLightData.AmbientLightColor;
}

float CGame::GetAmbientLightIntensity() const
{
	return m_CBLightData.AmbientLightIntensity;
}

void CGame::SetExposure(float Value)
{
	m_CBLightData.Exposure = Value;

	m_CBLight->Update();
}

float CGame::GetExposure()
{
	return m_CBLightData.Exposure;
}

void CGame::CreateTerrain(const XMFLOAT2& TerrainSize, uint32_t MaskingDetail, float UniformScaling)
{
	if (!GetMaterial("brown_mud_dry")) InitializeEditorAssets();

	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Create(TerrainSize, *GetMaterial("brown_mud_dry"), MaskingDetail, UniformScaling);
	UpdateCBTerrainData(m_Terrain->GetTerrainData());
	UpdateCBTerrainMaskingSpace(m_Terrain->GetMaskingSpaceData());
	
	ID3D11ShaderResourceView* NullSRVs[20]{};
	m_DeviceContext->DSSetShaderResources(0, 1, NullSRVs);
	m_DeviceContext->PSSetShaderResources(0, 20, NullSRVs);
}

void CGame::LoadTerrain(const string& TerrainFileName)
{
	if (TerrainFileName.empty()) return;

	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Load(TerrainFileName);
	UpdateCBTerrainData(m_Terrain->GetTerrainData());
	UpdateCBTerrainMaskingSpace(m_Terrain->GetMaskingSpaceData());
	
	int MaterialCount{ m_Terrain->GetMaterialCount() };
	for (int iMaterial = 0; iMaterial < MaterialCount; ++iMaterial)
	{
		const CMaterialData& TerrainMaterialData{ m_Terrain->GetMaterial(iMaterial) };
		InsertMaterialCreateTextures(TerrainMaterialData, false);
	}
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
					m_vCopyObject3DInstances.emplace_back(Object3D->GetInstanceCPUData(SelectionData.Name), Object3D);
				}
				else
				{
					m_vCopyObject3Ds.emplace_back(SelectionData.Name, Object3D->GetModel(),
						Object3D->ComponentTransform, Object3D->ComponentPhysics, Object3D->ComponentRender,
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
				m_vCopyLights.emplace_back(m_Light->GetInstanceCPUData(SelectionData.Name), m_Light->GetInstanceGPUData(SelectionData.Name));
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
			Object3D->ComponentTransform = CopyObject3D.ComponentTransform;
			Object3D->ComponentPhysics = CopyObject3D.ComponentPhysics;
			Object3D->ComponentRender = CopyObject3D.ComponentRender;

			Object3D->UpdateWorldMatrix();
			Object3D->UpdateAllInstancesWorldMatrix();

			SelectAdditional(SSelectionData(EObjectType::Object3D, NewName, Object3D));
			++SelectionCount;

			++CopyObject3D.PasteCounter;
		}
	}

	for (auto& CopyObject3DInstance : m_vCopyObject3DInstances)
	{
		//string NewName{ CopyObject3DInstance.InstanceCPUData.Name + "_" + to_string(CopyObject3DInstance.PasteCounter) };
		CObject3D* const Object3D{ CopyObject3DInstance.PtrObject3D };

		//if (Object3D->InsertInstance(NewName))
		if (Object3D->InsertInstance())
		{
			auto& InstanceCPUData{ Object3D->GetLastInstanceCPUData() };
			string SavedName{ InstanceCPUData.Name };
			InstanceCPUData = CopyObject3DInstance.InstanceCPUData;
			InstanceCPUData.Name = SavedName; // @important

			Object3D->UpdateInstanceWorldMatrix(SavedName);

			SelectAdditional(SSelectionData(EObjectType::Object3DInstance, SavedName, Object3D));
			++SelectionCount;

			++CopyObject3DInstance.PasteCounter;
		}
	}

	for (auto& CopyLight : m_vCopyLights)
	{
		string NewName{ CopyLight.InstanceCPUData.Name + "_" + to_string(CopyLight.PasteCounter) };

		if (InsertLight(CopyLight.InstanceCPUData.eType, NewName))
		{
			m_Light->SetInstanceGPUData(NewName, CopyLight.InstanceGPUData);
			m_LightRep->SetInstancePosition(NewName, CopyLight.InstanceGPUData.Position);

			SelectAdditional(SSelectionData(EObjectType::Light, NewName));
			++SelectionCount;

			++CopyLight.PasteCounter;
		}
	}

	// @important
	m_MultipleSelectionWorldCenter /= (float)SelectionCount;
	m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_MultipleSelectionWorldCenter;
}

void CGame::DeleteSelectedObject()
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
				DeleteLight(SelectionData.Name);
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

bool CGame::InsertCamera(const string& Name)
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
	m_vCameras.back()->SetEyePosition(XMVectorSet(0, 1.0f, 0, 1));

	m_mapCameraNameToIndex[Name] = m_vCameras.size() - 1;

	m_CameraRep->InsertInstance(Name);
	m_CameraRep->UpdateInstanceWorldMatrix(Name, m_vCameras.back()->GetWorldMatrix());

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

	Deselect(EObjectType::Camera);
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
	case EBaseShader::VSInstance:
		Result = m_VSInstance.get();
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
	case EBaseShader::VSGizmo:
		Result = m_VSGizmo.get();
		break;
	case EBaseShader::VSTerrain:
		Result = m_VSTerrain.get();
		break;
	case EBaseShader::VSFoliage:
		Result = m_VSFoliage.get();
		break;
	case EBaseShader::VSParticle:
		Result = m_VSParticle.get();
		break;
	case EBaseShader::VSScreenQuad:
		Result = m_VSScreenQuad.get();
		break;
	case EBaseShader::VSBase2D:
		Result = m_VSBase2D.get();
		break;
	case EBaseShader::VSBillboard:
		Result = m_VSBillboard.get();
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
	case EBaseShader::HSBillboard:
		Result = m_HSBillboard.get();
		break;
	case EBaseShader::HSPointLight:
		Result = m_HSPointLight.get();
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
	case EBaseShader::DSBillboard:
		Result = m_DSBillboard.get();
		break;
	case EBaseShader::DSPointLight:
		Result = m_DSPointLight.get();
		break;
	case EBaseShader::GSNormal:
		Result = m_GSNormal.get();
		break;
	case EBaseShader::GSParticle:
		Result = m_GSParticle.get();
		break;
	case EBaseShader::PSBase:
		Result = m_PSBase.get();
		break;
	case EBaseShader::PSVertexColor:
		Result = m_PSVertexColor.get();
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
	case EBaseShader::PSGizmo:
		Result = m_PSGizmo.get();
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
	case EBaseShader::PSParticle:
		Result = m_PSParticle.get();
		break;
	case EBaseShader::PSCamera:
		Result = m_PSCamera.get();
		break;
	case EBaseShader::PSScreenQuad:
		Result = m_PSScreenQuad.get();
		break;
	case EBaseShader::PSScreenQuad_Opaque:
		Result = m_PSScreenQuad_opaque.get();
		break;
	case EBaseShader::PSEdgeDetector:
		Result = m_PSEdgeDetector.get();
		break;
	case EBaseShader::PSSky:
		Result = m_PSSky.get();
		break;
	case EBaseShader::PSIrradianceGenerator:
		Result = m_PSIrradianceGenerator.get();
		break;
	case EBaseShader::PSFromHDR:
		Result = m_PSFromHDR.get();
		break;
	case EBaseShader::PSRadiancePrefiltering:
		Result = m_PSRadiancePrefiltering.get();
		break;
	case EBaseShader::PSBRDFIntegrator:
		Result = m_PSBRDFIntegrator.get();
		break;
	case EBaseShader::PSBillboard:
		Result = m_PSBillboard.get();
		break;
	case EBaseShader::PSBase_GBuffer:
		Result = m_PSBase_gbuffer.get();
		break;
	case EBaseShader::PSDirectionalLight:
		Result = m_PSDirectionalLight.get();
		break;
	case EBaseShader::PSPointLight:
		Result = m_PSPointLight.get();
		break;
	case EBaseShader::PSBase2D:
		Result = m_PSBase2D.get();
		break;
	case EBaseShader::PSMasking2D:
		Result = m_PSMasking2D.get();
		break;
	case EBaseShader::PSHeightMap2D:
		Result = m_PSHeightMap2D.get();
		break;
	case EBaseShader::PSCubemap2D:
		Result = m_PSCubemap2D.get();
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
		m_vObject3Ds.emplace_back(make_unique<CObject3D>(Name, m_Device.Get(), m_DeviceContext.Get(), this));
		m_mapObject3DNameToIndex[Name] = m_vObject3Ds.size() - 1;

		return true;
	}
	return false;
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
	Deselect(EObjectType::Object3D);
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
	Deselect(EObjectType::Object3DLine);
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
	Deselect(EObjectType::Object2D);
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

bool CGame::InsertMaterial(const string& Name, bool bShowWarning)
{
	if (m_mapMaterialNameToIndex.find(Name) != m_mapMaterialNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Material 생성 실패");
		return false;
	}

	if (Name.size() >= KAssetNameMaxLength)
	{
		if (bShowWarning) MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Material 생성 실패");
		return false;
	}
	else if (Name.size() == 0)
	{
		if (bShowWarning) MB_WARN("이름은 공백일 수 없습니다.", "Material 생성 실패");
		return false;
	}

	m_vMaterialData.emplace_back();
	m_vMaterialData.back().Name(Name);
	m_vMaterialTextureSets.emplace_back();
	
	m_mapMaterialNameToIndex[Name] = m_vMaterialData.size() - 1;
	
	return true;
}

bool CGame::InsertMaterialCreateTextures(const CMaterialData& MaterialData, bool bShowWarning)
{
	if (InsertMaterial(MaterialData.Name(), bShowWarning))
	{
		CMaterialData* const Material{ GetMaterial(MaterialData.Name()) };
		*Material = MaterialData; // copy it!

		CreateMaterialTextures(*Material);
		return true;
	}
	return false;
}

void CGame::DeleteMaterial(const std::string& Name)
{
	if (!m_vMaterialData.size()) return;
	if (Name.length() == 0) return;
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end())
	{
		MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Material 삭제 실패");
		return;
	}

	size_t iMaterial{ m_mapMaterialNameToIndex[Name] };
	if (iMaterial < m_vMaterialData.size() - 1)
	{
		const string& SwappedName{ m_vMaterialData.back().Name() };

		swap(m_vMaterialData[iMaterial], m_vMaterialData.back());
		swap(m_vMaterialTextureSets[iMaterial], m_vMaterialTextureSets.back());

		m_mapMaterialNameToIndex[SwappedName] = iMaterial;
	}

	m_mapMaterialNameToIndex.erase(Name);
	m_vMaterialData.pop_back();
	m_vMaterialTextureSets.pop_back();
}

void CGame::CreateMaterialTextures(CMaterialData& MaterialData)
{
	size_t iMaterial{ m_mapMaterialNameToIndex[MaterialData.Name()] };
	m_vMaterialTextureSets[iMaterial] = make_unique<CMaterialTextureSet>(m_Device.Get(), m_DeviceContext.Get());
	m_vMaterialTextureSets[iMaterial]->CreateTextures(MaterialData);
}

CMaterialData* CGame::GetMaterial(const string& Name, bool bShowWarning)
{
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Material 얻어오기 실패");
		return nullptr;
	}
	return &m_vMaterialData[m_mapMaterialNameToIndex.at(Name)];
}

CMaterialTextureSet* CGame::GetMaterialTextureSet(const std::string& Name, bool bShowWarning)
{
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Material 얻어오기 실패");
		return nullptr;
	}
	return m_vMaterialTextureSets[m_mapMaterialNameToIndex.at(Name)].get();
}

void CGame::ClearMaterials()
{
	m_vMaterialData.clear();
	m_vMaterialTextureSets.clear();
	m_mapMaterialNameToIndex.clear();
}

size_t CGame::GetMaterialCount() const
{
	return m_vMaterialData.size();
}

bool CGame::ChangeMaterialName(const string& OldName, const string& NewName)
{
	if (GetMaterial(NewName, false))
	{
		MB_WARN(("[" + NewName + "] 은 이미 존재하는 이름입니다. 다른 이름을 골라주세요.").c_str(), "재질 이름 충돌");
		return false;
	}

	size_t iMaterial{ m_mapMaterialNameToIndex[OldName] };
	CMaterialData* Material{ GetMaterial(OldName) };
	auto a =m_mapMaterialNameToIndex.find(OldName);
	
	m_mapMaterialNameToIndex.erase(OldName);
	m_mapMaterialNameToIndex.insert(make_pair(NewName, iMaterial));

	Material->Name(NewName);

	return true;
}

ID3D11ShaderResourceView* CGame::GetMaterialTextureSRV(STextureData::EType eType, const string& Name) const
{
	assert(m_mapMaterialNameToIndex.find(Name) != m_mapMaterialNameToIndex.end());
	size_t iMaterial{ m_mapMaterialNameToIndex.at(Name) };

	if (m_vMaterialTextureSets[iMaterial]) return m_vMaterialTextureSets[iMaterial]->GetTextureSRV(eType);
	return nullptr;
}

bool CGame::InsertLight(CLight::EType eType, const std::string& Name)
{
	string InstanceName{ Name };
	if (m_Light->InsertInstance(InstanceName, eType))
	{
		m_LightRep->InsertInstance(InstanceName);
		return true;
	}
	return false;
}

void CGame::DeleteLight(const std::string& Name)
{
	if (m_Light->DeleteInstance(Name))
	{
		m_LightRep->DeleteInstance(Name);
	}
}

void CGame::ClearLights()
{
	m_Light->ClearInstances();
	m_LightRep->ClearInstances();
}

void CGame::SetMode(EMode eMode)
{
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
	if (IsGizmoSelected()) return;

	if (m_eMode == EMode::Edit)
	{	
		// Multiple region-selection
		SelectMultipleObjects();

		if (!m_vSelectionData.size())
		{
			// Single ray-selection

			CastPickingRay();
			UpdatePickingRay();

			PickBoundingSphere();
			PickObject3DTriangle();
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

		UpdatePickingRay();

		PickBoundingSphere();
	}
}

void CGame::Select(const CGame::SSelectionData& SelectionData)
{
	DeselectAll();

	m_vSelectionData.emplace_back(SelectionData);

	static constexpr size_t KSelectionIndex{ 0 };
	switch (SelectionData.eObjectType)
	{
	default:
	case CGame::EObjectType::NONE:
	case CGame::EObjectType::Object3DLine:
		m_vSelectionData.clear();
		return;

	case CGame::EObjectType::Object3D:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		const XMVECTOR KObjectTranslation{ Object3D->ComponentTransform.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset };
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = KObjectTranslation;
		
		m_umapSelectionObject3D[SelectionData.Name] = KSelectionIndex;
		break;
	}
	case CGame::EObjectType::Object2D:
		break;
	case CGame::EObjectType::EditorCamera:
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_EditorCamera->GetEyePosition();

		m_EditorCameraSelectionIndex = KSelectionIndex;
		break;
	case CGame::EObjectType::Camera:
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_vCameras[GetCameraID(SelectionData.Name)]->GetEyePosition();

		m_umapSelectionCamera[SelectionData.Name] = KSelectionIndex;

		m_CameraRep->SetInstanceHighlight(SelectionData.Name, true);
		m_CameraRep->UpdateInstanceBuffers();
		break;
	case CGame::EObjectType::Light:
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_Light->GetInstanceGPUData(SelectionData.Name).Position;

		m_umapSelectionLight[SelectionData.Name] = KSelectionIndex;

		m_LightRep->SetInstanceHighlight(SelectionData.Name, true);
		m_LightRep->UpdateInstanceBuffer();
		break;
	case CGame::EObjectType::Object3DInstance:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		const XMVECTOR KInstanceTranslation{ 
			Object3D->GetInstanceCPUData(SelectionData.Name).Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset };
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = KInstanceTranslation;

		m_umapSelectionObject3DInstance[SelectionData.Name] = KSelectionIndex;
		break;
	}
	}

	Select3DGizmos();
}

void CGame::SelectAdditional(const SSelectionData& SelectionData)
{
	m_vSelectionData.emplace_back(SelectionData);
	const size_t KSelectionIndex{ m_vSelectionData.size() - 1 };

	switch (SelectionData.eObjectType)
	{
	default:
	case CGame::EObjectType::NONE:
	case CGame::EObjectType::Object3DLine:
		return;

	case CGame::EObjectType::Object3D:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		const XMVECTOR KObjectTranslation{ Object3D->ComponentTransform.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset };
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = KObjectTranslation;

		m_umapSelectionObject3D[SelectionData.Name] = KSelectionIndex;

		m_MultipleSelectionWorldCenter += KObjectTranslation;
		break;
	}
	case CGame::EObjectType::Object2D:
		break;
	case CGame::EObjectType::EditorCamera:
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_EditorCamera->GetEyePosition();

		m_EditorCameraSelectionIndex = KSelectionIndex;

		m_MultipleSelectionWorldCenter += m_EditorCamera->GetEyePosition();
		break;
	case CGame::EObjectType::Camera:
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_vCameras[GetCameraID(SelectionData.Name)]->GetEyePosition();

		m_umapSelectionCamera[SelectionData.Name] = KSelectionIndex;

		m_CameraRep->SetInstanceHighlight(SelectionData.Name, true);
		m_CameraRep->UpdateInstanceBuffers();

		m_MultipleSelectionWorldCenter += m_vCameras[GetCameraID(SelectionData.Name)]->GetEyePosition();
		break;
	case CGame::EObjectType::Light:
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_Light->GetInstanceGPUData(SelectionData.Name).Position;

		m_umapSelectionLight[SelectionData.Name] = KSelectionIndex;

		m_LightRep->SetInstanceHighlight(SelectionData.Name, true);
		m_LightRep->UpdateInstanceBuffer();

		m_MultipleSelectionWorldCenter += m_Light->GetInstanceGPUData(SelectionData.Name).Position;
		break;
	case CGame::EObjectType::Object3DInstance:
	{
		CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
		const XMVECTOR KInstanceTranslation{
			Object3D->GetInstanceCPUData(SelectionData.Name).Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset };
		m_GizmoRecentTranslation = m_CapturedGizmoTranslation = KInstanceTranslation;

		m_umapSelectionObject3DInstance[SelectionData.Name] = KSelectionIndex;

		m_MultipleSelectionWorldCenter += KInstanceTranslation;
		break;
	}
	}

	Select3DGizmos();
}

void CGame::Deselect(const SSelectionData& Data)
{
	switch (Data.eObjectType)
	{
	case CGame::EObjectType::NONE:
	default:
		break;
	case CGame::EObjectType::Object3D:
		if (m_umapSelectionObject3D.find(Data.Name) != m_umapSelectionObject3D.end())
		{
			size_t iSelectionData{ m_umapSelectionObject3D.at(Data.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject3D.erase(Data.Name);
			m_vSelectionData.pop_back();
		}
		break;
	case CGame::EObjectType::Object3DLine:
		if (m_umapSelectionObject3DLine.find(Data.Name) != m_umapSelectionObject3DLine.end())
		{
			size_t iSelectionData{ m_umapSelectionObject3DLine.at(Data.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject3DLine.erase(Data.Name);
			m_vSelectionData.pop_back();
		}
		break;
	case CGame::EObjectType::Object2D:
		if (m_umapSelectionObject2D.find(Data.Name) != m_umapSelectionObject2D.end())
		{
			size_t iSelectionData{ m_umapSelectionObject2D.at(Data.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject2D.erase(Data.Name);
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
		if (m_umapSelectionCamera.find(Data.Name) != m_umapSelectionCamera.end())
		{
			size_t iSelectionData{ m_umapSelectionCamera.at(Data.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionCamera.erase(Data.Name);
			m_vSelectionData.pop_back();

			m_CameraRep->SetInstanceHighlight(Data.Name, false);
			m_CameraRep->UpdateInstanceBuffers();
		}
		break;
	case CGame::EObjectType::Light:
		if (m_umapSelectionLight.find(Data.Name) != m_umapSelectionLight.end())
		{
			size_t iSelectionData{ m_umapSelectionLight.at(Data.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionLight.erase(Data.Name);
			m_vSelectionData.pop_back();

			m_LightRep->SetInstanceHighlight(Data.Name, false);
			m_LightRep->UpdateInstanceBuffer();
		}
		break;
	case CGame::EObjectType::Object3DInstance:
		if (m_umapSelectionObject3DInstance.find(Data.Name) != m_umapSelectionObject3DInstance.end())
		{
			size_t iSelectionData{ m_umapSelectionObject3DInstance.at(Data.Name) };
			if (iSelectionData < m_vSelectionData.size() - 1)
			{
				swap(m_vSelectionData[iSelectionData], m_vSelectionData.back());
			}
			m_umapSelectionObject3DInstance.erase(Data.Name);
			m_vSelectionData.pop_back();
		}
		break;
	}
}

void CGame::Deselect(EObjectType eObjectType)
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
		m_CameraRep->UpdateInstanceBuffers();
		break;
	case CGame::EObjectType::Light:
		m_umapSelectionLight.clear();
		m_LightRep->SetAllInstancesHighlightOff();
		m_LightRep->UpdateInstanceBuffer();
		break;
	case CGame::EObjectType::Object3DInstance:
		m_umapSelectionObject3DInstance.clear();
		break;
	}
}

void CGame::DeselectAll()
{
	m_vSelectionData.clear();

	m_MultipleSelectionWorldCenter = XMVectorSet(0, 0, 0, 1);

	m_umapSelectionObject3D.clear();
	m_umapSelectionObject3DInstance.clear();
	m_umapSelectionObject3DLine.clear();
	m_umapSelectionObject2D.clear();
	m_EditorCameraSelectionIndex = KInvalidIndex;
	m_umapSelectionCamera.clear();
	m_umapSelectionLight.clear();

	m_CameraRep->SetAllInstancesHighlightOff();
	m_CameraRep->UpdateInstanceBuffers();

	m_LightRep->SetAllInstancesHighlightOff();
	m_LightRep->UpdateInstanceBuffer();

	Deselect3DGizmos();
}

bool CGame::IsAnythingSelected() const
{
	return !(m_vSelectionData.empty());
}

void CGame::CastPickingRay()
{
	float ViewSpaceRayDirectionX{ (m_CapturedMouseState.x / (m_WindowSize.x / 2.0f) - 1.0f) / XMVectorGetX(m_MatrixProjection.r[0]) };
	float ViewSpaceRayDirectionY{ (-(m_CapturedMouseState.y / (m_WindowSize.y / 2.0f) - 1.0f)) / XMVectorGetY(m_MatrixProjection.r[1]) };
	static float ViewSpaceRayDirectionZ{ 1.0f };

	static XMVECTOR ViewSpaceRayOrigin{ XMVectorSet(0, 0, 0, 1) };
	XMVECTOR ViewSpaceRayDirection{ XMVectorSet(ViewSpaceRayDirectionX, ViewSpaceRayDirectionY, ViewSpaceRayDirectionZ, 0) };

	XMMATRIX MatrixViewInverse{ XMMatrixInverse(nullptr, m_MatrixView) };
	m_PickingRayWorldSpaceOrigin = XMVector3TransformCoord(ViewSpaceRayOrigin, MatrixViewInverse);
	m_PickingRayWorldSpaceDirection = XMVector3TransformNormal(ViewSpaceRayDirection, MatrixViewInverse);
}

void CGame::UpdatePickingRay()
{
	m_PickingRayRep->GetVertices().at(0).Position = m_PickingRayWorldSpaceOrigin;
	m_PickingRayRep->GetVertices().at(1).Position = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * KPickingRayLength;
	m_PickingRayRep->UpdateVertexBuffer();
}

void CGame::PickBoundingSphere()
{
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
				Select(SSelectionData(EObjectType::Camera, Camera->GetName()));
				break;
			}
		}
	}
	if (IsAnythingSelected()) return;

	// Pick Object3D
	XMVECTOR T{ KVectorGreatest };
	for (const auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->ComponentPhysics.bIsPickable)
		{
			if (Object3D->IsInstanced())
			{
				for (const auto& InstanceCPUData : Object3D->GetInstanceCPUDataVector())
				{
					XMVECTOR NewT{ KVectorGreatest };
					if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
						InstanceCPUData.BoundingSphere.Radius, InstanceCPUData.Translation + InstanceCPUData.BoundingSphere.CenterOffset, &NewT))
					{
						m_vObject3DPickingCandidates.emplace_back(Object3D.get(), InstanceCPUData.Name, NewT);
					}
				}
			}
			else
			{
				XMVECTOR NewT{ KVectorGreatest };
				if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
					Object3D->ComponentPhysics.BoundingSphere.Radius, 
					Object3D->ComponentTransform.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
				{
					m_vObject3DPickingCandidates.emplace_back(Object3D.get(), NewT);
				}
			}
		}
	}
	if (m_vObject3DPickingCandidates.size()) return;

	// Pick Light
	for (const auto& LightPair : m_Light->GetInstanceNameToIndexMap())
	{
		const auto& Instance{ m_Light->GetInstanceGPUData(LightPair.first) };

		XMVECTOR NewT{ KVectorGreatest };
		if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
			m_Light->GetBoundingSphereRadius(), Instance.Position, &NewT))
		{
			Select(SSelectionData(EObjectType::Light, LightPair.first));
			break;
		}
	}
	if (IsAnythingSelected()) return;
}

bool CGame::PickObject3DTriangle()
{
	XMVECTOR T{ KVectorGreatest };
	if (!IsAnythingSelected())
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
			XMMATRIX WorldMatrix{ Candidate.PtrObject3D->ComponentTransform.MatrixWorld };
			if (!Candidate.InstanceName.empty())
			{
				const auto& InstanceGPUData{ Candidate.PtrObject3D->GetInstanceGPUData(Candidate.InstanceName) };
				WorldMatrix = InstanceGPUData.WorldMatrix;
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
		if (!vFilteredCandidates.empty())
		{
			XMVECTOR TCmp{ KVectorGreatest };
			for (const SObject3DPickingCandiate& FilteredCandidate : vFilteredCandidates)
			{
				if (XMVector3Less(FilteredCandidate.T, TCmp))
				{
					if (FilteredCandidate.InstanceName.empty())
					{
						Select(SSelectionData(EObjectType::Object3D, FilteredCandidate.PtrObject3D->GetName(), FilteredCandidate.PtrObject3D));
					}
					else
					{
						Select(SSelectionData(EObjectType::Object3DInstance, FilteredCandidate.InstanceName, FilteredCandidate.PtrObject3D));
					}
				}
			}
			return true;
		}
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
	m_PtrCurrentCamera = Camera;
	m_CurrentCameraID = (int)m_mapCameraNameToIndex.at(Camera->GetName());
}

void CGame::SelectTerrain(bool bShouldEdit, bool bIsLeftButton)
{
	if (!m_Terrain) return;
	if (m_eEditMode != EEditMode::EditTerrain) return;

	CastPickingRay();

	m_Terrain->Select(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, bShouldEdit, bIsLeftButton);
}

void CGame::SelectMultipleObjects()
{
	const XMMATRIX KViewProjection{ m_MatrixView * m_MatrixProjection };

	DeselectAll();

	//OutputDebugString("\n\n=== Capturing current projection space ... ===\n");
	for (const auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->IsInstanced())
		{
			const auto& vInstanceCPUData{ Object3D->GetInstanceCPUDataVector() };
			for (const auto& Instance : vInstanceCPUData)
			{
				const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(Instance.Translation, KViewProjection) };
				const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };
				
				if (IsInsideSelectionRegion(KProjectionXY))
				{
					m_vSelectionData.emplace_back(EObjectType::Object3DInstance, Instance.Name, Object3D.get());
					m_MultipleSelectionWorldCenter += Instance.Translation;

					// @important
					m_umapSelectionObject3DInstance[Instance.Name] = m_vSelectionData.size() - 1;
				}
			}
		}
		else
		{
			//XMVECTOR ViewCenter{ XMVector3TransformCoord(Object3D->ComponentTransform.Translation, m_MatrixView) };
			//OutputDebugString(("View z: " + to_string(XMVectorGetZ(ViewCenter)) + "\n").c_str());

			const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(Object3D->ComponentTransform.Translation, KViewProjection) };
			const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };
			//OutputDebugString(("Projection center: (" + to_string(KProjectionXY.x) + ", " + to_string(KProjectionXY.y) + ")\n").c_str());

			if (IsInsideSelectionRegion(KProjectionXY))
			{
				m_vSelectionData.emplace_back(EObjectType::Object3D, Object3D->GetName(), Object3D.get());
				m_MultipleSelectionWorldCenter += Object3D->ComponentTransform.Translation;

				// @important
				m_umapSelectionObject3D[Object3D->GetName()] = m_vSelectionData.size() - 1;
			}
		}
	}

	size_t iCamera{};
	for (auto& Camera : m_vCameras)
	{
		const string& CameraName{ Camera->GetName() };
		const XMVECTOR& WorldPosition{ Camera->GetEyePosition() };
		const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(WorldPosition, KViewProjection) };
		const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };

		if (IsInsideSelectionRegion(KProjectionXY))
		{
			m_vSelectionData.emplace_back(EObjectType::Camera, CameraName);
			m_MultipleSelectionWorldCenter += WorldPosition;

			// @important
			m_umapSelectionCamera[CameraName] = m_vSelectionData.size() - 1;
			m_CameraRep->SetInstanceHighlight(CameraName, true);
		}
		++iCamera;
	}
	m_CameraRep->UpdateInstanceBuffers();

	for (const auto& LightPair : m_Light->GetInstanceNameToIndexMap())
	{
		const XMVECTOR& WorldPosition{ m_Light->GetInstanceGPUData(LightPair.first).Position };
		const XMVECTOR KProjectionCenter{ XMVector3TransformCoord(WorldPosition, KViewProjection) };
		const XMFLOAT2 KProjectionXY{ XMVectorGetX(KProjectionCenter), XMVectorGetY(KProjectionCenter) };

		if (IsInsideSelectionRegion(KProjectionXY))
		{
			m_vSelectionData.emplace_back(EObjectType::Light, LightPair.first);
			m_MultipleSelectionWorldCenter += WorldPosition;

			// @important
			m_umapSelectionLight[LightPair.first] = m_vSelectionData.size() - 1;
			m_LightRep->SetInstanceHighlight(LightPair.first, true);
		}
	}
	m_LightRep->UpdateInstanceBuffer();

	m_MultipleSelectionWorldCenter /= (float)m_vSelectionData.size();

	m_GizmoRecentTranslation = m_CapturedGizmoTranslation = m_MultipleSelectionWorldCenter;
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

void CGame::Select3DGizmos()
{
	if (EFLAG_HAS_NO(m_eFlagsRendering, EFlagsRendering::Use3DGizmos)) return;
	if (m_eMode != EMode::Edit) return;
	
	if (!IsAnythingSelected()) return;

	if (IsGizmoSelected())
	{
		int DeltaX{ m_CapturedMouseState.x - m_PrevCapturedMouseX };
		int DeltaY{ m_CapturedMouseState.y - m_PrevCapturedMouseY };
		int DeltaSum{ DeltaY - DeltaX };

		float DistanceObejctCamera{ abs(XMVectorGetX(m_CapturedGizmoTranslation - GetCurrentCamera()->GetEyePosition())) };
		if (DistanceObejctCamera < K3DGizmoCameraDistanceThreshold) DistanceObejctCamera = K3DGizmoCameraDistanceThreshold;
		const float KDeltaFactor{ pow(DistanceObejctCamera, K3DGizmoDistanceFactorExponent) };

		float DeltaTranslationScalar{ -DeltaSum * KTranslationDelta * KDeltaFactor };
		float DeltaRotationScalar{ -DeltaSum * KRotation360To2PI * KRotationDelta * KDeltaFactor };
		float DeltaScalingScalar{ -DeltaSum * KScalingDelta * KDeltaFactor };
		switch (m_e3DGizmoMode)
		{
		case CGame::E3DGizmoMode::Translation:
			DeltaRotationScalar = 0;
			DeltaScalingScalar = 0;
			break;
		case CGame::E3DGizmoMode::Rotation:
			DeltaTranslationScalar = 0;
			DeltaScalingScalar = 0;
			break;
		case CGame::E3DGizmoMode::Scaling:
			DeltaTranslationScalar = 0;
			DeltaRotationScalar = 0;
			break;
		default:
			break;
		}

		XMVECTOR DeltaTranslation{ XMVectorSet(
			(m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisX) ? DeltaTranslationScalar : 0,
			(m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisY) ? DeltaTranslationScalar : 0,
			(m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisZ) ? DeltaTranslationScalar : 0, 0) };
		float DeltaPitch{ (m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisX) ? DeltaRotationScalar : 0 };
		float DeltaYaw{ (m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisY) ? DeltaRotationScalar : 0 };
		float DeltaRoll{ (m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisZ) ? DeltaRotationScalar : 0 };
		XMVECTOR DeltaScaling{ XMVectorSet(
			(m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisX) ? DeltaScalingScalar : 0,
			(m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisY) ? DeltaScalingScalar : 0,
			(m_e3DGizmoSelectedAxis == E3DGizmoAxis::AxisZ) ? DeltaScalingScalar : 0, 0) };

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
				Camera->SetEyePosition(Camera->GetEyePosition() + DeltaTranslation);
				Camera->SetPitch(Camera->GetPitch() + DeltaPitch);
				Camera->SetYaw(Camera->GetYaw() + DeltaYaw);
				Camera->Update();
				if (Camera) m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, Camera->GetWorldMatrix());

				break;
			}
			case CGame::EObjectType::Light:
			{
				const XMVECTOR& KTranslation{ m_Light->GetInstanceGPUData(SelectionData.Name).Position };
				m_Light->SetInstancePosition(SelectionData.Name, KTranslation + DeltaTranslation);
				m_LightRep->SetInstancePosition(SelectionData.Name, KTranslation + DeltaTranslation);
				break;
			}
			case CGame::EObjectType::Object3D:
			case CGame::EObjectType::Object3DInstance:
			{
				CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
				if (SelectionData.eObjectType == EObjectType::Object3DInstance)
				{
					auto& InstanceCPUData{ Object3D->GetInstanceCPUData(SelectionData.Name) };
					InstanceCPUData.Translation += DeltaTranslation;
					InstanceCPUData.Pitch += DeltaPitch;
					InstanceCPUData.Yaw += DeltaYaw;
					InstanceCPUData.Roll += DeltaRoll;
					InstanceCPUData.Scaling += DeltaScaling;

					Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
				}
				else
				{
					Object3D->ComponentTransform.Translation += DeltaTranslation;
					Object3D->ComponentTransform.Pitch += DeltaPitch;
					Object3D->ComponentTransform.Yaw += DeltaYaw;
					Object3D->ComponentTransform.Roll += DeltaRoll;
					Object3D->ComponentTransform.Scaling += DeltaScaling;

					Object3D->UpdateWorldMatrix();
				}

				break;
			}
			}
		}

		m_GizmoRecentTranslation += DeltaTranslation;
	}
	else
	{
		// Gizmo is not selected.
		CastPickingRay();

		switch (m_e3DGizmoMode)
		{
		case E3DGizmoMode::Translation:
			m_bIsGizmoHovered = true;
			if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoTranslationX.get(), E3DGizmoAxis::AxisX))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoTranslationY.get(), E3DGizmoAxis::AxisY))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoTranslationZ.get(), E3DGizmoAxis::AxisZ))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
			}
			else
			{
				m_bIsGizmoHovered = false;
			}
			break;
		case E3DGizmoMode::Rotation:
		{
			XMVECTOR T{ KVectorGreatest };
			m_bIsGizmoHovered = false;
			if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationPitch.get(), E3DGizmoAxis::AxisX, &T))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
				m_bIsGizmoHovered = true;
			}
			if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationYaw.get(), E3DGizmoAxis::AxisY, &T))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
				m_bIsGizmoHovered = true;
			}
			if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationRoll.get(), E3DGizmoAxis::AxisZ, &T))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
				m_bIsGizmoHovered = true;
			}
			break;
		}
		case E3DGizmoMode::Scaling:
			m_bIsGizmoHovered = true;
			if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoScalingX.get(), E3DGizmoAxis::AxisX))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoScalingY.get(), E3DGizmoAxis::AxisY))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoScalingZ.get(), E3DGizmoAxis::AxisZ))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
			}
			else
			{
				m_bIsGizmoHovered = false;
			}
			break;
		default:
			break;
		}

		if (m_bIsGizmoHovered && m_CapturedMouseState.leftButton)
		{
			m_bIsGizmoSelected = true;
		}
	}

	// @important
	// Translate 3D gizmos
	if (m_vSelectionData.size() >= 2)
	{
		m_Object3D_3DGizmoTranslationX->ComponentTransform.Translation =
			m_Object3D_3DGizmoTranslationY->ComponentTransform.Translation = m_Object3D_3DGizmoTranslationZ->ComponentTransform.Translation =
			m_Object3D_3DGizmoRotationPitch->ComponentTransform.Translation =
			m_Object3D_3DGizmoRotationYaw->ComponentTransform.Translation = m_Object3D_3DGizmoRotationRoll->ComponentTransform.Translation =
			m_Object3D_3DGizmoScalingX->ComponentTransform.Translation =
			m_Object3D_3DGizmoScalingY->ComponentTransform.Translation = m_Object3D_3DGizmoScalingZ->ComponentTransform.Translation = m_GizmoRecentTranslation;
	}
	else
	{
		XMVECTOR GizmoTranslation{ m_GizmoRecentTranslation };
		EObjectType eObjectType{ m_vSelectionData.back().eObjectType };
		if (eObjectType == EObjectType::Object3D || eObjectType == EObjectType::Object3DInstance)
		{
			CObject3D* const Object3D{ (CObject3D*)m_vSelectionData.back().PtrObject };
			GizmoTranslation += Object3D->ComponentPhysics.BoundingSphere.CenterOffset;
		}
		m_Object3D_3DGizmoTranslationX->ComponentTransform.Translation =
			m_Object3D_3DGizmoTranslationY->ComponentTransform.Translation = m_Object3D_3DGizmoTranslationZ->ComponentTransform.Translation =
			m_Object3D_3DGizmoRotationPitch->ComponentTransform.Translation =
			m_Object3D_3DGizmoRotationYaw->ComponentTransform.Translation = m_Object3D_3DGizmoRotationRoll->ComponentTransform.Translation =
			m_Object3D_3DGizmoScalingX->ComponentTransform.Translation =
			m_Object3D_3DGizmoScalingY->ComponentTransform.Translation = m_Object3D_3DGizmoScalingZ->ComponentTransform.Translation = GizmoTranslation;
	}

	// @important
	// Calculate scalar IAW the distance from the camera	
	m_3DGizmoDistanceScalar = XMVectorGetX(XMVector3Length(
		m_PtrCurrentCamera->GetEyePosition() - m_Object3D_3DGizmoTranslationX->ComponentTransform.Translation)) * 0.1f;
	m_3DGizmoDistanceScalar = pow(m_3DGizmoDistanceScalar, 0.7f);

	m_PrevCapturedMouseX = m_CapturedMouseState.x;
	m_PrevCapturedMouseY = m_CapturedMouseState.y;
}

void CGame::Deselect3DGizmos()
{
	m_bIsGizmoSelected = false;
}

bool CGame::IsGizmoHovered() const
{
	return m_bIsGizmoHovered;
}

bool CGame::IsGizmoSelected() const
{
	return m_bIsGizmoSelected;
}

bool CGame::ShouldSelectRotationGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis, XMVECTOR* const OutPtrT)
{
	static constexpr float KHollowCylinderInnerRaidus{ 0.9375f };
	static constexpr float KHollowCylinderOuterRaidus{ 1.0625f };
	static constexpr float KHollowCylinderHeight{ 0.125f };

	XMVECTOR CylinderSpaceRayOrigin{ m_PickingRayWorldSpaceOrigin - Gizmo->ComponentTransform.Translation };
	XMVECTOR CylinderSpaceRayDirection{ m_PickingRayWorldSpaceDirection };

	XMVECTOR NewT{ KVectorGreatest };
	switch (Axis)
	{
	case E3DGizmoAxis::None:
	{
		return false;
	}
	case E3DGizmoAxis::AxisX:
	{
		XMMATRIX RotationMatrix{ XMMatrixRotationZ(XM_PIDIV2) };
		CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
		CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
		if (IntersectRayHollowCylinderCentered(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, KHollowCylinderHeight,
			KHollowCylinderInnerRaidus * m_3DGizmoDistanceScalar, KHollowCylinderOuterRaidus * m_3DGizmoDistanceScalar, &NewT))
		{
			if (XMVector3Less(NewT, *OutPtrT))
			{
				*OutPtrT = NewT;
				return true;
			}
		}
		break;
	}
	case E3DGizmoAxis::AxisY:
	{
		if (IntersectRayHollowCylinderCentered(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, KHollowCylinderHeight,
			KHollowCylinderInnerRaidus * m_3DGizmoDistanceScalar, KHollowCylinderOuterRaidus * m_3DGizmoDistanceScalar, &NewT))
		{
			if (XMVector3Less(NewT, *OutPtrT))
			{
				*OutPtrT = NewT;
				return true;
			}
		}
		break;
	}
	case E3DGizmoAxis::AxisZ:
	{
		XMMATRIX RotationMatrix{ XMMatrixRotationX(XM_PIDIV2) };
		CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
		CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
		if (IntersectRayHollowCylinderCentered(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, KHollowCylinderHeight,
			KHollowCylinderInnerRaidus * m_3DGizmoDistanceScalar, KHollowCylinderOuterRaidus * m_3DGizmoDistanceScalar, &NewT))
		{
			if (XMVector3Less(NewT, *OutPtrT))
			{
				*OutPtrT = NewT;
				return true;
			}
		}
		break;
	}
	default:
		break;
	}
	return false;
}

bool CGame::ShouldSelectTranslationScalingGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis)
{
	static constexpr float KGizmoLengthFactor{ 1.1875f };
	static constexpr float KGizmoRaidus{ 0.05859375f };
	XMVECTOR CylinderSpaceRayOrigin{ m_PickingRayWorldSpaceOrigin - Gizmo->ComponentTransform.Translation };
	XMVECTOR CylinderSpaceRayDirection{ m_PickingRayWorldSpaceDirection };
	switch (Axis)
	{
	case E3DGizmoAxis::None:
		return false;
		break;
	case E3DGizmoAxis::AxisX:
		{
			XMMATRIX RotationMatrix{ XMMatrixRotationZ(XM_PIDIV2) };
			CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
			CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
			if (IntersectRayCylinder(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, 
				KGizmoLengthFactor * m_3DGizmoDistanceScalar, KGizmoRaidus * m_3DGizmoDistanceScalar)) return true;
		}
		break;
	case E3DGizmoAxis::AxisY:
		if (IntersectRayCylinder(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, 
			KGizmoLengthFactor * m_3DGizmoDistanceScalar, KGizmoRaidus * m_3DGizmoDistanceScalar)) return true;
		break;
	case E3DGizmoAxis::AxisZ:
		{
			XMMATRIX RotationMatrix{ XMMatrixRotationX(-XM_PIDIV2) };
			CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
			CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
			if (IntersectRayCylinder(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, 
				KGizmoLengthFactor * m_3DGizmoDistanceScalar, KGizmoRaidus * m_3DGizmoDistanceScalar)) return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void CGame::BeginRendering(const FLOAT* ClearColor)
{
	ID3D11SamplerState* LinearWrapSampler{ m_CommonStates->LinearWrap() };
	ID3D11SamplerState* LinearClampSampler{ m_CommonStates->LinearClamp() };
	m_DeviceContext->PSSetSamplers(0, 1, &LinearWrapSampler);
	m_DeviceContext->PSSetSamplers(1, 1, &LinearClampSampler);
	m_DeviceContext->DSSetSamplers(0, 1, &LinearWrapSampler); // @important: in order to use displacement mapping

	SetUniversalRSState();

	XMVECTOR EyePosition{ m_PtrCurrentCamera->GetEyePosition() };
	XMVECTOR FocusPosition{ m_PtrCurrentCamera->GetFocusPosition() };
	XMVECTOR UpDirection{ m_PtrCurrentCamera->GetUpDirection() };
	m_MatrixView = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);

	if (m_EnvironmentTexture) m_EnvironmentTexture->Use();
	if (m_IrradianceTexture) m_IrradianceTexture->Use();
	if (m_PrefilteredRadianceTexture) m_PrefilteredRadianceTexture->Use();
	if (m_IntegratedBRDFTexture) m_IntegratedBRDFTexture->Use();
}

void CGame::Update()
{
	// Calculate time
	m_TimeNow = m_Clock.now().time_since_epoch().count();
	if (m_TimePrev == 0) m_TimePrev = m_TimeNow;
	m_DeltaTimeF = static_cast<float>((m_TimeNow - m_TimePrev) * 0.000'000'001);

	if (m_TimeNow > m_PreviousFrameTime + 1'000'000'000)
	{
		m_FPS = m_FrameCount;
		m_FrameCount = 0;
		m_PreviousFrameTime = m_TimeNow;
	}

	// Capture inputs
	m_CapturedKeyboardState = GetKeyState();
	m_CapturedMouseState = GetMouseState();

	// Process keyboard inputs
	if (m_CapturedKeyboardState.LeftAlt && m_CapturedKeyboardState.Q)
	{
		Destroy();
		return;
	}
	if (m_CapturedKeyboardState.Escape)
	{
		DeselectAll();
	}
	if (!ImGui::IsAnyItemActive())
	{
		if (m_CapturedKeyboardState.W)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Forward, m_DeltaTimeF);

			if (m_PtrCurrentCamera != m_EditorCamera.get())
				m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
		}
		if (m_CapturedKeyboardState.S)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Backward, m_DeltaTimeF);

			if (m_PtrCurrentCamera != m_EditorCamera.get())
				m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
		}
		if (m_CapturedKeyboardState.A  && !m_CapturedKeyboardState.LeftControl)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Leftward, m_DeltaTimeF);

			if (m_PtrCurrentCamera != m_EditorCamera.get())
				m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
		}
		if (m_CapturedKeyboardState.D)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Rightward, m_DeltaTimeF);

			if (m_PtrCurrentCamera != m_EditorCamera.get())
				m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
		}
		if (m_CapturedKeyboardState.D1)
		{
			Set3DGizmoMode(E3DGizmoMode::Translation);
		}
		if (m_CapturedKeyboardState.D2)
		{
			Set3DGizmoMode(E3DGizmoMode::Rotation);
		}
		if (m_CapturedKeyboardState.D3)
		{
			Set3DGizmoMode(E3DGizmoMode::Scaling);
		}
	}

	// Process moue inputs
	static int PrevMouseX{ m_CapturedMouseState.x };
	static int PrevMouseY{ m_CapturedMouseState.y };
	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
	{
		if (m_CapturedMouseState.rightButton) ImGui::SetWindowFocus(nullptr);

		if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
		{
			Select3DGizmos();

			if (m_bLeftButtonPressedOnce)
			{
				m_MultipleSelectionTopLeft = XMFLOAT2((float)m_CapturedMouseState.x, (float)m_CapturedMouseState.y);
				m_MultipleSelectionChanging = true;
			}

			if (IsGizmoSelected()) m_MultipleSelectionChanging = false;

			if (m_MultipleSelectionChanging)
			{
				m_MultipleSelectionBottomRight = XMFLOAT2((float)m_CapturedMouseState.x, (float)m_CapturedMouseState.y);
			}
			
			if (m_bLeftButtonUpOnce)
			{
				m_MultipleSelectionChanging = false;

				Select();
			}

			if (!m_CapturedMouseState.leftButton) Deselect3DGizmos();
			if (m_CapturedMouseState.rightButton) DeselectAll();

			if (m_CapturedMouseState.x != PrevMouseX || m_CapturedMouseState.y != PrevMouseY)
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

		if (m_CapturedMouseState.x != PrevMouseX || m_CapturedMouseState.y != PrevMouseY)
		{
			if (m_CapturedMouseState.middleButton)
			//if (m_CapturedMouseState.rightButton)
			{
				m_PtrCurrentCamera->Rotate(m_CapturedMouseState.x - PrevMouseX, m_CapturedMouseState.y - PrevMouseY, m_DeltaTimeF);

				if (m_PtrCurrentCamera != m_EditorCamera.get())
					m_CameraRep->UpdateInstanceWorldMatrix(m_PtrCurrentCamera->GetName(), m_PtrCurrentCamera->GetWorldMatrix());
			}

			PrevMouseX = m_CapturedMouseState.x;
			PrevMouseY = m_CapturedMouseState.y;
		}
	}

	m_TimePrev = m_TimeNow;
	++m_FrameCount;
}

void CGame::Draw()
{
	if (m_IsDestroyed) return;

	m_CBEditorTimeData.NormalizedTime += m_DeltaTimeF;
	m_CBEditorTimeData.NormalizedTimeHalfSpeed += m_DeltaTimeF * 0.5f;
	if (m_CBEditorTimeData.NormalizedTime > 1.0f) m_CBEditorTimeData.NormalizedTime = 0.0f;
	if (m_CBEditorTimeData.NormalizedTimeHalfSpeed > 1.0f) m_CBEditorTimeData.NormalizedTimeHalfSpeed = 0.0f;
	m_CBEditorTime->Update();

	m_CBWaterTimeData.Time += m_DeltaTimeF * 0.1f;
	if (m_CBWaterTimeData.Time > 1.0f) m_CBWaterTimeData.Time = 0.0f;
	m_CBWaterTime->Update();

	m_CBTerrainData.Time = m_CBEditorTimeData.NormalizedTime;

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	m_CBLightData.EyePosition = m_PtrCurrentCamera->GetEyePosition();
	m_CBLight->Update();
	m_CBDirectionalLight->Update();

	m_CBPSFlagsData.EnvironmentTextureMipLevels = (m_EnvironmentTexture) ? m_EnvironmentTexture->GetMipLevels() : 0;
	m_CBPSFlagsData.PrefilteredRadianceTextureMipLevels = (m_PrefilteredRadianceTexture) ? m_PrefilteredRadianceTexture->GetMipLevels() : 0;
	m_CBPSFlagsData.bUsePhysicallyBasedRendering = EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UsePhysicallyBasedRendering);
	m_CBPSFlags->Update();

	// Deferred shading
	{
		// @important
		SetForwardRenderTargets(true); // @important: just for clearing...
		SetDeferredRenderTargets(true);

		m_DeviceContext->OMSetBlendState(m_CommonStates->Opaque(), nullptr, 0xFFFFFFFF);

		// Terrain
		DrawTerrain(m_DeltaTimeF);

		// Opaque Object3Ds
		for (auto& Object3D : m_vObject3Ds)
		{
			if (Object3D->ComponentRender.bIsTransparent) continue;

			if (Object3D->IsRigged())
			{
				if (Object3D->HasBakedAnimationTexture())
				{
					UpdateCBAnimationData(Object3D->GetAnimationData());
				}
				else
				{
					UpdateCBAnimationBoneMatrices(Object3D->GetAnimationBoneMatrices());
				}

				Object3D->Animate(m_DeltaTimeF);
			}

			Object3D->UpdateWorldMatrix();
			DrawObject3D(Object3D.get());

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
			{
				DrawObject3DBoundingSphere(Object3D.get());
			}
		}

		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
		m_DeviceContext->OMSetRenderTargets(1, m_BackBufferRTV.GetAddressOf(), nullptr);

		ID3D11ShaderResourceView* SRVs[]{ m_DepthStencilSRV.Get(), m_BaseColorRoughSRV.Get(), m_NormalSRV.Get(), m_MetalAOSRV.Get() };

		// @important
		m_CBGBufferUnpackingData.PerspectiveValues =
			XMFLOAT4(1.0f / XMVectorGetX(m_MatrixProjection.r[0]), 1.0f / XMVectorGetY(m_MatrixProjection.r[1]), 
				XMVectorGetZ(m_MatrixProjection.r[3]), -XMVectorGetZ(m_MatrixProjection.r[2]));
		m_CBGBufferUnpackingData.InverseViewMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m_MatrixView));
		m_CBGBufferUnpacking->Update();

		// Directional light
		{
			// @important
			UpdateCBDirectionalLight();

			DrawFullScreenQuad(m_PSDirectionalLight.get(), SRVs, ARRAYSIZE(SRVs));
		}

		// Point light
		if (m_Light->GetInstanceCount() > 0)
		{
			m_DeviceContext->OMSetBlendState(m_BlendAdditiveLighting.Get(), nullptr, 0xFFFFFFFF);
			m_DeviceContext->RSSetState(m_CommonStates->CullCounterClockwise());

			UpdateCBSpace();

			m_VSLight->Use();
			m_HSPointLight->Use();
			m_DSPointLight->Use();
			m_PSPointLight->Use();

			ID3D11SamplerState* const Samplers[]{ m_CommonStates->PointClamp() };
			m_DeviceContext->PSSetSamplers(0, ARRAYSIZE(Samplers), Samplers);

			m_Light->Light();

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

		if (m_eMode == EMode::Edit)
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
		}

		if (m_SkyData.bIsDataSet)
		{
			DrawSky(m_DeltaTimeF);
		}

		if (m_eMode == EMode::Edit)
		{
			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals))
			{
				UpdateCBSpace();
				m_GSNormal->Use();
			}
		}

		m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

		// Transparent Object3Ds
		for (auto& Object3D : m_vObject3Ds)
		{
			if (!Object3D->ComponentRender.bIsTransparent) continue;

			Object3D->Animate(m_DeltaTimeF);
			Object3D->UpdateWorldMatrix();
			DrawObject3D(Object3D.get());

			if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
			{
				DrawObject3DBoundingSphere(Object3D.get());
			}
		}
	}
	
	// 2D
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
		m_DeviceContext->GSSetShader(nullptr, nullptr, 0);

		DrawObject2Ds();

		if (m_eMode == EMode::Edit)
		{
			DrawRegionSelectionRep();

			m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);

			// Not 2D, but Billboard
			DrawLightRep();
		}
		else
		{
			m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
		}
	}
}

void CGame::DrawObject3D(CObject3D* const PtrObject3D, bool bIgnoreInstances, bool bIgnoreOwnTexture)
{
	if (!PtrObject3D) return;

	UpdateCBSpace(PtrObject3D->ComponentTransform.MatrixWorld);

	SetUniversalbUseLighiting();

	// @important: flag setting algorithms are different from each other!
	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoLighting)) m_CBPSFlagsData.bUseLighting = false;
	m_CBPSFlagsData.bUseTexture = EFLAG_HAS_NO(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoTexture);
	m_CBPSFlags->Update();

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRSState();
	}

	if (PtrObject3D->IsRigged())
	{
		m_VSAnimation->Use();
	}
	else
	{
		(PtrObject3D->IsInstanced() && !bIgnoreInstances) ? m_VSInstance->Use() : m_VSBase->Use();
	}
	
	if (PtrObject3D->ShouldTessellate())
	{
		UpdateCBTessFactorData(PtrObject3D->GetTessFactorData());
		UpdateCBDisplacementData(PtrObject3D->GetDisplacementData());

		m_HSStatic->Use();
		m_DSStatic->Use();
	}

	if (m_bIsDeferredRenderTargetsSet)
	{
		m_PSBase_gbuffer->Use();
	}
	else
	{
		m_PSBase->Use();
	}

	PtrObject3D->Draw(bIgnoreOwnTexture, bIgnoreInstances);

	if (PtrObject3D->ShouldTessellate())
	{
		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	}
}

void CGame::DrawObject3DBoundingSphere(const CObject3D* const PtrObject3D)
{
	m_VSBase->Use();

	XMMATRIX Translation{ XMMatrixTranslationFromVector(PtrObject3D->ComponentTransform.Translation + 
		PtrObject3D->ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX Scaling{ XMMatrixScaling(PtrObject3D->ComponentPhysics.BoundingSphere.Radius,
		PtrObject3D->ComponentPhysics.BoundingSphere.Radius, PtrObject3D->ComponentPhysics.BoundingSphere.Radius) };
	UpdateCBSpace(Scaling * Translation);

	m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

	m_BoundingSphereRep->Draw();

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
	m_PSBase2D->Use();

	for (auto& Object2D : m_vObject2Ds)
	{
		if (!Object2D->IsVisible()) continue;

		UpdateCBSpace(Object2D->GetWorldMatrix());
		
		m_CBPS2DFlagsData.bUseTexture = (Object2D->HasTexture()) ? TRUE : FALSE;
		m_CBPS2DFlags->Update();

		Object2D->Draw();
	}
}

void CGame::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	m_VSBase->Use();
	m_PSBase->Use();

	m_CBPSFlagsData.bUseLighting = false;
	m_CBPSFlagsData.bUseTexture = false;
	m_CBPSFlags->Update();

	for (auto& Object3D : m_vMiniAxes)
	{
		UpdateCBSpace(Object3D->ComponentTransform.MatrixWorld);

		Object3D->Draw();

		Object3D->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition() + m_PtrCurrentCamera->GetForward();

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
	
	m_PSVertexColor->Use();

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

void CGame::DrawSky(float DeltaTime)
{
	m_CBPSFlagsData.bUseLighting = false;
	m_CBPSFlags->Update();

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
			m_Object3DSkySphere->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition();
			m_Object3DSkySphere->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DSkySphere->ComponentTransform.MatrixWorld);

			m_Object3DSkySphere->Draw();
		}

		m_PSBase->Use();

		// Sun
		{
			float SunRoll{ XM_2PI * m_CBSkyTimeData.SkyTime - XM_PIDIV2 };
			XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, SunRoll)) };
			m_Object3DSun->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition() + Offset;
			m_Object3DSun->ComponentTransform.Roll = SunRoll;
			m_Object3DSun->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DSun->ComponentTransform.MatrixWorld);
			
			m_Object3DSun->Draw();
		}

		// Moon
		{
			float MoonRoll{ XM_2PI * m_CBSkyTimeData.SkyTime + XM_PIDIV2 };
			XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, MoonRoll)) };
			m_Object3DMoon->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition() + Offset;
			m_Object3DMoon->ComponentTransform.Roll = (MoonRoll > XM_2PI) ? (MoonRoll - XM_2PI) : MoonRoll;
			m_Object3DMoon->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DMoon->ComponentTransform.MatrixWorld);

			m_Object3DMoon->Draw();
		}
	}
	else
	{
		// SkySphere
		{
			m_Object3DSkySphere->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition();
			m_Object3DSkySphere->UpdateWorldMatrix();
			UpdateCBSpace(m_Object3DSkySphere->ComponentTransform.MatrixWorld);

			m_VSSky->Use();
			m_PSSky->Use();

			m_Object3DSkySphere->Draw();
		}
	}

	m_CBSkyTime->Update();
}

void CGame::DrawTerrain(float DeltaTime)
{
	if (!m_Terrain) return;
	
	SetUniversalRSState();
	SetUniversalbUseLighiting();
	
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

	if (false)
	{
		UpdateCBSpace(m_Terrain->GetWindRepresentationWorldMatrix());
		m_Terrain->DrawWindRepresentation();
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

		if (m_vSelectionData.back().eObjectType == EObjectType::Object3D)
		{
			CObject3D* const Object3D{ (CObject3D*)m_vSelectionData.back().PtrObject };
			if (Object3D->IsInstanced()) return;
		}
	}

	m_VSGizmo->Use();
	m_PSGizmo->Use();

	switch (m_e3DGizmoMode)
	{
	case E3DGizmoMode::Translation:
		Draw3DGizmoTranslations(m_e3DGizmoSelectedAxis);
		break;
	case E3DGizmoMode::Rotation:
		Draw3DGizmoRotations(m_e3DGizmoSelectedAxis);
		break;
	case E3DGizmoMode::Scaling:
		Draw3DGizmoScalings(m_e3DGizmoSelectedAxis);
		break;
	default:
		break;
	}
}

void CGame::Draw3DGizmoTranslations(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

	if (IsGizmoHovered())
	{
		switch (Axis)
		{
		case E3DGizmoAxis::AxisX:
			bHighlightX = true;
			break;
		case E3DGizmoAxis::AxisY:
			bHighlightY = true;
			break;
		case E3DGizmoAxis::AxisZ:
			bHighlightZ = true;
			break;
		default:
			break;
		}
	}

	Draw3DGizmo(m_Object3D_3DGizmoTranslationX.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoTranslationY.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoTranslationZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmoRotations(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

	if (IsGizmoHovered())
	{
		switch (Axis)
		{
		case E3DGizmoAxis::AxisX:
			bHighlightX = true;
			break;
		case E3DGizmoAxis::AxisY:
			bHighlightY = true;
			break;
		case E3DGizmoAxis::AxisZ:
			bHighlightZ = true;
			break;
		default:
			break;
		}
	}

	Draw3DGizmo(m_Object3D_3DGizmoRotationPitch.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoRotationYaw.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoRotationRoll.get(), bHighlightZ);
}

void CGame::Draw3DGizmoScalings(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

	if (IsGizmoHovered())
	{
		switch (Axis)
		{
		case E3DGizmoAxis::AxisX:
			bHighlightX = true;
			break;
		case E3DGizmoAxis::AxisY:
			bHighlightY = true;
			break;
		case E3DGizmoAxis::AxisZ:
			bHighlightZ = true;
			break;
		default:
			break;
		}
	}

	Draw3DGizmo(m_Object3D_3DGizmoScalingX.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoScalingY.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoScalingZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmo(CObject3D* const Gizmo, bool bShouldHighlight)
{
	float Scalar{ XMVectorGetX(XMVector3Length(m_PtrCurrentCamera->GetEyePosition() - Gizmo->ComponentTransform.Translation)) * 0.1f };
	Scalar = pow(Scalar, 0.7f);

	Gizmo->ComponentTransform.Scaling = XMVectorSet(Scalar, Scalar, Scalar, 0.0f);
	Gizmo->UpdateWorldMatrix();
	UpdateCBSpace(Gizmo->ComponentTransform.MatrixWorld);

	m_CBGizmoColorFactorData.ColorFactor = (bShouldHighlight) ? XMVectorSet(2.0f, 2.0f, 2.0f, 0.95f) : XMVectorSet(0.75f, 0.75f, 0.75f, 0.75f);
	m_CBGizmoColorFactor->Update();

	Gizmo->Draw();
}

void CGame::DrawCameraRep()
{
	if (!m_vCameras.size()) return;

	m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	
	UpdateCBSpace();

	m_CBCameraData.CurrentCameraID = m_CurrentCameraID;
	m_CBCamera->Update();

	m_VSInstance->Use();
	m_PSCamera->Use();

	m_CameraRep->Draw();
	
	SetUniversalRSState();
}

void CGame::DrawLightRep()
{
	if (!m_LightRep) return;

	ID3D11SamplerState* const Samplers[]{ m_CommonStates->LinearClamp(), m_CommonStates->PointClamp() };
	m_DeviceContext->PSSetSamplers(0, 2, Samplers);

	UpdateCBBillboard(m_LightRep->GetCBBillboard());
	UpdateCBSpace();

	m_VSBillboard->Use();
	m_HSBillboard->Use();
	m_DSBillboard->Use();
	m_PSBillboard->Use();

	m_LightRep->Draw();

	m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
	m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
}

void CGame::DrawRegionSelectionRep()
{
	if (!m_MultipleSelectionChanging) return;
	
	m_CBPS2DFlagsData.bUseTexture = FALSE;
	m_CBPS2DFlags->Update();

	m_VSBase2D->Use();
	m_PSBase2D->Use();

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

	UpdateCBSpace(m_MultipleSelectionRep->GetWorldMatrix());

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
				SetMode(EMode::Test);
			}

			ImGui::End();
		}
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
				SetMode(EMode::Edit);
			}

			ImGui::End();
		}
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
			static CFileDialog FileDialog{ GetWorkingDirectory() };
			if (FileDialog.OpenFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 불러오기"))
			{
				LoadTerrain(FileDialog.GetRelativeFileName());
			}
		}

		if (bShowDialogSaveTerrain)
		{
			if (GetTerrain())
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
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

		static const char* const KLightTypes[1]{ u8"점 광원" };
		static int iSelectedLightType{};

		static XMFLOAT4 MaterialUniformColor{ 1.0f, 1.0f, 1.0f, 1.0f };

		bool bShowDialogLoad3DModel{};

		ImGui::SetItemDefaultFocus();
		ImGui::SetNextItemWidth(140);
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
				if (ImGui::BeginCombo(u8"##카메라 종류", KLightTypes[iSelectedLightType]))
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
							Object3D->ComponentPhysics.BoundingSphere.Radius = sqrt(2.0f);
							break;
						case 1:
							Mesh = GenerateSquareXZPlane();
							ScaleMesh(Mesh, XMVectorSet(WidthScalar3D, 1.0f, HeightScalar3D, 0));
							Object3D->ComponentPhysics.BoundingSphere.Radius = sqrt(2.0f);
							break;
						case 2:
							Mesh = GenerateSquareYZPlane();
							ScaleMesh(Mesh, XMVectorSet(1.0f, WidthScalar3D, HeightScalar3D, 0));
							Object3D->ComponentPhysics.BoundingSphere.Radius = sqrt(2.0f);
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
							Object3D->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSetY(Object3D->ComponentPhysics.BoundingSphere.CenterOffset, -0.5f);
							break;
						case 6:
							Mesh = GenerateCylinder(1.0f, 1.0f, SideCount);
							Object3D->ComponentPhysics.BoundingSphere.Radius = sqrt(1.5f);
							break;
						case 7:
							Mesh = GenerateSphere(SegmentCount);
							break;
						case 8:
							Mesh = GenerateTorus(InnerRadius, SideCount, SegmentCount);
							Object3D->ComponentPhysics.BoundingSphere.Radius += InnerRadius;
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
							Object3D->LoadOB3D(ModelFileNameWithPath);
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
						InsertCamera(NewObejctName);
						CCamera* const Camera{ GetCamera(NewObejctName) };
						switch (iSelectedCameraType)
						{
						case 0:
							Camera->SetType(CCamera::EType::FirstPerson);
							break;
						case 1:
							Camera->SetType(CCamera::EType::ThirdPerson);
							break;
						case 2:
							Camera->SetType(CCamera::EType::FreeLook);
							break;
						default:
							break;
						}
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
			static CFileDialog FileDialog{ GetWorkingDirectory() };
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
		ImGui::SetNextWindowPos(ImVec2(m_WindowSize.x - KInitialWindowWidth, 21), ImGuiCond_Appearing);
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

					static constexpr float KLabelsWidth{ 220 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };

					// No information about multiple selection
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

								if (SelectionData.eObjectType == EObjectType::Object3DInstance)
								{
									auto& InstanceCPUData{ Object3D->GetInstanceCPUData(SelectionData.Name) };

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"선택된 인스턴스:");
									ImGui::SameLine(ItemsOffsetX);
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"<%s>", InstanceCPUData.Name.c_str());

									ImGui::Separator();

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"위치");
									ImGui::SameLine(ItemsOffsetX);
									float Translation[3]{ XMVectorGetX(InstanceCPUData.Translation),
										XMVectorGetY(InstanceCPUData.Translation), XMVectorGetZ(InstanceCPUData.Translation) };
									if (ImGui::DragFloat3(u8"##위치", Translation, KTranslationDelta,
										KTranslationMinLimit, KTranslationMaxLimit, "%.2f"))
									{
										InstanceCPUData.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
										Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"회전");
									ImGui::SameLine(ItemsOffsetX);
									int PitchYawRoll360[3]{ (int)(InstanceCPUData.Pitch * KRotation2PITo360),
										(int)(InstanceCPUData.Yaw * KRotation2PITo360),
										(int)(InstanceCPUData.Roll * KRotation2PITo360) };
									if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, KRotation360Unit,
										KRotation360MinLimit, KRotation360MaxLimit))
									{
										InstanceCPUData.Pitch = PitchYawRoll360[0] * KRotation360To2PI;
										InstanceCPUData.Yaw = PitchYawRoll360[1] * KRotation360To2PI;
										InstanceCPUData.Roll = PitchYawRoll360[2] * KRotation360To2PI;
										Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"크기");
									ImGui::SameLine(ItemsOffsetX);
									float Scaling[3]{ XMVectorGetX(InstanceCPUData.Scaling),
										XMVectorGetY(InstanceCPUData.Scaling), XMVectorGetZ(InstanceCPUData.Scaling) };
									if (ImGui::DragFloat3(u8"##크기", Scaling, KScalingDelta,
										KScalingMinLimit, KScalingMaxLimit, "%.3f"))
									{
										InstanceCPUData.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
										Object3D->UpdateInstanceWorldMatrix(SelectionData.Name);
									}

									ImGui::Separator();
								}
								else
								{
									if (Object3D->IsInstanced())
									{
										ImGui::Text(u8"<인스턴스를 선택해 주세요.>");
									}
									else
									{
										// Non-instanced Object3D
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"위치");
										ImGui::SameLine(ItemsOffsetX);
										float Translation[3]{ XMVectorGetX(Object3D->ComponentTransform.Translation),
										XMVectorGetY(Object3D->ComponentTransform.Translation), XMVectorGetZ(Object3D->ComponentTransform.Translation) };
										if (ImGui::DragFloat3(u8"##위치", Translation, KTranslationDelta,
											KTranslationMinLimit, KTranslationMaxLimit, "%.2f"))
										{
											Object3D->ComponentTransform.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
											Object3D->UpdateWorldMatrix();
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"회전");
										ImGui::SameLine(ItemsOffsetX);
										int PitchYawRoll360[3]{ (int)(Object3D->ComponentTransform.Pitch * KRotation2PITo360),
											(int)(Object3D->ComponentTransform.Yaw * KRotation2PITo360),
											(int)(Object3D->ComponentTransform.Roll * KRotation2PITo360) };
										if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, KRotation360Unit,
											KRotation360MinLimit, KRotation360MaxLimit))
										{
											Object3D->ComponentTransform.Pitch = PitchYawRoll360[0] * KRotation360To2PI;
											Object3D->ComponentTransform.Yaw = PitchYawRoll360[1] * KRotation360To2PI;
											Object3D->ComponentTransform.Roll = PitchYawRoll360[2] * KRotation360To2PI;
											Object3D->UpdateWorldMatrix();
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"크기");
										ImGui::SameLine(ItemsOffsetX);
										float Scaling[3]{ XMVectorGetX(Object3D->ComponentTransform.Scaling),
											XMVectorGetY(Object3D->ComponentTransform.Scaling), XMVectorGetZ(Object3D->ComponentTransform.Scaling) };
										if (ImGui::DragFloat3(u8"##크기", Scaling, KScalingDelta,
											KScalingMinLimit, KScalingMaxLimit, "%.3f"))
										{
											Object3D->ComponentTransform.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
											Object3D->UpdateWorldMatrix();
										}
									}
								}

								ImGui::Separator();

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"오브젝트 BS 중심");
								ImGui::SameLine(ItemsOffsetX);
								float BSCenterOffset[3]{
									XMVectorGetX(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
									XMVectorGetY(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
									XMVectorGetZ(Object3D->ComponentPhysics.BoundingSphere.CenterOffset) };
								if (ImGui::DragFloat3(u8"##오브젝트 BS 중심", BSCenterOffset, KBSCenterOffsetDelta,
									KBSCenterOffsetMinLimit, KBSCenterOffsetMaxLimit, "%.2f"))
								{
									Object3D->ComponentPhysics.BoundingSphere.CenterOffset =
										XMVectorSet(BSCenterOffset[0], BSCenterOffset[1], BSCenterOffset[2], 1.0f);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"오브젝트 BS 반지름 편중치");
								ImGui::SameLine(ItemsOffsetX);
								float BSRadiusBias{ Object3D->ComponentPhysics.BoundingSphere.RadiusBias };
								if (ImGui::DragFloat(u8"##오브젝트 BS반지름 편중치", &BSRadiusBias, KBSRadiusBiasDelta,
									KBSRadiusBiasMinLimit, KBSRadiusBiasMaxLimit, "%.2f"))
								{
									Object3D->ComponentPhysics.BoundingSphere.RadiusBias = BSRadiusBias;
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"오브젝트 BS 반지름 (자동)");
								ImGui::SameLine(ItemsOffsetX);
								float BSRadius{ Object3D->ComponentPhysics.BoundingSphere.Radius };
								ImGui::DragFloat(u8"##오브젝트 BS반지름 (자동)", &BSRadius, KBSRadiusDelta,
									KBSRadiusMinLimit, KBSRadiusMaxLimit, "%.2f");

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

								// Tessellation data
								ImGui::Separator();

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

								// Material data
								ImGui::Separator();

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"오브젝트 재질");
								if (Object3D->GetMaterialCount() > 0)
								{
									static CMaterialData* capturedMaterialData{};
									static CMaterialTextureSet* capturedMaterialTextureSet{};
									static STextureData::EType ecapturedTextureType{};
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
									DrawEditorGUIPopupMaterialNameChanger(capturedMaterialData, true);
								}

								// Rigged model
								if (Object3D->IsRigged())
								{
									ImGui::Separator();

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"애니메이션 개수:");
									ImGui::SameLine(ItemsOffsetX);
									int AnimationCount{ Object3D->GetAnimationCount() };
									ImGui::Text(u8"%d", AnimationCount);

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"애니메이션 ID");
									ImGui::SameLine(ItemsOffsetX);
									int AnimationID{ Object3D->GetAnimationID() };
									if (ImGui::SliderInt(u8"##애니메이션 ID", &AnimationID, 0, Object3D->GetAnimationCount() - 1))
									{
										Object3D->SetAnimationID(AnimationID);
									}

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
											static CFileDialog FileDialog{ GetWorkingDirectory() };
											if (FileDialog.SaveFileDialog("애니메이션 텍스처 파일(*.dds)\0*.dds\0", "애니메이션 텍스처 저장", ".dds"))
											{
												Object3D->BakeAnimationTexture();
												Object3D->SaveBakedAnimationTexture(FileDialog.GetFileName());
											}
										}
									}

									ImGui::SameLine();

									if (ImGui::Button(u8"열기"))
									{
										static CFileDialog FileDialog{ GetWorkingDirectory() };
										if (FileDialog.OpenFileDialog("애니메이션 텍스처 파일(*.dds)\0*.dds\0", "애니메이션 텍스처 불러오기"))
										{
											Object3D->LoadBakedAnimationTexture(FileDialog.GetFileName());
										}
									}

									if (ImGui::ListBoxHeader(u8"##애니메이션 목록", ImVec2(WindowWidth, 0)))
									{
										for (int iAnimation = 0; iAnimation < AnimationCount; ++iAnimation)
										{
											ImGui::PushID(iAnimation);
											if (ImGui::Selectable(Object3D->GetAnimationName(iAnimation).c_str(), (iAnimation == iSelectedAnimationID)))
											{
												iSelectedAnimationID = iAnimation;
											}
											ImGui::PopID();
										}

										ImGui::ListBoxFooter();
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
								ImGui::InputText(u8"##애니메이션 이름", Name, 16);

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"파일 이름");
								ImGui::SameLine(150);
								static char FileName[MAX_PATH]{};
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"%s", FileName);

								if (ImGui::Button(u8"불러오기"))
								{
									static CFileDialog FileDialog{ GetWorkingDirectory() };
									if (FileDialog.OpenFileDialog("모델 파일(*.fbx)\0*.fbx\0", "애니메이션 불러오기"))
									{
										strcpy_s(FileName, FileDialog.GetRelativeFileName().c_str());
									}
								}

								ImGui::SameLine();

								if (ImGui::Button(u8"결정"))
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

								if (ImGui::Button(u8"취소"))
								{
									bShowAnimationAdder = false;
									ImGui::CloseCurrentPopup();
								}

								ImGui::EndPopup();
							}

							if (bShowAnimationEditor) ImGui::OpenPopup(u8"애니메이션 수정");
							if (ImGui::BeginPopupModal(u8"애니메이션 수정", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
							{
								static bool bFirstTime{ true };
								CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"애니메이션 이름");
								ImGui::SameLine(150);
								static char AnimationName[CObject3D::KMaxAnimationNameLength]{};
								if (bFirstTime)
								{
									strcpy_s(AnimationName, Object3D->GetAnimationName(iSelectedAnimationID).c_str());
									bFirstTime = false;
								}
								ImGui::InputText(u8"##애니메이션 이름", AnimationName, CObject3D::KMaxAnimationNameLength);

								if (ImGui::Button(u8"결정"))
								{
									Object3D->SetAnimationName(iSelectedAnimationID, AnimationName);

									bFirstTime = true;
									bShowAnimationEditor = false;
									ImGui::CloseCurrentPopup();
								}

								ImGui::SameLine();

								if (ImGui::Button(u8"취소"))
								{
									bFirstTime = true;
									bShowAnimationEditor = false;
									ImGui::CloseCurrentPopup();
								}

								ImGui::EndPopup();
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
								const XMVECTOR& KEyePosition{ SelectedCamera->GetEyePosition() };
								float EyePosition[3]{ XMVectorGetX(KEyePosition), XMVectorGetY(KEyePosition), XMVectorGetZ(KEyePosition) };
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

										Select3DGizmos();
									}
								}

								ImGui::Separator();

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"선택된 카메라:");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"<%s>", SelectedCamera->GetName().c_str());

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"카메라 종류:");
								ImGui::SameLine(ItemsOffsetX);
								switch (SelectedCamera->GetType())
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
								if (ImGui::DragFloat3(u8"##위치", EyePosition, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									SelectedCamera->SetEyePosition(XMVectorSet(EyePosition[0], EyePosition[1], EyePosition[2], 1.0f));

									m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"회전 Pitch");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##회전 Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									SelectedCamera->SetPitch(Pitch);

									m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"회전 Yaw");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##회전 Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									SelectedCamera->SetYaw(Yaw);

									m_CameraRep->UpdateInstanceWorldMatrix(SelectionData.Name, SelectedCamera->GetWorldMatrix());
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"카메라 이동 속도");
								ImGui::SameLine(ItemsOffsetX);
								float MovementFactor{ SelectedCamera->GetMovementFactor() };
								if (ImGui::SliderFloat(u8"##카메라 이동 속도", &MovementFactor, 1.0f, 100.0f, "%.0f"))
								{
									SelectedCamera->SetMovementFactor(MovementFactor);
								}

								if (m_PtrCurrentCamera != SelectedCamera)
								{
									ImGui::SetCursorPosX(ItemsOffsetX);
									if (ImGui::Button(u8"현재 화면 카메라로 지정", ImVec2(ItemsWidth, 0)))
									{
										UseCamera(SelectedCamera);

										Select3DGizmos();
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
								const auto& SelectedLightCPU{ m_Light->GetInstanceCPUData(SelectionData.Name) };
								const auto& SelectedLightGPU{ m_Light->GetInstanceGPUData(SelectionData.Name) };

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
									m_Light->SetInstancePosition(SelectionData.Name, XMVectorSet(Position[0], Position[1], Position[2], 1.0f));
									m_LightRep->SetInstancePosition(SelectionData.Name, SelectedLightGPU.Position);
								}

								float Color[3]{ XMVectorGetX(SelectedLightGPU.Color), XMVectorGetY(SelectedLightGPU.Color), XMVectorGetZ(SelectedLightGPU.Color) };
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"색상");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::ColorEdit3(u8"##색상", Color, ImGuiColorEditFlags_RGB))
								{
									m_Light->SetInstanceColor(SelectionData.Name, XMVectorSet(Color[0], Color[1], Color[2], 1.0f));
								}

								float Range{ SelectedLightGPU.Range };
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"범위");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##범위", &Range, 0.1f, 0.1f, 10.0f, "%.1f"))
								{
									m_Light->SetInstanceRange(SelectionData.Name, Range);
								}
							}
							ImGui::PopItemWidth();
							break;
						}
						}

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
						ImGui::PopItemWidth();

						static bool bShowMaterialSelection{ false };
						static int iSelectedMaterialID{};
						if (Terrain)
						{
							if (ImGui::Button(u8"재질 추가"))
							{
								iSelectedMaterialID = -1;
								ImGui::OpenPopup(u8"재질 선택");
							}

							for (int iMaterial = 0; iMaterial < Terrain->GetMaterialCount(); ++iMaterial)
							{
								ImGui::PushItemWidth(100);
								ImGui::PushID(iMaterial);
								if (ImGui::Button(u8"변경"))
								{
									iSelectedMaterialID = iMaterial;
									bShowMaterialSelection = true;
								}
								ImGui::PopID();
								ImGui::PopItemWidth();

								ImGui::SameLine();

								const CMaterialData& MaterialData{ Terrain->GetMaterial(iMaterial) };
								ImGui::Text(u8"재질[%d] %s", iMaterial, MaterialData.Name().c_str());
							}
						}
						else
						{
							ImGui::Text(u8"텍스쳐: 없음");
						}

						if (bShowMaterialSelection) ImGui::OpenPopup(u8"재질 선택");
						if (ImGui::BeginPopup(u8"재질 선택", ImGuiWindowFlags_AlwaysAutoResize))
						{
							static int ListIndex{};
							static const string* SelectedMaterialName{};
							const auto& mapMaterial{ GetMaterialMap() };

							if (ImGui::ListBoxHeader("", (int)mapMaterial.size()))
							{
								int iListItem{};
								for (const auto& pairMaterial : mapMaterial)
								{
									if (ImGui::Selectable(pairMaterial.first.c_str(), (iListItem == ListIndex) ? true : false))
									{
										ListIndex = iListItem;
										SelectedMaterialName = &pairMaterial.first;
									}
									++iListItem;
								}
								ImGui::ListBoxFooter();
							}

							if (ImGui::Button(u8"결정"))
							{
								if (SelectedMaterialName)
								{
									if (iSelectedMaterialID == -1)
									{
										Terrain->AddMaterial(*GetMaterial(*SelectedMaterialName));
									}
									else
									{
										Terrain->SetMaterial(iSelectedMaterialID, *GetMaterial(*SelectedMaterialName));
									}
								}

								ImGui::CloseCurrentPopup();
							}

							bShowMaterialSelection = false;

							ImGui::EndPopup();
						}
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
								static CFileDialog FileDialog{ GetWorkingDirectory() };
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

				// 재질 탭
				if (ImGui::BeginTabItem(u8"재질"))
				{
					static constexpr float KLabelsWidth{ 220 };
					static constexpr float KItemsMaxWidth{ 240 };
					float ItemsWidth{ WindowWidth - KLabelsWidth };
					ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
					float ItemsOffsetX{ WindowWidth - ItemsWidth - 20 };

					ImGui::PushItemWidth(ItemsWidth);

					{
						ImGui::PushID(0);
						if (ImGui::Button(u8"새 재질 추가"))
						{
							size_t Count{ GetMaterialCount() };
							InsertMaterial("Material" + to_string(Count));
						}
						ImGui::PopID();

						static CMaterialData* capturedMaterialData{};
						static CMaterialTextureSet* capturedMaterialTextureSet{};
						static STextureData::EType ecapturedTextureType{};
						if (!ImGui::IsPopupOpen(u8"텍스처탐색기")) m_EditorGUIBools.bShowPopupMaterialTextureExplorer = false;

						const auto& mapMaterialList{ GetMaterialMap() };
						for (auto& pairMaterial : mapMaterialList)
						{
							CMaterialData* MaterialData{ GetMaterial(pairMaterial.first) };
							CMaterialTextureSet* MaterialTextureSet{ GetMaterialTextureSet(pairMaterial.first) };
							if (DrawEditorGUIWindowPropertyEditor_MaterialData(*MaterialData, MaterialTextureSet, ecapturedTextureType, ItemsWidth))
							{
								capturedMaterialData = MaterialData;
								capturedMaterialTextureSet = MaterialTextureSet;
							}
						}

						DrawEditorGUIPopupMaterialTextureExplorer(capturedMaterialData, capturedMaterialTextureSet, ecapturedTextureType);
						DrawEditorGUIPopupMaterialNameChanger(capturedMaterialData, true);

						ImGui::EndTabItem();
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
							static CFileDialog FileDialog{ GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0HDR 파일\0*.hdr\0", "Environment map 불러오기"))
							{
								m_EnvironmentTexture->CreateCubeMapFromFile(FileDialog.GetFileName());

								if (m_EnvironmentTexture->IsHDR())
								{
									GenerateCubemapFromHDRi();
								}
							}
						}

						if (m_EnvironmentTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Environment map");

							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8" : %s", m_EnvironmentTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetWorkingDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.DDS\0", "Environment map 내보내기", ".DDS"))
								{
									m_EnvironmentTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							m_CBSpace2DData.World = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2DData.Projection = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2D->Update();

							m_EnvironmentRep->UnfoldCubemap(m_EnvironmentTexture->GetShaderResourceViewPtr());
							SetForwardRenderTargets(); // @important

							ImGui::Image(m_EnvironmentRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx(u8"Irradiance map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0", "Irradiance map 불러오기"))
							{
								m_IrradianceTexture->CreateCubeMapFromFile(FileDialog.GetFileName());
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
							}
						}

						if (m_IrradianceTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Irradiance map");

							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8" : %s", m_IrradianceTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetWorkingDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.dds\0", "Irradiance map 내보내기", ".dds"))
								{
									m_IrradianceTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							m_CBSpace2DData.World = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2DData.Projection = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2D->Update();

							m_IrradianceRep->UnfoldCubemap(m_IrradianceTexture->GetShaderResourceViewPtr());
							SetForwardRenderTargets(); // @important

							ImGui::Image(m_IrradianceRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (ImGui::TreeNodeEx(u8"Prefiltered radiance map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0", "Prefiltered radiance map 불러오기"))
							{
								m_PrefilteredRadianceTexture->CreateCubeMapFromFile(FileDialog.GetFileName());
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
								static CFileDialog FileDialog{ GetWorkingDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.DDS\0", "Prefiltered radiance map 내보내기", ".DDS"))
								{
									m_PrefilteredRadianceTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							m_CBSpace2DData.World = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2DData.Projection = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2D->Update();

							m_PrefilteredRadianceRep->UnfoldCubemap(m_PrefilteredRadianceTexture->GetShaderResourceViewPtr());
							SetForwardRenderTargets(); // @important

							ImGui::Image(m_PrefilteredRadianceRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx(u8"Integrated BRDF map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0", "Integrated BRDF map 불러오기"))
							{
								m_IntegratedBRDFTexture->CreateTextureFromFile(FileDialog.GetFileName(), false);
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
								static CFileDialog FileDialog{ GetWorkingDirectory() };
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

					static constexpr float KLabelsWidth{ 220 };
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
							ToggleGameRenderingFlags(EFlagsRendering::DrawWireFrame);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"법선 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawNormals{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals) };
						if (ImGui::Checkbox(u8"##법선 표시", &bDrawNormals))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawNormals);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"화면 상단에 좌표축 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawMiniAxes{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawMiniAxes) };
						if (ImGui::Checkbox(u8"##화면 상단에 좌표축 표시", &bDrawMiniAxes))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawMiniAxes);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"Bounding Sphere 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawBoundingSphere{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere) };
						if (ImGui::Checkbox(u8"##Bounding Sphere 표시", &bDrawBoundingSphere))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawBoundingSphere);
						}


						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"지형 높이맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawTerrainHeightMapTexture{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainHeightMapTexture) };
						if (ImGui::Checkbox(u8"##지형 높이맵 표시", &bDrawTerrainHeightMapTexture))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawTerrainHeightMapTexture);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"지형 마스킹맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawTerrainMaskingTexture{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainMaskingTexture) };
						if (ImGui::Checkbox(u8"##지형 마스킹맵 표시", &bDrawTerrainMaskingTexture))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawTerrainMaskingTexture);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"지형 초목맵 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawTerrainFoliagePlacingTexture{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainFoliagePlacingTexture) };
						if (ImGui::Checkbox(u8"##지형 초목맵 표시", &bDrawTerrainFoliagePlacingTexture))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawTerrainFoliagePlacingTexture);
						}

						ImGui::Separator();
						ImGui::Separator();

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"엔진 플래그");

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

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"3D Gizmo 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bUse3DGizmos{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::Use3DGizmos) };
						if (ImGui::Checkbox(u8"##3D Gizmo 표시", &bUse3DGizmos))
						{
							ToggleGameRenderingFlags(EFlagsRendering::Use3DGizmos);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"피킹 데이터 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawPickingData{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawPickingData) };
						if (ImGui::Checkbox(u8"##피킹 데이터 표시", &bDrawPickingData))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawPickingData);
						}

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"그리드 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawGrid{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawGrid) };
						if (ImGui::Checkbox(u8"##그리드 표시", &bDrawGrid))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawGrid);
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
	STextureData::EType& eSeletedTextureType, float ItemsOffsetX)
{
	bool Result{ false };
	bool bUsePhysicallyBasedRendering{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UsePhysicallyBasedRendering) };

	if (ImGui::TreeNodeEx(MaterialData.Name().c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
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
		}

		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Specular 강도");
		ImGui::SameLine(ItemsOffsetX);
		float SpecularIntensity{ MaterialData.SpecularIntensity() };
		if (ImGui::DragFloat(u8"##Specular 강도", &SpecularIntensity, 0.01f, 0.0f, 1.0f, "%.2f"))
		{
			MaterialData.SpecularIntensity(SpecularIntensity);
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

		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"텍스처");

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
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::DiffuseTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = STextureData::EType::DiffuseTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::PushID(1);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Normal");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::NormalTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = STextureData::EType::NormalTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::PushID(2);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Opacity");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::OpacityTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = STextureData::EType::OpacityTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::PushID(3);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Specular Intensity");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::SpecularIntensityTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = STextureData::EType::SpecularIntensityTexture;
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
			if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::RoughnessTexture) : nullptr, KTextureSmallViewSize))
			{
				eSeletedTextureType = STextureData::EType::RoughnessTexture;
				m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
				Result = true;
			}
			ImGui::PopID();

			ImGui::PushID(5);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Metalness");
			ImGui::SameLine(ItemsOffsetX);
			if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::MetalnessTexture) : nullptr, KTextureSmallViewSize))
			{
				eSeletedTextureType = STextureData::EType::MetalnessTexture;
				m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
				Result = true;
			}
			ImGui::PopID();

			ImGui::PushID(6);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(u8"Ambient Occlusion");
			ImGui::SameLine(ItemsOffsetX);
			if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::AmbientOcclusionTexture) : nullptr, KTextureSmallViewSize))
			{
				eSeletedTextureType = STextureData::EType::AmbientOcclusionTexture;
				m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
				Result = true;
			}
			ImGui::PopID();
		}

		ImGui::PushID(7);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(u8"Displacement");
		ImGui::SameLine(ItemsOffsetX);
		if (ImGui::ImageButton((TextureSet) ? TextureSet->GetTextureSRV(STextureData::EType::DisplacementTexture) : nullptr, KTextureSmallViewSize))
		{
			eSeletedTextureType = STextureData::EType::DisplacementTexture;
			m_EditorGUIBools.bShowPopupMaterialTextureExplorer = true;
			Result = true;
		}
		ImGui::PopID();

		ImGui::TreePop();
	}

	return Result;
}

void CGame::DrawEditorGUIPopupMaterialNameChanger(CMaterialData*& capturedMaterialData, bool bIsEditorMaterial)
{
	static char OldName[KAssetNameMaxLength]{};
	static char NewName[KAssetNameMaxLength]{};

	// ### 재질 이름 변경 윈도우 ###
	if (m_EditorGUIBools.bShowPopupMaterialNameChanger) ImGui::OpenPopup(u8"재질 이름 변경");

	ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_Always);
	if (ImGui::BeginPopupModal(u8"재질 이름 변경", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::SetNextItemWidth(160);
		ImGui::InputText(u8"새 이름", NewName, KAssetNameMaxLength, ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::Separator();

		if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
		{
			strcpy_s(OldName, capturedMaterialData->Name().c_str());

			if (bIsEditorMaterial)
			{
				if (ChangeMaterialName(OldName, NewName))
				{
					ImGui::CloseCurrentPopup();
					m_EditorGUIBools.bShowPopupMaterialNameChanger = false;
					capturedMaterialData = nullptr;
				}
			}
			else
			{
				// TODO: 이름 충돌 검사
				// 에디터 재질이 아니면.. 이름 충돌해도 괜찮을까?

				capturedMaterialData->Name(NewName);
			}
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

void CGame::DrawEditorGUIPopupMaterialTextureExplorer(CMaterialData* const capturedMaterialData, CMaterialTextureSet* const capturedMaterialTextureSet,
	STextureData::EType eSelectedTextureType)
{
	// ### 텍스처 탐색기 윈도우 ###
	if (m_EditorGUIBools.bShowPopupMaterialTextureExplorer) ImGui::OpenPopup(u8"텍스처탐색기");
	if (ImGui::BeginPopup(u8"텍스처탐색기", ImGuiWindowFlags_AlwaysAutoResize))
	{
		ID3D11ShaderResourceView* SRV{};
		if (capturedMaterialTextureSet) SRV = capturedMaterialTextureSet->GetTextureSRV(eSelectedTextureType);

		if (ImGui::Button(u8"파일에서 텍스처 불러오기"))
		{
			static CFileDialog FileDialog{ GetWorkingDirectory() };
			if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
			{
				capturedMaterialData->SetTextureFileName(eSelectedTextureType, FileDialog.GetRelativeFileName());
				capturedMaterialTextureSet->CreateTexture(eSelectedTextureType, *capturedMaterialData);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"텍스처 해제하기"))
		{
			capturedMaterialData->ClearTextureData(eSelectedTextureType);
			capturedMaterialTextureSet->DestroyTexture(eSelectedTextureType);
		}

		ImGui::Image(SRV, ImVec2(600, 600));

		ImGui::EndPopup();
	}
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
		ImGui::SetNextWindowSizeConstraints(ImVec2(300, 60), ImVec2(400, 600));
		if (ImGui::Begin(u8"장면 편집기", &m_EditorGUIBools.bShowWindowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
		{
			// 장면 내보내기
			if (ImGui::Button(u8"장면 내보내기"))
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
				if (FileDialog.SaveFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 내보내기", ".scene"))
				{
					SaveScene(FileDialog.GetRelativeFileName(), FileDialog.GetDirectory());
				}
			}

			ImGui::SameLine();

			// 장면 불러오기
			if (ImGui::Button(u8"장면 불러오기"))
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
				if (FileDialog.OpenFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 불러오기"))
				{
					LoadScene(FileDialog.GetRelativeFileName(), FileDialog.GetDirectory());
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
				static CFileDialog FileDialog{ GetWorkingDirectory() };
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
								m_MeshPorter.ExportMesh(FileDialog.GetFileName(), Object3D->GetMESHData());
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
				DeleteSelectedObject();
			}

			ImGui::Separator();

			ImGui::Columns(2);
			ImGui::Text(u8"오브젝트 및 인스턴스"); ImGui::NextColumn();
			ImGui::Text(u8"인스턴스 관리"); ImGui::NextColumn();
			ImGui::Separator();

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

					bool bIsNodeOpen{ ImGui::TreeNodeEx(Object3DPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						Select(SSelectionData(EObjectType::Object3D, Object3DPair.first, Object3D));
					}

					ImGui::NextColumn();

					if (m_vSelectionData.size() == 1)
					{
						const SSelectionData& SelectionData{ m_vSelectionData.back() };

						if ((SelectionData.eObjectType == EObjectType::Object3D || SelectionData.eObjectType == EObjectType::Object3DInstance) &&
							!Object3D->IsRigged())
						{
							// 인스턴스 추가

							ImGui::PushID(iObject3DPair * 2 + 0);
							if (ImGui::Button(u8"추가"))
							{
								Object3D->InsertInstance();

								Select(SSelectionData(EObjectType::Object3DInstance, Object3D->GetLastInstanceCPUData().Name, Object3D));
							}
							ImGui::PopID();
						}

						if (SelectionData.eObjectType == EObjectType::Object3DInstance)
						{
							// 인스턴스 제거

							ImGui::SameLine();
							ImGui::PushID(iObject3DPair * 2 + 1);
							if (ImGui::Button(u8"제거"))
							{
								const SInstanceCPUData& InstanceCPUData{ Object3D->GetInstanceCPUData(SelectionData.Name) };
								Object3D->DeleteInstance(InstanceCPUData.Name);
								if (Object3D->GetInstanceCount() > 0)
								{
									Select(SSelectionData(EObjectType::Object3DInstance, Object3D->GetLastInstanceCPUData().Name, Object3D));
								}
								else
								{
									DeselectAll();
								}
							}
							ImGui::PopID();
						}
					}

					ImGui::NextColumn();

					if (bIsNodeOpen)
					{
						// 인스턴스 목록
						if (Object3D->IsInstanced())
						{
							for (const auto& InstancePair : Object3D->GetInstanceNameToIndexMap())
							{
								bool bSelected{ false };
								if (m_umapSelectionObject3DInstance.find(InstancePair.first) != m_umapSelectionObject3DInstance.end())
								{
									bSelected = true;
								}

								if (ImGui::Selectable(InstancePair.first.c_str(), bSelected))
								{
									Select(SSelectionData(EObjectType::Object3DInstance, InstancePair.first, Object3D));
								}
							}
						}

						ImGui::TreePop();
					}

					++iObject3DPair;
					if (!Object3D->IsInstanced()) ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
				}

				ImGui::TreePop();
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
						Select(SSelectionData(EObjectType::Object2D, Object2DPair.first, Object2D));
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
					bool bIsNodeOpen{ ImGui::TreeNodeEx(CameraPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						Select(SSelectionData(EObjectType::Camera, CameraPair.first));
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
						Select(SSelectionData(EObjectType::EditorCamera));
					}
					ImGui::TreePop();
				}
				ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx(u8"광원", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (const auto& LightPair : m_Light->GetInstanceNameToIndexMap())
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
						Select(SSelectionData(EObjectType::Light, LightPair.first));
					}
					if (bIsNodeOpen)
					{
						ImGui::TreePop();
					}
					ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
				}
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}
}

void CGame::GenerateCubemapFromHDRi()
{
	m_EnvironmentTexture->GetTexture2DPtr()->GetDesc(&m_GeneratedEnvironmentMapTextureDesc);

	// @important
	m_GeneratedEnvironmentMapTextureDesc.Width /= 4; // @important: HDRi -> Cubemap
	m_GeneratedEnvironmentMapTextureDesc.Height /= 2; // @important: HDRi -> Cubemap
	m_GeneratedEnvironmentMapTextureDesc.ArraySize = 6;
	m_GeneratedEnvironmentMapTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_GeneratedEnvironmentMapTextureDesc.CPUAccessFlags = 0;
	m_GeneratedEnvironmentMapTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
	m_GeneratedEnvironmentMapTextureDesc.MipLevels = 0;
	m_GeneratedEnvironmentMapTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	m_GeneratedEnvironmentMapTextureDesc.SampleDesc.Count = 1;
	m_GeneratedEnvironmentMapTextureDesc.SampleDesc.Quality = 0;
	m_GeneratedEnvironmentMapTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	m_Device->CreateTexture2D(&m_GeneratedEnvironmentMapTextureDesc, nullptr, m_GeneratedEnvironmentMapTexture.ReleaseAndGetAddressOf());

	// @important
	m_vGeneratedEnvironmentMapRTV.resize(6);

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
	RTVDesc.Format = m_GeneratedEnvironmentMapTextureDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	RTVDesc.Texture2DArray.ArraySize = 1;

	for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
	{
		RTVDesc.Texture2DArray.FirstArraySlice = iCubeFace;
		m_Device->CreateRenderTargetView(m_GeneratedEnvironmentMapTexture.Get(), &RTVDesc, m_vGeneratedEnvironmentMapRTV[iCubeFace].ReleaseAndGetAddressOf());
	}

	m_Device->CreateShaderResourceView(m_GeneratedEnvironmentMapTexture.Get(), nullptr, m_GeneratedEnvironmentMapSRV.ReleaseAndGetAddressOf());

	// Draw!
	{
		D3D11_VIEWPORT Viewport{};
		Viewport.Width = static_cast<FLOAT>(m_GeneratedEnvironmentMapTextureDesc.Width);
		Viewport.Height = static_cast<FLOAT>(m_GeneratedEnvironmentMapTextureDesc.Height);
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		m_DeviceContext->RSSetViewports(1, &Viewport);

		ID3D11SamplerState* LinearWrap{ m_CommonStates->LinearWrap() };
		m_DeviceContext->PSSetSamplers(0, 1, &LinearWrap);

		m_EnvironmentTexture->Use(0);

		m_VSScreenQuad->Use();
		m_PSFromHDR->Use();

		m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DeviceContext->IASetVertexBuffers(0, 1, m_CubemapVertexBuffer.GetAddressOf(), &m_CubemapVertexBufferStride, &m_CubemapVertexBufferOffset);

		for (auto& RTV : m_vGeneratedEnvironmentMapRTV)
		{
			if (RTV) m_DeviceContext->ClearRenderTargetView(RTV.Get(), Colors::Blue);
		}

		for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
		{
			m_DeviceContext->OMSetRenderTargets(1, m_vGeneratedEnvironmentMapRTV[iCubeFace].GetAddressOf(), nullptr);
			m_DeviceContext->Draw(6, iCubeFace * 6);
		}

		SetForwardRenderTargets();
	}

	m_DeviceContext->GenerateMips(m_GeneratedEnvironmentMapSRV.Get());

	m_EnvironmentTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_EnvironmentTexture->CopyTexture(m_GeneratedEnvironmentMapTexture.Get());
	m_EnvironmentTexture->SetSlot(KEnvironmentTextureSlot);
}

void CGame::GenerateIrradianceMap(float RangeFactor)
{
	const uint32_t KMipLevelBias{ 4 }; // @important: Maybe becuase of my poor hardware...???

	m_EnvironmentTexture->GetTexture2DPtr()->GetDesc(&m_GeneratedIrradianceMapTextureDesc);

	// @important
	m_GeneratedIrradianceMapTextureDesc.Width /= (uint32_t)pow(2.0f, (float)KMipLevelBias);
	m_GeneratedIrradianceMapTextureDesc.Height /= (uint32_t)pow(2.0f, (float)KMipLevelBias);
	m_GeneratedIrradianceMapTextureDesc.ArraySize = 6;
	m_GeneratedIrradianceMapTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_GeneratedIrradianceMapTextureDesc.CPUAccessFlags = 0;
	m_GeneratedIrradianceMapTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
	m_GeneratedIrradianceMapTextureDesc.MipLevels = 0;
	m_GeneratedIrradianceMapTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	m_GeneratedIrradianceMapTextureDesc.SampleDesc.Count = 1;
	m_GeneratedIrradianceMapTextureDesc.SampleDesc.Quality = 0;
	m_GeneratedIrradianceMapTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	m_Device->CreateTexture2D(&m_GeneratedIrradianceMapTextureDesc, nullptr, m_GeneratedIrradianceMapTexture.ReleaseAndGetAddressOf());

	// @important
	m_vGeneratedIrradianceMapRTV.resize(6);

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
	RTVDesc.Format = m_GeneratedIrradianceMapTextureDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	RTVDesc.Texture2DArray.ArraySize = 1;

	for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
	{
		RTVDesc.Texture2DArray.FirstArraySlice = iCubeFace;
		m_Device->CreateRenderTargetView(m_GeneratedIrradianceMapTexture.Get(), &RTVDesc, m_vGeneratedIrradianceMapRTV[iCubeFace].ReleaseAndGetAddressOf());
	}

	m_Device->CreateShaderResourceView(m_GeneratedIrradianceMapTexture.Get(), nullptr, m_GeneratedIrradianceMapSRV.ReleaseAndGetAddressOf());

	// Draw!
	{
		D3D11_VIEWPORT Viewport{};
		Viewport.Width = static_cast<FLOAT>(m_GeneratedIrradianceMapTextureDesc.Width);
		Viewport.Height = static_cast<FLOAT>(m_GeneratedIrradianceMapTextureDesc.Height);
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		m_DeviceContext->RSSetViewports(1, &Viewport);

		ID3D11SamplerState* LinearWrap{ m_CommonStates->LinearWrap() };
		m_DeviceContext->PSSetSamplers(0, 1, &LinearWrap);

		m_EnvironmentTexture->Use(0);

		m_VSScreenQuad->Use();
		m_PSIrradianceGenerator->Use();

		// @important
		m_CBIrradianceGeneratorData.RangeFactor = RangeFactor;
		m_CBIrradianceGenerator->Update();

		m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DeviceContext->IASetVertexBuffers(0, 1, m_CubemapVertexBuffer.GetAddressOf(), &m_CubemapVertexBufferStride, &m_CubemapVertexBufferOffset);

		for (auto& RTV : m_vGeneratedIrradianceMapRTV)
		{
			if (RTV) m_DeviceContext->ClearRenderTargetView(RTV.Get(), Colors::Blue);
		}

		for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
		{
			m_DeviceContext->OMSetRenderTargets(1, m_vGeneratedIrradianceMapRTV[iCubeFace].GetAddressOf(), nullptr);
			m_DeviceContext->Draw(6, iCubeFace * 6);
		}

		SetForwardRenderTargets();
	}

	m_DeviceContext->GenerateMips(m_GeneratedIrradianceMapSRV.Get());

	m_IrradianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_IrradianceTexture->CopyTexture(m_GeneratedIrradianceMapTexture.Get());
	m_IrradianceTexture->SetSlot(KIrradianceTextureSlot);
}

void CGame::GeneratePrefilteredRadianceMap(float RangeFactor)
{
	m_EnvironmentTexture->GetTexture2DPtr()->GetDesc(&m_PrefilteredRadianceMapTextureDesc);

	const uint32_t KMipLevelBias{ 2 }; // @important: Maybe becuase of my poor hardware...???
	const uint32_t BiasedMipMax{ m_PrefilteredRadianceMapTextureDesc.MipLevels - KMipLevelBias };

	// @important
	m_PrefilteredRadianceMapTextureDesc.Width /= (uint32_t)pow(2.0f, (float)KMipLevelBias);
	m_PrefilteredRadianceMapTextureDesc.Height /= (uint32_t)pow(2.0f, (float)KMipLevelBias);
	m_PrefilteredRadianceMapTextureDesc.ArraySize = 6;
	m_PrefilteredRadianceMapTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_PrefilteredRadianceMapTextureDesc.CPUAccessFlags = 0;
	m_PrefilteredRadianceMapTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
	m_PrefilteredRadianceMapTextureDesc.MipLevels = BiasedMipMax;
	m_PrefilteredRadianceMapTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
	m_PrefilteredRadianceMapTextureDesc.SampleDesc.Count = 1;
	m_PrefilteredRadianceMapTextureDesc.SampleDesc.Quality = 0;
	m_PrefilteredRadianceMapTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	m_Device->CreateTexture2D(&m_PrefilteredRadianceMapTextureDesc, nullptr, m_PrefilteredRadianceMapTexture.ReleaseAndGetAddressOf());

	m_Device->CreateShaderResourceView(m_PrefilteredRadianceMapTexture.Get(), nullptr, m_PrefilteredRadianceMapSRV.ReleaseAndGetAddressOf());

	// Draw!
	{
		ID3D11SamplerState* LinearWrap{ m_CommonStates->LinearWrap() };
		m_DeviceContext->PSSetSamplers(0, 1, &LinearWrap);

		m_EnvironmentTexture->Use(0);

		m_VSScreenQuad->Use();
		m_PSRadiancePrefiltering->Use();

		m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DeviceContext->IASetVertexBuffers(0, 1, m_CubemapVertexBuffer.GetAddressOf(), &m_CubemapVertexBufferStride, &m_CubemapVertexBufferOffset);

		{
			m_vPrefilteredRadianceMapRTV.resize(6);

			D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
			RTVDesc.Format = m_PrefilteredRadianceMapTextureDesc.Format;
			RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			RTVDesc.Texture2DArray.ArraySize = 1;

			for (uint32_t iMipLevel = 0; iMipLevel < BiasedMipMax; ++iMipLevel)
			{
				// @important
				float Roughness{ (float)(iMipLevel) / (float)(BiasedMipMax - 1) };
				m_CBRadiancePrefilteringData.Roughness = Roughness;
				m_CBRadiancePrefilteringData.RangeFactor = RangeFactor;
				m_CBRadiancePrefiltering->Update();

				D3D11_VIEWPORT Viewport{};
				Viewport.Width = static_cast<FLOAT>(m_PrefilteredRadianceMapTextureDesc.Width / (uint32_t)pow(2.0f, (float)iMipLevel));
				Viewport.Height = static_cast<FLOAT>(m_PrefilteredRadianceMapTextureDesc.Height / (uint32_t)pow(2.0f, (float)iMipLevel));
				Viewport.TopLeftX = 0.0f;
				Viewport.TopLeftY = 0.0f;
				Viewport.MinDepth = 0.0f;
				Viewport.MaxDepth = 1.0f;
				m_DeviceContext->RSSetViewports(1, &Viewport);

				for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
				{
					RTVDesc.Texture2DArray.MipSlice = iMipLevel;
					RTVDesc.Texture2DArray.FirstArraySlice = iCubeFace;
					m_Device->CreateRenderTargetView(m_PrefilteredRadianceMapTexture.Get(), &RTVDesc, m_vPrefilteredRadianceMapRTV[iCubeFace].ReleaseAndGetAddressOf());
				}

				for (auto& RTV : m_vPrefilteredRadianceMapRTV)
				{
					if (RTV) m_DeviceContext->ClearRenderTargetView(RTV.Get(), Colors::Blue);
				}

				for (int iCubeFace = 0; iCubeFace < 6; ++iCubeFace)
				{
					m_DeviceContext->OMSetRenderTargets(1, m_vPrefilteredRadianceMapRTV[iCubeFace].GetAddressOf(), nullptr);
					m_DeviceContext->Draw(6, iCubeFace * 6);
				}
			}
		}
		
		SetForwardRenderTargets();
	}

	m_PrefilteredRadianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_PrefilteredRadianceTexture->CopyTexture(m_PrefilteredRadianceMapTexture.Get());
	m_PrefilteredRadianceTexture->SetSlot(KPrefilteredRadianceTextureSlot);
}

void CGame::GenerateIntegratedBRDFMap()
{
	// @important
	m_IntegratedBRDFTextureDesc.Width = 512;
	m_IntegratedBRDFTextureDesc.Height = 512;
	m_IntegratedBRDFTextureDesc.ArraySize = 1;
	m_IntegratedBRDFTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_IntegratedBRDFTextureDesc.CPUAccessFlags = 0;
	m_IntegratedBRDFTextureDesc.Format = DXGI_FORMAT_R16G16_UNORM;
	m_IntegratedBRDFTextureDesc.MipLevels = 0;
	m_IntegratedBRDFTextureDesc.MiscFlags = 0;
	m_IntegratedBRDFTextureDesc.SampleDesc.Count = 1;
	m_IntegratedBRDFTextureDesc.SampleDesc.Quality = 0;
	m_IntegratedBRDFTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	m_Device->CreateTexture2D(&m_IntegratedBRDFTextureDesc, nullptr, m_IntegratedBRDFTextureRaw.ReleaseAndGetAddressOf());

	m_Device->CreateShaderResourceView(m_IntegratedBRDFTextureRaw.Get(), nullptr, m_IntegratedBRDFSRV.ReleaseAndGetAddressOf());
	
	{
		// @importnat: clamp!
		ID3D11SamplerState* LinearClamp{ m_CommonStates->LinearClamp() };
		m_DeviceContext->PSSetSamplers(0, 1, &LinearClamp);

		m_VSScreenQuad->Use();
		m_PSBRDFIntegrator->Use();

		D3D11_VIEWPORT Viewport{};
		Viewport.Width = static_cast<FLOAT>(m_IntegratedBRDFTextureDesc.Width);
		Viewport.Height = static_cast<FLOAT>(m_IntegratedBRDFTextureDesc.Height);
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		m_DeviceContext->RSSetViewports(1, &Viewport);

		m_Device->CreateRenderTargetView(m_IntegratedBRDFTextureRaw.Get(), nullptr, m_IntegratedBRDFRTV.ReleaseAndGetAddressOf());
		m_DeviceContext->ClearRenderTargetView(m_IntegratedBRDFRTV.Get(), Colors::Blue);

		m_DeviceContext->OMSetRenderTargets(1, m_IntegratedBRDFRTV.GetAddressOf(), nullptr);

		// Draw!
		{
			m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_DeviceContext->IASetVertexBuffers(0, 1, m_ScreenQuadVertexBuffer.GetAddressOf(), &m_ScreenQuadVertexBufferStride, &m_ScreenQuadVertexBufferOffset);
			m_DeviceContext->Draw(6, 0);
		}

		SetForwardRenderTargets();
	}

	m_IntegratedBRDFTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_IntegratedBRDFTexture->CopyTexture(m_IntegratedBRDFTextureRaw.Get());
	m_IntegratedBRDFTexture->SetSlot(KIntegratedBRDFTextureSlot);
}

void CGame::EndRendering()
{
	if (m_IsDestroyed) return;

	// Edge detection
	if (m_eMode == EMode::Edit)
	{
		m_bIsDeferredRenderTargetsSet = false;

		m_DeviceContext->OMSetRenderTargets(1, m_ScreenQuadRTV.GetAddressOf(), m_DepthStencilDSV.Get());
		m_DeviceContext->ClearRenderTargetView(m_ScreenQuadRTV.Get(), Colors::Transparent);
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (m_vSelectionData.size())
		{
			for (const auto& RegionSelection : m_vSelectionData)
			{
				if (RegionSelection.eObjectType == EObjectType::Object3D || RegionSelection.eObjectType == EObjectType::Object3DInstance)
				{
					CObject3D* const Object3D{ (CObject3D*)RegionSelection.PtrObject };

					if (RegionSelection.eObjectType == EObjectType::Object3DInstance)
					{
						Object3D->ComponentTransform.MatrixWorld = Object3D->GetInstanceGPUData(RegionSelection.Name).WorldMatrix;
					}
					
					DrawObject3D(Object3D, true);
				}
			}
		}
		else
		{
			// @TODO
			// This is very slow....
			if (IsAnythingSelected())
			{
				for (const auto& SelectionData : m_vSelectionData)
				{
					if (SelectionData.eObjectType == EObjectType::Object3D || SelectionData.eObjectType == EObjectType::Object3DInstance)
					{
						CObject3D* const Object3D{ (CObject3D*)SelectionData.PtrObject };
						if (SelectionData.eObjectType == EObjectType::Object3DInstance)
						{
							const auto& InstanceGPUData{ Object3D->GetInstanceGPUData(SelectionData.Name) };
							Object3D->ComponentTransform.MatrixWorld = InstanceGPUData.WorldMatrix;
							DrawObject3D(Object3D, true);
						}
						else
						{
							size_t InstanceCount{ Object3D->GetInstanceCount() };
							if (InstanceCount)
							{
								for (const auto& InstancePair : Object3D->GetInstanceNameToIndexMap())
								{
									Object3D->ComponentTransform.MatrixWorld = Object3D->GetInstanceGPUData(InstancePair.first).WorldMatrix;
									DrawObject3D(Object3D);
								}
							}
							else
							{
								Object3D->UpdateWorldMatrix();
								DrawObject3D(Object3D);
							}
						}
					}
				}
			}
		}

		SetForwardRenderTargets();

		DrawFullScreenQuad(m_PSEdgeDetector.get(), m_ScreenQuadSRV.GetAddressOf(), 1);
	}

	if (m_eMode != EMode::Play)
	{
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
	m_DeviceContext->OMSetRenderTargets(1, m_BackBufferRTV.GetAddressOf(), m_DepthStencilDSV.Get());

	if (bClearViews)
	{
		m_DeviceContext->ClearRenderTargetView(m_BackBufferRTV.Get(), Colors::CornflowerBlue);
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	m_bIsDeferredRenderTargetsSet = false;
}

void CGame::SetDeferredRenderTargets(bool bClearViews)
{
	ID3D11RenderTargetView* RTVs[]{ m_BaseColorRoughRTV.Get(), m_NormalRTV.Get(), m_MetalAORTV.Get() };

	m_DeviceContext->OMSetRenderTargets(ARRAYSIZE(RTVs), RTVs, m_DepthStencilDSV.Get());

	if (bClearViews)
	{
		for (auto& RTV : RTVs)
		{
			m_DeviceContext->ClearRenderTargetView(RTV, Colors::Transparent); // @important
		}
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	m_bIsDeferredRenderTargetsSet = true;
}

void CGame::DrawFullScreenQuad(CShader* const PixelShader, ID3D11ShaderResourceView** const SRVs, UINT NumSRVs)
{
	m_DeviceContext->RSSetState(m_CommonStates->CullNone());

	{
		m_VSScreenQuad->Use();
		PixelShader->Use();

		ID3D11SamplerState* const Samplers[]{ m_CommonStates->PointClamp(), m_CommonStates->LinearWrap(), m_CommonStates->LinearClamp() };
		m_DeviceContext->PSSetSamplers(0, ARRAYSIZE(Samplers), Samplers);
		m_DeviceContext->PSSetShaderResources(0, NumSRVs, SRVs);

		// Draw full-screen quad vertices
		m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DeviceContext->IASetVertexBuffers(0, 1, m_ScreenQuadVertexBuffer.GetAddressOf(), &m_ScreenQuadVertexBufferStride, &m_ScreenQuadVertexBufferOffset);
		m_DeviceContext->Draw(6, 0);
	}

	SetUniversalRSState();
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

const XMFLOAT2& CGame::GetWindowSize() const
{
	return m_WindowSize;
}