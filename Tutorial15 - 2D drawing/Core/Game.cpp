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

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition)
{
	cbPSBaseLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);

	PSBase->UpdateConstantBuffer(1);
}

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color)
{
	cbPSBaseLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	cbPSBaseLightsData.DirectionalColor = Color;

	PSBase->UpdateConstantBuffer(1);
}

void CGame::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	cbPSBaseLightsData.AmbientLightColor = Color;
	cbPSBaseLightsData.AmbientLightIntensity = Intensity;

	PSBase->UpdateConstantBuffer(1);
}

void CGame::SetGameObjectSky(CGameObject* GameObject)
{
	m_PtrSky = GameObject;
}

void CGame::SetGameObjectCloud(CGameObject* GameObject)
{
	m_PtrCloud = GameObject;
}

void CGame::SetGameObjectSun(CGameObject* GameObject)
{
	m_PtrSun = GameObject;
}

void CGame::SetGameObjectMoon(CGameObject* GameObject)
{
	m_PtrMoon = GameObject;
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

	CreateShaders();

	CreateMiniAxes();

	CreatePickingRay();
	CreatePickedTriangle();

	CreateBoundingSphere();

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

void CGame::CreateShaders()
{
	VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	VSBase->Create(EShaderType::VertexShader, L"Shader\\VSBase.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	VSBase->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));
	
	VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	VSAnimation->Create(EShaderType::VertexShader, L"Shader\\VSAnimation.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	VSAnimation->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));
	VSAnimation->AddConstantBuffer(&cbVSAnimationBonesData, sizeof(SCBVSAnimationBonesData));

	VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	VSSky->Create(EShaderType::VertexShader, L"Shader\\VSSky.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	VSSky->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));

	VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	VSLine->Create(EShaderType::VertexShader, L"Shader\\VSLine.hlsl", "main", KVSLineInputElementDescs, ARRAYSIZE(KVSLineInputElementDescs));
	VSLine->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));

	VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	VSBase2D->Create(EShaderType::VertexShader, L"Shader\\VSBase2D.hlsl", "main", KVS2DBaseInputLayout, ARRAYSIZE(KVS2DBaseInputLayout));
	VSBase2D->AddConstantBuffer(&cbVS2DSpaceData, sizeof(SCBVS2DSpaceData));

	GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormal.hlsl", "main");

	PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	PSBase->Create(EShaderType::PixelShader, L"Shader\\PSBase.hlsl", "main");
	PSBase->AddConstantBuffer(&cbPSBaseFlagsData, sizeof(SCBPSBaseFlagsData));
	PSBase->AddConstantBuffer(&cbPSBaseLightsData, sizeof(SCBPSBaseLightsData));
	PSBase->AddConstantBuffer(&cbPSBaseMaterialData, sizeof(SCBPSBaseMaterialData));
	PSBase->AddConstantBuffer(&cbPSBaseEyeData, sizeof(SCBPSBaseEyeData));

	PSVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	PSVertexColor->Create(EShaderType::PixelShader, L"Shader\\PSVertexColor.hlsl", "main");

	PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	PSSky->Create(EShaderType::PixelShader, L"Shader\\PSSky.hlsl", "main");
	PSSky->AddConstantBuffer(&cbPSSkyTimeData, sizeof(SCBPSSkyTimeData));

	PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	PSLine->Create(EShaderType::PixelShader, L"Shader\\PSLine.hlsl", "main");

	PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	PSBase2D->Create(EShaderType::PixelShader, L"Shader\\PSBase2D.hlsl", "main");
	PSBase2D->AddConstantBuffer(&cbPS2DFlagsData, sizeof(SCBPS2DFlagsData));
}

void CGame::CreateMiniAxes()
{
	m_vMiniAxisObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	m_vMiniAxisObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	m_vMiniAxisObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));

	SMesh Cone{ GenerateCone(0, 16) };
	vector<SMaterial> vMaterials{ SMaterial(XMFLOAT3(1, 0, 0)), SMaterial(XMFLOAT3(0, 1, 0)), SMaterial(XMFLOAT3(0, 0, 1)) };
	m_vMiniAxisObject3Ds[0]->Create(Cone, vMaterials[0]);
	m_vMiniAxisObject3Ds[1]->Create(Cone, vMaterials[1]);
	m_vMiniAxisObject3Ds[2]->Create(Cone, vMaterials[2]);

	m_vMiniAxisGameObjects.emplace_back(make_unique<CGameObject>("AxisX"));
	m_vMiniAxisGameObjects.emplace_back(make_unique<CGameObject>("AxisY"));
	m_vMiniAxisGameObjects.emplace_back(make_unique<CGameObject>("AxisZ"));
	m_vMiniAxisGameObjects[0]->ComponentRender.PtrObject3D = m_vMiniAxisObject3Ds[0].get();
	m_vMiniAxisGameObjects[0]->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(0, 0, -XM_PIDIV2);
	m_vMiniAxisGameObjects[1]->ComponentRender.PtrObject3D = m_vMiniAxisObject3Ds[1].get();
	m_vMiniAxisGameObjects[2]->ComponentRender.PtrObject3D = m_vMiniAxisObject3Ds[2].get();
	m_vMiniAxisGameObjects[2]->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(0, -XM_PIDIV2, -XM_PIDIV2);
	
	m_vMiniAxisGameObjects[0]->ComponentTransform.Scaling = 
		m_vMiniAxisGameObjects[1]->ComponentTransform.Scaling =
		m_vMiniAxisGameObjects[2]->ComponentTransform.Scaling = XMVectorSet(0.1f, 1.0f, 0.1f, 0);

	m_vMiniAxisGameObjects[0]->UpdateWorldMatrix();
	m_vMiniAxisGameObjects[1]->UpdateWorldMatrix();
	m_vMiniAxisGameObjects[2]->UpdateWorldMatrix();
}

