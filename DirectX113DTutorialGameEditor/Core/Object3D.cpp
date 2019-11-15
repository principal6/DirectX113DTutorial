#include "Object3D.h"
#include "Game.h"

using std::max;
using std::min;
using std::vector;
using std::string;
using std::to_string;
using std::make_unique;

void CObject3D::Create(const SMesh& Mesh)
{
	m_Model.vMeshes.clear();
	m_Model.vMeshes.emplace_back(Mesh);

	m_Model.vMaterialData.clear();
	m_Model.vMaterialData.emplace_back();

	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const SMesh& Mesh, const CMaterialData& MaterialData)
{
	m_Model.vMeshes.clear();
	m_Model.vMeshes.emplace_back(Mesh);

	m_Model.vMaterialData.clear();
	m_Model.vMaterialData.emplace_back(MaterialData);
	
	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const vector<SMesh>& vMeshes, const vector<CMaterialData>& vMaterialData)
{
	m_Model.vMeshes = vMeshes;
	m_Model.vMaterialData = vMaterialData;
	
	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const SModel& Model)
{
	m_Model = Model;

	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::CreateFromFile(const string& FileName, bool bIsModelRigged)
{
	m_ModelFileName = FileName;

	ULONGLONG StartTimePoint{ GetTickCount64() };
	if (bIsModelRigged)
	{
		m_AssimpLoader.LoadAnimatedModelFromFile(FileName, m_Model, m_PtrDevice, m_PtrDeviceContext);

		ComponentRender.PtrVS = m_PtrGame->GetBaseShader(CGame::EBaseShader::VSAnimation);
	}
	else
	{
		m_AssimpLoader.LoadStaticModelFromFile(FileName, m_Model, m_PtrDevice, m_PtrDeviceContext);
	}
	OutputDebugString(("- Model [" + FileName + "] loaded. [" + to_string(GetTickCount64() - StartTimePoint) + "] elapsed.\n").c_str());

	CreateMeshBuffers();
	CreateMaterialTextures();

	for (const CMaterialData& Material : m_Model.vMaterialData)
	{
		// @important
		if (Material.HasTexture(STextureData::EType::OpacityTexture))
		{
			ComponentRender.bIsTransparent = true;
			break;
		}
	}

	m_bIsCreated = true;
}

bool CObject3D::HasAnimations()
{
	return (m_Model.vAnimations.size()) ? true : false;
}

void CObject3D::AddAnimationFromFile(const string& FileName, const string& AnimationName)
{
	if (!m_Model.bIsModelRigged) return;

	m_AssimpLoader.AddAnimationFromFile(FileName, m_Model);
	m_Model.vAnimations.back().Name = AnimationName;
}

void CObject3D::SetAnimationID(int ID)
{
	ID = max(min(ID, (int)(m_Model.vAnimations.size() - 1)), 0);

	m_CurrentAnimationID = ID;
}

int CObject3D::GetAnimationID() const
{
	return m_CurrentAnimationID;
}

int CObject3D::GetAnimationCount() const
{
	return (int)m_Model.vAnimations.size();
}

void CObject3D::SetAnimationName(int ID, const string& Name)
{
	if (m_Model.vAnimations.empty())
	{
		MB_WARN("애니메이션이 존재하지 않습니다.", "애니메이션 이름 지정 실패");
		return;
	}

	if (Name.size() > KMaxAnimationNameLength)
	{
		MB_WARN(("애니메이션 이름은 최대 " + to_string(KMaxAnimationNameLength) + "자입니다.").c_str(), "애니메이션 이름 지정 실패");
		return;
	}

	m_Model.vAnimations[ID].Name = Name;
}

const string& CObject3D::GetAnimationName(int ID) const
{
	return m_Model.vAnimations[ID].Name;
}

bool CObject3D::HasBakedAnimationTexture() const
{
	return (m_BakedAnimationTexture) ? true : false;
}

bool CObject3D::CanBakeAnimationTexture() const
{
	return !m_bIsBakedAnimationLoaded;
}

void CObject3D::BakeAnimationTexture()
{
	if (m_Model.vAnimations.empty()) return;

	int32_t AnimationCount{ (int32_t)m_Model.vAnimations.size() };
	int32_t TextureHeight{ KAnimationTextureReservedHeight };
	vector<int32_t> vAnimationHeights{};
	for (const auto& Animation : m_Model.vAnimations)
	{
		vAnimationHeights.emplace_back((int32_t)Animation.Duration + 1);
		TextureHeight += vAnimationHeights.back();
	}

	m_BakedAnimationTexture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_BakedAnimationTexture->CreateBlankTexture(CTexture::EFormat::Pixel128Float, XMFLOAT2((float)KAnimationTextureWidth, (float)TextureHeight));
	m_BakedAnimationTexture->SetShaderType(EShaderType::VertexShader);

	vector<SPixel128Float> vRawData{};
	
	vRawData.resize((int64_t)KAnimationTextureWidth * TextureHeight);
	vRawData[0].R = 'A';
	vRawData[0].G = 'N';
	vRawData[0].B = 'I';
	vRawData[0].A = 'M';

	float fAnimationCount{ (float)AnimationCount };
	memcpy(&vRawData[1].R, &fAnimationCount, sizeof(float));

	int32_t AnimationHeightSum{ KAnimationTextureReservedHeight };
	for (int32_t iAnimation = 0; iAnimation < (int32_t)vAnimationHeights.size(); ++iAnimation)
	{
		float fAnimationHeightSum{ (float)AnimationHeightSum };
		memcpy(&vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].R,
			&fAnimationHeightSum, sizeof(float));
		memcpy(&vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].G,
			&m_Model.vAnimations[iAnimation].Duration, sizeof(float));
		memcpy(&vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].B,
			&m_Model.vAnimations[iAnimation].TicksPerSecond, sizeof(float));
		//A

		// RGBA = 4 floats = 16 chars!
		memcpy(&vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].R,
			m_Model.vAnimations[iAnimation].Name.c_str(), sizeof(char) * 16);

		AnimationHeightSum += vAnimationHeights[iAnimation];
	}

	for (int32_t iAnimation = 0; iAnimation < (int32_t)m_Model.vAnimations.size(); ++iAnimation)
	{
		float fAnimationOffset{};
		memcpy(&fAnimationOffset, &vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].R, sizeof(float));

		const int32_t KAnimationYOffset{ (int32_t)fAnimationOffset };
		const int32_t KAnimationOffset{ (int32_t)((int64_t)KAnimationYOffset * KAnimationTextureWidth) };
		const SModel::SAnimation& Animation{ m_Model.vAnimations[iAnimation] };
		
		int32_t Duration{ (int32_t)Animation.Duration + 1 };
		for (int32_t iTime = 0; iTime < Duration; ++iTime)
		{
			const int32_t KTimeOffset{ (int32_t)((int64_t)iTime * KAnimationTextureWidth) };
			CalculateAnimatedBoneMatrices(Animation, (float)iTime, m_Model.vNodes[0], XMMatrixIdentity());

			for (int32_t iBoneMatrix = 0; iBoneMatrix < (int32_t)KMaxBoneMatrixCount; ++iBoneMatrix)
			{
				const auto& BoneMatrix{ m_AnimatedBoneMatrices[iBoneMatrix] };
				XMMATRIX TransposedBoneMatrix{ XMMatrixTranspose(BoneMatrix) };
				memcpy(&vRawData[(int64_t)KAnimationOffset + KTimeOffset + (int64_t)iBoneMatrix * 4].R, &TransposedBoneMatrix, sizeof(XMMATRIX));
			}
		}
	}

	m_BakedAnimationTexture->UpdateTextureRawData(&vRawData[0]);
}

