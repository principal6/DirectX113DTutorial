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

	{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT	, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 1, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "INSTANCEWORLD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCEWORLD"	, 1, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCEWORLD"	, 2, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCEWORLD"	, 3, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
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
	CreateEditorCamera();

	if (!m_EnvironmentRep) m_EnvironmentRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get(), this);
	if (!m_IrradianceRep) m_IrradianceRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get(), this);
	if (!m_PrefilteredRadianceRep) m_PrefilteredRadianceRep = make_unique<CCubemapRep>(m_Device.Get(), m_DeviceContext.Get(), this);
	
	if (!m_Object3D_CameraRepresentation)
	{
		m_Object3D_CameraRepresentation = make_unique<CObject3D>("CameraRepresentation", m_Device.Get(), m_DeviceContext.Get(), this);
		m_Object3D_CameraRepresentation->CreateFromFile("Asset\\camera_repr.fbx", false);
		m_Object3D_CameraRepresentation->ComponentRender.PtrVS = GetBaseShader(EBaseShader::VSBase);
		m_Object3D_CameraRepresentation->ComponentRender.PtrPS = GetBaseShader(EBaseShader::PSCamera);
	}

	if (InsertObject3DLine("Default3DAxes", false))
	{
		CObject3DLine* Grid{ GetObject3DLine("Default3DAxes") };
		Grid->Create(Generate3DGrid(0));
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
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\cobblestone_large_diffuse.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::NormalTexture, "Asset\\cobblestone_large_normal.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::RoughnessTexture, "Asset\\cobblestone_large_roughness.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::AmbientOcclusionTexture, "Asset\\cobblestone_large_occlusion.jpg");
		MaterialData.SetTextureFileName(STextureData::EType::DisplacementTexture, "Asset\\cobblestone_large_displacement.jpg");

		InsertMaterialCreateTextures(MaterialData);
	}

	{
		CMaterialData MaterialData{ "burned_ground" };
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\burned_ground_diffuse.jpg");
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
		MaterialData.SetTextureFileName(STextureData::EType::DiffuseTexture, "Asset\\brown_mud_dry_diffuse.jpg");
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
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // LDR
	//SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // HDR
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
		&SwapChainDesc, &m_SwapChain, &m_Device, nullptr, &m_DeviceContext);
}

void CGame::CreateViews()
{
	// Create back buffer RTV
	ComPtr<ID3D11Texture2D> BackBuffer{};
	m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &BackBuffer);
	m_Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &m_DeviceRTV);

	// Create deferred RTV
	{
		D3D11_TEXTURE2D_DESC Texture2DDesc{};
		Texture2DDesc.ArraySize = 1;
		Texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		Texture2DDesc.CPUAccessFlags = 0;
		Texture2DDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // LDR
		//Texture2DDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // HDR
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

	// Create depth-stencil view
	D3D11_TEXTURE2D_DESC DepthStencilBufferDesc{};
	DepthStencilBufferDesc.ArraySize = 1;
	DepthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilBufferDesc.CPUAccessFlags = 0;
	DepthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilBufferDesc.Width = static_cast<UINT>(m_WindowSize.x);
	DepthStencilBufferDesc.Height = static_cast<UINT>(m_WindowSize.y);
	DepthStencilBufferDesc.MipLevels = 0;
	DepthStencilBufferDesc.MiscFlags = 0;
	DepthStencilBufferDesc.SampleDesc.Count = 1;
	DepthStencilBufferDesc.SampleDesc.Quality = 0;
	DepthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	m_Device->CreateTexture2D(&DepthStencilBufferDesc, nullptr, &m_DepthStencilBuffer);
	m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, &m_DepthStencilView);
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
	D3D11_BLEND_DESC BlendDesc{};
	BlendDesc.AlphaToCoverageEnable = TRUE;
	BlendDesc.IndependentBlendEnable = FALSE;
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendDesc.RenderTarget[0].BlendOp;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_Device->CreateBlendState(&BlendDesc, m_BlendAlphaToCoverage.ReleaseAndGetAddressOf());
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
	m_CBCameraSelection = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBCameraSelectionData, sizeof(m_CBCameraSelectionData));
	m_CBScreen = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBScreenData, sizeof(m_CBScreenData));
	m_CBRadiancePrefiltering = make_unique<CConstantBuffer>(m_Device.Get(), m_DeviceContext.Get(),
		&m_CBRadiancePrefilteringData, sizeof(m_CBRadiancePrefilteringData));

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
	m_CBMaterial->Create();
	m_CBPSFlags->Create();
	m_CBGizmoColorFactor->Create();
	m_CBPS2DFlags->Create();
	m_CBTerrainMaskingSpace->Create();
	m_CBTerrainSelection->Create();
	m_CBSkyTime->Create();
	m_CBWaterTime->Create();
	m_CBEditorTime->Create();
	m_CBCameraSelection->Create();
	m_CBScreen->Create();
	m_CBRadiancePrefiltering->Create();
}

