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

void CTerrain::Create(const XMFLOAT2& TerrainSize, const CMaterial& Material, float MaskingDetail)
{
	m_Size = TerrainSize;
	m_MaskingTextureDetail = MaskingDetail;
	m_MaskingTextureDetail = max(m_MaskingTextureDetail, KMaskingMinDetail);
	m_MaskingTextureDetail = min(m_MaskingTextureDetail, KMaskingMaxDetail);

	m_cbTerrainData.TerrainSizeX = m_Size.x;
	m_cbTerrainData.TerrainSizeZ = m_Size.y;
	m_cbTerrainData.TerrainHeightRange = KHeightRange;
	m_PtrGame->UpdateVSTerrainData(m_cbTerrainData);

	vector<CMaterial> vMaterials{};
	vMaterials.emplace_back(Material);
	vMaterials.back().ShouldGenerateAutoMipMap(true); // @important

	CreateTerrainObject3D(vMaterials);

	m_Object2DTextureRepresentation.release();
	m_Object2DTextureRepresentation = make_unique<CObject2D>(m_PtrDevice, m_PtrDeviceContext);
	m_Object2DTextureRepresentation->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	CreateHeightMapTexture(true);
	CreateMaskingTexture(true);

	CreateWater();
}

void CTerrain::Load(const string& FileName)
{
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
	m_cbTerrainData.TerrainSizeX = m_Size.x;
	
	// 4B (float) Terrain Z Size
	READ_BYTES(4);
	m_Size.y = READ_BYTES_TO_FLOAT;
	m_cbTerrainData.TerrainSizeZ = m_Size.y;

	// 4B (float) Terrain Height Range
	READ_BYTES(4);
	m_cbTerrainData.TerrainHeightRange = READ_BYTES_TO_FLOAT;
	m_PtrGame->UpdateVSTerrainData(m_cbTerrainData);

	// 4B (float) Water height
	READ_BYTES(4);
	m_WaterHeight = READ_BYTES_TO_FLOAT;

	// 4B (float) Masking detail
	READ_BYTES(4);
	m_MaskingTextureDetail = READ_BYTES_TO_FLOAT;

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

	CreateTerrainObject3D(Model.vMaterials);

	m_Object2DTextureRepresentation.release();
	m_Object2DTextureRepresentation = make_unique<CObject2D>(m_PtrDevice, m_PtrDeviceContext);
	m_Object2DTextureRepresentation->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	CreateHeightMapTexture(false);
	CreateMaskingTexture(false);

	CreateWater();
}

void CTerrain::Save(const string& FileName)
{
	if (!m_Object3DTerrain) return;

	std::ofstream ofs{};
	ofs.open(FileName, std::ofstream::binary);
	assert(ofs.is_open());

	char FloatBytes[4]{};
	char Uint32Bytes[4]{};
	char XMFLOAT4Bytes[16]{};

	// 8B Signature
	ofs.write("TERR_KJW", 8);

	// 4B (float) Terrain X Size
	WRITE_FLOAT_TO_BYTES(m_Size.x);

	// 4B (float) Terrain Z Size
	WRITE_FLOAT_TO_BYTES(m_Size.y);

	// 4B (float) Terrain Height Range
	WRITE_FLOAT_TO_BYTES(m_cbTerrainData.TerrainHeightRange);

	// 4B (float) Water height
	WRITE_FLOAT_TO_BYTES(m_WaterHeight);

	// 4B (float) Masking detail
	WRITE_FLOAT_TO_BYTES(m_MaskingTextureDetail);

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
	// 1B (uint8_t) Material count
	ofs.put((uint8_t)m_Object3DTerrain->GetModel().vMaterials.size());

	_WriteModelMaterials(ofs, m_Object3DTerrain->GetModel().vMaterials);

	ofs.close();
}

