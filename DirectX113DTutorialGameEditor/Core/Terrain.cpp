#include "Terrain.h"
#include "PrimitiveGenerator.h"
#include "ModelPorter.h"
#include "Math.h"
#include "Game.h"

// ###########################
// << TERR FILE STRUCTURE >> : << SMOD FILE STRUCTURE >>
// 8B TERR Signature
// 4B (float) Terrain X Size
// 4B (float) Terrain Z Size
// 4B (float) Masking detail
// 4B (uint32_t) Masking texture raw data size
// ### Masking texture Raw data ###
// 16B (XMFLAOT4) RGBA  ( multiplied by Masking texture raw data size )
// ...
// << SMOD FILE STRUCTURE>>
// ...
// ###########################

void CTerrain::Create(const XMFLOAT2& TerrainSize, const CMaterial& Material, int MaskingDetail, float UniformScaling)
{
	m_Size = TerrainSize;
	m_MaskingTextureDetail = max(min(MaskingDetail, KMaskingMaxDetail), KMaskingMinDetail);

	m_CBTerrainData.TerrainSizeX = m_Size.x;
	m_CBTerrainData.TerrainSizeZ = m_Size.y;
	m_CBTerrainData.TerrainHeightRange = KHeightRange;
	m_PtrGame->UpdateCBTerrainData(m_CBTerrainData);

	vector<CMaterial> vMaterials{};
	vMaterials.emplace_back(Material);
	vMaterials.back().ShouldGenerateAutoMipMap(true); // @important

	CreateTerrainObject3D(vMaterials);

	m_Object2DTextureRepresentation = make_unique<CObject2D>("TextureRepresentation", m_PtrDevice, m_PtrDeviceContext);
	m_Object2DTextureRepresentation->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	CreateHeightMapTexture(true);
	CreateMaskingTexture(true);

	CreateWater();

	Scale(XMVectorSet(UniformScaling, UniformScaling, UniformScaling, 0));
}

void CTerrain::Load(const string& FileName)
{
	m_FileName = FileName;

	SModel Model{};
	
	std::ifstream ifs{};
	ifs.open(FileName, std::ofstream::binary);
	assert(ifs.is_open());

	char ReadBytes[512]{};

	// 8B Signature
	READ_BYTES(8);

	// 4B (float) Terrain X Size
	READ_BYTES(4);
	m_Size.x = READ_BYTES_TO_FLOAT;
	m_CBTerrainData.TerrainSizeX = m_Size.x;
	
	// 4B (float) Terrain Z Size
	READ_BYTES(4);
	m_Size.y = READ_BYTES_TO_FLOAT;
	m_CBTerrainData.TerrainSizeZ = m_Size.y;

	// 4B (float) Terrain Height Range
	READ_BYTES(4);
	m_CBTerrainData.TerrainHeightRange = READ_BYTES_TO_FLOAT;
	m_PtrGame->UpdateCBTerrainData(m_CBTerrainData);

	// 4B (float) Terrain tessellation factor
	READ_BYTES(4);
	m_TerrainTessFactor = READ_BYTES_TO_FLOAT;

	// 1B (bool) Should draw water
	READ_BYTES(1);
	m_bShouldDrawWater = (ReadBytes[0] != 0) ? true : false;

	// 4B (float) Water height
	READ_BYTES(4);
	m_WaterHeight = READ_BYTES_TO_FLOAT;

	// 4B (float) Water tessellation factor
	READ_BYTES(4);
	m_WaterTessFactor = READ_BYTES_TO_FLOAT;

	// 4B (float) Masking detail
	READ_BYTES(4);
	m_MaskingTextureDetail = READ_BYTES_TO_INT32;

	// 4B (uint32_t) Masking texture raw data size
	READ_BYTES(4);
	m_MaskingTextureRawData.resize(READ_BYTES_TO_UINT32);

	// Masking texture raw data
	for (SPixel32UInt& Pixel : m_MaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		READ_BYTES(4);

		Pixel.R = ReadBytes[0];
		Pixel.G = ReadBytes[1];
		Pixel.B = ReadBytes[2];
		Pixel.A = ReadBytes[3];
	}

	// 4B (uint32_t) HeightMap texture raw data size
	READ_BYTES(4);
	m_HeightMapTextureRawData.resize(READ_BYTES_TO_UINT32);

	// HeightMap texture raw data
	for (SPixel8UInt& Pixel : m_HeightMapTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		READ_BYTES(1);

		Pixel.R = ReadBytes[0];
	}

	// 1B (uint8_t) Material count
	READ_BYTES(1);
	Model.vMaterials.resize(READ_BYTES_TO_UINT32);

	// Material data
	_ReadModelMaterials(ifs, Model.vMaterials);

	// @important
	for (size_t iMaterial = 0; iMaterial < Model.vMaterials.size(); ++iMaterial)
	{
		Model.vMaterials[iMaterial].SetIndex(iMaterial);
	}

	CreateTerrainObject3D(Model.vMaterials);

	m_Object2DTextureRepresentation = make_unique<CObject2D>("TextureRepresentation", m_PtrDevice, m_PtrDeviceContext);
	m_Object2DTextureRepresentation->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	CreateHeightMapTexture(false);
	CreateMaskingTexture(false);

	CreateWater();
}