void CObject3D::SaveBakedAnimationTexture(const string& FileName)
{
	if (!m_BakedAnimationTexture) return;

	m_BakedAnimationTexture->SaveToDDSFile(FileName);
}

void CObject3D::LoadBakedAnimationTexture(const string& FileName)
{
	m_BakedAnimationTexture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_BakedAnimationTexture->CreateTextureFromFile(FileName, false);

	ID3D11Texture2D* const AnimationTexture{ m_BakedAnimationTexture->GetTexture2DPtr() };

	D3D11_TEXTURE2D_DESC AnimationTextureDesc{};
	AnimationTexture->GetDesc(&AnimationTextureDesc);

	AnimationTextureDesc.BindFlags = 0;
	AnimationTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	AnimationTextureDesc.Usage = D3D11_USAGE_STAGING;

	ComPtr<ID3D11Texture2D> ReadableAnimationTexture{};
	assert(SUCCEEDED(m_PtrDevice->CreateTexture2D(&AnimationTextureDesc, nullptr, ReadableAnimationTexture.GetAddressOf())));

	m_PtrDeviceContext->CopyResource(ReadableAnimationTexture.Get(), AnimationTexture);

	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(ReadableAnimationTexture.Get(), 0, D3D11_MAP_READ, 0, &MappedSubresource)))
	{
		vector<SPixel128Float> vPixels{};
		vPixels.resize(KAnimationTextureWidth);
		memcpy(&vPixels[0], MappedSubresource.pData, sizeof(SPixel128Float) * KAnimationTextureWidth);

		// Animation count
		m_Model.vAnimations.clear();
		m_Model.vAnimations.resize((size_t)vPixels[1].R);

		for (int32_t iAnimation = 0; iAnimation < (int32_t)m_Model.vAnimations.size(); ++iAnimation)
		{
			m_Model.vAnimations[iAnimation].Duration = vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].G;
			m_Model.vAnimations[iAnimation].TicksPerSecond = vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].B;

			float NameR{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].R };
			float NameG{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].G };
			float NameB{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].B };
			float NameA{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].A };

			char Name[16]{};
			memcpy(&Name[0], &NameR, 4);
			memcpy(&Name[4], &NameG, 4);
			memcpy(&Name[8], &NameB, 4);
			memcpy(&Name[12], &NameA, 4);

			m_Model.vAnimations[iAnimation].Name = Name;
		}

		m_PtrDeviceContext->Unmap(ReadableAnimationTexture.Get(), 0);
	}

	m_BakedAnimationTexture->SetShaderType(EShaderType::VertexShader);

	m_bIsBakedAnimationLoaded = true;
}