void CTerrain::CreateTerrainObject3D(vector<CMaterial>& vMaterials)
{
	SModel Model{};
	assert(vMaterials.size());

	Model.vMeshes.clear();
	Model.vMeshes.emplace_back(GenerateTerrainBase(m_Size));
	CalculateNormals(Model.vMeshes.back());
	CalculateTangents(Model.vMeshes.back());
	Model.vMaterials = vMaterials;
	Model.bUseMultipleTexturesInSingleMesh = true; // @important

	m_Object3DTerrain.release();
	m_Object3DTerrain = make_unique<CObject3D>(m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3DTerrain->Create(Model);
	m_Object3DTerrain->ShouldTessellate(true); // @important
}

void CTerrain::CreateHeightMapTexture(bool bShouldClear)
{
	m_HeightMapTexture.release();
	m_HeightMapTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);

	m_HeightMapTextureSize = m_Size;
	m_HeightMapTextureSize.x += 1.0f;
	m_HeightMapTextureSize.y += 1.0f;
	m_HeightMapTexture->CreateBlankTexture(DXGI_FORMAT_R8_UNORM, m_HeightMapTextureSize);
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

	UpdateHeightMapTexture();
}

void CTerrain::CreateMaskingTexture(bool bShouldClear)
{
	m_MaskingTextureSize.x = m_Size.x * m_MaskingTextureDetail;
	m_MaskingTextureSize.y = m_Size.y * m_MaskingTextureDetail;

	m_MaskingTexture.release();
	m_MaskingTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_MaskingTexture->CreateBlankTexture(DXGI_FORMAT_R8G8B8A8_UNORM, m_MaskingTextureSize);
	m_MaskingTexture->SetSlot(CObject3D::KTerrainMaskingTextureSlot);
	m_MaskingTexture->Use();

	if (bShouldClear)
	{
		m_MaskingTextureRawData.clear();
		m_MaskingTextureRawData.resize(static_cast<size_t>(m_MaskingTextureSize.x) * static_cast<size_t>(m_MaskingTextureSize.y));
	}

	UpdateMaskingTexture();

	XMMATRIX Translation{ XMMatrixTranslation(m_Size.x / 2.0f, 0, m_Size.y / 2.0f) };
	XMMATRIX Scaling{ XMMatrixScaling(1 / m_Size.x, 1.0f, 1 / m_Size.y) };
	m_MatrixMaskingSpace = Translation * Scaling;
	m_PtrGame->UpdatePSTerrainSpace(m_MatrixMaskingSpace);
}

void CTerrain::CreateWater()
{
	const XMVECTOR KWaterColor{ XMVectorSet(0.0f, 0.5f, 0.6f, 0.9f) };

	m_Object3DWater.release();
	m_Object3DWater = make_unique<CObject3D>(m_PtrDevice, m_PtrDeviceContext, m_PtrGame);

	SMesh WaterMesh{ GenerateTerrainBase(m_Size, false, KWaterColor) };
	m_Object3DWater->Create(WaterMesh);
	m_Object3DWater->ShouldTessellate(true);

	m_WaterNormalTexture.release();
	m_WaterNormalTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_WaterNormalTexture->CreateTextureFromFile("Asset\\water_normal.jpg", false);
	m_WaterNormalTexture->SetSlot(0);
	m_WaterNormalTexture->Use();

	m_WaterDisplacementTexture.release();
	m_WaterDisplacementTexture = make_unique<CMaterial::CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_WaterDisplacementTexture->CreateTextureFromFile("Asset\\water_displacement.jpg", false);
	m_WaterDisplacementTexture->SetSlot(0);
	m_WaterDisplacementTexture->SetShaderType(EShaderType::DomainShader);
	m_WaterDisplacementTexture->Use();
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
	if (m_eEditMode == EEditMode::Masking)
	{
		UpdateHoverPosition(PickingRayOrigin, PickingRayDirection);

		if (bShouldEdit)
		{
			if (bIsLeftButton)
			{
				UpdateMasking(m_eMaskingLayer, m_HoverPosition, m_MaskingRatio, m_MaskingRadius);
			}
			else
			{
				UpdateMasking(m_eMaskingLayer, m_HoverPosition, 0.0f, m_MaskingRadius, true);
			}
		}
	}
	else
	{
		UpdateSelection(PickingRayOrigin, PickingRayDirection);

		if (bShouldEdit) UpdateHeights(bIsLeftButton);
	}
}

