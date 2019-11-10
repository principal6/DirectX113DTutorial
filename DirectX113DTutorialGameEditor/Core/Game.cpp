#include <thread>

#include "Game.h"
#include "Core/FileDialog.h"

using std::min;
using std::vector;
using std::string;
using std::wstring;
using std::thread;
using std::chrono::steady_clock;
using std::to_string;
using std::stof;
using std::make_unique;

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

void CGame::CreateWin32(WNDPROC const WndProc, const std::string& WindowName, bool bWindowed)
{
	CreateWin32Window(WndProc, WindowName);

	InitializeDirectX(bWindowed);

	GetCurrentDirectoryA(MAX_PATH, m_WorkingDirectory);

	CMaterial MaterialDefaultGround{};
	{
		MaterialDefaultGround.SetName("DefaultGround");
		MaterialDefaultGround.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGround.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\ground0.jpg");
		MaterialDefaultGround.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\ground0_normal.jpg");
		MaterialDefaultGround.SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, "Asset\\ground0_displacement.jpg");
		AddMaterial(MaterialDefaultGround);
	}

	InitializeImGui();
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
	assert(!m_hWnd);

	constexpr LPCTSTR KClassName{ TEXT("GameWindow") };
	constexpr DWORD KWindowStyle{ WS_CAPTION | WS_SYSMENU };

	WNDCLASSEX WindowClass{};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WindowClass.hIcon = WindowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	WindowClass.hInstance = m_hInstance;
	WindowClass.lpfnWndProc = WndProc;
	WindowClass.lpszClassName = KClassName;
	WindowClass.lpszMenuName = nullptr;
	WindowClass.style = CS_VREDRAW | CS_HREDRAW;
	RegisterClassEx(&WindowClass);

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

	CreateSetViews();

	SetViewports();

	CreateDepthStencilStates();
	CreateBlendStates();

	SetPerspective(KDefaultFOV, KDefaultNearZ, KDefaultFarZ);

	CreateInputDevices();

	CreateBaseShaders();

	CreateMiniAxes();

	CreatePickingRay();
	CreatePickedTriangle();

	CreateBoundingSphere();

	Create3DGizmos();

	m_MatrixProjection2D = XMMatrixOrthographicLH(m_WindowSize.x, m_WindowSize.y, 0.0f, 1.0f);
	m_CommonStates = make_unique<CommonStates>(m_Device.Get());
}

void CGame::InitializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_hWnd);
	ImGui_ImplDX11_Init(m_Device.Get(), m_DeviceContext.Get());

	ImGuiIO& igIO{ ImGui::GetIO() };
	igIO.Fonts->AddFontDefault();

	m_ImGuiFont = igIO.Fonts->AddFontFromFileTTF("Asset\\D2Coding.ttf", 15.0f, nullptr, igIO.Fonts->GetGlyphRangesKorean());
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
		&SwapChainDesc, &m_SwapChain, &m_Device, nullptr, &m_DeviceContext);
}

void CGame::CreateSetViews()
{
	ComPtr<ID3D11Texture2D> BackBuffer{};
	m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &BackBuffer);

	m_Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &m_RenderTargetView);

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

	m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
}

void CGame::SetViewports()
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

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateLessEqualNoWrite.GetAddressOf())));

	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateAlways.GetAddressOf())));
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

