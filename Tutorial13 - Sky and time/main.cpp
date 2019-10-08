#include <chrono>
#include "Core/GameWindow.h"
#include "Core/AssimpLoader.h"

using std::chrono::steady_clock;

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	constexpr float KClearColor[4]{ 0.2f, 0.6f, 0.9f, 1.0f };

	CGameWindow GameWindow{ hInstance, XMFLOAT2(800, 600) };
	GameWindow.CreateWin32(WndProc, TEXT("Game"), L"Asset\\dotumche_10_korean.spritefont", true);

	SCameraData MainCamera{ SCameraData(ECameraType::FreeLook) };
	{
		MainCamera.FocusPosition = XMVectorSet(0, +2.0f, +1.0f, 0);
		MainCamera.EyePosition = XMVectorSet(0, +2.0f, 0, 0);
		GameWindow.AddCamera(MainCamera);
	}
	GameWindow.SetCamera(0);

	GameWindow.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	GameWindow.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));
	GameWindow.SetGameRenderingFlags(EFlagsGameRendering::UseLighting);
	
	CTexture* TextureGround{ GameWindow.AddTexture() };
	{
		TextureGround->CreateFromFile(L"Asset\\ground.png");
	}

	CTexture* TextureSky{ GameWindow.AddTexture() };
	{
		TextureSky->CreateFromFile(L"Asset\\sky.png");
	}

	CObject3D* ObjectFloor{ GameWindow.AddObject3D() };
	{
		ObjectFloor->Create(GenerateSquareXZPlane(XMVectorSet(0.0f, 0.8f, 0.4f, 1)));
		ObjectFloor->UpdateQuadUV(XMFLOAT2(0, 0), XMFLOAT2(10.0f, 10.0f));
	}

	CObject3D* ObjectSphere{ GameWindow.AddObject3D() };
	{
		SMaterial Material{ XMFLOAT3(1.0f, 0.5f, 1.0f) };
		Material.SpecularExponent = 20.0f;
		Material.SpecularIntensity = 0.8f;

		ObjectSphere->Create(GenerateSphere(32), Material);
	}

	CObject3D* ObjectFarmhouse{ GameWindow.AddObject3D() };
	{
		SModel Model{ LoadStaticModelFromFile("Asset/farmhouse.fbx") };
		ObjectFarmhouse->Create(Model);
	}

	CObject3D* ObjectYBot{ GameWindow.AddObject3D() };
	{
		SModel Model{ LoadAnimatedModelFromFile("Asset/ybot_mixamo_idle.fbx") };
		ObjectYBot->Create(Model);
	}

	CObject3D* ObjectSkySphere{ GameWindow.AddObject3D() };
	{
		ObjectSkySphere->Create(GenerateSphere(32, XMVectorSet(0.1f, 0.5f, 1.0f, 1), XMVectorSet(1.0f, 1.0f, 1.0f, 1)));
	}

	CObject3D* ObjectSun{ GameWindow.AddObject3D() };
	{
		ObjectSun->Create(GenerateSquareYZPlane(XMVectorSet(1, 1, 1, 1)));

		ObjectSun->UpdateQuadUV(XMFLOAT2(0.0f, 0.5f), XMFLOAT2(0.5f, 0.25f));
	}

	CObject3D* ObjectMoon{ GameWindow.AddObject3D() };
	{
		ObjectMoon->Create(GenerateSquareYZPlane(XMVectorSet(1, 1, 1, 1)));

		ObjectMoon->UpdateQuadUV(XMFLOAT2(0.0f, 0.75f), XMFLOAT2(0.5f, 0.25f));
	}

	CObject3D* ObjectCloud{ GameWindow.AddObject3D() };
	{
		ObjectCloud->Create(GenerateSquareXZPlane(XMVectorSet(1, 1, 1, 1)));

		ObjectCloud->UpdateQuadUV(XMFLOAT2(0.0f, 0.25f), XMFLOAT2(0.5f, 0.25f));
	}

	CGameObject* goFloor{ GameWindow.AddGameObject() };
	{
		goFloor->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
		goFloor->ComponentTransform.Scaling = XMVectorSet(30.0f, 1.0f, 30.0f, 0);
		goFloor->UpdateWorldMatrix();

		goFloor->ComponentRender.PtrObject3D = ObjectFloor;
		goFloor->ComponentRender.PtrTexture = TextureGround;
	}

	CGameObject* goSphere{ GameWindow.AddGameObject() };
	{
		goSphere->ComponentTransform.Translation = XMVectorSet(0.0f, +1.0f, +3.0f, 0);
		goSphere->ComponentTransform.RotationQuaternion = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_PIDIV4);
		goSphere->UpdateWorldMatrix();

		goSphere->ComponentRender.PtrObject3D = ObjectSphere;
	}

	CGameObject* goFarmhouse{ GameWindow.AddGameObject() };
	{
		goFarmhouse->ComponentTransform.Translation = XMVectorSet(-10.0f, 0.0f, 0.0f, 0);
		goFarmhouse->UpdateWorldMatrix();

		goFarmhouse->ComponentRender.PtrObject3D = ObjectFarmhouse;
	}

	CGameObject* goYBot{ GameWindow.AddGameObject() };
	{
		goYBot->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
		goYBot->ComponentTransform.Scaling = XMVectorSet(0.02f, 0.02f, 0.02f, 0);
		goYBot->UpdateWorldMatrix();

		goYBot->ComponentRender.PtrObject3D = ObjectYBot;
	}

	CGameObject* goSkySphere{ GameWindow.AddGameObject() };
	{
		goSkySphere->ComponentRender.PtrObject3D = ObjectSkySphere;

		goSkySphere->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoCulling | EFlagsGameObjectRendering::NoLighting;
	}
	GameWindow.SetGameObjectSky(goSkySphere);

	CGameObject* goSun{ GameWindow.AddGameObject() };
	{
		goSun->ComponentTransform.Scaling = XMVectorSet(1.0f, 15.0f, 40.7f, 0);

		goSun->ComponentRender.PtrObject3D = ObjectSun;
		goSun->ComponentRender.PtrTexture = TextureSky;
		goSun->ComponentRender.IsTransparent = true;

		goSun->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoLighting;
	}
	GameWindow.SetGameObjectSun(goSun);

	CGameObject* goMoon{ GameWindow.AddGameObject() };
	{
		goMoon->ComponentTransform.Scaling = XMVectorSet(1.0f, 15.0f, 40.7f, 0);

		goMoon->ComponentRender.PtrObject3D = ObjectMoon;
		goMoon->ComponentRender.PtrTexture = TextureSky;
		goMoon->ComponentRender.IsTransparent = true;

		goMoon->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoLighting;
	}
	GameWindow.SetGameObjectMoon(goMoon);

	CGameObject* goCloud{ GameWindow.AddGameObject() };
	{
		goCloud->ComponentTransform.Scaling = XMVectorSet(40.7f, 1.0f, 15.0f, 0);

		goCloud->ComponentRender.PtrObject3D = ObjectCloud;
		goCloud->ComponentRender.PtrTexture = TextureSky;
		goCloud->ComponentRender.IsTransparent = true;

		goCloud->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoLighting;
	}
	GameWindow.SetGameObjectCloud(goCloud);

	while (true)
	{
		static MSG Msg{};
		static char key_down{};

		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_KEYDOWN) key_down = (char)Msg.wParam;
			if (Msg.message == WM_QUIT) break;

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			static steady_clock Clock{};
			long long TimeNow{ Clock.now().time_since_epoch().count() };
			static long long TimePrev{ TimeNow };
			long long DeltaTimeLL{ TimeNow - TimePrev };
			float DeltaTimeF{ DeltaTimeLL * 0.000'000'001f };

			GameWindow.BeginRendering(Colors::CornflowerBlue);

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

			if (key_down == VK_F3)
			{
				GameWindow.ToggleGameRenderingFlags(EFlagsGameRendering::DrawNormals);
			}

			if (key_down == VK_F4)
			{
				GameWindow.ToggleGameRenderingFlags(EFlagsGameRendering::UseLighting);
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

			GameWindow.AnimateGameObjects();

			GameWindow.DrawGameObjects(DeltaTimeF);

			GameWindow.DrawMiniAxes();

			SpriteBatch* PtrSpriteBatch{ GameWindow.GetSpriteBatchPtr() };
			SpriteFont* PtrSpriteFont{ GameWindow.GetSpriteFontPtr() };

			PtrSpriteBatch->Begin();

			PtrSpriteFont->DrawString(PtrSpriteBatch, "Test", XMVectorSet(0, 0, 0, 0));
			PtrSpriteFont->DrawString(PtrSpriteBatch, to_string(DeltaTimeF).c_str(), XMVectorSet(0, 10, 0, 0));

			PtrSpriteBatch->End();

			GameWindow.EndRendering();

			key_down = 0;

			TimePrev = TimeNow;
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