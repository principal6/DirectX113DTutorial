#include "Game.h"
#include <thread>

static constexpr D3D11_INPUT_ELEMENT_DESC KBaseInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT	, 1,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 1, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "INSTANCEWORLD"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCEWORLD"	, 1, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCEWORLD"	, 2, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "INSTANCEWORLD"	, 3, DXGI_FORMAT_R32G32B32A32_FLOAT	, 2, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

static constexpr D3D11_INPUT_ELEMENT_DESC KVSLineInputElementDescs[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static constexpr D3D11_INPUT_ELEMENT_DESC KVS2DBaseInputLayout[]
{
	{ "POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD"	, 0, DXGI_FORMAT_R32G32_FLOAT		, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static constexpr D3D11_INPUT_ELEMENT_DESC KParticleInputElementDescs[]
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "ROTATION", 0, DXGI_FORMAT_R32_FLOAT			, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "SCALING"	, 0, DXGI_FORMAT_R32G32_FLOAT		, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void CGame::CreateWin32(WNDPROC const WndProc, LPCTSTR const WindowName, const wstring& FontFileName, bool bWindowed)
{
	CreateWin32Window(WndProc, WindowName);

	InitializeDirectX(FontFileName, bWindowed);

	GetCurrentDirectoryA(MAX_PATH, m_WorkingDirectory);
}

void CGame::Destroy()
{
	DestroyWindow(m_hWnd);
}

void CGame::LoadScene(const string& FileName)
{
	using namespace tinyxml2;

	tinyxml2::XMLDocument xmlDocument{};
	xmlDocument.LoadFile(FileName.c_str());

	XMLElement* xmlScene{ xmlDocument.FirstChildElement() };
	vector<XMLElement*> vxmlSceneChildren{};
	XMLElement* xmlSceneChild{ xmlScene->FirstChildElement() };
	while (xmlSceneChild)
	{
		vxmlSceneChildren.emplace_back(xmlSceneChild);
		xmlSceneChild = xmlSceneChild->NextSiblingElement();
	}

	for (const auto& xmlSceneChild : vxmlSceneChildren)
	{
		const char* ID{ xmlSceneChild->Attribute("ID") };

		if (strcmp(ID, "ObjectList") == 0)
		{
			vector<XMLElement*> vxmlObjects{};
			XMLElement* xmlObject{ xmlSceneChild->FirstChildElement() };
			while (xmlObject)
			{
				vxmlObjects.emplace_back(xmlObject);
				xmlObject = xmlObject->NextSiblingElement();
			}

			ClearObject3Ds();
			m_vObject3Ds.reserve(vxmlObjects.size());
			vector<CObject3D*> vPtrObject3D{};
			vector<std::pair<string, bool>> vModelDatas{};

			for (const auto& xmlObject : vxmlObjects)
			{
				const char* ObjectName{ xmlObject->Attribute("Name") };
				
				InsertObject3D(ObjectName);
				vPtrObject3D.emplace_back(GetObject3D(ObjectName));
				CObject3D* Object3D{ GetObject3D(ObjectName) };
				
				vector<XMLElement*> vxmlObjectChildren{};
				XMLElement* xmlObjectChild{ xmlObject->FirstChildElement() };
				while (xmlObjectChild)
				{
					vxmlObjectChildren.emplace_back(xmlObjectChild);
					xmlObjectChild = xmlObjectChild->NextSiblingElement();
				}

				for (const auto& xmlObjectChild : vxmlObjectChildren)
				{
					const char* ID{ xmlObjectChild->Attribute("ID") };

					if (strcmp(ID, "Model") == 0)
					{
						const char* ModelFileName{ xmlObjectChild->Attribute("FileName") };
						bool bIsRigged{ xmlObjectChild->BoolAttribute("IsRigged") };
						vModelDatas.emplace_back(std::make_pair(ModelFileName, bIsRigged));
					}

					if (strcmp(ID, "Translation") == 0)
					{
						float x{ xmlObjectChild->FloatAttribute("x") };
						float y{ xmlObjectChild->FloatAttribute("y") };
						float z{ xmlObjectChild->FloatAttribute("z") };
						Object3D->ComponentTransform.Translation = XMVectorSet(x, y, z, 1.0f);
					}

					if (strcmp(ID, "Rotation") == 0)
					{
						float Pitch{ xmlObjectChild->FloatAttribute("Pitch") };
						float Yaw{ xmlObjectChild->FloatAttribute("Yaw") };
						float Roll{ xmlObjectChild->FloatAttribute("Roll") };
						Object3D->ComponentTransform.Pitch = Pitch;
						Object3D->ComponentTransform.Yaw = Yaw;
						Object3D->ComponentTransform.Roll = Roll;
					}

					if (strcmp(ID, "Scaling") == 0)
					{
						float x{ xmlObjectChild->FloatAttribute("x") };
						float y{ xmlObjectChild->FloatAttribute("y") };
						float z{ xmlObjectChild->FloatAttribute("z") };
						Object3D->ComponentTransform.Scaling = XMVectorSet(x, y, z, 1.0f);
					}

					if (strcmp(ID, "BSCenterOffset") == 0)
					{
						float x{ xmlObjectChild->FloatAttribute("x") };
						float y{ xmlObjectChild->FloatAttribute("y") };
						float z{ xmlObjectChild->FloatAttribute("z") };
						Object3D->ComponentPhysics.BoundingSphere.CenterOffset = XMVectorSet(x, y, z, 1.0f);
					}

					if (strcmp(ID, "BSRadius") == 0)
					{
						float Radius{ xmlObjectChild->FloatAttribute("Value") };
						Object3D->ComponentPhysics.BoundingSphere.Radius = Radius;
					}
				}
			}

			// Create models using multiple threads
			{
				ULONGLONG StartTimePoint{ GetTickCount64() };
				OutputDebugString((to_string(StartTimePoint) + " Loading models.\n").c_str());
				vector<std::thread> vThreads{};
				for (size_t iObject3D = 0; iObject3D < vPtrObject3D.size(); ++iObject3D)
				{
					vThreads.emplace_back
					(
						[](CObject3D* const P, const string& ModelFileName, bool bIsRigged)
						{
							P->CreateFromFile(ModelFileName, bIsRigged);
						}
						, vPtrObject3D[iObject3D], vModelDatas[iObject3D].first, vModelDatas[iObject3D].second
							);
				}
				for (auto& Thread : vThreads)
				{
					Thread.join();
				}
				OutputDebugString((to_string(GetTickCount64()) + " All models are loaded. [" 
					+ to_string(GetTickCount64() - StartTimePoint) + "] elapsed.\n").c_str());
			}
		}

		if (strcmp(ID, "Terrain") == 0)
		{
			const char* TerrainFileName{ xmlSceneChild->Attribute("FileName") };
			if (TerrainFileName) LoadTerrain(TerrainFileName);
		}

		if (strcmp(ID, "Sky") == 0)
		{
			const char* SkyFileName{ xmlSceneChild->Attribute("FileName") };
			float ScalingFactor{ xmlSceneChild->FloatAttribute("ScalingFactor") };
			if (SkyFileName) SetSky(SkyFileName, ScalingFactor);
		}
	}
}

void CGame::SaveScene(const string& FileName)
{
	using namespace tinyxml2;

	if (m_Terrain)
	{
		if (m_Terrain->GetFileName().size() == 0)
		{
			MessageBox(nullptr, "지형이 존재하지만 저장되지 않았습니다.\n먼저 지형을 저장해 주세요.", "지형 저장", MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		else
		{
			m_Terrain->Save(m_Terrain->GetFileName());
		}
	}

	tinyxml2::XMLDocument xmlDocument{};
	XMLElement* xmlRoot{ xmlDocument.NewElement("Scene") };
	{
		XMLElement* xmlObjectList{ xmlDocument.NewElement("ObjectList") };
		xmlObjectList->SetAttribute("ID", "ObjectList");
		{
			for (const auto& Object3D : m_vObject3Ds)
			{
				XMLElement* xmlObject{ xmlDocument.NewElement("Object") };
				xmlObject->SetAttribute("Name", Object3D->GetName().c_str());

				XMLElement* xmlModelFileName{ xmlDocument.NewElement("Model") };
				{
					xmlModelFileName->SetAttribute("ID", "Model");
					xmlModelFileName->SetAttribute("FileName", Object3D->GetModelFileName().c_str());
					xmlModelFileName->SetAttribute("IsRigged", Object3D->IsRiggedModel());
					xmlObject->InsertEndChild(xmlModelFileName);
				}
				
				XMLElement* xmlTranslation{ xmlDocument.NewElement("Translation") };
				{
					xmlTranslation->SetAttribute("ID", "Translation");
					XMFLOAT4 Translation{};
					XMStoreFloat4(&Translation, Object3D->ComponentTransform.Translation);
					xmlTranslation->SetAttribute("x", Translation.x);
					xmlTranslation->SetAttribute("y", Translation.y);
					xmlTranslation->SetAttribute("z", Translation.z);
					xmlObject->InsertEndChild(xmlTranslation);
				}
				
				XMLElement* xmlRotation{ xmlDocument.NewElement("Rotation") };
				{
					xmlRotation->SetAttribute("ID", "Rotation");
					xmlRotation->SetAttribute("Pitch", Object3D->ComponentTransform.Pitch);
					xmlRotation->SetAttribute("Yaw", Object3D->ComponentTransform.Yaw);
					xmlRotation->SetAttribute("Roll", Object3D->ComponentTransform.Roll);
					xmlObject->InsertEndChild(xmlRotation);
				}

				XMLElement* xmlScaling{ xmlDocument.NewElement("Scaling") };
				{
					xmlScaling->SetAttribute("ID", "Scaling");
					XMFLOAT4 Scaling{};
					XMStoreFloat4(&Scaling, Object3D->ComponentTransform.Scaling);
					xmlScaling->SetAttribute("x", Scaling.x);
					xmlScaling->SetAttribute("y", Scaling.y);
					xmlScaling->SetAttribute("z", Scaling.z);
					xmlObject->InsertEndChild(xmlScaling);
				}

				XMLElement* xmlBSCenterOffset{ xmlDocument.NewElement("BSCenterOffset") };
				{
					xmlBSCenterOffset->SetAttribute("ID", "BSCenterOffset");
					XMFLOAT4 BSCenterOffset{};
					XMStoreFloat4(&BSCenterOffset, Object3D->ComponentPhysics.BoundingSphere.CenterOffset);
					xmlBSCenterOffset->SetAttribute("x", BSCenterOffset.x);
					xmlBSCenterOffset->SetAttribute("y", BSCenterOffset.y);
					xmlBSCenterOffset->SetAttribute("z", BSCenterOffset.z);
					xmlObject->InsertEndChild(xmlBSCenterOffset);
				}

				XMLElement* xmlBSRadius{ xmlDocument.NewElement("BSRadius") };
				{
					xmlBSRadius->SetAttribute("ID", "BSRadius");
					xmlBSRadius->SetAttribute("Value", Object3D->ComponentPhysics.BoundingSphere.Radius);
					xmlObject->InsertEndChild(xmlBSRadius);
				}

				xmlObjectList->InsertEndChild(xmlObject);
			}
			xmlRoot->InsertEndChild(xmlObjectList);
		}

		XMLElement* xmlTerrain{ xmlDocument.NewElement("Terrain") };
		xmlTerrain->SetAttribute("ID", "Terrain");
		{
			if (m_Terrain)
			{
				xmlTerrain->SetAttribute("FileName", m_Terrain->GetFileName().c_str());
			}

			xmlRoot->InsertEndChild(xmlTerrain);
		}

		XMLElement* xmlSky{ xmlDocument.NewElement("Sky") };
		xmlSky->SetAttribute("ID", "Sky");
		{
			if (m_SkyData.bIsDataSet)
			{
				xmlSky->SetAttribute("FileName", m_SkyFileName.c_str());
				xmlSky->SetAttribute("ScalingFactor", m_SkyScalingFactor);
			}

			xmlRoot->InsertEndChild(xmlSky);
		}
	}
	xmlDocument.InsertEndChild(xmlRoot);

	xmlDocument.SaveFile(FileName.c_str());
}

void CGame::SetPerspective(float FOV, float NearZ, float FarZ)
{
	m_NearZ = NearZ;
	m_FarZ = FarZ;

	m_MatrixProjection = XMMatrixPerspectiveFovLH(FOV, m_WindowSize.x / m_WindowSize.y, m_NearZ, m_FarZ);
}

void CGame::SetGameRenderingFlags(EFlagsRendering Flags)
{
	m_eFlagsRendering = Flags;
}

void CGame::ToggleGameRenderingFlags(EFlagsRendering Flags)
{
	m_eFlagsRendering ^= Flags;
}

void CGame::Set3DGizmoMode(E3DGizmoMode Mode)
{
	m_e3DGizmoMode = Mode;
}

void CGame::UpdateVSSpace(const XMMATRIX& World)
{
	m_cbVSSpaceData.World = XMMatrixTranspose(World);
	m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
}

void CGame::UpdateVS2DSpace(const XMMATRIX& World)
{
	m_cbVS2DSpaceData.World = XMMatrixTranspose(World);
	m_cbVS2DSpaceData.Projection = XMMatrixTranspose(m_MatrixProjection2D);
}

void CGame::UpdateVSAnimationBoneMatrices(const XMMATRIX* const BoneMatrices)
{
	memcpy(m_cbVSAnimationBonesData.BoneMatrices, BoneMatrices, sizeof(SCBVSAnimationBonesData));
}

void CGame::UpdateVSTerrainData(const CTerrain::SCBVSTerrainData& Data)
{
	m_cbVSTerrainData = Data;
}

void CGame::UpdateHSTessFactor(float TessFactor)
{
	m_cbHSTessFactor.TessFactor = TessFactor;
}

void CGame::UpdateDSDisplacementData(bool bUseDisplacement)
{
	m_cbDSDisplacementData.bUseDisplacement = bUseDisplacement;

	m_DSTerrain->UpdateAllConstantBuffers();
}

void CGame::UpdateGSSpace()
{
	m_cbGSSpaceData.VP = GetTransposedVPMatrix();
}

void CGame::UpdatePSBaseMaterial(const CMaterial& Material)
{
	m_cbPSBaseMaterialData.MaterialAmbient = Material.GetAmbientColor();
	m_cbPSBaseMaterialData.MaterialDiffuse = Material.GetDiffuseColor();
	m_cbPSBaseMaterialData.MaterialSpecular = Material.GetSpecularColor();
	m_cbPSBaseMaterialData.SpecularExponent = Material.GetSpecularExponent();
	m_cbPSBaseMaterialData.SpecularIntensity = Material.GetSpecularIntensity();

	m_cbPSBaseMaterialData.bHasDiffuseTexture = Material.HasTexture(CMaterial::CTexture::EType::DiffuseTexture);
	m_cbPSBaseMaterialData.bHasNormalTexture = Material.HasTexture(CMaterial::CTexture::EType::NormalTexture);
	m_cbPSBaseMaterialData.bHasOpacityTexture = Material.HasTexture(CMaterial::CTexture::EType::OpacityTexture);

	m_PSBase->UpdateConstantBuffer(2);
}

void CGame::UpdatePSTerrainSpace(const XMMATRIX& Matrix)
{
	m_cbPSTerrainSpaceData.Matrix = XMMatrixTranspose(Matrix);
}

void CGame::UpdatePSTerrainSelection(const CTerrain::SCBPSTerrainSelectionData& Selection)
{
	m_cbPSTerrainSelectionData = Selection;
}

void CGame::UpdatePSBase2DFlagOn(EFlagPSBase2D Flag)
{
	switch (Flag)
	{
	case EFlagPSBase2D::UseTexture:
		m_cbPS2DFlagsData.bUseTexture = TRUE;
		break;
	default:
		break;
	}
	m_PSBase2D->UpdateConstantBuffer(0);
}

void CGame::UpdatePSBase2DFlagOff(EFlagPSBase2D Flag)
{
	switch (Flag)
	{
	case EFlagPSBase2D::UseTexture:
		m_cbPS2DFlagsData.bUseTexture = FALSE;
		break;
	default:
		break;
	}
	m_PSBase2D->UpdateConstantBuffer(0);
}

void CGame::SetSky(const string& SkyDataFileName, float ScalingFactor)
{
	using namespace tinyxml2;

	m_SkyFileName = SkyDataFileName;
	m_SkyScalingFactor = ScalingFactor;

	size_t Point{ SkyDataFileName.find_last_of('.') };
	string Extension{ SkyDataFileName.substr(Point + 1) };
	for (auto& c : Extension)
	{
		c = toupper(c);
	}
	assert(Extension == "XML");
	
	tinyxml2::XMLDocument xmlDocument{};
	if (xmlDocument.LoadFile(SkyDataFileName.c_str()) != XML_SUCCESS)
	{
		MessageBox(nullptr, ("Sky 설정 파일을 찾을 수 없습니다. (" + SkyDataFileName + ")").c_str(), "Sky 설정 불러오기 실패", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	
	XMLElement* xmlRoot{ xmlDocument.FirstChildElement() };
	XMLElement* xmlTexture{ xmlRoot->FirstChildElement() };
	m_SkyData.TextureFileName = xmlTexture->GetText();

	XMLElement* xmlSun{ xmlTexture->NextSiblingElement() };
	{
		LoadSkyObjectData(xmlSun, m_SkyData.Sun);
	}

	XMLElement* xmlMoon{ xmlSun->NextSiblingElement() };
	{
		LoadSkyObjectData(xmlMoon, m_SkyData.Moon);
	}

	XMLElement* xmlCloud{ xmlMoon->NextSiblingElement() };
	{
		LoadSkyObjectData(xmlCloud, m_SkyData.Cloud);
	}

	m_SkyMaterial.SetTextureFileName(CMaterial::CTexture::EType::DiffuseTexture, m_SkyData.TextureFileName);

	m_Object3DSkySphere = make_unique<CObject3D>("SkySphere", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSkySphere->Create(GenerateSphere(KSkySphereSegmentCount, KSkySphereColorUp, KSkySphereColorBottom), m_SkyMaterial);
	m_Object3DSkySphere->ComponentTransform.Scaling = XMVectorSet(KSkyDistance, KSkyDistance, KSkyDistance, 0);
	m_Object3DSkySphere->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSkySphere->ComponentRender.PtrPS = m_PSSky.get();
	m_Object3DSkySphere->ComponentPhysics.bIsPickable = false;
	m_Object3DSkySphere->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;

	m_Object3DSun = make_unique<CObject3D>("Sun", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DSun->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterial);
	m_Object3DSun->UpdateQuadUV(m_SkyData.Sun.UVOffset, m_SkyData.Sun.UVSize);
	m_Object3DSun->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Sun.WidthHeightRatio, 0);
	m_Object3DSun->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DSun->ComponentRender.PtrPS = m_PSBase.get();
	m_Object3DSun->ComponentRender.bIsTransparent = true;
	m_Object3DSun->ComponentPhysics.bIsPickable = false;
	m_Object3DSun->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	
	m_Object3DMoon = make_unique<CObject3D>("Moon", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DMoon->Create(GenerateSquareYZPlane(KColorWhite), m_SkyMaterial);
	m_Object3DMoon->UpdateQuadUV(m_SkyData.Moon.UVOffset, m_SkyData.Moon.UVSize);
	m_Object3DMoon->ComponentTransform.Scaling = XMVectorSet(1.0f, ScalingFactor, ScalingFactor * m_SkyData.Moon.WidthHeightRatio, 0);
	m_Object3DMoon->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DMoon->ComponentRender.PtrPS = m_PSBase.get();
	m_Object3DMoon->ComponentRender.bIsTransparent = true;
	m_Object3DMoon->ComponentPhysics.bIsPickable = false;
	m_Object3DMoon->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	
	/*
	SModel CloudModel{};
	CloudModel.vMeshes.emplace_back(GenerateSphere(64));
	CloudModel.vMaterials.resize(1);
	CloudModel.vMaterials[0].SetDiffuseTextureFileName("Asset\\earth_clouds.png");

	m_Object3DCloud = make_unique<CObject3D>("Cloud", m_Device.Get(), m_DeviceContext.Get(), this);
	m_Object3DCloud->Create(CloudModel);
	m_Object3DCloud->ComponentTransform.Scaling = XMVectorSet(KSkyDistance * 2, KSkyDistance * 4, KSkyDistance * 2, 0);
	m_Object3DCloud->ComponentTransform.Roll = -XM_PIDIV2;
	m_Object3DCloud->ComponentRender.PtrVS = m_VSSky.get();
	m_Object3DCloud->ComponentRender.PtrPS = m_PSCloud.get();
	m_Object3DCloud->ComponentRender.bIsTransparent = true;
	m_Object3DCloud->ComponentPhysics.bIsPickable = false;
	m_Object3DCloud->eFlagsRendering = CObject3D::EFlagsRendering::NoCulling | CObject3D::EFlagsRendering::NoLighting;
	*/

	m_SkyData.bIsDataSet = true;

	return;
}

void CGame::LoadSkyObjectData(const tinyxml2::XMLElement* const xmlSkyObject, SSkyData::SSkyObjectData& SkyObjectData)
{
	using namespace tinyxml2;

	const XMLElement* xmlUVOffset{ xmlSkyObject->FirstChildElement() };
	SkyObjectData.UVOffset.x = xmlUVOffset->FloatAttribute("U");
	SkyObjectData.UVOffset.y = xmlUVOffset->FloatAttribute("V");

	const XMLElement* xmlUVSize{ xmlUVOffset->NextSiblingElement() };
	SkyObjectData.UVSize.x = xmlUVSize->FloatAttribute("U");
	SkyObjectData.UVSize.y = xmlUVSize->FloatAttribute("V");

	const XMLElement* xmlWidthHeightRatio{ xmlUVSize->NextSiblingElement() };
	SkyObjectData.WidthHeightRatio = stof(xmlWidthHeightRatio->GetText());
}

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition)
{
	m_cbPSLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
}

void CGame::SetDirectionalLight(const XMVECTOR& LightSourcePosition, const XMVECTOR& Color)
{
	m_cbPSLightsData.DirectionalLightDirection = XMVector3Normalize(LightSourcePosition);
	m_cbPSLightsData.DirectionalLightColor = Color;
}

const XMVECTOR& CGame::GetDirectionalLightDirection() const
{
	return m_cbPSLightsData.DirectionalLightDirection;
}

void CGame::SetAmbientlLight(const XMFLOAT3& Color, float Intensity)
{
	m_cbPSLightsData.AmbientLightColor = Color;
	m_cbPSLightsData.AmbientLightIntensity = Intensity;
}

const XMFLOAT3& CGame::GetAmbientLightColor() const
{
	return m_cbPSLightsData.AmbientLightColor;
}

float CGame::GetAmbientLightIntensity() const
{
	return m_cbPSLightsData.AmbientLightIntensity;
}

void CGame::CreateTerrain(const XMFLOAT2& TerrainSize, const CMaterial& Material, float MaskingDetail)
{
	m_Terrain.release();
	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Create(TerrainSize, Material, MaskingDetail);

	ID3D11ShaderResourceView* NullSRVs[11]{};
	m_DeviceContext->DSSetShaderResources(0, 1, NullSRVs);
	m_DeviceContext->PSSetShaderResources(0, 11, NullSRVs);
}

void CGame::LoadTerrain(const string& TerrainFileName)
{
	if (TerrainFileName.empty()) return;

	m_Terrain.release();
	m_Terrain = make_unique<CTerrain>(m_Device.Get(), m_DeviceContext.Get(), this);
	m_Terrain->Load(TerrainFileName);
	
	ClearMaterials();
	int MaterialCount{ m_Terrain->GetMaterialCount() };
	for (int iMaterial = 0; iMaterial < MaterialCount; ++iMaterial)
	{
		const CMaterial& Material{ m_Terrain->GetMaterial(iMaterial) };

		AddMaterial(Material);
	}
}

void CGame::SaveTerrain(const string& TerrainFileName)
{
	if (!m_Terrain) return;
	if (TerrainFileName.empty()) return;

	m_Terrain->Save(TerrainFileName);
}

void CGame::AddTerrainMaterial(const CMaterial& Material)
{
	if (!m_Terrain) return;

	m_Terrain->AddMaterial(Material);
}

void CGame::SetTerrainMaterial(int MaterialID, const CMaterial& Material)
{
	if (!m_Terrain) return;

	m_Terrain->SetMaterial(MaterialID, Material);
}

void CGame::SetTerrainSelectionSize(float& Size)
{
	m_Terrain->SetSelectionSize(Size);
}

CCamera* CGame::AddCamera(const CCamera::SCameraData& CameraData)
{
	m_vCameras.emplace_back(CameraData);

	return &m_vCameras.back();
}

CCamera* CGame::GetCamera(size_t Index)
{
	assert(Index < m_vCameras.size());
	return &m_vCameras[Index];
}

void CGame::CreateWin32Window(WNDPROC const WndProc, LPCTSTR const WindowName)
{
	assert(!m_hWnd);

	constexpr LPCTSTR KClassName{ TEXT("GameWindow") };
	constexpr DWORD KWindowStyle{ WS_CAPTION | WS_SYSMENU };

	WNDCLASSEX WindowClass{};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	WindowClass.hIcon = WindowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	WindowClass.hInstance = m_hInstance;
	WindowClass.lpfnWndProc = WndProc;
	WindowClass.lpszClassName = KClassName;
	WindowClass.lpszMenuName = nullptr;
	WindowClass.style = CS_VREDRAW | CS_HREDRAW;
	RegisterClassEx(&WindowClass);

	RECT WindowRect{};
	WindowRect.right = static_cast<LONG>(m_WindowSize.x);
	WindowRect.bottom = static_cast<LONG>(m_WindowSize.y);
	AdjustWindowRect(&WindowRect, KWindowStyle, false);

	assert(m_hWnd = CreateWindowEx(0, KClassName, WindowName, KWindowStyle,
		CW_USEDEFAULT,  CW_USEDEFAULT, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top,
		nullptr, nullptr, m_hInstance, nullptr));

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
}

void CGame::InitializeDirectX(const wstring& FontFileName, bool bWindowed)
{
	CreateSwapChain(bWindowed);

	CreateSetViews();

	SetViewports();

	CreateDepthStencilStates();

	SetPerspective(KDefaultFOV, KDefaultNearZ, KDefaultFarZ);

	CreateInputDevices();

	CreateBaseShaders();

	CreateMiniAxes();

	CreatePickingRay();
	CreatePickedTriangle();

	CreateBoundingSphere();

	Create3DGizmos();

	m_MatrixProjection2D = XMMatrixOrthographicLH(m_WindowSize.x, m_WindowSize.y, 0.0f, 1.0f);
	m_SpriteBatch = make_unique<SpriteBatch>(m_DeviceContext.Get());
	m_SpriteFont = make_unique<SpriteFont>(m_Device.Get(), FontFileName.c_str());
	m_CommonStates = make_unique<CommonStates>(m_Device.Get());
}


void CGame::CreateSwapChain(bool bWindowed)
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc{};
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.Width = static_cast<UINT>(m_WindowSize.x);
	SwapChainDesc.BufferDesc.Height = static_cast<UINT>(m_WindowSize.y);
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.Flags = 0;
	SwapChainDesc.OutputWindow = m_hWnd;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Windowed = bWindowed;

	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
		&SwapChainDesc, &m_SwapChain, &m_Device, nullptr, &m_DeviceContext);
}

void CGame::CreateSetViews()
{
	ComPtr<ID3D11Texture2D> BackBuffer{};
	m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &BackBuffer);

	m_Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &m_RenderTargetView);

	D3D11_TEXTURE2D_DESC DepthStencilBufferDesc{};
	DepthStencilBufferDesc.ArraySize = 1;
	DepthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilBufferDesc.CPUAccessFlags = 0;
	DepthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilBufferDesc.Width = static_cast<UINT>(m_WindowSize.x);
	DepthStencilBufferDesc.Height = static_cast<UINT>(m_WindowSize.y);
	DepthStencilBufferDesc.MipLevels = 0;
	DepthStencilBufferDesc.MiscFlags = 0;
	DepthStencilBufferDesc.SampleDesc.Count = 1;
	DepthStencilBufferDesc.SampleDesc.Quality = 0;
	DepthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	m_Device->CreateTexture2D(&DepthStencilBufferDesc, nullptr, &m_DepthStencilBuffer);
	m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, &m_DepthStencilView);

	m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
}

void CGame::SetViewports()
{
	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = m_WindowSize.x;
		Viewport.Height = m_WindowSize.y;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 20.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = m_WindowSize.y * 7.0f / 8.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}

	{
		m_vViewports.emplace_back();

		D3D11_VIEWPORT& Viewport{ m_vViewports.back() };
		Viewport.TopLeftX = m_WindowSize.x * 1.0f / 8.0f;
		Viewport.TopLeftY = m_WindowSize.y * 7.0f / 8.0f;
		Viewport.Width = m_WindowSize.x / 8.0f;
		Viewport.Height = m_WindowSize.y / 8.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
	}
}

void CGame::CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc{};
	DepthStencilDesc.DepthEnable = TRUE;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.StencilEnable = FALSE;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateLessEqualNoWrite.GetAddressOf())));

	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	assert(SUCCEEDED(m_Device->CreateDepthStencilState(&DepthStencilDesc, m_DepthStencilStateAlways.GetAddressOf())));
}