void CTerrain::Save(const string& FileName)
{
	if (!m_Object3DTerrain) return;

	m_FileName = FileName;

	std::ofstream ofs{};
	ofs.open(FileName, std::ofstream::binary);
	assert(ofs.is_open());

	char BoolByte{};
	char FloatBytes[4]{};
	char Int32Bytes[4]{};
	char Uint32Bytes[4]{};
	char XMFLOAT4Bytes[16]{};

	// 8B Signature
	ofs.write("TERR_KJW", 8);

	// 4B (float) Terrain X Size
	WRITE_FLOAT_TO_BYTES(m_Size.x);

	// 4B (float) Terrain Z Size
	WRITE_FLOAT_TO_BYTES(m_Size.y);

	// 4B (float) Terrain Height Range
	WRITE_FLOAT_TO_BYTES(m_CBTerrainData.TerrainHeightRange);

	// 4B (float) Terrain tessellation factor
	WRITE_FLOAT_TO_BYTES(m_TerrainTessFactor);

	// 1B (bool) Should draw water
	WRITE_BOOL_TO_BYTE(m_bShouldDrawWater);
	
	// 4B (float) Water height
	WRITE_FLOAT_TO_BYTES(m_WaterHeight);

	// 4B (float) Water tessellation factor
	WRITE_FLOAT_TO_BYTES(m_WaterTessFactor);

	// 4B (float) Masking detail
	WRITE_INT32_TO_BYTES(m_MaskingTextureDetail);

	// 4B (uint32_t) Masking texture raw data size
	WRITE_UINT32_TO_BYTES(m_MaskingTextureRawData.size());

	// Masking texture raw data
	for (const SPixel32UInt& Pixel : m_MaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		ofs.write((const char*)&Pixel.R, 1);
		ofs.write((const char*)&Pixel.G, 1);
		ofs.write((const char*)&Pixel.B, 1);
		ofs.write((const char*)&Pixel.A, 1);
	}

	// 4B (uint32_t) HeightMap texture raw data size
	WRITE_UINT32_TO_BYTES(m_HeightMapTextureRawData.size());

	// HeightMap texture raw data
	for (SPixel8UInt& Pixel : m_HeightMapTextureRawData)
	{
		// 1B (uint8_t) R (UNORM)
		ofs.write((const char*)&Pixel.R, 1);
	}

	// 1B (uint8_t) Material count
	ofs.put((uint8_t)m_Object3DTerrain->GetModel().vMaterials.size());

	_WriteModelMaterials(ofs, m_Object3DTerrain->GetModel().vMaterials);

	ofs.close();
}

void CTerrain::Scale(const XMVECTOR& Scaling)
{
	if (m_Object3DTerrain) 
	{
		m_Object3DTerrain->ComponentTransform.Scaling = Scaling;
		m_Object3DTerrain->UpdateWorldMatrix();
	}

	if (m_Object3DWater)
	{
		m_Object3DWater->ComponentTransform.Scaling = Scaling;
		m_Object3DWater->UpdateWorldMatrix();
	}
}

void CTerrain::CreateFoliageCluster(const vector<string>& vFoliageFileNames, int PlacingDetail)
{
	srand((unsigned int)GetTickCount64());

	m_FoliagePlacingDetail = PlacingDetail;

	CreateFoliagePlaceTexutre();
	
	if (!m_PerlinNoiseTexture)
	{
		m_PerlinNoiseTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);
		m_PerlinNoiseTexture->CreateTextureFromFile("Asset\\perlin_noise.jpg", false);
		m_PerlinNoiseTexture->SetShaderType(EShaderType::VertexShader);
		m_PerlinNoiseTexture->SetSlot(1);
	}

	for (const auto& FoliageFileName : vFoliageFileNames)
	{
		m_vFoliages.emplace_back(make_unique<CObject3D>("Foliage", m_PtrDevice, m_PtrDeviceContext, m_PtrGame));
		m_vFoliages.back()->CreateFromFile(FoliageFileName, false);
	}

	m_bHasFoliageCluster = true;

	CreateWindRepresentation();
}

void CTerrain::SetFoliageDensity(float Density)
{
	m_FoliageDenstiy = Density;
}

float CTerrain::GetFoliageDenstiy() const
{
	return m_FoliageDenstiy;
}

void CTerrain::SetWindVelocity(const XMFLOAT3& Velocity)
{
	XMVECTOR xmvVelocity{ XMLoadFloat3(&Velocity) };
	m_CBWindData.Velocity = xmvVelocity;
	m_PtrGame->UpdateCBWindData(m_CBWindData);
}

void CTerrain::SetWindVelocity(const XMVECTOR& Velocity)
{
	m_CBWindData.Velocity = Velocity;
	m_PtrGame->UpdateCBWindData(m_CBWindData);
}

const XMVECTOR& CTerrain::GetWindVelocity() const
{
	return m_CBWindData.Velocity;
}

void CTerrain::SetWindRadius(float Radius)
{
	Radius = max(Radius, KMinWindRadius);
	m_CBWindData.Radius = Radius;
}

float CTerrain::GetWindRadius() const
{
	return m_CBWindData.Radius;
}

