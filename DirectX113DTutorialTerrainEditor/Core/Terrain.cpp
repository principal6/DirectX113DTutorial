#include "Terrain.h"
#include "PrimitiveGenerator.h"
#include "ModelPorter.h"
#include "Math.h"
#include "Game.h"

void CTerrain::Create(const XMFLOAT2& TerrainSize, const string& TextureFileName)
{
	SModel Model{};
	Model.vMeshes.emplace_back(GenerateTerrainBase(TerrainSize));
	
	SMaterial Material{};
	Material.bHasTexture = true;
	Material.TextureFileName = TextureFileName;
	Model.vMaterials.emplace_back(Material);
	Model.bUseMultipleTexturesInSingleMesh = true; // @important

	m_Object3D = make_unique<CObject3D>(m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3D->Create(Model);

	m_Object2DMaskingTexture = make_unique<CObject2D>(m_PtrDevice, m_PtrDeviceContext);
	m_Object2DMaskingTexture->CreateDynamic(Generate2DRectangle(XMFLOAT2(600, 480)));

	m_Size = TerrainSize;

	SetMaskingTexture();
}

void CTerrain::Load(const string& FileName)
{
	SModel Model{};
	ImportTerrain(FileName, Model, m_Size);
	Model.bUseMultipleTexturesInSingleMesh = true; // @important

	m_Object3D = make_unique<CObject3D>(m_PtrDevice, m_PtrDeviceContext, m_PtrGame);
	m_Object3D->Create(Model);

	SetMaskingTexture();
}

void CTerrain::Save(const string& FileName)
{
	ExportTerrain(m_Object3D->m_Model, m_Size, FileName);
}

void CTerrain::SetMaskingTexture()
{
	m_MaskingTextureSize.x = m_Size.x * KMaskingTextureDetail;
	m_MaskingTextureSize.y = m_Size.y * KMaskingTextureDetail;

	m_MaskingTexture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext, "MaskingTexture");
	m_MaskingTexture->CreateBlankTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, m_MaskingTextureSize);
	m_MaskingTexture->SetSlot(5);
	m_MaskingTexture->Use();

	m_MaskingTextureRawData.clear();
	m_MaskingTextureRawData.resize(static_cast<size_t>(m_MaskingTextureSize.x) * static_cast<size_t>(m_MaskingTextureSize.y));

	XMMATRIX Translation{ XMMatrixTranslation(m_Size.x / 2.0f, 0, m_Size.y / 2.0f) };
	XMMATRIX Scaling{ XMMatrixScaling(1 / m_Size.x, 1.0f, 1 / m_Size.y) };
	m_MatrixMaskingSpace = Translation * Scaling;
	m_PtrGame->UpdatePSTerrainEditSpace(m_MatrixMaskingSpace);
}

int CTerrain::GetTextureCount()
{
	assert(m_Object3D);
	return (int)m_Object3D->m_Model.vMaterials.size();
}

const string& CTerrain::GetTextureFileName(int TextureID)
{
	assert(m_Object3D);
	assert(TextureID >= 0 && TextureID < KTextureMaxCount);

	return m_Object3D->m_Model.vMaterials[TextureID].TextureFileName;
}

const XMFLOAT2& CTerrain::GetSelectionRoundUpPosition()
{
	return m_SelectionRoundUpPosition;
}

void CTerrain::SetTexture(int TextureID, const string& TextureFileName)
{
	assert(m_Object3D);
	assert(TextureID >= 0 && TextureID < 4);

	m_Object3D->m_Model.vMaterials[TextureID].bHasTexture = true;
	m_Object3D->m_Model.vMaterials[TextureID].TextureFileName = TextureFileName;
	
	m_Object3D->m_vOwnTextures[TextureID].release();
	m_Object3D->m_vOwnTextures[TextureID].reset();
	m_Object3D->m_vOwnTextures[TextureID] = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext, "Own");
	m_Object3D->m_vOwnTextures[TextureID]->CreateFromFile(TextureFileName);
	m_Object3D->m_vOwnTextures[TextureID]->SetSlot(TextureID);
}