void CGame::CreateInputDevices()
{
	m_Keyboard = make_unique<Keyboard>();

	m_Mouse = make_unique<Mouse>();
	m_Mouse->SetWindow(m_hWnd);
	m_Mouse->SetMode(Mouse::Mode::MODE_ABSOLUTE);
}

void CGame::CreateBaseShaders()
{
	m_VSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase->Create(EShaderType::VertexShader, L"Shader\\VSBase.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSBase->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSInstance = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSInstance->Create(EShaderType::VertexShader, L"Shader\\VSInstance.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSInstance->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSAnimation = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSAnimation->Create(EShaderType::VertexShader, L"Shader\\VSAnimation.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSAnimation->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));
	m_VSAnimation->AddConstantBuffer(&m_cbVSAnimationBonesData, sizeof(SCBVSAnimationBonesData));

	m_VSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSSky->Create(EShaderType::VertexShader, L"Shader\\VSSky.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSSky->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSLine->Create(EShaderType::VertexShader, L"Shader\\VSLine.hlsl", "main", KVSLineInputElementDescs, ARRAYSIZE(KVSLineInputElementDescs));
	m_VSLine->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSGizmo->Create(EShaderType::VertexShader, L"Shader\\VSGizmo.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSGizmo->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));

	m_VSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSTerrain->Create(EShaderType::VertexShader, L"Shader\\VSTerrain.hlsl", "main", KBaseInputElementDescs, ARRAYSIZE(KBaseInputElementDescs));
	m_VSTerrain->AddConstantBuffer(&m_cbVSSpaceData, sizeof(SCBVSSpaceData));
	m_VSTerrain->AddConstantBuffer(&m_cbVSTerrainData, sizeof(CTerrain::SCBVSTerrainData));

	m_VSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSParticle->Create(EShaderType::VertexShader, L"Shader\\VSParticle.hlsl", "main", KParticleInputElementDescs, ARRAYSIZE(KParticleInputElementDescs));

	m_VSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_VSBase2D->Create(EShaderType::VertexShader, L"Shader\\VSBase2D.hlsl", "main", KVS2DBaseInputLayout, ARRAYSIZE(KVS2DBaseInputLayout));
	m_VSBase2D->AddConstantBuffer(&m_cbVS2DSpaceData, sizeof(SCBVS2DSpaceData));

	m_HSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSTerrain->Create(EShaderType::HullShader, L"Shader\\HSTerrain.hlsl", "main");
	m_HSTerrain->AddConstantBuffer(&m_cbHSCameraData, sizeof(SCBHSCameraData));
	m_HSTerrain->AddConstantBuffer(&m_cbHSTessFactor, sizeof(SCBHSTessFactorData));

	m_HSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_HSWater->Create(EShaderType::HullShader, L"Shader\\HSWater.hlsl", "main");
	m_HSWater->AddConstantBuffer(&m_cbHSCameraData, sizeof(SCBHSCameraData));
	m_HSWater->AddConstantBuffer(&m_cbHSTessFactor, sizeof(SCBHSTessFactorData));

	m_DSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSTerrain->Create(EShaderType::DomainShader, L"Shader\\DSTerrain.hlsl", "main");
	m_DSTerrain->AddConstantBuffer(&m_cbDSSpaceData, sizeof(SCBDSSpaceData));
	m_DSTerrain->AddConstantBuffer(&m_cbDSDisplacementData, sizeof(SCBDSDisplacementData));

	m_DSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_DSWater->Create(EShaderType::DomainShader, L"Shader\\DSWater.hlsl", "main");
	m_DSWater->AddConstantBuffer(&m_cbDSSpaceData, sizeof(SCBDSSpaceData));
	m_DSWater->AddConstantBuffer(&m_cbWaterTimeData, sizeof(SCBWaterTimeData));

	m_GSNormal = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSNormal->Create(EShaderType::GeometryShader, L"Shader\\GSNormal.hlsl", "main");
	m_GSNormal->AddConstantBuffer(&m_cbGSSpaceData, sizeof(SCBGSSpaceData));

	m_GSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_GSParticle->Create(EShaderType::GeometryShader, L"Shader\\GSParticle.hlsl", "main");
	m_GSParticle->AddConstantBuffer(&m_cbGSSpaceData, sizeof(SCBGSSpaceData));

	m_PSBase = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase->Create(EShaderType::PixelShader, L"Shader\\PSBase.hlsl", "main");
	m_PSBase->AddConstantBuffer(&m_cbPSBaseFlagsData, sizeof(SCBPSBaseFlagsData));
	m_PSBase->AddConstantBuffer(&m_cbPSLightsData, sizeof(SCBPSLightsData));
	m_PSBase->AddConstantBuffer(&m_cbPSBaseMaterialData, sizeof(SCBPSBaseMaterialData));

	m_PSVertexColor = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSVertexColor->Create(EShaderType::PixelShader, L"Shader\\PSVertexColor.hlsl", "main");

	m_PSSky = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSSky->Create(EShaderType::PixelShader, L"Shader\\PSSky.hlsl", "main");
	m_PSSky->AddConstantBuffer(&m_cbPSSkyTimeData, sizeof(SCBPSSkyTimeData));

	m_PSCloud = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSCloud->Create(EShaderType::PixelShader, L"Shader\\PSCloud.hlsl", "main");
	m_PSCloud->AddConstantBuffer(&m_cbPSSkyTimeData, sizeof(SCBPSSkyTimeData));

	m_PSLine = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSLine->Create(EShaderType::PixelShader, L"Shader\\PSLine.hlsl", "main");

	m_PSGizmo = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSGizmo->Create(EShaderType::PixelShader, L"Shader\\PSGizmo.hlsl", "main");
	m_PSGizmo->AddConstantBuffer(&m_cbPSGizmoColorFactorData, sizeof(SCBPSGizmoColorFactorData));

	m_PSTerrain = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSTerrain->Create(EShaderType::PixelShader, L"Shader\\PSTerrain.hlsl", "main");
	m_PSTerrain->AddConstantBuffer(&m_cbPSTerrainSpaceData, sizeof(SCBPSTerrainSpaceData));
	m_PSTerrain->AddConstantBuffer(&m_cbPSLightsData, sizeof(SCBPSLightsData));
	m_PSTerrain->AddConstantBuffer(&m_cbPSTerrainSelectionData, sizeof(CTerrain::SCBPSTerrainSelectionData));
	m_PSTerrain->AddConstantBuffer(&m_cbEditorTimeData, sizeof(SCBEditorTimeData));

	m_PSWater = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSWater->Create(EShaderType::PixelShader, L"Shader\\PSWater.hlsl", "main");
	m_PSWater->AddConstantBuffer(&m_cbWaterTimeData, sizeof(SCBWaterTimeData));
	m_PSWater->AddConstantBuffer(&m_cbPSLightsData, sizeof(SCBPSLightsData));

	m_PSParticle = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSParticle->Create(EShaderType::PixelShader, L"Shader\\PSParticle.hlsl", "main");

	m_PSBase2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSBase2D->Create(EShaderType::PixelShader, L"Shader\\PSBase2D.hlsl", "main");
	m_PSBase2D->AddConstantBuffer(&m_cbPS2DFlagsData, sizeof(SCBPS2DFlagsData));

	m_PSMasking2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSMasking2D->Create(EShaderType::PixelShader, L"Shader\\PSMasking2D.hlsl", "main");

	m_PSHeightMap2D = make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get());
	m_PSHeightMap2D->Create(EShaderType::PixelShader, L"Shader\\PSHeightMap2D.hlsl", "main");
}

void CGame::CreateMiniAxes()
{
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisX", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisY", m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3DMiniAxes.emplace_back(make_unique<CObject3D>("AxisZ", m_Device.Get(), m_DeviceContext.Get(), this));

	SMesh Cone{ GenerateCone(0, 1.0f, 1.0f, 16) };
	vector<CMaterial> vMaterials{};
	vMaterials.resize(3);
	vMaterials[0].SetUniformColor(XMFLOAT3(1, 0, 0));
	vMaterials[1].SetUniformColor(XMFLOAT3(0, 1, 0));
	vMaterials[2].SetUniformColor(XMFLOAT3(0, 0, 1));
	m_vObject3DMiniAxes[0]->Create(Cone, vMaterials[0]);
	m_vObject3DMiniAxes[0]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[0]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[0]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vObject3DMiniAxes[0]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[1]->Create(Cone, vMaterials[1]);
	m_vObject3DMiniAxes[1]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[1]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[1]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;

	m_vObject3DMiniAxes[2]->Create(Cone, vMaterials[2]);
	m_vObject3DMiniAxes[2]->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3DMiniAxes[2]->ComponentRender.PtrPS = m_PSBase.get();
	m_vObject3DMiniAxes[2]->ComponentTransform.Yaw = -XM_PIDIV2;
	m_vObject3DMiniAxes[2]->ComponentTransform.Roll = -XM_PIDIV2;
	m_vObject3DMiniAxes[2]->eFlagsRendering = CObject3D::EFlagsRendering::NoLighting;
	
	m_vObject3DMiniAxes[0]->ComponentTransform.Scaling =
		m_vObject3DMiniAxes[1]->ComponentTransform.Scaling =
		m_vObject3DMiniAxes[2]->ComponentTransform.Scaling = XMVectorSet(0.1f, 0.8f, 0.1f, 0);
}

void CGame::CreatePickingRay()
{
	m_Object3DLinePickingRay = make_unique<CObject3DLine>("PickingRay", m_Device.Get(), m_DeviceContext.Get());

	vector<SVertex3DLine> Vertices{};
	Vertices.emplace_back(XMVectorSet(0, 0, 0, 1), XMVectorSet(1, 0, 0, 1));
	Vertices.emplace_back(XMVectorSet(10.0f, 10.0f, 0, 1), XMVectorSet(0, 1, 0, 1));

	m_Object3DLinePickingRay->Create(Vertices);
}

void CGame::CreateBoundingSphere()
{
	m_Object3DBoundingSphere = make_unique<CObject3D>("BoundingSphere", m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DBoundingSphere->Create(GenerateSphere(16));
}

void CGame::CreatePickedTriangle()
{
	m_Object3DPickedTriangle = make_unique<CObject3D>("PickedTriangle", m_Device.Get(), m_DeviceContext.Get(), this);

	m_Object3DPickedTriangle->Create(GenerateTriangle(XMVectorSet(0, 0, 1.5f, 1), XMVectorSet(+1.0f, 0, 0, 1), XMVectorSet(-1.0f, 0, 0, 1),
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f)));
}

void CGame::Create3DGizmos()
{
	const static XMVECTOR ColorX{ XMVectorSet(1.0f, 0.1f, 0.1f, 1) };
	const static XMVECTOR ColorY{ XMVectorSet(0.1f, 1.0f, 0.1f, 1) };
	const static XMVECTOR ColorZ{ XMVectorSet(0.1f, 0.1f, 1.0f, 1) };

	m_Object3D_3DGizmoRotationPitch = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorX, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationPitch->Create(MeshRing);
		m_Object3D_3DGizmoRotationPitch->ComponentTransform.Roll = -XM_PIDIV2;
		m_Object3D_3DGizmoRotationPitch->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoRotationPitch->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoRotationYaw = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorY, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationYaw->Create(MeshRing);
		m_Object3D_3DGizmoRotationYaw->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoRotationYaw->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoRotationRoll = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshRing{ GenerateTorus(ColorZ, 0.05f) };
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		MeshRing = MergeStaticMeshes(MeshRing, MeshAxis);
		m_Object3D_3DGizmoRotationRoll->Create(MeshRing);
		m_Object3D_3DGizmoRotationRoll->ComponentTransform.Pitch = XM_PIDIV2;
		m_Object3D_3DGizmoRotationRoll->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoRotationRoll->ComponentRender.PtrPS = m_PSGizmo.get();
	}


	m_Object3D_3DGizmoTranslationX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorX) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationX->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationX->ComponentTransform.Roll = -XM_PIDIV2;
		m_Object3D_3DGizmoTranslationX->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoTranslationX->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoTranslationY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorY) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationY->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationY->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoTranslationY->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoTranslationZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, ColorZ) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoTranslationZ->Create(MeshAxis);
		m_Object3D_3DGizmoTranslationZ->ComponentTransform.Pitch = XM_PIDIV2;
		m_Object3D_3DGizmoTranslationZ->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoTranslationZ->ComponentRender.PtrPS = m_PSGizmo.get();
	}


	m_Object3D_3DGizmoScalingX = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorX) };
		SMesh MeshCube{ GenerateCube(ColorX) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingX->Create(MeshAxis);
		m_Object3D_3DGizmoScalingX->ComponentTransform.Roll = -XM_PIDIV2;
		m_Object3D_3DGizmoScalingX->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoScalingX->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoScalingY = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorY) };
		SMesh MeshCube{ GenerateCube(ColorY) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingY->Create(MeshAxis);
		m_Object3D_3DGizmoScalingY->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoScalingY->ComponentRender.PtrPS = m_PSGizmo.get();
	}

	m_Object3D_3DGizmoScalingZ = make_unique<CObject3D>("Gizmo", m_Device.Get(), m_DeviceContext.Get(), this);
	{
		SMesh MeshAxis{ GenerateCylinder(0.05f, 1.0f, 16, ColorZ) };
		SMesh MeshCube{ GenerateCube(ColorZ) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_Object3D_3DGizmoScalingZ->Create(MeshAxis);
		m_Object3D_3DGizmoScalingZ->ComponentTransform.Pitch = XM_PIDIV2;
		m_Object3D_3DGizmoScalingZ->ComponentRender.PtrVS = m_VSGizmo.get();
		m_Object3D_3DGizmoScalingZ->ComponentRender.PtrPS = m_PSGizmo.get();
	}
}

