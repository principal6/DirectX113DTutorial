#include <chrono>
#include "Core/Game.h"
#include "Core/AssimpLoader.h"

using std::chrono::steady_clock;

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	constexpr float KClearColor[4]{ 0.2f, 0.6f, 0.9f, 1.0f };
	
	CGame Game{ hInstance, XMFLOAT2(800, 600) };
	Game.CreateWin32(WndProc, TEXT("Game"), L"Asset\\dotumche_10_korean.spritefont", true);
	Game.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	Game.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));
	Game.SetGameRenderingFlags(EFlagsGameRendering::UseLighting | EFlagsGameRendering::DrawMiniAxes |
		EFlagsGameRendering::DrawPickingData | EFlagsGameRendering::DrawBoundingSphere);

	CCamera* MainCamera{ Game.AddCamera(SCameraData(ECameraType::FreeLook, XMVectorSet(0, +2.0f, 0, 0), XMVectorSet(0, +2.0f, +1.0f, 0))) };
	
	CTexture* TextureGround{ Game.AddTexture() };
	{
		TextureGround->CreateFromFile(L"Asset\\ground.png");
	}

	CTexture* TextureSky{ Game.AddTexture() };
	{
		TextureSky->CreateFromFile(L"Asset\\sky.png");
	}

	CObject3D* ObjectFloor{ Game.AddObject3D() };
	{
		ObjectFloor->Create(GenerateSquareXZPlane(XMVectorSet(0.0f, 0.8f, 0.4f, 1)));
		ObjectFloor->UpdateQuadUV(XMFLOAT2(0, 0), XMFLOAT2(10.0f, 10.0f));
	}

	CObject3D* ObjectSphere{ Game.AddObject3D() };
	{
		SMaterial Material{ XMFLOAT3(1.0f, 0.5f, 1.0f) };
		Material.SpecularExponent = 20.0f;
		Material.SpecularIntensity = 0.8f;

		ObjectSphere->Create(GenerateSphere(32), Material);
	}

	CObject3D* ObjectFarmhouse{ Game.AddObject3D() };
	{
		SModel Model{ LoadStaticModelFromFile("Asset/farmhouse.fbx") };
		ObjectFarmhouse->Create(Model);
	}

	CObject3D* ObjectYBot{ Game.AddObject3D() };
	{
		SModel Model{ LoadAnimatedModelFromFile("Asset/ybot_mixamo_idle.fbx") };
		ObjectYBot->Create(Model);
	}

	CObject3D* ObjectSkySphere{ Game.AddObject3D() };
	{
		ObjectSkySphere->Create(GenerateSphere(32, XMVectorSet(0.1f, 0.5f, 1.0f, 1), XMVectorSet(1.0f, 1.0f, 1.0f, 1)));
	}

	CObject3D* ObjectSun{ Game.AddObject3D() };
	{
		ObjectSun->Create(GenerateSquareYZPlane(XMVectorSet(1, 1, 1, 1)));
		ObjectSun->UpdateQuadUV(XMFLOAT2(0.0f, 0.5f), XMFLOAT2(0.5f, 0.25f));
	}

	CObject3D* ObjectMoon{ Game.AddObject3D() };
	{
		ObjectMoon->Create(GenerateSquareYZPlane(XMVectorSet(1, 1, 1, 1)));
		ObjectMoon->UpdateQuadUV(XMFLOAT2(0.0f, 0.75f), XMFLOAT2(0.5f, 0.25f));
	}

	CObject3D* ObjectCloud{ Game.AddObject3D() };
	{
		ObjectCloud->Create(GenerateSquareXZPlane(XMVectorSet(1, 1, 1, 1)));
		ObjectCloud->UpdateQuadUV(XMFLOAT2(0.0f, 0.25f), XMFLOAT2(0.5f, 0.25f));
	}

	CObject2D* Object2DRectangle(Game.AddObject2D());
	{
		Object2DRectangle->CreateDynamic(Generate2DRectangle(XMFLOAT2(100, 50)));
	}

	CGameObject* goFloor{ Game.AddGameObject("Floor") };
	{
		goFloor->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
		goFloor->ComponentTransform.Scaling = XMVectorSet(30.0f, 1.0f, 30.0f, 0);

		goFloor->ComponentRender.PtrObject3D = ObjectFloor;
		goFloor->ComponentRender.PtrTexture = TextureGround;

		goFloor->ComponentPhysics.BoundingSphere.Radius = 43.0f;
	}

	CGameObject* goSphere{ Game.AddGameObject("Sphere") };
	{
		goSphere->ComponentTransform.Translation = XMVectorSet(0.0f, +1.0f, +3.0f, 0);
		goSphere->ComponentTransform.RotationQuaternion = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_PIDIV4);

		goSphere->ComponentRender.PtrObject3D = ObjectSphere;
	}

	CGameObject* goFarmhouse{ Game.AddGameObject("Farmhouse") };
	{
		goFarmhouse->ComponentTransform.Translation = XMVectorSet(-10.0f, 0.0f, 0.0f, 0);

		goFarmhouse->ComponentRender.PtrObject3D = ObjectFarmhouse;

		goFarmhouse->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSet(0, 1.0f, -2.5f, 0);
		goFarmhouse->ComponentPhysics.BoundingSphere.Radius = 11.5f;
	}

	CGameObject* goYBot{ Game.AddGameObject("YBot") };
	{
		goYBot->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
		goYBot->ComponentTransform.Scaling = XMVectorSet(0.02f, 0.02f, 0.02f, 0);

		goYBot->ComponentRender.PtrObject3D = ObjectYBot;

		goYBot->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSet(0, 1.5f, 0, 0);
		goYBot->ComponentPhysics.BoundingSphere.Radius = 2.0f;
	}

	CGameObject* goSkySphere{ Game.AddGameObject("SkySphere") };
	{
		goSkySphere->ComponentRender.PtrObject3D = ObjectSkySphere;

		goSkySphere->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoCulling | EFlagsGameObjectRendering::NoLighting;

		goSkySphere->ComponentPhysics.bIsPickable = false;
	}
	Game.SetGameObjectSky(goSkySphere);

	CGameObject* goSun{ Game.AddGameObject("Sun") };
	{
		goSun->ComponentTransform.Scaling = XMVectorSet(1.0f, 15.0f, 40.7f, 0);

		goSun->ComponentRender.PtrObject3D = ObjectSun;
		goSun->ComponentRender.PtrTexture = TextureSky;
		goSun->ComponentRender.IsTransparent = true;

		goSun->ComponentPhysics.bIsPickable = false;

		goSun->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoLighting;
	}
	Game.SetGameObjectSun(goSun);

	CGameObject* goMoon{ Game.AddGameObject("Moon") };
	{
		goMoon->ComponentTransform.Scaling = XMVectorSet(1.0f, 15.0f, 40.7f, 0);

		goMoon->ComponentRender.PtrObject3D = ObjectMoon;
		goMoon->ComponentRender.PtrTexture = TextureSky;
		goMoon->ComponentRender.IsTransparent = true;

		goMoon->ComponentPhysics.bIsPickable = false;

		goMoon->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoLighting;
	}
	Game.SetGameObjectMoon(goMoon);

	CGameObject* goCloud{ Game.AddGameObject("Cloud") };
	{
		goCloud->ComponentTransform.Scaling = XMVectorSet(40.7f, 1.0f, 15.0f, 0);

		goCloud->ComponentRender.PtrObject3D = ObjectCloud;
		goCloud->ComponentRender.PtrTexture = TextureSky;
		goCloud->ComponentRender.IsTransparent = true;

		goCloud->ComponentPhysics.bIsPickable = false;

		goCloud->eFlagsGameObjectRendering = EFlagsGameObjectRendering::NoLighting;
	}
	Game.SetGameObjectCloud(goCloud);

	CGameObject2D* go2DRectangle{ Game.AddGameObject2D("2DRectangle") };
	{
		go2DRectangle->ComponentRender.PtrObject2D = Object2DRectangle;
		go2DRectangle->ComponentRender.PtrTexture = TextureSky;
		go2DRectangle->bIsVisible = false;
	}

	while (true)
	{
		static MSG Msg{};
		static char KeyDown{};

		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_KEYDOWN) KeyDown = (char)Msg.wParam;
			if (Msg.message == WM_QUIT) break;

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			static steady_clock Clock{};
			long long TimeNow{ Clock.now().time_since_epoch().count() };
			static long long TimePrev{ TimeNow };
			float DeltaTimeF{ (TimeNow - TimePrev) * 0.000'000'001f };

			Game.BeginRendering(Colors::CornflowerBlue);

			const Keyboard::State& KeyState{ Game.GetKeyState() };
			if (KeyState.Escape)
			{
				Game.Destroy();
			}
			if (KeyState.W)
			{
				MainCamera->MoveCamera(ECameraMovementDirection::Forward, 0.01f);
			}
			if (KeyState.S)
			{
				MainCamera->MoveCamera(ECameraMovementDirection::Backward, 0.01f);
			}
			if (KeyState.A)
			{
				MainCamera->MoveCamera(ECameraMovementDirection::Leftward, 0.01f);
			}
			if (KeyState.D)
			{
				MainCamera->MoveCamera(ECameraMovementDirection::Rightward, 0.01f);
			}

			if (KeyDown == VK_F1)
			{
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::DrawWireFrame);
			}
			if (KeyDown == VK_F2)
			{
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::DrawNormals);
			}
			if (KeyDown == VK_F3)
			{
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::UseLighting);
			}
			if (KeyDown == VK_F4)
			{
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::DrawMiniAxes);
			}
			if (KeyDown == VK_F5)
			{
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::DrawBoundingSphere);
			}
			if (KeyDown == VK_F6)
			{
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::DrawPickingData);
			}

			const Mouse::State& MouseState{ Game.GetMouseState() };
			static int MouseX{ MouseState.x };
			static int MouseY{ MouseState.y };
			if (MouseState.leftButton)
			{
				Game.Pick(MouseState.x, MouseState.y);
			}
			if (MouseState.x != MouseX || MouseState.y != MouseY)
			{
				if (MouseState.rightButton)
				{
					MainCamera->RotateCamera(MouseState.x - MouseX, MouseState.y - MouseY, 0.01f);
				}

				MouseX = MouseState.x;
				MouseY = MouseState.y;
			}
			if (MouseState.scrollWheelValue)
			{
				MainCamera->ZoomCamera(MouseState.scrollWheelValue, 0.01f);
			}

			Game.AnimateGameObjects();
			Game.DrawGameObjects(DeltaTimeF);
			Game.DrawGameObject2Ds(DeltaTimeF);

			static SpriteBatch* PtrSpriteBatch{ Game.GetSpriteBatchPtr() };
			static SpriteFont* PtrSpriteFont{ Game.GetSpriteFontPtr() };
			PtrSpriteBatch->Begin();
			{
				PtrSpriteFont->DrawString(PtrSpriteBatch, to_string(DeltaTimeF).c_str(), XMVectorSet(0, 0, 0, 0));
				if (Game.GetPickedGameObjectName())
				{
					PtrSpriteFont->DrawString(PtrSpriteBatch, (string("Picked: ") + Game.GetPickedGameObjectName()).c_str(), XMVectorSet(0, 15, 0, 0));
				}
			}
			PtrSpriteBatch->End();

			Game.EndRendering();

			KeyDown = 0;
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