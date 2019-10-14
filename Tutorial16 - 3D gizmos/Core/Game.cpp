#include "Game.h"

static constexpr D3D11_INPUT_ELEMENT_DESC KBaseInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT	, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 1, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static constexpr D3D11_INPUT_ELEMENT_DESC KVSLineInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static constexpr D3D11_INPUT_ELEMENT_DESC KVS2DBaseInputLayout[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT		, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void CGame::CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed)
{
	CreateWin32Window(WndProc, WindowName);

	InitializeDirectX(FontFileName, bWindowed);
}

void CGame::Destroy()
{
	DestroyWindow(m_hWnd);
}

void CGame::SetPerspective(float FOV, float NearZ, float FarZ)
{
	m_NearZ = NearZ;
	m_FarZ = FarZ;

	m_MatrixProjection = XMMatrixPerspectiveFovLH(FOV, m_WindowSize.x / m_WindowSize.y, m_NearZ, m_FarZ);
}

void CGame::SetGameRenderingFlags(EFlagsGameRendering Flags)
{
	m_eFlagsGameRendering = Flags;
}

void CGame::ToggleGameRenderingFlags(EFlagsGameRendering Flags)
{
	m_eFlagsGameRendering ^= Flags;
}

void CGame::Set3DGizmoMode(E3DGizmoMode Mode)
{
	m_e3DGizmoMode = Mode;
}

void CGame::UpdatePSBaseFlagOn(EFlagPSBase Flag)
{
	switch (Flag)
	{
	case EFlagPSBase::UseTexture:
		m_cbPSBaseFlagsData.bUseTexture = TRUE;
		break;
	case EFlagPSBase::UseLighting:
		m_cbPSBaseFlagsData.bUseLighting = TRUE;
		break;
	default:
		break;
	}
	m_PSBase->UpdateConstantBuffer(0);
}

void CGame::UpdatePSBaseFlagOff(EFlagPSBase Flag)
{
	switch (Flag)
	{
	case EFlagPSBase::UseTexture:
		m_cbPSBaseFlagsData.bUseTexture = FALSE;
		break;
	case EFlagPSBase::UseLighting:
		m_cbPSBaseFlagsData.bUseLighting = FALSE;
		break;
	default:
		break;
	}
	m_PSBase->UpdateConstantBuffer(0);
}

void CGame::UpdateVSBaseMaterial(const SMaterial& Material)
{
	m_cbPSBaseMaterialData.MaterialAmbient = Material.MaterialAmbient;
	m_cbPSBaseMaterialData.MaterialDiffuse = Material.MaterialDiffuse;
	m_cbPSBaseMaterialData.MaterialSpecular = Material.MaterialSpecular;
	m_cbPSBaseMaterialData.SpecularExponent = Material.SpecularExponent;
	m_cbPSBaseMaterialData.SpecularIntensity = Material.SpecularIntensity;
	m_PSBase->UpdateConstantBuffer(2);
}

void CGame::UpdateVSAnimationBoneMatrices(const XMMATRIX* BoneMatrices)
{
	memcpy(m_cbVSAnimationBonesData.BoneMatrices, BoneMatrices, sizeof(SCBVSAnimationBonesData));
	m_VSAnimation->UpdateConstantBuffer(1);
}

void CGame::SetSky(const string& SkyDataFileName, float ScalingFactor)
{
	using namespace tinyxml2;

	size_t Point{ SkyDataFileName.find_last_of('.') };
	string Extension{ SkyDataFileName.substr(Point + 1) };
	for (auto& c : Extension)
	{
		c = toupper(c);
	}
	assert(Extension == "XML");
	
	tinyxml2::XMLDocument xmlDocument{};
	assert(xmlDocument.LoadFile(SkyDataFileName.c_str()) == XML_SUCCESS);
	
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

	m_SkyTexture = make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get());
	m_SkyTexture->CreateFromFile(wstring(m_SkyData.TextureFileName.begin(), m_SkyData.TextureFileName.end()));

	m_Object3DSkySphere = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom));

	m_Object3DSun = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSun->Create(GenerateSquareYZPlane(KColorWhite));
	m_Object3DSun->UpdateQuadUV(m_SkyData.Sun.UVOffset, m_SkyData.Sun.UVSize);

	m_Object3DMoon = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DMoon->Create(GenerateSquareYZPlane(KColorWhite));
	m_Object3DMoon->UpdateQuadUV(m_SkyData.Moon.UVOffset, m_SkyData.Moon.UVSize);

	m_Object3DCloud = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DCloud->Create(GenerateSquareYZPlane(KColorWhite));
	m_Object3DCloud->UpdateQuadUV(m_SkyData.Cloud.UVOffset, m_SkyData.Cloud.UVSize);

	m_GameObject3DSkySphere = make_unique<CGameObject3D>("SkySphere");
	m_GameObject3DSkySphere->ComponentRender.PtrObject3D = m_Object3DSkySphere.get();
	m_GameObject3DSkySphere->ComponentRender.PtrVS = m_VSSky.get();
	m_GameObject3DSkySphere->ComponentRender.PtrPS = m_PSSky.get();
	m_GameObject3DSkySphere->ComponentPhysics.bIsPickable = false;
	m_GameObject3DSkySphere->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoCulling | EFlagsGameObject3DRendering::NoLighting;

	m_GameObject3DSun = make_unique<CGameObject3D>("Sun");
	m_GameObject3DSun->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Sun.WidthHeightRatio, 0);
	m_GameObject3DSun->ComponentRender.PtrObject3D = m_Object3DSun.get();
	m_GameObject3DSun->ComponentRender.PtrVS = m_VSSky.get();
	m_GameObject3DSun->ComponentRender.PtrPS = m_PSBase.get();
	m_GameObject3DSun->ComponentRender.PtrTexture = m_SkyTexture.get();
	m_GameObject3DSun->ComponentRender.bIsTransparent = true;
	m_GameObject3DSun->ComponentPhysics.bIsPickable = false;
	m_GameObject3DSun->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoCulling | EFlagsGameObject3DRendering::NoLighting;

	m_GameObject3DMoon = make_unique<CGameObject3D>("Moon");
	m_GameObject3DMoon->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Moon.WidthHeightRatio, 0);
	m_GameObject3DMoon->ComponentRender.PtrObject3D = m_Object3DMoon.get();
	m_GameObject3DMoon->ComponentRender.PtrVS = m_VSSky.get();
	m_GameObject3DMoon->ComponentRender.PtrPS = m_PSBase.get();
	m_GameObject3DMoon->ComponentRender.PtrTexture = m_SkyTexture.get();
	m_GameObject3DMoon->ComponentRender.bIsTransparent = true;
	m_GameObject3DMoon->ComponentPhysics.bIsPickable = false;
	m_GameObject3DMoon->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoCulling | EFlagsGameObject3DRendering::NoLighting;

	m_GameObject3DCloud = make_unique<CGameObject3D>("Cloud");
	m_GameObject3DCloud->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Cloud.WidthHeightRatio, 0);
	m_GameObject3DCloud->ComponentRender.PtrObject3D = m_Object3DCloud.get();
	m_GameObject3DCloud->ComponentRender.PtrVS = m_VSSky.get();
	m_GameObject3DCloud->ComponentRender.PtrPS = m_PSBase.get();
	m_GameObject3DCloud->ComponentRender.PtrTexture = m_SkyTexture.get();
	m_GameObject3DCloud->ComponentRender.bIsTransparent = true;
	m_GameObject3DCloud->ComponentPhysics.bIsPickable = false;
	m_GameObject3DCloud->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoCulling | EFlagsGameObject3DRendering::NoLighting;

	m_SkyData.bIsDataSet = true;

	return;
}

