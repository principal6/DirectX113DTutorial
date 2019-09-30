#include "Core/GameWindow.h"

constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT		, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	constexpr float KClearColor[4]{ 0.2f, 0.6f, 0.9f, 1.0f };

	CGameWindow GameWindow{ hInstance, XMFLOAT2(800, 450) };
	GameWindow.CreateWin32(WndProc, TEXT("Game"), L"Asset\\dotumche_10_korean.spritefont", true);

	GameWindow.AddCamera(SCameraData(ECameraType::FreeLook));
	GameWindow.SetCamera(0);
	
	CShader* VS{ GameWindow.AddShader() };
	VS->Create(EShaderType::VertexShader, L"Shader\\VertexShader.hlsl", "main", KInputElementDescs, ARRAYSIZE(KInputElementDescs));
	
	CShader* PS{ GameWindow.AddShader() };
	PS->Create(EShaderType::PixelShader, L"Shader\\PixelShader.hlsl", "main");

	CTexture* TextureGround{ GameWindow.AddTexture() };
	{
		TextureGround->CreateFromFile(L"Asset\\ground.png");
	}

	CTexture* TextureTransparent{ GameWindow.AddTexture() };
	{
		TextureTransparent->CreateFromFile(L"Asset\\transparent.png");
	}

	CObject3D* ObjectSquare{ GameWindow.AddObject3D() };
	{
		ObjectSquare->Create(GenerateSquareXZPlane(XMVectorSet(0.0f, 0.8f, 0.4f, 1)));
	}

	CObject3D* ObjectCube{ GameWindow.AddObject3D() };
	{
		ObjectCube->Create(GenerateCube(XMVectorSet(1.0f, 0.5f, 1.0f, 1)));
	}

	CGameObject* goCube{ GameWindow.AddGameObject() };
	goCube->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, +3.0f, 0);
	goCube->ComponentTransform.Rotation = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_PIDIV4);
	goCube->UpdateWorldMatrix();
	goCube->ComponentRender.PtrObject3D = ObjectCube;
	goCube->ComponentRender.PtrTexture = TextureTransparent;
	goCube->ComponentRender.IsTransparent = true;

	CGameObject* goFloor{ GameWindow.AddGameObject() };
	goFloor->ComponentTransform.Translation = XMVectorSet(0.0f, -1.0f, 0.0f, 0);
	goFloor->ComponentTransform.Scaling = XMVectorSet(10.0f, 0.0f, 10.0f, 0);
	goFloor->UpdateWorldMatrix();
	goFloor->ComponentRender.PtrObject3D = ObjectSquare;
	goFloor->ComponentRender.PtrTexture = TextureGround;

	while (true)
	{
		static MSG Msg{};

		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT) break;

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			GameWindow.BeginRendering(Colors::CornflowerBlue);

			VS->Use();
			PS->Use();

			Keyboard::State KeyState{ GameWindow.GetKeyState() };
			if (KeyState.Escape)
			{
				DestroyWindow(GameWindow.GetHWND());
			}
			if (KeyState.W)
			{
				GameWindow.MoveCamera(ECameraMovementDirection::Forward, 0.01f);
			}
			if (KeyState.S)
			{
				GameWindow.MoveCamera(ECameraMovementDirection::Backward, 0.01f);
			}
			if (KeyState.A)
			{
				GameWindow.MoveCamera(ECameraMovementDirection::Leftward, 0.01f);
			}
			if (KeyState.D)
			{
				GameWindow.MoveCamera(ECameraMovementDirection::Rightward, 0.01f);
			}
			if (KeyState.F1)
			{
				GameWindow.SetRasterizerState(ERasterizerState::WireFrame);
			}
			if (KeyState.F2)
			{
				GameWindow.SetRasterizerState(ERasterizerState::CullCounterClockwise);
			}

			Mouse::State MouseState{ GameWindow.GetMouseState() };
			if (MouseState.x || MouseState.y)
			{
				GameWindow.RotateCamera(MouseState.x, MouseState.y, 0.01f);
			}
			if (MouseState.scrollWheelValue)
			{
				GameWindow.ZoomCamera(MouseState.scrollWheelValue, 0.01f);
			}

			GameWindow.DrawGameObjects();

			SpriteBatch* PtrSpriteBatch{ GameWindow.GetSpriteBatchPtr() };
			SpriteFont* PtrSpriteFont{ GameWindow.GetSpriteFontPtr() };

			PtrSpriteBatch->Begin();

			PtrSpriteFont->DrawString(PtrSpriteBatch, "Test", XMVectorSet(0, 0, 0, 0));

			PtrSpriteBatch->End();

			GameWindow.EndRendering();
		}
	}
	return 0;
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (Msg)
	{
	case WM_ACTIVATEAPP:
		Keyboard::ProcessMessage(Msg, wParam, lParam);
		break;
	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		Mouse::ProcessMessage(Msg, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Keyboard::ProcessMessage(Msg, wParam, lParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}