void CObject3D::AddMaterial(const CMaterialData& MaterialData)
{
	m_Model.vMaterialData.emplace_back(MaterialData);
	m_Model.vMaterialData.back().Index(m_Model.vMaterialData.size() - 1);

	CreateMaterialTexture(m_Model.vMaterialData.size() - 1);
}

void CObject3D::SetMaterial(size_t Index, const CMaterialData& MaterialData)
{
	assert(Index < m_Model.vMaterialData.size());

	m_Model.vMaterialData[Index] = MaterialData;
	m_Model.vMaterialData[Index].Index(Index);

	CreateMaterialTexture(Index);
}

size_t CObject3D::GetMaterialCount() const
{
	return m_Model.vMaterialData.size();
}

void CObject3D::CreateInstances(int InstanceCount)
{
	m_vInstanceCPUData.clear();
	m_vInstanceCPUData.resize(InstanceCount);
	for (auto& InstanceCPUData : m_vInstanceCPUData)
	{
		InstanceCPUData.Scaling = ComponentTransform.Scaling;
	}

	m_vInstanceGPUData.clear();
	m_vInstanceGPUData.resize(InstanceCount);

	m_mapInstanceNameToIndex.clear();
	for (int iInstance = 0; iInstance < InstanceCount; ++iInstance)
	{
		string Name{ "instance" + to_string(iInstance) };
		m_mapInstanceNameToIndex[Name] = iInstance;
	}

	ComponentRender.PtrVS = m_PtrGame->GetBaseShader(CGame::EBaseShader::VSInstance);

	CreateInstanceBuffers();

	UpdateAllInstancesWorldMatrix();
}

void CObject3D::InsertInstance(bool bShouldCreateInstanceBuffers)
{
	uint32_t InstanceCount{ GetInstanceCount() };
	string Name{ "instance" + to_string(InstanceCount) };

	if (m_mapInstanceNameToIndex.find(Name) != m_mapInstanceNameToIndex.end())
	{
		for (uint32_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			Name = "instance" + to_string(iInstance);
			if (m_mapInstanceNameToIndex.find(Name) == m_mapInstanceNameToIndex.end()) break;
		}
	}

	if (Name.length() >= KInstanceNameZeroEndedMaxLength)
	{
		MB_WARN(("인스턴스 이름 [" + Name + "] 이 최대 길이(" +
			to_string(KInstanceNameZeroEndedMaxLength - 1) + " 자)를 넘어\n인스턴스 생성에 실패했습니다.").c_str(), "인스턴스 생성 실패");
		return;
	}

	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	m_vInstanceCPUData.emplace_back();
	m_vInstanceCPUData.back().Name = Name;
	m_vInstanceCPUData.back().Scaling = ComponentTransform.Scaling;
	m_vInstanceCPUData.back().BoundingSphere = ComponentPhysics.BoundingSphere; // @important
	m_mapInstanceNameToIndex[Name] = m_vInstanceCPUData.size() - 1;

	m_vInstanceGPUData.emplace_back();
	
	if (bShouldCreateInstanceBuffers)
	{
		if (m_vInstanceCPUData.size() == 1) CreateInstanceBuffers();
		if (bShouldRecreateInstanceBuffer) CreateInstanceBuffers();
	}

	ComponentRender.PtrVS = m_PtrGame->GetBaseShader(CGame::EBaseShader::VSInstance);

	UpdateInstanceWorldMatrix(static_cast<int>(m_vInstanceCPUData.size() - 1));
}