void CGame::LoadSkyObjectData(tinyxml2::XMLElement* xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData)
{
	using namespace tinyxml2;

	XMLElement* xmlUVOffset{ xmlSkyObject->FirstChildElement() };
	SkyObjectData.UVOffset.x = xmlUVOffset->FloatAttribute("U");
	SkyObjectData.UVOffset.y = xmlUVOffset->FloatAttribute("V");

	XMLElement* xmlUVSize{ xmlUVOffset->NextSiblingElement() };
	SkyObjectData.UVSize.x = xmlUVSize->FloatAttribute("U");
	SkyObjectData.UVSize.y = xmlUVSize->FloatAttribute("V");

	XMLElement* xmlWidthHeightRatio{ xmlUVSize->NextSiblingElement() };
	SkyObjectData.WidthHeightRatio = stof(xmlWidthHeightRatio->GetText());
}

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition)
{
	m_cbPSBaseLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);

	m_PSBase->UpdateConstantBuffer(1);
}

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color)
{
	m_cbPSBaseLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	m_cbPSBaseLightsData.DirectionalColor = Color;

	m_PSBase->UpdateConstantBuffer(1);
}

void CGame::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	m_cbPSBaseLightsData.AmbientLightColor = Color;
	m_cbPSBaseLightsData.AmbientLightIntensity = Intensity;

	m_PSBase->UpdateConstantBuffer(1);
}

CCamera* CGame::AddCamera(const SCameraData& CameraData)
{
	m_vCameras.emplace_back(CameraData);
	return &m_vCameras.back();
}

CCamera* CGame::GetCamera(size_t Index)
{
	assert(Index < m_vCameras.size());
	return &m_vCameras[Index];
}

void CGame::CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName)
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

	assert(m_hWnd = CreateWindowEx(0, KClassName, WindowName, KWindowStyle,
		CW_USEDEFAULT,  CW_USEDEFAULT, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top,
		nullptr, nullptr, m_hInstance, nullptr));

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
}

void CGame::InitializeDirectX(const wstring& FontFileName, bool bWindowed)
{
	CreateSwapChain(bWindowed);

	CreateSetViews();

	SetViewports();

	SetPerspective(KDefaultFOV, KDefaultNearZ, KDefaultFarZ);

	CreateInputDevices();

	CreateBaseShaders();

	CreateMiniAxes();

	CreatePickingRay();
	CreatePickedTriangle();

	CreateBoundingSphere();

	Create3DGizmos();

	m_MatrixProjection2D = XMMatrixOrthographicLH(m_WindowSize.x, m_WindowSize.y, 0.0f, 1.0f);
	m_SpriteBatch = make_unique<SpriteBatch>(m_DeviceContext.Get());
	m_SpriteFont = make_unique<SpriteFont>(m_Device.Get(), FontFileName.c_str());
	m_CommonStates = make_unique<CommonStates>(m_Device.Get());
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
		Viewport.TopLeftX = m_WindowSize.x * 0.875f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}
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
	m_VSBase->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSAnimation->Create(EShaderType::VertexShader, L"Shader\\VSAnimation.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSAnimation->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));
	m_VSAnimation->AddConstantBuffer(&m_cbVSAnimationBonesData, sizeof(SCBVSAnimationBonesData));

	m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSSky->Create(EShaderType::VertexShader, L"Shader\\VSSky.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSSky->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSLine->Create(EShaderType::VertexShader, L"Shader\\VSLine.hlsl", "main", KVSLineInputElementDescs, ARRAYSIZE(KVSLineInputElementDescs));
	m_VSLine->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSGizmo->Create(EShaderType::VertexShader, L"Shader\\VSGizmo.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSGizmo->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase2D->Create(EShaderType::VertexShader, L"Shader\\VSBase2D.hlsl", "main", KVS2DBaseInputLayout, ARRAYSIZE(KVS2DBaseInputLayout));
	m_VSBase2D->AddConstantBuffer(&m_cbVS2DSpaceData, sizeof(SCBVS2DSpaceData));

	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormal.hlsl", "main");

	m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase->Create(EShaderType::PixelShader, L"Shader\\PSBase.hlsl", "main");
	m_PSBase->AddConstantBuffer(&m_cbPSBaseFlagsData, sizeof(SCBPSBaseFlagsData));
	m_PSBase->AddConstantBuffer(&m_cbPSBaseLightsData, sizeof(SCBPSBaseLightsData));
	m_PSBase->AddConstantBuffer(&m_cbPSBaseMaterialData, sizeof(SCBPSBaseMaterialData));
	m_PSBase->AddConstantBuffer(&m_cbPSBaseEyeData, sizeof(SCBPSBaseEyeData));

	m_PSVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSVertexColor->Create(EShaderType::PixelShader, L"Shader\\PSVertexColor.hlsl", "main");

	m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSSky->Create(EShaderType::PixelShader, L"Shader\\PSSky.hlsl", "main");
	m_PSSky->AddConstantBuffer(&m_cbPSSkyTimeData, sizeof(SCBPSSkyTimeData));

	m_PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSLine->Create(EShaderType::PixelShader, L"Shader\\PSLine.hlsl", "main");

	m_PSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSGizmo->Create(EShaderType::PixelShader, L"Shader\\PSGizmo.hlsl", "main");
	m_PSGizmo->AddConstantBuffer(&m_cbPSGizmoColorFactorData, sizeof(SCBPSGizmoColorFactorData));

	m_PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase2D->Create(EShaderType::PixelShader, L"Shader\\PSBase2D.hlsl", "main");
	m_PSBase2D->AddConstantBuffer(&m_cbPS2DFlagsData, sizeof(SCBPS2DFlagsData));
}

void CGame::CreateMiniAxes()
{
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));

	SMesh Cone{ GenerateCone(0, 1.0f, 1.0f, 16) };
	vector<SMaterial> vMaterials{ SMaterial(XMFLOAT3(1, 0, 0)), SMaterial(XMFLOAT3(0, 1, 0)), SMaterial(XMFLOAT3(0, 0, 1)) };
	m_vObject3DMiniAxes[0]->Create(Cone, vMaterials[0]);
	m_vObject3DMiniAxes[1]->Create(Cone, vMaterials[1]);
	m_vObject3DMiniAxes[2]->Create(Cone, vMaterials[2]);

	m_vGameObject3DMiniAxes.emplace_back(make_unique<CGameObject3D>("AxisX"));
	m_vGameObject3DMiniAxes.emplace_back(make_unique<CGameObject3D>("AxisY"));
	m_vGameObject3DMiniAxes.emplace_back(make_unique<CGameObject3D>("AxisZ"));

	m_vGameObject3DMiniAxes[0]->ComponentRender.PtrObject3D = m_vObject3DMiniAxes[0].get();
	m_vGameObject3DMiniAxes[0]->ComponentRender.PtrVS = m_VSBase.get();
	m_vGameObject3DMiniAxes[0]->ComponentRender.PtrPS = m_PSBase.get();
	m_vGameObject3DMiniAxes[0]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vGameObject3DMiniAxes[0]->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoLighting;

	m_vGameObject3DMiniAxes[1]->ComponentRender.PtrObject3D = m_vObject3DMiniAxes[1].get();
	m_vGameObject3DMiniAxes[1]->ComponentRender.PtrVS = m_VSBase.get();
	m_vGameObject3DMiniAxes[1]->ComponentRender.PtrPS = m_PSBase.get();
	m_vGameObject3DMiniAxes[1]->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoLighting;

	m_vGameObject3DMiniAxes[2]->ComponentRender.PtrObject3D = m_vObject3DMiniAxes[2].get();
	m_vGameObject3DMiniAxes[2]->ComponentRender.PtrVS = m_VSBase.get();
	m_vGameObject3DMiniAxes[2]->ComponentRender.PtrPS = m_PSBase.get();
	m_vGameObject3DMiniAxes[2]->ComponentTransform.Yaw = -XM_PIDIV2;
	m_vGameObject3DMiniAxes[2]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vGameObject3DMiniAxes[2]->eFlagsGameObject3DRendering = EFlagsGameObject3DRendering::NoLighting;

	m_vGameObject3DMiniAxes[0]->ComponentTransform.Scaling =
		m_vGameObject3DMiniAxes[1]->ComponentTransform.Scaling =
		m_vGameObject3DMiniAxes[2]->ComponentTransform.Scaling = XMVectorSet(0.1f, 0.8f, 0.1f, 0);
}