void CGame::CreateBaseShaders()
{
	m_VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase->Create(EShaderType::VertexShader, L"Shader\\VSBase.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSBase->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));

	m_VSInstance = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSInstance->Create(EShaderType::VertexShader, L"Shader\\VSInstance.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSInstance->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));

	m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSAnimation->Create(EShaderType::VertexShader, L"Shader\\VSAnimation.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSAnimation->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));
	m_VSAnimation->AddConstantBuffer(&m_CBAnimationBonesData, sizeof(SCBAnimationBonesData));
	m_VSAnimation->AddConstantBuffer(&m_CBAnimationData, sizeof(CObject3D::SCBAnimationData));

	m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSSky->Create(EShaderType::VertexShader, L"Shader\\VSSky.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSSky->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));

	m_VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSLine->Create(EShaderType::VertexShader, L"Shader\\VSLine.hlsl", "main", CObject3DLine::KInputElementDescs, ARRAYSIZE(CObject3DLine::KInputElementDescs));
	m_VSLine->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));

	m_VSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSGizmo->Create(EShaderType::VertexShader, L"Shader\\VSGizmo.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSGizmo->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));

	m_VSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSTerrain->Create(EShaderType::VertexShader, L"Shader\\VSTerrain.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSTerrain->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));
	m_VSTerrain->AddConstantBuffer(&m_CBTerrainData, sizeof(CTerrain::SCBTerrainData));

	m_VSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSFoliage->Create(EShaderType::VertexShader, L"Shader\\VSFoliage.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSFoliage->AddConstantBuffer(&m_CBSpaceWVPData, sizeof(SCBSpaceWVPData));
	m_VSFoliage->AddConstantBuffer(&m_CBTerrainData, sizeof(CTerrain::SCBTerrainData));
	m_VSFoliage->AddConstantBuffer(&m_CBWindData, sizeof(CTerrain::SCBWindData));

	m_VSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSParticle->Create(EShaderType::VertexShader, L"Shader\\VSParticle.hlsl", "main",
		CParticlePool::KInputElementDescs, ARRAYSIZE(CParticlePool::KInputElementDescs));

	m_VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase2D->Create(EShaderType::VertexShader, L"Shader\\VSBase2D.hlsl", "main", CObject2D::KInputLayout, ARRAYSIZE(CObject2D::KInputLayout));
	m_VSBase2D->AddConstantBuffer(&m_CBSpace2DData, sizeof(SCBSpace2DData));

	m_HSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSTerrain->Create(EShaderType::HullShader, L"Shader\\HSTerrain.hlsl", "main");
	m_HSTerrain->AddConstantBuffer(&m_CBCameraData, sizeof(SCBCameraData));
	m_HSTerrain->AddConstantBuffer(&m_CBTessFactorData, sizeof(SCBTessFactorData));

	m_HSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSWater->Create(EShaderType::HullShader, L"Shader\\HSWater.hlsl", "main");
	m_HSWater->AddConstantBuffer(&m_CBCameraData, sizeof(SCBCameraData));
	m_HSWater->AddConstantBuffer(&m_CBTessFactorData, sizeof(SCBTessFactorData));

	m_DSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSTerrain->Create(EShaderType::DomainShader, L"Shader\\DSTerrain.hlsl", "main");
	m_DSTerrain->AddConstantBuffer(&m_CBSpaceVPData, sizeof(SCBSpaceVPData));
	m_DSTerrain->AddConstantBuffer(&m_CBDisplacementData, sizeof(SCBDisplacementData));

	m_DSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSWater->Create(EShaderType::DomainShader, L"Shader\\DSWater.hlsl", "main");
	m_DSWater->AddConstantBuffer(&m_CBSpaceVPData, sizeof(SCBSpaceVPData));
	m_DSWater->AddConstantBuffer(&m_CBWaterTimeData, sizeof(SCBWaterTimeData));

	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormal.hlsl", "main");
	m_GSNormal->AddConstantBuffer(&m_CBSpaceVPData, sizeof(SCBSpaceVPData));

	m_GSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSParticle->Create(EShaderType::GeometryShader, L"Shader\\GSParticle.hlsl", "main");
	m_GSParticle->AddConstantBuffer(&m_CBSpaceVPData, sizeof(SCBSpaceVPData));

	m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase->Create(EShaderType::PixelShader, L"Shader\\PSBase.hlsl", "main");
	m_PSBase->AddConstantBuffer(&m_cbPSBaseFlagsData, sizeof(SCBPSBaseFlagsData));
	m_PSBase->AddConstantBuffer(&m_CBLightData, sizeof(SCBLightData));
	m_PSBase->AddConstantBuffer(&m_CBMaterialData, sizeof(SCBMaterialData));

	m_PSVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSVertexColor->Create(EShaderType::PixelShader, L"Shader\\PSVertexColor.hlsl", "main");

	m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSSky->Create(EShaderType::PixelShader, L"Shader\\PSSky.hlsl", "main");
	m_PSSky->AddConstantBuffer(&m_CBSkyTimeData, sizeof(SCBSkyTimeData));

	m_PSCloud = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCloud->Create(EShaderType::PixelShader, L"Shader\\PSCloud.hlsl", "main");
	m_PSCloud->AddConstantBuffer(&m_CBSkyTimeData, sizeof(SCBSkyTimeData));

	m_PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSLine->Create(EShaderType::PixelShader, L"Shader\\PSLine.hlsl", "main");

	m_PSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSGizmo->Create(EShaderType::PixelShader, L"Shader\\PSGizmo.hlsl", "main");
	m_PSGizmo->AddConstantBuffer(&m_CBGizmoColorFactorData, sizeof(SCBGizmoColorFactorData));

	m_PSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSTerrain->Create(EShaderType::PixelShader, L"Shader\\PSTerrain.hlsl", "main");
	m_PSTerrain->AddConstantBuffer(&m_CBTerrainMaskingSpaceData, sizeof(SCBTerrainMaskingSpaceData));
	m_PSTerrain->AddConstantBuffer(&m_CBLightData, sizeof(SCBLightData));
	m_PSTerrain->AddConstantBuffer(&m_CBTerrainSelectionData, sizeof(CTerrain::SCBTerrainSelectionData));
	m_PSTerrain->AddConstantBuffer(&m_CBEditorTimeData, sizeof(SCBEditorTimeData));

	m_PSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSWater->Create(EShaderType::PixelShader, L"Shader\\PSWater.hlsl", "main");
	m_PSWater->AddConstantBuffer(&m_CBWaterTimeData, sizeof(SCBWaterTimeData));
	m_PSWater->AddConstantBuffer(&m_CBLightData, sizeof(SCBLightData));

	m_PSFoliage = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSFoliage->Create(EShaderType::PixelShader, L"Shader\\PSFoliage.hlsl", "main");
	m_PSFoliage->AddConstantBuffer(&m_cbPSBaseFlagsData, sizeof(SCBPSBaseFlagsData));
	m_PSFoliage->AddConstantBuffer(&m_CBLightData, sizeof(SCBLightData));
	m_PSFoliage->AddConstantBuffer(&m_CBMaterialData, sizeof(SCBMaterialData));

	m_PSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSParticle->Create(EShaderType::PixelShader, L"Shader\\PSParticle.hlsl", "main");

	m_PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase2D->Create(EShaderType::PixelShader, L"Shader\\PSBase2D.hlsl", "main");
	m_PSBase2D->AddConstantBuffer(&m_cbPS2DFlagsData, sizeof(SCBPS2DFlagsData));

	m_PSMasking2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSMasking2D->Create(EShaderType::PixelShader, L"Shader\\PSMasking2D.hlsl", "main");

	m_PSHeightMap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSHeightMap2D->Create(EShaderType::PixelShader, L"Shader\\PSHeightMap2D.hlsl", "main");
}

void CGame::CreateMiniAxes()
{
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisX", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisY", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisZ", m_Device.Get(), m_DeviceContext.Get(), this));

	SMesh Cone{ GenerateCone(0, 1.0f, 1.0f, 16) };
	vector<CMaterial> vMaterials{};
	vMaterials.resize(3);
	vMaterials[0].SetUniformColor(XMFLOAT3(1, 0, 0));
	vMaterials[1].SetUniformColor(XMFLOAT3(0, 1, 0));
	vMaterials[2].SetUniformColor(XMFLOAT3(0, 0, 1));
	m_vObject3DMiniAxes[0]->Create(Cone, vMaterials[0]);
	m_vObject3DMiniAxes[0]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[0]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[0]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vObject3DMiniAxes[0]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[1]->Create(Cone, vMaterials[1]);
	m_vObject3DMiniAxes[1]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[1]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[1]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[2]->Create(Cone, vMaterials[2]);
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

void CGame::CreateBoundingSphere()
{
	m_Object3DBoundingSphere = make_unique<CObject3D>("BoundingSphere", m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DBoundingSphere->Create(GenerateSphere(16));
}

void CGame::CreatePickedTriangle()
{
	m_Object3DPickedTriangle = make_unique<CObject3D>("PickedTriangle", m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DPickedTriangle->Create(GenerateTriangle(XMVectorSet(0, 0, 1.5f, 1), XMVectorSet(+1.0f, 0, 0, 1), XMVectorSet(-1.0f, 0, 0, 1),
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f)));
}

void CGame::Create3DGizmos()
{
	const static XMVECTOR ColorX{ XMVectorSet(1.0f, 0.2f, 0.2f, 1) };
	const static XMVECTOR ColorY{ XMVectorSet(0.2f, 1.0f, 0.2f, 1) };
	const static XMVECTOR ColorZ{ XMVectorSet(0.2f, 0.2f, 1.0f, 1) };

	m_Object3D_3DGizmoRotationPitch = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorX, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationPitch->Create(MeshRing);
		m_Object3D_3DGizmoRotationPitch->ComponentTransform.Roll = -XM_PIDIV2;
		m_Object3D_3DGizmoRotationPitch->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoRotationPitch->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoRotationYaw = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorY, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationYaw->Create(MeshRing);
		m_Object3D_3DGizmoRotationYaw->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoRotationYaw->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoRotationRoll = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorZ, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationRoll->Create(MeshRing);
		m_Object3D_3DGizmoRotationRoll->ComponentTransform.Pitch = XM_PIDIV2;
		m_Object3D_3DGizmoRotationRoll->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoRotationRoll->ComponentRender.PtrPS = m_PSGizmo.get();
	}


	m_Object3D_3DGizmoTranslationX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorX) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationX->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationX->ComponentTransform.Roll = -XM_PIDIV2;
		m_Object3D_3DGizmoTranslationX->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoTranslationX->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoTranslationY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorY) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationY->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationY->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoTranslationY->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoTranslationZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorZ) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationZ->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationZ->ComponentTransform.Pitch = XM_PIDIV2;
		m_Object3D_3DGizmoTranslationZ->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoTranslationZ->ComponentRender.PtrPS = m_PSGizmo.get();
	}


	m_Object3D_3DGizmoScalingX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		SMesh MeshCube{ GenerateCube(ColorX) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingX->Create(MeshAxis);
		m_Object3D_3DGizmoScalingX->ComponentTransform.Roll = -XM_PIDIV2;
		m_Object3D_3DGizmoScalingX->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoScalingX->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoScalingY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		SMesh MeshCube{ GenerateCube(ColorY) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingY->Create(MeshAxis);
		m_Object3D_3DGizmoScalingY->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoScalingY->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoScalingZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		SMesh MeshCube{ GenerateCube(ColorZ) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingZ->Create(MeshAxis);
		m_Object3D_3DGizmoScalingZ->ComponentTransform.Pitch = XM_PIDIV2;
		m_Object3D_3DGizmoScalingZ->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoScalingZ->ComponentRender.PtrPS = m_PSGizmo.get();
	}
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
							auto& Instance{ Object3D->GetInstance(iInstance) };
							for (auto& xmlInstanceChild : vxmlInstanceChildren)
							{
								const char* ID{ xmlInstanceChild->Attribute("ID") };
								if (strcmp(ID, "Translation") == 0)
								{
									float x{ xmlInstanceChild->FloatAttribute("x") };
									float y{ xmlInstanceChild->FloatAttribute("y") };
									float z{ xmlInstanceChild->FloatAttribute("z") };
									Instance.Translation = XMVectorSet(x, y, z, 1.0f);
								}

								if (strcmp(ID, "Rotation") == 0)
								{
									float Pitch{ xmlInstanceChild->FloatAttribute("Pitch") };
									float Yaw{ xmlInstanceChild->FloatAttribute("Yaw") };
									float Roll{ xmlInstanceChild->FloatAttribute("Roll") };
									Instance.Pitch = Pitch;
									Instance.Yaw = Yaw;
									Instance.Roll = Roll;
								}

								if (strcmp(ID, "Scaling") == 0)
								{
									float x{ xmlInstanceChild->FloatAttribute("x") };
									float y{ xmlInstanceChild->FloatAttribute("y") };
									float z{ xmlInstanceChild->FloatAttribute("z") };
									Instance.Scaling = XMVectorSet(x, y, z, 1.0f);
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
			if (SkyFileName) SetSky(SkyFileName, ScalingFactor);
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

								const auto& Instance{ Object3D->GetInstance(InstancePair.first) };

								XMLElement* xmlTranslation{ xmlDocument.NewElement("Translation") };
								{
									xmlTranslation->SetAttribute("ID", "Translation");
									XMFLOAT4 Translation{};
									XMStoreFloat4(&Translation, Instance.Translation);
									xmlTranslation->SetAttribute("x", Translation.x);
									xmlTranslation->SetAttribute("y", Translation.y);
									xmlTranslation->SetAttribute("z", Translation.z);
									xmlInstance->InsertEndChild(xmlTranslation);
								}

								XMLElement* xmlRotation{ xmlDocument.NewElement("Rotation") };
								{
									xmlRotation->SetAttribute("ID", "Rotation");
									xmlRotation->SetAttribute("Pitch", Instance.Pitch);
									xmlRotation->SetAttribute("Yaw", Instance.Yaw);
									xmlRotation->SetAttribute("Roll", Instance.Roll);
									xmlInstance->InsertEndChild(xmlRotation);
								}

								XMLElement* xmlScaling{ xmlDocument.NewElement("Scaling") };
								{
									xmlScaling->SetAttribute("ID", "Scaling");
									XMFLOAT4 Scaling{};
									XMStoreFloat4(&Scaling, Instance.Scaling);
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

void CGame::SetPerspective(float FOV, float NearZ, float FarZ)
{
	m_NearZ = NearZ;
	m_FarZ = FarZ;

	m_MatrixProjection = XMMatrixPerspectiveFovLH(FOV, m_WindowSize.x / m_WindowSize.y, m_NearZ, m_FarZ);
}

void CGame::SetGameRenderingFlags(EFlagsRendering Flags)
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
		m_cbPSBaseFlagsData.bUseLighting = TRUE;
	}
}

void CGame::UpdateCBSpace(const XMMATRIX& World)
{
	m_CBSpaceWVPData.World = m_CBSpace2DData.World = XMMatrixTranspose(World);
	m_CBSpaceWVPData.ViewProjection = GetTransposedViewProjectionMatrix();
	m_CBSpaceVPData.ViewProjection = GetTransposedViewProjectionMatrix();

	m_CBSpace2DData.Projection = XMMatrixTranspose(m_MatrixProjection2D);
}

void CGame::UpdateCBAnimationBoneMatrices(const XMMATRIX* const BoneMatrices)
{
	memcpy(m_CBAnimationBonesData.BoneMatrices, BoneMatrices, sizeof(SCBAnimationBonesData));
}

void CGame::UpdateCBAnimationData(const CObject3D::SCBAnimationData& Data)
{
	m_CBAnimationData = Data;
}

void CGame::UpdateCBTerrainData(const CTerrain::SCBTerrainData& Data)
{
	m_CBTerrainData = Data;
}

void CGame::UpdateCBWindData(const CTerrain::SCBWindData& Data)
{
	m_CBWindData = Data;
}

void CGame::UpdateCBTessFactor(float TessFactor)
{
	m_CBTessFactorData.TessFactor = TessFactor;
}

void CGame::UpdateCBDisplacementData(bool bUseDisplacement)
{
	m_CBDisplacementData.bUseDisplacement = bUseDisplacement;

	m_DSTerrain->UpdateAllConstantBuffers();
}

void CGame::UpdateCBMaterial(const CMaterial& Material)
{
	m_CBMaterialData.MaterialAmbient = Material.GetAmbientColor();
	m_CBMaterialData.MaterialDiffuse = Material.GetDiffuseColor();
	m_CBMaterialData.MaterialSpecular = Material.GetSpecularColor();
	m_CBMaterialData.SpecularExponent = Material.GetSpecularExponent();
	m_CBMaterialData.SpecularIntensity = Material.GetSpecularIntensity();

	m_CBMaterialData.bHasDiffuseTexture = Material.HasTexture(CMaterial::CTexture::EType::DiffuseTexture);
	m_CBMaterialData.bHasNormalTexture = Material.HasTexture(CMaterial::CTexture::EType::NormalTexture);
	m_CBMaterialData.bHasOpacityTexture = Material.HasTexture(CMaterial::CTexture::EType::OpacityTexture);

	m_PSBase->UpdateConstantBuffer(2);
	m_PSFoliage->UpdateConstantBuffer(2);
}

void CGame::UpdateCBTerrainMaskingSpace(const XMMATRIX& Matrix)
{
	m_CBTerrainMaskingSpaceData.Matrix = XMMatrixTranspose(Matrix);
}

void CGame::UpdateCBTerrainSelection(const CTerrain::SCBTerrainSelectionData& Selection)
{
	m_CBTerrainSelectionData = Selection;
}

void CGame::SetSky(const string& SkyDataFileName, float ScalingFactor)
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

	m_SkyMaterial.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, m_SkyData.TextureFileName);

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom), m_SkyMaterial);
	m_Object3DSkySphere->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
	m_Object3DSkySphere->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSkySphere->ComponentRender.PtrPS = m_PSSky.get();
	m_Object3DSkySphere->ComponentPhysics.bIsPickable = false;
	m_Object3DSkySphere->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;

	m_Object3DSun = make_unique<CObject3D>("Sun", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSun->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterial);
	m_Object3DSun->UpdateQuadUV(m_SkyData.Sun.UVOffset, m_SkyData.Sun.UVSize);
	m_Object3DSun->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Sun.WidthHeightRatio, 0);
	m_Object3DSun->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSun->ComponentRender.PtrPS = m_PSBase.get();
	m_Object3DSun->ComponentRender.bIsTransparent = true;
	m_Object3DSun->ComponentPhysics.bIsPickable = false;
	m_Object3DSun->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	
	m_Object3DMoon = make_unique<CObject3D>("Moon", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DMoon->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterial);
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

	return;
}

void CGame::UnsetSky()
{
	m_SkyData.bIsDataSet = false;
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

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color)
{
	m_CBLightData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	m_CBLightData.DirectionalLightColor = Color;
}

void CGame::SetDirectionalLightDirection(const XMVECTOR& LightSourcePosition)
{
	m_CBLightData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
}

void CGame::SetDirectionalLightColor(const XMVECTOR& Color)
{
	m_CBLightData.DirectionalLightColor = Color;
}

const XMVECTOR& CGame::GetDirectionalLightDirection() const
{
	return m_CBLightData.DirectionalLightDirection;
}

const XMVECTOR& CGame::GetDirectionalLightColor() const
{
	return m_CBLightData.DirectionalLightColor;
}

void CGame::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	m_CBLightData.AmbientLightColor = Color;
	m_CBLightData.AmbientLightIntensity = Intensity;
}