CShader* CGame::AddShader()
{
	m_vShaders.emplace_back(make_unique<CShader>(m_Device.Get(), m_DeviceContext.Get()));
	return m_vShaders.back().get();
}

CShader* CGame::GetShader(size_t Index) const
{
	assert(Index < m_vShaders.size());
	return m_vShaders[Index].get();
}

CShader* CGame::GetBaseShader(EBaseShader eShader) const
{
	CShader* Result{};
	switch (eShader)
	{
	case EBaseShader::VSBase:
		Result = m_VSBase.get();
		break;
	case EBaseShader::VSInstance:
		Result = m_VSInstance.get();
		break;
	case EBaseShader::VSAnimation:
		Result = m_VSAnimation.get();
		break;
	case EBaseShader::VSSky:
		Result = m_VSSky.get();
		break;
	case EBaseShader::VSLine:
		Result = m_VSLine.get();
		break;
	case EBaseShader::VSGizmo:
		Result = m_VSGizmo.get();
		break;
	case EBaseShader::VSTerrain:
		Result = m_VSTerrain.get();
		break;
	case EBaseShader::VSBase2D:
		Result = m_VSBase2D.get();
		break;
	case EBaseShader::HSTerrain:
		Result = m_HSTerrain.get();
		break;
	case EBaseShader::HSWater:
		Result = m_HSWater.get();
		break;
	case EBaseShader::DSTerrain:
		Result = m_DSTerrain.get();
		break;
	case EBaseShader::DSWater:
		Result = m_DSWater.get();
		break;
	case EBaseShader::GSNormal:
		Result = m_GSNormal.get();
		break;
	case EBaseShader::PSBase:
		Result = m_PSBase.get();
		break;
	case EBaseShader::PSVertexColor:
		Result = m_PSVertexColor.get();
		break;
	case EBaseShader::PSSky:
		Result = m_PSSky.get();
		break;
	case EBaseShader::PSCloud:
		Result = m_PSCloud.get();
		break;
	case EBaseShader::PSLine:
		Result = m_PSLine.get();
		break;
	case EBaseShader::PSGizmo:
		Result = m_PSGizmo.get();
		break;
	case EBaseShader::PSTerrain:
		Result = m_PSTerrain.get();
		break;
	case EBaseShader::PSWater:
		Result = m_PSWater.get();
		break;
	case EBaseShader::PSBase2D:
		Result = m_PSBase2D.get();
		break;
	case EBaseShader::PSMasking2D:
		Result = m_PSMasking2D.get();
		break;
	case EBaseShader::PSHeightMap2D:
		Result = m_PSHeightMap2D.get();
		break;
	default:
		assert(Result);
		break;
	}

	return Result;
}