void CTerrain::AddTexture(const string& TextureFileName)
{
	assert(m_Object3D);
	if ((int)m_Object3D->m_Model.vMaterials.size() == KTextureMaxCount) return;

	size_t TextureID{ m_Object3D->m_Model.vMaterials.size() };
	m_Object3D->m_Model.vMaterials.emplace_back();
	m_Object3D->m_Model.vMaterials.back().bHasTexture = true;
	m_Object3D->m_Model.vMaterials.back().TextureFileName = TextureFileName;
	m_Object3D->m_Model.vMaterials.back().TextureID = TextureID;

	m_Object3D->m_vOwnTextures.emplace_back(make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext, "Own"));
	m_Object3D->m_vOwnTextures.back()->CreateFromFile(TextureFileName);
	m_Object3D->m_vOwnTextures.back()->SetSlot(static_cast<UINT>(TextureID));
}

void CTerrain::UpdateMasking(EMaskingLayer eLayer, const XMFLOAT2& Position, float Value, float Radius, bool bForceSet)
{
	float RadiusSquare{ Radius * Radius * KMaskingTextureDetail * KMaskingTextureDetail };
	int CenterU{ static_cast<int>((+m_Size.x / 2.0f + Position.x) * KMaskingTextureDetail) };
	int CenterV{ static_cast<int>(-(-m_Size.y / 2.0f + Position.y) * KMaskingTextureDetail) };

	for (int iPixel = 0; iPixel < (int)m_MaskingTextureRawData.size(); ++iPixel)
	{
		int U{ iPixel % (int)m_MaskingTextureSize.x };
		int V{ iPixel / (int)m_MaskingTextureSize.x };

		float dU{ float(U - CenterU) };
		float dV{ float(V - CenterV) };
		float DistanceSquare{ dU * dU + dV * dV };
		if (DistanceSquare <= RadiusSquare)
		{
			float Factor{ pow(m_MaskingAttenuation, sqrt(DistanceSquare) / m_MaskingRadius) };
			Factor = max(Factor, 0.0f);
			Factor = min(Factor, 1.0f);

			if (bForceSet)
			{
				switch (eLayer)
				{
				case EMaskingLayer::LayerR:
					m_MaskingTextureRawData[iPixel].x = Value * Factor;
					break;
				case EMaskingLayer::LayerG:
					m_MaskingTextureRawData[iPixel].y = Value * Factor;
					break;
				case EMaskingLayer::LayerB:
					m_MaskingTextureRawData[iPixel].z = Value * Factor;
					break;
				case EMaskingLayer::LayerA:
					m_MaskingTextureRawData[iPixel].w = Value * Factor;
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
					m_MaskingTextureRawData[iPixel].x = max(m_MaskingTextureRawData[iPixel].x, Value * Factor);
					break;
				case EMaskingLayer::LayerG:
					m_MaskingTextureRawData[iPixel].y = max(m_MaskingTextureRawData[iPixel].y, Value * Factor);
					break;
				case EMaskingLayer::LayerB:
					m_MaskingTextureRawData[iPixel].z = max(m_MaskingTextureRawData[iPixel].z, Value * Factor);
					break;
				case EMaskingLayer::LayerA:
					m_MaskingTextureRawData[iPixel].w = max(m_MaskingTextureRawData[iPixel].w, Value * Factor);
					break;
				default:
					break;
				}
			}
		}
	}

	UpdateMaskingTexture();
}