void CGame::CreatePickingRay()
{
	m_ObjectLinePickingRay = make_unique<CObjectLine>(m_Device.Get(), m_DeviceContext.Get());

	vector<SVertexLine> Vertices{};
	Vertices.emplace_back(XMVectorSet(0, 0, 0, 1), XMVectorSet(1, 0, 0, 1));
	Vertices.emplace_back(XMVectorSet(10.0f, 10.0f, 0, 1), XMVectorSet(0, 1, 0, 1));

	m_ObjectLinePickingRay->Create(Vertices);
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

void CGame::PickBoundingSphere()
{
	m_PtrPickedGameObject = nullptr;

	XMVECTOR T{ KVectorGreatest };
	for (auto& i : m_vGameObjects)
	{
		auto* GO{ i.get() };
		if (GO->ComponentPhysics.bIsPickable)
		{
			XMVECTOR NewT{ KVectorGreatest };
			if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
				GO->ComponentPhysics.BoundingSphere.Radius, GO->ComponentTransform.Translation + GO->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
			{
				if (XMVector3Less(NewT, T))
				{
					T = NewT;
					m_PtrPickedGameObject = GO;
				}
			}
		}
	}
}

void CGame::PickTriangle()
{
	XMVECTOR T{ KVectorGreatest };
	if (m_PtrPickedGameObject)
	{
		assert(m_PtrPickedGameObject->ComponentRender.PtrObject3D);

		// Pick only static models' triangle.
		if (m_PtrPickedGameObject->ComponentRender.PtrObject3D->m_Model.bIsAnimated) return;

		const XMMATRIX& World{ m_PtrPickedGameObject->ComponentTransform.MatrixWorld };
		for (auto& Mesh : m_PtrPickedGameObject->ComponentRender.PtrObject3D->m_Model.vMeshes)
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
					}
				}
			}
		}
	}
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

CGameObject* CGame::AddGameObject(const string& Name)
{
	assert(m_mapGameObjectNameToIndex.find(Name) == m_mapGameObjectNameToIndex.end());

	m_vGameObjects.emplace_back(make_unique<CGameObject>(Name));

	m_mapGameObjectNameToIndex[Name] = m_vGameObjects.size() - 1;

	return m_vGameObjects.back().get();
}

CGameObject* CGame::GetGameObject(const string& Name)
{
	assert(m_mapGameObjectNameToIndex.find(Name) != m_mapGameObjectNameToIndex.end());
	return m_vGameObjects[m_mapGameObjectNameToIndex[Name]].get();
}

