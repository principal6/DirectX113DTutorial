#include "GameWindow.h"

constexpr D3D11_INPUT_ELEMENT_DESC KBaseInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT	, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 1, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void CGameWindow::CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, const wstring& FontFileName, bool bWindowed)
{
	CreateWin32Window(WndProc, WindowName);

	InitializeDirectX(FontFileName, bWindowed);
}

void CGameWindow::SetPerspective(float FOV, float NearZ, float FarZ)
{
	m_MatrixProjection = XMMatrixPerspectiveFovLH(FOV, m_WindowSize.x / m_WindowSize.y, NearZ, FarZ);
}

void CGameWindow::SetGameRenderingFlags(EFlagsGameRendering Flags)
{
	m_eFlagsGamerendering = Flags;
}

void CGameWindow::ToggleGameRenderingFlags(EFlagsGameRendering Flags)
{
	m_eFlagsGamerendering ^= Flags;
}

void CGameWindow::SetDirectionalLight(const XMVECTOR& LightSourcePosition)
{
	cbPSBaseLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);

	m_PSBase->UpdateConstantBuffer(1);
}

void CGameWindow::SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color)
{
	cbPSBaseLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	cbPSBaseLightsData.DirectionalColor = Color;

	m_PSBase->UpdateConstantBuffer(1);
}

void CGameWindow::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	cbPSBaseLightsData.AmbientLightColor = Color;
	cbPSBaseLightsData.AmbientLightIntensity = Intensity;

	m_PSBase->UpdateConstantBuffer(1);
}

void CGameWindow::SetGameObjectSky(CGameObject* GameObject)
{
	m_PtrSky = GameObject;
}

void CGameWindow::SetGameObjectCloud(CGameObject* GameObject)
{
	m_PtrCloud = GameObject;
}

void CGameWindow::SetGameObjectSun(CGameObject* GameObject)
{
	m_PtrSun = GameObject;
}

void CGameWindow::SetGameObjectMoon(CGameObject* GameObject)
{
	m_PtrMoon = GameObject;
}

void CGameWindow::AddCamera(const SCameraData& CameraData)
{
	m_vCameras.emplace_back(CameraData);
}

void CGameWindow::SetCamera(size_t Index)
{
	assert(Index < m_vCameras.size());

	m_PtrCurrentCamera = &m_vCameras[Index];

	m_BaseForward = m_PtrCurrentCamera->Forward = XMVector3Normalize(m_PtrCurrentCamera->FocusPosition - m_PtrCurrentCamera->EyePosition);
	m_BaseUp = m_PtrCurrentCamera->UpDirection;

	if (m_PtrCurrentCamera->CameraType == ECameraType::ThirdPerson)
	{
		m_PtrCurrentCamera->EyePosition = m_PtrCurrentCamera->FocusPosition - m_BaseForward * m_PtrCurrentCamera->Distance;
	}
}

void CGameWindow::MoveCamera(ECameraMovementDirection Direction, float StrideFactor)
{
	assert(m_PtrCurrentCamera);

	XMVECTOR dPosition{};

	if (m_PtrCurrentCamera->CameraType == ECameraType::FreeLook)
	{
		XMVECTOR Rightward{ XMVector3Normalize(XMVector3Cross(m_PtrCurrentCamera->UpDirection, m_PtrCurrentCamera->Forward)) };
		
		switch (Direction)
		{
		case ECameraMovementDirection::Forward:
			dPosition = +m_PtrCurrentCamera->Forward * StrideFactor;
			break;
		case ECameraMovementDirection::Backward:
			dPosition = -m_PtrCurrentCamera->Forward * StrideFactor;
			break;
		case ECameraMovementDirection::Rightward:
			dPosition = +Rightward * StrideFactor;
			break;
		case ECameraMovementDirection::Leftward:
			dPosition = -Rightward * StrideFactor;
			break;
		default:
			break;
		}
	}
	else if (m_PtrCurrentCamera->CameraType == ECameraType::FirstPerson || m_PtrCurrentCamera->CameraType == ECameraType::ThirdPerson)
	{
		XMVECTOR GroundRightward{ XMVector3Normalize(XMVector3Cross(m_BaseUp, m_PtrCurrentCamera->Forward)) };
		XMVECTOR GroundForward{ XMVector3Normalize(XMVector3Cross(GroundRightward, m_BaseUp)) };

		switch (Direction)
		{
		case ECameraMovementDirection::Forward:
			dPosition = +GroundForward * StrideFactor;
			break;
		case ECameraMovementDirection::Backward:
			dPosition = -GroundForward * StrideFactor;
			break;
		case ECameraMovementDirection::Rightward:
			dPosition = +GroundRightward * StrideFactor;
			break;
		case ECameraMovementDirection::Leftward:
			dPosition = -GroundRightward * StrideFactor;
			break;
		default:
			break;
		}
	}

	m_PtrCurrentCamera->EyePosition += dPosition;
	m_PtrCurrentCamera->FocusPosition += dPosition;
}

