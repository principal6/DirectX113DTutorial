#include "Core/GameWindow.h"
#include "Core/Shader.h"
#include "Core/Object3D.h"

constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	constexpr float KClearColor[4]{ 0.2f, 0.6f, 0.9f, 1.0f };

	CGameWindow game_window{ hInstance, XMFLOAT2(800, 450) };
	game_window.CreateWin32(WndProc, TEXT("Game"), true);

	CShader vs{ game_window.GetDevicePtr(), game_window.GetDeviceContextPtr() };
	vs.Create(EShaderType::VertexShader, L"Shader\\VertexShader.hlsl", "main", KInputElementDescs, ARRAYSIZE(KInputElementDescs));
	
	CShader ps{ game_window.GetDevicePtr(), game_window.GetDeviceContextPtr() };
	ps.Create(EShaderType::PixelShader, L"Shader\\PixelShader.hlsl", "main");

	CObject3D obj{ game_window.GetDevicePtr(), game_window.GetDeviceContextPtr() };
	{
		vector<SVertex3D> vertices{};
		vertices.emplace_back(XMVectorSet(-0.5f, +0.5f, 0, 1), XMVectorSet(1.0f, 0.5f, 1.0f, 1));
		vertices.emplace_back(XMVectorSet(+0.5f, +0.5f, 0, 1), XMVectorSet(0.5f, 1.0f, 0.5f, 1));
		vertices.emplace_back(XMVectorSet(-0.5f, -0.5f, 0, 1), XMVectorSet(0.5f, 1.0f, 1.0f, 1));
		vertices.emplace_back(XMVectorSet(+0.5f, -0.5f, 0, 1), XMVectorSet(0.5f, 0.5f, 0.5f, 1));

		vector<SFace> faces{};
		faces.emplace_back(0, 1, 2);
		faces.emplace_back(1, 3, 2);

		obj.Create(vertices, faces);
	}
	

	while (true)
	{
		static MSG msg{};

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			game_window.BeginRendering(Colors::CornflowerBlue);

			vs.Use();
			ps.Use();

			obj.Draw();

			game_window.EndRendering();
		}
	}
	return 0;
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (Msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}