void CGame::CreatePickingRay()
{
	m_Object3DLinePickingRay = make_unique<CObject3DLine>(m_Device.Get(), m_DeviceContext.Get());

	vector<SVertex3DLine> Vertices{};
	Vertices.emplace_back(XMVectorSet(0, 0, 0, 1), XMVectorSet(1, 0, 0, 1));
	Vertices.emplace_back(XMVectorSet(10.0f, 10.0f, 0, 1), XMVectorSet(0, 1, 0, 1));

	m_Object3DLinePickingRay->Create(Vertices);
}

void CGame::CreateBoundingSphere()
{
	m_Object3DBoundingSphere = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DBoundingSphere->Create(GenerateSphere(16));
}

void CGame::CreatePickedTriangle()
{
	m_Object3DPickedTriangle = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DPickedTriangle->Create(GenerateTriangle(XMVectorSet(0, 0, 1.5f, 1), XMVectorSet(+1.0f, 0, 0, 1), XMVectorSet(-1.0f, 0, 0, 1),
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f)));
}

void CGame::Create3DGizmos()
{
	const static XMVECTOR ColorX{ XMVectorSet(1.0f, 0.1f, 0.1f, 1) };
	const static XMVECTOR ColorY{ XMVectorSet(0.1f, 1.0f, 0.1f, 1) };
	const static XMVECTOR ColorZ{ XMVectorSet(0.1f, 0.1f, 1.0f, 1) };

	m_Object3D_3DGizmoRotationPitch = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorX, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationPitch->Create(MeshRing);
	}

	m_Object3D_3DGizmoRotationYaw = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorY, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationYaw->Create(MeshRing);
	}

	m_Object3D_3DGizmoRotationRoll = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorZ, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationRoll->Create(MeshRing);
	}

	m_GameObject3D_3DGizmoRotationPitch = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoRotationPitch->ComponentTransform.Roll = -XM_PIDIV2;
	m_GameObject3D_3DGizmoRotationPitch->ComponentRender.PtrObject3D = m_Object3D_3DGizmoRotationPitch.get();
	m_GameObject3D_3DGizmoRotationPitch->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoRotationPitch->ComponentRender.PtrPS = m_PSGizmo.get();

	m_GameObject3D_3DGizmoRotationYaw = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoRotationYaw->ComponentRender.PtrObject3D = m_Object3D_3DGizmoRotationYaw.get();
	m_GameObject3D_3DGizmoRotationYaw->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoRotationYaw->ComponentRender.PtrPS = m_PSGizmo.get();

	m_GameObject3D_3DGizmoRotationRoll = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoRotationRoll->ComponentTransform.Pitch = XM_PIDIV2;
	m_GameObject3D_3DGizmoRotationRoll->ComponentRender.PtrObject3D = m_Object3D_3DGizmoRotationRoll.get();
	m_GameObject3D_3DGizmoRotationRoll->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoRotationRoll->ComponentRender.PtrPS = m_PSGizmo.get();


	m_Object3D_3DGizmoTranslationX = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorX) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationX->Create(MeshAxis);
	}

	m_Object3D_3DGizmoTranslationY = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorY) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationY->Create(MeshAxis);
	}

	m_Object3D_3DGizmoTranslationZ = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorZ) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationZ->Create(MeshAxis);
	}

	m_GameObject3D_3DGizmoTranslationX = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoTranslationX->ComponentTransform.Roll = -XM_PIDIV2;
	m_GameObject3D_3DGizmoTranslationX->ComponentRender.PtrObject3D = m_Object3D_3DGizmoTranslationX.get();
	m_GameObject3D_3DGizmoTranslationX->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoTranslationX->ComponentRender.PtrPS = m_PSGizmo.get();

	m_GameObject3D_3DGizmoTranslationY = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoTranslationY->ComponentRender.PtrObject3D = m_Object3D_3DGizmoTranslationY.get();
	m_GameObject3D_3DGizmoTranslationY->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoTranslationY->ComponentRender.PtrPS = m_PSGizmo.get();

	m_GameObject3D_3DGizmoTranslationZ = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoTranslationZ->ComponentTransform.Pitch = XM_PIDIV2;
	m_GameObject3D_3DGizmoTranslationZ->ComponentRender.PtrObject3D = m_Object3D_3DGizmoTranslationZ.get();
	m_GameObject3D_3DGizmoTranslationZ->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoTranslationZ->ComponentRender.PtrPS = m_PSGizmo.get();


	m_Object3D_3DGizmoScalingX = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		SMesh MeshCube{ GenerateCube(ColorX) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingX->Create(MeshAxis);
	}

	m_Object3D_3DGizmoScalingY = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		SMesh MeshCube{ GenerateCube(ColorY) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingY->Create(MeshAxis);
	}

	m_Object3D_3DGizmoScalingZ = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		SMesh MeshCube{ GenerateCube(ColorZ) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingZ->Create(MeshAxis);
	}

	m_GameObject3D_3DGizmoScalingX = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoScalingX->ComponentTransform.Roll = -XM_PIDIV2;
	m_GameObject3D_3DGizmoScalingX->ComponentRender.PtrObject3D = m_Object3D_3DGizmoScalingX.get();
	m_GameObject3D_3DGizmoScalingX->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoScalingX->ComponentRender.PtrPS = m_PSGizmo.get();

	m_GameObject3D_3DGizmoScalingY = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoScalingY->ComponentRender.PtrObject3D = m_Object3D_3DGizmoScalingY.get();
	m_GameObject3D_3DGizmoScalingY->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoScalingY->ComponentRender.PtrPS = m_PSGizmo.get();

	m_GameObject3D_3DGizmoScalingZ = make_unique<CGameObject3D>("Gizmo");
	m_GameObject3D_3DGizmoScalingZ->ComponentTransform.Pitch = XM_PIDIV2;
	m_GameObject3D_3DGizmoScalingZ->ComponentRender.PtrObject3D = m_Object3D_3DGizmoScalingZ.get();
	m_GameObject3D_3DGizmoScalingZ->ComponentRender.PtrVS = m_VSGizmo.get();
	m_GameObject3D_3DGizmoScalingZ->ComponentRender.PtrPS = m_PSGizmo.get();
}

