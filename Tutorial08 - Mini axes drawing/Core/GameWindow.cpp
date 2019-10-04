#include "GameWindow.h"

constexpr D3D11_INPUT_ELEMENT_DESC KVSMiniAxisInputElementDescs[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

void CGameWindow::ToggleGameRenderingFlags(EFlagsGameRendering Flags)
{
	m_eFlagsGamerendering ^= Flags;
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

	CreateGSNormal();
	CreateVSMiniAxis();

	CreateCBs();

	CreateMiniAxis();

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
		m_vViewPorts.emplace_back();
		D3D11_VIEWPORT& ViewPort{ m_vViewPorts.back() };
		ViewPort.TopLeftX = 0.0f;
		ViewPort.TopLeftY = 0.0f;
		ViewPort.Width = m_WindowSize.x;
		ViewPort.Height = m_WindowSize.y;
		ViewPort.MinDepth = 0.0f;
		ViewPort.MaxDepth = 1.0f;
	}

	{
		m_vViewPorts.emplace_back();
		D3D11_VIEWPORT& ViewPort{ m_vViewPorts.back() };
		ViewPort.TopLeftX = m_WindowSize.x * 6.0f / 7.0f;
		ViewPort.TopLeftY = m_WindowSize.y * 0.2f / 7.0f;
		ViewPort.Width = m_WindowSize.x / 7.0f;
		ViewPort.Height = m_WindowSize.y / 7.0f;
		ViewPort.MinDepth = 0.0f;
		ViewPort.MaxDepth = 1.0f;
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewPorts[0]);
}

void CGameWindow::CreateInputDevices()
{
	m_Keyboard = make_unique<Keyboard>();

	m_Mouse = make_unique<Mouse>();
	m_Mouse->SetWindow(m_hWnd);

	m_Mouse->SetMode(Mouse::Mode::MODE_RELATIVE);
}

void CGameWindow::CreateGSNormal()
{
	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormals.hlsl", "main");
}

void CGameWindow::CreateVSMiniAxis()
{
	m_VSMiniAxis = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSMiniAxis->Create(EShaderType::VertexShader, L"Shader\\VSMiniAxis.hlsl", "main",
		KVSMiniAxisInputElementDescs, ARRAYSIZE(KVSMiniAxisInputElementDescs));
}

void CGameWindow::CreateCBs()
{
	CreateCB(m_CBVSDefaultSpace.GetAddressOf(), sizeof(SCBVSDefaultSpaceData));
	CreateCB(m_CBVSMiniAxisSpace.GetAddressOf(), sizeof(SCBVSMiniAxisSpaceData));
	CreateCB(m_CBPSDefaultTexture.GetAddressOf(), sizeof(SCBPSDefaultTextureData));
}

void CGameWindow::CreateCB(ID3D11Buffer** PPBuffer, size_t ByteWidth)
{
	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = static_cast<UINT>(ByteWidth);
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	m_Device->CreateBuffer(&cbDesc, nullptr, PPBuffer);
}

void CGameWindow::UpdateCB(ID3D11Buffer* PPBuffer, void* PtrValue, size_t ValueByteSize)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_DeviceContext->Map(PPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, PtrValue, ValueByteSize);

		m_DeviceContext->Unmap(PPBuffer, 0);
	}
}

void CGameWindow::CreateMiniAxis()
{
	m_Object3DAxisX = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get());
	m_Object3DAxisX->Create(GenerateCone(0, 16, XMVectorSet(1.0f, 0.0f, 0.0f, 1)));

	m_Object3DAxisY = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get());
	m_Object3DAxisY->Create(GenerateCone(0, 16, XMVectorSet(0.0f, 1.0f, 0.0f, 1)));

	m_Object3DAxisZ = make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get());
	m_Object3DAxisZ->Create(GenerateCone(0, 16, XMVectorSet(0.0f, 0.0f, 1.0f, 1)));

	m_GOAxisX = make_unique<CGameObject>();
	m_GOAxisX->ComponentRender.PtrObject3D = m_Object3DAxisX.get();
	m_GOAxisX->ComponentTransform.Scaling = XMVectorSet(0.06f, 1.0f, 0.06f, 0);
	m_GOAxisX->ComponentTransform.Rotation = XMQuaternionRotationRollPitchYaw(0, 0, -XM_PIDIV2);

	m_GOAxisY = make_unique<CGameObject>();
	m_GOAxisY->ComponentRender.PtrObject3D = m_Object3DAxisY.get();
	m_GOAxisY->ComponentTransform.Scaling = XMVectorSet(0.06f, 1.0f, 0.06f, 0);

	m_GOAxisZ = make_unique<CGameObject>();
	m_GOAxisZ->ComponentRender.PtrObject3D = m_Object3DAxisZ.get();
	m_GOAxisZ->ComponentTransform.Scaling = XMVectorSet(0.06f, 1.0f, 0.06f, 0);
	m_GOAxisZ->ComponentTransform.Rotation = XMQuaternionRotationRollPitchYaw(0, XM_PIDIV2, XM_PIDIV2);
}