const XMFLOAT3& CGame::GetAmbientLightColor() const
{
	return m_CBLightData.AmbientLightColor;
}

float CGame::GetAmbientLightIntensity() const
{
	return m_CBLightData.AmbientLightIntensity;
}

void CGame::CreateTerrain(const XMFLOAT2& TerrainSize, const CMaterial& Material, uint32_t MaskingDetail, float UniformScaling)
{
	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Create(TerrainSize, Material, MaskingDetail, UniformScaling);
	
	ID3D11ShaderResourceView* NullSRVs[11]{};
	m_DeviceContext->DSSetShaderResources(0, 1, NullSRVs);
	m_DeviceContext->PSSetShaderResources(0, 11, NullSRVs);
}

void CGame::LoadTerrain(const string& TerrainFileName)
{
	if (TerrainFileName.empty()) return;

	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Load(TerrainFileName);
	
	ClearMaterials();
	int MaterialCount{ m_Terrain->GetMaterialCount() };
	for (int iMaterial = 0; iMaterial < MaterialCount; ++iMaterial)
	{
		const CMaterial& Material{ m_Terrain->GetMaterial(iMaterial) };

		AddMaterial(Material);
	}
}

void CGame::SaveTerrain(const string& TerrainFileName)
{
	if (!m_Terrain) return;
	if (TerrainFileName.empty()) return;

	m_Terrain->Save(TerrainFileName);
}

void CGame::AddTerrainMaterial(const CMaterial& Material)
{
	if (!m_Terrain) return;

	m_Terrain->AddMaterial(Material);
}

void CGame::SetTerrainMaterial(int MaterialID, const CMaterial& Material)
{
	if (!m_Terrain) return;

	m_Terrain->SetMaterial(MaterialID, Material);
}

CCamera* CGame::AddCamera(const CCamera::SCameraData& CameraData)
{
	m_vCameras.emplace_back(CameraData);

	return &m_vCameras.back();
}

CCamera* CGame::GetCamera(size_t Index)
{
	assert(Index < m_vCameras.size());
	return &m_vCameras[Index];
}