CShader* CGame::AddShader()
{
	m_vShaders.emplace_back(make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vShaders.back().get();
}

CShader* CGame::GetShader(size_t Index)
{
	assert(Index < m_vShaders.size());
	return m_vShaders[Index].get();
}

CShader* CGame::GetBaseShader(EBaseShader eShader)
{
	CShader* Result{};
	switch (eShader)
	{
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
	case EBaseShader::VSBase2D:
		Result = m_VSBase2D.get();
		break;
	case EBaseShader::GSNormal:
		Result = m_GSNormal.get();
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
	case EBaseShader::PSLine:
		Result = m_PSLine.get();
		break;
	case EBaseShader::PSGizmo:
		Result = m_PSGizmo.get();
		break;
	case EBaseShader::PSBase2D:
		Result = m_PSBase2D.get();
		break;
	default:
		assert(Result);
		break;
	}

	return Result;
}

CObject3D* CGame::AddObject3D()
{
	m_vObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	return m_vObject3Ds.back().get();
}

CObject3D* CGame::GetObject3D(size_t Index)
{
	assert(Index < m_vObject3Ds.size());
	return m_vObject3Ds[Index].get();
}

CObject3DLine* CGame::AddObject3DLine()
{
	m_vObject3DLines.emplace_back(make_unique<CObject3DLine>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vObject3DLines.back().get();
}

CObject3DLine* CGame::GetObject3DLine(size_t Index)
{
	assert(Index < m_vObject3DLines.size());
	return m_vObject3DLines[Index].get();
}

CObject2D* CGame::AddObject2D()
{
	m_vObject2Ds.emplace_back(make_unique<CObject2D>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vObject2Ds.back().get();
}

CObject2D* CGame::GetObject2D(size_t Index)
{
	assert(Index < m_vObject2Ds.size());
	return m_vObject2Ds[Index].get();
}

CTexture* CGame::AddTexture()
{
	m_vTextures.emplace_back(make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vTextures.back().get();
}

CTexture* CGame::GetTexture(size_t Index)
{
	assert(Index < m_vTextures.size());
	return m_vTextures[Index].get();
}

CGameObject3D* CGame::AddGameObject3D(const string& Name)
{
	assert(m_mapGameObject3DNameToIndex.find(Name) == m_mapGameObject3DNameToIndex.end());

	m_vGameObject3Ds.emplace_back(make_unique<CGameObject3D>(Name));
	m_vGameObject3Ds.back()->ComponentRender.PtrVS = m_VSBase.get();
	m_vGameObject3Ds.back()->ComponentRender.PtrPS = m_PSBase.get();

	m_mapGameObject3DNameToIndex[Name] = m_vGameObject3Ds.size() - 1;

	return m_vGameObject3Ds.back().get();
}

CGameObject3D* CGame::GetGameObject3D(const string& Name)
{
	assert(m_mapGameObject3DNameToIndex.find(Name) != m_mapGameObject3DNameToIndex.end());
	return m_vGameObject3Ds[m_mapGameObject3DNameToIndex[Name]].get();
}

CGameObject3D* CGame::GetGameObject3D(size_t Index)
{
	assert(Index < m_vGameObject3Ds.size());
	return m_vGameObject3Ds[Index].get();
}

CGameObject3DLine* CGame::AddGameObject3DLine(const string& Name)
{
	assert(m_mapGameObject3DLineNameToIndex.find(Name) == m_mapGameObject3DLineNameToIndex.end());

	m_vGameObject3DLines.emplace_back(make_unique<CGameObject3DLine>(Name));

	m_mapGameObject3DLineNameToIndex[Name] = m_vGameObject3DLines.size() - 1;

	return m_vGameObject3DLines.back().get();
}

CGameObject3DLine* CGame::GetGameObject3DLine(const string& Name)
{
	assert(m_mapGameObject3DLineNameToIndex.find(Name) != m_mapGameObject3DLineNameToIndex.end());
	return m_vGameObject3DLines[m_mapGameObject3DLineNameToIndex[Name]].get();
}

CGameObject3DLine* CGame::GetGameObject3DLine(size_t Index)
{
	assert(Index < m_vGameObject3DLines.size());
	return m_vGameObject3DLines[Index].get();
}

CGameObject2D* CGame::AddGameObject2D(const string& Name)
{
	assert(m_mapGameObject2DNameToIndex.find(Name) == m_mapGameObject2DNameToIndex.end());

	m_vGameObject2Ds.emplace_back(make_unique<CGameObject2D>(Name));

	m_mapGameObject2DNameToIndex[Name] = m_vGameObject2Ds.size() - 1;

	return m_vGameObject2Ds.back().get();
}

CGameObject2D* CGame::GetGameObject2D(const string& Name)
{
	assert(m_mapGameObject2DNameToIndex.find(Name) != m_mapGameObject2DNameToIndex.end());
	return m_vGameObject2Ds[m_mapGameObject2DNameToIndex[Name]].get();
}

CGameObject2D* CGame::GetGameObject2D(size_t Index)
{
	assert(Index < m_vGameObject2Ds.size());
	return m_vGameObject2Ds[Index].get();
}

void CGame::Pick()
{
	CastPickingRay();

	UpdatePickingRay();

	PickBoundingSphere();

	PickTriangle();
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

bool CGame::PickBoundingSphere()
{
	m_PtrPickedGameObject3D = nullptr;

	XMVECTOR T{ KVectorGreatest };
	for (auto& i : m_vGameObject3Ds)
	{
		auto* GO3D{ i.get() };
		if (GO3D->ComponentPhysics.bIsPickable)
		{
			XMVECTOR NewT{ KVectorGreatest };
			if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
				GO3D->ComponentPhysics.BoundingSphere.Radius, GO3D->ComponentTransform.Translation + GO3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
			{
				if (XMVector3Less(NewT, T))
				{
					T = NewT;
					m_PtrPickedGameObject3D = GO3D;
				}
			}
		}
	}

	return (m_PtrPickedGameObject3D) ? true : false;
}

bool CGame::PickTriangle()
{
	XMVECTOR T{ KVectorGreatest };
	if (m_PtrPickedGameObject3D)
	{
		assert(m_PtrPickedGameObject3D->ComponentRender.PtrObject3D);

		// Pick only static models' triangle.
		if (m_PtrPickedGameObject3D->ComponentRender.PtrObject3D->m_Model.bIsModelAnimated) return false;

		const XMMATRIX& World{ m_PtrPickedGameObject3D->ComponentTransform.MatrixWorld };
		for (auto& Mesh : m_PtrPickedGameObject3D->ComponentRender.PtrObject3D->m_Model.vMeshes)
		{
			for (auto& Triangle : Mesh.vTriangles)
			{
				XMVECTOR V0{ Mesh.vVertices[Triangle.I0].Position };
				XMVECTOR V1{ Mesh.vVertices[Triangle.I1].Position };
				XMVECTOR V2{ Mesh.vVertices[Triangle.I2].Position };
				V0 = XMVector3TransformCoord(V0, World);
				V1 = XMVector3TransformCoord(V1, World);
				V2 = XMVector3TransformCoord(V2, World);

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

	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

	SetUniversalRasterizerState();

	m_MatrixView = XMMatrixLookAtLH(m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition, 
		m_vCameras[m_CurrentCameraIndex].m_CameraData.FocusPosition,
		m_vCameras[m_CurrentCameraIndex].m_CameraData.UpDirection);
}

void CGame::Animate()
{
	for (auto& i : m_vGameObject3Ds)
	{
		if (i->ComponentRender.PtrObject3D) i->ComponentRender.PtrObject3D->Animate();
	}
}

void CGame::Draw(float DeltaTime)
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	m_cbPSBaseEyeData.EyePosition = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition;

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawWireFrame))
	{
		m_eRasterizerState = ERasterizerState::WireFrame;
	}
	else
	{
		m_eRasterizerState = ERasterizerState::CullCounterClockwise;
	}

	for (auto& i : m_vGameObject3Ds)
	{
		if (i->ComponentRender.bIsTransparent) continue;

		UpdateGameObject3D(i.get());
		DrawGameObject3D(i.get());

		if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawBoundingSphere))
		{
			DrawGameObject3DBoundingSphere(i.get());
		}
	}

	for (auto& i : m_vGameObject3Ds)
	{
		if (!i->ComponentRender.bIsTransparent) continue;

		UpdateGameObject3D(i.get());
		DrawGameObject3D(i.get());
		
		if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawBoundingSphere))
		{
			DrawGameObject3DBoundingSphere(i.get());
		}
	}

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawNormals))
	{
		for (auto& i : m_vGameObject3Ds)
		{
			DrawGameObject3DNormal(i.get());
		}

		m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	}

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawMiniAxes))
	{
		DrawMiniAxes();
	}

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawPickingData))
	{
		DrawPickingRay();

		DrawPickedTriangle();
	}

	if (m_SkyData.bIsDataSet)
	{
		DrawSky(DeltaTime);
	}

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::Use3DGizmos))
	{
		Draw3DGizmos();
	}

	DrawGameObject3DLines();

	DrawGameObject2Ds();
}