void CGame::InsertObject3D(const string& Name)
{
	if (m_mapObject3DNameToIndex.find(Name) != m_mapObject3DNameToIndex.end())
	{
		MessageBox(nullptr, string("이미 존재하는 이름입니다. (" + Name + ")").c_str(), "이름 충돌", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if (Name.size() >= KObject3DNameMaxLength)
	{
		MessageBox(nullptr, string("이름이 너무 깁니다. (" + Name + ")").c_str(), "이름 길이 제한 초과", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	else if (Name.size() == 0)
	{
		MessageBox(nullptr, "이름은 공백일 수 없습니다.", "이름 조건 불일치", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	m_vObject3Ds.emplace_back(make_unique<CObject3D>(Name, m_Device.Get(), m_DeviceContext.Get(), this));
	m_vObject3Ds.back()->ComponentRender.PtrVS = m_VSBase.get();
	m_vObject3Ds.back()->ComponentRender.PtrPS = m_PSBase.get();

	m_mapObject3DNameToIndex[Name] = m_vObject3Ds.size() - 1;
}

void CGame::EraseObject3D(const string& Name)
{
	if (!m_vObject3Ds.size()) return;
	if (Name.length() == 0) return;
	if (m_mapObject3DNameToIndex.find(Name) == m_mapObject3DNameToIndex.end())
	{
		MessageBox(nullptr, string("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "이름 검색 실패", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	size_t iObject3D{ m_mapObject3DNameToIndex[Name] };
	if (iObject3D < m_vObject3Ds.size() - 1)
	{
		const string& SwappedName{ m_vObject3Ds.back()->GetName() };
		swap(m_vObject3Ds[iObject3D], m_vObject3Ds.back());

		m_mapObject3DNameToIndex[SwappedName] = iObject3D;
	}

	if (IsAnyObject3DSelected())
	{
		if (Name == GetSelectedObject3DName())
		{
			DeselectObject3D();
		}
	}

	m_vObject3Ds.back().release();
	m_vObject3Ds.pop_back();
	m_mapObject3DNameToIndex.erase(Name);
}

void CGame::ClearObject3Ds()
{
	m_mapObject3DNameToIndex.clear();
	m_vObject3Ds.clear();

	m_PtrSelectedObject3D = nullptr;
}

CObject3D* CGame::GetObject3D(const string& Name) const
{
	if (m_mapObject3DNameToIndex.find(Name) == m_mapObject3DNameToIndex.end())
	{
		MessageBox(nullptr, string("존재하지 않는 이름입니다. (" + Name + ")").c_str(), "이름 검색 실패", MB_OK | MB_ICONEXCLAMATION);
		return nullptr;
	}
	return m_vObject3Ds[m_mapObject3DNameToIndex.at(Name)].get();
}

void CGame::InsertObject3DLine(const string& Name)
{
	assert(m_umapObject3DLineNameToIndex.find(Name) == m_umapObject3DLineNameToIndex.end());

	m_vObject3DLines.emplace_back(make_unique<CObject3DLine>(Name, m_Device.Get(), m_DeviceContext.Get()));
	
	m_umapObject3DLineNameToIndex[Name] = m_vObject3DLines.size() - 1;
}

CObject3DLine* CGame::GetObject3DLine(const string& Name) const
{
	assert(m_umapObject3DLineNameToIndex.find(Name) != m_umapObject3DLineNameToIndex.end());
	return m_vObject3DLines[m_umapObject3DLineNameToIndex.at(Name)].get();
}

void CGame::InsertObject2D(const string& Name)
{
	assert(m_umapObject2DNameToIndex.find(Name) == m_umapObject2DNameToIndex.end());
	m_vObject2Ds.emplace_back(make_unique<CObject2D>(Name, m_Device.Get(), m_DeviceContext.Get()));
	
	m_umapObject2DNameToIndex[Name] = m_vObject2Ds.size() - 1;
}

CObject2D* CGame::GetObject2D(const string& Name) const
{
	assert(m_umapObject2DNameToIndex.find(Name) != m_umapObject2DNameToIndex.end());
	return m_vObject2Ds[m_umapObject2DNameToIndex.at(Name)].get();
}

CMaterial* CGame::AddMaterial(const CMaterial& Material)
{
	if (m_mapMaterialNameToIndex.find(Material.GetName()) != m_mapMaterialNameToIndex.end()) return nullptr;

	m_vMaterials.emplace_back(make_unique<CMaterial>(Material));
	
	m_vMaterialDiffuseTextures.resize(m_vMaterials.size());
	m_vMaterialNormalTextures.resize(m_vMaterials.size());
	m_vMaterialDisplacementTextures.resize(m_vMaterials.size());

	m_mapMaterialNameToIndex[Material.GetName()] = m_vMaterials.size() - 1;

	LoadMaterial(Material.GetName());

	return m_vMaterials.back().get();
}

CMaterial* CGame::GetMaterial(const string& Name) const
{
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end()) return nullptr;

	return m_vMaterials[m_mapMaterialNameToIndex.at(Name)].get();
}

void CGame::ClearMaterials()
{
	m_vMaterials.clear();
	m_vMaterialDiffuseTextures.clear();
	m_vMaterialNormalTextures.clear();
	m_mapMaterialNameToIndex.clear();
}

size_t CGame::GetMaterialCount() const
{
	return m_vMaterials.size();
}

void CGame::ChangeMaterialName(const string& OldName, const string& NewName)
{
	size_t iMaterial{ m_mapMaterialNameToIndex[OldName] };
	CMaterial* Material{ m_vMaterials[iMaterial].get() };
	auto a =m_mapMaterialNameToIndex.find(OldName);
	
	m_mapMaterialNameToIndex.erase(OldName);
	m_mapMaterialNameToIndex.insert(make_pair(NewName, iMaterial));

	Material->SetName(NewName);
}

void CGame::LoadMaterial(const string& Name)
{
	if (m_mapMaterialNameToIndex.find(Name) == m_mapMaterialNameToIndex.end()) return;

	size_t iMaterial{ m_mapMaterialNameToIndex[Name] };
	CMaterial* Material{ m_vMaterials[iMaterial].get() };
	if (!Material->HasTexture()) return;

	CreateMaterialTexture(CMaterial::CTexture::EType::DiffuseTexture, *Material);
	CreateMaterialTexture(CMaterial::CTexture::EType::NormalTexture, *Material);
	CreateMaterialTexture(CMaterial::CTexture::EType::DisplacementTexture, *Material);
	CreateMaterialTexture(CMaterial::CTexture::EType::OpacityTexture, *Material);
}

void CGame::CreateMaterialTexture(CMaterial::CTexture::EType eType, CMaterial& Material)
{
	if (Material.HasTexture(eType))
	{
		CMaterial::CTexture* PtrTexture{};
		size_t iTexture{};

		switch (eType)
		{
		case CMaterial::CTexture::EType::DiffuseTexture:
			m_vMaterialDiffuseTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialDiffuseTextures.back().get();
			iTexture = m_vMaterialDiffuseTextures.size() - 1;
			break;
		case CMaterial::CTexture::EType::NormalTexture:
			m_vMaterialNormalTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialNormalTextures.back().get();
			iTexture = m_vMaterialNormalTextures.size() - 1;
			break;
		case CMaterial::CTexture::EType::DisplacementTexture:
			m_vMaterialDisplacementTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialDisplacementTextures.back().get();
			iTexture = m_vMaterialDisplacementTextures.size() - 1;
			break;
		case CMaterial::CTexture::EType::OpacityTexture:
			m_vMaterialOpacityTextures.back() = make_unique<CMaterial::CTexture>(m_Device.Get(), m_DeviceContext.Get());
			PtrTexture = m_vMaterialOpacityTextures.back().get();
			iTexture = m_vMaterialOpacityTextures.size() - 1;
			break;
		default:
			break;
		}

		if (Material.IsTextureEmbedded(eType))
		{
			PtrTexture->CreateTextureFromMemory(Material.GetTextureRawData(eType));
			Material.ClearEmbeddedTextureData(eType);
		}
		else
		{
			PtrTexture->CreateTextureFromFile(Material.GetTextureFileName(eType), Material.ShouldGenerateAutoMipMap());
		}
	}
}

CMaterial::CTexture* CGame::GetMaterialTexture(CMaterial::CTexture::EType eType, const string& Name) const
{
	assert(m_mapMaterialNameToIndex.find(Name) != m_mapMaterialNameToIndex.end());
	size_t iMaterial{ m_mapMaterialNameToIndex.at(Name) };

	switch (eType)
	{
	case CMaterial::CTexture::EType::DiffuseTexture:
		if (m_vMaterialDiffuseTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialDiffuseTextures[iMaterial].get();
	case CMaterial::CTexture::EType::NormalTexture:
		if (m_vMaterialNormalTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialNormalTextures[iMaterial].get();
	case CMaterial::CTexture::EType::DisplacementTexture:
		if (m_vMaterialDisplacementTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialDisplacementTextures[iMaterial].get();
	case CMaterial::CTexture::EType::OpacityTexture:
		if (m_vMaterialOpacityTextures.size() <= iMaterial) return nullptr;
		return m_vMaterialOpacityTextures[iMaterial].get();
	default:
		return nullptr;
	}
}

void CGame::SetEditMode(EEditMode Mode, bool bForcedSet)
{
	if (!bForcedSet)
	{
		if (m_eEditMode == Mode) return;
	}

	m_eEditMode = Mode;
	if (m_eEditMode == EEditMode::EditTerrain)
	{
		if (m_Terrain) m_Terrain->ShouldShowSelection(TRUE);
		DeselectObject3D();
	}
	else
	{
		if (m_Terrain) m_Terrain->ShouldShowSelection(FALSE);
	}
}

bool CGame::Pick()
{
	if (m_eEditMode != EEditMode::EditObject) return false;

	CastPickingRay();

	UpdatePickingRay();

	PickBoundingSphere();

	PickTriangle();

	if (m_PtrPickedObject3D) return true;
	return false;
}

void CGame::SelectObject3D(const string& Name)
{
	if (m_eEditMode != EEditMode::EditObject) return;

	m_PtrSelectedObject3D = GetObject3D(Name);
	if (m_PtrSelectedObject3D)
	{
		if (!m_PtrSelectedObject3D->IsInstanced())
		{
			m_SelectedInstanceID = -1;
			Interact3DGizmos();
		}
	}
}

const string& CGame::GetSelectedObject3DName() const
{
	if (m_PtrSelectedObject3D)
	{
		return m_PtrSelectedObject3D->GetName();
	}
	return m_NullString;
}

void CGame::SelectInstance(int InstanceID)
{
	m_SelectedInstanceID = InstanceID;

	if (IsAnyObject3DSelected() && IsAnyInstanceSelected())
	{
		Interact3DGizmos();
	}
}

void CGame::DeselectInstance()
{
	m_SelectedInstanceID = -1;
}

bool CGame::IsAnyInstanceSelected() const
{
	return (m_SelectedInstanceID != -1) ? true : false;
}

int CGame::GetPickedInstanceID() const
{
	return m_PickedInstanceID;
}

bool CGame::IsAnyObject3DSelected() const
{
	return (m_PtrSelectedObject3D) ? true : false;
}

const string& CGame::GetPickedObject3DName() const
{
	if (m_PtrPickedObject3D)
	{
		return m_PtrPickedObject3D->GetName();
	}
	return m_NullString;
}

CObject3D* CGame::GetSelectedObject3D()
{
	return m_PtrSelectedObject3D;
}

int CGame::GetSelectedInstanceID() const
{
	return m_SelectedInstanceID;
}

void CGame::DeselectObject3D()
{
	const Mouse::State& MouseState{ m_Mouse->GetState() };

	m_PtrSelectedObject3D = nullptr;
	m_SelectedInstanceID = -1;
}

void CGame::CastPickingRay()
{
	const Mouse::State& MouseState{ m_Mouse->GetState() };

	float ViewSpaceRayDirectionX{ (MouseState.x / (m_WindowSize.x / 2.0f) - 1.0f) / XMVectorGetX(m_MatrixProjection.r[0]) };
	float ViewSpaceRayDirectionY{ (-(MouseState.y / (m_WindowSize.y / 2.0f) - 1.0f)) / XMVectorGetY(m_MatrixProjection.r[1]) };
	static float ViewSpaceRayDirectionZ{ 1.0f };

	static XMVECTOR ViewSpaceRayOrigin{ XMVectorSet(0, 0, 0, 1) };
	XMVECTOR ViewSpaceRayDirection{ XMVectorSet(ViewSpaceRayDirectionX, ViewSpaceRayDirectionY, ViewSpaceRayDirectionZ, 0) };

	XMMATRIX MatrixViewInverse{ XMMatrixInverse(nullptr, m_MatrixView) };
	m_PickingRayWorldSpaceOrigin = XMVector3TransformCoord(ViewSpaceRayOrigin, MatrixViewInverse);
	m_PickingRayWorldSpaceDirection = XMVector3TransformNormal(ViewSpaceRayDirection, MatrixViewInverse);
}

void CGame::PickBoundingSphere()
{
	m_PtrPickedObject3D = nullptr;
	m_PickedInstanceID = -1;

	XMVECTOR T{ KVectorGreatest };
	for (auto& i : m_vObject3Ds)
	{
		auto* Object3D{ i.get() };
		if (Object3D->ComponentPhysics.bIsPickable)
		{
			if (Object3D->ComponentPhysics.bIgnoreBoundingSphere)
			{
				m_PtrPickedObject3D = Object3D;

				if (PickTriangle())
				{
					break;
				}
				else
				{
					m_PtrPickedObject3D = nullptr;
				}
			}
			else
			{
				if (Object3D->IsInstanced())
				{
					int InstanceCount{ (int)Object3D->GetInstanceCount() };
					for (int iInstance = 0; iInstance < InstanceCount; ++iInstance)
					{
						auto& Instance{ Object3D->GetInstance(iInstance) };
						XMVECTOR NewT{ KVectorGreatest };
						if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
							Object3D->ComponentPhysics.BoundingSphere.Radius, Instance.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
						{
							if (XMVector3Less(NewT, T))
							{
								T = NewT;
								m_PtrPickedObject3D = Object3D;
								m_PickedInstanceID = iInstance;
							}
						}
					}
				}
				else
				{
					XMVECTOR NewT{ KVectorGreatest };
					if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
						Object3D->ComponentPhysics.BoundingSphere.Radius, Object3D->ComponentTransform.Translation + Object3D->ComponentPhysics.BoundingSphere.CenterOffset, &NewT))
					{
						if (XMVector3Less(NewT, T))
						{
							T = NewT;
							m_PtrPickedObject3D = Object3D;
						}
					}
				}
			}
		}
	}
}

bool CGame::PickTriangle()
{
	XMVECTOR T{ KVectorGreatest };
	if (m_PtrPickedObject3D)
	{
		// Pick only static models' triangle.
		if (m_PtrPickedObject3D->GetModel().bIsModelAnimated) return false;

		const XMMATRIX& WorldMatrix{ m_PtrPickedObject3D->ComponentTransform.MatrixWorld };
		for (const SMesh& Mesh : m_PtrPickedObject3D->GetModel().vMeshes)
		{
			for (const STriangle& Triangle : Mesh.vTriangles)
			{
				XMVECTOR V0{ Mesh.vVertices[Triangle.I0].Position };
				XMVECTOR V1{ Mesh.vVertices[Triangle.I1].Position };
				XMVECTOR V2{ Mesh.vVertices[Triangle.I2].Position };
				V0 = XMVector3TransformCoord(V0, WorldMatrix);
				V1 = XMVector3TransformCoord(V1, WorldMatrix);
				V2 = XMVector3TransformCoord(V2, WorldMatrix);

				XMVECTOR NewT{};
				if (IntersectRayTriangle(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, V0, V1, V2, &NewT))
				{
					if (XMVector3Less(NewT, T))
					{
						T = NewT;

						XMVECTOR N{ CalculateTriangleNormal(V0, V1, V2) };

						m_PickedTriangleV0 = V0 + N * 0.01f;
						m_PickedTriangleV1 = V1 + N * 0.01f;
						m_PickedTriangleV2 = V2 + N * 0.01f;

						return true;
					}
				}
			}
		}
	}
	return false;
}

void CGame::SelectTerrain(bool bShouldEdit, bool bIsLeftButton)
{
	if (!m_Terrain) return;
	if (m_eEditMode != EEditMode::EditTerrain) return;

	CastPickingRay();

	m_Terrain->Select(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, bShouldEdit, bIsLeftButton);
}

void CGame::SetTerrainEditMode(CTerrain::EEditMode Mode)
{
	if (!m_Terrain) return;
	m_Terrain->SetEditMode(Mode);
}

void CGame::SetTerrainMaskingLayer(CTerrain::EMaskingLayer eLayer)
{
	if (!m_Terrain) return;
	m_Terrain->SetMaskingLayer(eLayer);
}

void CGame::SetTerrainMaskingAttenuation(float Attenuation)
{
	if (!m_Terrain) return;
	m_Terrain->SetMaskingAttenuation(Attenuation);
}

void CGame::SetTerrainMaskingSize(float Size)
{
	if (!m_Terrain) return;
	m_Terrain->SetMaskingRadius(Size);
}

void CGame::BeginRendering(const FLOAT* ClearColor)
{
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), Colors::CornflowerBlue);
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ID3D11SamplerState* SamplerState{ m_CommonStates->LinearWrap() };
	m_DeviceContext->PSSetSamplers(0, 1, &SamplerState);
	m_DeviceContext->DSSetSamplers(0, 1, &SamplerState); // @important: in order to use displacement mapping

	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);

	SetUniversalRasterizerState();

	m_MatrixView = XMMatrixLookAtLH(m_vCameras[m_CurrentCameraIndex].GetEyePosition(), 
		m_vCameras[m_CurrentCameraIndex].GetFocusPosition(),
		m_vCameras[m_CurrentCameraIndex].GetUpDirection());
}

void CGame::Animate()
{
	for (auto& Object3D : m_vObject3Ds)
	{
		if (Object3D) Object3D->Animate();
	}
}

void CGame::Draw(float DeltaTime)
{
	m_cbEditorTimeData.NormalizedTime += DeltaTime;
	m_cbEditorTimeData.NormalizedTimeHalfSpeed += DeltaTime * 0.5f;
	if (m_cbEditorTimeData.NormalizedTime > 1.0f) m_cbEditorTimeData.NormalizedTime = 0.0f;
	if (m_cbEditorTimeData.NormalizedTimeHalfSpeed > 1.0f) m_cbEditorTimeData.NormalizedTimeHalfSpeed = 0.0f;

	m_cbWaterTimeData.Time += DeltaTime * 0.1f;
	if (m_cbWaterTimeData.Time > 1.0f) m_cbWaterTimeData.Time = 0.0f;

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);

	m_cbPSLightsData.EyePosition = m_vCameras[m_CurrentCameraIndex].GetEyePosition();

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawWireFrame))
	{
		m_eRasterizerState = ERasterizerState::WireFrame;
	}
	else
	{
		m_eRasterizerState = ERasterizerState::CullCounterClockwise;
	}

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawMiniAxes))
	{
		DrawMiniAxes();
	}

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawPickingData))
	{
		DrawPickingRay();

		DrawPickedTriangle();
	}

	if (m_SkyData.bIsDataSet)
	{
		DrawSky(DeltaTime);
	}

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::Use3DGizmos))
	{
		Draw3DGizmos();
	}

	DrawTerrain();

	for (auto& Object3D : m_vObject3Ds)
	{
		if (Object3D->ComponentRender.bIsTransparent) continue;

		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
		{
			DrawObject3DBoundingSphere(Object3D.get());
		}
	}

	DrawObject3DLines();

	// Transparent Object3Ds
	for (auto& Object3D : m_vObject3Ds)
	{
		if (!Object3D->ComponentRender.bIsTransparent) continue;

		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawBoundingSphere))
		{
			DrawObject3DBoundingSphere(Object3D.get());
		}
	}

	DrawObject2Ds();
}

