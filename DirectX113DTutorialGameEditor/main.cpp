#include <chrono>
#include "Core/Game.h"
#include "Core/FileDialog.h"
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
	Game.CreateWin32(WndProc, TEXT("Game Editor"), L"Asset\\dotumche_10_korean.spritefont", true);
	
	Game.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	Game.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));

	Game.SetGameRenderingFlags(CGame::EFlagsRendering::UseLighting | CGame::EFlagsRendering::DrawMiniAxes |
		CGame::EFlagsRendering::DrawTerrainHeightMapTexture | CGame::EFlagsRendering::DrawTerrainMaskingTexture | 
		CGame::EFlagsRendering::DrawTerrainFoliagePlacingTexture |
		CGame::EFlagsRendering::TessellateTerrain | CGame::EFlagsRendering::Use3DGizmos);

	CCamera* MainCamera{ Game.AddCamera(CCamera::SCameraData(CCamera::EType::FreeLook, XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 0, 1, 0))) };
	MainCamera->SetEyePosition(XMVectorSet(0, 2, 0, 1));

	//Game.SetSky("Asset\\Sky.xml", 30.0f);

	Game.InsertObject3DLine("Grid");
	{
		CObject3DLine* Grid{ Game.GetObject3DLine("Grid") };
		Grid->Create(Generate3DGrid(0));
	}

	Game.InsertObject3D("YBot");
	{
		CObject3D* YBot{ Game.GetObject3D("YBot") };
		YBot->CreateFromFile("Asset\\ybot_mixamo_idle.fbx", true);
		YBot->AddAnimationFromFile("Asset\\ybot_mixamo_walking.fbx");
		YBot->AddAnimationFromFile("Asset\\ybot_mixamo_punching.fbx");

		YBot->ComponentTransform.Scaling = XMVectorSet(0.01f, 0.01f, 0.01f, 0);
		YBot->UpdateWorldMatrix();
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

	CMaterial MaterialDefaultGround0{};
	{
		MaterialDefaultGround0.SetName("DefaultGround0");
		MaterialDefaultGround0.ShouldGenerateAutoMipMap(true);
		MaterialDefaultGround0.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, "Asset\\ground0.jpg");
		MaterialDefaultGround0.SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, "Asset\\ground0_normal.jpg");
		MaterialDefaultGround0.SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, "Asset\\ground0_displacement.jpg");
		Game.AddMaterial(MaterialDefaultGround0);
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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(Game.GethWnd());
	ImGui_ImplDX11_Init(Game.GetDevicePtr(), Game.GetDeviceContextPtr());

	ImGuiIO& igIO{ ImGui::GetIO() };
	igIO.Fonts->AddFontDefault();
	ImFont* igFont{ igIO.Fonts->AddFontFromFileTTF("Asset/D2Coding.ttf", 16.0f, nullptr, igIO.Fonts->GetGlyphRangesKorean()) };
	
	// Main loop
	while (true)
	{
		static MSG Msg{};
		static char KeyDown{};
		static bool bLeftButtonPressedOnce{ false };
		if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT) break;

			if (Msg.message == WM_KEYDOWN) KeyDown = (char)Msg.wParam;
			
			if (Msg.message == WM_LBUTTONDOWN) bLeftButtonPressedOnce = true;
			if (Msg.message == WM_LBUTTONUP) bLeftButtonPressedOnce = false;

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
				Game.DeselectObject3D();
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
			if (KeyDown == VK_F4)
			{
				Game.ToggleGameRenderingFlags(CGame::EFlagsRendering::DrawBoundingSphere);
			}

			// Mouse input
			const Mouse::State& MouseState{ Game.GetMouseState() };
			static int PrevMouseX{ MouseState.x };
			static int PrevMouseY{ MouseState.y };
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
			{
				if (MouseState.rightButton) ImGui::SetWindowFocus(nullptr);

				if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
				{
					Game.Interact3DGizmos();

					if (bLeftButtonPressedOnce)
					{
						if (Game.Pick())
						{
							Game.SelectObject3D(Game.GetPickedObject3DName());
							Game.SelectInstance(Game.GetPickedInstanceID());
						}
						bLeftButtonPressedOnce = false;
					}

					if (MouseState.rightButton)
					{
						Game.DeselectObject3D();
					}

					if (MouseState.x != PrevMouseX || MouseState.y != PrevMouseY)
					{
						if (MouseState.leftButton || MouseState.rightButton)
						{
							Game.SelectTerrain(true, MouseState.leftButton);
						}
						else
						{
							Game.SelectTerrain(false, false);
						}
					}
				}
				else
				{
					Game.SelectTerrain(false, false);
				}

				if (MouseState.x != PrevMouseX || MouseState.y != PrevMouseY)
				{
					if (MouseState.middleButton)
					{
						MainCamera->Rotate(MouseState.x - PrevMouseX, MouseState.y - PrevMouseY, 0.01f);
					}

					PrevMouseX = MouseState.x;
					PrevMouseY = MouseState.y;
				}
			}

			Game.Animate(DeltaTimeF);
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
						static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
						if (FileDialog.OpenFileDialog("���� ����(*.terr)\0*.terr\0", "���� ���� �ҷ�����"))
						{
							Game.LoadTerrain(FileDialog.GetRelativeFileName());
						}
						bShowOpenFileDialog = false;
					}

					if (bShowSaveFileDialog)
					{
						if (Game.GetTerrain())
						{
							static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
							if (FileDialog.SaveFileDialog("���� ����(*.terr)\0*.terr\0", "���� ���� ��������", ".terr"))
							{
								Game.SaveTerrain(FileDialog.GetRelativeFileName());
							}
						}

						bShowSaveFileDialog = false;
					}

					if (ImGui::BeginMenu(u8"â"))
					{
						ImGui::MenuItem(u8"�Ӽ� ������", nullptr, &bShowPropertyEditor);
						ImGui::MenuItem(u8"��� ������", nullptr, &bShowSceneEditor);
						
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
						CTerrain::EEditMode eTerrainEditMode{};
						if (Game.GetTerrain()) eTerrainEditMode = Game.GetTerrain()->GetEditMode();

						XMFLOAT2 TerrainSize{ (float)SizeX, (float)SizeZ };
						Game.CreateTerrain(TerrainSize, MaterialDefaultGround0, MaskingDetail);

						CTerrain* const Terrain{ Game.GetTerrain() };

						SizeX = CTerrain::KDefaultSize;
						SizeZ = CTerrain::KDefaultSize;

						Game.SetEditMode(Game.GetEditMode(), true);
						Terrain->SetEditMode(eTerrainEditMode);

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

				// ### �Ӽ� ������ ������ ###
				if (bShowPropertyEditor)
				{
					ImGui::SetNextWindowPos(ImVec2(400, 22), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(400, 100), ImVec2(400, 578));
					
					if (ImGui::Begin(u8"�Ӽ� ������", &bShowPropertyEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						if (ImGui::BeginTabBar(u8"�ǹ�", ImGuiTabBarFlags_None))
						{
							if (ImGui::BeginTabItem(u8"������Ʈ"))
							{
								Game.SetEditMode(CGame::EEditMode::EditObject);

								if (Game.IsAnyObject3DSelected())
								{
									CObject3D* const Object3D{ Game.GetSelectedObject3D() };

									ImGui::Text(u8"���õ� ������Ʈ: <%s>", Game.GetSelectedObject3DName().c_str());

									if (Object3D->IsInstanced())
									{
										int iSelectedInstance{ Game.GetSelectedInstanceID() };
										if (iSelectedInstance == -1)
										{
											ImGui::Text(u8"<�ν��Ͻ��� ������ �ּ���.>");
										}
										else
										{
											auto& Instance{ Object3D->GetInstance(Game.GetSelectedInstanceID()) };
											ImGui::Text(u8"���õ� �ν��Ͻ�: <%s>", Instance.Name.c_str());

											float Translation[3]{ XMVectorGetX(Instance.Translation),
												XMVectorGetY(Instance.Translation), XMVectorGetZ(Instance.Translation) };
											if (ImGui::DragFloat3(u8"��ġ", Translation, CGame::KTranslationUnit,
												CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.1f"))
											{
												Instance.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
												Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
											}

											int PitchYawRoll360[3]{ (int)(Instance.Pitch * CGame::KRotation2PITo360),
												(int)(Instance.Yaw * CGame::KRotation2PITo360),
												(int)(Instance.Roll * CGame::KRotation2PITo360) };
											if (ImGui::DragInt3(u8"ȸ��", PitchYawRoll360, CGame::KRotation360Unit,
												CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
											{
												Instance.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
												Instance.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
												Instance.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
												Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
											}

											float Scaling[3]{ XMVectorGetX(Instance.Scaling),
												XMVectorGetY(Instance.Scaling), XMVectorGetZ(Instance.Scaling) };
											if (ImGui::DragFloat3(u8"ũ��", Scaling, CGame::KScalingUnit,
												CGame::KScalingMinLimit, CGame::KScalingMaxLimit, "%.3f"))
											{
												Instance.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
												Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
											}
										}
									}
									else
									{
										// Non-instanced Object3D

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"��ġ");
										ImGui::SameLine(120);
										float Translation[3]{ XMVectorGetX(Object3D->ComponentTransform.Translation),
										XMVectorGetY(Object3D->ComponentTransform.Translation), XMVectorGetZ(Object3D->ComponentTransform.Translation) };
										if (ImGui::DragFloat3(u8"##��ġ", Translation, CGame::KTranslationUnit,
											CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.1f"))
										{
											Object3D->ComponentTransform.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
											Object3D->UpdateWorldMatrix();
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"ȸ��");
										ImGui::SameLine(120);
										int PitchYawRoll360[3]{ (int)(Object3D->ComponentTransform.Pitch * CGame::KRotation2PITo360),
											(int)(Object3D->ComponentTransform.Yaw * CGame::KRotation2PITo360),
											(int)(Object3D->ComponentTransform.Roll * CGame::KRotation2PITo360) };
										if (ImGui::DragInt3(u8"##ȸ��", PitchYawRoll360, CGame::KRotation360Unit,
											CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
										{
											Object3D->ComponentTransform.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
											Object3D->ComponentTransform.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
											Object3D->ComponentTransform.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
											Object3D->UpdateWorldMatrix();
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"ũ��");
										ImGui::SameLine(120);
										float Scaling[3]{ XMVectorGetX(Object3D->ComponentTransform.Scaling),
											XMVectorGetY(Object3D->ComponentTransform.Scaling), XMVectorGetZ(Object3D->ComponentTransform.Scaling) };
										if (ImGui::DragFloat3(u8"##ũ��", Scaling, CGame::KScalingUnit,
											CGame::KScalingMinLimit, CGame::KScalingMaxLimit, "%.3f"))
										{
											Object3D->ComponentTransform.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
											Object3D->UpdateWorldMatrix();
										}
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"BS �߽�");
									ImGui::SameLine(120);
									float BSCenterOffset[3]{
										XMVectorGetX(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
										XMVectorGetY(Object3D->ComponentPhysics.BoundingSphere.CenterOffset), 
										XMVectorGetZ(Object3D->ComponentPhysics.BoundingSphere.CenterOffset) };
									if (ImGui::DragFloat3(u8"##BS�߽�", BSCenterOffset, CGame::KBSCenterOffsetUnit,
										CGame::KBSCenterOffsetMinLimit, CGame::KBSCenterOffsetMaxLimit, "%.2f"))
									{
										Object3D->ComponentPhysics.BoundingSphere.CenterOffset =
											XMVectorSet(BSCenterOffset[0], BSCenterOffset[1], BSCenterOffset[2], 1.0f);
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"BS ������");
									ImGui::SameLine(120);
									float BSRadius{ Object3D->ComponentPhysics.BoundingSphere.Radius };
									if (ImGui::DragFloat(u8"##BS������", &BSRadius, CGame::KBSRadiusUnit,
										CGame::KBSRadiusMinLimit, CGame::KBSRadiusMaxLimit, "%.2f"))
									{
										Object3D->ComponentPhysics.BoundingSphere.Radius = BSRadius;
									}

									if (Object3D->IsRiggedModel())
									{
										ImGui::Separator();

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"�ִϸ��̼� ID");
										ImGui::SameLine(120);
										int AnimationID{ Object3D->GetAnimationID() };
										if (ImGui::SliderInt(u8"##�ִϸ��̼�ID", &AnimationID, 0, Object3D->GetAnimationCount() - 1))
										{
											Object3D->SetAnimationID(AnimationID);
										}
									}
								}
								else
								{
									ImGui::Text(u8"<���� ������Ʈ�� �����ϼ���.>");
								}

								ImGui::EndTabItem();
							}

							// ���� ������
							if (ImGui::BeginTabItem(u8"����"))
							{
								Game.SetEditMode(CGame::EEditMode::EditTerrain);

								CTerrain* const Terrain{ Game.GetTerrain() };
								if (Terrain)
								{
									static const char* const KModeLabelList[4]{ 
										u8"<���� ���� ���>", u8"<���� ���� ���>", u8"<����ŷ ���>", u8"<�ʸ� ��ġ ���>" };
									static int iSelectedMode{};

									const XMFLOAT2& KTerrainSize{ Terrain->GetSize() };
									ImGui::Text(u8"���� x ����: %d x %d", (int)KTerrainSize.x, (int)KTerrainSize.y);
									ImGui::Text(u8"����ŷ ������: %d", (int)Game.GetTerrain()->GetMaskingDetail());

									ImGui::SetNextItemWidth(100);
									float TerrainTessFactor{ Terrain->GetTerrainTessFactor() };
									if (ImGui::DragFloat(u8"���� �׼����̼� ���", &TerrainTessFactor, CTerrain::KTessFactorUnit,
										CTerrain::KTessFactorMin, CTerrain::KTessFactorMax, "%.1f"))
									{
										Terrain->SetTerrainTessFactor(TerrainTessFactor);
									}

									ImGui::Separator();

									bool bDrawWater{ Terrain->ShouldDrawWater() };
									if (ImGui::Checkbox(u8"�� �׸���", &bDrawWater))
									{
										Terrain->ShouldDrawWater(bDrawWater);
									}

									ImGui::SetNextItemWidth(100);
									float WaterHeight{ Terrain->GetWaterHeight() };
									if (ImGui::DragFloat(u8"�� ����", &WaterHeight, CTerrain::KWaterHeightUnit,
										CTerrain::KWaterMinHeight, CTerrain::KWaterMaxHeight, "%.1f"))
									{
										Terrain->SetWaterHeight(WaterHeight);
									}

									ImGui::SetNextItemWidth(100);
									float WaterTessFactor{ Terrain->GetWaterTessFactor() };
									if (ImGui::DragFloat(u8"�� �׼����̼� ���", &WaterTessFactor, CTerrain::KTessFactorUnit,
										CTerrain::KTessFactorMin, CTerrain::KTessFactorMax, "%.1f"))
									{
										Terrain->SetWaterTessFactor(WaterTessFactor);
									}

									ImGui::Separator();

									ImGui::SetNextItemWidth(150);
									float FoliageDenstiy{ Terrain->GetFoliageDenstiy() };
									if (ImGui::DragFloat(u8"�ʸ� �е�", &FoliageDenstiy, 0.01f, 0.0f, 1.0f, "%.2f"))
									{
										Terrain->SetFoliageDensity(FoliageDenstiy);
									}

									ImGui::SetNextItemWidth(150);
									XMFLOAT3 WindVelocity{}; XMStoreFloat3(&WindVelocity, Terrain->GetWindVelocity());
									if (ImGui::DragFloat3(u8"�ٶ� �ӵ�", &WindVelocity.x, 0.001f, -10.0f, 10.0f, "%.3f"))
									{
										Terrain->SetWindVelocity(WindVelocity);
									}

									ImGui::SetNextItemWidth(150);
									float WindRadius{ Terrain->GetWindRadius() };
									if (ImGui::DragFloat(u8"�ٶ� ������", &WindRadius, 0.01f, 0.1f, 10.0f, "%.2f"))
									{
										Terrain->SetWindRadius(WindRadius);
									}

									ImGui::Separator();

									for (int iListItem = 0; iListItem < ARRAYSIZE(KModeLabelList); ++iListItem)
									{
										if (ImGui::Selectable(KModeLabelList[iListItem], (iListItem == iSelectedMode) ? true : false))
										{
											iSelectedMode = iListItem;

											switch (iSelectedMode)
											{
											case 0:
												Terrain->SetEditMode(CTerrain::EEditMode::SetHeight);
												break;
											case 1:
												Terrain->SetEditMode(CTerrain::EEditMode::DeltaHeight);
												break;
											case 2:
												Terrain->SetEditMode(CTerrain::EEditMode::Masking);
												break;
											case 3:
												Terrain->SetEditMode(CTerrain::EEditMode::FoliagePlacing);
												break;
											default:
												break;
											}
										}
									}

									if (iSelectedMode == 0)
									{
										ImGui::PushItemWidth(100);
										float TerrainSetHeightValue{ Terrain->GetSetHeightValue() };
										if (ImGui::DragFloat(u8"������ ����", &TerrainSetHeightValue, CTerrain::KHeightUnit,
											CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
										{
											Terrain->SetSetHeightValue(TerrainSetHeightValue);
										}
										ImGui::PopItemWidth();
									}
									else if (iSelectedMode == 2)
									{
										ImGui::PushItemWidth(100);
										int TerrainMaskingLayer{ (int)Terrain->GetMaskingLayer() };
										if (ImGui::SliderInt(u8"����ŷ ���̾�", &TerrainMaskingLayer, 0, CTerrain::KMaterialMaxCount - 2))
										{
											switch (TerrainMaskingLayer)
											{
											case 0:
												Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerR);
												break;
											case 1:
												Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerG);
												break;
											case 2:
												Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerB);
												break;
											case 3:
												Terrain->SetMaskingLayer(CTerrain::EMaskingLayer::LayerA);
												break;
											default:
												break;
											}
										}
										ImGui::PopItemWidth();

										ImGui::PushItemWidth(100);
										float TerrainMaskingRatio{ Terrain->GetMaskingRatio() };
										if (ImGui::DragFloat(u8"����ŷ ���� ����", &TerrainMaskingRatio, CTerrain::KMaskingRatioUnit,
											CTerrain::KMaskingMinRatio, CTerrain::KMaskingMaxRatio, "%.3f"))
										{
											Terrain->SetMaskingRatio(TerrainMaskingRatio);
										}
										ImGui::PopItemWidth();

										ImGui::PushItemWidth(100);
										float TerrainMaskingAttenuation{ Terrain->GetMaskingAttenuation() };
										if (ImGui::DragFloat(u8"����ŷ ����", &TerrainMaskingAttenuation, CTerrain::KMaskingAttenuationUnit,
											CTerrain::KMaskingMinAttenuation, CTerrain::KMaskingMaxAttenuation, "%.3f"))
										{
											Terrain->SetMaskingAttenuation(TerrainMaskingAttenuation);
										}
										ImGui::PopItemWidth();
									}

									if (iSelectedMode == 2)
									{
										ImGui::PushItemWidth(100);
										float TerrainMaskingRadius{ Terrain->GetMaskingRadius() };
										if (ImGui::DragFloat(u8"����ŷ ������", &TerrainMaskingRadius, CTerrain::KMaskingRadiusUnit,
											CTerrain::KMaskingMinRadius, CTerrain::KMaskingMaxRadius, "%.1f"))
										{
											Terrain->SetMaskingRadius(TerrainMaskingRadius);
										}
										if (MouseState.scrollWheelValue)
										{
											if (MouseState.scrollWheelValue > 0) TerrainMaskingRadius += CTerrain::KMaskingRadiusUnit;
											if (MouseState.scrollWheelValue < 0) TerrainMaskingRadius -= CTerrain::KMaskingRadiusUnit;
											Terrain->SetMaskingRadius(TerrainMaskingRadius);
										}
										ImGui::PopItemWidth();
									}
									else if (iSelectedMode == 3)
									{

									}
									else
									{
										ImGui::PushItemWidth(100);
										float TerrainSelectionSize{ Terrain->GetSelectionSize() };
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
									if (Terrain)
									{
										if (ImGui::Button(u8"���� �߰�"))
										{
											iSelectedMaterialID = -1;
											ImGui::OpenPopup(u8"���� ����");
										}

										for (int iMaterial = 0; iMaterial < Terrain->GetMaterialCount(); ++iMaterial)
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

											const CMaterial& Material{ Terrain->GetMaterial(iMaterial) };
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

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"����"))
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

								static constexpr float KUniformWidth{ 180.0f };
								static const char KLabelAdd[]{ u8"�߰�" };
								static const char KLabelChange[]{ u8"����" };

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
										if (ImGui::Button(u8"�̸� ����"))
										{
											strcpy_s(OldName, Material->GetName().c_str());
											strcpy_s(NewName, Material->GetName().c_str());

											bShowMaterialNameChangeWindow = true;
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										XMFLOAT3 AmbientColor{ Material->GetAmbientColor() };
										if (ImGui::ColorEdit3(u8"ȯ�汤(Ambient)", &AmbientColor.x, ImGuiColorEditFlags_RGB))
										{
											Material->SetAmbientColor(AmbientColor);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										XMFLOAT3 DiffuseColor{ Material->GetDiffuseColor() };
										if (ImGui::ColorEdit3(u8"���ݻ籤(Diffuse)", &DiffuseColor.x, ImGuiColorEditFlags_RGB))
										{
											Material->SetDiffuseColor(DiffuseColor);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										XMFLOAT3 SpecularColor{ Material->GetSpecularColor() };
										if (ImGui::ColorEdit3(u8"���ݻ籤(Specular)", &SpecularColor.x, ImGuiColorEditFlags_RGB))
										{
											Material->SetSpecularColor(SpecularColor);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										float SpecularExponent{ Material->GetSpecularExponent() };
										if (ImGui::DragFloat(u8"���ݻ籤(Specular) ����", &SpecularExponent, 0.001f, 0.0f, 1.0f, "%.3f"))
										{
											Material->SetSpecularExponent(SpecularExponent);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										float SpecularIntensity{ Material->GetSpecularIntensity() };
										if (ImGui::DragFloat(u8"���ݻ籤(Specular) ����", &SpecularIntensity, 0.001f, 0.0f, 1.0f, "%.3f"))
										{
											Material->SetSpecularIntensity(SpecularIntensity);
										}

										static const char KTextureDialogFilter[]{ "PNG ����\0*.png\0JPG ����\0*.jpg\0��� ����\0*.*\0" };
										static const char KTextureDialogTitle[]{ "�ؽ��� �ҷ�����" };

										// Diffuse texture
										{
											const char* PtrDiffuseTextureLabel{ KLabelAdd };
											if (Material->HasTexture(CMaterial::CTexture::EType::DiffuseTexture))
											{
												ImGui::Image(Game.GetMaterialTexture(CMaterial::CTexture::EType::DiffuseTexture,
													pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
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
												static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
												if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, FileDialog.GetRelativeFileName());
													Game.LoadMaterial(pairMaterial.first);
												}
											}
											ImGui::PopID();
										}

										// Normal texture
										{
											const char* PtrNormalTextureLabel{ KLabelAdd };
											if (Material->HasTexture(CMaterial::CTexture::EType::NormalTexture))
											{
												ImGui::Image(Game.GetMaterialTexture(CMaterial::CTexture::EType::NormalTexture,
													pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
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
												static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
												if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, FileDialog.GetRelativeFileName());
													Game.LoadMaterial(pairMaterial.first);
												}
											}
											ImGui::PopID();
										}

										// Displacement texture
										{
											const char* PtrDisplacementTextureLabel{ KLabelAdd };
											if (Material->HasTexture(CMaterial::CTexture::EType::DisplacementTexture))
											{
												ImGui::Image(Game.GetMaterialTexture(CMaterial::CTexture::EType::DisplacementTexture,
													pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
												ImGui::SameLine();
												ImGui::Text(u8"Displacement �ؽ���");

												PtrDisplacementTextureLabel = KLabelChange;
											}
											else
											{
												ImGui::Image(0, ImVec2(100, 100));
												ImGui::SameLine();
												ImGui::SetNextItemWidth(KUniformWidth);
												ImGui::Text(u8"Displacement �ؽ���: ����");

												PtrDisplacementTextureLabel = KLabelAdd;
											}

											ImGui::SameLine();

											ImGui::PushID(2);
											if (ImGui::Button(PtrDisplacementTextureLabel))
											{
												static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
												if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetTextureFileName(CMaterial::CTexture::EType::DisplacementTexture, FileDialog.GetRelativeFileName());
													Game.LoadMaterial(pairMaterial.first);
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

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"����"))
							{
								const XMVECTOR& KDirectionalLightDirection{ Game.GetDirectionalLightDirection() };
								float DirectionalLightDirection[3]{ XMVectorGetX(KDirectionalLightDirection), XMVectorGetY(KDirectionalLightDirection),
									XMVectorGetZ(KDirectionalLightDirection) };

								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat3(u8" Directional ��ġ", DirectionalLightDirection, 0.02f, -1.0f, +1.0f, "%.2f"))
								{
									Game.SetDirectionalLight(XMVectorSet(DirectionalLightDirection[0], DirectionalLightDirection[1],
										DirectionalLightDirection[2], 0.0f));
								}

								XMFLOAT3 AmbientLightColor{ Game.GetAmbientLightColor() };
								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat3(u8" Ambient ����", &AmbientLightColor.x, 0.02f, 0.0f, +1.0f, "%.2f"))
								{
									Game.SetAmbientlLight(AmbientLightColor, Game.GetAmbientLightIntensity());
								}

								float AmbientLightIntensity{ Game.GetAmbientLightIntensity() };
								ImGui::SetNextItemWidth(180);
								if (ImGui::DragFloat(u8" Ambient ����", &AmbientLightIntensity, 0.02f, 0.0f, +1.0f, "%.2f"))
								{
									Game.SetAmbientlLight(Game.GetAmbientLightColor(), AmbientLightIntensity);
								}

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem(u8"ī�޶�"))
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

								ImGui::EndTabItem();
							}

							ImGui::EndTabBar();
						}
					}

					ImGui::End();
				}

				static bool bShowAddObject3D{ false };
				static bool bShowLoadModelDialog{ false };

				// ### ��� ������ ������ ###
				if (bShowSceneEditor)
				{
					const auto& mapObject3D{ Game.GetObject3DMap() };

					ImGui::SetNextWindowPos(ImVec2(0, 122), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(300, 60), ImVec2(400, 300));
					if (ImGui::Begin(u8"��� ������", &bShowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						// ��� ��������
						if (ImGui::Button(u8"��� ��������"))
						{
							static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
							if (FileDialog.SaveFileDialog("��� ����(*.scene)\0*.scene\0", "��� ��������", ".scene"))
							{
								Game.SaveScene(FileDialog.GetRelativeFileName());
							}
							
						}

						ImGui::SameLine();

						// ��� �ҷ�����
						if (ImGui::Button(u8"��� �ҷ�����"))
						{
							static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("��� ����(*.scene)\0*.scene\0", "��� �ҷ�����"))
							{
								Game.LoadScene(FileDialog.GetRelativeFileName());
							}
						}

						ImGui::Separator();

						// ������Ʈ �߰�
						if (ImGui::Button(u8"������Ʈ �߰�"))
						{
							bShowAddObject3D = true;
						}

						ImGui::SameLine();

						// ������Ʈ ����
						if (ImGui::Button(u8"������Ʈ ����"))
						{
							Game.EraseObject3D(Game.GetSelectedObject3DName());
						}

						ImGui::Separator();

						ImGui::Columns(2);
						ImGui::Text(u8"������Ʈ �� �ν��Ͻ�"); ImGui::NextColumn();
						ImGui::Text(u8"�ν��Ͻ� ����"); ImGui::NextColumn();
						ImGui::Separator();

						// ������Ʈ ���
						int iObject3DPair{};
						for (const auto& Object3DPair : mapObject3D)
						{
							CObject3D* const Object3D{ Game.GetObject3D(Object3DPair.first) };
							bool bIsThisObject3DSelected{ false };
							if (Game.GetSelectedObject3DName() == Object3DPair.first) bIsThisObject3DSelected = true;

							ImGuiTreeNodeFlags Flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth };
							if (bIsThisObject3DSelected) Flags |= ImGuiTreeNodeFlags_Selected;
							if (!Object3D->IsInstanced()) Flags |= ImGuiTreeNodeFlags_Leaf;
							
							if (!Object3D->IsInstanced()) ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

							bool bIsNodeOpen{ ImGui::TreeNodeEx(Object3DPair.first.c_str(), Flags) };
							if (ImGui::IsItemClicked())
							{
								Game.SelectObject3D(Object3DPair.first);

								Game.SelectInstance(-1);
							}

							ImGui::NextColumn();

							if (!Object3D->IsRiggedModel())
							{
								// �ν��Ͻ� �߰�
								ImGui::PushID(iObject3DPair * 2 + 0);
								if (ImGui::Button(u8"�߰�"))
								{
									Object3D->InsertInstance();
								}
								ImGui::PopID();

								// �ν��Ͻ� ����
								if (Object3D->IsInstanced() && Game.IsAnyInstanceSelected())
								{
									ImGui::SameLine();

									ImGui::PushID(iObject3DPair * 2 + 1);
									if (ImGui::Button(u8"����"))
									{
										const CObject3D::SInstanceCPUData& Instance{ Object3D->GetInstance(Game.GetSelectedInstanceID()) };
										Object3D->DeleteInstance(Instance.Name);
									}
									ImGui::PopID();
								}
							}

							ImGui::NextColumn();

							if (bIsNodeOpen)
							{
								// �ν��Ͻ� ���
								if (Object3D->IsInstanced())
								{
									int iInstancePair{};
									const auto& InstanceMap{ Object3D->GetInstanceMap() };
									for (auto& InstancePair : InstanceMap)
									{
										bool bSelected{ (iInstancePair == Game.GetSelectedInstanceID()) };
										if (!bIsThisObject3DSelected) bSelected = false;

										if (ImGui::Selectable(InstancePair.first.c_str(), bSelected))
										{
											if (!bIsThisObject3DSelected)
											{
												Game.SelectObject3D(Object3DPair.first);
											}
											
											Game.SelectInstance(iInstancePair);
										}
										++iInstancePair;
									}
								}

								ImGui::TreePop();
							}

							++iObject3DPair;

							if (!Object3D->IsInstanced()) ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
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
					ImGui::InputText(u8"������Ʈ �̸�", NewObejct3DName, CGame::KObject3DNameMaxLength);

					if (ImGui::Button(u8"�� �ҷ�����"))
					{
						bShowLoadModelDialog = true;
					}
					ImGui::SameLine();

					ImGui::Text(ModelFileNameWithoutPath);

					ImGui::SameLine();
					
					static bool bIsModelRigged{ false };
					ImGui::Checkbox(u8"���� ����", &bIsModelRigged);
					
					if (ImGui::Button(u8"����") || KeyState.Enter)
					{
						if (ModelFileNameWithPath[0] == 0)
						{
							MessageBox(nullptr, "���� �ҷ�������.", "�� ������", MB_OK | MB_ICONEXCLAMATION);
						}
						else if (strlen(NewObejct3DName) == 0)
						{
							MessageBox(nullptr, "�̸��� ���ϼ���.", "�̸� ������", MB_OK | MB_ICONEXCLAMATION);
						}
						else
						{
							Game.InsertObject3D(NewObejct3DName);
							CObject3D* Object3D{ Game.GetObject3D(NewObejct3DName) };
							Object3D->CreateFromFile(ModelFileNameWithPath, bIsModelRigged);

							bShowAddObject3D = false;
							memset(ModelFileNameWithPath, 0, MAX_PATH);
							memset(ModelFileNameWithoutPath, 0, MAX_PATH);
							memset(NewObejct3DName, 0, CGame::KObject3DNameMaxLength);
							ImGui::CloseCurrentPopup();
						}
					}
					
					ImGui::SameLine();

					if (ImGui::Button(u8"���"))
					{
						bShowAddObject3D = false;
						memset(ModelFileNameWithPath, 0, MAX_PATH);
						memset(ModelFileNameWithoutPath, 0, MAX_PATH);
						memset(NewObejct3DName, 0, CGame::KObject3DNameMaxLength);
						ImGui::CloseCurrentPopup();
					}

					if (bShowLoadModelDialog)
					{
						static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
						if (FileDialog.OpenFileDialog("FBX ����\0*.fbx\0��� ����\0*.*\0", "�� �ҷ�����"))
						{
							strcpy_s(ModelFileNameWithPath, FileDialog.GetRelativeFileName().c_str());
							strcpy_s(ModelFileNameWithoutPath, FileDialog.GetFileNameWithoutPath().c_str());
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