void CGame::UpdateGameObject3D(CGameObject3D* PtrGO)
{
	assert(PtrGO->ComponentRender.PtrVS);
	assert(PtrGO->ComponentRender.PtrPS);

	CShader* VS{ PtrGO->ComponentRender.PtrVS };
	CShader* PS{ PtrGO->ComponentRender.PtrPS };

	m_cbVSSpaceData.World = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld);
	m_cbVSSpaceData.WVP = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);
	PtrGO->UpdateWorldMatrix();

	if (EFLAG_HAS(PtrGO->eFlagsGameObject3DRendering, EFlagsGameObject3DRendering::UseRawVertexColor))
	{
		PS = m_PSVertexColor.get();
	}

	SetUniversalbUseLighiting();
	if (EFLAG_HAS(PtrGO->eFlagsGameObject3DRendering, EFlagsGameObject3DRendering::NoLighting))
	{
		m_cbPSBaseFlagsData.bUseLighting = FALSE;
	}

	m_cbPSBaseFlagsData.bUseTexture = FALSE;
	if (PtrGO->ComponentRender.PtrTexture)
	{
		m_cbPSBaseFlagsData.bUseTexture = TRUE;
		PtrGO->ComponentRender.PtrTexture->Use();
	}

	VS->Use();
	VS->UpdateAllConstantBuffers();

	PS->Use();
	PS->UpdateAllConstantBuffers();
}

void CGame::DrawGameObject3D(CGameObject3D* PtrGO)
{
	if (EFLAG_HAS(PtrGO->eFlagsGameObject3DRendering, EFlagsGameObject3DRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRasterizerState();
	}

	if (EFLAG_HAS(PtrGO->eFlagsGameObject3DRendering, EFlagsGameObject3DRendering::NoDepthComparison))
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	}
	else
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
	}

	PtrGO->ComponentRender.PtrObject3D->Draw();
}

void CGame::DrawGameObject3DNormal(CGameObject3D* PtrGO)
{
	if (PtrGO->ComponentRender.PtrObject3D == nullptr) return;

	assert(PtrGO->ComponentRender.PtrVS);
	assert(PtrGO->ComponentRender.PtrPS);

	CShader* VS{ PtrGO->ComponentRender.PtrVS };
	CShader* PS{ m_PSVertexColor.get() };

	m_cbVSSpaceData.World = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld);
	m_cbVSSpaceData.WVP = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);
	PtrGO->UpdateWorldMatrix();

	m_cbPSBaseFlagsData.bUseLighting = FALSE;
	m_cbPSBaseFlagsData.bUseTexture = FALSE;

	VS->Use();
	VS->UpdateAllConstantBuffers();

	m_GSNormal->Use();

	PS->Use();
	PS->UpdateAllConstantBuffers();

	PtrGO->ComponentRender.PtrObject3D->DrawNormals();
}