void CGame::UpdateObject3D(CObject3D* const PtrObject3D)
{
	if (!PtrObject3D) return;

	assert(PtrObject3D->ComponentRender.PtrVS);
	assert(PtrObject3D->ComponentRender.PtrPS);
	CShader* VS{ PtrObject3D->ComponentRender.PtrVS };
	CShader* PS{ PtrObject3D->ComponentRender.PtrPS };

	m_cbVSSpaceData.World = XMMatrixTranspose(PtrObject3D->ComponentTransform.MatrixWorld);
	m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	PtrObject3D->UpdateWorldMatrix();

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::UseRawVertexColor))
	{
		PS = m_PSVertexColor.get();
	}

	SetUniversalbUseLighiting();
	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoLighting))
	{
		m_cbPSBaseFlagsData.bUseLighting = FALSE;
	}

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoTexture))
	{
		m_cbPSBaseFlagsData.bUseTexture = FALSE;
	}
	else
	{
		m_cbPSBaseFlagsData.bUseTexture = TRUE;
	}
	
	VS->Use();
	VS->UpdateAllConstantBuffers();

	PS->Use();
	PS->UpdateAllConstantBuffers();
}

void CGame::DrawObject3D(const CObject3D* const PtrObject3D)
{
	if (!PtrObject3D) return;

	if (PtrObject3D->ShouldTessellate())
	{
		m_HSTerrain->Use();
		m_cbHSCameraData.EyePosition = m_vCameras[m_CurrentCameraIndex].GetEyePosition();
		m_HSTerrain->UpdateConstantBuffer(0);

		m_DSTerrain->Use();
		m_cbDSSpaceData.VP = GetTransposedVPMatrix();
		m_DSTerrain->UpdateConstantBuffer(0);
	}
	else
	{
		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	}

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoCulling))
	{
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
	}
	else
	{
		SetUniversalRasterizerState();
	}

	if (EFLAG_HAS(PtrObject3D->eFlagsRendering, CObject3D::EFlagsRendering::NoDepthComparison))
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	}
	else
	{
		m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
	}

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals))
	{
		UpdateGSSpace();

		m_GSNormal->Use();
		m_GSNormal->UpdateAllConstantBuffers();
		
		PtrObject3D->Draw();

		m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	}
	else
	{
		PtrObject3D->Draw();
	}
}