void CGame::CreateBaseShaders()
{
	m_VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase->Create(EShaderType::VertexShader, L"Shader\\VSBase.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSBase->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSInstance = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSInstance->Create(EShaderType::VertexShader, L"Shader\\VSInstance.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSInstance->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSAnimation->Create(EShaderType::VertexShader, L"Shader\\VSAnimation.hlsl", "main", 
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSAnimation->AttachConstantBuffer(m_CBSpaceWVP.get());
	m_VSAnimation->AttachConstantBuffer(m_CBAnimationBones.get());
	m_VSAnimation->AttachConstantBuffer(m_CBAnimation.get());

	m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSSky->Create(EShaderType::VertexShader, L"Shader\\VSSky.hlsl", "main", 
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSSky->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSLine->Create(EShaderType::VertexShader, L"Shader\\VSLine.hlsl", "main", 
		CObject3DLine::KInputElementDescs, ARRAYSIZE(CObject3DLine::KInputElementDescs));
	m_VSLine->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSGizmo->Create(EShaderType::VertexShader, L"Shader\\VSGizmo.hlsl", "main", 
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSGizmo->AttachConstantBuffer(m_CBSpaceWVP.get());

	m_VSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSTerrain->Create(EShaderType::VertexShader, L"Shader\\VSTerrain.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSTerrain->AttachConstantBuffer(m_CBSpaceWVP.get());
	m_VSTerrain->AttachConstantBuffer(m_CBTerrain.get());

	m_VSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSFoliage->Create(EShaderType::VertexShader, L"Shader\\VSFoliage.hlsl", "main",
		KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSFoliage->AttachConstantBuffer(m_CBSpaceWVP.get());
	m_VSFoliage->AttachConstantBuffer(m_CBTerrain.get());
	m_VSFoliage->AttachConstantBuffer(m_CBWind.get());

	m_VSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSParticle->Create(EShaderType::VertexShader, L"Shader\\VSParticle.hlsl", "main",
		CParticlePool::KInputElementDescs, ARRAYSIZE(CParticlePool::KInputElementDescs));

	m_VSScreenQuad = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	//m_VSScreenQuad->Create(EShaderType::VertexShader, L"Shader\\VSScreenQuad.hlsl", "main");
	m_VSScreenQuad->Create(EShaderType::VertexShader, L"Shader\\VSScreenQuad.hlsl", "main", 
		KScreenQuadInputElementDescs, ARRAYSIZE(KScreenQuadInputElementDescs));

	m_VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase2D->Create(EShaderType::VertexShader, L"Shader\\VSBase2D.hlsl", "main",
		CObject2D::KInputLayout, ARRAYSIZE(CObject2D::KInputLayout));
	m_VSBase2D->AttachConstantBuffer(m_CBSpace2D.get());

	m_HSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSTerrain->Create(EShaderType::HullShader, L"Shader\\HSTerrain.hlsl", "main");
	m_HSTerrain->AttachConstantBuffer(m_CBLight.get());
	m_HSTerrain->AttachConstantBuffer(m_CBTessFactor.get());

	m_HSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSWater->Create(EShaderType::HullShader, L"Shader\\HSWater.hlsl", "main");
	m_HSWater->AttachConstantBuffer(m_CBLight.get());
	m_HSWater->AttachConstantBuffer(m_CBTessFactor.get());

	m_HSStatic = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSStatic->Create(EShaderType::HullShader, L"Shader\\HSStatic.hlsl", "main");
	m_HSStatic->AttachConstantBuffer(m_CBTessFactor.get());

	m_DSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSTerrain->Create(EShaderType::DomainShader, L"Shader\\DSTerrain.hlsl", "main");
	m_DSTerrain->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSTerrain->AttachConstantBuffer(m_CBDisplacement.get());

	m_DSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSWater->Create(EShaderType::DomainShader, L"Shader\\DSWater.hlsl", "main");
	m_DSWater->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSWater->AttachConstantBuffer(m_CBWaterTime.get());

	m_DSStatic = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSStatic->Create(EShaderType::DomainShader, L"Shader\\DSStatic.hlsl", "main");
	m_DSStatic->AttachConstantBuffer(m_CBSpaceVP.get());
	m_DSStatic->AttachConstantBuffer(m_CBDisplacement.get());

	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormal.hlsl", "main");
	m_GSNormal->AttachConstantBuffer(m_CBSpaceVP.get());

	m_GSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSParticle->Create(EShaderType::GeometryShader, L"Shader\\GSParticle.hlsl", "main");
	m_GSParticle->AttachConstantBuffer(m_CBSpaceVP.get());

	m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase->Create(EShaderType::PixelShader, L"Shader\\PSBase.hlsl", "main");
	m_PSBase->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSBase->AttachConstantBuffer(m_CBLight.get());
	m_PSBase->AttachConstantBuffer(m_CBMaterial.get());

	m_PSVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSVertexColor->Create(EShaderType::PixelShader, L"Shader\\PSVertexColor.hlsl", "main");

	m_PSDynamicSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSDynamicSky->Create(EShaderType::PixelShader, L"Shader\\PSDynamicSky.hlsl", "main");
	m_PSDynamicSky->AttachConstantBuffer(m_CBSkyTime.get());

	m_PSCloud = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCloud->Create(EShaderType::PixelShader, L"Shader\\PSCloud.hlsl", "main");
	m_PSCloud->AttachConstantBuffer(m_CBSkyTime.get());

	m_PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSLine->Create(EShaderType::PixelShader, L"Shader\\PSLine.hlsl", "main");

	m_PSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSGizmo->Create(EShaderType::PixelShader, L"Shader\\PSGizmo.hlsl", "main");
	m_PSGizmo->AttachConstantBuffer(m_CBGizmoColorFactor.get());

	m_PSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSTerrain->Create(EShaderType::PixelShader, L"Shader\\PSTerrain.hlsl", "main");
	m_PSTerrain->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSTerrain->AttachConstantBuffer(m_CBTerrainMaskingSpace.get());
	m_PSTerrain->AttachConstantBuffer(m_CBLight.get());
	m_PSTerrain->AttachConstantBuffer(m_CBTerrainSelection.get());
	m_PSTerrain->AttachConstantBuffer(m_CBEditorTime.get());
	m_PSTerrain->AttachConstantBuffer(m_CBMaterial.get());

	m_PSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSWater->Create(EShaderType::PixelShader, L"Shader\\PSWater.hlsl", "main");
	m_PSWater->AttachConstantBuffer(m_CBWaterTime.get());
	m_PSWater->AttachConstantBuffer(m_CBLight.get());

	m_PSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSFoliage->Create(EShaderType::PixelShader, L"Shader\\PSFoliage.hlsl", "main");
	m_PSFoliage->AttachConstantBuffer(m_CBPSFlags.get());
	m_PSFoliage->AttachConstantBuffer(m_CBLight.get());
	m_PSFoliage->AttachConstantBuffer(m_CBMaterial.get());

	m_PSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSParticle->Create(EShaderType::PixelShader, L"Shader\\PSParticle.hlsl", "main");

	m_PSCamera = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCamera->Create(EShaderType::PixelShader, L"Shader\\PSCamera.hlsl", "main");
	m_PSCamera->AttachConstantBuffer(m_CBMaterial.get());
	m_PSCamera->AttachConstantBuffer(m_CBEditorTime.get());
	m_PSCamera->AttachConstantBuffer(m_CBCameraSelection.get());

	m_PSScreenQuad = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSScreenQuad->Create(EShaderType::PixelShader, L"Shader\\PSScreenQuad.hlsl", "main");

	m_CBScreenData.InverseScreenSize = XMFLOAT2(1.0f / m_WindowSize.x, 1.0f / m_WindowSize.y);
	m_CBScreen->Update();

	m_PSEdgeDetector = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSEdgeDetector->Create(EShaderType::PixelShader, L"Shader\\PSEdgeDetector.hlsl", "main");
	m_PSEdgeDetector->AttachConstantBuffer(m_CBScreen.get());

	m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSSky->Create(EShaderType::PixelShader, L"Shader\\PSSky.hlsl", "main");

	m_PSIrradianceGenerator = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSIrradianceGenerator->Create(EShaderType::PixelShader, L"Shader\\PSIrradianceGenerator.hlsl", "main");

	m_PSFromHDR = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSFromHDR->Create(EShaderType::PixelShader, L"Shader\\PSFromHDR.hlsl", "main");

	m_PSRadiancePrefiltering = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSRadiancePrefiltering->Create(EShaderType::PixelShader, L"Shader\\PSRadiancePrefiltering.hlsl", "main");
	m_PSRadiancePrefiltering->AttachConstantBuffer(m_CBRadiancePrefiltering.get());

	m_PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase2D->Create(EShaderType::PixelShader, L"Shader\\PSBase2D.hlsl", "main");
	m_PSBase2D->AttachConstantBuffer(m_CBPS2DFlags.get());

	m_PSMasking2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSMasking2D->Create(EShaderType::PixelShader, L"Shader\\PSMasking2D.hlsl", "main");

	m_PSHeightMap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSHeightMap2D->Create(EShaderType::PixelShader, L"Shader\\PSHeightMap2D.hlsl", "main");
	
	m_PSCubemap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCubemap2D->Create(EShaderType::PixelShader, L"Shader\\PSCubemap2D.hlsl", "main");
}

void CGame::CreateMiniAxes()
{
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisX", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisY", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisZ", m_Device.Get(), m_DeviceContext.Get(), this));

	const SMesh KAxisCone{ GenerateCone(0, 1.0f, 1.0f, 16) };
	vector<CMaterialData> vMaterialData{};
	vMaterialData.resize(3);
	vMaterialData[0].SetUniformColor(XMFLOAT3(1, 0, 0));
	vMaterialData[1].SetUniformColor(XMFLOAT3(0, 1, 0));
	vMaterialData[2].SetUniformColor(XMFLOAT3(0, 0, 1));
	m_vObject3DMiniAxes[0]->Create(KAxisCone, vMaterialData[0]);
	m_vObject3DMiniAxes[0]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[0]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[0]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vObject3DMiniAxes[0]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[1]->Create(KAxisCone, vMaterialData[1]);
	m_vObject3DMiniAxes[1]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[1]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[1]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[2]->Create(KAxisCone, vMaterialData[2]);
	m_vObject3DMiniAxes[2]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[2]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[2]->ComponentTransform.Yaw = -XM_PIDIV2;
	m_vObject3DMiniAxes[2]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vObject3DMiniAxes[2]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[0]->ComponentTransform.Scaling =
		m_vObject3DMiniAxes[1]->ComponentTransform.Scaling =
		m_vObject3DMiniAxes[2]->ComponentTransform.Scaling = XMVectorSet(0.1f, 0.8f, 0.1f, 0);
}

void CGame::CreatePickingRay()
{
	m_Object3DLinePickingRay = make_unique<CObject3DLine>("PickingRay", m_Device.Get(), m_DeviceContext.Get());

	vector<SVertex3DLine> Vertices{};
	Vertices.emplace_back(XMVectorSet(0, 0, 0, 1), XMVectorSet(1, 0, 0, 1));
	Vertices.emplace_back(XMVectorSet(10.0f, 10.0f, 0, 1), XMVectorSet(0, 1, 0, 1));

	m_Object3DLinePickingRay->Create(Vertices);
}

void CGame::CreatePickedTriangle()
{
	m_Object3DPickedTriangle = make_unique<CObject3D>("PickedTriangle", m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DPickedTriangle->Create(GenerateTriangle(XMVectorSet(0, 0, 1.5f, 1), XMVectorSet(+1.0f, 0, 0, 1), XMVectorSet(-1.0f, 0, 0, 1),
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f)));
}

void CGame::CreateBoundingSphere()
{
	m_Object3DBoundingSphere = make_unique<CObject3D>("BoundingSphere", m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DBoundingSphere->Create(GenerateSphere(16));
}

void CGame::Create3DGizmos()
{
	const static XMVECTOR ColorX{ XMVectorSet(1.00f, 0.25f, 0.25f, 1) };
	const static XMVECTOR ColorY{ XMVectorSet(0.25f, 1.00f, 0.25f, 1) };
	const static XMVECTOR ColorZ{ XMVectorSet(0.25f, 0.25f, 1.00f, 1) };

	if (!m_Object3D_3DGizmoRotationPitch)
	{
		m_Object3D_3DGizmoRotationPitch = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, ColorX) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorX) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoRotationPitch->Create(MergeStaticMeshes(MeshRing, MeshAxis));
		m_Object3D_3DGizmoRotationPitch->ComponentTransform.Roll = -XM_PIDIV2;
	}
	
	if (!m_Object3D_3DGizmoRotationYaw)
	{
		m_Object3D_3DGizmoRotationYaw = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, ColorY) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorY) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoRotationYaw->Create(MergeStaticMeshes(MeshRing, MeshAxis));
	}

	if (!m_Object3D_3DGizmoRotationRoll)
	{
		m_Object3D_3DGizmoRotationRoll = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, ColorZ) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorZ) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoRotationRoll->Create(MergeStaticMeshes(MeshRing, MeshAxis));
		m_Object3D_3DGizmoRotationRoll->ComponentTransform.Pitch = XM_PIDIV2;
	}
	
	if (!m_Object3D_3DGizmoTranslationX)
	{
		m_Object3D_3DGizmoTranslationX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorX) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorX) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationX->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationX->ComponentTransform.Roll = -XM_PIDIV2;
	}
	
	if (!m_Object3D_3DGizmoTranslationY)
	{
		m_Object3D_3DGizmoTranslationY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorY) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorY) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationY->Create(MeshAxis);
	}
	
	if (!m_Object3D_3DGizmoTranslationZ)
	{
		m_Object3D_3DGizmoTranslationZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorZ) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorZ) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationZ->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationZ->ComponentTransform.Pitch = XM_PIDIV2;
	}

	if (!m_Object3D_3DGizmoScalingX)
	{
		m_Object3D_3DGizmoScalingX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorX) };
		SMesh MeshCube{ GenerateCube(ColorX) };
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
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorY) };
		SMesh MeshCube{ GenerateCube(ColorY) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingY->Create(MeshAxis);
	}

	if (!m_Object3D_3DGizmoScalingZ)
	{
		m_Object3D_3DGizmoScalingZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, ColorZ) };
		SMesh MeshCube{ GenerateCube(ColorZ) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingZ->Create(MeshAxis);
		m_Object3D_3DGizmoScalingZ->ComponentTransform.Pitch = XM_PIDIV2;
	}

	m_Object3D_3DGizmoRotationPitch->ComponentRender.PtrVS =
		m_Object3D_3DGizmoRotationYaw->ComponentRender.PtrVS = m_Object3D_3DGizmoRotationRoll->ComponentRender.PtrVS =
		m_Object3D_3DGizmoTranslationX->ComponentRender.PtrVS =
		m_Object3D_3DGizmoTranslationY->ComponentRender.PtrVS = m_Object3D_3DGizmoTranslationZ->ComponentRender.PtrVS =
		m_Object3D_3DGizmoScalingX->ComponentRender.PtrVS =
		m_Object3D_3DGizmoScalingY->ComponentRender.PtrVS = m_Object3D_3DGizmoScalingZ->ComponentRender.PtrVS = m_VSGizmo.get();

	m_Object3D_3DGizmoRotationPitch->ComponentRender.PtrPS =
		m_Object3D_3DGizmoRotationYaw->ComponentRender.PtrPS = m_Object3D_3DGizmoRotationRoll->ComponentRender.PtrPS =
		m_Object3D_3DGizmoTranslationX->ComponentRender.PtrPS =
		m_Object3D_3DGizmoTranslationY->ComponentRender.PtrPS = m_Object3D_3DGizmoTranslationZ->ComponentRender.PtrPS =
		m_Object3D_3DGizmoScalingX->ComponentRender.PtrPS = 
		m_Object3D_3DGizmoScalingY->ComponentRender.PtrPS = m_Object3D_3DGizmoScalingZ->ComponentRender.PtrPS = m_PSGizmo.get();
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