void CGameWindow::RotateCamera(int DeltaX, int DeltaY, float RotationFactor)
{
	assert(m_PtrCurrentCamera);

	m_PtrCurrentCamera->Pitch += RotationFactor * DeltaY;
	m_PtrCurrentCamera->Yaw += RotationFactor * DeltaX;

	static constexpr float KPitchLimit{ XM_PIDIV2 - 0.01f };
	m_PtrCurrentCamera->Pitch = max(-KPitchLimit, m_PtrCurrentCamera->Pitch);
	m_PtrCurrentCamera->Pitch = min(+KPitchLimit, m_PtrCurrentCamera->Pitch);

	if (m_PtrCurrentCamera->Yaw > XM_PI)
	{
		m_PtrCurrentCamera->Yaw -= XM_2PI;
	}
	else if (m_PtrCurrentCamera->Yaw < -XM_PI)
	{
		m_PtrCurrentCamera->Yaw += XM_2PI;
	}

	XMMATRIX MatrixRotation{ XMMatrixRotationRollPitchYaw(m_PtrCurrentCamera->Pitch, m_PtrCurrentCamera->Yaw, 0) };
	m_PtrCurrentCamera->Forward = XMVector3TransformNormal(m_BaseForward, MatrixRotation);
	XMVECTOR Rightward{ XMVector3Normalize(XMVector3Cross(m_BaseUp, m_PtrCurrentCamera->Forward)) };
	XMVECTOR Upward{ XMVector3Normalize(XMVector3Cross(m_PtrCurrentCamera->Forward, Rightward)) };

	m_PtrCurrentCamera->UpDirection = Upward;

	if (m_PtrCurrentCamera->CameraType == ECameraType::FirstPerson || m_PtrCurrentCamera->CameraType == ECameraType::FreeLook)
	{
		m_PtrCurrentCamera->FocusPosition = m_PtrCurrentCamera->EyePosition + m_PtrCurrentCamera->Forward;
	}
	else if (m_PtrCurrentCamera->CameraType == ECameraType::ThirdPerson)
	{
		m_PtrCurrentCamera->EyePosition = m_PtrCurrentCamera->FocusPosition - m_PtrCurrentCamera->Forward * m_PtrCurrentCamera->Distance;
	}
}

void CGameWindow::ZoomCamera(int DeltaWheel, float ZoomFactor)
{
	m_PtrCurrentCamera->Distance -= DeltaWheel * ZoomFactor;
	m_PtrCurrentCamera->Distance = max(m_PtrCurrentCamera->Distance, m_PtrCurrentCamera->MinDistance);
	m_PtrCurrentCamera->Distance = min(m_PtrCurrentCamera->Distance, m_PtrCurrentCamera->MaxDistance);

	m_PtrCurrentCamera->EyePosition = m_PtrCurrentCamera->FocusPosition - m_PtrCurrentCamera->Forward * m_PtrCurrentCamera->Distance;
}

void CGameWindow::CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName)
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

	m_hWnd = CreateWindowEx(0, KClassName, WindowName, KWindowStyle,
		CW_USEDEFAULT, 
		CW_USEDEFAULT,
		WindowRect.right - WindowRect.left, 
		WindowRect.bottom - WindowRect.top,
		nullptr, nullptr, m_hInstance, nullptr);

	ShowWindow(m_hWnd, SW_SHOW);

	UpdateWindow(m_hWnd);

	assert(m_hWnd);
}