void CGameWindow::UpdateCBVSDefaultSpace(const XMMATRIX& MatrixWorld)
{
	m_CBVSDefaultSpaceData.WVP = XMMatrixTranspose(MatrixWorld * m_MatrixView * m_MatrixProjection);
	m_CBVSDefaultSpaceData.World = XMMatrixTranspose(MatrixWorld);

	UpdateCB(m_CBVSDefaultSpace.Get(), &m_CBVSDefaultSpaceData, sizeof(SCBVSDefaultSpaceData));

	m_DeviceContext->VSSetConstantBuffers(0, 1, m_CBVSDefaultSpace.GetAddressOf());
}

void CGameWindow::UpdateCBVSMiniAxisSpace(const XMMATRIX& MatrixWorld)
{
	m_CBVSMiniAxisSpaceData.WVP = XMMatrixTranspose(MatrixWorld * m_MatrixView * m_MatrixProjection);

	UpdateCB(m_CBVSMiniAxisSpace.Get(), &m_CBVSMiniAxisSpaceData, sizeof(SCBVSDefaultSpaceData));

	m_DeviceContext->VSSetConstantBuffers(0, 1, m_CBVSMiniAxisSpace.GetAddressOf());
}

void CGameWindow::UpdateCBPSDefaultTexture(BOOL UseTexture)
{
	m_CBPSDefaultTextureData.bUseTexture = UseTexture;

	UpdateCB(m_CBPSDefaultTexture.Get(), &m_CBPSDefaultTextureData, sizeof(SCBPSDefaultTextureData));

	m_DeviceContext->PSSetConstantBuffers(0, 1, m_CBPSDefaultTexture.GetAddressOf());
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
	m_vObject3Ds.emplace_back(make_unique<CObject3D>(m_Device.Get(), m_DeviceContext.Get()));
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

	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);

	ID3D11SamplerState* SamplerState{ m_CommonStates->LinearWrap() };
	m_DeviceContext->PSSetSamplers(0, 1, &SamplerState);

	m_DeviceContext->OMSetBlendState(m_CommonStates->AlphaBlend(), nullptr, 0xFFFFFFFF);

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

	m_MatrixView = XMMatrixLookAtLH(m_PtrCurrentCamera->EyePosition, m_PtrCurrentCamera->FocusPosition, m_PtrCurrentCamera->UpDirection);
}

void CGameWindow::DrawGameObjects()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewPorts[0]);

	for (auto& go : m_vGameObjects)
	{
		if (go->ComponentRender.IsTransparent) continue;

		DrawGameObject(go.get());
	}

	for (auto& go : m_vGameObjects)
	{
		if (!go->ComponentRender.IsTransparent) continue;

		DrawGameObject(go.get());
	}
}

void CGameWindow::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewPorts[1]);

	m_VSMiniAxis->Use();
	UpdateCBPSDefaultTexture(FALSE);

	DrawMiniAxis(m_GOAxisX.get());
	DrawMiniAxis(m_GOAxisY.get());
	DrawMiniAxis(m_GOAxisZ.get());

	m_DeviceContext->RSSetViewports(1, &m_vViewPorts[0]);
}

void CGameWindow::DrawGameObject(CGameObject* PtrGO)
{
	UpdateCBVSDefaultSpace(PtrGO->ComponentTransform.MatrixWorld);

	if (PtrGO->ComponentRender.PtrTexture)
	{
		UpdateCBPSDefaultTexture(TRUE);

		PtrGO->ComponentRender.PtrTexture->Use();
	}
	else
	{
		UpdateCBPSDefaultTexture(FALSE);
	}

	if (PtrGO->ComponentRender.PtrObject3D)
	{
		PtrGO->ComponentRender.PtrObject3D->Draw();

		if ((m_eFlagsGamerendering & EFlagsGameRendering::DrawNormals) == EFlagsGameRendering::DrawNormals)
		{
			UpdateCBPSDefaultTexture(FALSE);

			m_GSNormal->Use();

			PtrGO->ComponentRender.PtrObject3D->DrawNormals();

			m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
		}
	}
}

void CGameWindow::DrawMiniAxis(CGameObject* PtrGO)
{
	UpdateCBVSMiniAxisSpace(PtrGO->ComponentTransform.MatrixWorld);

	if (PtrGO->ComponentRender.PtrObject3D)
	{
		PtrGO->ComponentRender.PtrObject3D->Draw();
	}

	PtrGO->ComponentTransform.Translation = m_PtrCurrentCamera->EyePosition + m_PtrCurrentCamera->Forward;

	PtrGO->UpdateWorldMatrix();
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