void CObject3D::InsertInstance(const string& Name)
{
	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	std::string LimitedName{ Name };
	if (LimitedName.length() >= KInstanceNameZeroEndedMaxLength)
	{
		MB_WARN(("인스턴스 이름 [" + LimitedName + "] 이 최대 길이(" + to_string(KInstanceNameZeroEndedMaxLength - 1) +
			" 자)를 넘어 잘려서 저장됩니다.").c_str(), "이름 길이 제한");

		LimitedName.resize(KInstanceNameZeroEndedMaxLength - 1);
	}

	m_vInstanceCPUData.emplace_back();
	m_vInstanceCPUData.back().Name = LimitedName;
	m_vInstanceCPUData.back().Scaling = ComponentTransform.Scaling;
	m_mapInstanceNameToIndex[LimitedName] = m_vInstanceCPUData.size() - 1;

	m_vInstanceGPUData.emplace_back();

	if (m_vInstanceCPUData.size() == 1) CreateInstanceBuffers();
	if (bShouldRecreateInstanceBuffer) CreateInstanceBuffers();

	ComponentRender.PtrVS = m_PtrGame->GetBaseShader(CGame::EBaseShader::VSInstance);

	UpdateInstanceWorldMatrix(static_cast<int>(m_vInstanceCPUData.size() - 1));
}

void CObject3D::DeleteInstance(const string& Name)
{
	if (m_vInstanceCPUData.empty()) return;

	if (Name.empty())
	{
		MB_WARN("이름이 잘못되었습니다.", "인스턴스 삭제 실패");
		return;
	}
	if (m_mapInstanceNameToIndex.find(Name) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN("해당 인스턴스는 존재하지 않습니다.", "인스턴스 삭제 실패");
		return;
	}

	uint32_t iInstance{ (uint32_t)m_mapInstanceNameToIndex.at(Name) };
	uint32_t iLastInstance{ GetInstanceCount() - 1 };
	string InstanceName{ Name };
	if (iInstance == iLastInstance)
	{
		// End instance
		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(InstanceName);

		// @important
		--iInstance;

		if (m_vInstanceCPUData.empty())
		{
			ComponentRender.PtrVS = m_PtrGame->GetBaseShader(CGame::EBaseShader::VSBase);

			// @important
			m_PtrGame->DeselectInstance();
			return;
		}
	}
	else
	{
		// Non-end instance
		string LastInstanceName{ m_vInstanceCPUData[iLastInstance].Name };

		std::swap(m_vInstanceCPUData[iInstance], m_vInstanceCPUData[iLastInstance]);
		std::swap(m_vInstanceGPUData[iInstance], m_vInstanceGPUData[iLastInstance]);

		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(InstanceName);
		m_mapInstanceNameToIndex[LastInstanceName] = iInstance;

		UpdateInstanceBuffers();
	}

	// @important
	m_PtrGame->SelectInstance((int)iInstance);	
}

CObject3D::SInstanceCPUData& CObject3D::GetInstanceCPUData(int InstanceID)
{
	assert(InstanceID < m_vInstanceCPUData.size());
	return m_vInstanceCPUData[InstanceID];
}

CObject3D::SInstanceCPUData& CObject3D::GetInstanceCPUData(const string& Name)
{
	assert(m_mapInstanceNameToIndex.find(Name) != m_mapInstanceNameToIndex.end());
	return m_vInstanceCPUData[m_mapInstanceNameToIndex.at(Name)];
}

SInstanceGPUData& CObject3D::GetInstanceGPUData(int InstanceID)
{
	assert(InstanceID < m_vInstanceGPUData.size());
	return m_vInstanceGPUData[InstanceID];
}

SInstanceGPUData& CObject3D::GetInstanceGPUData(const std::string& Name)
{
	assert(m_mapInstanceNameToIndex.find(Name) != m_mapInstanceNameToIndex.end());
	return m_vInstanceGPUData[m_mapInstanceNameToIndex.at(Name)];
}