void CGameWindow::InitializeDirectX(const wstring& FontFileName, bool bWindowed)
{
	CreateSwapChain(bWindowed);

	CreateSetViews();

	SetViewports();

	SetPerspective(KDefaultFOV, KDefaultNearZ, KDefaultFarZ);

	CreateInputDevices();

	CreateShaders();

	CreateMiniAxes();

	m_SpriteBatch = make_unique<SpriteBatch>(m_DeviceContext.Get());
	m_SpriteFont = make_unique<SpriteFont>(m_Device.Get(), FontFileName.c_str());
	m_CommonStates = make_unique<CommonStates>(m_Device.Get());
}

void CGameWindow::CreateSwapChain(bool bWindowed)
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

void CGameWindow::CreateSetViews()
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

void CGameWindow::SetViewports()
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

void CGameWindow::CreateInputDevices()
{
	m_Keyboard = make_unique<Keyboard>();

	m_Mouse = make_unique<Mouse>();
	m_Mouse->SetWindow(m_hWnd);

	m_Mouse->SetMode(Mouse::Mode::MODE_RELATIVE);
}

void CGameWindow::CreateShaders()
{
	m_VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase->Create(EShaderType::VertexShader, L"Shader\\VSBase.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSBase->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));
	
	m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSAnimation->Create(EShaderType::VertexShader, L"Shader\\VSAnimation.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSAnimation->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));
	m_VSAnimation->AddConstantBuffer(&cbVSAnimationBonesData, sizeof(SCBVSAnimationBonesData));

	m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSSky->Create(EShaderType::VertexShader, L"Shader\\VSSky.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSSky->AddConstantBuffer(&cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormal.hlsl", "main");

	m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase->Create(EShaderType::PixelShader, L"Shader\\PSBase.hlsl", "main");
	m_PSBase->AddConstantBuffer(&cbPSBaseFlagsData, sizeof(SCBPSBaseFlagsData));
	m_PSBase->AddConstantBuffer(&cbPSBaseLightsData, sizeof(SCBPSBaseLightsData));
	m_PSBase->AddConstantBuffer(&cbPSBaseMaterialData, sizeof(SCBPSBaseMaterialData));
	m_PSBase->AddConstantBuffer(&cbPSBaseEyeData, sizeof(SCBPSBaseEyeData));

	m_PSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSNormal->Create(EShaderType::PixelShader, L"Shader\\PSNormal.hlsl", "main");

	m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSSky->Create(EShaderType::PixelShader, L"Shader\\PSSky.hlsl", "main");
	m_PSSky->AddConstantBuffer(&cbPSSkyTimeData, sizeof(SCBPSSkyTimeData));
}

void CGameWindow::CreateMiniAxes()
{
	m_vMiniAxisObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	m_vMiniAxisObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	m_vMiniAxisObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));

	SMesh Cone{ GenerateCone(0, 16) };
	vector<SMaterial> vMaterials{ SMaterial(XMFLOAT3(1, 0, 0)), SMaterial(XMFLOAT3(0, 1, 0)), SMaterial(XMFLOAT3(0, 0, 1)) };
	m_vMiniAxisObject3Ds[0]->Create(Cone, vMaterials[0]);
	m_vMiniAxisObject3Ds[1]->Create(Cone, vMaterials[1]);
	m_vMiniAxisObject3Ds[2]->Create(Cone, vMaterials[2]);

	m_vMiniAxisGameObjects.emplace_back(make_unique<CGameObject>());
	m_vMiniAxisGameObjects.emplace_back(make_unique<CGameObject>());
	m_vMiniAxisGameObjects.emplace_back(make_unique<CGameObject>());
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

CShader* CGameWindow::AddShader()
{
	m_vShaders.emplace_back(make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vShaders.back().get();
}

CShader* CGameWindow::GetShader(size_t Index)
{
	assert(Index < m_vShaders.size());
	return m_vShaders[Index].get();
}

CObject3D* CGameWindow::AddObject3D()
{
	m_vObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get(), this));
	return m_vObject3Ds.back().get();
}

CObject3D* CGameWindow::GetObject3D(size_t Index)
{
	assert(Index < m_vObject3Ds.size());
	return m_vObject3Ds[Index].get();
}