CGameObject* CGame::GetGameObject(size_t Index)
{
	assert(Index < m_vGameObjects.size());
	return m_vGameObjects[Index].get();
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

void CGame::Pick(int ScreenMousePositionX, int ScreenMousePositionY)
{
	float ViewSpaceRayDirectionX{ (ScreenMousePositionX / (m_WindowSize.x / 2.0f) - 1.0f) / XMVectorGetX(m_MatrixProjection.r[0]) };
	float ViewSpaceRayDirectionY{ (-(ScreenMousePositionY / (m_WindowSize.y / 2.0f) - 1.0f)) / XMVectorGetY(m_MatrixProjection.r[1]) };
	float ViewSpaceRayDirectionZ{ 1.0f };

	XMVECTOR ViewSpaceRayOrigin{ XMVectorSet(0, 0, 0, 1) };
	XMVECTOR ViewSpaceRayDirection{ XMVectorSet(ViewSpaceRayDirectionX, ViewSpaceRayDirectionY, ViewSpaceRayDirectionZ, 0) };

	XMMATRIX MatrixViewInverse{ XMMatrixInverse(nullptr, m_MatrixView) };
	m_PickingRayWorldSpaceOrigin = XMVector3TransformCoord(ViewSpaceRayOrigin, MatrixViewInverse);
	m_PickingRayWorldSpaceDirection = XMVector3TransformNormal(ViewSpaceRayDirection, MatrixViewInverse);

	UpdatePickingRay();

	PickBoundingSphere();

	PickTriangle();
}

const char* CGame::GetPickedGameObjectName()
{
	if (m_PtrPickedGameObject)
	{
		return m_PtrPickedGameObject->m_Name.c_str();
	}
	return nullptr;
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

void CGame::AnimateGameObjects()
{
	for (auto& go : m_vGameObjects)
	{
		if (go->ComponentRender.PtrObject3D) go->ComponentRender.PtrObject3D->Animate();
	}
}

void CGame::DrawGameObjects(float DeltaTime)
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	cbPSBaseEyeData.EyePosition = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition;

	// Update directional light source position
	static float DirectionalLightRoll{};
	DirectionalLightRoll += XM_2PI * DeltaTime * KSkyTimeFactorAbsolute / 4.0f;
	if (DirectionalLightRoll >= XM_PIDIV2) DirectionalLightRoll = -XM_PIDIV2;
	XMVECTOR DirectionalLightSourcePosition{ XMVector3TransformCoord(XMVectorSet(0, 1, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, DirectionalLightRoll)) };
	SetDirectionalLight(DirectionalLightSourcePosition);

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawWireFrame))
	{
		m_eRasterizerState = ERasterizerState::WireFrame;
	}
	else
	{
		m_eRasterizerState = ERasterizerState::CullCounterClockwise;
	}

	for (auto& go : m_vGameObjects)
	{
		if (go->ComponentRender.IsTransparent) continue;

		UpdateGameObject(go.get(), DeltaTime);
		DrawGameObject(go.get());
		DrawGameObjectBoundingSphere(go.get());
	}

	for (auto& go : m_vGameObjects)
	{
		if (!go->ComponentRender.IsTransparent) continue;

		UpdateGameObject(go.get(), DeltaTime);
		DrawGameObject(go.get());
		DrawGameObjectBoundingSphere(go.get());
	}

	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawNormals))
	{
		VSBase->Use();
		GSNormal->Use();
		PSVertexColor->Use();

		for (auto& go : m_vGameObjects)
		{
			DrawGameObjectNormals(go.get());
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
}

void CGame::DrawGameObject2Ds(float DeltaTime)
{
	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);
	
	cbVS2DSpaceData.Projection = m_MatrixProjection2D;

	VSBase2D->Use();
	PSBase2D->Use();

	for (auto& i : m_vGameObject2Ds)
	{
		CGameObject2D* GO2D{ i.get() };

		if (!GO2D->bIsVisible) continue;

		GO2D->UpdateWorldMatrix();
		cbVS2DSpaceData.World = GO2D->ComponentTransform.MatrixWorld;
		VSBase2D->UpdateConstantBuffer(0);

		if (GO2D->ComponentRender.PtrTexture)
		{
			GO2D->ComponentRender.PtrTexture->Use();
			cbPS2DFlagsData.bUseTexture = TRUE;
			PSBase2D->UpdateConstantBuffer(0);
		}
		else
		{
			cbPS2DFlagsData.bUseTexture = FALSE;
			PSBase2D->UpdateConstantBuffer(0);
		}

		if (GO2D->ComponentRender.PtrObject2D) GO2D->ComponentRender.PtrObject2D->Draw();
	}
}