void CTerrain::UpdateSelection(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection)
{
	// Do not consider World transformation!!
	// Terrain uses single mesh
	SMesh& Mesh{ m_Object3DTerrain->GetModel().vMeshes[0] };

	XMVECTOR T{ KVectorGreatest };
	XMVECTOR PlaneT{};
	XMVECTOR PointOnPlane{};
	if (IntersectRayPlane(PickingRayOrigin, PickingRayDirection, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0), &PlaneT))
	{
		PointOnPlane = PickingRayOrigin + PickingRayDirection * PlaneT;

		m_SelectionPosition.x = XMVectorGetX(PointOnPlane);
		if (m_SelectionPosition.x > 0) m_SelectionPosition.x += 0.5;
		if (m_SelectionPosition.x < 0) m_SelectionPosition.x -= 0.5;
		m_SelectionPosition.x = float((int)m_SelectionPosition.x);
		m_SelectionPosition.x = min(m_SelectionPosition.x, +m_Size.x / 2.0f);
		m_SelectionPosition.x = max(m_SelectionPosition.x, -m_Size.x / 2.0f);

		m_SelectionPosition.y = XMVectorGetZ(PointOnPlane);
		if (m_SelectionPosition.y > 0) m_SelectionPosition.y += 0.5;
		if (m_SelectionPosition.y < 0) m_SelectionPosition.y -= 0.5;
		m_SelectionPosition.y = float((int)m_SelectionPosition.y);
		m_SelectionPosition.y = min(m_SelectionPosition.y, +m_Size.y / 2.0f);
		m_SelectionPosition.y = max(m_SelectionPosition.y, -m_Size.y / 2.0f);

		for (auto& Triangle : Mesh.vTriangles)
		{
			SVertex3D& V0{ Mesh.vVertices[Triangle.I0] };
			SVertex3D& V1{ Mesh.vVertices[Triangle.I1] };
			SVertex3D& V2{ Mesh.vVertices[Triangle.I2] };
			XMVECTOR& V0WorldPosition{ V0.Position };
			XMVECTOR& V1WorldPosition{ V1.Position };
			XMVECTOR& V2WorldPosition{ V2.Position };

			float MaxX{ max(max(XMVectorGetX(V0WorldPosition), XMVectorGetX(V1WorldPosition)), XMVectorGetX(V2WorldPosition)) };
			float MinX{ min(min(XMVectorGetX(V0WorldPosition), XMVectorGetX(V1WorldPosition)), XMVectorGetX(V2WorldPosition)) };
			float MaxZ{ max(max(XMVectorGetZ(V0WorldPosition), XMVectorGetZ(V1WorldPosition)), XMVectorGetZ(V2WorldPosition)) };
			float MinZ{ min(min(XMVectorGetZ(V0WorldPosition), XMVectorGetZ(V1WorldPosition)), XMVectorGetZ(V2WorldPosition)) };

			if (MaxX >= m_SelectionPosition.x - m_SelectionHalfSize &&
				m_SelectionPosition.x + m_SelectionHalfSize >= MinX &&
				MaxZ >= m_SelectionPosition.y - m_SelectionHalfSize &&
				m_SelectionPosition.y + m_SelectionHalfSize >= MinZ)
			{
				float V0Length{ XMVectorGetX(XMVector3Length(V0.Position -
					XMVectorSet(m_SelectionPosition.x, XMVectorGetY(V0.Position), m_SelectionPosition.y, 0.0f))) };
				float V1Length{ XMVectorGetX(XMVector3Length(V1.Position -
					XMVectorSet(m_SelectionPosition.x, XMVectorGetY(V1.Position), m_SelectionPosition.y, 0.0f))) };
				float V2Length{ XMVectorGetX(XMVector3Length(V2.Position -
					XMVectorSet(m_SelectionPosition.x, XMVectorGetY(V2.Position), m_SelectionPosition.y, 0.0f))) };

				V0.TexCoord = XMVectorSetZ(V0.TexCoord, V0Length);
				V1.TexCoord = XMVectorSetZ(V1.TexCoord, V1Length);
				V2.TexCoord = XMVectorSetZ(V2.TexCoord, V2Length);
			}
			else
			{
				V0.TexCoord = XMVectorSetZ(V0.TexCoord, 0.0f);
				V1.TexCoord = XMVectorSetZ(V1.TexCoord, 0.0f);
				V2.TexCoord = XMVectorSetZ(V2.TexCoord, 0.0f);
			}
		}

		m_Object3DTerrain->UpdateMeshBuffer();
	}
}

