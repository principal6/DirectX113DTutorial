#include <chrono>
#include "Core/Game.h"
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
	CGame Game{ hInstance, XMFLOAT2(800, 600) };
	Game.CreateWin32(WndProc, TEXT("Game"), L"Asset\\dotumche_10_korean.spritefont", true);
	
	Game.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	Game.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));

	Game.SetGameRenderingFlags(CGame::EFlagsRendering::UseLighting | CGame::EFlagsRendering::DrawMiniAxes |
		CGame::EFlagsRendering::DrawTerrainHeightMapTexture | CGame::EFlagsRendering::DrawTerrainMaskingTexture | 
		CGame::EFlagsRendering::TessellateTerrain | CGame::EFlagsRendering::Use3DGizmos | CGame::EFlagsRendering::DrawBoundingSphere);

	CCamera* MainCamera{ Game.AddCamera(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0))) };
	MainCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));

	Game.InsertObject3DLine("Grid");
	{
		CObject3DLine* Grid{ Game.GetObject3DLine("Grid") };
		Grid->Create(Generate3DGrid(0));
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
			float DeltaTimeF{ static_cast<float>((TimeNow - TimePrev) * 0.000'000'001) };

			Game.BeginRendering(Colors::CornflowerBlue);

			// Keyboard input
			const Keyboard::State& KeyState{ Game.GetKeyState() };
			if (KeyState.LeftAlt && KeyState.Q)
			{
				Game.Destroy();
			}
			if (KeyState.Escape)
			{
				Game.ReleaseCapturedObject3D();
			}
			if (!ImGui::IsAnyItemActive())
			{
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
					Game.Set3DGizmoMode(CGame::E3DGizmoMode::Translation);
				}
				if (KeyState.D2)
				{
					Game.Set3DGizmoMode(CGame::E3DGizmoMode::Rotation);
				}
				if (KeyState.D3)
				{
					Game.Set3DGizmoMode(CGame::E3DGizmoMode::Scaling);
				}
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
				if (bRightButton) ImGui::SetWindowFocus(nullptr);

				if ((bLeftButton || bRightButton) && !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
				{
					if (bLeftButton)
					{
						Game.Pick();
					}

					if (bRightButton)
					{
						Game.ReleaseCapturedObject3D();
					}

					Game.SelectTerrain(true, bLeftButton);
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
				static bool bShowPropertyEditor{ true };
				static bool bShowSceneEditor{ true };
				static bool bShowTerrainGenerator{ false };
				static bool bShowOpenFileDialog{ false };
				static bool bShowSaveFileDialog{ false };

				// ### 메뉴 바 ###
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
						if (Game.OpenFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 불러오기"))
						{
							Game.LoadTerrain(Game.GetDialogFileNameWithPath());
						}

						bShowOpenFileDialog = false;
					}

					if (bShowSaveFileDialog)
					{
						if (Game.GetTerrain())
						{
							if (Game.SaveFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 내보내기", ".terr"))
							{
								Game.SaveTerrain(Game.GetDialogFileNameWithPath());
							}
						}

						bShowSaveFileDialog = false;
					}

					if (ImGui::BeginMenu(u8"창"))
					{
						ImGui::MenuItem(u8"속성 편집기", nullptr, &bShowPropertyEditor);
						ImGui::MenuItem(u8"장면 편집기", nullptr, &bShowSceneEditor);
						
						ImGui::EndMenu();
					}

					if (ImGui::MenuItem(u8"종료", "Alt+Q"))
					{
						Game.Destroy();
					}

					ImGui::EndMainMenuBar();
				}


				// ### 지형 생성기 윈도우 ###
				ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_Always);
				if (bShowTerrainGenerator) ImGui::OpenPopup(u8"지형 생성기");
				if (ImGui::BeginPopupModal(u8"지형 생성기", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
				{
					static int SizeX{ CTerrain::KDefaultSize };
					static int SizeZ{ CTerrain::KDefaultSize };
					static float MaskingDetail{ CTerrain::KMaskingDefaultDetail };
					
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

					ImGui::PushID(2);
					ImGui::Text(u8"마스킹 디테일:");
					ImGui::SameLine();
					ImGui::PushItemWidth(62);
					ImGui::DragFloat("", &MaskingDetail, 1.0f, CTerrain::KMaskingMinDetail, CTerrain::KMaskingMaxDetail, "%.0f");
					ImGui::PopItemWidth();
					ImGui::PopID();

					ImGui::Separator();

					if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
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

					if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
					{
						SizeX = CTerrain::KDefaultSize;
						SizeZ = CTerrain::KDefaultSize;

						bShowTerrainGenerator = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}


				// ### 속성 편집기 윈도우 ###
				if (bShowPropertyEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(400, 22), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(400, 100), ImVec2(400, 600));
					if (ImGui::Begin(u8"속성 편집기", &bShowPropertyEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						if (ImGui::BeginTabBar(u8"탭바", ImGuiTabBarFlags_None))
						{
							if (ImGui::BeginTabItem(u8"오브젝트"))
							{
								Game.SetEditMode(CGame::EEditMode::EditObject);

								const char* KCapturedObject3DName{ Game.GetCapturedObject3DName() };
								if (KCapturedObject3DName)
								{
									CObject3D* Object3D{ Game.GetObject3D(KCapturedObject3DName) };

									ImGui::Text(u8"선택된 오브젝트: <%s>", KCapturedObject3DName);

									float Translation[3]{ XMVectorGetX(Object3D->ComponentTransform.Translation),
										XMVectorGetY(Object3D->ComponentTransform.Translation), XMVectorGetZ(Object3D->ComponentTransform.Translation) };
									if (ImGui::DragFloat3(u8"위치", Translation, CGame::KTranslationUnit,
										CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.1f"))
									{
										Object3D->ComponentTransform.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
										Object3D->UpdateWorldMatrix();
									}

									int PitchYawRoll360[3]{ (int)(Object3D->ComponentTransform.Pitch * CGame::KRotation2PITo360),
										(int)(Object3D->ComponentTransform.Yaw * CGame::KRotation2PITo360), 
										(int)(Object3D->ComponentTransform.Roll * CGame::KRotation2PITo360) };
									if (ImGui::DragInt3(u8"회전", PitchYawRoll360, CGame::KRotation360Unit,
										CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
									{
										Object3D->ComponentTransform.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
										Object3D->ComponentTransform.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
										Object3D->ComponentTransform.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
										Object3D->UpdateWorldMatrix();
									}

									float Scaling[3]{ XMVectorGetX(Object3D->ComponentTransform.Scaling),
										XMVectorGetY(Object3D->ComponentTransform.Scaling), XMVectorGetZ(Object3D->ComponentTransform.Scaling) };
									if (ImGui::DragFloat3(u8"크기", Scaling, CGame::KScalingUnit,
										CGame::KScalingMinLimit, CGame::KScalingMaxLimit, "%.2f"))
									{
										Object3D->ComponentTransform.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
										Object3D->UpdateWorldMatrix();
									}
								}
								else
								{
									ImGui::Text(u8"<먼저 오브젝트를 선택하세요.>");
								}

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"지형"))
							{
								Game.SetEditMode(CGame::EEditMode::EditTerrain);

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
									const char* Lists[]{ u8"<높이 지정 모드>", u8"<높이 변경 모드>", u8"<마스킹 모드>" };
									static int iSelectedItem{};

									const XMFLOAT2& TerrainSize{ Game.GetTerrain()->GetSize() };
									ImGui::Text(u8"가로 x 세로: %d x %d", (int)TerrainSize.x, (int)TerrainSize.y);
									ImGui::Text(u8"마스킹 디테일: %d", (int)Game.GetTerrain()->GetMaskingDetail());

									ImGui::SetNextItemWidth(100);
									float WaterHeight{ Terrain->GetWaterHeight() };
									if (ImGui::DragFloat(u8"물 높이", &WaterHeight, CTerrain::KWaterHeightUnit,
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
										if (ImGui::DragFloat(u8"지정할 높이", &TerrainSetHeightValue, CTerrain::KHeightUnit,
											CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
										{
											Game.GetTerrain()->SetSetHeightValue(TerrainSetHeightValue);
										}
										ImGui::PopItemWidth();
									}
									else if (iSelectedItem == 2)
									{
										ImGui::PushItemWidth(100);
										if (ImGui::SliderInt(u8"마스킹 레이어", &TerrainMaskingLayer, 0, CTerrain::KMaterialMaxCount - 2))
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
										if (ImGui::DragFloat(u8"마스킹 배합 비율", &TerrainMaskingRatio, CTerrain::KMaskingRatioUnit,
											CTerrain::KMaskingMinRatio, CTerrain::KMaskingMaxRatio, "%.3f"))
										{
											Game.GetTerrain()->SetMaskingValue(TerrainMaskingRatio);
										}
										ImGui::PopItemWidth();

										ImGui::PushItemWidth(100);
										if (ImGui::DragFloat(u8"마스킹 감쇠", &TerrainMaskingAttenuation, CTerrain::KMaskingAttenuationUnit,
											CTerrain::KMaskingMinAttenuation, CTerrain::KMaskingMaxAttenuation, "%.3f"))
										{
											Game.SetTerrainMaskingAttenuation(TerrainMaskingAttenuation);
										}
										ImGui::PopItemWidth();
									}

									if (iSelectedItem == 2)
									{
										ImGui::PushItemWidth(100);
										if (ImGui::DragFloat(u8"마스킹 반지름", &TerrainMaskingRadius, CTerrain::KMaskingRadiusUnit,
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
										if (ImGui::DragFloat(u8"지형 선택 크기", &TerrainSelectionSize, CTerrain::KSelectionSizeUnit,
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

									ImGui::Text(u8"현재 선택 위치: (%.0f, %.0f)", Game.GetTerrainSelectionPosition().x, Game.GetTerrainSelectionPosition().y);

									ImGui::Separator();

									static bool bShowMaterialSelection{ false };
									static int iSelectedMaterialID{};
									if (Game.GetTerrain())
									{
										if (ImGui::Button(u8"재질 추가"))
										{
											iSelectedMaterialID = -1;
											ImGui::OpenPopup(u8"재질 선택");
										}

										for (int iMaterial = 0; iMaterial < Game.GetTerrain()->GetMaterialCount(); ++iMaterial)
										{
											ImGui::PushItemWidth(100);
											ImGui::PushID(iMaterial);
											if (ImGui::Button(u8"변경"))
											{
												iSelectedMaterialID = iMaterial;
												bShowMaterialSelection = true;
											}
											ImGui::PopID();
											ImGui::PopItemWidth();

											ImGui::SameLine();

											const CMaterial& Material{ Game.GetTerrain()->GetMaterial(iMaterial) };
											ImGui::Text(u8"재질[%d] %s", iMaterial, Material.GetName().c_str());
										}
									}
									else
									{
										ImGui::Text(u8"텍스쳐: 없음");
									}

									if (bShowMaterialSelection) ImGui::OpenPopup(u8"재질 선택");
									if (ImGui::BeginPopup(u8"재질 선택", ImGuiWindowFlags_AlwaysAutoResize))
									{
										static int ListIndex{};
										static const string* SelectedMaterialName{};
										const auto& mapMaterial{ Game.GetMaterialMap() };

										if (ImGui::ListBoxHeader("", (int)mapMaterial.size()))
										{
											int iListItem{};
											for (const auto& pairMaterial : mapMaterial)
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

										if (ImGui::Button(u8"결정"))
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
									ImGui::Text(u8"<먼저 지형을 만들거나 불러오세요.>");
								}

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"재질"))
							{
								ImGui::PushID(0);
								if (ImGui::Button(u8"새 재질 추가"))
								{
									size_t Count{ Game.GetMaterialCount() };

									CMaterial Material{};
									Material.SetName("Material" + to_string(Count));
									Material.ShouldGenerateAutoMipMap(true);

									Game.AddMaterial(Material);
								}
								ImGui::PopID();

								static constexpr float KUniformWidth{ 180.0f };
								static float AmbientColor[3]{};
								static float DiffuseColor[3]{};
								static float SpecularColor[3]{};
								static float SpecularExponent{};
								static float SpecularIntensity{};
								static constexpr const char KLabelAdd[]{ u8"추가" };
								static constexpr const char KLabelChange[]{ u8"변경" };

								static constexpr size_t KNameMaxLength{ 255 };
								static char OldName[KNameMaxLength]{};
								static char NewName[KNameMaxLength]{};
								static bool bShowMaterialNameChangeWindow{ false };
								const auto& mapMaterialList{ Game.GetMaterialMap() };
								for (auto& pairMaterial : mapMaterialList)
								{
									CMaterial* Material{ Game.GetMaterial(pairMaterial.first) };

									if (ImGui::TreeNodeEx(pairMaterial.first.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
									{
										if (ImGui::Button(u8"이름 변경"))
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
										if (ImGui::ColorEdit3(u8"환경광(Ambient)", AmbientColor, ImGuiColorEditFlags_RGB))
										{
											Material->SetAmbientColor(XMFLOAT3(AmbientColor[0], AmbientColor[1], AmbientColor[2]));
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										const XMFLOAT3& DiffuseColor3{ Material->GetDiffuseColor() };
										DiffuseColor[0] = DiffuseColor3.x;
										DiffuseColor[1] = DiffuseColor3.y;
										DiffuseColor[2] = DiffuseColor3.z;
										if (ImGui::ColorEdit3(u8"난반사광(Diffuse)", DiffuseColor, ImGuiColorEditFlags_RGB))
										{
											Material->SetDiffuseColor(XMFLOAT3(DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]));
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										const XMFLOAT3& SpecularColor3{ Material->GetSpecularColor() };
										SpecularColor[0] = SpecularColor3.x;
										SpecularColor[1] = SpecularColor3.y;
										SpecularColor[2] = SpecularColor3.z;
										if (ImGui::ColorEdit3(u8"정반사광(Specular)", SpecularColor, ImGuiColorEditFlags_RGB))
										{
											Material->SetSpecularColor(XMFLOAT3(SpecularColor[0], SpecularColor[1], SpecularColor[2]));
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										SpecularExponent = Material->GetSpecularExponent();
										if (ImGui::DragFloat(u8"정반사광(Specular) 지수", &SpecularExponent, 0.001f, 0.0f, 1.0f, "%.3f"))
										{
											Material->SetSpecularExponent(SpecularExponent);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										SpecularIntensity = Material->GetSpecularIntensity();
										if (ImGui::DragFloat(u8"정반사광(Specular) 강도", &SpecularIntensity, 0.001f, 0.0f, 1.0f, "%.3f"))
										{
											Material->SetSpecularIntensity(SpecularIntensity);
										}

										static constexpr const char KTextureDialogFilter[]{ "PNG 파일\0*.png\0JPG 파일\0*.jpg\0모든 파일\0*.*\0" };
										static constexpr const char KTextureDialogTitle[]{ "텍스쳐 불러오기" };

										// Diffuse texture
										{
											const char* PtrDiffuseTextureLabel{ KLabelAdd };
											if (Material->HasDiffuseTexture())
											{
												ImGui::Image(Game.GetMaterialDiffuseTexture(pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
												ImGui::SameLine();
												ImGui::SetNextItemWidth(KUniformWidth);
												ImGui::Text(u8"Diffuse 텍스쳐");

												PtrDiffuseTextureLabel = KLabelChange;
											}
											else
											{
												ImGui::Image(0, ImVec2(100, 100));
												ImGui::SameLine();
												ImGui::SetNextItemWidth(KUniformWidth);
												ImGui::Text(u8"Diffuse 텍스쳐: 없음");

												PtrDiffuseTextureLabel = KLabelAdd;
											}

											ImGui::SameLine();

											ImGui::PushID(0);
											if (ImGui::Button(PtrDiffuseTextureLabel))
											{
												if (Game.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetDiffuseTextureFileName(Game.GetDialogFileNameWithPath());
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
												ImGui::Text(u8"Normal 텍스쳐");

												PtrNormalTextureLabel = KLabelChange;
											}
											else
											{
												ImGui::Image(0, ImVec2(100, 100));
												ImGui::SameLine();
												ImGui::SetNextItemWidth(KUniformWidth);
												ImGui::Text(u8"Normal 텍스쳐: 없음");

												PtrNormalTextureLabel = KLabelAdd;
											}

											ImGui::SameLine();

											ImGui::PushID(1);
											if (ImGui::Button(PtrNormalTextureLabel))
											{
												if (Game.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetNormalTextureFileName(Game.GetDialogFileNameWithPath());
													Game.UpdateMaterial(pairMaterial.first);
												}
											}
											ImGui::PopID();
										}

										ImGui::TreePop();
									}
								}

								// ### 재질 이름 변경 윈도우 ###
								if (bShowMaterialNameChangeWindow) ImGui::OpenPopup(u8"재질 이름 변경");
								{
									ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_Always);
									if (ImGui::BeginPopupModal(u8"재질 이름 변경", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
									{
										ImGui::SetNextItemWidth(160);
										ImGui::InputText(u8"새 이름", NewName, KNameMaxLength, ImGuiInputTextFlags_EnterReturnsTrue);

										ImGui::Separator();

										if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
										{
											if (mapMaterialList.find(NewName) != mapMaterialList.end())
											{
												MessageBox(nullptr, "이미 존재하는 이름입니다. 다른 이름을 골라 주세요.",
													"이름 충돌", MB_OK | MB_ICONWARNING);
											}
											else
											{
												Game.ChangeMaterialName(OldName, NewName);

												ImGui::CloseCurrentPopup();
												bShowMaterialNameChangeWindow = false;
											}
										}

										ImGui::SameLine();

										if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
										{
											ImGui::CloseCurrentPopup();
											bShowMaterialNameChangeWindow = false;
										}

										ImGui::EndPopup();
									}
								}

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"조명"))
							{
								const XMVECTOR& KDirectionalLightDirection{ Game.GetDirectionalLightDirection() };
								float DirectionalLightDirection[3]{ XMVectorGetX(KDirectionalLightDirection), XMVectorGetY(KDirectionalLightDirection),
									XMVectorGetZ(KDirectionalLightDirection) };

								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat3(u8" Directional 위치", DirectionalLightDirection, 0.02f, -1.0f, +1.0f, "%.2f"))
								{
									Game.SetDirectionalLight(XMVectorSet(DirectionalLightDirection[0], DirectionalLightDirection[1],
										DirectionalLightDirection[2], 0.0f));
								}

								const XMFLOAT3& KAmbientLightColor{ Game.GetAmbientLightColor() };
								float AmbientLightColor[3]{ KAmbientLightColor.x, KAmbientLightColor.y, KAmbientLightColor.z };

								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat3(u8" Ambient 색상", AmbientLightColor, 0.02f, 0.0f, +1.0f, "%.2f"))
								{
									Game.SetAmbientlLight(XMFLOAT3(AmbientLightColor[0], AmbientLightColor[1], AmbientLightColor[2]), Game.GetAmbientLightIntensity());
								}

								float AmbientLightIntensity{ Game.GetAmbientLightIntensity() };
								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat(u8" Ambient 강도", &AmbientLightIntensity, 0.02f, 0.0f, +1.0f, "%.2f"))
								{
									Game.SetAmbientlLight(Game.GetAmbientLightColor(), AmbientLightIntensity);
								}

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"카메라"))
							{
								const XMVECTOR& KEyePosition{ MainCamera->GetEyePosition() };
								float EyePosition[3]{ XMVectorGetX(KEyePosition), XMVectorGetY(KEyePosition), XMVectorGetZ(KEyePosition) };
								float Pitch{ MainCamera->GetPitch() };
								float Yaw{ MainCamera->GetYaw() };

								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat3(u8"위치", EyePosition, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									MainCamera->SetEyePosition(XMVectorSet(EyePosition[0], EyePosition[1], EyePosition[2], 1.0f));
								}

								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat(u8"회전 Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									MainCamera->SetPitch(Pitch);
								}

								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat(u8"회전 Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
								{
									MainCamera->SetYaw(Yaw);
								}

								ImGui::EndTabItem();
							}

							ImGui::EndTabBar();
						}
					}

					ImGui::End();
				}

				static bool bShowAddObject3D{ false };
				static bool bShowLoadModelDialog{ false };
				// ### 장면 편집기 윈도우 ###
				if (bShowSceneEditor)
				{
					static char SelectedObject3DName[CGame::KObject3DNameMaxLength]{};
					static int iSelectedObject3D{ -1 };
					const auto& mapObject3D{ Game.GetObject3DMap() };

					ImGui::SetNextWindowPos(ImVec2(0, 122), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(200, 60), ImVec2(200, 200));
					if (ImGui::Begin(u8"장면 편집기", &bShowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						if (ImGui::Button(u8"추가"))
						{
							bShowAddObject3D = true;
						}

						ImGui::SameLine();

						if (ImGui::Button(u8"삭제"))
						{
							Game.EraseObject3D(SelectedObject3DName);
							memset(SelectedObject3DName, 0, CGame::KObject3DNameMaxLength);
						}

						ImGui::Columns(1, "Object3Ds");
						ImGui::Separator();
						ImGui::Text(u8"오브젝트 목록"); ImGui::NextColumn();
						ImGui::Separator();

						int iPair{};
						for (const auto& pairObject3D : mapObject3D)
						{
							if (ImGui::Selectable(pairObject3D.first.c_str(), iSelectedObject3D == iPair, ImGuiSelectableFlags_SpanAllColumns))
							{
								iSelectedObject3D = iPair;
								strcpy_s(SelectedObject3DName, pairObject3D.first.c_str());
								Game.PickObject3D(SelectedObject3DName);
							}
							ImGui::NextColumn();
							++iPair;
						}
					}
					ImGui::End();
				}

				if (bShowAddObject3D) ImGui::OpenPopup(u8"AddObject3D");

				if (ImGui::BeginPopup(u8"AddObject3D"))
				{
					static char NewObejct3DName[CGame::KObject3DNameMaxLength]{};
					static char ModelFileNameWithPath[MAX_PATH]{};
					static char ModelFileNameWithoutPath[MAX_PATH]{};

					ImGui::SetItemDefaultFocus();
					ImGui::SetNextItemWidth(140);
					ImGui::InputText(u8"오브젝트 이름", NewObejct3DName, CGame::KObject3DNameMaxLength);

					ImGui::Text(ModelFileNameWithoutPath);
					ImGui::SameLine();
					if (ImGui::Button(u8"모델 불러오기"))
					{
						bShowLoadModelDialog = true;
					}
					
					if (ImGui::Button(u8"결정") || KeyState.Enter)
					{
						if (ModelFileNameWithPath[0] == 0)
						{
							MessageBox(nullptr, "모델을 불러오세요.", "모델 미지정", MB_OK | MB_ICONEXCLAMATION);
						}
						else
						{
							if (NewObejct3DName[0] != 0)
							{
								Game.InsertObject3D(NewObejct3DName);
								CObject3D* Object3D{ Game.GetObject3D(NewObejct3DName) };
								Object3D->CreateFromFile(ModelFileNameWithPath);
							}

							bShowAddObject3D = false;
							memset(ModelFileNameWithPath, 0, MAX_PATH);
							memset(ModelFileNameWithoutPath, 0, MAX_PATH);
							memset(NewObejct3DName, 0, CGame::KObject3DNameMaxLength);
							ImGui::CloseCurrentPopup();
						}
					}
					
					ImGui::SameLine();

					if (ImGui::Button(u8"취소"))
					{
						bShowAddObject3D = false;
						memset(ModelFileNameWithPath, 0, MAX_PATH);
						memset(ModelFileNameWithoutPath, 0, MAX_PATH);
						memset(NewObejct3DName, 0, CGame::KObject3DNameMaxLength);
						ImGui::CloseCurrentPopup();
					}

					if (bShowLoadModelDialog)
					{
						if (Game.OpenFileDialog("FBX 파일\0*.fbx\0모든 파일\0*.*\0", "모델 불러오기"))
						{
							strcpy_s(ModelFileNameWithPath, Game.GetDialogFileNameWithPath());
							strcpy_s(ModelFileNameWithoutPath, Game.GetDialogFileNameWithoutPath());
						}
						bShowLoadModelDialog = false;
					}

					ImGui::EndPopup();
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