#include <chrono>
#include "Core/Game.h"
#include "Core/AssimpLoader.h"
#include "Core/ModelPorter.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

using std::chrono::steady_clock;

static ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	constexpr float KClearColor[4]{ 0.2f, 0.6f, 0.9f, 1.0f };

	char WorkingDirectory[MAX_PATH]{};
	GetCurrentDirectoryA(MAX_PATH, WorkingDirectory);
	
	CGame Game{ hInstance, XMFLOAT2(800, 600) };
	Game.CreateWin32(WndProc, TEXT("Game"), L"Asset\\dotumche_10_korean.spritefont", true);
	
	Game.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	Game.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));

	Game.SetGameRenderingFlags(EFlagsGameRendering::UseLighting | EFlagsGameRendering::DrawMiniAxes | EFlagsGameRendering::UseTerrainSelector);

	CCamera* MainCamera{ Game.AddCamera(SCameraData(ECameraType::FreeLook, XMVectorSet(0, +2.0f, 0, 0), XMVectorSet(0, +2.0f, +1.0f, 0))) };

	CTexture* TextureGround{ Game.AddTexture("ground.png") };
	{
		TextureGround->CreateFromFile("Asset\\ground.png");
	}

	CObject3D* ObjectTerrain{ Game.AddObject3D() };
	{
		ObjectTerrain->Create(GenerateTerrainBase(XMFLOAT2(4, 4)));
	}

	CGameObject3D* goTerrain{ Game.AddGameObject3D("Terrain") };
	{
		goTerrain->ComponentRender.PtrObject3D = nullptr;
		goTerrain->ComponentRender.PtrTexture = TextureGround;

		goTerrain->ComponentPhysics.bIgnoreBoundingSphere = true;
	}

	CGameObject3DLine* goGrid{ Game.AddGameObject3DLine("Grid") };
	{
		goGrid->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);

		goGrid->ComponentRender.PtrObject3DLine = Game.AddObject3DLine();
		goGrid->ComponentRender.PtrObject3DLine->Create(Generate3DGrid(0));
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(Game.GethWnd());
	ImGui_ImplDX11_Init(Game.GetDevicePtr(), Game.GetDeviceContextPtr());

	ImGuiIO& igIO{ ImGui::GetIO() };
	igIO.Fonts->AddFontDefault();
	ImFont* igFont{ igIO.Fonts->AddFontFromFileTTF("Asset/D2Coding.ttf", 16.0f, nullptr, igIO.Fonts->GetGlyphRangesKorean()) };
	
	while (true)
	{
		static MSG Msg{};
		static char KeyDown{};
		static bool bLeftButton{ false };
		static bool bRightButton{ false };

		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_LBUTTONDOWN) bLeftButton = true;
			if (Msg.message == WM_RBUTTONDOWN) bRightButton = true;
			if (Msg.message == WM_MOUSEMOVE)
			{
				if (Msg.wParam == MK_LBUTTON) bLeftButton = true;
				if (Msg.wParam == MK_RBUTTON) bRightButton = true;
			}
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

			// Keyboard input
			const Keyboard::State& KeyState{ Game.GetKeyState() };
			if (KeyState.LeftAlt && KeyState.Q)
			{
				Game.Destroy();
			}
			if (KeyState.Escape)
			{
				Game.ReleasePickedGameObject();
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
				Game.ToggleGameRenderingFlags(EFlagsGameRendering::DrawMiniAxes);
			}


			// Mouse input
			const Mouse::State& MouseState{ Game.GetMouseState() };
			static int PrevMouseX{ MouseState.x };
			static int PrevMouseY{ MouseState.y };
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
			{
				if (bLeftButton || bRightButton)
				{
					if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) Game.SelectTerrain(true, bLeftButton);
				}
				else
				{
					Game.SelectTerrain(false, false);
				}
				
				if (MouseState.leftButton)
				{
					Game.Pick();
				}
				if (MouseState.x != PrevMouseX || MouseState.y != PrevMouseY)
				{
					Game.SelectTerrain(false, false);

					if (MouseState.middleButton)
					{
						MainCamera->RotateCamera(MouseState.x - PrevMouseX, MouseState.y - PrevMouseY, 0.01f);
					}

					PrevMouseX = MouseState.x;
					PrevMouseY = MouseState.y;
				}
			}

			Game.Animate();
			Game.Draw(DeltaTimeF);

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			ImGui::PushFont(igFont);

			{
				static bool bShowGameObjectEditor{ true };
				static bool bShowTextureListWindow{ true };
				static bool bShowTerrainGenerator{ false };
				static bool bShowOpenFileDialog{ false };
				static bool bShowSaveFileDialog{ false };

				if (ImGui::BeginMainMenuBar())
				{
					if (KeyState.LeftControl && KeyState.N) bShowTerrainGenerator = true;
					if (KeyState.LeftControl && KeyState.O) bShowOpenFileDialog = true;
					if (KeyState.LeftControl && KeyState.S) bShowSaveFileDialog = true;

					if (ImGui::BeginMenu(u8"지형"))
					{
						if (ImGui::MenuItem(u8"만들기", "Ctrl+N", nullptr)) bShowTerrainGenerator = true;
						if (ImGui::MenuItem(u8"불러오기", "Ctrl+O", nullptr)) bShowOpenFileDialog = true;
						if (ImGui::MenuItem(u8"내보내기", "Ctrl+S", nullptr)) bShowSaveFileDialog = true;

						ImGui::EndMenu();
					}

					if (bShowOpenFileDialog)
					{
						char FileName[MAX_PATH]{};
						OPENFILENAME ofn{};
						ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
						ofn.lpstrFilter = "지형 파일(*.terr)\0*.terr\0";
						ofn.lpstrFile = FileName;
						ofn.lpstrTitle = "지형 모델 불러오기";
						ofn.lStructSize = sizeof(OPENFILENAME);
						ofn.nMaxFile = MAX_PATH;
						if (GetOpenFileName(&ofn))
						{
							SetCurrentDirectoryA(WorkingDirectory);

							if (!goTerrain->ComponentRender.PtrObject3D) goTerrain->ComponentRender.PtrObject3D = Game.AddObject3D();

							SModel TerrainModel{};
							XMFLOAT2 TerrainSize{};
							ImportTerrain(FileName, TerrainModel, TerrainSize);
							goTerrain->ComponentRender.PtrObject3D->Create(TerrainModel);

							Game.SetTerrain(goTerrain, TerrainSize);
						}

						bShowOpenFileDialog = false;
					}

					if (bShowSaveFileDialog)
					{
						if (Game.GetTerrainModelPtr())
						{
							char FileName[MAX_PATH]{};
							OPENFILENAME ofn{};
							ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
							ofn.lpstrDefExt = ".terr";
							ofn.lpstrFilter = "지형 파일(*.terr)\0*.terr\0";
							ofn.lpstrFile = FileName;
							ofn.lpstrTitle = "지형 모델 내보내기";
							ofn.lStructSize = sizeof(OPENFILENAME);
							ofn.nMaxFile = MAX_PATH;
							if (GetSaveFileName(&ofn))
							{
								SetCurrentDirectoryA(WorkingDirectory);

								ExportTerrain(*Game.GetTerrainModelPtr(), Game.GetTerrainSize(), FileName);
							}
							SetCurrentDirectoryA(WorkingDirectory);
						}

						bShowSaveFileDialog = false;
					}

					if (ImGui::BeginMenu(u8"창"))
					{
						ImGui::MenuItem(u8"지형 편집기", nullptr, &bShowGameObjectEditor);
						ImGui::MenuItem(u8"텍스쳐 목록", nullptr, &bShowTextureListWindow);
						
						ImGui::EndMenu();
					}

					if (ImGui::MenuItem(u8"종료", "Alt+Q"))
					{
						Game.Destroy();
					}

					ImGui::EndMainMenuBar();
				}

				ImGui::SetNextWindowSize(ImVec2(200, 120), ImGuiCond_Always);
				if (bShowTerrainGenerator) ImGui::OpenPopup(u8"지형 생성기");
				if (ImGui::BeginPopupModal(u8"지형 생성기", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
				{
					static int SizeX{ CGame::KTerrainMinSize };
					static int SizeZ{ CGame::KTerrainMinSize };
					
					ImGui::PushID(0);
					ImGui::Text(u8"가로:");
					ImGui::SameLine();
					ImGui::DragInt("", &SizeX, 2.0f, 2, 1000);
					ImGui::PopID();
					if (SizeX % 2) ++SizeX;

					ImGui::PushID(1);
					ImGui::Text(u8"세로:");
					ImGui::SameLine();
					ImGui::DragInt("", &SizeZ, 2.0f, 2, 1000);
					ImGui::PopID();
					if (SizeZ % 2) ++SizeZ;

					if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
					{
						XMFLOAT2 TerrainSize{ (float)SizeX, (float)SizeZ };

						if (!goTerrain->ComponentRender.PtrObject3D) goTerrain->ComponentRender.PtrObject3D = Game.AddObject3D();

						SMaterial Material{};
						Material.bHasTexture = true;
						Material.TextureFileName = TextureGround->GetFileName();

						goTerrain->ComponentRender.PtrObject3D->Create(GenerateTerrainBase(TerrainSize), Material);
						
						Game.SetTerrain(goTerrain, TerrainSize);

						SizeX = CGame::KTerrainMinSize;
						SizeZ = CGame::KTerrainMinSize;

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();

					if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
					{
						SizeX = CGame::KTerrainMinSize;
						SizeZ = CGame::KTerrainMinSize;

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (bShowGameObjectEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiCond_Appearing);
					ImGui::SetNextWindowSize(ImVec2(340, 230), ImGuiCond_Appearing);

					if (ImGui::Begin(u8"지형 편집기", &bShowGameObjectEditor))
					{
						if (goTerrain)
						{
							static bool bShouldRecalculateTerrainVertexNormals{ false };
							static float TerrainHeightSetValue{};
							static float TerrainSelectionSize{ CGame::KTerrainSelectionMinSize };
							const char* Lists[]{ u8"<높이 지정 모드>", u8"<높이 변경 모드>" };
							static int iSelectedItem{};

							for (int iListItem = 0; iListItem < 2; ++iListItem)
							{
								if (ImGui::Selectable(Lists[iListItem], (iListItem == iSelectedItem) ? true : false))
								{
									iSelectedItem = iListItem;

									switch (iSelectedItem)
									{
									case 0:
										Game.SetTerrainEditMode(ETerrainEditMode::SetHeight, TerrainHeightSetValue);
										break;
									case 1:
										Game.SetTerrainEditMode(ETerrainEditMode::DeltaHeight, CGame::KTerrainHeightUnit);
										break;
									default:
										break;
									}
								}
							}

							if (iSelectedItem == 0)
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"지정할 높이", &TerrainHeightSetValue, CGame::KTerrainHeightUnit,
									CGame::KTerrainMinHeight, CGame::KTerrainMaxHeight, "%.1f"))
								{
									Game.SetTerrainEditMode(ETerrainEditMode::SetHeight, TerrainHeightSetValue);
								}
								ImGui::PopItemWidth();
							}

							ImGui::PushItemWidth(100);
							if (ImGui::DragFloat(u8"지형 선택 크기", &TerrainSelectionSize, CGame::KTerrainSelectionSizeUnit, 
								CGame::KTerrainSelectionMinSize, CGame::KTerrainSelectionMaxSize, "%.0f"))
							{
								Game.SetTerrainSelectionSize(TerrainSelectionSize);
							}
							if (MouseState.scrollWheelValue)
							{
								if (MouseState.scrollWheelValue > 0) TerrainSelectionSize += CGame::KTerrainSelectionSizeUnit;
								if (MouseState.scrollWheelValue < 0) TerrainSelectionSize -= CGame::KTerrainSelectionSizeUnit;
								Game.SetTerrainSelectionSize(TerrainSelectionSize);
							}
							ImGui::PopItemWidth();

							ImGui::Separator();

							ImGui::Text(u8"현재 선택 위치: (%.0f, %.0f)", Game.GetTerrainSelectionPosition().x, Game.GetTerrainSelectionPosition().y);

							if (ImGui::Button(u8"전체 법선 재계산"))
							{
								Game.UpdateTerrainVertexNormals();
							}

							ImGui::Separator();

							if (ImGui::Button(u8"텍스쳐 변경")) ImGui::OpenPopup(u8"텍스쳐 선택");
							if (goTerrain->ComponentRender.PtrTexture)
							{
								ImGui::Text(u8"텍스쳐: %s", goTerrain->ComponentRender.PtrTexture->GetName().c_str());
							}
							else
							{
								ImGui::Text(u8"텍스쳐: 없음");
							}

							if (ImGui::BeginPopup(u8"텍스쳐 선택", ImGuiWindowFlags_AlwaysAutoResize))
							{
								static int ListIndex{};
								static const string* SelectedTextureName{};
								const auto& TextureListMap{ Game.GetTextureListMap() };

								if (ImGui::ListBoxHeader("", (int)TextureListMap.size()))
								{
									int iListItem{};
									for (const auto& Texture : TextureListMap)
									{
										if (ImGui::Selectable(Texture.first.c_str(), (iListItem == ListIndex) ? true : false))
										{
											ListIndex = iListItem;
											SelectedTextureName = &Texture.first;
										}
										++iListItem;
									}
									ImGui::ListBoxFooter();
								}

								if (ImGui::Button(u8"결정"))
								{
									if (SelectedTextureName) goTerrain->ComponentRender.PtrTexture = Game.GetTexture(*SelectedTextureName);

									ImGui::CloseCurrentPopup();
								}

								ImGui::EndPopup();
							}
							
						}
						else
						{
							ImGui::Text(u8"<먼저 GameObject를 선택해 주세요.>");
						}
					}

					ImGui::End();
				}
				
				if (bShowTextureListWindow)
				{
					ImGui::SetNextWindowPos(ImVec2(460, 250), ImGuiCond_Appearing);
					ImGui::SetNextWindowSize(ImVec2(340, 120), ImGuiCond_Appearing);

					if (ImGui::Begin(u8"텍스쳐 목록", &bShowTextureListWindow, ImGuiWindowFlags_AlwaysAutoResize))
					{
						static int ListIndex{};
						static const string* SelectedTextureName{};
						const auto& TextureListMap{ Game.GetTextureListMap() };

						ImGui::PushID(0);
						if (ImGui::Button(u8"추가"))
						{
							char FileName[MAX_PATH]{};
							OPENFILENAME ofn{};
							ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
							ofn.lpstrFilter = "PNG 파일\0*.png\0JPG 파일\0*.jpg\0모든 파일\0*.*\0";
							ofn.lpstrFile = FileName;
							ofn.lpstrTitle = "텍스쳐 불러오기";
							ofn.lStructSize = sizeof(OPENFILENAME);
							ofn.nMaxFile = MAX_PATH;
							if (GetOpenFileName(&ofn))
							{
								SetCurrentDirectoryA(WorkingDirectory);

								string TextureName{ FileName };
								size_t FoundPos{ TextureName.find_last_of('\\') };
								CTexture* Texture{ Game.AddTexture(TextureName.substr(FoundPos + 1)) };
								if (Texture) Texture->CreateFromFile(FileName);
							}
						}
						ImGui::PopID();

						if (ImGui::ListBoxHeader("", (int)TextureListMap.size()))
						{
							int iListItem{};
							for (const auto& Texture : TextureListMap)
							{
								if (ImGui::Selectable(Texture.first.c_str(), (iListItem == ListIndex) ? true : false))
								{
									ListIndex = iListItem;
									SelectedTextureName = &Texture.first;
								}
								++iListItem;
							}
							ImGui::ListBoxFooter();
						}
					}

					ImGui::End();
				}

			}

			ImGui::PopFont();

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			Game.EndRendering();

			KeyDown = 0;
			bLeftButton = false;
			bRightButton = false;
			TimePrev = TimeNow;
		}
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

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