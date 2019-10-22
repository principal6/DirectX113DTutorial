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
	SModel Model{};
	Model.vMeshes.emplace_back(GenerateTerrainBase(TerrainSize));
	Model.vMaterials.emplace_back(Material);
	Model.vMaterials.back().SetbShouldGenerateAutoMipMap(true); // @important
	Model.bUseMultipleTexturesInSingleMesh = true; // @important

	m_Size = TerrainSize;

	m_MaskingTextureDetail = MaskingDetail;
	m_MaskingTextureDetail = max(m_MaskingTextureDetail, KMaskingMinDetail);
	m_MaskingTextureDetail = min(m_MaskingTextureDetail, KMaskingMaxDetail);

	m_Object3D.release();
	m_Object3D = make_unique<CObject3D>(m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3D->Create(Model);
	m_Object3D->m_bTesselate = true; // @important

	m_Object2DMaskingTextureRepresentation.release();
	m_Object2DMaskingTextureRepresentation = make_unique<CObject2D>(m_PtrDevice, m_PtrDeviceContext);
	m_Object2DMaskingTextureRepresentation->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	CreateMaskingTexture(true);

	UpdateVertexNormalsTangents();
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

	// 4B (float) Terrain Z Size
	READ_BYTES(4);
	m_Size.y = READ_BYTES_TO_FLOAT;

	// 4B (float) Masking detail
	READ_BYTES(4);
	m_MaskingTextureDetail = READ_BYTES_TO_FLOAT;

	// 4B (uint32_t) Masking texture raw data size
	READ_BYTES(4);
	m_MaskingTextureRawData.resize(READ_BYTES_TO_UINT32);

	// Raw data
	for (SPixelUNorm& Pixel : m_MaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		READ_BYTES(4);

		Pixel.R = ReadBytes[0];
		Pixel.G = ReadBytes[1];
		Pixel.B = ReadBytes[2];
		Pixel.A = ReadBytes[3];
	}

	// Model data
	_ReadStaticModelFile(ifs, Model);

	Model.bUseMultipleTexturesInSingleMesh = true; // @important

	m_Object3D.release();
	m_Object3D = make_unique<CObject3D>(m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3D->Create(Model);
	m_Object3D->m_bTesselate = true; // @important

	m_Object2DMaskingTextureRepresentation.release();
	m_Object2DMaskingTextureRepresentation = make_unique<CObject2D>(m_PtrDevice, m_PtrDeviceContext);
	m_Object2DMaskingTextureRepresentation->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	CreateMaskingTexture(false);

	UpdateVertexNormalsTangents();
}

void CTerrain::Save(const string& FileName)
{
	if (!m_Object3D) return;

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

	// 4B (float) Masking detail
	WRITE_FLOAT_TO_BYTES(m_MaskingTextureDetail);

	// 4B (uint32_t) Masking texture raw data size
	WRITE_UINT32_TO_BYTES(m_MaskingTextureRawData.size());

	// Raw data
	for (const SPixelUNorm& Pixel : m_MaskingTextureRawData)
	{
		// 4B (uint8_t * 4) RGBA (UNORM)
		ofs.write((const char*)&Pixel.R, 1);
		ofs.write((const char*)&Pixel.G, 1);
		ofs.write((const char*)&Pixel.B, 1);
		ofs.write((const char*)&Pixel.A, 1);
	}

	// Model data
	_WriteStaticModelFile(ofs, m_Object3D->m_Model);

	ofs.close();
}

void CTerrain::CreateMaskingTexture(bool bShouldClear)
{
	m_MaskingTextureSize.x = m_Size.x * m_MaskingTextureDetail;
	m_MaskingTextureSize.y = m_Size.y * m_MaskingTextureDetail;

	m_MaskingTexture.release();
	m_MaskingTexture = make_unique<CMaterialTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_MaskingTexture->CreateBlankTexture(DXGI_FORMAT_R8G8B8A8_UNORM, m_MaskingTextureSize);
	m_MaskingTexture->SetSlot(CObject3D::KTerrainMaskingTextureSlot);
	m_MaskingTexture->Use();

	if (bShouldClear)
	{
		m_MaskingTextureRawData.clear();
		m_MaskingTextureRawData.resize(static_cast<size_t>(m_MaskingTextureSize.x)* static_cast<size_t>(m_MaskingTextureSize.y));
	}
	else
	{
		UpdateMaskingTexture();
	}

	XMMATRIX Translation{ XMMatrixTranslation(m_Size.x / 2.0f, 0, m_Size.y / 2.0f) };
	XMMATRIX Scaling{ XMMatrixScaling(1 / m_Size.x, 1.0f, 1 / m_Size.y) };
	m_MatrixMaskingSpace = Translation * Scaling;
	m_PtrGame->UpdatePSTerrainSpace(m_MatrixMaskingSpace);
}

const XMFLOAT2& CTerrain::GetSize() const
{
	return m_Size;
}

int CTerrain::GetMaterialCount() const
{
	assert(m_Object3D);
	return (int)m_Object3D->m_Model.vMaterials.size();
}

const XMFLOAT2& CTerrain::GetSelectionRoundUpPosition() const
{
	return m_SelectionRoundUpPosition;
}

float CTerrain::GetMaskingTextureDetail() const
{
	return m_MaskingTextureDetail;
}

const CMaterial& CTerrain::GetMaterial(int Index) const
{
	assert(m_Object3D);
	return m_Object3D->m_Model.vMaterials[Index];
}

void CTerrain::AddMaterial(const CMaterial& Material)
{
	assert(m_Object3D);
	if ((int)m_Object3D->m_Model.vMaterials.size() == KMaterialMaxCount) return;

	m_Object3D->AddMaterial(Material);
}

void CTerrain::SetMaterial(int MaterialID, const CMaterial& NewMaterial)
{
	assert(m_Object3D);
	assert(MaterialID >= 0 && MaterialID < 4);

	m_Object3D->SetMaterial(MaterialID, NewMaterial);
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

void CTerrain::UpdateVertexNormalsTangents()
{
	assert(m_Object3D);

	CalculateNormals(m_Object3D->m_Model.vMeshes[0]);
	AverageNormals(m_Object3D->m_Model.vMeshes[0]);
	CalculateTangents(m_Object3D->m_Model.vMeshes[0]);
	AverageTangents(m_Object3D->m_Model.vMeshes[0]);

	m_Object3D->UpdateMeshBuffer();
}

void CTerrain::SetSelectionSize(float& Size)
{
	Size = min(Size, KSelectionMaxSize);
	Size = max(Size, KSelectionMinSize);

	m_SelectionHalfSize = Size / 2.0f;
}

void CTerrain::UpdateSelection(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection)
{
	// Do not consider World transformation!!
	// Terrain uses single mesh
	SMesh& Mesh{ m_Object3D->m_Model.vMeshes[0] };

	XMVECTOR T{ KVectorGreatest };
	XMVECTOR PlaneT{};
	XMVECTOR PointOnPlane{};
	if (IntersectRayPlane(PickingRayOrigin, PickingRayDirection, XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0), &PlaneT))
	{
		PointOnPlane = PickingRayOrigin + PickingRayDirection * PlaneT;

		m_SelectionRoundUpPosition.x = XMVectorGetX(PointOnPlane);
		m_SelectionRoundUpPosition.x = static_cast<float>(int(m_SelectionRoundUpPosition.x + 0.5f));
		m_SelectionRoundUpPosition.y = XMVectorGetZ(PointOnPlane);
		m_SelectionRoundUpPosition.y = static_cast<float>(int(m_SelectionRoundUpPosition.y + 0.5f));

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

			if (MaxX >= m_SelectionRoundUpPosition.x - m_SelectionHalfSize &&
				m_SelectionRoundUpPosition.x + m_SelectionHalfSize >= MinX &&
				MaxZ >= m_SelectionRoundUpPosition.y - m_SelectionHalfSize &&
				m_SelectionRoundUpPosition.y + m_SelectionHalfSize >= MinZ)
			{
				float V0Length{ XMVectorGetX(XMVector3Length(V0.Position -
					XMVectorSet(m_SelectionRoundUpPosition.x, XMVectorGetY(V0.Position), m_SelectionRoundUpPosition.y, 0.0f))) };
				float V1Length{ XMVectorGetX(XMVector3Length(V1.Position -
					XMVectorSet(m_SelectionRoundUpPosition.x, XMVectorGetY(V1.Position), m_SelectionRoundUpPosition.y, 0.0f))) };
				float V2Length{ XMVectorGetX(XMVector3Length(V2.Position -
					XMVectorSet(m_SelectionRoundUpPosition.x, XMVectorGetY(V2.Position), m_SelectionRoundUpPosition.y, 0.0f))) };

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

		m_Object3D->UpdateMeshBuffer();
	}
}

void CTerrain::ReleaseSelection()
{
	// Do not consider World transformation!!
	// Terrain uses single mesh
	SMesh& Mesh{ m_Object3D->m_Model.vMeshes[0] };

	for (auto& Triangle : Mesh.vTriangles)
	{
		SVertex3D& V0{ Mesh.vVertices[Triangle.I0] };
		SVertex3D& V1{ Mesh.vVertices[Triangle.I1] };
		SVertex3D& V2{ Mesh.vVertices[Triangle.I2] };

		V0.TexCoord = XMVectorSetZ(V0.TexCoord, 0.0f);
		V1.TexCoord = XMVectorSetZ(V1.TexCoord, 0.0f);
		V2.TexCoord = XMVectorSetZ(V2.TexCoord, 0.0f);
	}

	m_Object3D->UpdateMeshBuffer();
}

void CTerrain::UpdateHoverPosition(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection)
{
	if (!m_Object3D) return;

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

void CTerrain::SelectTerrain(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, bool bShouldEdit, bool bIsLeftButton, float DeltaHeightFactor)
{
	if (m_eEditMode == ETerrainEditMode::Masking)
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

		if (bShouldEdit) UpdateHeight(bIsLeftButton, DeltaHeightFactor);
	}
}

void CTerrain::UpdateMaskingTexture()
{
	m_MaskingTexture->UpdateTextureRawData(&m_MaskingTextureRawData[0]);
}

void CTerrain::ShouldTessellate(bool Value)
{
	if (m_Object3D)
	{
		m_Object3D->m_bTesselate = Value;
	}
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

void CTerrain::SetEditMode(ETerrainEditMode Mode)
{
	m_eEditMode = Mode;

	if (m_eEditMode == ETerrainEditMode::Masking) ReleaseSelection();
}

ETerrainEditMode CTerrain::GetEditMode()
{
	return m_eEditMode;
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

void CTerrain::Draw(bool bUseTerrainSelector, bool bDrawNormals)
{
	if (!m_Object3D) return;

	CShader* VS{ m_PtrGame->GetBaseShader(EBaseShader::VSBase) };
	CShader* PS{ m_PtrGame->GetBaseShader(EBaseShader::PSTerrain) };
	
	VS->Use();
	PS->Use();
	
	m_PtrGame->UpdateVSSpace(KMatrixIdentity);
	VS->UpdateConstantBuffer(0);

	m_MaskingTexture->Use();

	if (bDrawNormals)
	{
		m_PtrGame->GetBaseShader(EBaseShader::GSNormal)->Use();
		m_PtrGame->UpdateGSSpace();

		m_Object3D->Draw();

		m_PtrDeviceContext->GSSetShader(nullptr, nullptr, 0);
	}
	else
	{
		m_Object3D->Draw();
	}
	
	if (bUseTerrainSelector)
	{
		m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->Wireframe());
		m_PtrDeviceContext->PSSetShaderResources(0, 0, nullptr);

		m_Object3D->Draw(true);
	}
}

void CTerrain::DrawMaskingTexture()
{
	if (!m_Object2DMaskingTextureRepresentation) return;

	m_MaskingTexture->Use(0);

	m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->CullCounterClockwise());
	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthNone(), 0);

	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->Use();
	m_PtrGame->UpdateVS2DSpace(KMatrixIdentity);
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->UpdateConstantBuffer(0);
	m_PtrGame->GetBaseShader(EBaseShader::PSMasking2D)->Use();
	
	m_Object2DMaskingTextureRepresentation->Draw();

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthDefault(), 0);
}