void CTerrain::CreateTerrainObject3D(vector<CMaterial>& vMaterials)
{
	SModel Model{};
	assert(vMaterials.size());

	Model.vMeshes.clear();
	Model.vMeshes.emplace_back(GenerateTerrainBase(m_Size));
	Model.vMaterials = vMaterials;
	Model.bUseMultipleTexturesInSingleMesh = true; // @important

	m_Object3DTerrain = make_unique<CObject3D>("Terrain", m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3DTerrain->Create(Model);
	m_Object3DTerrain->ShouldTessellate(true); // @important
}

void CTerrain::CreateHeightMapTexture(bool bShouldClear)
{
	m_HeightMapTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

	m_HeightMapTextureSize = m_Size;
	m_HeightMapTextureSize.x += 1.0f;
	m_HeightMapTextureSize.y += 1.0f;
	m_HeightMapTexture->CreateBlankTexture(CMaterial::CTexture::EFormat::Pixel8Int, m_HeightMapTextureSize);
	m_HeightMapTexture->SetSlot(0);
	m_HeightMapTexture->SetShaderType(EShaderType::VertexShader);
	m_HeightMapTexture->Use();

	if (bShouldClear)
	{
		m_HeightMapTextureRawData.clear();
		m_HeightMapTextureRawData.resize(static_cast<size_t>(m_HeightMapTextureSize.x)* static_cast<size_t>(m_HeightMapTextureSize.y));
		for (auto& Data : m_HeightMapTextureRawData)
		{
			Data.R = 127;
		}
	}

	// Update HeightMap Texture
	m_HeightMapTexture->UpdateTextureRawData(&m_HeightMapTextureRawData[0]);
}

void CTerrain::CreateMaskingTexture(bool bShouldClear)
{
	m_MaskingTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

	m_MaskingTextureSize.x = m_Size.x * m_MaskingTextureDetail;
	m_MaskingTextureSize.y = m_Size.y * m_MaskingTextureDetail;
	m_MaskingTexture->CreateBlankTexture(CMaterial::CTexture::EFormat::Pixel32Int, m_MaskingTextureSize);
	m_MaskingTexture->SetSlot(KMaskingTextureSlot);
	m_MaskingTexture->Use();

	if (bShouldClear)
	{
		m_MaskingTextureRawData.clear();
		m_MaskingTextureRawData.resize(static_cast<size_t>(m_MaskingTextureSize.x) * static_cast<size_t>(m_MaskingTextureSize.y));
	}

	// Update Masking Texture
	m_MaskingTexture->UpdateTextureRawData(&m_MaskingTextureRawData[0]);

	XMMATRIX Translation{ XMMatrixTranslation(m_Size.x / 2.0f, 0, m_Size.y / 2.0f) };
	XMMATRIX Scaling{ XMMatrixScaling(1 / m_Size.x, 1.0f, 1 / m_Size.y) };
	m_MatrixMaskingSpace = Translation * Scaling;
	m_PtrGame->UpdateCBTerrainMaskingSpace(m_MatrixMaskingSpace);
}

void CTerrain::CreateWater()
{
	static constexpr XMVECTOR KWaterColor{ 0.0f, 0.5f, 0.75f, 0.8125f };

	m_Object3DWater = make_unique<CObject3D>("Water", m_PtrDevice, m_PtrDeviceContext, m_PtrGame);

	SMesh WaterMesh{ GenerateTerrainBase(m_Size, false, KWaterColor) };
	m_Object3DWater->Create(WaterMesh);
	m_Object3DWater->ShouldTessellate(true);

	m_WaterNormalTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_WaterNormalTexture->CreateTextureFromFile("Asset\\water_normal.jpg", false);
	m_WaterNormalTexture->SetSlot(0);
	m_WaterNormalTexture->Use();

	m_WaterDisplacementTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_WaterDisplacementTexture->CreateTextureFromFile("Asset\\water_displacement.jpg", false);
	m_WaterDisplacementTexture->SetSlot(0);
	m_WaterDisplacementTexture->SetShaderType(EShaderType::DomainShader);
	m_WaterDisplacementTexture->Use();
}

void CTerrain::CreateFoliagePlaceTexutre()
{
	m_FoliagePlaceTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

	m_FoliagePlaceTextureSize.x = static_cast<float>(m_Size.x * m_FoliagePlacingDetail);
	m_FoliagePlaceTextureSize.y = static_cast<float>(m_Size.y * m_FoliagePlacingDetail);
	m_FoliagePlaceTexture->CreateBlankTexture(CMaterial::CTexture::EFormat::Pixel8Int, m_FoliagePlaceTextureSize);

	m_FoliagePlaceTextureRawData.clear();
	m_FoliagePlaceTextureRawData.resize(static_cast<size_t>(m_FoliagePlaceTextureSize.x) * static_cast<size_t>(m_FoliagePlaceTextureSize.y));
}

void CTerrain::CreateWindRepresentation()
{
	m_Object3DWindRepresentation = make_unique<CObject3D>("WindRepr", m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3DWindRepresentation->Create(GenerateSphere());
}

void CTerrain::AddMaterial(const CMaterial& Material)
{
	assert(m_Object3DTerrain);
	if ((int)m_Object3DTerrain->GetModel().vMaterials.size() == KMaterialMaxCount) return;

	m_Object3DTerrain->AddMaterial(Material);
}

void CTerrain::SetMaterial(int MaterialID, const CMaterial& NewMaterial)
{
	assert(m_Object3DTerrain);
	assert(MaterialID >= 0 && MaterialID < 4);

	m_Object3DTerrain->SetMaterial(MaterialID, NewMaterial);
}

const CMaterial& CTerrain::GetMaterial(int Index) const
{
	assert(m_Object3DTerrain);
	return m_Object3DTerrain->GetModel().vMaterials[Index];
}

void CTerrain::Select(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, bool bShouldEdit, bool bIsLeftButton)
{
	if (!m_Object3DTerrain) return;

	UpdateSelectionPosition(PickingRayOrigin, PickingRayDirection);

	if (bShouldEdit)
	{
		if (m_eEditMode == EEditMode::Masking)
		{
			if (bIsLeftButton)
			{
				UpdateMasking(m_eMaskingLayer, m_MaskingRatio);
			}
			else
			{
				UpdateMasking(m_eMaskingLayer, 0.0f, true);
			}
		}
		else if (m_eEditMode == EEditMode::FoliagePlacing)
		{
			UpdateFoliagePlacing(!bIsLeftButton);
		}
		else
		{
			UpdateHeights(bIsLeftButton);
		}
	}
}

void CTerrain::UpdateSelectionPosition(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection)
{
	XMVECTOR PlaneT{};
	if (IntersectRayPlane(PickingRayOrigin, PickingRayDirection, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0), &PlaneT))
	{
		const XMFLOAT2 KTerrainHalfSize{ m_Size.x / 2.0f, m_Size.y / 2.0f };
		const XMVECTOR KPointOnPlane{ PickingRayOrigin + PickingRayDirection * PlaneT };

		m_CBTerrainSelectionData.Position.x = XMVectorGetX(KPointOnPlane);
		m_CBTerrainSelectionData.Position.y = XMVectorGetZ(KPointOnPlane);
	}
}

void CTerrain::UpdateHeights(bool bIsLeftButton)
{
	const XMMATRIX KInverseWorld{ XMMatrixInverse(nullptr, m_CBTerrainSelectionData.TerrainWorld) };
	const float KLocalSelectionRadius{ m_CBTerrainSelectionData.SelectionRadius / XMVectorGetX(m_Object3DTerrain->ComponentTransform.Scaling) };
	const float KLocalRadiusSquare{ KLocalSelectionRadius * KLocalSelectionRadius };
	const int KTerrainSizeX{ (int)m_Size.x };
	const int KTerrainSizeZ{ (int)m_Size.y };
	
	XMVECTOR LocalSelectionPosition{ m_CBTerrainSelectionData.Position.x, 0, m_CBTerrainSelectionData.Position.y, 1 };
	LocalSelectionPosition = XMVector3TransformCoord(LocalSelectionPosition, KInverseWorld);
	const int KCenterU{ (int)(XMVectorGetX(LocalSelectionPosition) + 0.5f) + KTerrainSizeX / 2 };
	const int KCenterV{ (int)(-XMVectorGetZ(LocalSelectionPosition) + 0.5f) + KTerrainSizeZ / 2 };

	for (int iPixel = 0; iPixel < (int)m_HeightMapTextureRawData.size(); ++iPixel)
	{
		int U{ iPixel % (int)m_HeightMapTextureSize.x };
		int V{ iPixel / (int)m_HeightMapTextureSize.x };

		float dU{ float(U - KCenterU) };
		float dV{ float(V - KCenterV) };
		float DistanceSquare{ dU * dU + dV * dV };
		if (DistanceSquare <= KLocalRadiusSquare)
		{
			switch (m_eEditMode)
			{
			case EEditMode::SetHeight:
			{
				float NewY{ (m_SetHeightValue + KHeightRangeHalf) / KHeightRange };
				NewY = min(max(NewY, 0.0f), 1.0f);

				m_HeightMapTextureRawData[iPixel].R = static_cast<uint8_t>(NewY * 255);
			} break;
			case EEditMode::DeltaHeight:
			{
				if (bIsLeftButton)
				{
					int NewY{ m_HeightMapTextureRawData[iPixel].R + 1 };
					NewY = min(NewY, 255);
					m_HeightMapTextureRawData[iPixel].R = static_cast<uint8_t>(NewY);
				}
				else
				{
					int NewY{ m_HeightMapTextureRawData[iPixel].R - 1 };
					NewY = max(NewY, 0);
					m_HeightMapTextureRawData[iPixel].R = static_cast<uint8_t>(NewY);
				}
			} break;
			default:
			{

			} break;
			}
		}
	}

	// Update HeightMap Texture
	m_HeightMapTexture->UpdateTextureRawData(&m_HeightMapTextureRawData[0]);
}

void CTerrain::UpdateMasking(EMaskingLayer eLayer, float Value, bool bForceSet)
{
	const XMMATRIX KInverseWorld{ XMMatrixInverse(nullptr, m_CBTerrainSelectionData.TerrainWorld) };
	const float KLocalSelectionRadius{ m_CBTerrainSelectionData.SelectionRadius / XMVectorGetX(m_Object3DTerrain->ComponentTransform.Scaling) };
	const float KDetailSquare{ (float)m_MaskingTextureDetail * (float)m_MaskingTextureDetail };
	const float KLocalRadiusSquare{ KLocalSelectionRadius * KLocalSelectionRadius * KDetailSquare };
	const int KTerrainSizeX{ (int)m_Size.x };
	const int KTerrainSizeZ{ (int)m_Size.y };

	XMVECTOR LocalSelectionPosition{ m_CBTerrainSelectionData.Position.x, 0, m_CBTerrainSelectionData.Position.y, 1 };
	LocalSelectionPosition = XMVector3TransformCoord(LocalSelectionPosition, KInverseWorld);
	const int KCenterU{ static_cast<int>((+m_Size.x / 2.0f + XMVectorGetX(LocalSelectionPosition)) * m_MaskingTextureDetail) };
	const int KCenterV{ static_cast<int>(-(-m_Size.y / 2.0f + XMVectorGetZ(LocalSelectionPosition)) * m_MaskingTextureDetail) };

	for (int iPixel = 0; iPixel < (int)m_MaskingTextureRawData.size(); ++iPixel)
	{
		int U{ iPixel % (int)m_MaskingTextureSize.x };
		int V{ iPixel / (int)m_MaskingTextureSize.x };

		float dU{ float(U - KCenterU) };
		float dV{ float(V - KCenterV) };
		float DistanceSquare{ dU * dU + dV * dV };
		if (DistanceSquare <= KLocalRadiusSquare)
		{
			float Factor{ 1.0f -
				(sqrt(DistanceSquare / KDetailSquare) / KLocalSelectionRadius) * m_MaskingAttenuation - // Distance attenuation
				((DistanceSquare / KDetailSquare) / KLocalSelectionRadius) * m_MaskingAttenuation }; // Distance square attenuation
			Factor = max(Factor, 0.0f);
			Factor = min(Factor, 1.0f);

			if (bForceSet)
			{
				switch (eLayer)
				{
				case EMaskingLayer::LayerR:
					m_MaskingTextureRawData[iPixel].R = static_cast<uint8_t>(Value * Factor * 255.0f);
					break;
				case EMaskingLayer::LayerG:
					m_MaskingTextureRawData[iPixel].G = static_cast<uint8_t>(Value * Factor * 255.0f);
					break;
				case EMaskingLayer::LayerB:
					m_MaskingTextureRawData[iPixel].B = static_cast<uint8_t>(Value * Factor * 255.0f);
					break;
				case EMaskingLayer::LayerA:
					m_MaskingTextureRawData[iPixel].A = static_cast<uint8_t>(Value * Factor * 255.0f);
					break;
				default:
					break;
				}
			}
			else
			{
				switch (eLayer)
				{
				case EMaskingLayer::LayerR:
					m_MaskingTextureRawData[iPixel].R = max(m_MaskingTextureRawData[iPixel].R, static_cast<uint8_t>(Value * Factor * 255.0f));
					break;
				case EMaskingLayer::LayerG:
					m_MaskingTextureRawData[iPixel].G = max(m_MaskingTextureRawData[iPixel].G, static_cast<uint8_t>(Value * Factor * 255.0f));
					break;
				case EMaskingLayer::LayerB:
					m_MaskingTextureRawData[iPixel].B = max(m_MaskingTextureRawData[iPixel].B, static_cast<uint8_t>(Value * Factor * 255.0f));
					break;
				case EMaskingLayer::LayerA:
					m_MaskingTextureRawData[iPixel].A = max(m_MaskingTextureRawData[iPixel].A, static_cast<uint8_t>(Value * Factor * 255.0f));
					break;
				default:
					break;
				}
			}
		}
	}

	// Update Masking Texture
	m_MaskingTexture->UpdateTextureRawData(&m_MaskingTextureRawData[0]);
}

void CTerrain::UpdateFoliagePlacing(bool bErase)
{
	if (!m_FoliagePlaceTexture) return;

	const XMMATRIX KInverseWorld{ XMMatrixInverse(nullptr, m_CBTerrainSelectionData.TerrainWorld) };
	const float KScalingX{ XMVectorGetX(m_Object3DTerrain->ComponentTransform.Scaling) };
	const float KScalingZ{ XMVectorGetX(m_Object3DTerrain->ComponentTransform.Scaling) };
	const float KLocalSelectionRadius{ m_CBTerrainSelectionData.SelectionRadius / KScalingX };
	const float KDetailSquare{ (float)m_FoliagePlacingDetail * (float)m_FoliagePlacingDetail };
	const float KLocalRadiusSquare{ KLocalSelectionRadius * KLocalSelectionRadius * KDetailSquare };
	const int KTerrainSizeX{ (int)m_Size.x };
	const int KTerrainSizeZ{ (int)m_Size.y };

	XMVECTOR LocalSelectionPosition{ m_CBTerrainSelectionData.Position.x, 0, m_CBTerrainSelectionData.Position.y, 1 };
	LocalSelectionPosition = XMVector3TransformCoord(LocalSelectionPosition, KInverseWorld);
	const int KCenterU{ static_cast<int>((+m_Size.x / 2.0f + XMVectorGetX(LocalSelectionPosition)) * m_FoliagePlacingDetail) };
	const int KCenterV{ static_cast<int>(-(-m_Size.y / 2.0f + XMVectorGetZ(LocalSelectionPosition)) * m_FoliagePlacingDetail) };

	for (int iPixel = 0; iPixel < (int)m_FoliagePlaceTextureRawData.size(); ++iPixel)
	{
		int U{ iPixel % (int)(m_FoliagePlaceTextureSize.x) };
		int V{ iPixel / (int)(m_FoliagePlaceTextureSize.x) };

		float dU{ float(U - KCenterU) };
		float dV{ float(V - KCenterV) };
		float DistanceSquare{ dU * dU + dV * dV };
		if (DistanceSquare <= KLocalRadiusSquare)
		{
			if (bErase)
			{
				// 지우기
				if (m_FoliagePlaceTextureRawData[iPixel].R == 255)
				{
					m_FoliagePlaceTextureRawData[iPixel].R = 0;

					const string KInstanceName{ "Foliage" + to_string(U) + "." + to_string(V) };
					for (auto& Foliage : m_vFoliages)
					{
						Foliage->DeleteInstance(KInstanceName);
					}
				}
			}
			else
			{
				// 그리기
				float InverseDenstiy{ 1.0f - m_FoliageDenstiy };
				float Exponent{ GetRandom(6.0f, 8.0f) };
				int DenstiyModular{ iPixel % (int)(pow(InverseDenstiy + 1.0f, Exponent)) };
				if (m_FoliagePlaceTextureRawData[iPixel].R != 255 && DenstiyModular == 0)
				{
					m_FoliagePlaceTextureRawData[iPixel].R = 255;

					const string KInstanceName{ "Foliage" + to_string(U) + "." + to_string(V) };
					const float Interval{ 1.0f / (float)m_FoliagePlacingDetail };
					for (auto& Foliage : m_vFoliages)
					{
						Foliage->InsertInstance(KInstanceName);

						int iInstance{ Foliage->GetInstanceCount() - 1 };
						auto& Instance{ Foliage->GetInstance(iInstance) };

						float XDisplacement{ GetRandom(-0.2f, +0.2f) };
						float YDisplacement{ GetRandom(-0.1f, 0.0f) };
						float ZDisplacement{ GetRandom(-0.2f, +0.2f) };
						Instance.Translation =
							XMVectorSet
							(
								XDisplacement + (U - (int)(m_FoliagePlaceTextureSize.x * 0.5f)) * KScalingX * Interval,
								YDisplacement,
								ZDisplacement - (V - (int)(m_FoliagePlaceTextureSize.y * 0.5f)) * KScalingZ * Interval,
								1
							);

						float YRotationAngle{ GetRandom(0.0f, XM_2PI) };
						Instance.Yaw = YRotationAngle;

						Foliage->UpdateInstanceWorldMatrix(iInstance);
					}
				}
			}
		}
	}
	
	// Update Foliage Place Texture
	m_FoliagePlaceTexture->UpdateTextureRawData(&m_FoliagePlaceTextureRawData[0]);
}

void CTerrain::SetMaskingLayer(EMaskingLayer eLayer)
{
	m_eMaskingLayer = eLayer;
}

CTerrain::EMaskingLayer CTerrain::GetMaskingLayer() const
{
	return m_eMaskingLayer;
}

void CTerrain::SetMaskingAttenuation(float Attenuation)
{
	m_MaskingAttenuation = Attenuation;
}

float CTerrain::GetMaskingAttenuation() const
{
	return m_MaskingAttenuation;
}

void CTerrain::SetSelectionRadius(float Radius)
{
	Radius = max(min(Radius, KMaxSelectionRadius), KMinSelectionRadius);
	
	m_CBTerrainSelectionData.SelectionRadius = Radius;
}

float CTerrain::GetSelectionRadius() const
{
	return m_CBTerrainSelectionData.SelectionRadius;
}

void CTerrain::SetSetHeightValue(float Value)
{
	m_SetHeightValue = Value;
}

float CTerrain::GetSetHeightValue() const
{
	return m_SetHeightValue;
}

void CTerrain::SetDeltaHeightValue(float Value)
{
	m_DeltaHeightValue = Value;
}

float CTerrain::GetDeltaHeightValue() const
{
	return m_DeltaHeightValue;
}

void CTerrain::SetMaskingRatio(float Value)
{
	m_MaskingRatio = Value;
}

float CTerrain::GetMaskingRatio() const
{
	return m_MaskingRatio;
}

void CTerrain::ShouldTessellate(bool Value)
{
	if (m_Object3DTerrain) m_Object3DTerrain->ShouldTessellate(Value);
}

bool CTerrain::ShouldTessellate() const
{
	if (m_Object3DTerrain) return m_Object3DTerrain->ShouldTessellate();
	return false;
}

void CTerrain::ShouldDrawWater(bool Value)
{
	m_bShouldDrawWater = Value;
}

bool CTerrain::ShouldDrawWater() const
{
	return m_bShouldDrawWater;
}

void CTerrain::ShouldShowSelection(bool Value)
{
	m_CBTerrainSelectionData.bShowSelection = ((Value == true) ? TRUE : FALSE);
}

void CTerrain::SetEditMode(EEditMode Mode)
{
	m_eEditMode = Mode;
}

CTerrain::EEditMode CTerrain::GetEditMode()
{
	return m_eEditMode;
}

void CTerrain::SetWaterHeight(float Value)
{
	m_WaterHeight = Value;
}

float CTerrain::GetWaterHeight() const
{
	return m_WaterHeight;
}

void CTerrain::SetTerrainTessFactor(float Value)
{
	m_TerrainTessFactor = Value;
}

float CTerrain::GetTerrainTessFactor() const
{
	return m_TerrainTessFactor;
}

void CTerrain::SetWaterTessFactor(float Value)
{
	m_WaterTessFactor = Value;
}

float CTerrain::GetWaterTessFactor() const
{
	return m_WaterTessFactor;
}

const XMFLOAT2& CTerrain::GetSize() const
{
	return m_Size;
}

int CTerrain::GetMaterialCount() const
{
	assert(m_Object3DTerrain);
	return (int)m_Object3DTerrain->GetMaterialCount();
}

int CTerrain::GetMaskingDetail() const
{
	return m_MaskingTextureDetail;
}

bool CTerrain::HasFoliageCluster() const
{
	return m_bHasFoliageCluster;
}

const string& CTerrain::GetFileName() const
{
	return m_FileName;
}

const XMFLOAT2& CTerrain::GetSelectionPosition() const
{
	return m_CBTerrainSelectionData.Position;
}

int CTerrain::GetFoliagePlacingDetail() const
{
	return m_FoliagePlacingDetail;
}

float CTerrain::GetTerrainHeightAt(float X, float Z)
{
	if (m_HeightMapTextureRawData.size())
	{
		int iTerrainHalfSizeX{ (int)(m_Size.x * 0.5f) };
		int iTerrainHalfSizeZ{ (int)(m_Size.y * 0.5f) };
		int iX{ (int)floor(X) + iTerrainHalfSizeX }; // [0, TerrainSizeX + 1]
		int iZ{ -(int)floor(Z) + iTerrainHalfSizeZ }; // [0, TerrainSizeZ + 1]
		float dX{ X - floor(X) };
		float dZ{ Z - floor(Z) };
		
		if (dX == 0.0f && dZ == 0.0f)
		{
			return GetTerrainHeightAt(iX, iZ);
		}
		else
		{
			int idX{}, idZ{};
			if (dX != 0.0f)
			{
				idX = (int)(dX / dX);
				if (dX < 0.0f) idX = -idX;
			}
			if (dZ != 0.0f)
			{
				idZ = (int)(dZ / dZ);
				if (dZ < 0.0f) idZ = -idZ;
			}
			int iCmpX{ iX + idX };
			int iCmpZ{ iZ - idZ };
			float Height{ GetTerrainHeightAt(iX, iZ) };
			float CmpHeightX{ GetTerrainHeightAt(iCmpX, iZ) };
			float CmpHeightZ{ GetTerrainHeightAt(iX, iCmpZ) };

			float XLerp{ Lerp(Height, CmpHeightX, abs(dX)) };
			float ZLerp{ Lerp(Height, CmpHeightZ, abs(dZ)) };

			if (dX == 0.0f) return ZLerp;
			if (dZ == 0.0f) return XLerp;

			float Tan{ abs(dZ) / abs(dX) };
			float Theta{ atan(Tan) };
			float Weight{ Theta / XM_PIDIV4 }; // [0.0f, 1.0f]

			return Lerp(XLerp, ZLerp, Weight);
		}
	}
	return 0.0f;
}

float CTerrain::GetTerrainHeightAt(int iX, int iZ)
{
	if (m_HeightMapTextureRawData.size())
	{
		const int KHeightMapSizeX{ (int)m_HeightMapTextureSize.x };
		const int KHeightMapSizeZ{ (int)m_HeightMapTextureSize.y };

		if (iX < 0) iX = KHeightMapSizeX - 1;
		if (iZ < 0) iZ = KHeightMapSizeZ - 1;
		if (iX >= KHeightMapSizeX) iX = 0;
		if (iZ >= KHeightMapSizeZ) iZ = 0;

		int iPixel{ iZ * KHeightMapSizeX + iX };
		float NormalizeHeight{ (float)m_HeightMapTextureRawData[iPixel].R / 255.0f };
		float Height{ NormalizeHeight * KHeightRange }; // [0, KHeightRange]
		Height += KMinHeight; // [-KMinHeight, +KMaxHeight]
		return Height;
	}
	return 0.0f;
}

void CTerrain::UpdateWind(float DeltaTime)
{
	XMVECTOR xmvPosition{ XMLoadFloat3(&m_CBWindData.Position) };
	xmvPosition += m_CBWindData.Velocity * DeltaTime;	
	XMStoreFloat3(&m_CBWindData.Position, xmvPosition);
	
	float HalfTerrainSizeX{ m_Size.x * 0.5f };
	float HalfTerrainSizeZ{ m_Size.y * 0.5f };
	if (m_CBWindData.Position.x < -HalfTerrainSizeX - m_CBWindData.Radius)
	{
		m_CBWindData.Position.x = +HalfTerrainSizeX + m_CBWindData.Radius;
	}
	else if (m_CBWindData.Position.x > +HalfTerrainSizeX + m_CBWindData.Radius)
	{
		m_CBWindData.Position.x = -HalfTerrainSizeX - m_CBWindData.Radius;
	}
	if (m_CBWindData.Position.z < -HalfTerrainSizeZ - m_CBWindData.Radius)
	{
		m_CBWindData.Position.z = +HalfTerrainSizeZ + m_CBWindData.Radius;
	}
	else if (m_CBWindData.Position.z > +HalfTerrainSizeZ + m_CBWindData.Radius)
	{
		m_CBWindData.Position.z = -HalfTerrainSizeZ - m_CBWindData.Radius;
	}
	m_CBWindData.Position.y = GetTerrainHeightAt(m_CBWindData.Position.x, m_CBWindData.Position.z);

	m_PtrGame->UpdateCBWindData(m_CBWindData);
}

void CTerrain::Draw(bool bDrawNormals)
{
	if (!m_Object3DTerrain) return;

	m_CBTerrainSelectionData.TerrainWorld = m_Object3DTerrain->ComponentTransform.MatrixWorld;
	m_CBTerrainSelectionData.InverseTerrainWorld = XMMatrixInverse(nullptr, m_CBTerrainSelectionData.TerrainWorld);
	m_PtrGame->UpdateCBTerrainSelection(m_CBTerrainSelectionData);
	m_PtrGame->UpdateVSSpace(m_CBTerrainSelectionData.TerrainWorld);

	CShader* VS{ m_PtrGame->GetBaseShader(EBaseShader::VSTerrain) };
	CShader* PS{ m_PtrGame->GetBaseShader(EBaseShader::PSTerrain) };

	VS->Use();
	VS->UpdateAllConstantBuffers();

	PS->Use();
	PS->UpdateAllConstantBuffers();

	m_HeightMapTexture->SetShaderType(EShaderType::VertexShader);
	m_HeightMapTexture->Use();
	m_MaskingTexture->Use();

	if (bDrawNormals)
	{
		m_PtrGame->UpdateGSSpace();
		m_PtrGame->GetBaseShader(EBaseShader::GSNormal)->Use();
		m_PtrGame->GetBaseShader(EBaseShader::GSNormal)->UpdateAllConstantBuffers();
	}

	m_Object3DTerrain->Draw();

	if (bDrawNormals)
	{
		m_PtrDeviceContext->GSSetShader(nullptr, nullptr, 0);
	}

	if (m_bShouldDrawWater)
	{
		DrawWater();
	}

	if (m_bHasFoliageCluster)
	{
		DrawFoliageCluster();
	}
}

void CTerrain::DrawHeightMapTexture()
{
	if (!m_Object2DTextureRepresentation) return;

	m_HeightMapTexture->SetShaderType(EShaderType::PixelShader);
	m_HeightMapTexture->Use();

	m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->CullCounterClockwise());
	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthNone(), 0);

	m_PtrGame->UpdateVS2DSpace(KMatrixIdentity);
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->UpdateAllConstantBuffers();
	m_PtrGame->GetBaseShader(EBaseShader::PSHeightMap2D)->Use();

	m_Object2DTextureRepresentation->Draw();

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthDefault(), 0);
}