void CObject3D::CreateMeshBuffers()
{
	m_vMeshBuffers.clear();
	m_vMeshBuffers.resize(m_Model.vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		CreateMeshBuffer(iMesh, m_Model.bIsModelRigged);
	}
}

void CObject3D::CreateMeshBuffer(size_t MeshIndex, bool IsAnimated)
{
	const SMesh& Mesh{ m_Model.vMeshes[MeshIndex] };

	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertex3D) * Mesh.vVertices.size());
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vVertices[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].VertexBuffer);
	}

	if (IsAnimated)
	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertexAnimation) * Mesh.vVerticesAnimation.size());
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vVerticesAnimation[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].VertexBufferAnimation);
	}

	{
		D3D11_BUFFER_DESC BufferDesc{};
		BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(STriangle) * Mesh.vTriangles.size());
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vTriangles[0];
		m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vMeshBuffers[MeshIndex].IndexBuffer);
	}
}

void CObject3D::CreateInstanceBuffers()
{
	if (m_vInstanceCPUData.empty()) return;

	m_vInstanceBuffers.clear();
	m_vInstanceBuffers.resize(m_Model.vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		CreateInstanceBuffer(iMesh);
	}
}

void CObject3D::CreateInstanceBuffer(size_t MeshIndex)
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SInstanceGPUData) * m_vInstanceGPUData.capacity()); // @important
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vInstanceGPUData[0];
	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vInstanceBuffers[MeshIndex].Buffer);
}

void CObject3D::CreateMaterialTextures()
{
	m_vMaterialTextureSets.clear();
	
	for (CMaterialData& MaterialData : m_Model.vMaterialData)
	{
		// @important
		m_vMaterialTextureSets.emplace_back(make_unique<CMaterialTextureSet>(m_PtrDevice, m_PtrDeviceContext));

		if (MaterialData.HasAnyTexture())
		{
			m_vMaterialTextureSets.back()->CreateTextures(MaterialData);
		}
	}
}

void CObject3D::CreateMaterialTexture(size_t Index)
{
	if (Index == m_vMaterialTextureSets.size())
	{
		m_vMaterialTextureSets.emplace_back(make_unique<CMaterialTextureSet>(m_PtrDevice, m_PtrDeviceContext));
	}
	else
	{
		m_vMaterialTextureSets[Index] = make_unique<CMaterialTextureSet>(m_PtrDevice, m_PtrDeviceContext);
	}
	m_vMaterialTextureSets[Index]->CreateTextures(m_Model.vMaterialData[Index]);
}

void CObject3D::UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize)
{
	float U0{ UVOffset.x };
	float V0{ UVOffset.y };
	float U1{ U0 + UVSize.x };
	float V1{ V0 + UVSize.y };

	m_Model.vMeshes[0].vVertices[0].TexCoord = XMVectorSet(U0, V0, 0, 0);
	m_Model.vMeshes[0].vVertices[1].TexCoord = XMVectorSet(U1, V0, 0, 0);
	m_Model.vMeshes[0].vVertices[2].TexCoord = XMVectorSet(U0, V1, 0, 0);
	m_Model.vMeshes[0].vVertices[3].TexCoord = XMVectorSet(U1, V1, 0, 0);

	UpdateMeshBuffer();
}

void CObject3D::UpdateMeshBuffer(size_t MeshIndex)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vMeshBuffers[MeshIndex].VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_Model.vMeshes[MeshIndex].vVertices[0], sizeof(SVertex3D) * m_Model.vMeshes[MeshIndex].vVertices.size());

		m_PtrDeviceContext->Unmap(m_vMeshBuffers[MeshIndex].VertexBuffer.Get(), 0);
	}
}

void CObject3D::UpdateInstanceBuffers()
{
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		UpdateInstanceBuffer(iMesh);
	}
}

void CObject3D::UpdateInstanceBuffer(size_t MeshIndex)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vInstanceGPUData[0], sizeof(SInstanceGPUData) * m_vInstanceGPUData.size());

		m_PtrDeviceContext->Unmap(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0);
	}
}

void CObject3D::LimitFloatRotation(float& Value, const float Min, const float Max)
{
	if (Value > Max) Value = Min;
	if (Value < Min) Value = Max;
}