void CTerrain::ReleaseSelection()
{
	// Do not consider World transformation!!
	// Terrain uses single mesh
	SMesh& Mesh{ m_Object3DTerrain->GetModel().vMeshes[0] };

	for (auto& Triangle : Mesh.vTriangles)
	{
		SVertex3D& V0{ Mesh.vVertices[Triangle.I0] };
		SVertex3D& V1{ Mesh.vVertices[Triangle.I1] };
		SVertex3D& V2{ Mesh.vVertices[Triangle.I2] };

		V0.TexCoord = XMVectorSetZ(V0.TexCoord, 0.0f);
		V1.TexCoord = XMVectorSetZ(V1.TexCoord, 0.0f);
		V2.TexCoord = XMVectorSetZ(V2.TexCoord, 0.0f);
	}

	m_Object3DTerrain->UpdateMeshBuffer();
}

void CTerrain::UpdateHeights(bool bIsLeftButton)
{
	if (!m_Object3DTerrain) return;

	int TerrainSizeX{ (int)m_Size.x };
	int TerrainSizeZ{ (int)m_Size.y };
	int CenterX{ (int)m_SelectionPosition.x + TerrainSizeX / 2 };
	int CenterZ{ (int)(-m_SelectionPosition.y) + TerrainSizeZ / 2 };

	int SelectionSize{ (int)(m_SelectionHalfSize * 2.0f) };
	if (SelectionSize == 1)
	{
		size_t iPixel{ CenterZ * (size_t)m_HeightMapTextureSize.x + CenterX };
		iPixel = min(iPixel, (size_t)((double)m_HeightMapTextureSize.x * m_HeightMapTextureSize.y) - 1);

		UpdateHeight(iPixel, bIsLeftButton);
	}
	else
	{
		for (int X = (int)(CenterX - m_SelectionHalfSize); X <= (int)(CenterX + m_SelectionHalfSize); ++X)
		{
			for (int Z = (int)(CenterZ - m_SelectionHalfSize); Z <= (int)(CenterZ + m_SelectionHalfSize); ++Z)
			{
				if (X < 0 || Z < 0) continue;
				if (X > TerrainSizeX || Z > TerrainSizeZ) continue;

				size_t iPixel{ Z * (size_t)m_HeightMapTextureSize.x + X };

				UpdateHeight(iPixel, bIsLeftButton);
			}
		}
	}

	UpdateHeightMapTexture();
}

void CTerrain::UpdateHeight(size_t iPixel, bool bIsLeftButton)
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

void CTerrain::UpdateHeightMapTexture()
{
	m_HeightMapTexture->UpdateTextureRawData(&m_HeightMapTextureRawData[0]);
}

void CTerrain::UpdateHoverPosition(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection)
{
	if (!m_Object3DTerrain) return;

	// Do not consider World transformation!!
	// Terrain uses single mesh
	XMVECTOR T{ KVectorGreatest };
	XMVECTOR PlaneT{};
	XMVECTOR PointOnPlane{};
	if (IntersectRayPlane(PickingRayOrigin, PickingRayDirection, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0), &PlaneT))
	{
		PointOnPlane = PickingRayOrigin + PickingRayDirection * PlaneT;

		m_HoverPosition.x = XMVectorGetX(PointOnPlane);
		m_HoverPosition.y = XMVectorGetZ(PointOnPlane);
	}
}