void CTerrain::DrawMaskingTexture()
{
	if (!m_Object2DTextureRepresentation) return;

	m_MaskingTexture->Use(0);

	m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->CullCounterClockwise());
	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthNone(), 0);

	m_PtrGame->UpdateVS2DSpace(KMatrixIdentity);
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->UpdateAllConstantBuffers();
	m_PtrGame->GetBaseShader(EBaseShader::PSMasking2D)->Use();
	
	m_Object2DTextureRepresentation->Draw();

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthDefault(), 0);
}

void CTerrain::DrawFoliagePlacingTexture()
{
	if (!m_FoliagePlaceTexture) return;
	if (!m_Object2DTextureRepresentation) return;

	m_FoliagePlaceTexture->SetShaderType(EShaderType::PixelShader);
	m_FoliagePlaceTexture->Use();

	m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->CullCounterClockwise());
	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthNone(), 0);

	m_PtrGame->UpdateVS2DSpace(KMatrixIdentity);
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->UpdateAllConstantBuffers();
	m_PtrGame->GetBaseShader(EBaseShader::PSHeightMap2D)->Use();

	m_Object2DTextureRepresentation->Draw();

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthDefault(), 0);
}

void CTerrain::DrawWater()
{
	m_Object3DWater->ComponentTransform = m_Object3DTerrain->ComponentTransform;
	m_Object3DWater->ComponentTransform.Translation += XMVectorSet(0, m_WaterHeight, 0, 1);
	m_Object3DWater->UpdateWorldMatrix();

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetDepthStencilStateLessEqualNoWrite(), 0);
	m_PtrGame->UpdateVSSpace(m_Object3DWater->ComponentTransform.MatrixWorld);
	m_PtrGame->UpdateHSTessFactor(m_WaterTessFactor);
	m_PtrGame->GetBaseShader(EBaseShader::VSBase)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::VSBase)->UpdateAllConstantBuffers();
	m_PtrGame->GetBaseShader(EBaseShader::HSWater)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::HSWater)->UpdateAllConstantBuffers();
	m_PtrGame->GetBaseShader(EBaseShader::DSWater)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::DSWater)->UpdateAllConstantBuffers();
	m_PtrGame->GetBaseShader(EBaseShader::PSWater)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::PSWater)->UpdateAllConstantBuffers();
	m_WaterNormalTexture->Use();
	m_WaterDisplacementTexture->Use();
	m_Object3DWater->Draw();
	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthDefault(), 0);
}