CTexture* CGameWindow::AddTexture()
{
	m_vTextures.emplace_back(make_unique<CTexture>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vTextures.back().get();
}

CTexture* CGameWindow::GetTexture(size_t Index)
{
	assert(Index < m_vTextures.size());
	return m_vTextures[Index].get();
}

CGameObject* CGameWindow::AddGameObject()
{
	m_vGameObjects.emplace_back(make_unique<CGameObject>());
	return m_vGameObjects.back().get();
}

CGameObject* CGameWindow::GetGameObject(size_t Index)
{
	assert(Index < m_vGameObjects.size());
	return m_vGameObjects[Index].get();
}

void CGameWindow::SetRasterizerState(ERasterizerState State)
{
	m_eRasterizerState = State;
}

void CGameWindow::BeginRendering(const FLOAT* ClearColor)
{
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), Colors::CornflowerBlue);
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ID3D11SamplerState* SamplerState{ m_CommonStates->LinearWrap() };
	m_DeviceContext->PSSetSamplers(0, 1, &SamplerState);

	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

	SetGameWindowCullMode();

	m_MatrixView = XMMatrixLookAtLH(m_PtrCurrentCamera->EyePosition, m_PtrCurrentCamera->FocusPosition, m_PtrCurrentCamera->UpDirection);
}

void CGameWindow::AnimateGameObjects()
{
	for (auto& go : m_vGameObjects)
	{
		if (go->ComponentRender.PtrObject3D) go->ComponentRender.PtrObject3D->Animate();
	}
}

void CGameWindow::DrawGameObjects(float DeltaTime)
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	cbPSBaseEyeData.EyePosition = m_PtrCurrentCamera->EyePosition;

	// Update directional light source position
	static float DirectionalLightRoll{};
	DirectionalLightRoll += XM_2PI * DeltaTime * KSkyTimeFactorAbsolute / 4.0f;
	if (DirectionalLightRoll >= XM_PIDIV2) DirectionalLightRoll = -XM_PIDIV2;
	XMVECTOR DirectionalLightSourcePosition{ XMVector3TransformCoord(XMVectorSet(0, 1, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, DirectionalLightRoll)) };
	SetDirectionalLight(DirectionalLightSourcePosition);

	for (auto& go : m_vGameObjects)
	{
		if (go->ComponentRender.IsTransparent) continue;

		UpdateGameObject(go.get(), DeltaTime);
		DrawGameObject(go.get());
	}

	for (auto& go : m_vGameObjects)
	{
		if (!go->ComponentRender.IsTransparent) continue;

		UpdateGameObject(go.get(), DeltaTime);
		DrawGameObject(go.get());
	}

	if (EFLAG_HAS(m_eFlagsGamerendering, EFlagsGameRendering::DrawNormals))
	{
		m_VSBase->Use();
		m_GSNormal->Use();
		m_PSNormal->Use();

		for (auto& go : m_vGameObjects)
		{
			DrawGameObjectNormals(go.get());
		}

		m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	}
}