void CObject3D::UpdateWorldMatrix()
{
	LimitFloatRotation(ComponentTransform.Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(ComponentTransform.Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(ComponentTransform.Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

	if (XMVectorGetX(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetX(ComponentTransform.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetY(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetY(ComponentTransform.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetZ(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetZ(ComponentTransform.Scaling, CGame::KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(ComponentTransform.Pitch,
		ComponentTransform.Yaw, ComponentTransform.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	// @important
	float ScalingX{ XMVectorGetX(ComponentTransform.Scaling) };
	float ScalingY{ XMVectorGetY(ComponentTransform.Scaling) };
	float ScalingZ{ XMVectorGetZ(ComponentTransform.Scaling) };
	float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
	ComponentPhysics.BoundingSphere.Radius = ComponentPhysics.BoundingSphere.RadiusBias * MaxScaling;

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };

	ComponentTransform.MatrixWorld = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;
}

void CObject3D::UpdateInstanceWorldMatrix(uint32_t InstanceID)
{
	if (InstanceID >= (uint32_t)m_vInstanceCPUData.size()) return;

	// Update CPU data
	LimitFloatRotation(m_vInstanceCPUData[InstanceID].Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(m_vInstanceCPUData[InstanceID].Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(m_vInstanceCPUData[InstanceID].Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

	if (XMVectorGetX(m_vInstanceCPUData[InstanceID].Scaling) < CGame::KScalingMinLimit)
		m_vInstanceCPUData[InstanceID].Scaling = XMVectorSetX(m_vInstanceCPUData[InstanceID].Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetY(m_vInstanceCPUData[InstanceID].Scaling) < CGame::KScalingMinLimit)
		m_vInstanceCPUData[InstanceID].Scaling = XMVectorSetY(m_vInstanceCPUData[InstanceID].Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetZ(m_vInstanceCPUData[InstanceID].Scaling) < CGame::KScalingMinLimit)
		m_vInstanceCPUData[InstanceID].Scaling = XMVectorSetZ(m_vInstanceCPUData[InstanceID].Scaling, CGame::KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(m_vInstanceCPUData[InstanceID].Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(m_vInstanceCPUData[InstanceID].Pitch,
		m_vInstanceCPUData[InstanceID].Yaw, m_vInstanceCPUData[InstanceID].Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(m_vInstanceCPUData[InstanceID].Scaling) };

	// @important
	float ScalingX{ XMVectorGetX(m_vInstanceCPUData[InstanceID].Scaling) };
	float ScalingY{ XMVectorGetY(m_vInstanceCPUData[InstanceID].Scaling) };
	float ScalingZ{ XMVectorGetZ(m_vInstanceCPUData[InstanceID].Scaling) };
	float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
	m_vInstanceCPUData[InstanceID].BoundingSphere.Radius = ComponentPhysics.BoundingSphere.RadiusBias * MaxScaling;

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };
	
	// Update GPU data
	m_vInstanceGPUData[InstanceID].WorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;

	UpdateInstanceBuffers();
}

void CObject3D::UpdateAllInstancesWorldMatrix()
{
	// Update CPU data
	for (uint32_t iInstance = 0; iInstance < GetInstanceCount(); ++iInstance)
	{
		LimitFloatRotation(m_vInstanceCPUData[iInstance].Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
		LimitFloatRotation(m_vInstanceCPUData[iInstance].Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
		LimitFloatRotation(m_vInstanceCPUData[iInstance].Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

		if (XMVectorGetX(m_vInstanceCPUData[iInstance].Scaling) < CGame::KScalingMinLimit)
			m_vInstanceCPUData[iInstance].Scaling = XMVectorSetX(m_vInstanceCPUData[iInstance].Scaling, CGame::KScalingMinLimit);
		if (XMVectorGetY(m_vInstanceCPUData[iInstance].Scaling) < CGame::KScalingMinLimit)
			m_vInstanceCPUData[iInstance].Scaling = XMVectorSetY(m_vInstanceCPUData[iInstance].Scaling, CGame::KScalingMinLimit);
		if (XMVectorGetZ(m_vInstanceCPUData[iInstance].Scaling) < CGame::KScalingMinLimit)
			m_vInstanceCPUData[iInstance].Scaling = XMVectorSetZ(m_vInstanceCPUData[iInstance].Scaling, CGame::KScalingMinLimit);

		XMMATRIX Translation{ XMMatrixTranslationFromVector(m_vInstanceCPUData[iInstance].Translation) };
		XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(m_vInstanceCPUData[iInstance].Pitch,
			m_vInstanceCPUData[iInstance].Yaw, m_vInstanceCPUData[iInstance].Roll) };
		XMMATRIX Scaling{ XMMatrixScalingFromVector(m_vInstanceCPUData[iInstance].Scaling) };

		XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
		XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };

		// Update GPU data
		m_vInstanceGPUData[iInstance].WorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;
	}
	
	UpdateInstanceBuffers();
}

void CObject3D::Animate(float DeltaTime)
{
	if (!m_Model.vAnimations.size()) return;

	SModel::SAnimation& CurrentAnimation{ m_Model.vAnimations[m_CurrentAnimationID] };
	m_CurrentAnimationTick += CurrentAnimation.TicksPerSecond * DeltaTime;
	if (m_CurrentAnimationTick > CurrentAnimation.Duration)
	{
		m_CurrentAnimationTick = 0.0f;
	}

	if (m_BakedAnimationTexture)
	{
		m_CBAnimationData.bUseGPUSkinning = TRUE;
		m_CBAnimationData.AnimationID = m_CurrentAnimationID;
		m_CBAnimationData.AnimationTick = m_CurrentAnimationTick;

		m_PtrGame->UpdateCBAnimationData(m_CBAnimationData);
	}
	else
	{
		CalculateAnimatedBoneMatrices(m_Model.vAnimations[m_CurrentAnimationID], m_CurrentAnimationTick, m_Model.vNodes[0], XMMatrixIdentity());

		m_PtrGame->UpdateCBAnimationBoneMatrices(m_AnimatedBoneMatrices);
	}
}

void CObject3D::ShouldTessellate(bool Value)
{
	m_bShouldTesselate = Value;
}

void CObject3D::SetTessFactorData(const CObject3D::SCBTessFactorData& Data)
{
	m_CBTessFactorData = Data;
}

CObject3D::SCBTessFactorData& CObject3D::GetTessFactorData()
{
	return m_CBTessFactorData;
}

void CObject3D::SetDisplacementData(const CObject3D::SCBDisplacementData& Data)
{
	m_CBDisplacementData = Data;
}

CObject3D::SCBDisplacementData& CObject3D::GetDisplacementData()
{
	return m_CBDisplacementData;
}

CMaterialTextureSet* CObject3D::GetMaterialTextureSet(size_t iMaterial)
{
	if (iMaterial >= m_vMaterialTextureSets.size()) return nullptr;
	return m_vMaterialTextureSets[iMaterial].get();
}

void CObject3D::CalculateAnimatedBoneMatrices(const SModel::SAnimation& CurrentAnimation, float AnimationTick, 
	const SModel::SNode& Node, XMMATRIX ParentTransform)
{
	XMMATRIX MatrixTransformation{ Node.MatrixTransformation * ParentTransform };

	if (Node.bIsBone)
	{
		if (CurrentAnimation.vNodeAnimations.size())
		{
			if (CurrentAnimation.mapNodeAnimationNameToIndex.find(Node.Name) != CurrentAnimation.mapNodeAnimationNameToIndex.end())
			{
				size_t NodeAnimationIndex{ CurrentAnimation.mapNodeAnimationNameToIndex.at(Node.Name) };

				const SModel::SAnimation::SNodeAnimation& NodeAnimation{ CurrentAnimation.vNodeAnimations[NodeAnimationIndex] };

				XMMATRIX MatrixPosition{ XMMatrixIdentity() };
				XMMATRIX MatrixRotation{ XMMatrixIdentity() };
				XMMATRIX MatrixScaling{ XMMatrixIdentity() };

				{
					const vector<SModel::SAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vPositionKeys };
					SModel::SAnimation::SNodeAnimation::SKey KeyA{};
					SModel::SAnimation::SNodeAnimation::SKey KeyB{};
					for (uint32_t iKey = 0; iKey < (uint32_t)vKeys.size(); ++iKey)
					{
						if (vKeys[iKey].Time <= AnimationTick)
						{
							KeyA = vKeys[iKey];
							KeyB = vKeys[(iKey < (uint32_t)vKeys.size() - 1) ? iKey + 1 : 0];
						}
					}

					MatrixPosition = XMMatrixTranslationFromVector(KeyA.Value);
				}

				{
					const vector<SModel::SAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vRotationKeys };
					SModel::SAnimation::SNodeAnimation::SKey KeyA{};
					SModel::SAnimation::SNodeAnimation::SKey KeyB{};
					for (uint32_t iKey = 0; iKey < (uint32_t)vKeys.size(); ++iKey)
					{
						if (vKeys[iKey].Time <= AnimationTick)
						{
							KeyA = vKeys[iKey];
							KeyB = vKeys[(iKey < (uint32_t)vKeys.size() - 1) ? iKey + 1 : 0];
						}
					}

					MatrixRotation = XMMatrixRotationQuaternion(KeyA.Value);
				}

				{
					const vector<SModel::SAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vScalingKeys };
					SModel::SAnimation::SNodeAnimation::SKey KeyA{};
					SModel::SAnimation::SNodeAnimation::SKey KeyB{};
					for (uint32_t iKey = 0; iKey < (uint32_t)vKeys.size(); ++iKey)
					{
						if (vKeys[iKey].Time <= AnimationTick)
						{
							KeyA = vKeys[iKey];
							KeyB = vKeys[(iKey < (uint32_t)vKeys.size() - 1) ? iKey + 1 : 0];
						}
					}

					MatrixScaling = XMMatrixScalingFromVector(KeyA.Value);
				}

				MatrixTransformation = MatrixScaling * MatrixRotation * MatrixPosition * ParentTransform;
			}
		}

		// Transpose at the last moment!
		m_AnimatedBoneMatrices[Node.BoneIndex] = XMMatrixTranspose(Node.MatrixBoneOffset * MatrixTransformation);
	}
	
	if (Node.vChildNodeIndices.size())
	{
		for (auto iChild : Node.vChildNodeIndices)
		{
			CalculateAnimatedBoneMatrices(CurrentAnimation, AnimationTick, m_Model.vNodes[iChild], MatrixTransformation);
		}
	}
}

void CObject3D::Draw(bool bIgnoreOwnTexture, bool bIgnoreInstances) const
{
	if (HasBakedAnimationTexture()) m_BakedAnimationTexture->Use();

	// per model
	m_PtrGame->UpdateCBTessFactorData(m_CBTessFactorData);
	m_PtrGame->UpdateCBDisplacementData(m_CBDisplacementData);

	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_Model.vMeshes[iMesh] };
		const CMaterialData& MaterialData{ m_Model.vMaterialData[Mesh.MaterialID] };

		// per mesh
		m_PtrGame->UpdateCBMaterialData(MaterialData);

		if (MaterialData.HasAnyTexture())
		{
			if (m_Model.bUseMultipleTexturesInSingleMesh) // This bool is for CTerrain
			{
				for (const CMaterialData& MaterialDatum : m_Model.vMaterialData)
				{
					const CMaterialTextureSet* MaterialTextureSet{ m_vMaterialTextureSets[MaterialDatum.Index()].get() };
					if (MaterialDatum.HasAnyTexture() && !bIgnoreOwnTexture)
					{
						MaterialTextureSet->UseTextures();
					}
				}
			}
			else
			{
				const CMaterialTextureSet* MaterialTextureSet{ m_vMaterialTextureSets[Mesh.MaterialID].get() };
				m_PtrGame->UpdateCBMaterialData(MaterialData);
				if (MaterialData.HasAnyTexture() && !bIgnoreOwnTexture)
				{
					MaterialTextureSet->UseTextures();
				}
			}
		}

		if (ShouldTessellate())
		{
			m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		}
		else
		{
			m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		m_PtrDeviceContext->IASetIndexBuffer(m_vMeshBuffers[iMesh].IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_vMeshBuffers[iMesh].VertexBuffer.GetAddressOf(), 
			&m_vMeshBuffers[iMesh].VertexBufferStride, &m_vMeshBuffers[iMesh].VertexBufferOffset);

		if (IsRiggedModel())
		{
			m_PtrDeviceContext->IASetVertexBuffers(1, 1, m_vMeshBuffers[iMesh].VertexBufferAnimation.GetAddressOf(),
				&m_vMeshBuffers[iMesh].VertexBufferAnimationStride, &m_vMeshBuffers[iMesh].VertexBufferAnimationOffset);
		}

		if (IsInstanced() && !bIgnoreInstances)
		{
			m_PtrDeviceContext->IASetVertexBuffers(2, 1, m_vInstanceBuffers[iMesh].Buffer.GetAddressOf(),
				&m_vInstanceBuffers[iMesh].Stride, &m_vInstanceBuffers[iMesh].Offset);

			m_PtrDeviceContext->DrawIndexedInstanced(static_cast<UINT>(Mesh.vTriangles.size() * 3), static_cast<UINT>(m_vInstanceCPUData.size()), 0, 0, 0);
		}
		else
		{
			m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Mesh.vTriangles.size() * 3), 0, 0);
		}
	}
}