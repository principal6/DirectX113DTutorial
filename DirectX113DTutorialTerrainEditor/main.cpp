#include <chrono>
#include "Core/Game.h"
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

	Game.SetGameRenderingFlags(CGame::EFlagsRendering::UseLighting | CGame::EFlagsRendering::DrawMiniAxes | CGame::EFlagsRendering::UseTerrainSelector |
		CGame::EFlagsRendering::DrawTerrainHeightMapTexture | CGame::EFlagsRendering::DrawTerrainMaskingTexture | CGame::EFlagsRendering::TessellateTerrain);

	CCamera* MainCamera{ Game.AddCamera(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0))) };
	MainCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));

	CGameObject3DLine* goGrid{ Game.AddGameObject3DLine("Grid") };
	{
		goGrid->ComponentTransform.Translation = XMVectorSet(0.0f, 0.0f, 0.0f, 0);

		goGrid->ComponentRender.PtrObject3DLine = Game.AddObject3DLine();
		goGrid->ComponentRender.PtrObject3DLine->Create(Generate3DGrid(0));
	}

	CMaterial MaterialDefaultGround{};
	{
		MaterialDefaultGround.SetName("DefaultGround");
		MaterialDefaultGround.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGround.SetDiffuseTextureFileName("Asset\\ground.png");
		MaterialDefaultGround.SetNormalTextureFileName("Asset\\ground_normal.png");
		Game.AddMaterial(MaterialDefaultGround);
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
				MainCamera->Move(CCamera::EMovementDirection::Forward, DeltaTimeF * 10.0f);
			}
			if (KeyState.S)
			{
				MainCamera->Move(CCamera::EMovementDirection::Backward, DeltaTimeF * 10.0f);
			}
			if (KeyState.A)
			{
				MainCamera->Move(CCamera::EMovementDirection::Leftward, DeltaTimeF * 10.0f);
			}
			if (KeyState.D)
			{
				MainCamera->Move(CCamera::EMovementDirection::Rightward, DeltaTimeF * 10.0f);
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

			// Mouse input
			const Mouse::State& MouseState{ Game.GetMouseState() };
			static int PrevMouseX{ MouseState.x };
			static int PrevMouseY{ MouseState.y };
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
			{
				if ((bLeftButton || bRightButton) && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
				{
					Game.SelectTerrain(true, bLeftButton);
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
						MainCamera->Rotate(MouseState.x - PrevMouseX, MouseState.y - PrevMouseY, 0.01f);
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
				static bool bShowMaterialEditor{ true };
				static bool bShowCameraEditor{ true };
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
						ImGui::MenuItem(u8"���� ������", nullptr, &bShowMaterialEditor);
						ImGui::MenuItem(u8"ī�޶� ������", nullptr, &bShowCameraEditor);
						
						ImGui::EndMenu();
					}

					if (ImGui::MenuItem(u8"����", "Alt+Q"))
					{
						Game.Destroy();
					}

					ImGui::EndMainMenuBar();
				}


				// ### ���� ������ ������ ###
				ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_Always);
				if (bShowTerrainGenerator) ImGui::OpenPopup(u8"���� ������");
				if (ImGui::BeginPopupModal(u8"���� ������", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
				{
					static int SizeX{ CTerrain::KDefaultSize };
					static int SizeZ{ CTerrain::KDefaultSize };
					static float MaskingDetail{ CTerrain::KMaskingDefaultDetail };
					
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

					ImGui::PushID(2);
					ImGui::Text(u8"����ŷ ������:");
					ImGui::SameLine();
					ImGui::PushItemWidth(62);
					ImGui::DragFloat("", &MaskingDetail, 1.0f, CTerrain::KMaskingMinDetail, CTerrain::KMaskingMaxDetail, "%.0f");
					ImGui::PopItemWidth();
					ImGui::PopID();

					ImGui::Separator();

					if (ImGui::Button(u8"����") || ImGui::IsKeyDown(VK_RETURN))
					{
						CTerrain::EEditMode eEditMode{};
						if (Game.GetTerrain()) eEditMode = Game.GetTerrain()->GetEditMode();

						XMFLOAT2 TerrainSize{ (float)SizeX, (float)SizeZ };
						Game.CreateTerrain(TerrainSize, MaterialDefaultGround, MaskingDetail);

						SizeX = CTerrain::KDefaultSize;
						SizeZ = CTerrain::KDefaultSize;

						Game.SetTerrainEditMode(eEditMode);

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();

					if (ImGui::Button(u8"�ݱ�") || ImGui::IsKeyDown(VK_ESCAPE))
					{
						SizeX = CTerrain::KDefaultSize;
						SizeZ = CTerrain::KDefaultSize;

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}


				// ### ���� ������ ������ ###
				if (bShowTerrainEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(500, 22), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(300, 100), ImVec2(300, 400));
					if (ImGui::Begin(u8"���� ������", &bShowTerrainEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						CTerrain* Terrain{ Game.GetTerrain() };
						if (Terrain)
						{
							static bool bShouldRecalculateTerrainVertexNormals{ false };
							static float TerrainSelectionSize{ CTerrain::KSelectionMinSize };
							static float TerrainSetHeightValue{};
							static float TerrainDeltaHeightValue{ CTerrain::KHeightUnit };
							static int TerrainMaskingLayer{};
							static float TerrainMaskingRadius{ CTerrain::KMaskingDefaultRadius };
							static float TerrainMaskingRatio{ CTerrain::KMaskingDefaultRatio };
							static float TerrainMaskingAttenuation{ CTerrain::KMaskingDefaultAttenuation };
							const char* Lists[]{ u8"<���� ���� ���>", u8"<���� ���� ���>", u8"<����ŷ ���>" };
							static int iSelectedItem{};

							const XMFLOAT2& TerrainSize{ Game.GetTerrain()->GetSize() };
							ImGui::Text(u8"���� x ����: %d x %d", (int)TerrainSize.x, (int)TerrainSize.y);
							ImGui::Text(u8"����ŷ ������: %d", (int)Game.GetTerrain()->GetMaskingDetail());

							ImGui::SetNextItemWidth(100);
							float WaterHeight{ Terrain->GetWaterHeight() };
							if (ImGui::DragFloat(u8"�� ����", &WaterHeight, CTerrain::KWaterHeightUnit, 
								CTerrain::KWaterMinHeight, CTerrain::KWaterMaxHeight, "%.1f"))
							{
								Terrain->SetWaterHeight(WaterHeight);
							}

							ImGui::Separator();

							for (int iListItem = 0; iListItem < ARRAYSIZE(Lists); ++iListItem)
							{
								if (ImGui::Selectable(Lists[iListItem], (iListItem == iSelectedItem) ? true : false))
								{
									iSelectedItem = iListItem;

									switch (iSelectedItem)
									{
									case 0:
										Game.SetTerrainEditMode(CTerrain::EEditMode::SetHeight);
										break;
									case 1:
										Game.SetTerrainEditMode(CTerrain::EEditMode::DeltaHeight);
										break;
									case 2:
										Game.SetTerrainEditMode(CTerrain::EEditMode::Masking);
										break;
									default:
										break;
									}
								}
							}

							if (iSelectedItem == 0)
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"������ ����", &TerrainSetHeightValue, CTerrain::KHeightUnit,
									CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
								{
									Game.GetTerrain()->SetSetHeightValue(TerrainSetHeightValue);
								}
								ImGui::PopItemWidth();
							}
							else if (iSelectedItem == 2)
							{
								ImGui::PushItemWidth(100);
								if (ImGui::DragInt(u8"����ŷ ���̾�", &TerrainMaskingLayer, 1.0f, 0, CTerrain::KMaterialMaxCount - 2, "%.0f"))
								{
									switch (TerrainMaskingLayer)
									{
									case 0:
										Game.SetTerrainMaskingLayer(CTerrain::EMaskingLayer::LayerR);
										break;
									case 1:
										Game.SetTerrainMaskingLayer(CTerrain::EMaskingLayer::LayerG);
										break;
									case 2:
										Game.SetTerrainMaskingLayer(CTerrain::EMaskingLayer::LayerB);
										break;
									case 3:
										Game.SetTerrainMaskingLayer(CTerrain::EMaskingLayer::LayerA);
										break;
									default:
										break;
									}
								}
								ImGui::PopItemWidth();

								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"����ŷ ���� ����", &TerrainMaskingRatio, CTerrain::KMaskingRatioUnit, 
									CTerrain::KMaskingMinRatio, CTerrain::KMaskingMaxRatio, "%.3f"))
								{
									Game.GetTerrain()->SetMaskingValue(TerrainMaskingRatio);
								}
								ImGui::PopItemWidth();

								ImGui::PushItemWidth(100);
								if (ImGui::DragFloat(u8"����ŷ ����", &TerrainMaskingAttenuation, CTerrain::KMaskingAttenuationUnit,
									CTerrain::KMaskingMinAttenuation, CTerrain::KMaskingMaxAttenuation, "%.3f"))
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

							ImGui::Text(u8"���� ���� ��ġ: (%.0f, %.0f)", Game.GetTerrainSelectionPosition().x, Game.GetTerrainSelectionPosition().y);

							ImGui::Separator();

							static bool bShowMaterialSelection{ false };
							static int iSelectedMaterialID{};
							if (Game.GetTerrain())
							{
								if (ImGui::Button(u8"���� �߰�"))
								{
									iSelectedMaterialID = -1;
									ImGui::OpenPopup(u8"���� ����");
								}

								for (int iMaterial = 0; iMaterial < Game.GetTerrain()->GetMaterialCount(); ++iMaterial)
								{
									ImGui::PushItemWidth(100);
									ImGui::PushID(iMaterial);
									if (ImGui::Button(u8"����"))
									{
										iSelectedMaterialID = iMaterial;
										bShowMaterialSelection = true;
									}
									ImGui::PopID();
									ImGui::PopItemWidth();

									ImGui::SameLine();
									
									const CMaterial& Material{ Game.GetTerrain()->GetMaterial(iMaterial) };
									ImGui::Text(u8"����[%d] %s", iMaterial, Material.GetName().c_str());
								}
							}
							else
							{
								ImGui::Text(u8"�ؽ���: ����");
							}

							if (bShowMaterialSelection) ImGui::OpenPopup(u8"���� ����");
							if (ImGui::BeginPopup(u8"���� ����", ImGuiWindowFlags_AlwaysAutoResize))
							{
								static int ListIndex{};
								static const string* SelectedMaterialName{};
								const auto& mapMaterialList{ Game.GetMaterialListMap() };

								if (ImGui::ListBoxHeader("", (int)mapMaterialList.size()))
								{
									int iListItem{};
									for (const auto& pairMaterial : mapMaterialList)
									{
										if (ImGui::Selectable(pairMaterial.first.c_str(), (iListItem == ListIndex) ? true : false))
										{
											ListIndex = iListItem;
											SelectedMaterialName = &pairMaterial.first;
										}
										++iListItem;
									}
									ImGui::ListBoxFooter();
								}

								if (ImGui::Button(u8"����"))
								{
									if (SelectedMaterialName)
									{
										if (iSelectedMaterialID == -1)
										{
											Game.AddTerrainMaterial(*Game.GetMaterial(*SelectedMaterialName));
										}
										else
										{
											Game.SetTerrainMaterial(iSelectedMaterialID, *Game.GetMaterial(*SelectedMaterialName));
										}
									}

									ImGui::CloseCurrentPopup();
								}

								bShowMaterialSelection = false;

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

				// ### ���� ������ ������ ###
				if (bShowMaterialEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(400, 400), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(400, 60), ImVec2(400, 400));
					if (ImGui::Begin(u8"���� ������", &bShowMaterialEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						ImGui::PushID(0);
						if (ImGui::Button(u8"�� ���� �߰�"))
						{
							size_t Count{ Game.GetMaterialCount() };

							CMaterial Material{};
							Material.SetName("Material" + to_string(Count));
							Material.ShouldGenerateAutoMipMap(true);

							Game.AddMaterial(Material);
						}
						ImGui::PopID();

						if (ImGui::CollapsingHeader(u8"���� ���", ImGuiTreeNodeFlags_SpanAvailWidth))
						{
							static constexpr float KUniformWidth{ 180.0f };
							static float AmbientColor[3]{};
							static float DiffuseColor[3]{};
							static float SpecularColor[3]{};
							static float SpecularExponent{};
							static float SpecularIntensity{};
							static constexpr const char KLabelAdd[]{ u8"�߰�" };
							static constexpr const char KLabelChange[]{ u8"����" };

							static constexpr size_t KNameMaxLength{ 255 };
							static char OldName[KNameMaxLength]{};
							static char NewName[KNameMaxLength]{};
							static bool bShowMaterialNameChangeWindow{ false };
							const auto& mapMaterialList{ Game.GetMaterialListMap() };
							for (auto& pairMaterial : mapMaterialList)
							{
								CMaterial* Material{ Game.GetMaterial(pairMaterial.first) };
								
								if (ImGui::TreeNodeEx(pairMaterial.first.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
								{
									if (ImGui::Button(u8"�̸� ����"))
									{
										strcpy_s(OldName, Material->GetName().c_str());
										strcpy_s(NewName, Material->GetName().c_str());

										bShowMaterialNameChangeWindow = true;
									}

									ImGui::SetNextItemWidth(KUniformWidth);
									const XMFLOAT3& AmbientColor3{ Material->GetAmbientColor() };
									AmbientColor[0] = AmbientColor3.x;
									AmbientColor[1] = AmbientColor3.y;
									AmbientColor[2] = AmbientColor3.z;
									if (ImGui::ColorEdit3(u8"ȯ�汤(Ambient)", AmbientColor, ImGuiColorEditFlags_RGB))
									{
										Material->SetAmbientColor(XMFLOAT3(AmbientColor[0], AmbientColor[1], AmbientColor[2]));
									}

									ImGui::SetNextItemWidth(KUniformWidth);
									const XMFLOAT3& DiffuseColor3{ Material->GetDiffuseColor() };
									DiffuseColor[0] = DiffuseColor3.x;
									DiffuseColor[1] = DiffuseColor3.y;
									DiffuseColor[2] = DiffuseColor3.z;
									if (ImGui::ColorEdit3(u8"���ݻ籤(Diffuse)", DiffuseColor, ImGuiColorEditFlags_RGB))
									{
										Material->SetDiffuseColor(XMFLOAT3(DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]));
									}

									ImGui::SetNextItemWidth(KUniformWidth);
									const XMFLOAT3& SpecularColor3{ Material->GetSpecularColor() };
									SpecularColor[0] = SpecularColor3.x;
									SpecularColor[1] = SpecularColor3.y;
									SpecularColor[2] = SpecularColor3.z;
									if (ImGui::ColorEdit3(u8"���ݻ籤(Specular)", SpecularColor, ImGuiColorEditFlags_RGB))
									{
										Material->SetSpecularColor(XMFLOAT3(SpecularColor[0], SpecularColor[1], SpecularColor[2]));
									}

									ImGui::SetNextItemWidth(KUniformWidth);
									SpecularExponent = Material->GetSpecularExponent();
									if (ImGui::DragFloat(u8"���ݻ籤(Specular) ����", &SpecularExponent, 0.001f, 0.0f, 1.0f, "%.3f"))
									{
										Material->SetSpecularExponent(SpecularExponent);
									}

									ImGui::SetNextItemWidth(KUniformWidth);
									SpecularIntensity = Material->GetSpecularIntensity();
									if (ImGui::DragFloat(u8"���ݻ籤(Specular) ����", &SpecularIntensity, 0.001f, 0.0f, 1.0f, "%.3f"))
									{
										Material->SetSpecularIntensity(SpecularIntensity);
									}
									
									static char FileName[MAX_PATH]{};
									static OPENFILENAME ofn{};
									ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
									ofn.lpstrFilter = "PNG ����\0*.png\0JPG ����\0*.jpg\0��� ����\0*.*\0";
									ofn.lpstrFile = FileName;
									ofn.lpstrTitle = "�ؽ��� �ҷ�����";
									ofn.lStructSize = sizeof(OPENFILENAME);
									ofn.nMaxFile = MAX_PATH;

									// Diffuse texture
									{
										const char* PtrDiffuseTextureLabel{ KLabelAdd };
										if (Material->HasDiffuseTexture())
										{
											ImGui::Image(Game.GetMaterialDiffuseTexture(pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
											ImGui::SameLine();
											ImGui::SetNextItemWidth(KUniformWidth);
											ImGui::Text(u8"Diffuse �ؽ���");

											PtrDiffuseTextureLabel = KLabelChange;
										}
										else
										{
											ImGui::Image(0, ImVec2(100, 100));
											ImGui::SameLine();
											ImGui::SetNextItemWidth(KUniformWidth);
											ImGui::Text(u8"Diffuse �ؽ���: ����");

											PtrDiffuseTextureLabel = KLabelAdd;
										}

										ImGui::SameLine();

										ImGui::PushID(0);
										if (ImGui::Button(PtrDiffuseTextureLabel))
										{
											if (GetOpenFileName(&ofn))
											{
												SetCurrentDirectoryA(WorkingDirectory);
												Material->SetDiffuseTextureFileName(FileName);
												Game.UpdateMaterial(pairMaterial.first);
											}
										}
										ImGui::PopID();
									}
									
									// Normal texture
									{
										const char* PtrNormalTextureLabel{ KLabelAdd };
										if (Material->HasNormalTexture())
										{
											ImGui::Image(Game.GetMaterialNormalTexture(pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
											ImGui::SameLine();
											ImGui::Text(u8"Normal �ؽ���");

											PtrNormalTextureLabel = KLabelChange;
										}
										else
										{
											ImGui::Image(0, ImVec2(100, 100));
											ImGui::SameLine();
											ImGui::SetNextItemWidth(KUniformWidth);
											ImGui::Text(u8"Normal �ؽ���: ����");

											PtrNormalTextureLabel = KLabelAdd;
										}

										ImGui::SameLine();

										ImGui::PushID(1);
										if (ImGui::Button(PtrNormalTextureLabel))
										{
											if (GetOpenFileName(&ofn))
											{
												SetCurrentDirectoryA(WorkingDirectory);
												Material->SetNormalTextureFileName(FileName);
												Game.UpdateMaterial(pairMaterial.first);
											}
										}
										ImGui::PopID();
									}

									ImGui::TreePop();
								}
							}

							// ### ���� �̸� ���� ������ ###
							if (bShowMaterialNameChangeWindow) ImGui::OpenPopup(u8"���� �̸� ����");
							{
								ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_Always);
								if (ImGui::BeginPopupModal(u8"���� �̸� ����", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::SetNextItemWidth(160);
									ImGui::InputText(u8"�� �̸�", NewName, KNameMaxLength, ImGuiInputTextFlags_EnterReturnsTrue);

									ImGui::Separator();

									if (ImGui::Button(u8"����") || ImGui::IsKeyDown(VK_RETURN))
									{
										if (mapMaterialList.find(NewName) != mapMaterialList.end())
										{
											MessageBox(nullptr, "�̹� �����ϴ� �̸��Դϴ�. �ٸ� �̸��� ��� �ּ���.",
												"�̸� �浹", MB_OK | MB_ICONWARNING);
										}
										else
										{
											Game.ChangeMaterialName(OldName, NewName);

											ImGui::CloseCurrentPopup();
											bShowMaterialNameChangeWindow = false;
										}
									}

									ImGui::SameLine();

									if (ImGui::Button(u8"�ݱ�") || ImGui::IsKeyDown(VK_ESCAPE))
									{
										ImGui::CloseCurrentPopup();
										bShowMaterialNameChangeWindow = false;
									}

									ImGui::EndPopup();
								}
							}
						}
					}
					ImGui::End();
				}


				// ### ī�޶� ������ ������ ###
				if (bShowCameraEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(200, 22), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(300, 60), ImVec2(300, 200));
					if (ImGui::Begin(u8"ī�޶� ������", &bShowCameraEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						const XMVECTOR& KEyePosition{ MainCamera->GetEyePosition() };
						float EyePosition[3]{ XMVectorGetX(KEyePosition), XMVectorGetY(KEyePosition), XMVectorGetZ(KEyePosition) };
						float Pitch{ MainCamera->GetPitch() };
						float Yaw{ MainCamera->GetYaw() };

						ImGui::SetNextItemWidth(180);
						if (ImGui::DragFloat3(u8"��ġ", EyePosition, 0.01f, -10000.0f, +10000.0f, "%.3f"))
						{
							MainCamera->SetEyePosition(XMVectorSet(EyePosition[0], EyePosition[1], EyePosition[2], 1.0f));
						}

						ImGui::SetNextItemWidth(180);
						if (ImGui::DragFloat(u8"ȸ�� Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
						{
							MainCamera->SetPitch(Pitch);
						}

						ImGui::SetNextItemWidth(180);
						if (ImGui::DragFloat(u8"ȸ�� Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
						{
							MainCamera->SetYaw(Yaw);
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