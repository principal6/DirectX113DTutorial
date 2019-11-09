#include "Core/Game.h"

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	static constexpr XMFLOAT2 KGameWindowSize{ 1280.0f, 720.0f };
	CGame Game{ hInstance, KGameWindowSize };

	Game.CreateWin32(WndProc, TEXT("Game Editor"), true);
	Game.CreateSpriteFont(L"Asset\\dotumche_10_korean.spritefont");
	Game.SetMode(CGame::EMode::Edit);

	Game.SetGameRenderingFlags(CGame::EFlagsRendering::UseLighting | CGame::EFlagsRendering::DrawMiniAxes |
		CGame::EFlagsRendering::DrawTerrainHeightMapTexture | CGame::EFlagsRendering::DrawTerrainMaskingTexture | 
		CGame::EFlagsRendering::DrawTerrainFoliagePlacingTexture | CGame::EFlagsRendering::TessellateTerrain | 
		CGame::EFlagsRendering::Use3DGizmos);

	CCamera* MainCamera{ Game.AddCamera(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0))) };
	MainCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));

	Game.SetSky("Asset\\Sky.xml", 30.0f);
	Game.UnsetSky();

	Game.InsertObject3DLine("Grid");
	{
		CObject3DLine* Grid{ Game.GetObject3DLine("Grid") };
		Grid->Create(Generate3DGrid(0));
	}

	CMaterial MaterialTest{};
	{
		MaterialTest.SetName("Test");
		MaterialTest.ShouldGenerateAutoMipMap(true);
		MaterialTest.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\test.jpg");
		MaterialTest.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\test_normal.jpg");
		MaterialTest.SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, "Asset\\test_displacement.jpg");
		Game.AddMaterial(MaterialTest);
	}

	CMaterial MaterialDefaultGround1{};
	{
		MaterialDefaultGround1.SetName("DefaultGround1");
		MaterialDefaultGround1.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGround1.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\ground1.jpg");
		MaterialDefaultGround1.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\ground1_normal.jpg");
		MaterialDefaultGround1.SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, "Asset\\ground1_displacement.jpg");
		Game.AddMaterial(MaterialDefaultGround1);
	}

	CMaterial MaterialDefaultGround2{};
	{
		MaterialDefaultGround2.SetName("DefaultGround2");
		MaterialDefaultGround2.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGround2.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\ground2.jpg");
		MaterialDefaultGround2.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\ground2_normal.jpg");
		MaterialDefaultGround2.SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, "Asset\\ground2_displacement.jpg");
		Game.AddMaterial(MaterialDefaultGround2);
	}

	CMaterial MaterialDefaultGround3{};
	{
		MaterialDefaultGround3.SetName("DefaultGround3");
		MaterialDefaultGround3.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGround3.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\ground3.jpg");
		MaterialDefaultGround3.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\ground3_normal.jpg");
		MaterialDefaultGround3.SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, "Asset\\ground3_displacement.jpg");
		Game.AddMaterial(MaterialDefaultGround3);
	}

	CMaterial MaterialDefaultGrass{};
	{
		MaterialDefaultGrass.SetName("DefaultGrass");
		MaterialDefaultGrass.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGrass.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\grass.jpg");
		MaterialDefaultGrass.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\grass_normal.jpg");
		Game.AddMaterial(MaterialDefaultGrass);
	}

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
			Game.BeginRendering(Colors::CornflowerBlue);

			if (KeyDown == VK_F1)
			{
				Game.ToggleGameRenderingFlags(CGame::EFlagsRendering::DrawWireFrame);
			}
			if (KeyDown == VK_F2)
			{
				Game.ToggleGameRenderingFlags(CGame::EFlagsRendering::DrawNormals);
			}
			if (KeyDown == VK_F3)
			{
				Game.ToggleGameRenderingFlags(CGame::EFlagsRendering::DrawMiniAxes);
			}
			if (KeyDown == VK_F4)
			{
				Game.ToggleGameRenderingFlags(CGame::EFlagsRendering::DrawBoundingSphere);
			}

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