void CGame::DrawGameObject3DBoundingSphere(CGameObject3D* PtrGO)
{
	m_VSBase->Use();

	XMMATRIX Translation{ XMMatrixTranslationFromVector(PtrGO->ComponentTransform.Translation + PtrGO->ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX Scaling{ XMMatrixScaling(PtrGO->ComponentPhysics.BoundingSphere.Radius,
		PtrGO->ComponentPhysics.BoundingSphere.Radius, PtrGO->ComponentPhysics.BoundingSphere.Radius) };
	XMMATRIX World{ Scaling * Translation };
	m_cbVSSpaceData.World = XMMatrixTranspose(World);
	m_cbVSSpaceData.WVP = XMMatrixTranspose(World * m_MatrixView * m_MatrixProjection);
	m_VSBase->UpdateConstantBuffer(0);

	m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

	m_Object3DBoundingSphere->Draw();

	SetUniversalRasterizerState();
}

void CGame::DrawGameObject3DLines()
{
	m_VSLine->Use();
	m_PSLine->Use();

	for (auto& i : m_vGameObject3DLines)
	{
		CGameObject3DLine* GOLine{ i.get() };

		if (GOLine->ComponentRender.PtrObject3DLine)
		{
			if (!GOLine->bIsVisible) continue;

			GOLine->UpdateWorldMatrix();

			m_cbVSSpaceData.World = XMMatrixTranspose(GOLine->ComponentTransform.MatrixWorld);
			m_cbVSSpaceData.WVP = XMMatrixTranspose(GOLine->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);
			m_VSLine->UpdateConstantBuffer(0);

			GOLine->ComponentRender.PtrObject3DLine->Draw();
		}
	}
}

void CGame::DrawGameObject2Ds()
{
	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);
	
	m_cbVS2DSpaceData.Projection = m_MatrixProjection2D;

	m_VSBase2D->Use();
	m_PSBase2D->Use();

	for (auto& i : m_vGameObject2Ds)
	{
		CGameObject2D* GO2D{ i.get() };

		if (GO2D->ComponentRender.PtrObject2D)
		{
			if (!GO2D->bIsVisible) continue;

			GO2D->UpdateWorldMatrix();
			m_cbVS2DSpaceData.World = XMMatrixTranspose(GO2D->ComponentTransform.MatrixWorld);
			m_VSBase2D->UpdateConstantBuffer(0);

			if (GO2D->ComponentRender.PtrTexture)
			{
				GO2D->ComponentRender.PtrTexture->Use();
				m_cbPS2DFlagsData.bUseTexture = TRUE;
				m_PSBase2D->UpdateConstantBuffer(0);
			}
			else
			{
				m_cbPS2DFlagsData.bUseTexture = FALSE;
				m_PSBase2D->UpdateConstantBuffer(0);
			}

			GO2D->ComponentRender.PtrObject2D->Draw();
		}
	}

	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
}

void CGame::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	for (auto& i : m_vGameObject3DMiniAxes)
	{
		UpdateGameObject3D(i.get());
		DrawGameObject3D(i.get());

		i->ComponentTransform.Translation = 
			m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition +
			m_vCameras[m_CurrentCameraIndex].m_CameraData.Forward;

		i->UpdateWorldMatrix();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGame::UpdatePickingRay()
{
	m_Object3DLinePickingRay->vVertices[0].Position = m_PickingRayWorldSpaceOrigin;
	m_Object3DLinePickingRay->vVertices[1].Position = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * KPickingRayLength;
	m_Object3DLinePickingRay->Update();
}

void CGame::DrawPickingRay()
{
	m_VSLine->Use();
	m_cbVSSpaceData.World = XMMatrixTranspose(KMatrixIdentity);
	m_cbVSSpaceData.WVP = XMMatrixTranspose(KMatrixIdentity * m_MatrixView * m_MatrixProjection);
	m_VSLine->UpdateConstantBuffer(0);

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSLine->Use();

	m_Object3DLinePickingRay->Draw();
}

void CGame::DrawPickedTriangle()
{
	m_VSBase->Use();
	m_cbVSSpaceData.World = XMMatrixTranspose(KMatrixIdentity);
	m_cbVSSpaceData.WVP = XMMatrixTranspose(KMatrixIdentity * m_MatrixView * m_MatrixProjection);
	m_VSBase->UpdateConstantBuffer(0);

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSVertexColor->Use();

	m_Object3DPickedTriangle->m_Model.vMeshes[0].vVertices[0].Position = m_PickedTriangleV0;
	m_Object3DPickedTriangle->m_Model.vMeshes[0].vVertices[1].Position = m_PickedTriangleV1;
	m_Object3DPickedTriangle->m_Model.vMeshes[0].vVertices[2].Position = m_PickedTriangleV2;
	m_Object3DPickedTriangle->UpdateMeshBuffer();

	m_Object3DPickedTriangle->Draw();
}

void CGame::DrawSky(float DeltaTime)
{
	// Elapse SkyTime
	m_cbPSSkyTimeData.SkyTime += KSkyTimeFactorAbsolute * DeltaTime;
	if (m_cbPSSkyTimeData.SkyTime > 1.0f) m_cbPSSkyTimeData.SkyTime = 0.0f;

	// Update directional light source position
	static float DirectionalLightRoll{};
	if (m_cbPSSkyTimeData.SkyTime <= 0.25f)
	{
		DirectionalLightRoll = XM_2PI * (m_cbPSSkyTimeData.SkyTime + 0.25f);
	}
	else if (m_cbPSSkyTimeData.SkyTime > 0.25f && m_cbPSSkyTimeData.SkyTime <= 0.75f)
	{
		DirectionalLightRoll = XM_2PI * (m_cbPSSkyTimeData.SkyTime - 0.25f);
	}
	else
	{
		DirectionalLightRoll = XM_2PI * (m_cbPSSkyTimeData.SkyTime - 0.75f);
	}
	XMVECTOR DirectionalLightSourcePosition{ XMVector3TransformCoord(XMVectorSet(1, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, DirectionalLightRoll)) };
	SetDirectionalLight(DirectionalLightSourcePosition);

	// SkySphere
	{
		m_GameObject3DSkySphere->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
		m_GameObject3DSkySphere->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition;
	}

	// Sun
	{
		float SunRoll{ XM_2PI * m_cbPSSkyTimeData.SkyTime - XM_PIDIV2 };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, SunRoll)) };
		m_GameObject3DSun->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + Offset;
		m_GameObject3DSun->ComponentTransform.Roll = SunRoll;
	}

	// Moon
	{
		float MoonRoll{ XM_2PI * m_cbPSSkyTimeData.SkyTime + XM_PIDIV2 };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, MoonRoll)) };
		m_GameObject3DMoon->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + Offset;
		m_GameObject3DMoon->ComponentTransform.Roll = (MoonRoll > XM_2PI) ? (MoonRoll - XM_2PI) : MoonRoll;
	}

	// Cloud
	{
		float CloudYaw{ XM_2PI * m_cbPSSkyTimeData.SkyTime };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, CloudYaw, XM_PIDIV4)) };
		m_GameObject3DCloud->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + Offset;
		m_GameObject3DCloud->ComponentTransform.Yaw = CloudYaw;
		m_GameObject3DCloud->ComponentTransform.Roll = XM_PIDIV4;
	}

	UpdateGameObject3D(m_GameObject3DSkySphere.get());
	DrawGameObject3D(m_GameObject3DSkySphere.get());

	UpdateGameObject3D(m_GameObject3DSun.get());
	DrawGameObject3D(m_GameObject3DSun.get());

	UpdateGameObject3D(m_GameObject3DMoon.get());
	DrawGameObject3D(m_GameObject3DMoon.get());

	UpdateGameObject3D(m_GameObject3DCloud.get());
	DrawGameObject3D(m_GameObject3DCloud.get());
}