void CGameWindow::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	m_VSBase->Use();
	m_PSBase->Use();
	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);

	cbPSBaseFlagsData.bUseTexture = FALSE;
	cbPSBaseFlagsData.bUseLighting = FALSE;
	m_PSBase->UpdateConstantBuffer(0);

	for (auto& i : m_vMiniAxisGameObjects)
	{
		cbVSSpaceData.World = XMMatrixTranspose(i->ComponentTransform.MatrixWorld);
		cbVSSpaceData.WVP = XMMatrixTranspose(i->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);
		m_VSBase->UpdateConstantBuffer(0);

		i->ComponentRender.PtrObject3D->Draw();

		i->ComponentTransform.Translation = m_PtrCurrentCamera->EyePosition + m_PtrCurrentCamera->Forward;
		i->UpdateWorldMatrix();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGameWindow::UpdateGameObject(CGameObject* PtrGO, float DeltaTime)
{
	CShader* VS{ m_VSBase.get() };
	CShader* PS{ m_PSBase.get() };

	cbVSSpaceData.World = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld);
	cbVSSpaceData.WVP = XMMatrixTranspose(PtrGO->ComponentTransform.MatrixWorld * m_MatrixView * m_MatrixProjection);

	// Set VS
	if (PtrGO->ComponentRender.PtrObject3D->m_Model.bIsAnimated)
	{
		VS = m_VSAnimation.get();
	}
	else
	{
		if (PtrGO == m_PtrSky || PtrGO == m_PtrCloud || PtrGO == m_PtrSun || PtrGO == m_PtrMoon)
		{
			VS = m_VSSky.get();
		}
	}
	
	// Set PS
	if (PtrGO == m_PtrSky)
	{
		PS = m_PSSky.get();
	}

	if (PtrGO == m_PtrSky)
	{
		static float SkyTime{ 1.0f };
		static float SkyTimeFactor{ KSkyTimeFactorAbsolute };

		SkyTime -= DeltaTime * SkyTimeFactor;
		if (SkyTime < -1.0f) SkyTimeFactor = -SkyTimeFactor;
		if (SkyTime > +1.0f) SkyTimeFactor = -SkyTimeFactor;

		PtrGO->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
		PtrGO->ComponentTransform.Translation = m_PtrCurrentCamera->EyePosition;
		PtrGO->UpdateWorldMatrix();

		cbPSSkyTimeData.SkyTime = SkyTime;
	}

	if (PtrGO == m_PtrCloud)
	{
		static float Yaw{};
		Yaw -= XM_2PI * DeltaTime * 0.01f;
		if (Yaw <= -XM_2PI) Yaw = 0;

		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(0, 0, KSkyDistance, 1), XMMatrixRotationRollPitchYaw(-XM_PIDIV4, Yaw, 0)) };
		PtrGO->ComponentTransform.Translation = m_PtrCurrentCamera->EyePosition + Offset;

		PtrGO->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(-XM_PIDIV2 - XM_PIDIV4, Yaw, 0);

		PtrGO->UpdateWorldMatrix();
	}

	if (PtrGO == m_PtrSun)
	{
		static float SunRoll{};
		SunRoll += XM_2PI * DeltaTime * KSkyTimeFactorAbsolute / 4.0f;
		if (SunRoll >= XM_2PI) SunRoll = 0;

		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, XM_PIDIV2 + SunRoll)) };
		PtrGO->ComponentTransform.Translation = m_PtrCurrentCamera->EyePosition + Offset;

		PtrGO->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(0, 0, XM_PIDIV2 + SunRoll);
		
		PtrGO->UpdateWorldMatrix();
	}

	if (PtrGO == m_PtrMoon)
	{
		static float MoonRoll{};
		MoonRoll += XM_2PI * DeltaTime * KSkyTimeFactorAbsolute / 4.0f;
		if (MoonRoll >= XM_2PI) MoonRoll = 0;

		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, -XM_PIDIV2 + MoonRoll)) };
		PtrGO->ComponentTransform.Translation = m_PtrCurrentCamera->EyePosition + Offset;

		PtrGO->ComponentTransform.RotationQuaternion = XMQuaternionRotationRollPitchYaw(0, 0, -XM_PIDIV2 + MoonRoll);

		PtrGO->UpdateWorldMatrix();
	}

	if (EFLAG_HAS(PtrGO->eFlagsGameObjectRendering, EFlagsGameObjectRendering::NoLighting))
	{
		cbPSBaseFlagsData.bUseLighting = FALSE;
	}
	else
	{
		SetGameWindowUseLighiting();
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

	VS->Use();
	VS->UpdateAllConstantBuffers();

	PS->Use();
	PS->UpdateAllConstantBuffers();
}

void CGameWindow::DrawGameObject(CGameObject* PtrGO)
{
	assert(PtrGO->ComponentRender.PtrObject3D);

	if (EFLAG_HAS(PtrGO->eFlagsGameObjectRendering, EFlagsGameObjectRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetGameWindowCullMode();
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

void CGameWindow::DrawGameObjectNormals(CGameObject* PtrGO)
{
	if (PtrGO->ComponentRender.PtrObject3D)
	{
		if (PtrGO->ComponentRender.PtrObject3D->m_Model.bIsAnimated)
		{
			m_VSAnimation->Use();
		}
		else
		{
			m_VSBase->Use();
		}

		PtrGO->ComponentRender.PtrObject3D->DrawNormals();
	}
}

void CGameWindow::SetGameWindowCullMode()
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

void CGameWindow::SetGameWindowUseLighiting()
{
	if (EFLAG_HAS(m_eFlagsGamerendering, EFlagsGameRendering::UseLighting))
	{
		cbPSBaseFlagsData.bUseLighting = TRUE;
	}
}

void CGameWindow::EndRendering()
{
	m_SwapChain->Present(0, 0);
}

Keyboard::State CGameWindow::GetKeyState()
{
	return m_Keyboard->GetState();
}
Mouse::State CGameWindow::GetMouseState()
{ 
	Mouse::State ResultState{ m_Mouse->GetState() };

	m_Mouse->ResetScrollWheelValue();

	return ResultState;
}