void CTerrain::UpdateHeight(bool bIsLeftButton, float DeltaHeightFactor)
{
	if (!m_Object3D) return;

	SMesh& Mesh{ m_Object3D->m_Model.vMeshes[0] };

	int VertexCount{ static_cast<int>(m_Size.x * m_Size.y * 4) };
	const int ZMax{ (int)m_Size.y + 1 };
	const int XMax{ (int)m_Size.x + 1 };
	for (int iZ = 0; iZ < ZMax; ++iZ)
	{
		for (int iX = 0; iX < XMax; ++iX)
		{
			int iVertex0{ iX * 4 + iZ * XMax * 4 };
			int iVertex1{ iVertex0 + 1 };
			int iVertex2{ iVertex0 + 2 };
			int iVertex3{ iVertex0 + 3 };

			if (iVertex0 >= 0 && iVertex0 < VertexCount)
			{
				const XMVECTOR& Position{ Mesh.vVertices[iVertex0].Position };
				float PositionX{ XMVectorGetX(Position) };
				float PositionZ{ XMVectorGetZ(Position) };
				if (PositionX >= m_SelectionRoundUpPosition.x - m_SelectionHalfSize &&
					PositionX <= m_SelectionRoundUpPosition.x + m_SelectionHalfSize &&
					PositionZ >= m_SelectionRoundUpPosition.y - m_SelectionHalfSize &&
					PositionZ <= m_SelectionRoundUpPosition.y + m_SelectionHalfSize)
				{
					UpdateVertex(Mesh.vVertices[iVertex0], bIsLeftButton, DeltaHeightFactor);
				}
			}
			if (iVertex1 >= 0 && iVertex1 < VertexCount)
			{
				const XMVECTOR& Position{ Mesh.vVertices[iVertex1].Position };
				float PositionX{ XMVectorGetX(Position) };
				float PositionZ{ XMVectorGetZ(Position) };
				if (PositionX >= m_SelectionRoundUpPosition.x - m_SelectionHalfSize &&
					PositionX <= m_SelectionRoundUpPosition.x + m_SelectionHalfSize &&
					PositionZ >= m_SelectionRoundUpPosition.y - m_SelectionHalfSize &&
					PositionZ <= m_SelectionRoundUpPosition.y + m_SelectionHalfSize)
				{
					UpdateVertex(Mesh.vVertices[iVertex1], bIsLeftButton, DeltaHeightFactor);
				}
			}
			if (iVertex2 >= 0 && iVertex2 < VertexCount)
			{
				const XMVECTOR& Position{ Mesh.vVertices[iVertex2].Position };
				float PositionX{ XMVectorGetX(Position) };
				float PositionZ{ XMVectorGetZ(Position) };
				if (PositionX >= m_SelectionRoundUpPosition.x - m_SelectionHalfSize &&
					PositionX <= m_SelectionRoundUpPosition.x + m_SelectionHalfSize &&
					PositionZ >= m_SelectionRoundUpPosition.y - m_SelectionHalfSize &&
					PositionZ <= m_SelectionRoundUpPosition.y + m_SelectionHalfSize)
				{
					UpdateVertex(Mesh.vVertices[iVertex2], bIsLeftButton, DeltaHeightFactor);
				}
			}
			if (iVertex3 >= 0 && iVertex3 < VertexCount)
			{
				const XMVECTOR& Position{ Mesh.vVertices[iVertex3].Position };
				float PositionX{ XMVectorGetX(Position) };
				float PositionZ{ XMVectorGetZ(Position) };
				if (PositionX >= m_SelectionRoundUpPosition.x - m_SelectionHalfSize &&
					PositionX <= m_SelectionRoundUpPosition.x + m_SelectionHalfSize &&
					PositionZ >= m_SelectionRoundUpPosition.y - m_SelectionHalfSize &&
					PositionZ <= m_SelectionRoundUpPosition.y + m_SelectionHalfSize)
				{
					UpdateVertex(Mesh.vVertices[iVertex3], bIsLeftButton, DeltaHeightFactor);
				}
			}
		}
	}

	m_Object3D->UpdateMeshBuffer();
}