void CGame::DrawObject3DBoundingSphere(const CObject3D* const PtrObject3D)
{
	m_VSBase->Use();

	XMMATRIX Translation{ XMMatrixTranslationFromVector(PtrObject3D->ComponentTransform.Translation + 
		PtrObject3D->ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX Scaling{ XMMatrixScaling(PtrObject3D->ComponentPhysics.BoundingSphere.Radius,
		PtrObject3D->ComponentPhysics.BoundingSphere.Radius, PtrObject3D->ComponentPhysics.BoundingSphere.Radius) };
	XMMATRIX World{ Scaling * Translation };
	m_cbVSSpaceData.World = XMMatrixTranspose(World);
	m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	m_VSBase->UpdateConstantBuffer(0);

	m_DeviceContext->RSSetState(m_CommonStates->Wireframe());

	m_Object3DBoundingSphere->Draw();

	SetUniversalRasterizerState();
}

void CGame::DrawObject3DLines()
{
	m_VSLine->Use();
	m_PSLine->Use();

	for (auto& Object3DLine : m_vObject3DLines)
	{
		if (Object3DLine)
		{
			if (!Object3DLine->bIsVisible) continue;

			Object3DLine->UpdateWorldMatrix();

			m_cbVSSpaceData.World = XMMatrixTranspose(Object3DLine->ComponentTransform.MatrixWorld);
			m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
			m_VSLine->UpdateConstantBuffer(0);

			Object3DLine->Draw();
		}
	}
}

void CGame::DrawObject2Ds()
{
	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthNone(), 0);
	m_DeviceContext->OMSetBlendState(m_CommonStates->NonPremultiplied(), nullptr, 0xFFFFFFFF);
	
	m_cbVS2DSpaceData.Projection = XMMatrixTranspose(m_MatrixProjection2D);

	m_VSBase2D->Use();
	m_PSBase2D->Use();

	for (auto& Object2D : m_vObject2Ds)
	{
		if (Object2D)
		{
			if (!Object2D->bIsVisible) continue;

			Object2D->UpdateWorldMatrix();
			m_cbVS2DSpaceData.World = XMMatrixTranspose(Object2D->ComponentTransform.MatrixWorld);
			m_VSBase2D->UpdateConstantBuffer(0);

			/*
			if (GO2D->ComponentRender.PtrTexture)
			{
				GO2D->ComponentRender.PtrTexture->Use();
				m_cbPS2DFlagsData.bUseTexture = TRUE;
				m_PSBase2D->UpdateConstantBuffer(0);
			}
			else
			{
				m_cbPS2DFlagsData.bUseTexture = FALSE;
				m_PSBase2D->UpdateConstantBuffer(0);
			}
			*/

			Object2D->Draw();
		}
	}

	m_DeviceContext->OMSetDepthStencilState(m_CommonStates->DepthDefault(), 0);
}

void CGame::DrawMiniAxes()
{
	m_DeviceContext->RSSetViewports(1, &m_vViewports[1]);

	for (auto& Object3D : m_vObject3DMiniAxes)
	{
		UpdateObject3D(Object3D.get());
		DrawObject3D(Object3D.get());

		Object3D->ComponentTransform.Translation = 
			m_vCameras[m_CurrentCameraIndex].GetEyePosition() +
			m_vCameras[m_CurrentCameraIndex].GetForward();

		Object3D->UpdateWorldMatrix();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGame::UpdatePickingRay()
{
	m_Object3DLinePickingRay->GetVertices().at(0).Position = m_PickingRayWorldSpaceOrigin;
	m_Object3DLinePickingRay->GetVertices().at(1).Position = m_PickingRayWorldSpaceOrigin + m_PickingRayWorldSpaceDirection * KPickingRayLength;
	m_Object3DLinePickingRay->UpdateVertexBuffer();
}

void CGame::DrawPickingRay()
{
	m_VSLine->Use();
	m_cbVSSpaceData.World = XMMatrixTranspose(KMatrixIdentity);
	m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	m_VSLine->UpdateConstantBuffer(0);

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSLine->Use();

	m_Object3DLinePickingRay->Draw();
}

void CGame::DrawPickedTriangle()
{
	m_VSBase->Use();
	m_cbVSSpaceData.World = XMMatrixTranspose(KMatrixIdentity);
	m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	m_VSBase->UpdateConstantBuffer(0);

	m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
	
	m_PSVertexColor->Use();

	m_Object3DPickedTriangle->GetModel().vMeshes[0].vVertices[0].Position = m_PickedTriangleV0;
	m_Object3DPickedTriangle->GetModel().vMeshes[0].vVertices[1].Position = m_PickedTriangleV1;
	m_Object3DPickedTriangle->GetModel().vMeshes[0].vVertices[2].Position = m_PickedTriangleV2;
	m_Object3DPickedTriangle->UpdateMeshBuffer();

	m_Object3DPickedTriangle->Draw();
}

void CGame::DrawSky(float DeltaTime)
{
	// Elapse SkyTime [0.0f, 1.0f]
	m_cbPSSkyTimeData.SkyTime += KSkyTimeFactorAbsolute * DeltaTime;
	if (m_cbPSSkyTimeData.SkyTime > 1.0f) m_cbPSSkyTimeData.SkyTime = 0.0f;

	// Update directional light source position
	static float DirectionalLightRoll{};
	if (m_cbPSSkyTimeData.SkyTime <= 0.25f)
	{
		DirectionalLightRoll = XM_2PI * (m_cbPSSkyTimeData.SkyTime + 0.25f);
	}
	else if (m_cbPSSkyTimeData.SkyTime > 0.25f && m_cbPSSkyTimeData.SkyTime <= 0.75f)
	{
		DirectionalLightRoll = XM_2PI * (m_cbPSSkyTimeData.SkyTime - 0.25f);
	}
	else
	{
		DirectionalLightRoll = XM_2PI * (m_cbPSSkyTimeData.SkyTime - 0.75f);
	}
	XMVECTOR DirectionalLightSourcePosition{ XMVector3TransformCoord(XMVectorSet(1, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, DirectionalLightRoll)) };
	SetDirectionalLight(DirectionalLightSourcePosition);

	// SkySphere
	{
		m_Object3DSkySphere->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].GetEyePosition();
	}

	// Sun
	{
		float SunRoll{ XM_2PI * m_cbPSSkyTimeData.SkyTime - XM_PIDIV2 };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, SunRoll)) };
		m_Object3DSun->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].GetEyePosition() + Offset;
		m_Object3DSun->ComponentTransform.Roll = SunRoll;
	}

	// Moon
	{
		float MoonRoll{ XM_2PI * m_cbPSSkyTimeData.SkyTime + XM_PIDIV2 };
		XMVECTOR Offset{ XMVector3TransformCoord(XMVectorSet(KSkyDistance, 0, 0, 1), XMMatrixRotationRollPitchYaw(0, 0, MoonRoll)) };
		m_Object3DMoon->ComponentTransform.Translation = m_vCameras[m_CurrentCameraIndex].GetEyePosition() + Offset;
		m_Object3DMoon->ComponentTransform.Roll = (MoonRoll > XM_2PI) ? (MoonRoll - XM_2PI) : MoonRoll;
	}

	UpdateObject3D(m_Object3DSkySphere.get());
	DrawObject3D(m_Object3DSkySphere.get());

	UpdateObject3D(m_Object3DSun.get());
	DrawObject3D(m_Object3DSun.get());

	UpdateObject3D(m_Object3DMoon.get());
	DrawObject3D(m_Object3DMoon.get());
}

