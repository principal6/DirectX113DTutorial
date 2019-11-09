#include <chrono>
#include "Core/Game.h"
#include "Core/FileDialog.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

using std::min;
using std::vector;
using std::string;
using std::to_string;
using std::chrono::steady_clock;

static ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	static constexpr XMFLOAT2 KGameWindowSize{ 1280.0f, 720.0f };
	CGame Game{ hInstance, KGameWindowSize };

	Game.CreateWin32(WndProc, TEXT("Game Editor"), true);
	Game.CreateSpriteFont(L"Asset\\dotumche_10_korean.spritefont");
	Game.SetMode(CGame::EMode::EditTerrain);

	Game.SetAmbientlLight(XMFLOAT3(1, 1, 1), 0.2f);
	Game.SetDirectionalLight(XMVectorSet(0, 1, 0, 0), XMVectorSet(1, 1, 1, 1));

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
	ImFont* igFont{ igIO.Fonts->AddFontFromFileTTF("Asset/D2Coding.ttf", 15.0f, nullptr, igIO.Fonts->GetGlyphRangesKorean()) };
	
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
			static long long PreviousFrameTime{};
			static int FrameCount{};
			static int CapturedFPS{};
			static float CameraMovementFactor{ 10.0f };

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
					MainCamera->Move(CCamera::EMovementDirection::Forward, DeltaTimeF * CameraMovementFactor);
				}
				if (KeyState.S)
				{
					MainCamera->Move(CCamera::EMovementDirection::Backward, DeltaTimeF * CameraMovementFactor);
				}
				if (KeyState.A)
				{
					MainCamera->Move(CCamera::EMovementDirection::Leftward, DeltaTimeF * CameraMovementFactor);
				}
				if (KeyState.D)
				{
					MainCamera->Move(CCamera::EMovementDirection::Rightward, DeltaTimeF * CameraMovementFactor);
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
						MainCamera->Rotate(MouseState.x - PrevMouseX, MouseState.y - PrevMouseY, DeltaTimeF);
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

			if (Game.GetMode() != CGame::EMode::Play)
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
						static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
						if (FileDialog.OpenFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 불러오기"))
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
							if (FileDialog.SaveFileDialog("지형 파일(*.terr)\0*.terr\0", "지형 파일 내보내기", ".terr"))
							{
								Game.SaveTerrain(FileDialog.GetRelativeFileName());
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
				if (bShowTerrainGenerator) ImGui::OpenPopup(u8"지형 생성기");
				if (ImGui::BeginPopupModal(u8"지형 생성기", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
				{
					static int SizeX{ CTerrain::KDefaultSize };
					static int SizeZ{ CTerrain::KDefaultSize };
					static int MaskingDetail{ CTerrain::KMaskingDefaultDetail };
					static float UniformScaling{ CTerrain::KMinUniformScaling };
					
					ImGui::Text(u8"가로:");
					ImGui::SameLine(120);
					ImGui::SetNextItemWidth(80);
					ImGui::DragInt("##0", &SizeX, 2.0f, 2, 1000);
					if (SizeX % 2) ++SizeX;

					ImGui::Text(u8"세로:");
					ImGui::SameLine(120);
					ImGui::SetNextItemWidth(80);
					ImGui::DragInt("##1", &SizeZ, 2.0f, 2, 1000);
					if (SizeZ % 2) ++SizeZ;

					ImGui::Text(u8"마스킹 디테일:");
					ImGui::SameLine(120);
					ImGui::SetNextItemWidth(80);
					ImGui::SliderInt("##2", &MaskingDetail, CTerrain::KMaskingMinDetail, CTerrain::KMaskingMaxDetail);
					
					ImGui::Text(u8"스케일링:");
					ImGui::SameLine(120);
					ImGui::SetNextItemWidth(80);
					ImGui::DragFloat("##3", &UniformScaling, 0.1f, CTerrain::KMinUniformScaling, CTerrain::KMaxUniformScaling, "%.1f");

					ImGui::Separator();

					if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
					{
						CTerrain::EEditMode eTerrainEditMode{};
						if (Game.GetTerrain()) eTerrainEditMode = Game.GetTerrain()->GetEditMode();

						XMFLOAT2 TerrainSize{ (float)SizeX, (float)SizeZ };
						Game.CreateTerrain(TerrainSize, MaterialDefaultGround0, MaskingDetail, UniformScaling);
						
						CTerrain* const Terrain{ Game.GetTerrain() };
						Game.SetMode(Game.GetMode(), true);
						Terrain->SetEditMode(eTerrainEditMode);

						SizeX = CTerrain::KDefaultSize;
						SizeZ = CTerrain::KDefaultSize;
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
					static constexpr float KInitialWindowWidth{ 400 };
					ImGui::SetNextWindowPos(ImVec2(KGameWindowSize.x - KInitialWindowWidth, 21), ImGuiCond_Appearing);
					ImGui::SetNextWindowSize(ImVec2(KInitialWindowWidth, 0), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(
						ImVec2(KGameWindowSize.x * 0.25f, KGameWindowSize.y), ImVec2(KGameWindowSize.x * 0.5f, KGameWindowSize.y));
					
					if (ImGui::Begin(u8"속성 편집기", &bShowPropertyEditor, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar))
					{
						float WindowWidth{ ImGui::GetWindowWidth() };

						if (ImGui::BeginTabBar(u8"탭바", ImGuiTabBarFlags_None))
						{
							if (ImGui::BeginTabItem(u8"오브젝트"))
							{
								static bool bShowAnimationAdder{ false };
								static bool bShowAnimationEditor{ false };
								static int iSelectedAnimationID{};

								Game.SetMode(CGame::EMode::EditObject);

								static constexpr float KLabelsWidth{ 200 };
								static constexpr float KItemsMaxWidth{ 240 };
								float ItemsWidth{ WindowWidth - KLabelsWidth };
								ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
								float ItemsOffsetX{ WindowWidth - ItemsWidth };

								if (Game.IsAnyObject3DSelected())
								{
									ImGui::PushItemWidth(ItemsWidth);
									{
										CObject3D* const Object3D{ Game.GetSelectedObject3D() };

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"선택된 오브젝트:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"<%s>", Game.GetSelectedObject3DName().c_str());

										ImGui::Separator();

										if (Object3D->IsInstanced())
										{
											int iSelectedInstance{ Game.GetSelectedInstanceID() };
											if (iSelectedInstance == -1)
											{
												ImGui::Text(u8"<인스턴스를 선택해 주세요.>");
											}
											else
											{
												auto& Instance{ Object3D->GetInstance(Game.GetSelectedInstanceID()) };

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"선택된 인스턴스:");
												ImGui::SameLine(ItemsOffsetX);
												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"<%s>", Instance.Name.c_str());

												ImGui::Separator();

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"위치");
												ImGui::SameLine(ItemsOffsetX);
												float Translation[3]{ XMVectorGetX(Instance.Translation),
													XMVectorGetY(Instance.Translation), XMVectorGetZ(Instance.Translation) };
												if (ImGui::DragFloat3(u8"##위치", Translation, CGame::KTranslationUnit,
													CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.2f"))
												{
													Instance.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
													Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
												}

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"회전");
												ImGui::SameLine(ItemsOffsetX);
												int PitchYawRoll360[3]{ (int)(Instance.Pitch * CGame::KRotation2PITo360),
													(int)(Instance.Yaw * CGame::KRotation2PITo360),
													(int)(Instance.Roll * CGame::KRotation2PITo360) };
												if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, CGame::KRotation360Unit,
													CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
												{
													Instance.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
													Instance.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
													Instance.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
													Object3D->UpdateInstanceWorldMatrix(iSelectedInstance);
												}

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"크기");
												ImGui::SameLine(ItemsOffsetX);
												float Scaling[3]{ XMVectorGetX(Instance.Scaling),
													XMVectorGetY(Instance.Scaling), XMVectorGetZ(Instance.Scaling) };
												if (ImGui::DragFloat3(u8"##크기", Scaling, CGame::KScalingUnit,
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
											ImGui::Text(u8"위치");
											ImGui::SameLine(ItemsOffsetX);
											float Translation[3]{ XMVectorGetX(Object3D->ComponentTransform.Translation),
											XMVectorGetY(Object3D->ComponentTransform.Translation), XMVectorGetZ(Object3D->ComponentTransform.Translation) };
											if (ImGui::DragFloat3(u8"##위치", Translation, CGame::KTranslationUnit,
												CGame::KTranslationMinLimit, CGame::KTranslationMaxLimit, "%.2f"))
											{
												Object3D->ComponentTransform.Translation = XMVectorSet(Translation[0], Translation[1], Translation[2], 1.0f);
												Object3D->UpdateWorldMatrix();
											}

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"회전");
											ImGui::SameLine(ItemsOffsetX);
											int PitchYawRoll360[3]{ (int)(Object3D->ComponentTransform.Pitch * CGame::KRotation2PITo360),
												(int)(Object3D->ComponentTransform.Yaw * CGame::KRotation2PITo360),
												(int)(Object3D->ComponentTransform.Roll * CGame::KRotation2PITo360) };
											if (ImGui::DragInt3(u8"##회전", PitchYawRoll360, CGame::KRotation360Unit,
												CGame::KRotation360MinLimit, CGame::KRotation360MaxLimit))
											{
												Object3D->ComponentTransform.Pitch = PitchYawRoll360[0] * CGame::KRotation360To2PI;
												Object3D->ComponentTransform.Yaw = PitchYawRoll360[1] * CGame::KRotation360To2PI;
												Object3D->ComponentTransform.Roll = PitchYawRoll360[2] * CGame::KRotation360To2PI;
												Object3D->UpdateWorldMatrix();
											}

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"크기");
											ImGui::SameLine(ItemsOffsetX);
											float Scaling[3]{ XMVectorGetX(Object3D->ComponentTransform.Scaling),
												XMVectorGetY(Object3D->ComponentTransform.Scaling), XMVectorGetZ(Object3D->ComponentTransform.Scaling) };
											if (ImGui::DragFloat3(u8"##크기", Scaling, CGame::KScalingUnit,
												CGame::KScalingMinLimit, CGame::KScalingMaxLimit, "%.3f"))
											{
												Object3D->ComponentTransform.Scaling = XMVectorSet(Scaling[0], Scaling[1], Scaling[2], 0.0f);
												Object3D->UpdateWorldMatrix();
											}
										}

										ImGui::Separator();

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"BS 중심");
										ImGui::SameLine(ItemsOffsetX);
										float BSCenterOffset[3]{
											XMVectorGetX(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
											XMVectorGetY(Object3D->ComponentPhysics.BoundingSphere.CenterOffset),
											XMVectorGetZ(Object3D->ComponentPhysics.BoundingSphere.CenterOffset) };
										if (ImGui::DragFloat3(u8"##BS중심", BSCenterOffset, CGame::KBSCenterOffsetUnit,
											CGame::KBSCenterOffsetMinLimit, CGame::KBSCenterOffsetMaxLimit, "%.2f"))
										{
											Object3D->ComponentPhysics.BoundingSphere.CenterOffset =
												XMVectorSet(BSCenterOffset[0], BSCenterOffset[1], BSCenterOffset[2], 1.0f);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"BS 반지름");
										ImGui::SameLine(ItemsOffsetX);
										float BSRadius{ Object3D->ComponentPhysics.BoundingSphere.Radius };
										if (ImGui::DragFloat(u8"##BS반지름", &BSRadius, CGame::KBSRadiusUnit,
											CGame::KBSRadiusMinLimit, CGame::KBSRadiusMaxLimit, "%.2f"))
										{
											Object3D->ComponentPhysics.BoundingSphere.Radius = BSRadius;
										}

										// Rigged model
										if (Object3D->IsRiggedModel())
										{
											ImGui::Separator();

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"애니메이션 개수:");
											ImGui::SameLine(ItemsOffsetX);
											int AnimationCount{ Object3D->GetAnimationCount() };
											ImGui::Text(u8"%d", AnimationCount);

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"애니메이션 ID");
											ImGui::SameLine(ItemsOffsetX);
											int AnimationID{ Object3D->GetAnimationID() };
											if (ImGui::SliderInt(u8"##애니메이션 ID", &AnimationID, 0, Object3D->GetAnimationCount() - 1))
											{
												Object3D->SetAnimationID(AnimationID);
											}

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"애니메이션 목록");

											if (ImGui::Button(u8"추가")) bShowAnimationAdder = true;

											ImGui::SameLine();

											if (ImGui::Button(u8"수정")) bShowAnimationEditor = true;

											if (Object3D->CanBakeAnimationTexture())
											{
												ImGui::SameLine();

												if (ImGui::Button(u8"저장"))
												{
													static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
													if (FileDialog.SaveFileDialog("애니메이션 텍스처 파일(*.dds)\0*.dds\0", "애니메이션 텍스처 저장", ".dds"))
													{
														Object3D->BakeAnimationTexture();
														Object3D->SaveBakedAnimationTexture(FileDialog.GetFileName());
													}
												}
											}

											ImGui::SameLine();

											if (ImGui::Button(u8"열기"))
											{
												static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
												if (FileDialog.OpenFileDialog("애니메이션 텍스처 파일(*.dds)\0*.dds\0", "애니메이션 텍스처 불러오기"))
												{
													Object3D->LoadBakedAnimationTexture(FileDialog.GetFileName());
												}
											}

											if (ImGui::ListBoxHeader(u8"##애니메이션 목록", ImVec2(WindowWidth, 0)))
											{
												for (int iAnimation = 0; iAnimation < AnimationCount; ++iAnimation)
												{
													ImGui::PushID(iAnimation);
													if (ImGui::Selectable(Object3D->GetAnimationName(iAnimation).c_str(), (iAnimation == iSelectedAnimationID)))
													{
														iSelectedAnimationID = iAnimation;
													}
													ImGui::PopID();
												}

												ImGui::ListBoxFooter();
											}
										}
									}
									ImGui::PopItemWidth();
								}
								else
								{
									ImGui::Text(u8"<먼저 오브젝트를 선택하세요.>");
								}

								if (bShowAnimationAdder) ImGui::OpenPopup(u8"애니메이션 추가");
								if (ImGui::BeginPopupModal(u8"애니메이션 추가", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"애니메이션 이름");
									ImGui::SameLine(150);
									static char Name[16]{};
									ImGui::InputText(u8"##애니메이션 이름", Name, 16);

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"파일 이름");
									ImGui::SameLine(150);
									static char FileName[MAX_PATH]{};
									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"%s", FileName);

									if (ImGui::Button(u8"불러오기"))
									{
										static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
										if (FileDialog.OpenFileDialog("모델 파일(*.fbx)\0*.fbx\0", "애니메이션 불러오기"))
										{
											strcpy_s(FileName, FileDialog.GetRelativeFileName().c_str());
										}
									}

									ImGui::SameLine();

									if (ImGui::Button(u8"결정"))
									{
										if (FileName[0] == '\0')
										{
											MB_WARN("파일을 먼저 불러오세요.", "오류");
										}
										else
										{
											CObject3D* const Object3D{ Game.GetSelectedObject3D() };
											Object3D->AddAnimationFromFile(FileName, Name);

											bShowAnimationAdder = false;
											ImGui::CloseCurrentPopup();
										}
									}

									ImGui::SameLine();

									if (ImGui::Button(u8"취소"))
									{
										bShowAnimationAdder = false;
										ImGui::CloseCurrentPopup();
									}

									ImGui::EndPopup();
								}

								if (bShowAnimationEditor) ImGui::OpenPopup(u8"애니메이션 수정");
								if (ImGui::BeginPopupModal(u8"애니메이션 수정", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
								{
									static bool bFirstTime{ true };
									CObject3D* const Object3D{ Game.GetSelectedObject3D() };

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"애니메이션 이름");
									ImGui::SameLine(150);
									static char AnimationName[CObject3D::KMaxAnimationNameLength]{};
									if (bFirstTime)
									{
										strcpy_s(AnimationName, Object3D->GetAnimationName(iSelectedAnimationID).c_str());
										bFirstTime = false;
									}
									ImGui::InputText(u8"##애니메이션 이름", AnimationName, CObject3D::KMaxAnimationNameLength);

									if (ImGui::Button(u8"결정"))
									{
										CObject3D* const Object3D{ Game.GetSelectedObject3D() };
										Object3D->SetAnimationName(iSelectedAnimationID, AnimationName);

										bFirstTime = true;
										bShowAnimationEditor = false;
										ImGui::CloseCurrentPopup();
									}

									ImGui::SameLine();

									if (ImGui::Button(u8"취소"))
									{
										bFirstTime = true;
										bShowAnimationEditor = false;
										ImGui::CloseCurrentPopup();
									}

									ImGui::EndPopup();
								}
								ImGui::EndTabItem();
							}

							// 지형 편집기
							static bool bShowFoliageClusterGenerator{ false };
							if (ImGui::BeginTabItem(u8"지형"))
							{
								static constexpr float KLabelsWidth{ 200 };
								static constexpr float KItemsMaxWidth{ 240 };
								float ItemsWidth{ WindowWidth - KLabelsWidth };
								ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
								float ItemsOffsetX{ WindowWidth - ItemsWidth };

								Game.SetMode(CGame::EMode::EditTerrain);

								CTerrain* const Terrain{ Game.GetTerrain() };
								if (Terrain)
								{
									static const char* const KModeLabelList[4]{ 
										u8"<높이 지정 모드>", u8"<높이 변경 모드>", u8"<마스킹 모드>", u8"<초목 배치 모드>" };
									static int iSelectedMode{};

									const XMFLOAT2& KTerrainSize{ Terrain->GetSize() };
									
									ImGui::PushItemWidth(ItemsWidth);
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"가로 x 세로:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"%d x %d", (int)KTerrainSize.x, (int)KTerrainSize.y);

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"마스킹 디테일:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"%d", Game.GetTerrain()->GetMaskingDetail());

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"지형 테셀레이션 계수");
										ImGui::SameLine(ItemsOffsetX);
										float TerrainTessFactor{ Terrain->GetTerrainTessFactor() };
										if (ImGui::SliderFloat(u8"##지형 테셀레이션 계수", &TerrainTessFactor,
											CTerrain::KTessFactorMin, CTerrain::KTessFactorMax, "%.1f"))
										{
											Terrain->SetTerrainTessFactor(TerrainTessFactor);
										}

										ImGui::Separator();

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"물 그리기");
										ImGui::SameLine(ItemsOffsetX);
										bool bDrawWater{ Terrain->ShouldDrawWater() };
										if (ImGui::Checkbox(u8"##물 그리기", &bDrawWater))
										{
											Terrain->ShouldDrawWater(bDrawWater);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"물 높이");
										ImGui::SameLine(ItemsOffsetX);
										float WaterHeight{ Terrain->GetWaterHeight() };
										if (ImGui::DragFloat(u8"##물 높이", &WaterHeight, CTerrain::KWaterHeightUnit,
											CTerrain::KWaterMinHeight, CTerrain::KWaterMaxHeight, "%.1f"))
										{
											Terrain->SetWaterHeight(WaterHeight);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"물 테셀레이션 계수");
										ImGui::SameLine(ItemsOffsetX);
										float WaterTessFactor{ Terrain->GetWaterTessFactor() };
										if (ImGui::SliderFloat(u8"##물 테셀레이션 계수", &WaterTessFactor,
											CTerrain::KTessFactorMin, CTerrain::KTessFactorMax, "%.1f"))
										{
											Terrain->SetWaterTessFactor(WaterTessFactor);
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
											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"지정할 높이");
											ImGui::SameLine(ItemsOffsetX);
											float TerrainSetHeightValue{ Terrain->GetSetHeightValue() };
											if (ImGui::DragFloat(u8"##지정할 높이", &TerrainSetHeightValue, CTerrain::KHeightUnit,
												CTerrain::KMinHeight, CTerrain::KMaxHeight, "%.1f"))
											{
												Terrain->SetSetHeightValue(TerrainSetHeightValue);
											}
										}
										else if (iSelectedMode == 2)
										{
											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"마스킹 레이어");
											ImGui::SameLine(ItemsOffsetX);
											int TerrainMaskingLayer{ (int)Terrain->GetMaskingLayer() };
											if (ImGui::SliderInt(u8"##마스킹 레이어", &TerrainMaskingLayer, 0, CTerrain::KMaterialMaxCount - 2))
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

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"마스킹 배합 비율");
											ImGui::SameLine(ItemsOffsetX);
											float TerrainMaskingRatio{ Terrain->GetMaskingRatio() };
											if (ImGui::SliderFloat(u8"##마스킹 배합 비율", &TerrainMaskingRatio,
												CTerrain::KMaskingMinRatio, CTerrain::KMaskingMaxRatio, "%.3f"))
											{
												Terrain->SetMaskingRatio(TerrainMaskingRatio);
											}

											ImGui::AlignTextToFramePadding();
											ImGui::Text(u8"마스킹 감쇠");
											ImGui::SameLine(ItemsOffsetX);
											float TerrainMaskingAttenuation{ Terrain->GetMaskingAttenuation() };
											if (ImGui::SliderFloat(u8"##마스킹 감쇠", &TerrainMaskingAttenuation,
												CTerrain::KMaskingMinAttenuation, CTerrain::KMaskingMaxAttenuation, "%.3f"))
											{
												Terrain->SetMaskingAttenuation(TerrainMaskingAttenuation);
											}
										}
										else if (iSelectedMode == 3)
										{
											if (Terrain->HasFoliageCluster())
											{
												ImGui::SetCursorPosX(ItemsOffsetX);
												if (ImGui::Button(u8"초목 클러스터 재생성", ImVec2(ItemsWidth, 0)))
												{
													bShowFoliageClusterGenerator = true;
												}

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"초목 배치 디테일");
												ImGui::SameLine(ItemsOffsetX);
												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"%d", Terrain->GetFoliagePlacingDetail());

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"초목 밀도");
												ImGui::SameLine(ItemsOffsetX);
												float FoliageDenstiy{ Terrain->GetFoliageDenstiy() };
												if (ImGui::SliderFloat(u8"##초목밀도", &FoliageDenstiy, 0.0f, 1.0f, "%.2f"))
												{
													Terrain->SetFoliageDensity(FoliageDenstiy);
												}

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"바람 속도");
												ImGui::SameLine(ItemsOffsetX);
												XMFLOAT3 WindVelocity{}; XMStoreFloat3(&WindVelocity, Terrain->GetWindVelocity());
												if (ImGui::SliderFloat3(u8"##바람속도", &WindVelocity.x,
													CTerrain::KMinWindVelocityElement, CTerrain::KMaxWindVelocityElement, "%.2f"))
												{
													Terrain->SetWindVelocity(WindVelocity);
												}

												ImGui::AlignTextToFramePadding();
												ImGui::Text(u8"바람 반지름");
												ImGui::SameLine(ItemsOffsetX);
												float WindRadius{ Terrain->GetWindRadius() };
												if (ImGui::SliderFloat(u8"##바람반지름", &WindRadius,
													CTerrain::KMinWindRadius, CTerrain::KMaxWindRadius, "%.2f"))
												{
													Terrain->SetWindRadius(WindRadius);
												}
											}
											else
											{
												ImGui::SetCursorPosX(ItemsOffsetX);
												if (ImGui::Button(u8"초목 클러스터 생성", ImVec2(ItemsWidth, 0)))
												{
													bShowFoliageClusterGenerator = true;
												}
											}	
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"선택 반지름");
										ImGui::SameLine(ItemsOffsetX);
										float TerrainMaskingRadius{ Terrain->GetSelectionRadius() };
										if (ImGui::SliderFloat(u8"##선택 반지름", &TerrainMaskingRadius, 
											CTerrain::KMinSelectionRadius, CTerrain::KMaxSelectionRadius, "%.1f"))
										{
											Terrain->SetSelectionRadius(TerrainMaskingRadius);
										}
										if (MouseState.scrollWheelValue)
										{
											if (MouseState.scrollWheelValue > 0) TerrainMaskingRadius += CTerrain::KSelectionRadiusUnit;
											if (MouseState.scrollWheelValue < 0) TerrainMaskingRadius -= CTerrain::KSelectionRadiusUnit;
											Terrain->SetSelectionRadius(TerrainMaskingRadius);
										}

										ImGui::Separator();

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"현재 선택 위치:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::Text(u8"(%.2f, %.2f)", Terrain->GetSelectionPosition().x, Terrain->GetSelectionPosition().y);

										ImGui::Separator();
									}
									ImGui::PopItemWidth();

									static bool bShowMaterialSelection{ false };
									static int iSelectedMaterialID{};
									if (Terrain)
									{
										if (ImGui::Button(u8"재질 추가"))
										{
											iSelectedMaterialID = -1;
											ImGui::OpenPopup(u8"재질 선택");
										}

										for (int iMaterial = 0; iMaterial < Terrain->GetMaterialCount(); ++iMaterial)
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

											const CMaterial& Material{ Terrain->GetMaterial(iMaterial) };
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

							// ### 초목 클러스터 생성기 윈도우 ###
							if (bShowFoliageClusterGenerator) ImGui::OpenPopup(u8"초목 생성기");
							if (ImGui::BeginPopupModal(u8"초목 생성기", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
							{
								static const vector<string> KDefaultFoliages{
									{ "Asset\\basic_grass0.fbx" }, { "Asset\\basic_grass2.fbx" }, { "Asset\\basic_grass3.fbx" } 
								};
								static vector<string> vFoliageFileNames{};
								static uint32_t PlacingDetail{ CTerrain::KDefaultFoliagePlacingDetail };
								static bool bUseDefaultFoliages{ false };

								const float KItemsWidth{ 240 };
								ImGui::PushItemWidth(KItemsWidth);
								{
									if (!bUseDefaultFoliages)
									{
										if (ImGui::Button(u8"추가"))
										{
											static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
											if (FileDialog.OpenFileDialog("모델 파일(*.fbx)\0*.fbx\0", "초목 오브젝트 불러오기"))
											{
												vFoliageFileNames.emplace_back(FileDialog.GetRelativeFileName());
											}
										}
									}

									ImGui::AlignTextToFramePadding();
									ImGui::Text(u8"배치 디테일");
									ImGui::SameLine(100);
									ImGui::SetNextItemWidth(KItemsWidth - 100);
									ImGui::SliderInt(u8"##배치 디테일", (int*)&PlacingDetail, 
										CTerrain::KMinFoliagePlacingDetail, CTerrain::KMaxFoliagePlacingDetail);

									ImGui::Separator();

									if (ImGui::ListBoxHeader(u8"##lb"))
									{
										if (bUseDefaultFoliages)
										{
											for (const auto& FoliageFileName : KDefaultFoliages)
											{
												ImGui::Text(FoliageFileName.data());
											}
										}
										else
										{
											for (const auto& FoliageFileName : vFoliageFileNames)
											{
												ImGui::Text(FoliageFileName.data());
											}
										}

										ImGui::ListBoxFooter();
									}

									ImGui::Separator();

									if (ImGui::Button(u8"결정") || ImGui::IsKeyDown(VK_RETURN))
									{
										CTerrain* const Terrain{ Game.GetTerrain() };

										if (bUseDefaultFoliages)
										{
											Terrain->CreateFoliageCluster(KDefaultFoliages, PlacingDetail);
										}
										else
										{
											Terrain->CreateFoliageCluster(vFoliageFileNames, PlacingDetail);
										}

										vFoliageFileNames.clear();
										bShowFoliageClusterGenerator = false;
										ImGui::CloseCurrentPopup();
									}

									ImGui::SameLine();

									if (ImGui::Button(u8"닫기") || ImGui::IsKeyDown(VK_ESCAPE))
									{
										vFoliageFileNames.clear();
										bShowFoliageClusterGenerator = false;
										ImGui::CloseCurrentPopup();
									}

									ImGui::SameLine();

									ImGui::Checkbox(u8"기본값으로 생성", &bUseDefaultFoliages);
								}
								ImGui::PopItemWidth();

								ImGui::EndPopup();
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
								static const char KLabelAdd[]{ u8"추가" };
								static const char KLabelChange[]{ u8"변경" };

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
										XMFLOAT3 AmbientColor{ Material->GetAmbientColor() };
										if (ImGui::ColorEdit3(u8"환경광(Ambient)", &AmbientColor.x, ImGuiColorEditFlags_RGB))
										{
											Material->SetAmbientColor(AmbientColor);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										XMFLOAT3 DiffuseColor{ Material->GetDiffuseColor() };
										if (ImGui::ColorEdit3(u8"난반사광(Diffuse)", &DiffuseColor.x, ImGuiColorEditFlags_RGB))
										{
											Material->SetDiffuseColor(DiffuseColor);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										XMFLOAT3 SpecularColor{ Material->GetSpecularColor() };
										if (ImGui::ColorEdit3(u8"정반사광(Specular)", &SpecularColor.x, ImGuiColorEditFlags_RGB))
										{
											Material->SetSpecularColor(SpecularColor);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										float SpecularExponent{ Material->GetSpecularExponent() };
										if (ImGui::DragFloat(u8"정반사광(Specular) 지수", &SpecularExponent, 0.001f, 0.0f, 1.0f, "%.3f"))
										{
											Material->SetSpecularExponent(SpecularExponent);
										}

										ImGui::SetNextItemWidth(KUniformWidth);
										float SpecularIntensity{ Material->GetSpecularIntensity() };
										if (ImGui::DragFloat(u8"정반사광(Specular) 강도", &SpecularIntensity, 0.001f, 0.0f, 1.0f, "%.3f"))
										{
											Material->SetSpecularIntensity(SpecularIntensity);
										}

										static const char KTextureDialogFilter[]{ "PNG 파일\0*.png\0JPG 파일\0*.jpg\0모든 파일\0*.*\0" };
										static const char KTextureDialogTitle[]{ "텍스쳐 불러오기" };

										// Diffuse texture
										{
											const char* PtrDiffuseTextureLabel{ KLabelAdd };
											if (Material->HasTexture(CMaterial::CTexture::EType::DiffuseTexture))
											{
												ImGui::Image(Game.GetMaterialTexture(CMaterial::CTexture::EType::DiffuseTexture,
													pairMaterial.first)->GetShaderResourceViewPtr(), ImVec2(50, 50));
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
												static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
												if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, FileDialog.GetRelativeFileName());
													Game.ReloadMaterial(pairMaterial.first);
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
												static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
												if (FileDialog.OpenFileDialog(KTextureDialogFilter, KTextureDialogTitle))
												{
													Material->SetTextureFileName(CMaterial::CTexture::EType::NormalTexture, FileDialog.GetRelativeFileName());
													Game.ReloadMaterial(pairMaterial.first);
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
												ImGui::Text(u8"Displacement 텍스쳐");

												PtrDisplacementTextureLabel = KLabelChange;
											}
											else
											{
												ImGui::Image(0, ImVec2(100, 100));
												ImGui::SameLine();
												ImGui::SetNextItemWidth(KUniformWidth);
												ImGui::Text(u8"Displacement 텍스쳐: 없음");

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
													Game.ReloadMaterial(pairMaterial.first);
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
												MB_WARN("이미 존재하는 이름입니다. 다른 이름을 골라 주세요.", "오류");
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

							if (ImGui::BeginTabItem(u8"기타"))
							{
								const XMVECTOR& KDirectionalLightDirection{ Game.GetDirectionalLightDirection() };
								float DirectionalLightDirection[3]{ XMVectorGetX(KDirectionalLightDirection), XMVectorGetY(KDirectionalLightDirection),
									XMVectorGetZ(KDirectionalLightDirection) };

								const XMVECTOR& KEyePosition{ MainCamera->GetEyePosition() };
								float EyePosition[3]{ XMVectorGetX(KEyePosition), XMVectorGetY(KEyePosition), XMVectorGetZ(KEyePosition) };
								float Pitch{ MainCamera->GetPitch() };
								float Yaw{ MainCamera->GetYaw() };

								static constexpr float KLabelsWidth{ 200 };
								static constexpr float KItemsMaxWidth{ 240 };
								float ItemsWidth{ WindowWidth - KLabelsWidth };
								ItemsWidth = min(ItemsWidth, KItemsMaxWidth);
								float ItemsOffsetX{ WindowWidth - ItemsWidth };
								ImGui::PushItemWidth(ItemsWidth);
								{
									if (ImGui::TreeNodeEx(u8"조명", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Directional Light 위치");
										ImGui::SameLine(ItemsOffsetX);
										if (ImGui::DragFloat3(u8"##Directional Light 위치", DirectionalLightDirection, 0.02f, -1.0f, +1.0f, "%.2f"))
										{
											Game.SetDirectionalLight(XMVectorSet(DirectionalLightDirection[0], DirectionalLightDirection[1],
												DirectionalLightDirection[2], 0.0f));
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Ambient Light 색상");
										ImGui::SameLine(ItemsOffsetX);
										XMFLOAT3 AmbientLightColor{ Game.GetAmbientLightColor() };
										if (ImGui::DragFloat3(u8"##Ambient Light 색상", &AmbientLightColor.x, 0.02f, 0.0f, +1.0f, "%.2f"))
										{
											Game.SetAmbientlLight(AmbientLightColor, Game.GetAmbientLightIntensity());
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Ambient Light 강도");
										ImGui::SameLine(ItemsOffsetX);
										float AmbientLightIntensity{ Game.GetAmbientLightIntensity() };
										if (ImGui::DragFloat(u8"##Ambient Light 강도", &AmbientLightIntensity, 0.02f, 0.0f, +1.0f, "%.2f"))
										{
											Game.SetAmbientlLight(Game.GetAmbientLightColor(), AmbientLightIntensity);
										}

										ImGui::TreePop();
									}
									
									ImGui::Separator();
									
									if (ImGui::TreeNodeEx(u8"카메라", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"위치");
										ImGui::SameLine(ItemsOffsetX);
										if (ImGui::DragFloat3(u8"##위치", EyePosition, 0.01f, -10000.0f, +10000.0f, "%.3f"))
										{
											MainCamera->SetEyePosition(XMVectorSet(EyePosition[0], EyePosition[1], EyePosition[2], 1.0f));
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"회전 Pitch");
										ImGui::SameLine(ItemsOffsetX);
										if (ImGui::DragFloat(u8"##회전 Pitch", &Pitch, 0.01f, -10000.0f, +10000.0f, "%.3f"))
										{
											MainCamera->SetPitch(Pitch);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"회전 Yaw");
										ImGui::SameLine(ItemsOffsetX);
										if (ImGui::DragFloat(u8"##회전 Yaw", &Yaw, 0.01f, -10000.0f, +10000.0f, "%.3f"))
										{
											MainCamera->SetYaw(Yaw);
										}

										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"카메라 이동 속도");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::SliderFloat(u8"##카메라 이동 속도", &CameraMovementFactor, 1.0f, 100.0f, "%.0f");

										ImGui::TreePop();
									}

									if (ImGui::TreeNodeEx(u8"FPS", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen))
									{
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"Frames per second:");
										ImGui::SameLine(ItemsOffsetX);
										ImGui::AlignTextToFramePadding();
										ImGui::Text(u8"%d", CapturedFPS);

										ImGui::TreePop();
									}
								}
								ImGui::PopItemWidth();

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
					const auto& mapObject3D{ Game.GetObject3DMap() };

					ImGui::SetNextWindowPos(ImVec2(0, 122), ImGuiCond_Appearing);
					ImGui::SetNextWindowSizeConstraints(ImVec2(300, 60), ImVec2(400, 300));
					if (ImGui::Begin(u8"장면 편집기", &bShowSceneEditor, ImGuiWindowFlags_AlwaysAutoResize))
					{
						// 장면 내보내기
						if (ImGui::Button(u8"장면 내보내기"))
						{
							static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
							if (FileDialog.SaveFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 내보내기", ".scene"))
							{
								Game.SaveScene(FileDialog.GetRelativeFileName());
							}
							
						}

						ImGui::SameLine();

						// 장면 불러오기
						if (ImGui::Button(u8"장면 불러오기"))
						{
							static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
							if (FileDialog.OpenFileDialog("장면 파일(*.scene)\0*.scene\0", "장면 불러오기"))
							{
								Game.LoadScene(FileDialog.GetRelativeFileName());
							}
						}

						ImGui::Separator();

						// 오브젝트 추가
						if (ImGui::Button(u8"오브젝트 추가"))
						{
							bShowAddObject3D = true;
						}

						ImGui::SameLine();

						// 오브젝트 제거
						if (ImGui::Button(u8"오브젝트 제거"))
						{
							Game.DeleteObject3D(Game.GetSelectedObject3DName());
						}

						ImGui::Separator();

						ImGui::Columns(2);
						ImGui::Text(u8"오브젝트 및 인스턴스"); ImGui::NextColumn();
						ImGui::Text(u8"인스턴스 관리"); ImGui::NextColumn();
						ImGui::Separator();

						// 오브젝트 목록
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
								// 인스턴스 추가
								ImGui::PushID(iObject3DPair * 2 + 0);
								if (ImGui::Button(u8"추가"))
								{
									Object3D->InsertInstance();
								}
								ImGui::PopID();

								// 인스턴스 제거
								if (Object3D->IsInstanced() && Game.IsAnyInstanceSelected())
								{
									ImGui::SameLine();

									ImGui::PushID(iObject3DPair * 2 + 1);
									if (ImGui::Button(u8"제거"))
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
								// 인스턴스 목록
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
					ImGui::InputText(u8"오브젝트 이름", NewObejct3DName, CGame::KObject3DNameMaxLength);

					if (ImGui::Button(u8"모델 불러오기"))
					{
						bShowLoadModelDialog = true;
					}
					ImGui::SameLine();

					ImGui::Text(ModelFileNameWithoutPath);

					ImGui::SameLine();
					
					static bool bIsModelRigged{ false };
					ImGui::Checkbox(u8"리깅 여부", &bIsModelRigged);
					
					if (ImGui::Button(u8"결정") || KeyState.Enter)
					{
						if (ModelFileNameWithPath[0] == 0)
						{
							MB_WARN("모델을 불러오세요.", "오류");
						}
						else if (strlen(NewObejct3DName) == 0)
						{
							MB_WARN("이름을 정하세요.", "오류");
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
						static CFileDialog FileDialog{ Game.GetWorkingDirectory() };
						if (FileDialog.OpenFileDialog("FBX 파일\0*.fbx\0모든 파일\0*.*\0", "모델 불러오기"))
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

			if (TimeNow > PreviousFrameTime + 1'000'000'000)
			{
				CapturedFPS = FrameCount;
				FrameCount = 0;
				PreviousFrameTime = TimeNow;
			}

			KeyDown = 0;
			TimePrev = TimeNow;
			++FrameCount;
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