void CTerrain::UpdateVertex(SVertex3D& Vertex, bool bIsLeftButton, float DeltaHeightFactor)
{
	float Y{ XMVectorGetY(Vertex.Position) };

	switch (m_eEditMode)
	{
	case ETerrainEditMode::SetHeight:
	{
		float NewY{ m_SetHeightValue };
		NewY = min(NewY, KMaxHeight);
		NewY = max(NewY, KMinHeight);
		Vertex.Position = XMVectorSetY(Vertex.Position, NewY);
	} break;
	case ETerrainEditMode::DeltaHeight:
		if (bIsLeftButton)
		{
			float NewY{ Y + m_DeltaHeightValue * DeltaHeightFactor };
			NewY = min(NewY, KMaxHeight);
			NewY = max(NewY, KMinHeight);

			Vertex.Position = XMVectorSetY(Vertex.Position, NewY);
		}
		else
		{
			float NewY{ Y - m_DeltaHeightValue * DeltaHeightFactor };
			NewY = min(NewY, KMaxHeight);
			NewY = max(NewY, KMinHeight);

			Vertex.Position = XMVectorSetY(Vertex.Position, NewY);
		}
		break;
	default:
		break;
	}

	// To let you know that this vertex's normal should be recalculated.
	Vertex.Normal = XMVectorSetW(Vertex.Normal, 1.0f);
}