size_t CGame::GetCameraCount() const
{
	return m_vCameras.size();
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
	case EBaseShader::VSBase2D:
		Result = m_VSBase2D.get();
		break;
	case EBaseShader::HSTerrain:
		Result = m_HSTerrain.get();
		break;
	case EBaseShader::HSWater:
		Result = m_HSWater.get();
		break;
	case EBaseShader::DSTerrain:
		Result = m_DSTerrain.get();
		break;
	case EBaseShader::DSWater:
		Result = m_DSWater.get();
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
	case EBaseShader::PSSky:
		Result = m_PSSky.get();
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
	case EBaseShader::PSBase2D:
		Result = m_PSBase2D.get();
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

void CGame::InsertObject3D(const string& Name)
{
	if (m_mapObject3DNameToIndex.find(Name) != m_mapObject3DNameToIndex.end())
	{
		MB_WARN(("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "Object3D 생성 실패");
		return;
	}

	if (Name.size() >= KObject3DNameMaxLength)
	{
		MB_WARN(("이름이 너무 깁니다. (" + Name + ")").c_str(), "Object3D 생성 실패");
		return;
	}
	else if (Name.size() == 0)
	{
		MB_WARN("이름은 공백일 수 없습니다.", "Object3D 생성 실패");
		return;
	}

	m_vObject3Ds.emplace_back(make_unique<CObject3D>(Name, m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3Ds.back()->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3Ds.back()->ComponentRender.PtrPS = m_PSBase.get();

	m_mapObject3DNameToIndex[Name] = m_vObject3Ds.size() - 1;
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

	m_vObject3Ds.back().release();
	m_vObject3Ds.pop_back();
	m_mapObject3DNameToIndex.erase(Name);
}

void CGame::ClearObject3Ds()
{
	m_mapObject3DNameToIndex.clear();
	m_vObject3Ds.clear();

	m_PtrSelectedObject3D = nullptr;
}

CObject3D* CGame::GetObject3D(const string& Name) const
{
	if (m_mapObject3DNameToIndex.find(Name) == m_mapObject3DNameToIndex.end())
	{
		MB_WARN(("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "Object3D 얻어오기 실패");
		return nullptr;
	}
	return m_vObject3Ds[m_mapObject3DNameToIndex.at(Name)].get();
}

void CGame::InsertObject3DLine(const string& Name)
{
	assert(m_umapObject3DLineNameToIndex.find(Name) == m_umapObject3DLineNameToIndex.end());

	m_vObject3DLines.emplace_back(make_unique<CObject3DLine>(Name, m_Device.Get(), m_DeviceContext.Get()));
	
	m_umapObject3DLineNameToIndex[Name] = m_vObject3DLines.size() - 1;
}

CObject3DLine* CGame::GetObject3DLine(const string& Name) const
{
	assert(m_umapObject3DLineNameToIndex.find(Name) != m_umapObject3DLineNameToIndex.end());
	return m_vObject3DLines[m_umapObject3DLineNameToIndex.at(Name)].get();
}

void CGame::InsertObject2D(const string& Name)
{
	assert(m_umapObject2DNameToIndex.find(Name) == m_umapObject2DNameToIndex.end());
	m_vObject2Ds.emplace_back(make_unique<CObject2D>(Name, m_Device.Get(), m_DeviceContext.Get()));
	
	m_umapObject2DNameToIndex[Name] = m_vObject2Ds.size() - 1;
}

CObject2D* CGame::GetObject2D(const string& Name) const
{
	assert(m_umapObject2DNameToIndex.find(Name) != m_umapObject2DNameToIndex.end());
	return m_vObject2Ds[m_umapObject2DNameToIndex.at(Name)].get();
}

CMaterial* CGame::AddMaterial(const CMaterial& Material)
{
	if (m_mapMaterialNameToIndex.find(Material.GetName()) != m_mapMaterialNameToIndex.end()) return nullptr;

	m_vMaterials.emplace_back(make_unique<CMaterial>(Material));
	
	m_vMaterialDiffuseTextures.resize(m_vMaterials.size());
	m_vMaterialNormalTextures.resize(m_vMaterials.size());
	m_vMaterialDisplacementTextures.resize(m_vMaterials.size());

	m_mapMaterialNameToIndex[Material.GetName()] = m_vMaterials.size() - 1;

	ReloadMaterial(Material.GetName());

	return m_vMaterials.back().get();
}

CMaterial* CGame::GetMaterial(const string& Name) const
{
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end()) return nullptr;

	return m_vMaterials[m_mapMaterialNameToIndex.at(Name)].get();
}

void CGame::ClearMaterials()
{
	m_vMaterials.clear();
	m_vMaterialDiffuseTextures.clear();
	m_vMaterialNormalTextures.clear();
	m_mapMaterialNameToIndex.clear();
}

size_t CGame::GetMaterialCount() const
{
	return m_vMaterials.size();
}

void CGame::ChangeMaterialName(const string& OldName, const string& NewName)
{
	size_t iMaterial{ m_mapMaterialNameToIndex[OldName] };
	CMaterial* Material{ m_vMaterials[iMaterial].get() };
	auto a =m_mapMaterialNameToIndex.find(OldName);
	
	m_mapMaterialNameToIndex.erase(OldName);
	m_mapMaterialNameToIndex.insert(make_pair(NewName, iMaterial));

	Material->SetName(NewName);
}

void CGame::ReloadMaterial(const string& Name)
{
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end()) return;

	size_t iMaterial{ m_mapMaterialNameToIndex[Name] };
	CMaterial* Material{ m_vMaterials[iMaterial].get() };
	if (!Material->HasTexture()) return;

	CreateMaterialTexture(CMaterial::CTexture::EType::DiffuseTexture, *Material);
	CreateMaterialTexture(CMaterial::CTexture::EType::NormalTexture, *Material);
	CreateMaterialTexture(CMaterial::CTexture::EType::DisplacementTexture, *Material);
	CreateMaterialTexture(CMaterial::CTexture::EType::OpacityTexture, *Material);
}

void CGame::CreateMaterialTexture(CMaterial::CTexture::EType eType, CMaterial& Material)
{
	if (Material.HasTexture(eType))
	{
		CMaterial::CTexture* PtrTexture{};
		size_t iTexture{};

		switch (eType)
		{
		case CMaterial::CTexture::EType::DiffuseTexture:
			m_vMaterialDiffuseTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialDiffuseTextures.back().get();
			iTexture = m_vMaterialDiffuseTextures.size() - 1;
			break;
		case CMaterial::CTexture::EType::NormalTexture:
			m_vMaterialNormalTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialNormalTextures.back().get();
			iTexture = m_vMaterialNormalTextures.size() - 1;
			break;
		case CMaterial::CTexture::EType::DisplacementTexture:
			m_vMaterialDisplacementTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialDisplacementTextures.back().get();
			iTexture = m_vMaterialDisplacementTextures.size() - 1;
			break;
		case CMaterial::CTexture::EType::OpacityTexture:
			m_vMaterialOpacityTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialOpacityTextures.back().get();
			iTexture = m_vMaterialOpacityTextures.size() - 1;
			break;
		default:
			break;
		}

		if (Material.IsTextureEmbedded(eType))
		{
			PtrTexture->CreateTextureFromMemory(Material.GetTextureRawData(eType), Material.ShouldGenerateAutoMipMap());
			Material.ClearEmbeddedTextureData(eType);
		}
		else
		{
			PtrTexture->CreateTextureFromFile(Material.GetTextureFileName(eType), Material.ShouldGenerateAutoMipMap());
		}
	}
}

CMaterial::CTexture* CGame::GetMaterialTexture(CMaterial::CTexture::EType eType, const string& Name) const
{
	assert(m_mapMaterialNameToIndex.find(Name) != m_mapMaterialNameToIndex.end());
	size_t iMaterial{ m_mapMaterialNameToIndex.at(Name) };

	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		if (m_vMaterialDiffuseTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialDiffuseTextures[iMaterial].get();
	case CMaterial::CTexture::EType::NormalTexture:
		if (m_vMaterialNormalTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialNormalTextures[iMaterial].get();
	case CMaterial::CTexture::EType::DisplacementTexture:
		if (m_vMaterialDisplacementTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialDisplacementTextures[iMaterial].get();
	case CMaterial::CTexture::EType::OpacityTexture:
		if (m_vMaterialOpacityTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialOpacityTextures[iMaterial].get();
	default:
		return nullptr;
	}
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
		DeselectObject3D();
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
		DeselectObject3D();
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
	if (m_PtrPickedObject3D)
	{
		return m_PtrPickedObject3D->GetName();
	}
	return m_NullString;
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
		if (!m_PtrSelectedObject3D->IsInstanced())
		{
			m_SelectedInstanceID = -1;
			Interact3DGizmos();
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
	if (m_PtrSelectedObject3D)
	{
		return m_PtrSelectedObject3D->GetName();
	}
	return m_NullString;
}

void CGame::SelectInstance(int InstanceID)
{
	if (m_eEditMode != EEditMode::EditObject) return;

	m_SelectedInstanceID = InstanceID;

	if (IsAnyObject3DSelected() && IsAnyInstanceSelected())
	{
		Interact3DGizmos();
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

void CGame::Interact3DGizmos()
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

	const Mouse::State& MouseState{ m_Mouse->GetState() };
	XMVECTOR* pTranslation{ &m_PtrSelectedObject3D->ComponentTransform.Translation };
	XMVECTOR* pScaling{ &m_PtrSelectedObject3D->ComponentTransform.Scaling };
	float* pPitch{ &m_PtrSelectedObject3D->ComponentTransform.Pitch };
	float* pYaw{ &m_PtrSelectedObject3D->ComponentTransform.Yaw };
	float* pRoll{ &m_PtrSelectedObject3D->ComponentTransform.Roll };
	if (m_PtrSelectedObject3D->IsInstanced() && IsAnyInstanceSelected())
	{
		auto& Instance{ m_PtrSelectedObject3D->GetInstance(m_SelectedInstanceID) };
		pTranslation = &Instance.Translation;
		pScaling = &Instance.Scaling;
		pPitch = &Instance.Pitch;
		pYaw = &Instance.Yaw;
		pRoll = &Instance.Roll;
	}

	if (!MouseState.leftButton)
	{
		m_bIsGizmoSelected = false;
	}

	m_3DGizmoDistanceScalar = XMVectorGetX(XMVector3Length(m_vCameras[m_CurrentCameraIndex].GetEyePosition() - *pTranslation)) * 0.1f;
	m_3DGizmoDistanceScalar = pow(m_3DGizmoDistanceScalar, 0.7f);

	if (m_bIsGizmoSelected)
	{
		// Gizmo is selected.

		int DeltaX{ MouseState.x - m_PrevMouseX };
		int DeltaY{ MouseState.y - m_PrevMouseY };

		int DeltaSum{ DeltaY - DeltaX };

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
				*pTranslation =
					XMVectorSetX(*pTranslation, TranslationX - DeltaSum * CGame::KTranslationUnit);
				break;
			case E3DGizmoAxis::AxisY:
				*pTranslation =
					XMVectorSetY(*pTranslation, TranslationY - DeltaSum * CGame::KTranslationUnit);
				break;
			case E3DGizmoAxis::AxisZ:
				*pTranslation =
					XMVectorSetZ(*pTranslation, TranslationZ - DeltaSum * CGame::KTranslationUnit);
				break;
			default:
				break;
			}
			break;
		case E3DGizmoMode::Rotation:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pPitch -= DeltaSum * CGame::KRotation360To2PI;
				break;
			case E3DGizmoAxis::AxisY:
				*pYaw -= DeltaSum * CGame::KRotation360To2PI;
				break;
			case E3DGizmoAxis::AxisZ:
				*pRoll -= DeltaSum * CGame::KRotation360To2PI;
				break;
			default:
				break;
			}
			break;
		case E3DGizmoMode::Scaling:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pScaling =
					XMVectorSetX(*pScaling, ScalingX - DeltaSum * CGame::KScalingUnit);
				break;
			case E3DGizmoAxis::AxisY:
				*pScaling =
					XMVectorSetY(*pScaling, ScalingY - DeltaSum * CGame::KScalingUnit);
				break;
			case E3DGizmoAxis::AxisZ:
				*pScaling =
					XMVectorSetZ(*pScaling, ScalingZ - DeltaSum * CGame::KScalingUnit);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		m_PtrSelectedObject3D->UpdateInstanceWorldMatrix(m_SelectedInstanceID);
	}
	else
	{
		// Gizmo is not selected.

		CastPickingRay();

		const XMVECTOR& BSTransaltion{ m_PtrSelectedObject3D->ComponentPhysics.BoundingSphere.CenterOffset };

		m_Object3D_3DGizmoTranslationX->ComponentTransform.Translation =
			m_Object3D_3DGizmoTranslationY->ComponentTransform.Translation =
			m_Object3D_3DGizmoTranslationZ->ComponentTransform.Translation = *pTranslation + BSTransaltion;

		m_Object3D_3DGizmoRotationPitch->ComponentTransform.Translation =
			m_Object3D_3DGizmoRotationYaw->ComponentTransform.Translation =
			m_Object3D_3DGizmoRotationRoll->ComponentTransform.Translation = *pTranslation + BSTransaltion;

		m_Object3D_3DGizmoScalingX->ComponentTransform.Translation =
			m_Object3D_3DGizmoScalingY->ComponentTransform.Translation =
			m_Object3D_3DGizmoScalingZ->ComponentTransform.Translation = *pTranslation + BSTransaltion;

		switch (m_e3DGizmoMode)
		{
		case E3DGizmoMode::Translation:
			m_bIsGizmoSelected = true;
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
				m_bIsGizmoSelected = false;
			}

			break;
		case E3DGizmoMode::Rotation:
			m_bIsGizmoSelected = true;
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
				m_bIsGizmoSelected = false;
			}

			break;
		case E3DGizmoMode::Scaling:
			m_bIsGizmoSelected = true;
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
				m_bIsGizmoSelected = false;
			}

			break;
		default:
			break;
		}

	}

	m_PrevMouseX = MouseState.x;
	m_PrevMouseY = MouseState.y;
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
	XMVECTOR Center{};
	switch (Axis)
	{
	case E3DGizmoAxis::None:
		return false;
		break;
	case E3DGizmoAxis::AxisX:
		Center = Gizmo->ComponentTransform.Translation + XMVectorSet(0.5f * m_3DGizmoDistanceScalar, 0, 0, 1);
		break;
	case E3DGizmoAxis::AxisY:
		Center = Gizmo->ComponentTransform.Translation + XMVectorSet(0, 0.5f * m_3DGizmoDistanceScalar, 0, 1);
		break;
	case E3DGizmoAxis::AxisZ:
		Center = Gizmo->ComponentTransform.Translation + XMVectorSet(0, 0, 0.5f * m_3DGizmoDistanceScalar, 1);
		break;
	default:
		break;
	}

	if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, 0.5f * m_3DGizmoDistanceScalar, Center, nullptr)) return true;

	return false;
}

void CGame::CastPickingRay()
{
	const Mouse::State& MouseState{ m_Mouse->GetState() };

	float ViewSpaceRayDirectionX{ (MouseState.x / (m_WindowSize.x / 2.0f) - 1.0f) / XMVectorGetX(m_MatrixProjection.r[0]) };
	float ViewSpaceRayDirectionY{ (-(MouseState.y / (m_WindowSize.y / 2.0f) - 1.0f)) / XMVectorGetY(m_MatrixProjection.r[1]) };
	static float ViewSpaceRayDirectionZ{ 1.0f };

	static XMVECTOR ViewSpaceRayOrigin{ XMVectorSet(0, 0, 0, 1) };
	XMVECTOR ViewSpaceRayDirection{ XMVectorSet(ViewSpaceRayDirectionX, ViewSpaceRayDirectionY, ViewSpaceRayDirectionZ, 0) };

	XMMATRIX MatrixViewInverse{ XMMatrixInverse(nullptr, m_MatrixView) };
	m_PickingRayWorldSpaceOrigin = XMVector3TransformCoord(ViewSpaceRayOrigin, MatrixViewInverse);
	m_PickingRayWorldSpaceDirection = XMVector3TransformNormal(ViewSpaceRayDirection, MatrixViewInverse);
}

void CGame::PickBoundingSphere()
{
	m_PtrPickedObject3D = nullptr;
	m_PickedInstanceID = -1;

	XMVECTOR T{ KVectorGreatest };
	for (auto& i : m_vObject3Ds)
	{
		auto* Object3D{ i.get() };
		if (Object3D->ComponentPhysics.bIsPickable)
		{
			if (Object3D->ComponentPhysics.bIgnoreBoundingSphere)
			{
				m_PtrPickedObject3D = Object3D;

				if (PickTriangle())
				{
					break;
				}
				else
				{
					m_PtrPickedObject3D = nullptr;
				}
			}
			else
			{
				if (Object3D->IsInstanced())
				{
					int InstanceCount{ (int)Object3D->GetInstanceCount() };
					for (int iInstance = 0; iInstance < InstanceCount; ++iInstance)
					{
						auto& Instance{ Object3D->GetInstance(iInstance) };
						XMVECTOR NewT{ KVectorGreatest };
						if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
							Object3D->ComponentPhysics.BoundingSphere.Radius, Instance.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
						{
							if (XMVector3Less(NewT, T))
							{
								T = NewT;
								m_PtrPickedObject3D = Object3D;
								m_PickedInstanceID = iInstance;
							}
						}
					}
				}
				else
				{
					XMVECTOR NewT{ KVectorGreatest };
					if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
						Object3D->ComponentPhysics.BoundingSphere.Radius, Object3D->ComponentTransform.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
					{
						if (XMVector3Less(NewT, T))
						{
							T = NewT;
							m_PtrPickedObject3D = Object3D;
						}
					}
				}
			}
		}
	}
}

bool CGame::PickTriangle()
{
	XMVECTOR T{ KVectorGreatest };
	if (m_PtrPickedObject3D)
	{
		// Pick only static models' triangle.
		if (m_PtrPickedObject3D->GetModel().bIsModelAnimated) return false;

		const XMMATRIX& WorldMatrix{ m_PtrPickedObject3D->ComponentTransform.MatrixWorld };
		for (const SMesh& Mesh : m_PtrPickedObject3D->GetModel().vMeshes)
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

						XMVECTOR N{ CalculateTriangleNormal(V0, V1, V2) };

						m_PickedTriangleV0 = V0 + N * 0.01f;
						m_PickedTriangleV1 = V1 + N * 0.01f;
						m_PickedTriangleV2 = V2 + N * 0.01f;

						return true;
					}
				}
			}
		}
	}
	return false;
}