void CGame::LoadScene(const string& FileName)
{
	using namespace tinyxml2;

	tinyxml2::XMLDocument xmlDocument{};
	xmlDocument.LoadFile(FileName.c_str());

	XMLElement* xmlScene{ xmlDocument.FirstChildElement() };
	vector<XMLElement*> vxmlSceneChildren{};
	XMLElement* xmlSceneChild{ xmlScene->FirstChildElement() };
	while (xmlSceneChild)
	{
		vxmlSceneChildren.emplace_back(xmlSceneChild);
		xmlSceneChild = xmlSceneChild->NextSiblingElement();
	}

	for (const auto& xmlSceneChild : vxmlSceneChildren)
	{
		const char* ID{ xmlSceneChild->Attribute("ID") };

		if (strcmp(ID, "ObjectList") == 0)
		{
			vector<XMLElement*> vxmlObjects{};
			XMLElement* xmlObject{ xmlSceneChild->FirstChildElement() };
			while (xmlObject)
			{
				vxmlObjects.emplace_back(xmlObject);
				xmlObject = xmlObject->NextSiblingElement();
			}

			ClearObject3Ds();
			m_vObject3Ds.reserve(vxmlObjects.size());
			vector<CObject3D*> vPtrObject3D{};
			vector<std::pair<string, bool>> vModelDatas{};

			for (const auto& xmlObject : vxmlObjects)
			{
				const char* ObjectName{ xmlObject->Attribute("Name") };

				InsertObject3D(ObjectName);
				vPtrObject3D.emplace_back(GetObject3D(ObjectName));
				CObject3D* Object3D{ GetObject3D(ObjectName) };

				vector<XMLElement*> vxmlObjectChildren{};
				XMLElement* xmlObjectChild{ xmlObject->FirstChildElement() };
				while (xmlObjectChild)
				{
					vxmlObjectChildren.emplace_back(xmlObjectChild);
					xmlObjectChild = xmlObjectChild->NextSiblingElement();
				}

				for (const auto& xmlObjectChild : vxmlObjectChildren)
				{
					const char* ID{ xmlObjectChild->Attribute("ID") };

					if (strcmp(ID, "Model") == 0)
					{
						const char* ModelFileName{ xmlObjectChild->Attribute("FileName") };
						bool bIsRigged{ xmlObjectChild->BoolAttribute("IsRigged") };
						vModelDatas.emplace_back(std::make_pair(ModelFileName, bIsRigged));
					}

					if (strcmp(ID, "Translation") == 0)
					{
						float x{ xmlObjectChild->FloatAttribute("x") };
						float y{ xmlObjectChild->FloatAttribute("y") };
						float z{ xmlObjectChild->FloatAttribute("z") };
						Object3D->ComponentTransform.Translation = XMVectorSet(x, y, z, 1.0f);
					}

					if (strcmp(ID, "Rotation") == 0)
					{
						float Pitch{ xmlObjectChild->FloatAttribute("Pitch") };
						float Yaw{ xmlObjectChild->FloatAttribute("Yaw") };
						float Roll{ xmlObjectChild->FloatAttribute("Roll") };
						Object3D->ComponentTransform.Pitch = Pitch;
						Object3D->ComponentTransform.Yaw = Yaw;
						Object3D->ComponentTransform.Roll = Roll;
					}

					if (strcmp(ID, "Scaling") == 0)
					{
						float x{ xmlObjectChild->FloatAttribute("x") };
						float y{ xmlObjectChild->FloatAttribute("y") };
						float z{ xmlObjectChild->FloatAttribute("z") };
						Object3D->ComponentTransform.Scaling = XMVectorSet(x, y, z, 1.0f);
					}

					if (strcmp(ID, "BSCenterOffset") == 0)
					{
						float x{ xmlObjectChild->FloatAttribute("x") };
						float y{ xmlObjectChild->FloatAttribute("y") };
						float z{ xmlObjectChild->FloatAttribute("z") };
						Object3D->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSet(x, y, z, 1.0f);
					}

					if (strcmp(ID, "BSRadius") == 0)
					{
						float Radius{ xmlObjectChild->FloatAttribute("Value") };
						Object3D->ComponentPhysics.BoundingSphere.Radius = Radius;
					}

					if (strcmp(ID, "BSRadiusBias") == 0)
					{
						float RadiusBias{ xmlObjectChild->FloatAttribute("Value") };
						Object3D->ComponentPhysics.BoundingSphere.RadiusBias = RadiusBias;
					}

					if (strcmp(ID, "InstanceList") == 0)
					{
						vector<XMLElement*> vxmlInstances{};
						XMLElement* xmlInstance{ xmlObjectChild->FirstChildElement() };
						while (xmlInstance)
						{
							vxmlInstances.emplace_back(xmlInstance);
							xmlInstance = xmlInstance->NextSiblingElement();
						}

						int iInstance{};
						for (auto& xmlInstance : vxmlInstances)
						{
							vector<XMLElement*> vxmlInstanceChildren{};
							XMLElement* xmlInstanceChild{ xmlInstance->FirstChildElement() };
							while (xmlInstanceChild)
							{
								vxmlInstanceChildren.emplace_back(xmlInstanceChild);
								xmlInstanceChild = xmlInstanceChild->NextSiblingElement();
							}

							Object3D->InsertInstance(false);
							auto& InstanceCPUData{ Object3D->GetInstanceCPUData(iInstance) };
							for (auto& xmlInstanceChild : vxmlInstanceChildren)
							{
								const char* ID{ xmlInstanceChild->Attribute("ID") };
								if (strcmp(ID, "Translation") == 0)
								{
									float x{ xmlInstanceChild->FloatAttribute("x") };
									float y{ xmlInstanceChild->FloatAttribute("y") };
									float z{ xmlInstanceChild->FloatAttribute("z") };
									InstanceCPUData.Translation = XMVectorSet(x, y, z, 1.0f);
								}

								if (strcmp(ID, "Rotation") == 0)
								{
									float Pitch{ xmlInstanceChild->FloatAttribute("Pitch") };
									float Yaw{ xmlInstanceChild->FloatAttribute("Yaw") };
									float Roll{ xmlInstanceChild->FloatAttribute("Roll") };
									InstanceCPUData.Pitch = Pitch;
									InstanceCPUData.Yaw = Yaw;
									InstanceCPUData.Roll = Roll;
								}

								if (strcmp(ID, "Scaling") == 0)
								{
									float x{ xmlInstanceChild->FloatAttribute("x") };
									float y{ xmlInstanceChild->FloatAttribute("y") };
									float z{ xmlInstanceChild->FloatAttribute("z") };
									InstanceCPUData.Scaling = XMVectorSet(x, y, z, 1.0f);
								}
							}
							Object3D->UpdateInstanceWorldMatrix(iInstance);

							++iInstance;
						}
					}
				}
			}

			// Create models using multiple threads
			{
				ULONGLONG StartTimePoint{ GetTickCount64() };
				OutputDebugString((to_string(StartTimePoint) + " Loading models.\n").c_str());
				vector<thread> vThreads{};
				for (size_t iObject3D = 0; iObject3D < vPtrObject3D.size(); ++iObject3D)
				{
					vThreads.emplace_back
					(
						[](CObject3D* const P, const string& ModelFileName, bool bIsRigged)
						{
							P->CreateFromFile(ModelFileName, bIsRigged);
							if (P->IsInstanced()) P->CreateInstanceBuffers();
						}
						, vPtrObject3D[iObject3D], vModelDatas[iObject3D].first, vModelDatas[iObject3D].second
							);
				}
				for (auto& Thread : vThreads)
				{
					Thread.join();
				}
				OutputDebugString((to_string(GetTickCount64()) + " All models are loaded. ["
					+ to_string(GetTickCount64() - StartTimePoint) + "] elapsed.\n").c_str());
			}
		}

		if (strcmp(ID, "Terrain") == 0)
		{
			const char* TerrainFileName{ xmlSceneChild->Attribute("FileName") };
			if (TerrainFileName) LoadTerrain(TerrainFileName);
		}

		if (strcmp(ID, "Sky") == 0)
		{
			const char* SkyFileName{ xmlSceneChild->Attribute("FileName") };
			float ScalingFactor{ xmlSceneChild->FloatAttribute("ScalingFactor") };
			if (SkyFileName) CreateDynamicSky(SkyFileName, ScalingFactor);
		}
	}
}

void CGame::SaveScene(const string& FileName)
{
	using namespace tinyxml2;

	if (m_Terrain)
	{
		if (m_Terrain->GetFileName().size() == 0)
		{
			MB_WARN("지형이 존재하지만 저장되지 않았습니다.\n먼저 지형을 저장해 주세요.", "장면 저장 실패");
			return;
		}
		else
		{
			m_Terrain->Save(m_Terrain->GetFileName());
		}
	}

	tinyxml2::XMLDocument xmlDocument{};
	XMLElement* xmlRoot{ xmlDocument.NewElement("Scene") };
	{
		XMLElement* xmlObjectList{ xmlDocument.NewElement("ObjectList") };
		xmlObjectList->SetAttribute("ID", "ObjectList");
		{
			for (const auto& Object3D : m_vObject3Ds)
			{
				XMLElement* xmlObject{ xmlDocument.NewElement("Object") };
				xmlObject->SetAttribute("Name", Object3D->GetName().c_str());

				XMLElement* xmlModelFileName{ xmlDocument.NewElement("Model") };
				{
					xmlModelFileName->SetAttribute("ID", "Model");
					xmlModelFileName->SetAttribute("FileName", Object3D->GetModelFileName().c_str());
					xmlModelFileName->SetAttribute("IsRigged", Object3D->IsRiggedModel());
					xmlObject->InsertEndChild(xmlModelFileName);
				}

				XMLElement* xmlTranslation{ xmlDocument.NewElement("Translation") };
				{
					xmlTranslation->SetAttribute("ID", "Translation");
					XMFLOAT4 Translation{};
					XMStoreFloat4(&Translation, Object3D->ComponentTransform.Translation);
					xmlTranslation->SetAttribute("x", Translation.x);
					xmlTranslation->SetAttribute("y", Translation.y);
					xmlTranslation->SetAttribute("z", Translation.z);
					xmlObject->InsertEndChild(xmlTranslation);
				}

				XMLElement* xmlRotation{ xmlDocument.NewElement("Rotation") };
				{
					xmlRotation->SetAttribute("ID", "Rotation");
					xmlRotation->SetAttribute("Pitch", Object3D->ComponentTransform.Pitch);
					xmlRotation->SetAttribute("Yaw", Object3D->ComponentTransform.Yaw);
					xmlRotation->SetAttribute("Roll", Object3D->ComponentTransform.Roll);
					xmlObject->InsertEndChild(xmlRotation);
				}

				XMLElement* xmlScaling{ xmlDocument.NewElement("Scaling") };
				{
					xmlScaling->SetAttribute("ID", "Scaling");
					XMFLOAT4 Scaling{};
					XMStoreFloat4(&Scaling, Object3D->ComponentTransform.Scaling);
					xmlScaling->SetAttribute("x", Scaling.x);
					xmlScaling->SetAttribute("y", Scaling.y);
					xmlScaling->SetAttribute("z", Scaling.z);
					xmlObject->InsertEndChild(xmlScaling);
				}

				XMLElement* xmlBSCenterOffset{ xmlDocument.NewElement("BSCenterOffset") };
				{
					xmlBSCenterOffset->SetAttribute("ID", "BSCenterOffset");
					XMFLOAT4 BSCenterOffset{};
					XMStoreFloat4(&BSCenterOffset, Object3D->ComponentPhysics.BoundingSphere.CenterOffset);
					xmlBSCenterOffset->SetAttribute("x", BSCenterOffset.x);
					xmlBSCenterOffset->SetAttribute("y", BSCenterOffset.y);
					xmlBSCenterOffset->SetAttribute("z", BSCenterOffset.z);
					xmlObject->InsertEndChild(xmlBSCenterOffset);
				}

				XMLElement* xmlBSRadius{ xmlDocument.NewElement("BSRadius") };
				{
					xmlBSRadius->SetAttribute("ID", "BSRadius");
					xmlBSRadius->SetAttribute("Value", Object3D->ComponentPhysics.BoundingSphere.Radius);
					xmlObject->InsertEndChild(xmlBSRadius);
				}

				XMLElement* xmlBSRadiusBias{ xmlDocument.NewElement("BSRadiusBias") };
				{
					xmlBSRadiusBias->SetAttribute("ID", "BSRadiusBias");
					xmlBSRadiusBias->SetAttribute("Value", Object3D->ComponentPhysics.BoundingSphere.RadiusBias);
					xmlObject->InsertEndChild(xmlBSRadiusBias);
				}

				if (Object3D->IsInstanced())
				{
					XMLElement* xmlInstanceList{ xmlDocument.NewElement("InstanceList") };
					{
						xmlInstanceList->SetAttribute("ID", "InstanceList");

						for (const auto& InstancePair : Object3D->GetInstanceMap())
						{
							XMLElement* xmlInstance{ xmlDocument.NewElement("Instance") };
							{
								xmlInstance->SetAttribute("Name", InstancePair.first.c_str());

								const auto& InstanceCPUData{ Object3D->GetInstanceCPUData(InstancePair.first) };

								XMLElement* xmlTranslation{ xmlDocument.NewElement("Translation") };
								{
									xmlTranslation->SetAttribute("ID", "Translation");
									XMFLOAT4 Translation{};
									XMStoreFloat4(&Translation, InstanceCPUData.Translation);
									xmlTranslation->SetAttribute("x", Translation.x);
									xmlTranslation->SetAttribute("y", Translation.y);
									xmlTranslation->SetAttribute("z", Translation.z);
									xmlInstance->InsertEndChild(xmlTranslation);
								}

								XMLElement* xmlRotation{ xmlDocument.NewElement("Rotation") };
								{
									xmlRotation->SetAttribute("ID", "Rotation");
									xmlRotation->SetAttribute("Pitch", InstanceCPUData.Pitch);
									xmlRotation->SetAttribute("Yaw", InstanceCPUData.Yaw);
									xmlRotation->SetAttribute("Roll", InstanceCPUData.Roll);
									xmlInstance->InsertEndChild(xmlRotation);
								}

								XMLElement* xmlScaling{ xmlDocument.NewElement("Scaling") };
								{
									xmlScaling->SetAttribute("ID", "Scaling");
									XMFLOAT4 Scaling{};
									XMStoreFloat4(&Scaling, InstanceCPUData.Scaling);
									xmlScaling->SetAttribute("x", Scaling.x);
									xmlScaling->SetAttribute("y", Scaling.y);
									xmlScaling->SetAttribute("z", Scaling.z);
									xmlInstance->InsertEndChild(xmlScaling);
								}

								xmlInstanceList->InsertEndChild(xmlInstance);
							}
						}

						xmlObject->InsertEndChild(xmlInstanceList);
					}
				}

				xmlObjectList->InsertEndChild(xmlObject);
			}
			xmlRoot->InsertEndChild(xmlObjectList);
		}

		XMLElement* xmlTerrain{ xmlDocument.NewElement("Terrain") };
		xmlTerrain->SetAttribute("ID", "Terrain");
		{
			if (m_Terrain)
			{
				xmlTerrain->SetAttribute("FileName", m_Terrain->GetFileName().c_str());
			}

			xmlRoot->InsertEndChild(xmlTerrain);
		}

		XMLElement* xmlSky{ xmlDocument.NewElement("Sky") };
		xmlSky->SetAttribute("ID", "Sky");
		{
			if (m_SkyData.bIsDataSet)
			{
				xmlSky->SetAttribute("FileName", m_SkyFileName.c_str());
				xmlSky->SetAttribute("ScalingFactor", m_SkyScalingFactor);
			}

			xmlRoot->InsertEndChild(xmlSky);
		}
	}
	xmlDocument.InsertEndChild(xmlRoot);

	xmlDocument.SaveFile(FileName.c_str());
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
}