void CTerrain::UpdateVertexNormals()
{
	assert(m_Object3D);

	CalculateFaceNormals(m_Object3D->m_Model.vMeshes[0]);
	CalculateVertexNormalsFromFaceNormals(m_Object3D->m_Model.vMeshes[0]);

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

void CTerrain::SelectTerrain(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, bool bShouldEdit, bool bIsLeftButton)
{
	if (m_eEditMode == ETerrainEditMode::Masking)
	{
		UpdateHoverPosition(PickingRayOrigin, PickingRayDirection);

		if (bShouldEdit)
		{
			if (bIsLeftButton)
			{
				UpdateMasking(m_eMaskingLayer, m_HoverPosition, m_EditValue, m_MaskingRadius);
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

		if (bShouldEdit) UpdateHeight(bIsLeftButton);
	}
}

void CTerrain::UpdateMaskingTexture()
{
	m_MaskingTexture->UpdateTextureRawData(&m_MaskingTextureRawData[0]);
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

void CTerrain::SetEditMode(ETerrainEditMode Mode, float Value)
{
	m_eEditMode = Mode;
	m_EditValue = Value;

	if (m_eEditMode == ETerrainEditMode::Masking) ReleaseSelection();
}

void CTerrain::Draw(bool bUseTerrainSelector, bool bDrawNormals)
{
	if (!m_Object3D) return;

	CShader* VS{ m_PtrGame->GetBaseShader(EBaseShader::VSBase) };
	CShader* PS{ m_PtrGame->GetBaseShader(EBaseShader::PSTerrainEdit) };
	
	VS->Use();
	PS->Use();
	
	m_PtrGame->UpdateVSSpace(KMatrixIdentity);
	VS->UpdateConstantBuffer(0);

	m_MaskingTexture->Use();
	m_Object3D->Draw();

	if (bUseTerrainSelector)
	{
		m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->Wireframe());
		m_PtrDeviceContext->PSSetShaderResources(0, 0, nullptr);

		m_Object3D->Draw(true);
	}

	if (bDrawNormals)
	{
		m_PtrGame->GetBaseShader(EBaseShader::PSVertexColor)->Use();
		m_PtrGame->GetBaseShader(EBaseShader::GSNormal)->Use();

		m_Object3D->DrawNormals();

		m_PtrDeviceContext->GSSetShader(nullptr, nullptr, 0);
	}
}

void CTerrain::DrawMaskingTexture()
{
	if (!m_Object2DMaskingTexture) return;

	m_MaskingTexture->Use(0);

	m_PtrDeviceContext->RSSetState(m_PtrGame->GetCommonStates()->CullCounterClockwise());
	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthNone(), 0);

	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->Use();
	m_PtrGame->UpdateVS2DSpace(KMatrixIdentity);
	m_PtrGame->GetBaseShader(EBaseShader::VSBase2D)->UpdateConstantBuffer(0);
	m_PtrGame->GetBaseShader(EBaseShader::PSMasking2D)->Use();
	
	m_Object2DMaskingTexture->Draw();

	m_PtrDeviceContext->OMSetDepthStencilState(m_PtrGame->GetCommonStates()->DepthDefault(), 0);
}

void CTerrain::UpdateHeight(bool bIsLeftButton)
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
					UpdateVertex(Mesh.vVertices[iVertex0], bIsLeftButton);
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
					UpdateVertex(Mesh.vVertices[iVertex1], bIsLeftButton);
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
					UpdateVertex(Mesh.vVertices[iVertex2], bIsLeftButton);
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
					UpdateVertex(Mesh.vVertices[iVertex3], bIsLeftButton);
				}
			}
		}
	}

	m_Object3D->UpdateMeshBuffer();
}

void CTerrain::UpdateVertex(SVertex3D& Vertex, bool bIsLeftButton)
{
	float Y{ XMVectorGetY(Vertex.Position) };

	switch (m_eEditMode)
	{
	case ETerrainEditMode::SetHeight:
		Vertex.Position = XMVectorSetY(Vertex.Position, m_EditValue);
		break;
	case ETerrainEditMode::DeltaHeight:
		if (bIsLeftButton)
		{
			Vertex.Position = XMVectorSetY(Vertex.Position, Y + m_EditValue);
		}
		else
		{
			Vertex.Position = XMVectorSetY(Vertex.Position, Y - m_EditValue);
		}
		break;
	default:
		break;
	}

	// To let you know that this vertex's normal should be recalculated.
	Vertex.Normal = XMVectorSetW(Vertex.Normal, 1.0f);
}