void CTerrain::DrawFoliageCluster()
{
	if (m_vFoliages.empty()) return;
	if (m_vFoliages[0]->GetInstanceCount() == 0) return;

	m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->CullNone());
	m_PtrDeviceContext->OMSetBlendState(m_PtrGame->GetBlendStateAlphaToCoverage(), nullptr, 0xFFFFFFFF);

	m_PtrGame->UpdateVSSpace(KMatrixIdentity);
	m_HeightMapTexture->SetShaderType(EShaderType::VertexShader);
	m_HeightMapTexture->Use();
	m_PerlinNoiseTexture->Use();
	
	m_PtrGame->GetBaseShader(EBaseShader::VSFoliage)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::VSFoliage)->UpdateAllConstantBuffers();

	m_PtrGame->GetBaseShader(EBaseShader::PSFoliage)->Use();
	m_PtrGame->GetBaseShader(EBaseShader::PSFoliage)->UpdateAllConstantBuffers();

	m_PtrDeviceContext->HSSetShader(nullptr, nullptr, 0);
	m_PtrDeviceContext->DSSetShader(nullptr, nullptr, 0);

	for (const auto& Foliage : m_vFoliages)
	{
		Foliage->Draw();
	}

	m_PtrGame->SetUniversalRSState();

	// Wind sphere represenatation
	if (m_Object3DWindRepresentation && false)
	{
		m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->Wireframe());

		m_Object3DWindRepresentation->ComponentTransform.Translation = XMLoadFloat3(&m_CBWindData.Position);
		m_Object3DWindRepresentation->ComponentTransform.Scaling = XMVectorSet(m_CBWindData.Radius, m_CBWindData.Radius, m_CBWindData.Radius, 0);
		m_Object3DWindRepresentation->UpdateWorldMatrix();

		m_PtrGame->UpdateVSSpace(m_Object3DWindRepresentation->ComponentTransform.MatrixWorld);

		m_PtrGame->GetBaseShader(EBaseShader::VSBase)->Use();
		m_PtrGame->GetBaseShader(EBaseShader::VSBase)->UpdateAllConstantBuffers();

		m_PtrGame->GetBaseShader(EBaseShader::PSVertexColor)->Use();

		m_Object3DWindRepresentation->Draw();

		m_PtrGame->SetUniversalRSState();
	}
}