void CGame::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	VSBase->Use();
	PSBase->Use();
	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);

	cbPSBaseFlagsData.bUseTexture = FALSE;
	cbPSBaseFlagsData.bUseLighting = FALSE;
	PSBase->UpdateConstantBuffer(0);

	for (auto& i : m_vMiniAxisGameObjects)
	{
		cbVSSpaceData.World = XMMatrixTranspose(i->ComponentTransform.MatrixWorld);
		cbVSSpaceData.WVP = XMMatrixTranspose(i->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);
		VSBase->UpdateConstantBuffer(0);

		i->ComponentRender.PtrObject3D->Draw();

		i->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + 
			m_vCameras[m_CurrentCameraIndex].m_CameraData.Forward;

		i->UpdateWorldMatrix();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGame::DrawPickingRay()
{
	m_DeviceContext->RSSetState(m_CommonStates->CullNone());

	VSLine->Use();
	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	PSLine->Use();

	cbVSSpaceData.World = XMMatrixTranspose(KMatrixIdentity);
	cbVSSpaceData.WVP = XMMatrixTranspose(KMatrixIdentity * m_MatrixView * m_MatrixProjection);
	VSLine->UpdateConstantBuffer(0);

	m_ObjectLinePickingRay->Draw();
}

void CGame::DrawPickedTriangle()
{
	VSBase->Use();
	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	PSVertexColor->Use();

	cbVSSpaceData.World = XMMatrixTranspose(KMatrixIdentity);
	cbVSSpaceData.WVP = XMMatrixTranspose(KMatrixIdentity * m_MatrixView * m_MatrixProjection);
	VSBase->UpdateConstantBuffer(0);

	m_Object3DPickedTriangle->m_Model.vMeshes[0].vVertices[0].Position = m_PickedTriangleV0;
	m_Object3DPickedTriangle->m_Model.vMeshes[0].vVertices[1].Position = m_PickedTriangleV1;
	m_Object3DPickedTriangle->m_Model.vMeshes[0].vVertices[2].Position = m_PickedTriangleV2;
	m_Object3DPickedTriangle->UpdateMeshBuffer();

	m_Object3DPickedTriangle->Draw();
}

void CGame::UpdateGameObject(CGameObject* PtrGO, float DeltaTime)
{
	CShader* VS{ VSBase.get() };
	CShader* PS{ PSBase.get() };

	cbVSSpaceData.World = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld);
	cbVSSpaceData.WVP = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);

	// Set VS
	if (PtrGO->ComponentRender.PtrObject3D->m_Model.bIsAnimated)
	{
		VS = VSAnimation.get();
	}
	else
	{
		if (PtrGO == m_PtrSky || PtrGO == m_PtrCloud || PtrGO == m_PtrSun || PtrGO == m_PtrMoon)
		{
			VS = VSSky.get();
		}
	}
	
	// Set PS
	if (PtrGO == m_PtrSky)
	{
		PS = PSSky.get();
	}

	if (PtrGO == m_PtrSky)
	{
		static float SkyTime{ 1.0f };
		static float SkyTimeFactor{ KSkyTimeFactorAbsolute };

		SkyTime -= DeltaTime * SkyTimeFactor;
		if (SkyTime < -1.0f) SkyTimeFactor = -SkyTimeFactor;
		if (SkyTime > +1.0f) SkyTimeFactor = -SkyTimeFactor;

		PtrGO->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
		PtrGO->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition;

		cbPSSkyTimeData.SkyTime = SkyTime;
	}

	if (PtrGO == m_PtrCloud)
	{
		static float Yaw{};
		Yaw -= XM_2PI * DeltaTime * 0.01f;
		if (Yaw <= -XM_2PI) Yaw = 0;

		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(0, 0, KSkyDistance, 1), XMMatrixRotationRollPitchYaw(-XM_PIDIV4, Yaw, 0)) };
		PtrGO->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + Offset;

		PtrGO->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(-XM_PIDIV2 - XM_PIDIV4, Yaw, 0);
	}

	if (PtrGO == m_PtrSun)
	{
		static float SunRoll{};
		SunRoll += XM_2PI * DeltaTime * KSkyTimeFactorAbsolute / 4.0f;
		if (SunRoll >= XM_2PI) SunRoll = 0;

		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, XM_PIDIV2 + SunRoll)) };
		PtrGO->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + Offset;

		PtrGO->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2 + SunRoll);
	}

	if (PtrGO == m_PtrMoon)
	{
		static float MoonRoll{};
		MoonRoll += XM_2PI * DeltaTime * KSkyTimeFactorAbsolute / 4.0f;
		if (MoonRoll >= XM_2PI) MoonRoll = 0;

		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, -XM_PIDIV2 + MoonRoll)) };
		PtrGO->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].m_CameraData.EyePosition + Offset;

		PtrGO->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(0, 0, -XM_PIDIV2 + MoonRoll);
	}

	if (EFLAG_HAS(PtrGO->eFlagsGameObjectRendering, EFlagsGameObjectRendering::NoLighting))
	{
		cbPSBaseFlagsData.bUseLighting = FALSE;
	}
	else
	{
		SetUniversalbUseLighiting();
	}

	if (PtrGO->ComponentRender.PtrTexture)
	{
		cbPSBaseFlagsData.bUseTexture = TRUE;
		PtrGO->ComponentRender.PtrTexture->Use();
	}
	else
	{
		cbPSBaseFlagsData.bUseTexture = FALSE;
	}

	PtrGO->UpdateWorldMatrix();

	VS->Use();
	VS->UpdateAllConstantBuffers();

	PS->Use();
	PS->UpdateAllConstantBuffers();
}