void CGame::BeginRendering(const FLOAT* ClearColor)
{
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), Colors::CornflowerBlue);
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ID3D11SamplerState* SamplerState{ m_CommonStates->LinearWrap() };
	m_DeviceContext->PSSetSamplers(0, 1, &SamplerState);
	m_DeviceContext->DSSetSamplers(0, 1, &SamplerState); // @important: in order to use displacement mapping

	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

	SetUniversalRSState();

	m_MatrixView = XMMatrixLookAtLH(m_vCameras[m_CurrentCameraIndex].GetEyePosition(), 
		m_vCameras[m_CurrentCameraIndex].GetFocusPosition(),
		m_vCameras[m_CurrentCameraIndex].GetUpDirection());
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

	// Check if CGame has at least one camera
	if (GetCameraCount() == 0)
	{
		CCamera* MainCamera{ AddCamera(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0))) };
		MainCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));
	}
	CCamera* const MainCamera{ GetCamera(0) };

	// Process keyboard inputs
	if (m_CapturedKeyboardState.LeftAlt && m_CapturedKeyboardState.Q)
	{
		Destroy();
		return;
	}
	if (m_CapturedKeyboardState.Escape)
	{
		DeselectObject3D();
	}
	if (!ImGui::IsAnyItemActive())
	{
		if (m_CapturedKeyboardState.W)
		{
			MainCamera->Move(CCamera::EMovementDirection::Forward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.S)
		{
			MainCamera->Move(CCamera::EMovementDirection::Backward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.A)
		{
			MainCamera->Move(CCamera::EMovementDirection::Leftward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.D)
		{
			MainCamera->Move(CCamera::EMovementDirection::Rightward, m_DeltaTimeF * m_CameraMovementFactor);
		}
		if (m_CapturedKeyboardState.D1)
		{
			Set3DGizmoMode(CGame::E3DGizmoMode::Translation);
		}
		if (m_CapturedKeyboardState.D2)
		{
			Set3DGizmoMode(CGame::E3DGizmoMode::Rotation);
		}
		if (m_CapturedKeyboardState.D3)
		{
			Set3DGizmoMode(CGame::E3DGizmoMode::Scaling);
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
			Interact3DGizmos();

			if (m_bLeftButtonPressedOnce)
			{
				if (Pick())
				{
					SelectObject3D(GetPickedObject3DName());
					SelectInstance(GetPickedInstanceID());
				}
				m_bLeftButtonPressedOnce = false;
			}

			if (m_CapturedMouseState.rightButton)
			{
				DeselectObject3D();
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
			{
				MainCamera->Rotate(m_CapturedMouseState.x - PrevMouseX, m_CapturedMouseState.y - PrevMouseY, m_DeltaTimeF);
			}

			PrevMouseX = m_CapturedMouseState.x;
			PrevMouseY = m_CapturedMouseState.y;
		}
	}

	// Animate Object3Ds
	for (auto& Object3D : m_vObject3Ds)
	{
		if (Object3D) Object3D->Animate(m_DeltaTimeF);
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

	m_CBWaterTimeData.Time += m_DeltaTimeF * 0.1f;
	if (m_CBWaterTimeData.Time > 1.0f) m_CBWaterTimeData.Time = 0.0f;

	m_CBTerrainData.Time = m_CBEditorTimeData.NormalizedTime;

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	m_CBLightData.EyePosition = m_vCameras[m_CurrentCameraIndex].GetEyePosition();

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

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::Use3DGizmos))
		{
			Draw3DGizmos();
		}

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
			m_GSNormal->Use();
			m_GSNormal->UpdateAllConstantBuffers();
		}
	}

	for (auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->ComponentRender.bIsTransparent) continue;

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

		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
		{
			DrawObject3DBoundingSphere(Object3D.get());
		}
	}

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);

	DrawObject2Ds();

	if (m_eMode != EMode::Play)
	{
		DrawImGui();
	}
}

void CGame::UpdateObject3D(CObject3D* const PtrObject3D)
{
	if (!PtrObject3D) return;

	PtrObject3D->UpdateWorldMatrix();
	UpdateCBSpace(PtrObject3D->ComponentTransform.MatrixWorld);

	SetUniversalbUseLighiting();
	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoLighting))
	{
		m_cbPSBaseFlagsData.bUseLighting = FALSE;
	}

	m_cbPSBaseFlagsData.bUseTexture = TRUE;
	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoTexture))
	{
		m_cbPSBaseFlagsData.bUseTexture = FALSE;
	}

	assert(PtrObject3D->ComponentRender.PtrVS);
	assert(PtrObject3D->ComponentRender.PtrPS);
	CShader* VS{ PtrObject3D->ComponentRender.PtrVS };
	CShader* PS{ PtrObject3D->ComponentRender.PtrPS };
	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::UseRawVertexColor))
	{
		PS = m_PSVertexColor.get();
	}

	VS->Use();
	VS->UpdateAllConstantBuffers();
	PS->Use();
	PS->UpdateAllConstantBuffers();
}

void CGame::DrawObject3D(const CObject3D* const PtrObject3D)
{
	if (!PtrObject3D) return;

	if (PtrObject3D->ShouldTessellate())
	{
		UpdateCBSpace();
		m_CBCameraData.EyePosition = m_vCameras[m_CurrentCameraIndex].GetEyePosition();

		m_HSTerrain->UpdateAllConstantBuffers();
		m_HSTerrain->Use();
		m_DSTerrain->UpdateAllConstantBuffers();
		m_DSTerrain->Use();
	}

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRSState();
	}

	PtrObject3D->Draw();

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
	m_VSBase->UpdateConstantBuffer(0);

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
			m_VSLine->UpdateConstantBuffer(0);

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
		m_VSBase2D->UpdateConstantBuffer(0);

		if (Object2D->HasTexture())
		{
			m_cbPS2DFlagsData.bUseTexture = TRUE;
			m_PSBase2D->UpdateConstantBuffer(0);
		}
		else
		{
			m_cbPS2DFlagsData.bUseTexture = FALSE;
			m_PSBase2D->UpdateConstantBuffer(0);
		}

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

		Object3D->ComponentTransform.Translation = 
			m_vCameras[m_CurrentCameraIndex].GetEyePosition() +
			m_vCameras[m_CurrentCameraIndex].GetForward();

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
	m_VSLine->UpdateConstantBuffer(0);

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSLine->Use();

	m_Object3DLinePickingRay->Draw();
}

void CGame::DrawPickedTriangle()
{
	m_VSBase->Use();
	m_CBSpaceWVPData.World = XMMatrixTranspose(KMatrixIdentity);
	m_CBSpaceWVPData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	m_VSBase->UpdateConstantBuffer(0);

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
		m_Object3DSkySphere->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].GetEyePosition();

		UpdateObject3D(m_Object3DSkySphere.get());
		DrawObject3D(m_Object3DSkySphere.get());
	}

	// Sun
	{
		float SunRoll{ XM_2PI * m_CBSkyTimeData.SkyTime - XM_PIDIV2 };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, SunRoll)) };
		m_Object3DSun->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].GetEyePosition() + Offset;
		m_Object3DSun->ComponentTransform.Roll = SunRoll;

		UpdateObject3D(m_Object3DSun.get());
		DrawObject3D(m_Object3DSun.get());
	}

	// Moon
	{
		float MoonRoll{ XM_2PI * m_CBSkyTimeData.SkyTime + XM_PIDIV2 };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, MoonRoll)) };
		m_Object3DMoon->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].GetEyePosition() + Offset;
		m_Object3DMoon->ComponentTransform.Roll = (MoonRoll > XM_2PI) ? (MoonRoll - XM_2PI) : MoonRoll;

		UpdateObject3D(m_Object3DMoon.get());
		DrawObject3D(m_Object3DMoon.get());
	}
}