void CGame::UpdateCBSpace(const XMMATRIX& World)
{
	m_CBSpaceWVPData.World = m_CBSpace2DData.World = XMMatrixTranspose(World);
	m_CBSpaceWVPData.ViewProjection = GetTransposedViewProjectionMatrix();
	m_CBSpaceWVP->Update();

	m_CBSpaceVPData.ViewProjection = GetTransposedViewProjectionMatrix();
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

void CGame::UpdateCBMaterialData(const CMaterialData& MaterialData)
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
	m_Object3DSkySphere->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSkySphere->ComponentRender.PtrPS = m_PSDynamicSky.get();
	m_Object3DSkySphere->ComponentPhysics.bIsPickable = false;
	m_Object3DSkySphere->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;

	m_Object3DSun = make_unique<CObject3D>("Sun", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSun->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterialData);
	m_Object3DSun->UpdateQuadUV(m_SkyData.Sun.UVOffset, m_SkyData.Sun.UVSize);
	m_Object3DSun->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Sun.WidthHeightRatio, 0);
	m_Object3DSun->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSun->ComponentRender.PtrPS = m_PSBase.get();
	m_Object3DSun->ComponentRender.bIsTransparent = true;
	m_Object3DSun->ComponentPhysics.bIsPickable = false;
	m_Object3DSun->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	
	m_Object3DMoon = make_unique<CObject3D>("Moon", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DMoon->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterialData);
	m_Object3DMoon->UpdateQuadUV(m_SkyData.Moon.UVOffset, m_SkyData.Moon.UVSize);
	m_Object3DMoon->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Moon.WidthHeightRatio, 0);
	m_Object3DMoon->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DMoon->ComponentRender.PtrPS = m_PSBase.get();
	m_Object3DMoon->ComponentRender.bIsTransparent = true;
	m_Object3DMoon->ComponentPhysics.bIsPickable = false;
	m_Object3DMoon->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	
	/*
	SModel CloudModel{};
	CloudModel.vMeshes.emplace_back(GenerateSphere(64));
	CloudModel.vMaterials.resize(1);
	CloudModel.vMaterials[0].SetDiffuseTextureFileName("Asset\\earth_clouds.png");

	m_Object3DCloud = make_unique<CObject3D>("Cloud", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DCloud->Create(CloudModel);
	m_Object3DCloud->ComponentTransform.Scaling = XMVectorSet(KSkyDistance * 2, KSkyDistance * 4, KSkyDistance * 2, 0);
	m_Object3DCloud->ComponentTransform.Roll = -XM_PIDIV2;
	m_Object3DCloud->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DCloud->ComponentRender.PtrPS = m_PSCloud.get();
	m_Object3DCloud->ComponentRender.bIsTransparent = true;
	m_Object3DCloud->ComponentPhysics.bIsPickable = false;
	m_Object3DCloud->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	*/

	m_SkyData.bIsDataSet = true;
	m_SkyData.bIsDynamic = true;

	return;
}

void CGame::CreateStaticSky(float ScalingFactor)
{
	m_SkyScalingFactor = ScalingFactor;

	CMaterialData Material{};
	
	m_EnvironmentTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	// @important: use already mipmapped cubemap texture
	//m_EnvironmentTexture->CreateCubeMapFromFile("Asset\\noon_grass_environment.dds");
	m_EnvironmentTexture->CreateCubeMapFromFile("Asset\\autumn_forest_environment.dds");
	//m_EnvironmentTexture->CreateCubeMapFromFile("Asset\\test_environment.dds");
	//m_EnvironmentTexture->CreateCubeMapFromFile("Asset\\uffizi_environment.dds");
	m_EnvironmentTexture->SetSlot(KEnvironmentTextureSlot);

	m_IrradianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	// @important: use already mipmapped cubemap texture
	//m_IrradianceTexture->CreateCubeMapFromFile("Asset\\noon_grass_irradiance.dds");
	m_IrradianceTexture->CreateCubeMapFromFile("Asset\\autumn_forest_irradiance.dds");
	//m_IrradianceTexture->CreateCubeMapFromFile("Asset\\test_irradiance.dds");
	//m_IrradianceTexture->CreateCubeMapFromFile("Asset\\uffizi_irradiance.dds");
	m_IrradianceTexture->SetSlot(KIrradianceTextureSlot);

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get(), this);
	//m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom));
	m_Object3DSkySphere->Create(GenerateCubemapSphere(KSkySphereSegmentCount));
	m_Object3DSkySphere->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
	m_Object3DSkySphere->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSkySphere->ComponentRender.PtrPS = m_PSSky.get();
	m_Object3DSkySphere->ComponentPhysics.bIsPickable = false;
	m_Object3DSkySphere->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;

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

bool CGame::InsertCamera(const string& Name)
{
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
	m_mapCameraNameToIndex[Name] = m_vCameras.size() - 1;

	return true;
}

void CGame::DeleteCamera(const std::string& Name)
{
	if (Name == m_vCameras[0]->GetName()) return; // @important
	if (Name.length() == 0) return;
	if (m_mapCameraNameToIndex.find(Name) == m_mapCameraNameToIndex.end())
	{
		MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Camera 삭제 실패");
		return;
	}

	size_t iCamera{ m_mapCameraNameToIndex[Name] };
	if (iCamera < m_vCameras.size() - 1)
	{
		const string& SwappedName{ m_vCameras.back()->GetName() };
		swap(m_vCameras[iCamera], m_vCameras.back());
		
		m_mapCameraNameToIndex[SwappedName] = iCamera;
	}

	if (IsAnyCameraSelected())
	{
		if (Name == GetSelectedCameraName())
		{
			DeselectCamera();
		}
	}

	m_vCameras.pop_back();
	m_mapCameraNameToIndex.erase(Name);

	m_PtrCurrentCamera = GetEditorCamera();
}

void CGame::ClearCameras()
{
	CCamera* EditorCamera{ GetEditorCamera() };
	CCamera::SCameraData EditorCameraData{ EditorCamera->GetData() };

	m_mapCameraNameToIndex.clear();
	m_vCameras.clear();

	CreateEditorCamera();
}

CCamera* CGame::GetCamera(const string& Name, bool bShowWarning)
{
	if (m_mapCameraNameToIndex.find(Name) == m_mapCameraNameToIndex.end())
	{
		if (bShowWarning) MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Camera 얻어오기 실패");
		return nullptr;
	}
	return m_vCameras[m_mapCameraNameToIndex.at(Name)].get();
}

void CGame::CreateEditorCamera()
{
	if (GetEditorCamera(false)) return;

	assert(InsertCamera(u8"Editor Camera"));
	CCamera* EditorCamera{ GetEditorCamera() };
	EditorCamera->SetData(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0)));
	EditorCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));

	m_PtrCurrentCamera = EditorCamera;
}

CCamera* CGame::GetEditorCamera(bool bShowWarning)
{
	return GetCamera(u8"Editor Camera", bShowWarning);
}

