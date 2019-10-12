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

	Game.SetSky("Asset\\Sky.xml", 12.0f);
	Game.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	Game.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));

	Game.SetGameRenderingFlags(EFlagsGameRendering::UseLighting | EFlagsGameRendering::DrawMiniAxes |
		EFlagsGameRendering::DrawPickingData | EFlagsGameRendering::DrawBoundingSphere | EFlagsGameRendering::Use3DGizmos);

	CCamera* MainCamera{ Game.AddCamera(SCameraData(ECameraType::FreeLook, XMVectorSet(0, +2.0f, 0, 0), XMVectorSet(0, +2.0f, +1.0f, 0))) };
	
	CTexture* TextureGround{ Game.AddTexture() };
	{
		TextureGround->CreateFromFile(L"Asset\\ground.png");
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

	CGameObject3D* goFloor{ Game.AddGameObject3D("Floor") };
	{
		goFloor->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
		goFloor->ComponentTransform.Scaling = XMVectorSet(30.0f, 1.0f, 30.0f, 0);

		goFloor->ComponentRender.PtrObject3D = ObjectFloor;
		goFloor->ComponentRender.PtrTexture = TextureGround;

		goFloor->ComponentPhysics.BoundingSphere.Radius = 43.0f;
	}

	CGameObject3D* goSphere{ Game.AddGameObject3D("Sphere") };
	{
		goSphere->ComponentTransform.Translation = XMVectorSet(0.0f, +1.0f, +3.0f, 0);
		goSphere->ComponentRender.PtrObject3D = ObjectSphere;
	}

	CGameObject3D* goFarmhouse{ Game.AddGameObject3D("Farmhouse") };
	{
		goFarmhouse->ComponentTransform.Translation = XMVectorSet(-10.0f, 0.0f, 0.0f, 0);

		goFarmhouse->ComponentRender.PtrObject3D = ObjectFarmhouse;

		goFarmhouse->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSet(0, 1.0f, -2.5f, 0);
		goFarmhouse->ComponentPhysics.BoundingSphere.Radius = 11.5f;
	}

	CGameObject3D* goYBot{ Game.AddGameObject3D("YBot") };
	{
		goYBot->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);
		goYBot->ComponentTransform.Scaling = XMVectorSet(0.02f, 0.02f, 0.02f, 0);

		goYBot->ComponentRender.PtrObject3D = ObjectYBot;
		goYBot->ComponentRender.PtrVS = Game.VSAnimation.get();

		goYBot->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSet(0, 1.5f, 0, 0);
		goYBot->ComponentPhysics.BoundingSphere.Radius = 2.0f;
	}

	CGameObject3DLine* goGrid{ Game.AddGameObject3DLine("Grid") };
	{
		goGrid->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);

		goGrid->ComponentRender.PtrObject3DLine = Game.AddObject3DLine();
		goGrid->ComponentRender.PtrObject3DLine->Create(Generate3DGrid(20));
	}

	CGameObject2D* go2DRectangle{ Game.AddGameObject2D("2DRectangle") };
	{
		go2DRectangle->ComponentRender.PtrObject2D = Game.AddObject2D();
		go2DRectangle->ComponentRender.PtrObject2D->CreateDynamic(Generate2DRectangle(XMFLOAT2(100, 50)));
		go2DRectangle->ComponentRender.PtrTexture = TextureGround;
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
			if (KeyState.D1)
			{
				Game.Set3DGizmoMode(E3DGizmoMode::Translation);
			}
			if (KeyState.D2)
			{
				Game.Set3DGizmoMode(E3DGizmoMode::Rotation);
			}
			if (KeyState.D3)
			{
				Game.Set3DGizmoMode(E3DGizmoMode::Scaling);
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
			static int PrevMouseX{ MouseState.x };
			static int PrevMouseY{ MouseState.y };
			if (MouseState.leftButton)
			{
				Game.Pick();
			}
			if (MouseState.x != PrevMouseX || MouseState.y != PrevMouseY)
			{
				if (MouseState.rightButton)
				{
					MainCamera->RotateCamera(MouseState.x - PrevMouseX, MouseState.y - PrevMouseY, 0.01f);
				}

				PrevMouseX = MouseState.x;
				PrevMouseY = MouseState.y;
			}
			if (MouseState.scrollWheelValue)
			{
				MainCamera->ZoomCamera(MouseState.scrollWheelValue, 0.01f);
			}

			Game.Animate();
			Game.Draw(DeltaTimeF);

			static SpriteBatch* PtrSpriteBatch{ Game.GetSpriteBatchPtr() };
			static SpriteFont* PtrSpriteFont{ Game.GetSpriteFontPtr() };
			PtrSpriteBatch->Begin();
			{
				PtrSpriteFont->DrawString(PtrSpriteBatch, to_string(DeltaTimeF).c_str(), XMVectorSet(0, 0, 0, 0));
				if (Game.GetPickedGameObject3DName())
				{
					PtrSpriteFont->DrawString(PtrSpriteBatch, (string("Picked: ") + Game.GetPickedGameObject3DName()).c_str(),
						XMVectorSet(0, 15, 0, 0));
				}
				if (Game.GetCapturedPickedGameObject3DName())
				{
					PtrSpriteFont->DrawString(PtrSpriteBatch, (string("Captured: ") + Game.GetCapturedPickedGameObject3DName()).c_str(),
						XMVectorSet(0, 30, 0, 0));
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