void CGame::DrawTerrain(float DeltaTime)
{
	if (!m_Terrain) return;
	
	SetUniversalRSState();

	SetUniversalbUseLighiting();

	m_Terrain->UpdateWind(DeltaTime);

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::TessellateTerrain))
	{
		m_CBCameraData.EyePosition = m_vCameras[m_CurrentCameraIndex].GetEyePosition();
		m_CBTessFactorData.TessFactor = m_Terrain->GetTerrainTessFactor();
		m_HSTerrain->UpdateAllConstantBuffers();
		m_HSTerrain->Use();

		m_CBSpaceVPData.ViewProjection = GetTransposedViewProjectionMatrix();
		m_DSTerrain->UpdateAllConstantBuffers();
		m_DSTerrain->Use();

		m_Terrain->Draw(EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals) && (m_eMode == EMode::Edit));

		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	}
	else
	{
		m_Terrain->Draw(EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals) && (m_eMode == EMode::Edit));
	}

	if (m_eMode == EMode::Edit)
	{
		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainHeightMapTexture))
		{
			m_DeviceContext->RSSetViewports(1, &m_vViewports[2]);
			m_Terrain->DrawHeightMapTexture();
		}

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainMaskingTexture))
		{
			m_DeviceContext->RSSetViewports(1, &m_vViewports[3]);
			m_Terrain->DrawMaskingTexture();
		}

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainFoliagePlacingTexture))
		{
			m_DeviceContext->RSSetViewports(1, &m_vViewports[4]);
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

void CGame::Draw3DGizmoRotations(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

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

	Draw3DGizmo(m_Object3D_3DGizmoRotationPitch.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoRotationYaw.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoRotationRoll.get(), bHighlightZ);
}

void CGame::Draw3DGizmoTranslations(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

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

	Draw3DGizmo(m_Object3D_3DGizmoTranslationX.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoTranslationY.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoTranslationZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmoScalings(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

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

	Draw3DGizmo(m_Object3D_3DGizmoScalingX.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoScalingY.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoScalingZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmo(CObject3D* const Gizmo, bool bShouldHighlight)
{
	CShader* VS{ Gizmo->ComponentRender.PtrVS };
	CShader* PS{ Gizmo->ComponentRender.PtrPS };

	float Scalar{ XMVectorGetX(XMVector3Length(m_vCameras[m_CurrentCameraIndex].GetEyePosition() - Gizmo->ComponentTransform.Translation)) * 0.1f };
	Scalar = pow(Scalar, 0.7f);

	Gizmo->ComponentTransform.Scaling = XMVectorSet(Scalar, Scalar, Scalar, 0.0f);
	Gizmo->UpdateWorldMatrix();
	m_CBSpaceWVPData.World = XMMatrixTranspose(Gizmo->ComponentTransform.MatrixWorld);
	m_CBSpaceWVPData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	VS->UpdateConstantBuffer(0);
	VS->Use();

	if (bShouldHighlight)
	{
		m_CBGizmoColorFactorData.ColorFactor = XMVectorSet(1.25f, 1.25f, 1.25f, 0.95f);
	}
	else
	{
		m_CBGizmoColorFactorData.ColorFactor = XMVectorSet(0.75f, 0.75f, 0.75f, 0.75f);
	}
	PS->UpdateConstantBuffer(0);
	PS->Use();

	Gizmo->Draw();
}

void CGame::DrawImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushFont(m_ImGuiFont);

	if (m_eMode == CGame::EMode::Edit)
	{
		static bool bShowPropertyEditor{ true };
		static bool bShowSceneEditor{ true };
		static bool bShowTerrainGenerator{ false };
		static bool bShowOpenFileDialog{ false };
		static bool bShowSaveFileDialog{ false };

		// ### 메뉴 바 ###
		if (ImGui::BeginMainMenuBar())
		{
			if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.N) bShowTerrainGenerator = true;
			if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.O) bShowOpenFileDialog = true;
			if (m_CapturedKeyboardState.LeftControl && m_CapturedKeyboardState.S) bShowSaveFileDialog = true;

			if (ImGui::BeginMenu(u8"지형"))
			{
				if (ImGui::MenuItem(u8"만들기", "Ctrl+N", nullptr)) bShowTerrainGenerator = true;
				if (ImGui::MenuItem(u8"불러오기", "Ctrl+O", nullptr)) bShowOpenFileDialog = true;
				if (ImGui::MenuItem(u8"내보내기", "Ctrl+S", nullptr)) bShowSaveFileDialog = true;

				ImGui::EndMenu();
			}

			if (bShowOpenFileDialog)
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
				if (FileDialog.OpenFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 불러오기"))
				{
					LoadTerrain(FileDialog.GetRelativeFileName());
				}
				bShowOpenFileDialog = false;
			}

			if (bShowSaveFileDialog)
			{
				if (GetTerrain())
				{
					static CFileDialog FileDialog{ GetWorkingDirectory() };
					if (FileDialog.SaveFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 내보내기", ".terr"))
					{
						SaveTerrain(FileDialog.GetRelativeFileName());
					}
				}

				bShowSaveFileDialog = false;
			}

			if (ImGui::BeginMenu(u8"창"))
			{
				ImGui::MenuItem(u8"속성 편집기", nullptr, &bShowPropertyEditor);
				ImGui::MenuItem(u8"장면 편집기", nullptr, &bShowSceneEditor);

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem(u8"종료", "Alt+Q"))
			{
				Destroy();
				return;
			}

			ImGui::EndMainMenuBar();
		}

		// ### 지형 생성기 윈도우 ###
		if (bShowTerrainGenerator) ImGui::OpenPopup(u8"지형 생성기");
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
				CreateTerrain(TerrainSize, *GetMaterial("DefaultGround"), MaskingDetail, UniformScaling);

				CTerrain* const Terrain{ GetTerrain() };
				SetEditMode(GetEditMode(), true);
				Terrain->SetEditMode(eTerrainEditMode);

				SizeX = CTerrain::KDefaultSize;
				SizeZ = CTerrain::KDefaultSize;
				bShowTerrainGenerator = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
			{
				SizeX = CTerrain::KDefaultSize;
				SizeZ = CTerrain::KDefaultSize;

				bShowTerrainGenerator = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// ### 속성 편집기 윈도우 ###
		if (bShowPropertyEditor)
		{
			static constexpr float KInitialWindowWidth{ 400 };
			ImGui::SetNextWindowPos(ImVec2(m_WindowSize.x - KInitialWindowWidth, 21), ImGuiCond_Appearing);
			ImGui::SetNextWindowSize(ImVec2(KInitialWindowWidth, 0), ImGuiCond_Appearing);
			ImGui::SetNextWindowSizeConstraints(
				ImVec2(m_WindowSize.x * 0.25f, m_WindowSize.y), ImVec2(m_WindowSize.x * 0.5f, m_WindowSize.y));

			if (ImGui::Begin(u8"속성 편집기", &bShowPropertyEditor, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
			{
				float WindowWidth{ ImGui::GetWindowWidth() };

				if (ImGui::BeginTabBar(u8"탭바", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem(u8"오브젝트"))
					{
						static bool bShowAnimationAdder{ false };
						static bool bShowAnimationEditor{ false };
						static int iSelectedAnimationID{};

						SetEditMode(CGame::EEditMode::EditObject);

						static constexpr float KLabelsWidth{ 200 };
						static constexpr float KItemsMaxWidth{ 240 };
						float ItemsWidth{ WindowWidth - KLabelsWidth };
						ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
						float ItemsOffsetX{ WindowWidth - ItemsWidth };

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
										auto& Instance{ Object3D->GetInstance(GetSelectedInstanceID()) };

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"선택된 인스턴스:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"<%s>", Instance.Name.c_str());

										ImGui::Separator();

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"위치");
										ImGui::SameLine(ItemsOffsetX);
										float Translation[3]{ XMVectorGetX(Instance.Translation),
											XMVectorGetY(Instance.Translation), XMVectorGetZ(Instance.Translation) };
										if (ImGui::DragFloat3(u8"##위치", Translation, CGame::KTranslationUnit,
											CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.2f"))
										{
											Instance.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
											Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"회전");
										ImGui::SameLine(ItemsOffsetX);
										int PitchYawRoll360[3]{ (int)(Instance.Pitch * CGame::KRotation2PITo360),
											(int)(Instance.Yaw * CGame::KRotation2PITo360),
											(int)(Instance.Roll * CGame::KRotation2PITo360) };
										if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, CGame::KRotation360Unit,
											CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
										{
											Instance.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
											Instance.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
											Instance.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
											Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"크기");
										ImGui::SameLine(ItemsOffsetX);
										float Scaling[3]{ XMVectorGetX(Instance.Scaling),
											XMVectorGetY(Instance.Scaling), XMVectorGetZ(Instance.Scaling) };
										if (ImGui::DragFloat3(u8"##크기", Scaling, CGame::KScalingUnit,
											CGame::KScalingMinLimit, CGame::KScalingMaxLimit, "%.3f"))
										{
											Instance.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
											Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
										}
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
									if (ImGui::DragFloat3(u8"##위치", Translation, CGame::KTranslationUnit,
										CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.2f"))
									{
										Object3D->ComponentTransform.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
										Object3D->UpdateWorldMatrix();
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"회전");
									ImGui::SameLine(ItemsOffsetX);
									int PitchYawRoll360[3]{ (int)(Object3D->ComponentTransform.Pitch * CGame::KRotation2PITo360),
										(int)(Object3D->ComponentTransform.Yaw * CGame::KRotation2PITo360),
										(int)(Object3D->ComponentTransform.Roll * CGame::KRotation2PITo360) };
									if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, CGame::KRotation360Unit,
										CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
									{
										Object3D->ComponentTransform.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
										Object3D->ComponentTransform.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
										Object3D->ComponentTransform.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
										Object3D->UpdateWorldMatrix();
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"크기");
									ImGui::SameLine(ItemsOffsetX);
									float Scaling[3]{ XMVectorGetX(Object3D->ComponentTransform.Scaling),
										XMVectorGetY(Object3D->ComponentTransform.Scaling), XMVectorGetZ(Object3D->ComponentTransform.Scaling) };
									if (ImGui::DragFloat3(u8"##크기", Scaling, CGame::KScalingUnit,
										CGame::KScalingMinLimit, CGame::KScalingMaxLimit, "%.3f"))
									{
										Object3D->ComponentTransform.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
										Object3D->UpdateWorldMatrix();
									}
								}

								ImGui::Separator();

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"BS 중심");
								ImGui::SameLine(ItemsOffsetX);
								float BSCenterOffset[3]{
									XMVectorGetX(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
									XMVectorGetY(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
									XMVectorGetZ(Object3D->ComponentPhysics.BoundingSphere.CenterOffset) };
								if (ImGui::DragFloat3(u8"##BS중심", BSCenterOffset, CGame::KBSCenterOffsetUnit,
									CGame::KBSCenterOffsetMinLimit, CGame::KBSCenterOffsetMaxLimit, "%.2f"))
								{
									Object3D->ComponentPhysics.BoundingSphere.CenterOffset =
										XMVectorSet(BSCenterOffset[0], BSCenterOffset[1], BSCenterOffset[2], 1.0f);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"BS 반지름");
								ImGui::SameLine(ItemsOffsetX);
								float BSRadius{ Object3D->ComponentPhysics.BoundingSphere.Radius };
								if (ImGui::DragFloat(u8"##BS반지름", &BSRadius, CGame::KBSRadiusUnit,
									CGame::KBSRadiusMinLimit, CGame::KBSRadiusMaxLimit, "%.2f"))
								{
									Object3D->ComponentPhysics.BoundingSphere.Radius = BSRadius;
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

					// 지형 편집기
					static bool bShowFoliageClusterGenerator{ false };
					if (ImGui::BeginTabItem(u8"지형"))
					{
						static constexpr float KLabelsWidth{ 200 };
						static constexpr float KItemsMaxWidth{ 240 };
						float ItemsWidth{ WindowWidth - KLabelsWidth };
						ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
						float ItemsOffsetX{ WindowWidth - ItemsWidth };

						SetEditMode(CGame::EEditMode::EditTerrain);

						CTerrain* const Terrain{ GetTerrain() };
						if (Terrain)
						{
							static const char* const KModeLabelList[4]{
								u8"<높이 지정 모드>", u8"<높이 변경 모드>", u8"<마스킹 모드>", u8"<초목 배치 모드>" };
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
											Terrain->SetEditMode(CTerrain::EEditMode::Masking);
											break;
										case 3:
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
									if (ImGui::DragFloat(u8"##지정할 높이", &TerrainSetHeightValue, CTerrain::KHeightUnit,
										CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
									{
										Terrain->SetSetHeightValue(TerrainSetHeightValue);
									}
								}
								else if (iSelectedMode == 2)
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
								else if (iSelectedMode == 3)
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

									const CMaterial& Material{ Terrain->GetMaterial(iMaterial) };
									ImGui::Text(u8"재질[%d] %s", iMaterial, Material.GetName().c_str());
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
											AddTerrainMaterial(*GetMaterial(*SelectedMaterialName));
										}
										else
										{
											SetTerrainMaterial(iSelectedMaterialID, *GetMaterial(*SelectedMaterialName));
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

					if (ImGui::BeginTabItem(u8"재질"))
					{
						ImGui::PushID(0);
						if (ImGui::Button(u8"새 재질 추가"))
						{
							size_t Count{ GetMaterialCount() };

							CMaterial Material{};
							Material.SetName("Material" + to_string(Count));
							Material.ShouldGenerateAutoMipMap(true);

							AddMaterial(Material);
						}
						ImGui::PopID();

						static constexpr float KUniformWidth{ 180.0f };
						static const char KLabelAdd[]{ u8"추가" };
						static const char KLabelChange[]{ u8"변경" };

						static constexpr size_t KNameMaxLength{ 255 };
						static char OldName[KNameMaxLength]{};
						static char NewName[KNameMaxLength]{};
						static bool bShowMaterialNameChangeWindow{ false };
						const auto& mapMaterialList{ GetMaterialMap() };
						for (auto& pairMaterial : mapMaterialList)
						{
							CMaterial* Material{ GetMaterial(pairMaterial.first) };

							if (ImGui::TreeNodeEx(pairMaterial.first.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
							{
								if (ImGui::Button(u8"이름 변경"))
								{
									strcpy_s(OldName, Material->GetName().c_str());
									strcpy_s(NewName, Material->GetName().c_str());

									bShowMaterialNameChangeWindow = true;
								}

								ImGui::SetNextItemWidth(KUniformWidth);
								XMFLOAT3 AmbientColor{ Material->GetAmbientColor() };
								if (ImGui::ColorEdit3(u8"환경광(Ambient)", &AmbientColor.x, ImGuiColorEditFlags_RGB))
								{
									Material->SetAmbientColor(AmbientColor);
								}

								ImGui::SetNextItemWidth(KUniformWidth);
								XMFLOAT3 DiffuseColor{ Material->GetDiffuseColor() };
								if (ImGui::ColorEdit3(u8"난반사광(Diffuse)", &DiffuseColor.x, ImGuiColorEditFlags_RGB))
								{
									Material->SetDiffuseColor(DiffuseColor);
								}

								ImGui::SetNextItemWidth(KUniformWidth);
								XMFLOAT3 SpecularColor{ Material->GetSpecularColor() };
								if (ImGui::ColorEdit3(u8"정반사광(Specular)", &SpecularColor.x, ImGuiColorEditFlags_RGB))
								{
									Material->SetSpecularColor(SpecularColor);
								}

								ImGui::SetNextItemWidth(KUniformWidth);
								float SpecularExponent{ Material->GetSpecularExponent() };
								if (ImGui::DragFloat(u8"정반사광(Specular) 지수", &SpecularExponent, 0.001f, 0.0f, 1.0f, "%.3f"))
								{
									Material->SetSpecularExponent(SpecularExponent);
								}

								ImGui::SetNextItemWidth(KUniformWidth);
								float SpecularIntensity{ Material->GetSpecularIntensity() };
								if (ImGui::DragFloat(u8"정반사광(Specular) 강도", &SpecularIntensity, 0.001f, 0.0f, 1.0f, "%.3f"))
								{
									Material->SetSpecularIntensity(SpecularIntensity);
								}

								static const char KTextureDialogFilter[]{ "PNG 파일\0*.png\0JPG 파일\0*.jpg\0모든 파일\0*.*\0" };
								static const char KTextureDialogTitle[]{ "텍스쳐 불러오기" };

								// Diffuse texture
								{
									const char* PtrDiffuseTextureLabel{ KLabelAdd };
									if (Material->HasTexture(CMaterial::CTexture::EType::DiffuseTexture))
									{
										ImGui::Image(GetMaterialTexture(CMaterial::CTexture::EType::DiffuseTexture,
											pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
										ImGui::SameLine();
										ImGui::SetNextItemWidth(KUniformWidth);
										ImGui::Text(u8"Diffuse 텍스쳐");

										PtrDiffuseTextureLabel = KLabelChange;
									}
									else
									{
										ImGui::Image(0, ImVec2(100, 100));
										ImGui::SameLine();
										ImGui::SetNextItemWidth(KUniformWidth);
										ImGui::Text(u8"Diffuse 텍스쳐: 없음");

										PtrDiffuseTextureLabel = KLabelAdd;
									}

									ImGui::SameLine();

									ImGui::PushID(0);
									if (ImGui::Button(PtrDiffuseTextureLabel))
									{
										static CFileDialog FileDialog{ GetWorkingDirectory() };
										if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
										{
											Material->SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, FileDialog.GetRelativeFileName());
											ReloadMaterial(pairMaterial.first);
										}
									}
									ImGui::PopID();
								}

								// Normal texture
								{
									const char* PtrNormalTextureLabel{ KLabelAdd };
									if (Material->HasTexture(CMaterial::CTexture::EType::NormalTexture))
									{
										ImGui::Image(GetMaterialTexture(CMaterial::CTexture::EType::NormalTexture,
											pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
										ImGui::SameLine();
										ImGui::Text(u8"Normal 텍스쳐");

										PtrNormalTextureLabel = KLabelChange;
									}
									else
									{
										ImGui::Image(0, ImVec2(100, 100));
										ImGui::SameLine();
										ImGui::SetNextItemWidth(KUniformWidth);
										ImGui::Text(u8"Normal 텍스쳐: 없음");

										PtrNormalTextureLabel = KLabelAdd;
									}

									ImGui::SameLine();

									ImGui::PushID(1);
									if (ImGui::Button(PtrNormalTextureLabel))
									{
										static CFileDialog FileDialog{ GetWorkingDirectory() };
										if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
										{
											Material->SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, FileDialog.GetRelativeFileName());
											ReloadMaterial(pairMaterial.first);
										}
									}
									ImGui::PopID();
								}

								// Displacement texture
								{
									const char* PtrDisplacementTextureLabel{ KLabelAdd };
									if (Material->HasTexture(CMaterial::CTexture::EType::DisplacementTexture))
									{
										ImGui::Image(GetMaterialTexture(CMaterial::CTexture::EType::DisplacementTexture,
											pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
										ImGui::SameLine();
										ImGui::Text(u8"Displacement 텍스쳐");

										PtrDisplacementTextureLabel = KLabelChange;
									}
									else
									{
										ImGui::Image(0, ImVec2(100, 100));
										ImGui::SameLine();
										ImGui::SetNextItemWidth(KUniformWidth);
										ImGui::Text(u8"Displacement 텍스쳐: 없음");

										PtrDisplacementTextureLabel = KLabelAdd;
									}

									ImGui::SameLine();

									ImGui::PushID(2);
									if (ImGui::Button(PtrDisplacementTextureLabel))
									{
										static CFileDialog FileDialog{ GetWorkingDirectory() };
										if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
										{
											Material->SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, FileDialog.GetRelativeFileName());
											ReloadMaterial(pairMaterial.first);
										}
									}
									ImGui::PopID();
								}

								ImGui::TreePop();
							}
						}

						// ### 재질 이름 변경 윈도우 ###
						if (bShowMaterialNameChangeWindow) ImGui::OpenPopup(u8"재질 이름 변경");
						{
							ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_Always);
							if (ImGui::BeginPopupModal(u8"재질 이름 변경", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
							{
								ImGui::SetNextItemWidth(160);
								ImGui::InputText(u8"새 이름", NewName, KNameMaxLength, ImGuiInputTextFlags_EnterReturnsTrue);

								ImGui::Separator();

								if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
								{
									if (mapMaterialList.find(NewName) != mapMaterialList.end())
									{
										MB_WARN("이미 존재하는 이름입니다. 다른 이름을 골라 주세요.", "오류");
									}
									else
									{
										ChangeMaterialName(OldName, NewName);

										ImGui::CloseCurrentPopup();
										bShowMaterialNameChangeWindow = false;
									}
								}

								ImGui::SameLine();

								if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
								{
									ImGui::CloseCurrentPopup();
									bShowMaterialNameChangeWindow = false;
								}

								ImGui::EndPopup();
							}
						}

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem(u8"기타"))
					{
						const XMVECTOR& KDirectionalLightDirection{ GetDirectionalLightDirection() };
						float DirectionalLightDirection[3]{ XMVectorGetX(KDirectionalLightDirection), XMVectorGetY(KDirectionalLightDirection),
							XMVectorGetZ(KDirectionalLightDirection) };

						const XMVECTOR& KDirectionalLightColor{ GetDirectionalLightColor() };
						float DirectionalLightColor[3]{ XMVectorGetX(KDirectionalLightColor), XMVectorGetY(KDirectionalLightColor),
							XMVectorGetZ(KDirectionalLightColor) };

						CCamera* const MainCamera{ GetCamera(0) };
						const XMVECTOR& KEyePosition{ MainCamera->GetEyePosition() };
						float EyePosition[3]{ XMVectorGetX(KEyePosition), XMVectorGetY(KEyePosition), XMVectorGetZ(KEyePosition) };
						float Pitch{ MainCamera->GetPitch() };
						float Yaw{ MainCamera->GetYaw() };

						static constexpr float KLabelsWidth{ 200 };
						static constexpr float KItemsMaxWidth{ 240 };
						float ItemsWidth{ WindowWidth - KLabelsWidth };
						ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
						float ItemsOffsetX{ WindowWidth - ItemsWidth };
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
								if (ImGui::DragFloat3(u8"##Directional Light 색상", DirectionalLightColor, 0.02f, 0.0f, 1.0f, "%.2f"))
								{
									SetDirectionalLightColor(XMVectorSet(DirectionalLightColor[0], DirectionalLightColor[1],
										DirectionalLightColor[2], 1.0f));
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"Ambient Light 색상");
								ImGui::SameLine(ItemsOffsetX);
								XMFLOAT3 AmbientLightColor{ GetAmbientLightColor() };
								if (ImGui::DragFloat3(u8"##Ambient Light 색상", &AmbientLightColor.x, 0.02f, 0.0f, 1.0f, "%.2f"))
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

								ImGui::TreePop();
							}

							ImGui::Separator();

							if (ImGui::TreeNodeEx(u8"카메라", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"위치");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat3(u8"##위치", EyePosition, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									MainCamera->SetEyePosition(XMVectorSet(EyePosition[0], EyePosition[1], EyePosition[2], 1.0f));
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"회전 Pitch");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##회전 Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									MainCamera->SetPitch(Pitch);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"회전 Yaw");
								ImGui::SameLine(ItemsOffsetX);
								if (ImGui::DragFloat(u8"##회전 Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									MainCamera->SetYaw(Yaw);
								}

								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"카메라 이동 속도");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::SliderFloat(u8"##카메라 이동 속도", &m_CameraMovementFactor, 1.0f, 100.0f, "%.0f");

								ImGui::TreePop();
							}

							if (ImGui::TreeNodeEx(u8"FPS", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"Frames per second:");
								ImGui::SameLine(ItemsOffsetX);
								ImGui::AlignTextToFramePadding();
								ImGui::Text(u8"%d", m_FPS);

								ImGui::TreePop();
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

		static bool bShowAddObject3D{ false };
		static bool bShowLoadModelDialog{ false };

		// ### 장면 편집기 윈도우 ###
		if (bShowSceneEditor)
		{
			const auto& mapObject3D{ GetObject3DMap() };

			ImGui::SetNextWindowPos(ImVec2(0, 122), ImGuiCond_Appearing);
			ImGui::SetNextWindowSizeConstraints(ImVec2(300, 60), ImVec2(400, 300));
			if (ImGui::Begin(u8"장면 편집기", &bShowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
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
					bShowAddObject3D = true;
				}

				ImGui::SameLine();

				// 오브젝트 제거
				if (ImGui::Button(u8"오브젝트 제거"))
				{
					DeleteObject3D(GetSelectedObject3DName());
				}

				ImGui::Separator();

				ImGui::Columns(2);
				ImGui::Text(u8"오브젝트 및 인스턴스"); ImGui::NextColumn();
				ImGui::Text(u8"인스턴스 관리"); ImGui::NextColumn();
				ImGui::Separator();

				// 오브젝트 목록
				int iObject3DPair{};
				for (const auto& Object3DPair : mapObject3D)
				{
					CObject3D* const Object3D{ GetObject3D(Object3DPair.first) };
					bool bIsThisObject3DSelected{ false };
					if (GetSelectedObject3DName() == Object3DPair.first) bIsThisObject3DSelected = true;

					ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth };
					if (bIsThisObject3DSelected) Flags |= ImGuiTreeNodeFlags_Selected;
					if (!Object3D->IsInstanced()) Flags |= ImGuiTreeNodeFlags_Leaf;

					if (!Object3D->IsInstanced()) ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

					bool bIsNodeOpen{ ImGui::TreeNodeEx(Object3DPair.first.c_str(), Flags) };
					if (ImGui::IsItemClicked())
					{
						SelectObject3D(Object3DPair.first);

						SelectInstance(-1);
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
								const CObject3D::SInstanceCPUData& Instance{ Object3D->GetInstance(GetSelectedInstanceID()) };
								Object3D->DeleteInstance(Instance.Name);
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
								bool bSelected{ (iInstancePair == GetSelectedInstanceID()) };
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
			}
			ImGui::End();
		}

		if (bShowAddObject3D) ImGui::OpenPopup(u8"AddObject3D");

		if (ImGui::BeginPopup(u8"AddObject3D"))
		{
			static char NewObejct3DName[CGame::KObject3DNameMaxLength]{};
			static char ModelFileNameWithPath[MAX_PATH]{};
			static char ModelFileNameWithoutPath[MAX_PATH]{};

			ImGui::SetItemDefaultFocus();
			ImGui::SetNextItemWidth(140);
			ImGui::InputText(u8"오브젝트 이름", NewObejct3DName, CGame::KObject3DNameMaxLength);

			if (ImGui::Button(u8"모델 불러오기"))
			{
				bShowLoadModelDialog = true;
			}
			ImGui::SameLine();

			ImGui::Text(ModelFileNameWithoutPath);

			ImGui::SameLine();

			static bool bIsModelRigged{ false };
			ImGui::Checkbox(u8"리깅 여부", &bIsModelRigged);

			if (ImGui::Button(u8"결정") || m_CapturedKeyboardState.Enter)
			{
				if (ModelFileNameWithPath[0] == 0)
				{
					MB_WARN("모델을 불러오세요.", "오류");
				}
				else if (strlen(NewObejct3DName) == 0)
				{
					MB_WARN("이름을 정하세요.", "오류");
				}
				else
				{
					InsertObject3D(NewObejct3DName);
					CObject3D* Object3D{ GetObject3D(NewObejct3DName) };
					Object3D->CreateFromFile(ModelFileNameWithPath, bIsModelRigged);

					bShowAddObject3D = false;
					memset(ModelFileNameWithPath, 0, MAX_PATH);
					memset(ModelFileNameWithoutPath, 0, MAX_PATH);
					memset(NewObejct3DName, 0, CGame::KObject3DNameMaxLength);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button(u8"취소"))
			{
				bShowAddObject3D = false;
				memset(ModelFileNameWithPath, 0, MAX_PATH);
				memset(ModelFileNameWithoutPath, 0, MAX_PATH);
				memset(NewObejct3DName, 0, CGame::KObject3DNameMaxLength);
				ImGui::CloseCurrentPopup();
			}

			if (bShowLoadModelDialog)
			{
				static CFileDialog FileDialog{ GetWorkingDirectory() };
				if (FileDialog.OpenFileDialog("FBX 파일\0*.fbx\0모든 파일\0*.*\0", "모델 불러오기"))
				{
					strcpy_s(ModelFileNameWithPath, FileDialog.GetRelativeFileName().c_str());
					strcpy_s(ModelFileNameWithoutPath, FileDialog.GetFileNameWithoutPath().c_str());
				}
				bShowLoadModelDialog = false;
			}

			ImGui::EndPopup();
		}

		static constexpr float KTestWindowSizeX{ 110.0f };
		ImGui::SetNextWindowPos(ImVec2((m_WindowSize.x - KTestWindowSizeX) * 0.5f, 21), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(KTestWindowSizeX, 0), ImGuiCond_Always);
		if (ImGui::Begin(u8"테스트 윈도우", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button(u8"테스트 시작"))
			{
				SetMode(CGame::EMode::Test);
			}

			ImGui::End();
		}
	}
	else if (m_eMode == CGame::EMode::Test)
	{
		static constexpr float KTestWindowSizeX{ 110.0f };
		ImGui::SetNextWindowPos(ImVec2((m_WindowSize.x - KTestWindowSizeX) * 0.5f, 21), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(KTestWindowSizeX, 0), ImGuiCond_Always);
		if (ImGui::Begin(u8"테스트 윈도우", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button(u8"테스트 종료"))
			{
				SetMode(CGame::EMode::Edit);
			}

			ImGui::End();
		}
	}

	ImGui::PopFont();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CGame::EndRendering()
{
	if (m_IsDestroyed) return;

	m_SwapChain->Present(0, 0);
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