void CGame::DrawTerrain()
{
	if (!m_Terrain) return;
	
	SetUniversalRasterizerState();

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::TessellateTerrain))
	{
		m_cbHSCameraData.EyePosition = m_vCameras[m_CurrentCameraIndex].GetEyePosition();
		m_cbHSTessFactor.TessFactor = m_Terrain->GetTerrainTessFactor();
		m_HSTerrain->UpdateAllConstantBuffers();
		m_HSTerrain->Use();

		m_cbDSSpaceData.VP = GetTransposedVPMatrix();
		m_DSTerrain->UpdateAllConstantBuffers();
		m_DSTerrain->Use();

		m_Terrain->Draw(EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals));

		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
	}
	else
	{
		m_Terrain->Draw(EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawNormals));
	}
	
	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainHeightMapTexture))
	{
		m_DeviceContext->RSSetViewports(1, &m_vViewports[2]);
		m_Terrain->DrawHeightMapTexture();
	}

	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::DrawTerrainMaskingTexture))
	{
		m_DeviceContext->RSSetViewports(1, &m_vViewports[3]);
		m_Terrain->DrawMaskingTexture();
	}

	m_DeviceContext->RSSetViewports(1, &m_vViewports[0]);
}

void CGame::Interact3DGizmos()
{
	if (EFLAG_HAS_NO(m_eFlagsRendering, EFlagsRendering::Use3DGizmos)) return;
	
	if (IsAnyObject3DSelected())
	{
		if (m_PtrSelectedObject3D->IsInstanced() && !IsAnyInstanceSelected()) return;
	}
	else
	{
		return;
	}

	const Mouse::State& MouseState{ m_Mouse->GetState() };
	XMVECTOR* pTranslation{ &m_PtrSelectedObject3D->ComponentTransform.Translation };
	XMVECTOR* pScaling{ &m_PtrSelectedObject3D->ComponentTransform.Scaling };
	float* pPitch{ &m_PtrSelectedObject3D->ComponentTransform.Pitch };
	float* pYaw{ &m_PtrSelectedObject3D->ComponentTransform.Yaw };
	float* pRoll{ &m_PtrSelectedObject3D->ComponentTransform.Roll };
	if (m_SelectedInstanceID != -1)
	{
		auto& Instance{ m_PtrSelectedObject3D->GetInstance(m_SelectedInstanceID) };
		pTranslation = &Instance.Translation;
		pScaling = &Instance.Scaling;
		pPitch = &Instance.Pitch;
		pYaw = &Instance.Yaw;
		pRoll = &Instance.Roll;
	}

	if (!MouseState.leftButton)
	{
		m_bIsGizmoSelected = false;
	}

	m_3DGizmoDistanceScalar = XMVectorGetX(XMVector3Length(m_vCameras[m_CurrentCameraIndex].GetEyePosition() - *pTranslation)) * 0.1f;
	m_3DGizmoDistanceScalar = pow(m_3DGizmoDistanceScalar, 0.7f);

	if (m_bIsGizmoSelected)
	{
		// Gizmo is selected.

		int DeltaX{ MouseState.x - m_PrevMouseX };
		int DeltaY{ MouseState.y - m_PrevMouseY };

		int DeltaSum{ DeltaY - DeltaX };

		float TranslationX{ XMVectorGetX(*pTranslation) };
		float TranslationY{ XMVectorGetY(*pTranslation) };
		float TranslationZ{ XMVectorGetZ(*pTranslation) };

		float ScalingX{ XMVectorGetX(*pScaling) };
		float ScalingY{ XMVectorGetY(*pScaling) };
		float ScalingZ{ XMVectorGetZ(*pScaling) };

		switch (m_e3DGizmoMode)
		{
		case E3DGizmoMode::Translation:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pTranslation =
					XMVectorSetX(*pTranslation, TranslationX - DeltaSum * CGame::KTranslationUnit);
				break;
			case E3DGizmoAxis::AxisY:
				*pTranslation =
					XMVectorSetY(*pTranslation, TranslationY - DeltaSum * CGame::KTranslationUnit);
				break;
			case E3DGizmoAxis::AxisZ:
				*pTranslation =
					XMVectorSetZ(*pTranslation, TranslationZ - DeltaSum * CGame::KTranslationUnit);
				break;
			default:
				break;
			}
			break;
		case E3DGizmoMode::Rotation:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pPitch -= DeltaSum * CGame::KRotation360To2PI;
				break;
			case E3DGizmoAxis::AxisY:
				*pYaw -= DeltaSum * CGame::KRotation360To2PI;
				break;
			case E3DGizmoAxis::AxisZ:
				*pRoll -= DeltaSum * CGame::KRotation360To2PI;
				break;
			default:
				break;
			}
			break;
		case E3DGizmoMode::Scaling:
			switch (m_e3DGizmoSelectedAxis)
			{
			case E3DGizmoAxis::AxisX:
				*pScaling =
					XMVectorSetX(*pScaling, ScalingX - DeltaSum * CGame::KScalingUnit);
				break;
			case E3DGizmoAxis::AxisY:
				*pScaling =
					XMVectorSetY(*pScaling, ScalingY - DeltaSum * CGame::KScalingUnit);
				break;
			case E3DGizmoAxis::AxisZ:
				*pScaling =
					XMVectorSetZ(*pScaling, ScalingZ - DeltaSum * CGame::KScalingUnit);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		m_PtrSelectedObject3D->UpdateInstanceWorldMatrix(m_SelectedInstanceID);
	}
	else
	{
		// Gizmo is not selected.

		CastPickingRay();

		const XMVECTOR& BSTransaltion{ m_PtrSelectedObject3D->ComponentPhysics.BoundingSphere.CenterOffset };

		switch (m_e3DGizmoMode)
		{
		case E3DGizmoMode::Translation:
			m_Object3D_3DGizmoTranslationX->ComponentTransform.Translation =
				m_Object3D_3DGizmoTranslationY->ComponentTransform.Translation =
				m_Object3D_3DGizmoTranslationZ->ComponentTransform.Translation = *pTranslation + BSTransaltion;

			m_bIsGizmoSelected = true;
			if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoTranslationX.get(), E3DGizmoAxis::AxisX))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoTranslationY.get(), E3DGizmoAxis::AxisY))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoTranslationZ.get(), E3DGizmoAxis::AxisZ))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
			}
			else
			{
				m_bIsGizmoSelected = false;
			}

			break;
		case E3DGizmoMode::Rotation:
			m_Object3D_3DGizmoRotationPitch->ComponentTransform.Translation =
				m_Object3D_3DGizmoRotationYaw->ComponentTransform.Translation =
				m_Object3D_3DGizmoRotationRoll->ComponentTransform.Translation = *pTranslation + BSTransaltion;

			m_bIsGizmoSelected = true;
			if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationPitch.get(), E3DGizmoAxis::AxisX))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
			}
			else if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationYaw.get(), E3DGizmoAxis::AxisY))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
			}
			else if (ShouldSelectRotationGizmo(m_Object3D_3DGizmoRotationRoll.get(), E3DGizmoAxis::AxisZ))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
			}
			else
			{
				m_bIsGizmoSelected = false;
			}

			break;
		case E3DGizmoMode::Scaling:
			m_Object3D_3DGizmoScalingX->ComponentTransform.Translation =
				m_Object3D_3DGizmoScalingY->ComponentTransform.Translation =
				m_Object3D_3DGizmoScalingZ->ComponentTransform.Translation = *pTranslation + BSTransaltion;

			m_bIsGizmoSelected = true;
			if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoScalingX.get(), E3DGizmoAxis::AxisX))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisX;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoScalingY.get(), E3DGizmoAxis::AxisY))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisY;
			}
			else if (ShouldSelectTranslationScalingGizmo(m_Object3D_3DGizmoScalingZ.get(), E3DGizmoAxis::AxisZ))
			{
				m_e3DGizmoSelectedAxis = E3DGizmoAxis::AxisZ;
			}
			else
			{
				m_bIsGizmoSelected = false;
			}

			break;
		default:
			break;
		}

	}

	m_PrevMouseX = MouseState.x;
	m_PrevMouseY = MouseState.y;
}