bool CGame::ShouldSelectRotationGizmo(CGameObject3D* Gizmo, E3DGizmoAxis Axis)
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

bool CGame::ShouldSelectTranslationScalingGizmo(CGameObject3D* Gizmo, E3DGizmoAxis Axis)
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

void CGame::Draw3DGizmos()
{
	const Mouse::State& MouseState{ m_Mouse->GetState() };
	static int PrevMouseX{ MouseState.x };
	static int PrevMouseY{ MouseState.y };

	if (!MouseState.leftButton)
	{
		m_bIsGizmoSelected = false;
	}

	if (MouseState.rightButton)
	{
		m_PtrCapturedPickedGameObject3D = nullptr;
	}

	if (m_PtrCapturedPickedGameObject3D)
	{
		m_3DGizmoDistanceScalar =
			XMVectorGetX(XMVector3Length(m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition - 
				m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation)) * 0.1f;
		m_3DGizmoDistanceScalar = pow(m_3DGizmoDistanceScalar, 0.7f);

		if (m_bIsGizmoSelected)
		{
			if (MouseState.leftButton)
			{
				int DeltaX{ MouseState.x - PrevMouseX };
				int DeltaY{ MouseState.y - PrevMouseY };

				int DeltaSum{ DeltaX + DeltaY };

				float TranslationX{ XMVectorGetX(m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation) };
				float TranslationY{ XMVectorGetY(m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation) };
				float TranslationZ{ XMVectorGetZ(m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation) };

				float ScalingX{ XMVectorGetX(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling) };
				float ScalingY{ XMVectorGetY(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling) };
				float ScalingZ{ XMVectorGetZ(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling) };

				switch (m_e3DGizmoMode)
				{
				case E3DGizmoMode::Translation:
					switch (m_e3DGizmoSelectedAxis)
					{
					case E3DGizmoAxis::AxisX:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation =
							XMVectorSetX(m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation, TranslationX - DeltaSum * CGame::KTranslationUnit);
						break;
					case E3DGizmoAxis::AxisY:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation =
							XMVectorSetY(m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation, TranslationY - DeltaSum * CGame::KTranslationUnit);
						break;
					case E3DGizmoAxis::AxisZ:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation =
							XMVectorSetZ(m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation, TranslationZ - DeltaSum * CGame::KTranslationUnit);
						break;
					default:
						break;
					}

					Draw3DGizmoTranslations(m_e3DGizmoSelectedAxis);

					break;
				case E3DGizmoMode::Rotation:
					switch (m_e3DGizmoSelectedAxis)
					{
					case E3DGizmoAxis::AxisX:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Pitch -= DeltaSum * CGame::KRotation360To2PI;
						break;
					case E3DGizmoAxis::AxisY:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Yaw -= DeltaSum * CGame::KRotation360To2PI;
						break;
					case E3DGizmoAxis::AxisZ:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Roll -= DeltaSum * CGame::KRotation360To2PI;
						break;
					default:
						break;
					}

					Draw3DGizmoRotations(m_e3DGizmoSelectedAxis);
					break;
				case E3DGizmoMode::Scaling:
					switch (m_e3DGizmoSelectedAxis)
					{
					case E3DGizmoAxis::AxisX:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling =
							XMVectorSetX(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling, ScalingX - DeltaSum * CGame::KScalingUnit);
						break;
					case E3DGizmoAxis::AxisY:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling =
							XMVectorSetY(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling, ScalingY - DeltaSum * CGame::KScalingUnit);
						break;
					case E3DGizmoAxis::AxisZ:
						m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling =
							XMVectorSetZ(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling, ScalingZ - DeltaSum * CGame::KScalingUnit);
						break;
					default:
						break;
					}

					ScalingX = XMVectorGetX(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling);
					ScalingY = XMVectorGetY(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling);
					ScalingZ = XMVectorGetZ(m_PtrCapturedPickedGameObject3D->ComponentTransform.Scaling);

					m_PtrCapturedPickedGameObject3D->ComponentPhysics.BoundingSphere.Radius =
						max(max(ScalingX, ScalingY), ScalingZ);

					Draw3DGizmoScalings(m_e3DGizmoSelectedAxis);

					break;
				default:
					break;
				}
				
			}
		}
		else
		{
			CastPickingRay();

			switch (m_e3DGizmoMode)
			{
			case E3DGizmoMode::Translation:
				m_GameObject3D_3DGizmoTranslationX->ComponentTransform.Translation =
					m_GameObject3D_3DGizmoTranslationY->ComponentTransform.Translation =
					m_GameObject3D_3DGizmoTranslationZ->ComponentTransform.Translation = m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation;

				m_bIsGizmoSelected = true;
				if (ShouldSelectTranslationScalingGizmo(m_GameObject3D_3DGizmoTranslationX.get(), E3DGizmoAxis::AxisX))
				{
					Draw3DGizmoTranslations(E3DGizmoAxis::AxisX);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
				}
				else if (ShouldSelectTranslationScalingGizmo(m_GameObject3D_3DGizmoTranslationY.get(), E3DGizmoAxis::AxisY))
				{
					Draw3DGizmoTranslations(E3DGizmoAxis::AxisY);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
				}
				else if (ShouldSelectTranslationScalingGizmo(m_GameObject3D_3DGizmoTranslationZ.get(), E3DGizmoAxis::AxisZ))
				{
					Draw3DGizmoTranslations(E3DGizmoAxis::AxisZ);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
				}
				else
				{
					Draw3DGizmoTranslations(E3DGizmoAxis::None);
					m_bIsGizmoSelected = false;
				}

				break;
			case E3DGizmoMode::Rotation:
				m_GameObject3D_3DGizmoRotationPitch->ComponentTransform.Translation =
					m_GameObject3D_3DGizmoRotationYaw->ComponentTransform.Translation =
					m_GameObject3D_3DGizmoRotationRoll->ComponentTransform.Translation = m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation;

				m_bIsGizmoSelected = true;
				if (ShouldSelectRotationGizmo(m_GameObject3D_3DGizmoRotationPitch.get(), E3DGizmoAxis::AxisX))
				{
					Draw3DGizmoRotations(E3DGizmoAxis::AxisX);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
				}
				else if (ShouldSelectRotationGizmo(m_GameObject3D_3DGizmoRotationYaw.get(), E3DGizmoAxis::AxisY))
				{
					Draw3DGizmoRotations(E3DGizmoAxis::AxisY);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
				}
				else if (ShouldSelectRotationGizmo(m_GameObject3D_3DGizmoRotationRoll.get(), E3DGizmoAxis::AxisZ))
				{
					Draw3DGizmoRotations(E3DGizmoAxis::AxisZ);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
				}
				else
				{
					Draw3DGizmoRotations(E3DGizmoAxis::None);
					m_bIsGizmoSelected = false;
				}

				break;
			case E3DGizmoMode::Scaling:
				m_GameObject3D_3DGizmoScalingX->ComponentTransform.Translation =
					m_GameObject3D_3DGizmoScalingY->ComponentTransform.Translation =
					m_GameObject3D_3DGizmoScalingZ->ComponentTransform.Translation = m_PtrCapturedPickedGameObject3D->ComponentTransform.Translation;

				m_bIsGizmoSelected = true;
				if (ShouldSelectTranslationScalingGizmo(m_GameObject3D_3DGizmoScalingX.get(), E3DGizmoAxis::AxisX))
				{
					Draw3DGizmoScalings(E3DGizmoAxis::AxisX);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
				}
				else if (ShouldSelectTranslationScalingGizmo(m_GameObject3D_3DGizmoScalingY.get(), E3DGizmoAxis::AxisY))
				{
					Draw3DGizmoScalings(E3DGizmoAxis::AxisY);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
				}
				else if (ShouldSelectTranslationScalingGizmo(m_GameObject3D_3DGizmoScalingZ.get(), E3DGizmoAxis::AxisZ))
				{
					Draw3DGizmoScalings(E3DGizmoAxis::AxisZ);
					m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
				}
				else
				{
					Draw3DGizmoScalings(E3DGizmoAxis::None);
					m_bIsGizmoSelected = false;
				}

				break;
			default:
				break;
			}
			
		}
	}

	if (m_PtrPickedGameObject3D)
	{
		if (MouseState.leftButton && !m_bIsGizmoSelected)
		{
			m_PtrCapturedPickedGameObject3D = m_PtrPickedGameObject3D;
		}
	}

	PrevMouseX = MouseState.x;
	PrevMouseY = MouseState.y;

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
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

	Draw3DGizmo(m_GameObject3D_3DGizmoRotationPitch.get(), bHighlightX);
	Draw3DGizmo(m_GameObject3D_3DGizmoRotationYaw.get(), bHighlightY);
	Draw3DGizmo(m_GameObject3D_3DGizmoRotationRoll.get(), bHighlightZ);
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

	Draw3DGizmo(m_GameObject3D_3DGizmoTranslationX.get(), bHighlightX);
	Draw3DGizmo(m_GameObject3D_3DGizmoTranslationY.get(), bHighlightY);
	Draw3DGizmo(m_GameObject3D_3DGizmoTranslationZ.get(), bHighlightZ);
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

	Draw3DGizmo(m_GameObject3D_3DGizmoScalingX.get(), bHighlightX);
	Draw3DGizmo(m_GameObject3D_3DGizmoScalingY.get(), bHighlightY);
	Draw3DGizmo(m_GameObject3D_3DGizmoScalingZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmo(CGameObject3D* Gizmo, bool bShouldHighlight)
{
	CShader* VS{ Gizmo->ComponentRender.PtrVS };
	CShader* PS{ Gizmo->ComponentRender.PtrPS };

	float Scalar{ XMVectorGetX(XMVector3Length(m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition - Gizmo->ComponentTransform.Translation)) * 0.1f };
	Scalar = pow(Scalar, 0.7f);

	Gizmo->ComponentTransform.Scaling = XMVectorSet(Scalar, Scalar, Scalar, 0.0f);
	Gizmo->UpdateWorldMatrix();
	m_cbVSSpaceData.World = XMMatrixTranspose(Gizmo->ComponentTransform.MatrixWorld);
	m_cbVSSpaceData.WVP = XMMatrixTranspose(Gizmo->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);
	VS->UpdateConstantBuffer(0);
	VS->Use();

	if (bShouldHighlight)
	{
		m_cbPSGizmoColorFactorData.ColorFactor = XMVectorSet(1.0f, 1.0f, 1.0f, 0.95f);
	}
	else
	{
		m_cbPSGizmoColorFactorData.ColorFactor = XMVectorSet(0.75f, 0.75f, 0.75f, 0.75f);
	}
	PS->UpdateConstantBuffer(0);
	PS->Use();

	Gizmo->ComponentRender.PtrObject3D->Draw();
}

void CGame::SetUniversalRasterizerState()
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
	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::UseLighting))
	{
		m_cbPSBaseFlagsData.bUseLighting = TRUE;
	}
}

void CGame::EndRendering()
{
	m_SwapChain->Present(0, 0);
}

Keyboard::State CGame::GetKeyState()
{
	return m_Keyboard->GetState();
}

Mouse::State CGame::GetMouseState()
{ 
	Mouse::State ResultState{ m_Mouse->GetState() };
	
	m_Mouse->ResetScrollWheelValue();

	return ResultState;
}


const char* CGame::GetPickedGameObject3DName()
{
	if (m_PtrPickedGameObject3D)
	{
		return m_PtrPickedGameObject3D->m_Name.c_str();
	}
	return nullptr;
}

const char* CGame::GetCapturedPickedGameObject3DName()
{
	if (m_PtrCapturedPickedGameObject3D)
	{
		return m_PtrCapturedPickedGameObject3D->m_Name.c_str();
	}
	return nullptr;
}

float CGame::GetSkyTime()
{
	return m_cbPSSkyTimeData.SkyTime;
}