void CTerrain::UpdateMasking(EMaskingLayer eLayer, const XMFLOAT2& Position, float Value, float Radius, bool bForceSet)
{
	float DetailSquare{ m_MaskingTextureDetail * m_MaskingTextureDetail };
	float RadiusSquare{ Radius * Radius * DetailSquare };
	int CenterU{ static_cast<int>((+m_Size.x / 2.0f + Position.x) * m_MaskingTextureDetail) };
	int CenterV{ static_cast<int>(-(-m_Size.y / 2.0f + Position.y) * m_MaskingTextureDetail) };

	for (int iPixel = 0; iPixel < (int)m_MaskingTextureRawData.size(); ++iPixel)
	{
		int U{ iPixel % (int)m_MaskingTextureSize.x };
		int V{ iPixel / (int)m_MaskingTextureSize.x };

		float dU{ float(U - CenterU) };
		float dV{ float(V - CenterV) };
		float DistanceSquare{ dU * dU + dV * dV };
		if (DistanceSquare <= RadiusSquare)
		{
			float Factor{ 1.0f -
				(sqrt(DistanceSquare / DetailSquare) / m_MaskingRadius) * m_MaskingAttenuation - // Distance attenuation
				((DistanceSquare / DetailSquare) / m_MaskingRadius) * m_MaskingAttenuation }; // Distance square attenuation
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

	UpdateMaskingTexture();
}

void CTerrain::UpdateMaskingTexture()
{
	m_MaskingTexture->UpdateTextureRawData(&m_MaskingTextureRawData[0]);
}

void CTerrain::SetSelectionSize(float& Size)
{
	Size = min(Size, KSelectionMaxSize);
	Size = max(Size, KSelectionMinSize);

	m_SelectionHalfSize = Size / 2.0f;
}

void CTerrain::SetMaskingLayer(EMaskingLayer eLayer)
{
	m_eMaskingLayer = eLayer;
}

void CTerrain::SetMaskingAttenuation(float Attenuation)
{
	m_MaskingAttenuation = Attenuation;
}

void CTerrain::SetMaskingRadius(float Radius)
{
	m_MaskingRadius = Radius;
}

void CTerrain::SetSetHeightValue(float Value)
{
	m_SetHeightValue = Value;
}

void CTerrain::SetDeltaHeightValue(float Value)
{
	m_DeltaHeightValue = Value;
}

void CTerrain::SetMaskingValue(float Value)
{
	m_MaskingRatio = Value;
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

void CTerrain::SetEditMode(EEditMode Mode)
{
	m_eEditMode = Mode;

	if (m_eEditMode == EEditMode::Masking) ReleaseSelection();
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

const XMFLOAT2& CTerrain::GetSize() const
{
	return m_Size;
}

int CTerrain::GetMaterialCount() const
{
	assert(m_Object3DTerrain);
	return (int)m_Object3DTerrain->GetMaterialCount();
}

const XMFLOAT2& CTerrain::GetSelectionPosition() const
{
	return m_SelectionPosition;
}

float CTerrain::GetMaskingDetail() const
{
	return m_MaskingTextureDetail;
}

void CTerrain::Draw(bool bDrawNormals)
{
	if (!m_Object3DTerrain) return;

	CShader* VS{ m_PtrGame->GetBaseShader(EBaseShader::VSTerrain) };
	CShader* PS{ m_PtrGame->GetBaseShader(EBaseShader::PSTerrain) };
	
	m_PtrGame->UpdateVSSpace(KMatrixIdentity);
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

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetDepthStencilStateLessEqualNoWrite(), 0);
	m_PtrGame->UpdateVSSpace(XMMatrixTranslation(0, m_WaterHeight, 0));
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