CShader* CGame::AddCustomShader()
{
	m_vShaders.emplace_back(make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vShaders.back().get();
}

CShader* CGame::GetCustomShader(size_t Index) const
{
	assert(Index < m_vShaders.size());
	return m_vShaders[Index].get();
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
	case EBaseShader::HSTerrain:
		Result = m_HSTerrain.get();
		break;
	case EBaseShader::HSWater:
		Result = m_HSWater.get();
		break;
	case EBaseShader::HSStatic:
		Result = m_HSStatic.get();
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

bool CGame::InsertObject3D(const string& Name)
{
	if (m_mapObject3DNameToIndex.find(Name) != m_mapObject3DNameToIndex.end())
	{
		MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Object3D 생성 실패");
		return false;
	}

	if (Name.size() >= KAssetNameMaxLength)
	{
		MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Object3D 생성 실패");
		return false;
	}
	else if (Name.size() == 0)
	{
		MB_WARN("이름은 공백일 수 없습니다.", "Object3D 생성 실패");
		return false;
	}

	m_vObject3Ds.emplace_back(make_unique<CObject3D>(Name, m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3Ds.back()->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3Ds.back()->ComponentRender.PtrPS = m_PSBase.get();

	m_mapObject3DNameToIndex[Name] = m_vObject3Ds.size() - 1;

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

	size_t iObject3D{ m_mapObject3DNameToIndex[Name] };
	if (iObject3D < m_vObject3Ds.size() - 1)
	{
		const string& SwappedName{ m_vObject3Ds.back()->GetName() };
		swap(m_vObject3Ds[iObject3D], m_vObject3Ds.back());

		m_mapObject3DNameToIndex[SwappedName] = iObject3D;
	}

	if (IsAnyObject3DSelected())
	{
		if (Name == GetSelectedObject3DName())
		{
			DeselectObject3D();
		}
	}

	m_mapObject3DNameToIndex.erase(Name);
	m_vObject3Ds.pop_back();
}

void CGame::ClearObject3Ds()
{
	m_mapObject3DNameToIndex.clear();
	m_vObject3Ds.clear();

	m_PtrSelectedObject3D = nullptr;
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

	if (IsAnyObject2DSelected())
	{
		if (Name == GetSelectedObject2DName())
		{
			DeselectObject2D();
		}
	}

	m_vObject2Ds.pop_back();
	m_mapObject2DNameToIndex.erase(Name);
}

void CGame::ClearObject2Ds()
{
	m_mapObject2DNameToIndex.clear();
	m_vObject2Ds.clear();
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
	m_bLeftButtonPressedOnce = false;
}

bool CGame::Pick()
{
	if (m_eEditMode == EEditMode::EditTerrain) return false;

	CastPickingRay();

	UpdatePickingRay();

	PickBoundingSphere();

	PickTriangle();

	if (m_PtrPickedObject3D) return true;
	return false;
}

const string& CGame::GetPickedObject3DName() const
{
	assert(m_PtrPickedObject3D);
	return m_PtrPickedObject3D->GetName();
}

int CGame::GetPickedInstanceID() const
{
	return m_PickedInstanceID;
}

void CGame::SelectObject3D(const string& Name)
{
	if (m_eEditMode != EEditMode::EditObject) return;

	m_PtrSelectedObject3D = GetObject3D(Name);
	if (m_PtrSelectedObject3D)
	{
		const XMVECTOR& BSTransaltion{ m_PtrSelectedObject3D->ComponentPhysics.BoundingSphere.CenterOffset };
		const XMVECTOR KObjectTranslation{ m_PtrSelectedObject3D->ComponentTransform.Translation + BSTransaltion };
		m_CapturedGizmoTranslation = KObjectTranslation;

		if (!m_PtrSelectedObject3D->IsInstanced())
		{
			DeselectInstance();
			Select3DGizmos();
		}
	}
}

void CGame::DeselectObject3D()
{
	m_PtrSelectedObject3D = nullptr;
	m_SelectedInstanceID = -1;
}

bool CGame::IsAnyObject3DSelected() const
{
	return (m_PtrSelectedObject3D) ? true : false;
}

CObject3D* CGame::GetSelectedObject3D()
{
	return m_PtrSelectedObject3D;
}

const string& CGame::GetSelectedObject3DName() const
{
	assert(m_PtrSelectedObject3D);
	return m_PtrSelectedObject3D->GetName();
}

void CGame::SelectObject2D(const string& Name)
{
	if (m_eEditMode != EEditMode::EditObject) return;

	m_PtrSelectedObject2D = GetObject2D(Name);
	if (m_PtrSelectedObject2D)
	{
		if (!m_PtrSelectedObject2D->IsInstanced())
		{
			m_SelectedInstanceID = -1;
		}
	}
}

void CGame::DeselectObject2D()
{
	m_PtrSelectedObject2D = nullptr;
	m_SelectedInstanceID = -1;
}

bool CGame::IsAnyObject2DSelected() const
{
	return (m_PtrSelectedObject2D) ? true : false;
}

CObject2D* CGame::GetSelectedObject2D()
{
	return m_PtrSelectedObject2D;
}

const string& CGame::GetSelectedObject2DName() const
{
	assert(m_PtrSelectedObject2D);
	return m_PtrSelectedObject2D->GetName();
}

void CGame::SelectCamera(const string& Name)
{
	if (m_eEditMode != EEditMode::EditObject) return;

	m_PtrSelectedCamera = GetCamera(Name);
	if (m_PtrSelectedCamera)
	{
		m_SelectedInstanceID = -1;
	}
}

void CGame::DeselectCamera()
{
	m_PtrSelectedCamera = nullptr;
}

bool CGame::IsAnyCameraSelected() const
{
	return (m_PtrSelectedCamera) ? true : false;
}

CCamera* CGame::GetSelectedCamera()
{
	return m_PtrSelectedCamera;
}

const string& CGame::GetSelectedCameraName() const
{
	assert(m_PtrSelectedCamera);
	return m_PtrSelectedCamera->GetName();
}

CCamera* CGame::GetCurrentCamera()
{
	return m_PtrCurrentCamera;
}

void CGame::SelectInstance(int InstanceID)
{
	if (m_eEditMode != EEditMode::EditObject) return;

	m_SelectedInstanceID = InstanceID;

	if (IsAnyObject3DSelected() && IsAnyInstanceSelected())
	{
		int iInstance{ GetSelectedInstanceID() };
		const XMVECTOR& BSTransaltion{ m_PtrSelectedObject3D->ComponentPhysics.BoundingSphere.CenterOffset };
		const XMVECTOR KInstanceTranslation{ m_PtrSelectedObject3D->GetInstanceCPUData(iInstance).Translation + BSTransaltion };
		m_CapturedGizmoTranslation = KInstanceTranslation;

		Select3DGizmos();
	}
}

void CGame::DeselectInstance()
{
	m_SelectedInstanceID = -1;
}

bool CGame::IsAnyInstanceSelected() const
{
	return (m_SelectedInstanceID != -1) ? true : false;
}

int CGame::GetSelectedInstanceID() const
{
	return m_SelectedInstanceID;
}

void CGame::SelectTerrain(bool bShouldEdit, bool bIsLeftButton)
{
	if (!m_Terrain) return;
	if (m_eEditMode != EEditMode::EditTerrain) return;

	CastPickingRay();

	m_Terrain->Select(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, bShouldEdit, bIsLeftButton);
}

void CGame::Select3DGizmos()
{
	if (EFLAG_HAS_NO(m_eFlagsRendering, EFlagsRendering::Use3DGizmos)) return;
	if (IsAnyObject3DSelected())
	{
		if (m_PtrSelectedObject3D->IsInstanced() && !IsAnyInstanceSelected()) return;
	}
	else
	{
		return;
	}

	XMVECTOR* pTranslation{ &m_PtrSelectedObject3D->ComponentTransform.Translation };
	XMVECTOR* pScaling{ &m_PtrSelectedObject3D->ComponentTransform.Scaling };
	float* pPitch{ &m_PtrSelectedObject3D->ComponentTransform.Pitch };
	float* pYaw{ &m_PtrSelectedObject3D->ComponentTransform.Yaw };
	float* pRoll{ &m_PtrSelectedObject3D->ComponentTransform.Roll };
	if (m_PtrSelectedObject3D->IsInstanced() && IsAnyInstanceSelected())
	{
		auto& InstanceCPUData{ m_PtrSelectedObject3D->GetInstanceCPUData(m_SelectedInstanceID) };
		pTranslation = &InstanceCPUData.Translation;
		pScaling = &InstanceCPUData.Scaling;
		pPitch = &InstanceCPUData.Pitch;
		pYaw = &InstanceCPUData.Yaw;
		pRoll = &InstanceCPUData.Roll;
	}

	// Calculate scalar IAW the distance from the camera
	m_3DGizmoDistanceScalar = XMVectorGetX(XMVector3Length(m_PtrCurrentCamera->GetEyePosition() - *pTranslation)) * 0.1f;
	m_3DGizmoDistanceScalar = pow(m_3DGizmoDistanceScalar, 0.7f);

	// Translate 3D gizmos
	const XMVECTOR& BSTransaltion{ m_PtrSelectedObject3D->ComponentPhysics.BoundingSphere.CenterOffset };
	const XMVECTOR KGizmoTranslation{ *pTranslation + BSTransaltion };
	m_Object3D_3DGizmoTranslationX->ComponentTransform.Translation =
		m_Object3D_3DGizmoTranslationY->ComponentTransform.Translation = m_Object3D_3DGizmoTranslationZ->ComponentTransform.Translation =
		m_Object3D_3DGizmoRotationPitch->ComponentTransform.Translation =
		m_Object3D_3DGizmoRotationYaw->ComponentTransform.Translation = m_Object3D_3DGizmoRotationRoll->ComponentTransform.Translation =
		m_Object3D_3DGizmoScalingX->ComponentTransform.Translation =
		m_Object3D_3DGizmoScalingY->ComponentTransform.Translation = m_Object3D_3DGizmoScalingZ->ComponentTransform.Translation = KGizmoTranslation;

	if (IsGizmoSelected())
	{
		int DeltaX{ m_CapturedMouseState.x - m_PrevCapturedMouseX };
		int DeltaY{ m_CapturedMouseState.y - m_PrevCapturedMouseY };
		int DeltaSum{ DeltaY - DeltaX };

		float DistanceObejctCamera{ abs(XMVectorGetX(m_CapturedGizmoTranslation - GetCurrentCamera()->GetEyePosition())) };
		if (DistanceObejctCamera < K3DGizmoCameraDistanceThreshold) DistanceObejctCamera = K3DGizmoCameraDistanceThreshold;
		const float KDeltaFactor{ pow(DistanceObejctCamera, K3DGizmoDistanceFactorExponent) };

		float TranslationX{ XMVectorGetX(*pTranslation) };
		float TranslationY{ XMVectorGetY(*pTranslation) };
		float TranslationZ{ XMVectorGetZ(*pTranslation) };
		float ScalingX{ XMVectorGetX(*pScaling) };
		float ScalingY{ XMVectorGetY(*pScaling) };
		float ScalingZ{ XMVectorGetZ(*pScaling) };

		switch (m_e3DGizmoMode)
		{
		case E3DGizmoMode::Translation:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pTranslation = XMVectorSetX(*pTranslation, TranslationX - DeltaSum * KTranslationDelta * KDeltaFactor);
				break;
			case E3DGizmoAxis::AxisY:
				*pTranslation = XMVectorSetY(*pTranslation, TranslationY - DeltaSum * KTranslationDelta * KDeltaFactor);
				break;
			case E3DGizmoAxis::AxisZ:
				*pTranslation = XMVectorSetZ(*pTranslation, TranslationZ - DeltaSum * KTranslationDelta * KDeltaFactor);
				break;
			default:
				break;
			}
			break;
		case E3DGizmoMode::Rotation:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pPitch -= DeltaSum * KRotation360To2PI * KRotationDelta * KDeltaFactor;
				break;
			case E3DGizmoAxis::AxisY:
				*pYaw -= DeltaSum * KRotation360To2PI * KRotationDelta * KDeltaFactor;
				break;
			case E3DGizmoAxis::AxisZ:
				*pRoll -= DeltaSum * KRotation360To2PI * KRotationDelta * KDeltaFactor;
				break;
			default:
				break;
			}
			break;
		case E3DGizmoMode::Scaling:
		{
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pScaling = XMVectorSetX(*pScaling, ScalingX - DeltaSum * KScalingDelta * KDeltaFactor);
				break;
			case E3DGizmoAxis::AxisY:
				*pScaling = XMVectorSetY(*pScaling, ScalingY - DeltaSum * KScalingDelta * KDeltaFactor);
				break;
			case E3DGizmoAxis::AxisZ:
				*pScaling = XMVectorSetZ(*pScaling, ScalingZ - DeltaSum * KScalingDelta * KDeltaFactor);
				break;
			default:
				break;
			}
		}
			break;
		default:
			break;
		}
		m_PtrSelectedObject3D->UpdateWorldMatrix();
		m_PtrSelectedObject3D->UpdateInstanceWorldMatrix(m_SelectedInstanceID);
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
			m_bIsGizmoHovered = true;
			if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationPitch.get(), E3DGizmoAxis::AxisX))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
			}
			else if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationYaw.get(), E3DGizmoAxis::AxisY))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
			}
			else if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationRoll.get(), E3DGizmoAxis::AxisZ))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
			}
			else
			{
				m_bIsGizmoHovered = false;
			}
			break;
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

