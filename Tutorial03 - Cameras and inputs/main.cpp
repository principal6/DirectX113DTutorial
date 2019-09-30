#include "Core/GameWindow.h"

constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	constexpr float KClearColor[4]{ 0.2f, 0.6f, 0.9f, 1.0f };

	CGameWindow GameWindow{ hInstance, XMFLOAT2(800, 450) };
	GameWindow.CreateWin32(WndProc, TEXT("Game"), true);

	GameWindow.AddCamera(SCameraData(ECameraType::FreeLook));
	GameWindow.SetCamera(0);
	
	CShader* VS{ GameWindow.AddShader() };
	VS->Create(EShaderType::VertexShader, L"Shader\\VertexShader.hlsl", "main", KInputElementDescs, ARRAYSIZE(KInputElementDescs));
	
	CShader* PS{ GameWindow.AddShader() };
	PS->Create(EShaderType::PixelShader, L"Shader\\PixelShader.hlsl", "main");

	CObject3D* TestObjects{ GameWindow.AddObject3D() };
	{
		SObject3DData Data{};

		Data.vVertices.emplace_back(XMVectorSet(-1.0f, +1.0f, +3.0f, 1), XMVectorSet(1.0f, 0.5f, 1.0f, 1));
		Data.vVertices.emplace_back(XMVectorSet(+1.0f, +1.0f, +3.0f, 1), XMVectorSet(0.5f, 1.0f, 0.5f, 1));
		Data.vVertices.emplace_back(XMVectorSet(-1.0f, -1.0f, +3.0f, 1), XMVectorSet(0.5f, 1.0f, 1.0f, 1));
		Data.vVertices.emplace_back(XMVectorSet(+1.0f, -1.0f, +3.0f, 1), XMVectorSet(0.5f, 0.5f, 0.5f, 1));

		Data.vVertices.emplace_back(XMVectorSet(-10.0f, -1.0f, +10.0f, 1), XMVectorSet(0.3f, 0.8f, 0.0f, 1));
		Data.vVertices.emplace_back(XMVectorSet(+10.0f, -1.0f, +10.0f, 1), XMVectorSet(0.3f, 0.8f, 0.0f, 1));
		Data.vVertices.emplace_back(XMVectorSet(-10.0f, -1.0f, -10.0f, 1), XMVectorSet(0.3f, 1.0f, 0.3f, 1));
		Data.vVertices.emplace_back(XMVectorSet(+10.0f, -1.0f, -10.0f, 1), XMVectorSet(0.3f, 1.0f, 0.3f, 1));

		Data.vTriangles.emplace_back(0, 1, 2);
		Data.vTriangles.emplace_back(1, 3, 2);

		Data.vTriangles.emplace_back(4, 5, 6);
		Data.vTriangles.emplace_back(5, 7, 6);

		TestObjects->Create(Data);
	}

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

			Mouse::State MouseState{ GameWindow.GetMouseState() };
			if (MouseState.x || MouseState.y)
			{
				GameWindow.RotateCamera(MouseState.x, MouseState.y, 0.01f);
			}
			if (MouseState.scrollWheelValue)
			{
				GameWindow.ZoomCamera(MouseState.scrollWheelValue, 0.01f);
			}

			GameWindow.UpdateCBWVP(XMMatrixIdentity());

			TestObjects->Draw();

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