void CGame::UpdatePickingRay()
{
	m_ObjectLinePickingRay->vVertices[0].Position = m_PickingRayWorldSpaceOrigin;
	m_ObjectLinePickingRay->vVertices[1].Position = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * KPickingRayLength;
	m_ObjectLinePickingRay->Update();
}

void CGame::DrawGameObject(CGameObject* PtrGO)
{
	assert(PtrGO->ComponentRender.PtrObject3D);

	if (EFLAG_HAS(PtrGO->eFlagsGameObjectRendering, EFlagsGameObjectRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRasterizerState();
	}

	if (EFLAG_HAS(PtrGO->eFlagsGameObjectRendering, EFlagsGameObjectRendering::NoDepthComparison))
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	}
	else
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
	}

	PtrGO->ComponentRender.PtrObject3D->Draw();
}

void CGame::DrawGameObjectNormals(CGameObject* PtrGO)
{
	if (PtrGO->ComponentRender.PtrObject3D)
	{
		if (PtrGO->ComponentRender.PtrObject3D->m_Model.bIsAnimated)
		{
			VSAnimation->Use();
		}
		else
		{
			VSBase->Use();
		}

		PtrGO->ComponentRender.PtrObject3D->DrawNormals();
	}
}

void CGame::DrawGameObjectBoundingSphere(CGameObject* PtrGO)
{
	if (EFLAG_HAS(m_eFlagsGameRendering, EFlagsGameRendering::DrawBoundingSphere))
	{
		if (PtrGO == m_PtrSky || PtrGO == m_PtrCloud || PtrGO == m_PtrSun || PtrGO == m_PtrMoon) return;

		VSBase->Use();

		XMMATRIX Translation{ XMMatrixTranslationFromVector(PtrGO->ComponentTransform.Translation + PtrGO->ComponentPhysics.BoundingSphere.CenterOffset) };
		XMMATRIX Scaling{ XMMatrixScaling(PtrGO->ComponentPhysics.BoundingSphere.Radius, 
			PtrGO->ComponentPhysics.BoundingSphere.Radius, PtrGO->ComponentPhysics.BoundingSphere.Radius) };
		XMMATRIX World{ Scaling * Translation };
		cbVSSpaceData.World = XMMatrixTranspose(World);
		cbVSSpaceData.WVP = XMMatrixTranspose(World * m_MatrixView * m_MatrixProjection);
		VSBase->UpdateConstantBuffer(0);

		m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

		m_Object3DBoundingSphere->Draw();

		SetUniversalRasterizerState();
	}
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
		cbPSBaseFlagsData.bUseLighting = TRUE;
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