bool CGame::ShouldSelectRotationGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis)
{
	XMVECTOR PlaneNormal{};
	switch (Axis)
	{
	case E3DGizmoAxis::None:
		return false;
		break;
	case E3DGizmoAxis::AxisX:
		PlaneNormal = XMVectorSet(1, 0, 0, 0);
		break;
	case E3DGizmoAxis::AxisY:
		PlaneNormal = XMVectorSet(0, 1, 0, 0);
		break;
	case E3DGizmoAxis::AxisZ:
		PlaneNormal = XMVectorSet(0, 0, 1, 0);
		break;
	default:
		break;
	}

	if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
		K3DGizmoSelectionRadius * m_3DGizmoDistanceScalar, Gizmo->ComponentTransform.Translation, nullptr))
	{
		XMVECTOR PlaneT{};
		if (IntersectRayPlane(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, Gizmo->ComponentTransform.Translation, PlaneNormal, &PlaneT))
		{
			XMVECTOR PointOnPlane{ m_PickingRayWorldSpaceOrigin + PlaneT * m_PickingRayWorldSpaceDirection };

			float Dist{ XMVectorGetX(XMVector3Length(PointOnPlane - Gizmo->ComponentTransform.Translation)) };
			if (Dist >= K3DGizmoSelectionLowBoundary * m_3DGizmoDistanceScalar && Dist <= K3DGizmoSelectionHighBoundary * m_3DGizmoDistanceScalar) return true;
		}
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

void CGame::DeselectAll()
{
	DeselectObject3D();
	DeselectObject2D();
	DeselectInstance();
	DeselectCamera();
	Deselect3DGizmos();
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

void CGame::PickBoundingSphere()
{
	m_vObject3DPickingCandidates.clear();
	m_PtrPickedObject3D = nullptr;
	m_PickedInstanceID = -1;

	XMVECTOR T{ KVectorGreatest };
	for (auto& i : m_vObject3Ds)
	{
		auto* Object3D{ i.get() };
		if (Object3D->ComponentPhysics.bIsPickable)
		{
			if (Object3D->IsInstanced())
			{
				int InstanceCount{ (int)Object3D->GetInstanceCount() };
				for (int iInstance = 0; iInstance < InstanceCount; ++iInstance)
				{
					auto& InstanceCPUData{ Object3D->GetInstanceCPUData(iInstance) };
					XMVECTOR NewT{ KVectorGreatest };
					if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
						InstanceCPUData.BoundingSphere.Radius, InstanceCPUData.Translation + InstanceCPUData.BoundingSphere.CenterOffset, &NewT))
					{
						m_vObject3DPickingCandidates.emplace_back(Object3D, iInstance, NewT);
					}
				}
			}
			else
			{
				XMVECTOR NewT{ KVectorGreatest };
				if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
					Object3D->ComponentPhysics.BoundingSphere.Radius, Object3D->ComponentTransform.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
				{
					m_vObject3DPickingCandidates.emplace_back(Object3D, NewT);
				}
			}
		}
	}
}

bool CGame::PickTriangle()
{
	XMVECTOR T{ KVectorGreatest };
	if (m_PtrPickedObject3D == nullptr)
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
			if (Candidate.InstanceID >= 0)
			{
				const auto& InstanceGPUData{ Candidate.PtrObject3D->GetInstanceGPUData(Candidate.InstanceID) };
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
					m_PtrPickedObject3D = FilteredCandidate.PtrObject3D;
					m_PickedInstanceID = FilteredCandidate.InstanceID;
				}
			}
			return true;
		}
	}
	return false;
}

void CGame::BeginRendering(const FLOAT* ClearColor, bool bUseDeferredRendering)
{
	m_bUseDeferredRendering = bUseDeferredRendering;

	if (m_bUseDeferredRendering)
	{
		m_DeviceContext->OMSetRenderTargets(1, m_ScreenQuadRTV.GetAddressOf(), m_DepthStencilView.Get());

		m_DeviceContext->ClearRenderTargetView(m_ScreenQuadRTV.Get(), Colors::CornflowerBlue);
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
	else
	{
		m_DeviceContext->OMSetRenderTargets(1, m_DeviceRTV.GetAddressOf(), m_DepthStencilView.Get());

		m_DeviceContext->ClearRenderTargetView(m_DeviceRTV.Get(), Colors::CornflowerBlue);
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	ID3D11SamplerState* SamplerState{ m_CommonStates->LinearWrap() };
	m_DeviceContext->PSSetSamplers(0, 1, &SamplerState);
	m_DeviceContext->DSSetSamplers(0, 1, &SamplerState); // @important: in order to use displacement mapping

	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

	SetUniversalRSState();

	XMVECTOR EyePosition{ m_PtrCurrentCamera->GetEyePosition() };
	XMVECTOR FocusPosition{ m_PtrCurrentCamera->GetFocusPosition() };
	XMVECTOR UpDirection{ m_PtrCurrentCamera->GetUpDirection() };
	m_MatrixView = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);

	if (m_EnvironmentTexture) m_EnvironmentTexture->Use();
	if (m_IrradianceTexture) m_IrradianceTexture->Use();
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
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Forward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.S)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Backward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.A  && !m_CapturedKeyboardState.LeftControl)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Leftward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.D)
		{
			m_PtrCurrentCamera->Move(CCamera::EMovementDirection::Rightward, m_DeltaTimeF * m_CameraMovementFactor);
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
		if (m_CapturedKeyboardState.Delete)
		{
			// Object3D
			if (IsAnyObject3DSelected())
			{
				if (GetSelectedObject3D()->IsInstanced())
				{
					if (IsAnyInstanceSelected())
					{
						string InstanceName{ GetSelectedObject3D()->GetInstanceCPUData(GetSelectedInstanceID()).Name };
						GetSelectedObject3D()->DeleteInstance(InstanceName);
						DeselectInstance();

						if (GetSelectedObject3D()->GetInstanceCount() == 0) DeselectObject3D();
					}
					else
					{
						DeselectInstance();

						DeleteObject3D(GetSelectedObject3DName());
						DeselectObject3D();
					}
				}
				else
				{
					DeleteObject3D(GetSelectedObject3DName());
					DeselectObject3D();
				}
			}
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
				if (Pick() && !IsGizmoSelected())
				{
					DeselectAll();

					SelectObject3D(GetPickedObject3DName());
					SelectInstance(GetPickedInstanceID());
				}
				m_bLeftButtonPressedOnce = false;
			}

			if (!m_CapturedMouseState.leftButton) Deselect3DGizmos();
			if (m_CapturedMouseState.rightButton)
			{
				DeselectAll();
			}

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

	m_CBPSFlagsData.EnvironmentTextureMipLevels = (m_EnvironmentTexture) ? m_EnvironmentTexture->GetMipLevels() : 0;
	m_CBPSFlagsData.bUsePhysicallyBasedRendering = EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UsePhysicallyBasedRendering);

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

		DrawCameraRepresentations();

		DrawObject3DLines();
	}

	if (m_SkyData.bIsDataSet)
	{
		DrawSky(m_DeltaTimeF);
	}

	DrawTerrain(m_DeltaTimeF);
	
	if (m_eMode == EMode::Edit)
	{
		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals))
		{
			UpdateCBSpace();
			m_GSNormal->Use();
		}
	}

	// Opaque Object3Ds
	for (auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->ComponentRender.bIsTransparent) continue;

		if (Object3D->IsRiggedModel())
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
		
		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
		{
			DrawObject3DBoundingSphere(Object3D.get());
		}
	}

	// Transparent Object3Ds
	for (auto& Object3D : m_vObject3Ds)
	{
		if (!Object3D->ComponentRender.bIsTransparent) continue;

		Object3D->Animate(m_DeltaTimeF);
		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
		{
			DrawObject3DBoundingSphere(Object3D.get());
		}
	}

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);

	DrawObject2Ds();
}

void CGame::UpdateObject3D(CObject3D* const PtrObject3D)
{
	if (!PtrObject3D) return;

	PtrObject3D->UpdateWorldMatrix();
	UpdateCBSpace(PtrObject3D->ComponentTransform.MatrixWorld);

	SetUniversalbUseLighiting();

	m_CBPSFlagsData.bUseLighting = EFLAG_HAS_NO(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoLighting);
	m_CBPSFlagsData.bUseTexture = EFLAG_HAS_NO(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoTexture);
	m_CBPSFlags->Update();

	assert(PtrObject3D->ComponentRender.PtrVS);
	assert(PtrObject3D->ComponentRender.PtrPS);
	CShader* VS{ PtrObject3D->ComponentRender.PtrVS };
	CShader* PS{ PtrObject3D->ComponentRender.PtrPS };
	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::UseRawVertexColor))
	{
		PS = m_PSVertexColor.get();
	}

	VS->Use();
	PS->Use();
}

void CGame::DrawObject3D(const CObject3D* const PtrObject3D, bool bIgnoreInstances, bool bIgnoreOwnTexture)
{
	if (!PtrObject3D) return;

	if (PtrObject3D->ShouldTessellate())
	{
		UpdateCBTessFactorData(PtrObject3D->GetTessFactorData());
		UpdateCBDisplacementData(PtrObject3D->GetDisplacementData());

		m_HSStatic->Use();
		m_DSStatic->Use();
	}

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRSState();
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

	m_Object3DBoundingSphere->Draw();

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

			m_CBSpaceWVPData.World = XMMatrixTranspose(Object3DLine->ComponentTransform.MatrixWorld);
			m_CBSpaceWVPData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
			m_CBSpaceWVP->Update();

			Object3DLine->Draw();
		}
	}
}

void CGame::DrawObject2Ds()
{
	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);
	
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

	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
}

void CGame::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	for (auto& Object3D : m_vObject3DMiniAxes)
	{
		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		Object3D->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition() + m_PtrCurrentCamera->GetForward();

		Object3D->UpdateWorldMatrix();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGame::UpdatePickingRay()
{
	m_Object3DLinePickingRay->GetVertices().at(0).Position = m_PickingRayWorldSpaceOrigin;
	m_Object3DLinePickingRay->GetVertices().at(1).Position = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * KPickingRayLength;
	m_Object3DLinePickingRay->UpdateVertexBuffer();
}

void CGame::DrawPickingRay()
{
	m_VSLine->Use();
	m_CBSpaceWVPData.World = XMMatrixTranspose(KMatrixIdentity);
	m_CBSpaceWVPData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	m_CBSpaceWVP->Update();

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSLine->Use();

	m_Object3DLinePickingRay->Draw();
}

void CGame::DrawPickedTriangle()
{
	m_VSBase->Use();
	m_CBSpaceWVPData.World = XMMatrixTranspose(KMatrixIdentity);
	m_CBSpaceWVPData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	m_CBSpaceWVP->Update();

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSVertexColor->Use();

	m_Object3DPickedTriangle->GetModel().vMeshes[0].vVertices[0].Position = m_PickedTriangleV0;
	m_Object3DPickedTriangle->GetModel().vMeshes[0].vVertices[1].Position = m_PickedTriangleV1;
	m_Object3DPickedTriangle->GetModel().vMeshes[0].vVertices[2].Position = m_PickedTriangleV2;
	m_Object3DPickedTriangle->UpdateMeshBuffer();

	m_Object3DPickedTriangle->Draw();
}

void CGame::DrawSky(float DeltaTime)
{
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

		// SkySphere
		{
			m_Object3DSkySphere->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition();

			UpdateObject3D(m_Object3DSkySphere.get());
			DrawObject3D(m_Object3DSkySphere.get());
		}

		// Sun
		{
			float SunRoll{ XM_2PI * m_CBSkyTimeData.SkyTime - XM_PIDIV2 };
			XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, SunRoll)) };
			m_Object3DSun->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition() + Offset;
			m_Object3DSun->ComponentTransform.Roll = SunRoll;

			UpdateObject3D(m_Object3DSun.get());
			DrawObject3D(m_Object3DSun.get());
		}

		// Moon
		{
			float MoonRoll{ XM_2PI * m_CBSkyTimeData.SkyTime + XM_PIDIV2 };
			XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, MoonRoll)) };
			m_Object3DMoon->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition() + Offset;
			m_Object3DMoon->ComponentTransform.Roll = (MoonRoll > XM_2PI) ? (MoonRoll - XM_2PI) : MoonRoll;

			UpdateObject3D(m_Object3DMoon.get());
			DrawObject3D(m_Object3DMoon.get());
		}
	}
	else
	{
		if (m_EnvironmentTexture) m_EnvironmentTexture->Use();
		if (m_IrradianceTexture) m_IrradianceTexture->Use();

		// SkySphere
		{
			m_Object3DSkySphere->ComponentTransform.Translation = m_PtrCurrentCamera->GetEyePosition();

			UpdateObject3D(m_Object3DSkySphere.get());
			DrawObject3D(m_Object3DSkySphere.get(), true, true);
		}
	}

	m_CBSkyTime->Update();
}

