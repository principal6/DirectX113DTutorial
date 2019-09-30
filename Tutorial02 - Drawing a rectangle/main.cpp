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

	CGameWindow GameWindow{ hInstance, XMFLOAT2(800, 450) };
	GameWindow.CreateWin32(WndProc, TEXT("Game"), true);

	CShader VS{ GameWindow.GetDevicePtr(), GameWindow.GetDeviceContextPtr() };
	VS.Create(EShaderType::VertexShader, L"Shader\\VertexShader.hlsl", "main", KInputElementDescs, ARRAYSIZE(KInputElementDescs));
	
	CShader PS{ GameWindow.GetDevicePtr(), GameWindow.GetDeviceContextPtr() };
	PS.Create(EShaderType::PixelShader, L"Shader\\PixelShader.hlsl", "main");

	CObject3D Object{ GameWindow.GetDevicePtr(), GameWindow.GetDeviceContextPtr() };
	{
		SObject3DData Data{};

		Data.vVertices.emplace_back(XMVectorSet(-0.5f, +0.5f, 0, 1), XMVectorSet(1.0f, 0.5f, 1.0f, 1));
		Data.vVertices.emplace_back(XMVectorSet(+0.5f, +0.5f, 0, 1), XMVectorSet(0.5f, 1.0f, 0.5f, 1));
		Data.vVertices.emplace_back(XMVectorSet(-0.5f, -0.5f, 0, 1), XMVectorSet(0.5f, 1.0f, 1.0f, 1));
		Data.vVertices.emplace_back(XMVectorSet(+0.5f, -0.5f, 0, 1), XMVectorSet(0.5f, 0.5f, 0.5f, 1));

		Data.vTriangles.emplace_back(0, 1, 2);
		Data.vTriangles.emplace_back(1, 3, 2);

		Object.Create(Data);
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

			VS.Use();
			PS.Use();

			Object.Draw();

			GameWindow.EndRendering();
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