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

	Game.SetGameRenderingFlags(EFlagsGameRendering::UseLighting | EFlagsGameRendering::DrawMiniAxes | 
		EFlagsGameRendering::UseTerrainSelector | EFlagsGameRendering::DrawTerrainMaskingTexture);

	CCamera* MainCamera{ Game.AddCamera(SCameraData(ECameraType::FreeLook, XMVectorSet(0, +2.0f, 0, 0), XMVectorSet(0, +2.0f, +1.0f, 0))) };

	CTexture* TextureGround{ Game.AddTexture("ground.png") };
	{
		TextureGround->CreateFromFile("Asset\\ground.png");
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
				static bool bShowTerrainEditor{ true };
				static bool bShowTextureListWindow{ true };
				static bool bShowTerrainGenerator{ false };
				static bool bShowOpenFileDialog{ false };
				static bool bShowSaveFileDialog{ false };

				// ### �޴� �� ###
				if (ImGui::BeginMainMenuBar())
				{
					if (KeyState.LeftControl && KeyState.N) bShowTerrainGenerator = true;
					if (KeyState.LeftControl && KeyState.O) bShowOpenFileDialog = true;
					if (KeyState.LeftControl && KeyState.S) bShowSaveFileDialog = true;

					if (ImGui::BeginMenu(u8"����"))
					{
						if (ImGui::MenuItem(u8"�����", "Ctrl+N", nullptr)) bShowTerrainGenerator = true;
						if (ImGui::MenuItem(u8"�ҷ�����", "Ctrl+O", nullptr)) bShowOpenFileDialog = true;
						if (ImGui::MenuItem(u8"��������", "Ctrl+S", nullptr)) bShowSaveFileDialog = true;

						ImGui::EndMenu();
					}

					if (bShowOpenFileDialog)
					{
						char FileName[MAX_PATH]{};
						OPENFILENAME ofn{};
						ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
						ofn.lpstrFilter = "���� ����(*.terr)\0*.terr\0";
						ofn.lpstrFile = FileName;
						ofn.lpstrTitle = "���� �� �ҷ�����";
						ofn.lStructSize = sizeof(OPENFILENAME);
						ofn.nMaxFile = MAX_PATH;
						if (GetOpenFileName(&ofn))
						{
							SetCurrentDirectoryA(WorkingDirectory);
							Game.LoadTerrain(FileName);
						}

						bShowOpenFileDialog = false;
					}

					if (bShowSaveFileDialog)
					{
						if (Game.GetTerrain())
						{
							char FileName[MAX_PATH]{};
							OPENFILENAME ofn{};
							ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
							ofn.lpstrDefExt = ".terr";
							ofn.lpstrFilter = "���� ����(*.terr)\0*.terr\0";
							ofn.lpstrFile = FileName;
							ofn.lpstrTitle = "���� �� ��������";
							ofn.lStructSize = sizeof(OPENFILENAME);
							ofn.nMaxFile = MAX_PATH;
							if (GetSaveFileName(&ofn))
							{
								SetCurrentDirectoryA(WorkingDirectory);
								Game.SaveTerrain(FileName);
							}
						}

						bShowSaveFileDialog = false;
					}

					if (ImGui::BeginMenu(u8"â"))
					{
						ImGui::MenuItem(u8"���� ������", nullptr, &bShowTerrainEditor);
						ImGui::MenuItem(u8"�ؽ��� ���", nullptr, &bShowTextureListWindow);
						
						ImGui::EndMenu();
					}

					if (ImGui::MenuItem(u8"����", "Alt+Q"))
					{
						Game.Destroy();
					}

					ImGui::EndMainMenuBar();
				}


				// ### ���� ������ ������ ###
				ImGui::SetNextWindowSize(ImVec2(200, 120), ImGuiCond_Always);
				if (bShowTerrainGenerator) ImGui::OpenPopup(u8"���� ������");
				if (ImGui::BeginPopupModal(u8"���� ������", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
				{
					static int SizeX{ CTerrain::KMinSize };
					static int SizeZ{ CTerrain::KMinSize };
					
					ImGui::PushID(0);
					ImGui::Text(u8"����:");
					ImGui::SameLine();
					ImGui::DragInt("", &SizeX, 2.0f, 2, 1000);
					ImGui::PopID();
					if (SizeX % 2) ++SizeX;

					ImGui::PushID(1);
					ImGui::Text(u8"����:");
					ImGui::SameLine();
					ImGui::DragInt("", &SizeZ, 2.0f, 2, 1000);
					ImGui::PopID();
					if (SizeZ % 2) ++SizeZ;

					if (ImGui::Button(u8"����") || ImGui::IsKeyDown(VK_RETURN))
					{
						XMFLOAT2 TerrainSize{ (float)SizeX, (float)SizeZ };
						Game.CreateTerrain(TerrainSize, TextureGround->GetFileName());

						SizeX = CTerrain::KMinSize;
						SizeZ = CTerrain::KMinSize;

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();

					if (ImGui::Button(u8"�ݱ�") || ImGui::IsKeyDown(VK_ESCAPE))
					{
						SizeX = CTerrain::KMinSize;
						SizeZ = CTerrain::KMinSize;

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}


				// ### ���� ������ ������ ###
				if (bShowTerrainEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiCond_Appearing);
					ImGui::SetNextWindowSize(ImVec2(340, 260), ImGuiCond_Appearing);

					if (ImGui::Begin(u8"���� ������", &bShowTerrainEditor))
					{
						if (Game.GetTerrain())
						{
							static bool bShouldRecalculateTerrainVertexNormals{ false };
							static float TerrainHeightSetValue{};
							static float TerrainSelectionSize{ CTerrain::KSelectionMinSize };
							static int TerrainMaskingLayer{};
							static float TerrainMaskingRadius{ CTerrain::KMaskingDefaultRadius };
							static float TerrainMaskingRatio{};
							static float TerrainMaskingAttenuation{ CTerrain::KMaskingAttenuationMax };
							const char* Lists[]{ u8"<���� ���� ���>", u8"<���� ���� ���>", u8"<����ŷ ���>" };
							static int iSelectedItem{};

							for (int iListItem = 0; iListItem < ARRAYSIZE(Lists); ++iListItem)
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
										Game.SetTerrainEditMode(ETerrainEditMode::DeltaHeight, CTerrain::KHeightUnit);
										break;
									case 2:
										Game.SetTerrainEditMode(ETerrainEditMode::Masking, TerrainMaskingRatio);
										break;
									default:
										break;
									}
								}
							}

							if (iSelectedItem == 0)
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"������ ����", &TerrainHeightSetValue, CTerrain::KHeightUnit,
									CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
								{
									Game.SetTerrainEditMode(ETerrainEditMode::SetHeight, TerrainHeightSetValue);
								}
								ImGui::PopItemWidth();
							}

							if (iSelectedItem == 2)
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragInt(u8"����ŷ ���̾�", &TerrainMaskingLayer, 1.0f, 0, CTerrain::KTextureMaxCount - 2, "%.0f"))
								{
									switch (TerrainMaskingLayer)
									{
									case 0:
										Game.SetTerrainMaskingLayer(EMaskingLayer::LayerR);
										break;
									case 1:
										Game.SetTerrainMaskingLayer(EMaskingLayer::LayerG);
										break;
									case 2:
										Game.SetTerrainMaskingLayer(EMaskingLayer::LayerB);
										break;
									case 3:
										Game.SetTerrainMaskingLayer(EMaskingLayer::LayerA);
										break;
									default:
										break;
									}
								}
								ImGui::PopItemWidth();

								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"����ŷ ���� ����", &TerrainMaskingRatio, CTerrain::KMaskingRatioUnit, 
									0.0f, 1.0f, "%.3f"))
								{
									Game.SetTerrainEditMode(ETerrainEditMode::Masking, TerrainMaskingRatio);
								}
								ImGui::PopItemWidth();

								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"����ŷ ����", &TerrainMaskingAttenuation, CTerrain::KMaskingAttenuationUnit,
									CTerrain::KMaskingAttenuationMin, CTerrain::KMaskingAttenuationMax, "%.3f"))
								{
									Game.SetTerrainMaskingAttenuation(TerrainMaskingAttenuation);
								}
								ImGui::PopItemWidth();
							}

							if (iSelectedItem == 2)
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"����ŷ ������", &TerrainMaskingRadius, CTerrain::KMaskingRadiusUnit,
									CTerrain::KMaskingMinRadius, CTerrain::KMaskingMaxRadius, "%.1f"))
								{
									Game.SetTerrainMaskingSize(TerrainMaskingRadius);
								}
								if (MouseState.scrollWheelValue)
								{
									if (MouseState.scrollWheelValue > 0) TerrainMaskingRadius += CTerrain::KMaskingRadiusUnit;
									if (MouseState.scrollWheelValue < 0) TerrainMaskingRadius -= CTerrain::KMaskingRadiusUnit;
									Game.SetTerrainMaskingSize(TerrainMaskingRadius);
								}
								ImGui::PopItemWidth();
							}
							else
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"���� ���� ũ��", &TerrainSelectionSize, CTerrain::KSelectionSizeUnit,
									CTerrain::KSelectionMinSize, CTerrain::KSelectionMaxSize, "%.0f"))
								{
									Game.SetTerrainSelectionSize(TerrainSelectionSize);
								}
								if (MouseState.scrollWheelValue)
								{
									if (MouseState.scrollWheelValue > 0) TerrainSelectionSize += CTerrain::KSelectionSizeUnit;
									if (MouseState.scrollWheelValue < 0) TerrainSelectionSize -= CTerrain::KSelectionSizeUnit;
									Game.SetTerrainSelectionSize(TerrainSelectionSize);
								}
								ImGui::PopItemWidth();
							}

							ImGui::Separator();

							ImGui::Text(u8"���� ���� ��ġ: (%.0f, %.0f)", Game.GetTerrainSelectionRoundUpPosition().x, Game.GetTerrainSelectionRoundUpPosition().y);

							if (ImGui::Button(u8"��ü ���� ����"))
							{
								Game.RecalculateTerrainNormals();
							}

							ImGui::Separator();

							static bool bShowTextureSelection{ false };
							static int iSelectedTextureID{};
							if (Game.GetTerrain())
							{
								if (ImGui::Button(u8"�ؽ��� �߰�"))
								{
									iSelectedTextureID = -1;
									ImGui::OpenPopup(u8"�ؽ��� ����");
								}

								for (int iTexture = 0; iTexture < Game.GetTerrain()->GetTextureCount(); ++iTexture)
								{
									ImGui::PushItemWidth(100);
									ImGui::PushID(iTexture);
									if (ImGui::Button(u8"����"))
									{
										iSelectedTextureID = iTexture;
										bShowTextureSelection = true;
									}
									ImGui::PopID();
									ImGui::PopItemWidth();

									ImGui::SameLine();

									ImGui::Text(u8"�ؽ���[%d]: %s", iTexture, Game.GetTerrain()->GetTextureFileName(iTexture).c_str());
								}
							}
							else
							{
								ImGui::Text(u8"�ؽ���: ����");
							}

							if (bShowTextureSelection) ImGui::OpenPopup(u8"�ؽ��� ����");
							if (ImGui::BeginPopup(u8"�ؽ��� ����", ImGuiWindowFlags_AlwaysAutoResize))
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

								if (ImGui::Button(u8"����"))
								{
									if (SelectedTextureName)
									{
										if (iSelectedTextureID == -1)
										{
											Game.AddTerrainTexture(Game.GetTexture(*SelectedTextureName)->GetFileName());
										}
										else
										{
											Game.SetTerrainTexture(iSelectedTextureID, Game.GetTexture(*SelectedTextureName)->GetFileName());
										}
									}

									ImGui::CloseCurrentPopup();
								}

								bShowTextureSelection = false;

								ImGui::EndPopup();
							}
							
						}
						else
						{
							ImGui::Text(u8"<���� ������ ����ų� �ҷ�������.>");
						}
					}

					ImGui::End();
				}

				// ### �ؽ��� ��� ������ ###
				if (bShowTextureListWindow)
				{
					ImGui::SetNextWindowPos(ImVec2(460, 290), ImGuiCond_Appearing);
					ImGui::SetNextWindowSize(ImVec2(340, 120), ImGuiCond_Appearing);

					if (ImGui::Begin(u8"�ؽ��� ���", &bShowTextureListWindow, ImGuiWindowFlags_AlwaysAutoResize))
					{
						static int ListIndex{};
						static const string* SelectedTextureName{};
						const auto& TextureListMap{ Game.GetTextureListMap() };

						ImGui::PushID(0);
						if (ImGui::Button(u8"�߰�"))
						{
							char FileName[MAX_PATH]{};
							OPENFILENAME ofn{};
							ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
							ofn.lpstrFilter = "PNG ����\0*.png\0JPG ����\0*.jpg\0��� ����\0*.*\0";
							ofn.lpstrFile = FileName;
							ofn.lpstrTitle = "�ؽ��� �ҷ�����";
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