void CGame::DrawTerrain(float DeltaTime)
{
	if (!m_Terrain) return;
	
	SetUniversalRSState();
	SetUniversalbUseLighiting();
	
	m_CBSpaceVPData.ViewProjection = GetTransposedViewProjectionMatrix();

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

	if (m_eMode == EMode::Edit)
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

void CGame::Draw3DGizmos()
{
	if (IsAnyObject3DSelected())
	{
		if (m_PtrSelectedObject3D->IsInstanced() && !IsAnyInstanceSelected()) return;
	}
	else
	{
		return;
	}

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
	CShader* VS{ Gizmo->ComponentRender.PtrVS };
	CShader* PS{ Gizmo->ComponentRender.PtrPS };

	float Scalar{ XMVectorGetX(XMVector3Length(m_PtrCurrentCamera->GetEyePosition() - Gizmo->ComponentTransform.Translation)) * 0.1f };
	Scalar = pow(Scalar, 0.7f);

	Gizmo->ComponentTransform.Scaling = XMVectorSet(Scalar, Scalar, Scalar, 0.0f);
	Gizmo->UpdateWorldMatrix();
	UpdateCBSpace(Gizmo->ComponentTransform.MatrixWorld);
	VS->Use();

	m_CBGizmoColorFactorData.ColorFactor = (bShouldHighlight) ? XMVectorSet(2.0f, 2.0f, 2.0f, 0.95f) : XMVectorSet(0.75f, 0.75f, 0.75f, 0.75f);
	m_CBGizmoColorFactor->Update();
	PS->Use();

	Gizmo->Draw();
}

void CGame::DrawCameraRepresentations()
{
	if (GetCameraMap().size() == 1) return;

	m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	
	CShader* const VS{ m_Object3D_CameraRepresentation->ComponentRender.PtrVS };
	CShader* const PS{ m_Object3D_CameraRepresentation->ComponentRender.PtrPS };
	VS->Use();
	PS->Use();

	for (auto& Camera : m_vCameras)
	{
		if (Camera.get() == GetEditorCamera()) continue;
		if (Camera.get() == GetCurrentCamera()) continue;

		XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(Camera->GetPitch(), Camera->GetYaw(), 0.0f) };
		XMMATRIX Translation{ XMMatrixTranslationFromVector(Camera->GetEyePosition()) };
		UpdateCBSpace(Rotation * Translation);

		if (IsAnyCameraSelected())
		{
			m_CBCameraSelectionData.bIsSelected = (Camera.get() == GetSelectedCamera()) ? TRUE : FALSE;
		}
		else
		{
			m_CBCameraSelectionData.bIsSelected = FALSE;
		}
		m_CBCameraSelection->Update();

		m_Object3D_CameraRepresentation->Draw();
	}

	SetUniversalRSState();
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
	if (ImGui::BeginPopup(u8"오브젝트 추가기", ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char NewObejctName[KAssetNameMaxLength]{};
		static char ModelFileNameWithPath[MAX_PATH]{};
		static char ModelFileNameWithoutPath[MAX_PATH]{};
		static bool bIsModelRigged{ false };

		static constexpr float KIndentPerDepth{ 12.0f };
		static constexpr float KItemsOffetX{ 150.0f };
		static constexpr float KItemsWidth{ 150.0f };
		static const char* const KOptions[4]{ u8"3D 도형", u8"3D 모델 파일", u8"2D 도형", u8"카메라" };
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
			else if (strlen(NewObejctName) == 0)
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
						Object3D->CreateFromFile(ModelFileNameWithPath, bIsModelRigged);

						memset(ModelFileNameWithPath, 0, MAX_PATH);
						memset(ModelFileNameWithoutPath, 0, MAX_PATH);
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

				if (IsObjectCreated)
				{
					m_EditorGUIBools.bShowPopupObjectAdder = false;
					memset(NewObejctName, 0, KAssetNameMaxLength);

					ImGui::CloseCurrentPopup();
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button(u8"취소"))
		{
			m_EditorGUIBools.bShowPopupObjectAdder = false;
			memset(ModelFileNameWithPath, 0, MAX_PATH);
			memset(ModelFileNameWithoutPath, 0, MAX_PATH);
			memset(NewObejctName, 0, KAssetNameMaxLength);
			ImGui::CloseCurrentPopup();
		}

		if (bShowDialogLoad3DModel)
		{
			static CFileDialog FileDialog{ GetWorkingDirectory() };
			if (FileDialog.OpenFileDialog("FBX 파일\0*.fbx\0모든 파일\0*.*\0", "모델 불러오기"))
			{
				strcpy_s(ModelFileNameWithPath, FileDialog.GetRelativeFileName().c_str());
				strcpy_s(ModelFileNameWithoutPath, FileDialog.GetFileNameWithoutPath().c_str());
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

					if (IsAnyObject3DSelected())
					{
						ImGui::PushItemWidth(ItemsWidth);
						{
							CObject3D* const Object3D{ GetSelectedObject3D() };

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"선택된 오브젝트:");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"<%s>", GetSelectedObject3DName().c_str());

							ImGui::Separator();

							if (Object3D->IsInstanced())
							{
								int iSelectedInstance{ GetSelectedInstanceID() };
								if (iSelectedInstance == -1)
								{
									ImGui::Text(u8"<인스턴스를 선택해 주세요.>");
								}
								else
								{
									auto& InstanceCPUData{ Object3D->GetInstanceCPUData(GetSelectedInstanceID()) };

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
										Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
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
										Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
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
										Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
									}

									ImGui::Separator();
								}
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

							float InsideTessFactor{ TessFactorData.InsideTessFactor };
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"테셀레이션 내부 계수");
							ImGui::SameLine(ItemsOffsetX);
							if (ImGui::SliderFloat(u8"##테셀레이션 내부 계수", &InsideTessFactor, 0.0f, 64.0f, "%.2f"))
							{
								TessFactorData.InsideTessFactor = InsideTessFactor;
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
							if (Object3D->IsRiggedModel())
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
					}
					else if (IsAnyObject2DSelected())
					{
						// Object2D
					}
					else if (IsAnyCameraSelected())
					{
						ImGui::PushItemWidth(ItemsWidth);
						{
							CCamera* const SelectedCamera{ GetSelectedCamera() };
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

							if (m_PtrCurrentCamera != GetEditorCamera())
							{
								ImGui::SetCursorPosX(ItemsOffsetX);
								if (ImGui::Button(u8"에디터 카메라로 돌아가기", ImVec2(ItemsWidth, 0)))
								{
									m_PtrCurrentCamera = GetEditorCamera();
								}
							}

							ImGui::Separator();

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"선택된 카메라:");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							string Name{ GetSelectedCameraName() };
							ImGui::Text(u8"<%s>", GetSelectedCameraName().c_str());

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
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"회전 Pitch");
							ImGui::SameLine(ItemsOffsetX);
							if (ImGui::DragFloat(u8"##회전 Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
							{
								SelectedCamera->SetPitch(Pitch);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"회전 Yaw");
							ImGui::SameLine(ItemsOffsetX);
							if (ImGui::DragFloat(u8"##회전 Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
							{
								SelectedCamera->SetYaw(Yaw);
							}

							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"카메라 이동 속도");
							ImGui::SameLine(ItemsOffsetX);
							ImGui::SliderFloat(u8"##카메라 이동 속도", &m_CameraMovementFactor, 1.0f, 100.0f, "%.0f");

							if (m_PtrCurrentCamera != SelectedCamera)
							{
								ImGui::SetCursorPosX(ItemsOffsetX);
								if (ImGui::Button(u8"현재 화면 카메라로 지정", ImVec2(ItemsWidth, 0)))
								{
									m_PtrCurrentCamera = SelectedCamera;
								}
							}
						}
						ImGui::PopItemWidth();
					}
					else
					{
						ImGui::Text(u8"<먼저 오브젝트를 선택하세요.>");
					}

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
								CObject3D* const Object3D{ GetSelectedObject3D() };
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
						CObject3D* const Object3D{ GetSelectedObject3D() };

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
							CObject3D* const Object3D{ GetSelectedObject3D() };
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
									XMFLOAT3 WindVelocity{}; XMStoreFloat3(&WindVelocity, Terrain->GetWindVelocity());
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
									GenerateCubemapFromHDR();
								}
							}
						}

						if (m_EnvironmentTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Environment map: ");

							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(m_EnvironmentTexture->GetFileName().c_str());

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

							m_EnvironmentRep->UnfoldCubemap(m_EnvironmentTexture->GetShaderResourceViewPtr(),
								(m_bUseDeferredRendering) ? m_ScreenQuadRTV.GetAddressOf() : m_DeviceRTV.GetAddressOf(),
								m_DepthStencilView.Get());

							ImGui::Image(m_EnvironmentRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx(u8"Irradiance map"))
					{
						if (ImGui::Button(u8"불러오기"))
						{
							static CFileDialog FileDialog{ GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("DDS 파일\0*.dds\0HDR 파일\0*.hdr\0", "Irradiance map 불러오기"))
							{
								m_IrradianceTexture->CreateCubeMapFromFile(FileDialog.GetFileName());

								if (m_IrradianceTexture->IsHDR())
								{
									GenerateCubemapFromHDR();
								}
							}
						}

						if (m_EnvironmentTexture)
						{
							ImGui::SameLine();

							if (ImGui::Button(u8"Environment map에서 생성하기"))
							{
								GenerateIrradianceMap();
							}
						}

						if (m_IrradianceTexture)
						{
							ImGui::AlignTextToFramePadding();
							ImGui::Text(u8"현재 Irradiance map: ");

							ImGui::SameLine(ItemsOffsetX);
							ImGui::AlignTextToFramePadding();
							ImGui::Text(m_IrradianceTexture->GetFileName().c_str());

							if (ImGui::Button(u8"내보내기"))
							{
								static CFileDialog FileDialog{ GetWorkingDirectory() };
								if (FileDialog.SaveFileDialog("DDS 파일(*.DDS)\0*.DDS\0", "Irradiance map 내보내기", ".DDS"))
								{
									m_IrradianceTexture->SaveDDSFile(FileDialog.GetRelativeFileName());
								}
							}

							// @important
							m_CBSpace2DData.World = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2DData.Projection = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2D->Update();

							m_IrradianceRep->UnfoldCubemap(m_IrradianceTexture->GetShaderResourceViewPtr(),
								(m_bUseDeferredRendering) ? m_ScreenQuadRTV.GetAddressOf() : m_DeviceRTV.GetAddressOf(),
								m_DepthStencilView.Get());

							ImGui::Image(m_IrradianceRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (ImGui::TreeNodeEx(u8"Prefiltered radiance map"))
					{
						if (m_EnvironmentTexture)
						{
							if (ImGui::Button(u8"Prefiltered radiance map 생성하기"))
							{
								GeneratePrefilteredRadianceMap();
							}
						}

						if (m_PrefilteredRadianceTexture)
						{
							// @important
							m_CBSpace2DData.World = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2DData.Projection = XMMatrixTranspose(KMatrixIdentity);
							m_CBSpace2D->Update();

							m_PrefilteredRadianceRep->UnfoldCubemap(m_PrefilteredRadianceTexture->GetShaderResourceViewPtr(),
								(m_bUseDeferredRendering) ? m_ScreenQuadRTV.GetAddressOf() : m_DeviceRTV.GetAddressOf(),
								m_DepthStencilView.Get());

							ImGui::Image(m_PrefilteredRadianceRep->GetSRV(), ImVec2(CCubemapRep::KRepWidth, CCubemapRep::KRepHeight));
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

						/*
						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"3D Gizmo 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawBoundingSphere{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::Use3DGizmos) };
						if (ImGui::Checkbox(u8"##3D Gizmo 표시", &bDrawBoundingSphere))
						{
							ToggleGameRenderingFlags(EFlagsRendering::Use3DGizmos);
						}
						*/

						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"화면 상단에 좌표축 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawMiniAxes{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawMiniAxes) };
						if (ImGui::Checkbox(u8"##화면 상단에 좌표축 표시", &bDrawMiniAxes))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawMiniAxes);
						}

						/*
						ImGui::AlignTextToFramePadding();
						ImGui::Text(u8"피킹 데이터 표시");
						ImGui::SameLine(ItemsOffsetX);
						bool bDrawPickingData{ EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawPickingData) };
						if (ImGui::Checkbox(u8"##피킹 데이터 표시", &bDrawPickingData))
						{
							ToggleGameRenderingFlags(EFlagsRendering::DrawPickingData);
						}
						*/
						
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
		ImGui::SetNextWindowSizeConstraints(ImVec2(300, 60), ImVec2(400, 300));
		if (ImGui::Begin(u8"장면 편집기", &m_EditorGUIBools.bShowWindowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
		{
			// 장면 내보내기
			if (ImGui::Button(u8"장면 내보내기"))
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
				if (FileDialog.SaveFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 내보내기", ".scene"))
				{
					SaveScene(FileDialog.GetRelativeFileName());
				}
			}

			ImGui::SameLine();

			// 장면 불러오기
			if (ImGui::Button(u8"장면 불러오기"))
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
				if (FileDialog.OpenFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 불러오기"))
				{
					LoadScene(FileDialog.GetRelativeFileName());
				}
			}

			ImGui::Separator();

			// 오브젝트 추가
			if (ImGui::Button(u8"오브젝트 추가"))
			{
				m_EditorGUIBools.bShowPopupObjectAdder = true;
			}

			ImGui::SameLine();

			// 오브젝트 제거
			if (ImGui::Button(u8"오브젝트 제거"))
			{
				if (IsAnyObject3DSelected())
				{
					string Name{ GetSelectedObject3DName() };
					DeleteObject3D(Name);
				}
				if (IsAnyObject2DSelected())
				{
					string Name{ GetSelectedObject2DName() };
					DeleteObject2D(Name);
				}
				if (IsAnyCameraSelected())
				{
					string Name{ GetSelectedCameraName() };
					DeleteCamera(Name);
				}
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
					if (IsAnyObject3DSelected())
					{
						if (GetSelectedObject3DName() == Object3DPair.first) bIsThisObject3DSelected = true;
					}

					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth };
					if (bIsThisObject3DSelected) Flags |= ImGuiTreeNodeFlags_Selected;
					if (!Object3D->IsInstanced()) Flags |= ImGuiTreeNodeFlags_Leaf;

					if (!Object3D->IsInstanced()) ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

					bool bIsNodeOpen{ ImGui::TreeNodeEx(Object3DPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectObject3D(Object3DPair.first);

						DeselectObject2D();
						DeselectCamera();
						DeselectInstance();
					}

					ImGui::NextColumn();

					if (!Object3D->IsRiggedModel())
					{
						// 인스턴스 추가
						ImGui::PushID(iObject3DPair * 2 + 0);
						if (ImGui::Button(u8"추가"))
						{
							Object3D->InsertInstance();
						}
						ImGui::PopID();

						// 인스턴스 제거
						if (Object3D->IsInstanced() && IsAnyInstanceSelected())
						{
							ImGui::SameLine();

							ImGui::PushID(iObject3DPair * 2 + 1);
							if (ImGui::Button(u8"제거"))
							{
								const CObject3D::SInstanceCPUData& InstanceCPUData{ Object3D->GetInstanceCPUData(GetSelectedInstanceID()) };
								Object3D->DeleteInstance(InstanceCPUData.Name);
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
							int iInstancePair{};
							const auto& InstanceMap{ Object3D->GetInstanceMap() };
							for (auto& InstancePair : InstanceMap)
							{
								bool bSelected{ false };
								if (IsAnyInstanceSelected()) bSelected = (iInstancePair == GetSelectedInstanceID());
								
								if (!bIsThisObject3DSelected) bSelected = false;

								if (ImGui::Selectable(InstancePair.first.c_str(), bSelected))
								{
									if (!bIsThisObject3DSelected)
									{
										SelectObject3D(Object3DPair.first);
									}

									SelectInstance(iInstancePair);
								}
								++iInstancePair;
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
					if (IsAnyObject2DSelected())
					{
						if (GetSelectedObject2DName() == Object2DPair.first) bIsThisObject2DSelected = true;
					}

					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth };
					if (bIsThisObject2DSelected) Flags |= ImGuiTreeNodeFlags_Selected;
					if (!Object2D->IsInstanced()) Flags |= ImGuiTreeNodeFlags_Leaf;

					if (!Object2D->IsInstanced()) ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

					bool bIsNodeOpen{ ImGui::TreeNodeEx(Object2DPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectObject2D(Object2DPair.first);

						DeselectObject3D();
						DeselectCamera();
						DeselectInstance();
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
				int iCameraPair{};
				for (const auto& CameraPair : mapCamera)
				{
					CCamera* const Camera{ GetCamera(CameraPair.first) };
					bool bIsThisCameraSelected{ false };
					if (IsAnyCameraSelected())
					{
						if (GetSelectedCameraName() == CameraPair.first) bIsThisCameraSelected = true;
					}

					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf };
					if (bIsThisCameraSelected) Flags |= ImGuiTreeNodeFlags_Selected;

					ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
					bool bIsNodeOpen{ ImGui::TreeNodeEx(CameraPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectCamera(CameraPair.first);

						DeselectObject3D();
						DeselectObject2D();
						DeselectInstance();
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

void CGame::GenerateCubemapFromHDR()
{
	m_EnvironmentTexture->GetTexture2DPtr()->GetDesc(&m_GeneratedEnvironmentMapTextureDesc);

	// @important
	m_GeneratedEnvironmentMapTextureDesc.Width /= 4;
	m_GeneratedEnvironmentMapTextureDesc.Height /= 2;
	m_GeneratedEnvironmentMapTextureDesc.ArraySize = 6;
	m_GeneratedEnvironmentMapTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_GeneratedEnvironmentMapTextureDesc.CPUAccessFlags = 0;
	m_GeneratedEnvironmentMapTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//m_GeneratedEnvironmentMapTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
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

		m_DeviceContext->OMSetRenderTargets(1, (m_bUseDeferredRendering) ? m_ScreenQuadRTV.GetAddressOf() : m_DeviceRTV.GetAddressOf(),
			m_DepthStencilView.Get());
	}

	m_DeviceContext->GenerateMips(m_GeneratedEnvironmentMapSRV.Get());

	m_EnvironmentTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_EnvironmentTexture->CopyTexture(m_GeneratedEnvironmentMapTexture.Get());
	m_EnvironmentTexture->SetSlot(KEnvironmentTextureSlot);
}

void CGame::GenerateIrradianceMap()
{
	m_EnvironmentTexture->GetTexture2DPtr()->GetDesc(&m_GeneratedIrradianceMapTextureDesc);

	// @important
	m_GeneratedIrradianceMapTextureDesc.Width /= 4;
	m_GeneratedIrradianceMapTextureDesc.Height /= 4;
	m_GeneratedIrradianceMapTextureDesc.ArraySize = 6;
	m_GeneratedIrradianceMapTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_GeneratedIrradianceMapTextureDesc.CPUAccessFlags = 0;
	m_GeneratedIrradianceMapTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//m_GeneratedIrradianceMapTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
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

		m_DeviceContext->OMSetRenderTargets(1, (m_bUseDeferredRendering) ? m_ScreenQuadRTV.GetAddressOf() : m_DeviceRTV.GetAddressOf(),
			m_DepthStencilView.Get());
	}

	m_DeviceContext->GenerateMips(m_GeneratedIrradianceMapSRV.Get());

	m_IrradianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_IrradianceTexture->CopyTexture(m_GeneratedIrradianceMapTexture.Get());
	m_IrradianceTexture->SetSlot(KIrradianceTextureSlot);
}

void CGame::GeneratePrefilteredRadianceMap()
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
	m_PrefilteredRadianceMapTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//m_PrefilteredRadianceMapTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // @important: HDR
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
				float Roughness{ (float)iMipLevel / (float)(BiasedMipMax - 1) };
				m_CBRadiancePrefilteringData.Roughness = Roughness;
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
		
		m_DeviceContext->OMSetRenderTargets(1, (m_bUseDeferredRendering) ? m_ScreenQuadRTV.GetAddressOf() : m_DeviceRTV.GetAddressOf(),
			m_DepthStencilView.Get());
	}

	m_PrefilteredRadianceTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_PrefilteredRadianceTexture->CopyTexture(m_PrefilteredRadianceMapTexture.Get());
	m_PrefilteredRadianceTexture->SetSlot(KPrefilteredRadianceTextureSlot);
}

void CGame::EndRendering()
{
	if (m_IsDestroyed) return;

	// Pass-through drawing
	if (m_bUseDeferredRendering)
	{
		DrawScreenQuadToSceen(m_PSScreenQuad.get(), true);

		// Edge detection
		{
			m_DeviceContext->OMSetRenderTargets(1, m_ScreenQuadRTV.GetAddressOf(), m_DepthStencilView.Get());
			m_DeviceContext->ClearRenderTargetView(m_ScreenQuadRTV.Get(), Colors::Transparent);
			m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			if (IsAnyObject3DSelected())
			{
				CObject3D* const Object3D{ GetSelectedObject3D() };
				if (IsAnyInstanceSelected())
				{
					int InstanceID{ GetSelectedInstanceID() };
					const auto& InstanceGPUData{ Object3D->GetInstanceGPUData(InstanceID) };

					UpdateObject3D(Object3D);

					UpdateCBSpace(InstanceGPUData.WorldMatrix);
					m_VSBase->Use();

					DrawObject3D(Object3D, true);
				}
				else
				{
					UpdateObject3D(Object3D);
					DrawObject3D(Object3D);
				}
			}

			DrawScreenQuadToSceen(m_PSEdgeDetector.get(), false);
		}
	}
	else
	{
		DrawScreenQuadToSceen(m_PSScreenQuad.get(), false);
	}

	if (m_eMode != EMode::Play)
	{
		m_DeviceContext->OMSetRenderTargets(1, m_DeviceRTV.GetAddressOf(), m_DepthStencilView.Get());
		m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
}

void CGame::DrawScreenQuadToSceen(CShader* const PixelShader, bool bShouldClearDeviceRTV)
{
	m_DeviceContext->RSSetState(m_CommonStates->CullNone());

	// Deferred texture to screen
	m_DeviceContext->OMSetRenderTargets(1, m_DeviceRTV.GetAddressOf(), m_DepthStencilView.Get());
	if (bShouldClearDeviceRTV) m_DeviceContext->ClearRenderTargetView(m_DeviceRTV.Get(), Colors::Transparent);
	//m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_VSScreenQuad->Use();
	PixelShader->Use();

	ID3D11SamplerState* const PointSampler{ m_CommonStates->PointWrap() };
	m_DeviceContext->PSSetSamplers(0, 1, &PointSampler);
	m_DeviceContext->PSSetShaderResources(0, 1, m_ScreenQuadSRV.GetAddressOf());

	// Draw full-screen quad vertices
	m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_DeviceContext->IASetVertexBuffers(0, 1, m_ScreenQuadVertexBuffer.GetAddressOf(), &m_ScreenQuadVertexBufferStride, &m_ScreenQuadVertexBufferOffset);
	m_DeviceContext->Draw(6, 0);

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

float CGame::GetSkyTime() const
{
	return m_CBSkyTimeData.SkyTime;
}

XMMATRIX CGame::GetTransposedViewProjectionMatrix() const
{
	return XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
}