bool CGame::ShouldSelectRotationGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis)
{
	XMVECTOR PlaneNormal{};
	switch (Axis)
	{
	case E3DGizmoAxis::None:
		return false;
		break;
	case E3DGizmoAxis::AxisX:
		PlaneNormal = XMVectorSet(1, 0, 0, 0);
		break;
	case E3DGizmoAxis::AxisY:
		PlaneNormal = XMVectorSet(0, 1, 0, 0);
		break;
	case E3DGizmoAxis::AxisZ:
		PlaneNormal = XMVectorSet(0, 0, 1, 0);
		break;
	default:
		break;
	}

	if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection,
		K3DGizmoSelectionRadius * m_3DGizmoDistanceScalar, Gizmo->ComponentTransform.Translation, nullptr))
	{
		XMVECTOR PlaneT{};
		if (IntersectRayPlane(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, Gizmo->ComponentTransform.Translation, PlaneNormal, &PlaneT))
		{
			XMVECTOR PointOnPlane{ m_PickingRayWorldSpaceOrigin + PlaneT * m_PickingRayWorldSpaceDirection };

			float Dist{ XMVectorGetX(XMVector3Length(PointOnPlane - Gizmo->ComponentTransform.Translation)) };
			if (Dist >= K3DGizmoSelectionLowBoundary * m_3DGizmoDistanceScalar && Dist <= K3DGizmoSelectionHighBoundary * m_3DGizmoDistanceScalar) return true;
		}
	}
	return false;
}

bool CGame::ShouldSelectTranslationScalingGizmo(const CObject3D* const Gizmo, E3DGizmoAxis Axis)
{
	XMVECTOR Center{};
	switch (Axis)
	{
	case E3DGizmoAxis::None:
		return false;
		break;
	case E3DGizmoAxis::AxisX:
		Center = Gizmo->ComponentTransform.Translation + XMVectorSet(0.5f * m_3DGizmoDistanceScalar, 0, 0, 1);
		break;
	case E3DGizmoAxis::AxisY:
		Center = Gizmo->ComponentTransform.Translation + XMVectorSet(0, 0.5f * m_3DGizmoDistanceScalar, 0, 1);
		break;
	case E3DGizmoAxis::AxisZ:
		Center = Gizmo->ComponentTransform.Translation + XMVectorSet(0, 0, 0.5f * m_3DGizmoDistanceScalar, 1);
		break;
	default:
		break;
	}

	if (IntersectRaySphere(m_PickingRayWorldSpaceOrigin, m_PickingRayWorldSpaceDirection, 0.5f * m_3DGizmoDistanceScalar, Center, nullptr)) return true;

	return false;
}

void CGame::Draw3DGizmos()
{
	if (IsAnyObject3DSelected())
	{
		if (m_PtrSelectedObject3D->IsInstanced() && !IsAnyInstanceSelected()) return;
	}
	else
	{
		return;
	}

	switch (m_e3DGizmoMode)
	{
	case E3DGizmoMode::Translation:
		Draw3DGizmoTranslations(m_e3DGizmoSelectedAxis);
		break;
	case E3DGizmoMode::Rotation:
		Draw3DGizmoRotations(m_e3DGizmoSelectedAxis);
		break;
	case E3DGizmoMode::Scaling:
		Draw3DGizmoScalings(m_e3DGizmoSelectedAxis);
		break;
	default:
		break;
	}
}

void CGame::Draw3DGizmoRotations(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

	switch (Axis)
	{
	case E3DGizmoAxis::AxisX:
		bHighlightX = true;
		break;
	case E3DGizmoAxis::AxisY:
		bHighlightY = true;
		break;
	case E3DGizmoAxis::AxisZ:
		bHighlightZ = true;
		break;
	default:
		break;
	}

	Draw3DGizmo(m_Object3D_3DGizmoRotationPitch.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoRotationYaw.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoRotationRoll.get(), bHighlightZ);
}

void CGame::Draw3DGizmoTranslations(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

	switch (Axis)
	{
	case E3DGizmoAxis::AxisX:
		bHighlightX = true;
		break;
	case E3DGizmoAxis::AxisY:
		bHighlightY = true;
		break;
	case E3DGizmoAxis::AxisZ:
		bHighlightZ = true;
		break;
	default:
		break;
	}

	Draw3DGizmo(m_Object3D_3DGizmoTranslationX.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoTranslationY.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoTranslationZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmoScalings(E3DGizmoAxis Axis)
{
	bool bHighlightX{ false };
	bool bHighlightY{ false };
	bool bHighlightZ{ false };

	switch (Axis)
	{
	case E3DGizmoAxis::AxisX:
		bHighlightX = true;
		break;
	case E3DGizmoAxis::AxisY:
		bHighlightY = true;
		break;
	case E3DGizmoAxis::AxisZ:
		bHighlightZ = true;
		break;
	default:
		break;
	}

	Draw3DGizmo(m_Object3D_3DGizmoScalingX.get(), bHighlightX);
	Draw3DGizmo(m_Object3D_3DGizmoScalingY.get(), bHighlightY);
	Draw3DGizmo(m_Object3D_3DGizmoScalingZ.get(), bHighlightZ);
}

void CGame::Draw3DGizmo(CObject3D* const Gizmo, bool bShouldHighlight)
{
	CShader* VS{ Gizmo->ComponentRender.PtrVS };
	CShader* PS{ Gizmo->ComponentRender.PtrPS };

	float Scalar{ XMVectorGetX(XMVector3Length(m_vCameras[m_CurrentCameraIndex].GetEyePosition() - Gizmo->ComponentTransform.Translation)) * 0.1f };
	Scalar = pow(Scalar, 0.7f);

	Gizmo->ComponentTransform.Scaling = XMVectorSet(Scalar, Scalar, Scalar, 0.0f);
	Gizmo->UpdateWorldMatrix();
	m_cbVSSpaceData.World = XMMatrixTranspose(Gizmo->ComponentTransform.MatrixWorld);
	m_cbVSSpaceData.ViewProjection = XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
	VS->UpdateConstantBuffer(0);
	VS->Use();

	if (bShouldHighlight)
	{
		m_cbPSGizmoColorFactorData.ColorFactor = XMVectorSet(1.0f, 1.0f, 1.0f, 0.95f);
	}
	else
	{
		m_cbPSGizmoColorFactorData.ColorFactor = XMVectorSet(0.75f, 0.75f, 0.75f, 0.75f);
	}
	PS->UpdateConstantBuffer(0);
	PS->Use();

	Gizmo->Draw();
}

void CGame::SetUniversalRasterizerState()
{
	switch (m_eRasterizerState)
	{
	case ERasterizerState::CullNone:
		m_DeviceContext->RSSetState(m_CommonStates->CullNone());
		break;
	case ERasterizerState::CullClockwise:
		m_DeviceContext->RSSetState(m_CommonStates->CullClockwise());
		break;
	case ERasterizerState::CullCounterClockwise:
		m_DeviceContext->RSSetState(m_CommonStates->CullCounterClockwise());
		break;
	case ERasterizerState::WireFrame:
		m_DeviceContext->RSSetState(m_CommonStates->Wireframe());
		break;
	default:
		break;
	}
}

void CGame::SetUniversalbUseLighiting()
{
	if (EFLAG_HAS(m_eFlagsRendering, EFlagsRendering::UseLighting))
	{
		m_cbPSBaseFlagsData.bUseLighting = TRUE;
	}
}

void CGame::EndRendering()
{
	m_SwapChain->Present(0, 0);
}

Keyboard::State CGame::GetKeyState() const
{
	return m_Keyboard->GetState();
}

Mouse::State CGame::GetMouseState() const
{ 
	Mouse::State ResultState{ m_Mouse->GetState() };
	
	m_Mouse->ResetScrollWheelValue();

	return ResultState;
}

const XMFLOAT2& CGame::GetWindowSize() const
{
	return m_WindowSize;
}

const XMFLOAT2& CGame::GetTerrainSelectionPosition() const
{
	assert(m_Terrain);
	return m_Terrain->GetSelectionPosition();
}

float CGame::GetSkyTime() const
{
	return m_cbPSSkyTimeData.SkyTime;
}

XMMATRIX CGame::GetTransposedVPMatrix() const
{
	return XMMatrixTranspose(m_MatrixView * m_MatrixProjection);
}
