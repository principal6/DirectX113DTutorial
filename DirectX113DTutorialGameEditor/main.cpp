#include "Core/Game.h"

// @TODO
// implement anti-aliasing
// implement SSAO
// improve terrain tessellation using quad patch? or is it good with the current Bezier triangles?

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	static constexpr XMFLOAT2 KGameWindowSize{ 1280.0f, 720.0f };
	CGame Game{ hInstance, KGameWindowSize };

	Game.CreateWin32(WndProc, u8"Game Editor", true);
	Game.CreateSpriteFont(L"Asset\\dotumche_10_korean.spritefont");

	Game.SetMode(CGame::EMode::Edit);
	Game.SetRenderingFlags(CGame::EFlagsRendering::UseLighting | CGame::EFlagsRendering::DrawMiniAxes | CGame::EFlagsRendering::DrawGrid |
		CGame::EFlagsRendering::DrawTerrainHeightMapTexture | CGame::EFlagsRendering::DrawTerrainMaskingTexture | 
		CGame::EFlagsRendering::DrawTerrainFoliagePlacingTexture | CGame::EFlagsRendering::TessellateTerrain | 
		CGame::EFlagsRendering::Use3DGizmos | CGame::EFlagsRendering::UsePhysicallyBasedRendering);

	//Game.CreateDynamicSky("Asset\\Sky.xml", 30.0f);
	Game.CreateStaticSky(30.0f);

	// Main loop
	while (true)
	{
		static MSG Msg{};
		static char KeyDown{};
		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT) break;

			if (Msg.message == WM_KEYDOWN) KeyDown = (char)Msg.wParam;
			
			if (Msg.message == WM_LBUTTONDOWN) Game.NotifyMouseLeftDown();
			if (Msg.message == WM_LBUTTONUP) Game.NotifyMouseLeftUp();

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			if (KeyDown == VK_SPACE) Game.JumpPlayer(6.0f);
			if (KeyDown == VK_DELETE) Game.DeleteSelectedObjects();
			if (GetKeyState(VK_CONTROL) && (KeyDown == 'c' || KeyDown == 'C')) Game.CopySelectedObject();
			if (GetKeyState(VK_CONTROL) && (KeyDown == 'v' || KeyDown == 'V')) Game.PasteCopiedObject();

			Game.WalkPlayerToPickedPoint(2.75f);
			
			Game.BeginRendering(Colors::CornflowerBlue);

			Game.Update();
			Game.Draw();

			Game.EndRendering();

			KeyDown = 0;
		}
	}

	return 0;
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
		return 0;

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