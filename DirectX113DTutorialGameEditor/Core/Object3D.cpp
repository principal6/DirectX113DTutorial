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

void CObject3D::Create(const SModel& Model)
{
	m_Model = Model;

	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::Create(const CMeshPorter::SMESHData& MESHData)
{
	m_Model.vMeshes = MESHData.vMeshes;
	m_Model.vMaterialData = MESHData.vMaterialData;

	ComponentPhysics.BoundingSphere = MESHData.BoundingSphereData;

	CreateMeshBuffers();
	CreateMaterialTextures();

	m_bIsCreated = true;
}

void CObject3D::CreateFromFile(const string& FileName, bool bIsModelRigged)
{
	m_ModelFileName = FileName;

	size_t found{ m_ModelFileName.find_last_of(L'.') };
	string Ext{ m_ModelFileName.substr(found) };
	for (auto& c : Ext)
	{
		c = toupper(c);
	}

	if (Ext == ".MESH")
	{
		// MESH file
		CMeshPorter MeshPorter{};
		CMeshPorter::SMESHData MESHData{};
		MeshPorter.ImportMesh(m_ModelFileName, MESHData);
		Create(MESHData);

		m_bIsCreated = true;
	}
	else
	{
		// non-MESH file
		ULONGLONG StartTimePoint{ GetTickCount64() };
		if (bIsModelRigged)
		{
			m_AssimpLoader.LoadAnimatedModelFromFile(FileName, &m_Model, m_PtrDevice, m_PtrDeviceContext);
		}
		else
		{
			m_AssimpLoader.LoadStaticModelFromFile(FileName, &m_Model, m_PtrDevice, m_PtrDeviceContext);
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
		BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SAnimationVertex) * Mesh.vAnimationVertices.size());
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;
		BufferDesc.StructureByteStride = 0;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		D3D11_SUBRESOURCE_DATA SubresourceData{};
		SubresourceData.pSysMem = &Mesh.vAnimationVertices[0];
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

void CObject3D::LoadOB3D(const std::string& OB3DFileName, bool bIsRigged)
{
	m_OB3DFileName = OB3DFileName;

	CBinaryData Object3DBinary{};
	string ReadString{};

	Object3DBinary.LoadFromFile(OB3DFileName);

	// 8B (string) Signature
	Object3DBinary.ReadSkip(8);

	// 4B (in total) Version
	uint16_t VersionMajor{ Object3DBinary.ReadUint16() };
	uint8_t VersionMinor{ Object3DBinary.ReadUint8() };
	uint8_t VersionSubminor{ Object3DBinary.ReadUint8() };
	uint32_t Version{ (uint32_t)(VersionSubminor | (VersionMinor << 8) | (VersionMajor << 16)) };

	// <@PrefString> Object3D name
	Object3DBinary.ReadStringWithPrefixedLength(m_Name);

	// 1B (bool) bContainMeshData
	bool bContainMeshData{ Object3DBinary.ReadBool() };
	if (bContainMeshData)
	{
		// 4B (uint32_t) Mesh byte count
		// ?? (byte) Mesh bytes

		size_t MeshDataByteCount{ Object3DBinary.ReadUint32() };
		vector<byte> MeshDataBinary{};
		Object3DBinary.ReadBytes(MeshDataByteCount, MeshDataBinary);
		
		CMeshPorter MeshPorter{ MeshDataBinary };
		CMeshPorter::SMESHData MeshData{};
		MeshPorter.ReadMeshData(MeshData);

		Create(MeshData);
	}
	else
	{
		// <@PrefString> Model file name
		Object3DBinary.ReadStringWithPrefixedLength(ReadString);
		CreateFromFile(ReadString, bIsRigged);
	}
	
	// ### ComponentTransform ###
	{
		Object3DBinary.ReadXMVECTOR(ComponentTransform.Translation);

		Object3DBinary.ReadFloat(ComponentTransform.Pitch);
		Object3DBinary.ReadFloat(ComponentTransform.Yaw);
		Object3DBinary.ReadFloat(ComponentTransform.Roll);

		Object3DBinary.ReadXMVECTOR(ComponentTransform.Scaling);
	}

	// ### ComponentPhysics ###
	{
		Object3DBinary.ReadBool(ComponentPhysics.bIsPickable);

		Object3DBinary.ReadXMVECTOR(ComponentPhysics.BoundingSphere.CenterOffset);
		Object3DBinary.ReadFloat(ComponentPhysics.BoundingSphere.RadiusBias);
	}

	// ### ComponentRender ###
	{
		Object3DBinary.ReadBool(ComponentRender.bIsTransparent);
		Object3DBinary.ReadBool(ComponentRender.bShouldAnimate);
	}

	// ##### Animation data #####
	if (Version >= 0x10001)
	{
		// 1B (bool) bIsModelRigged
		Object3DBinary.ReadBool(m_Model.bIsModelRigged);

		// 4B (uint32_t) Tree node count
		m_Model.vTreeNodes.resize(Object3DBinary.ReadUint32());
		for (auto& Node : m_Model.vTreeNodes)
		{
			// <@PrefString> Node name
			Object3DBinary.ReadStringWithPrefixedLength(Node.Name);

			// 4B (int32_t) Node index
			Object3DBinary.ReadInt32(Node.Index);

			// 1B (bool) bIsBone
			Object3DBinary.ReadBool(Node.bIsBone);

			// 4B (uint32_t) Bone index
			Object3DBinary.ReadUint32(Node.BoneIndex);

			// 64B (XMMATRIX) Bone offset matrix
			Object3DBinary.ReadXMMATRIX(Node.MatrixBoneOffset);

			// 64B (XMMATRIX) Transformation matrix
			Object3DBinary.ReadXMMATRIX(Node.MatrixTransformation);

			// 4B (int32_t) Parent node index
			Object3DBinary.ReadInt32(Node.ParentNodeIndex);

			// 4B (uint32_t) Blend weight count
			Node.vBlendWeights.resize(Object3DBinary.ReadUint32());
			for (auto& BlendWeight : Node.vBlendWeights)
			{
				// 4B (uint32_t) Mesh index
				Object3DBinary.ReadUint32(BlendWeight.MeshIndex);

				// 4B (uint32_t) Vertex ID
				Object3DBinary.ReadUint32(BlendWeight.VertexID);

				// 4B (float) Weight
				Object3DBinary.ReadFloat(BlendWeight.Weight);
			}

			// 4B (uint32_t) Child node count
			Node.vChildNodeIndices.resize(Object3DBinary.ReadUint32());
			for (auto& ChildNodeIndex : Node.vChildNodeIndices)
			{
				// 4B (int32_t) Child node index
				Object3DBinary.ReadInt32(ChildNodeIndex);
			}
		}

		// @important
		for (const auto& Node : m_Model.vTreeNodes) 
		{
			m_Model.umapTreeNodeNameToIndex[Node.Name] = Node.Index;
		}

		// 4B (uint32_t) Model bone count
		Object3DBinary.ReadUint32(m_Model.ModelBoneCount);

		// 4B (uint32_t) Animation count
		m_Model.vAnimations.resize(Object3DBinary.ReadUint32());
		for (auto& Animation : m_Model.vAnimations)
		{
			// <@PrefString> Animation name
			Object3DBinary.ReadStringWithPrefixedLength(Animation.Name);

			// 4B (float) Duration
			Object3DBinary.ReadFloat(Animation.Duration);

			// 4B (float) Ticks per second
			Object3DBinary.ReadFloat(Animation.TicksPerSecond);

			// 4B (uint32_t) Node animation count
			Animation.vNodeAnimations.resize(Object3DBinary.ReadUint32());
			for (auto& NodeAnimation : Animation.vNodeAnimations)
			{
				// 4B (uint32_t) Node animation index
				Object3DBinary.ReadUint32(NodeAnimation.Index);

				// <@PrefString> Node animation name
				Object3DBinary.ReadStringWithPrefixedLength(NodeAnimation.Name);

				// 4B (uint32_t) Position key count
				NodeAnimation.vPositionKeys.resize(Object3DBinary.ReadUint32());
				for (auto& PositionKey : NodeAnimation.vPositionKeys)
				{
					// 4B (float) Time
					Object3DBinary.ReadFloat(PositionKey.Time);

					// 16B (XMVECTOR) Value
					Object3DBinary.ReadXMVECTOR(PositionKey.Value);
				}

				// 4B (uint32_t) Rotation key count
				NodeAnimation.vRotationKeys.resize(Object3DBinary.ReadUint32());
				for (auto& RotationKey : NodeAnimation.vRotationKeys)
				{
					// 4B (float) Time
					Object3DBinary.ReadFloat(RotationKey.Time);

					// 16B (XMVECTOR) Value
					Object3DBinary.ReadXMVECTOR(RotationKey.Value);
				}

				// 4B (uint32_t) Scaling key count
				NodeAnimation.vScalingKeys.resize(Object3DBinary.ReadUint32());
				for (auto& ScalingKey : NodeAnimation.vScalingKeys)
				{
					// 4B (float) Time
					Object3DBinary.ReadFloat(ScalingKey.Time);

					// 16B (XMVECTOR) Value
					Object3DBinary.ReadXMVECTOR(ScalingKey.Value);
				}
			}

			// @important
			for (const auto& NodeAnimation : Animation.vNodeAnimations)
			{
				Animation.umapNodeAnimationNameToIndex[NodeAnimation.Name] = NodeAnimation.Index;
			}
		}
	}

	// ### Instance ###
	{
		size_t InstanceCount{ Object3DBinary.ReadUint32() };

		vector<SInstanceCPUData> vInstanceCPUData{};
		vInstanceCPUData.resize(InstanceCount);
		for (size_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			SInstanceCPUData& InstanceCPUData{ vInstanceCPUData[iInstance] };

			Object3DBinary.ReadStringWithPrefixedLength(InstanceCPUData.Name);

			Object3DBinary.ReadXMVECTOR(InstanceCPUData.Translation);
			Object3DBinary.ReadFloat(InstanceCPUData.Pitch);
			Object3DBinary.ReadFloat(InstanceCPUData.Yaw);
			Object3DBinary.ReadFloat(InstanceCPUData.Roll);
			Object3DBinary.ReadXMVECTOR(InstanceCPUData.Scaling);

			Object3DBinary.ReadXMVECTOR(InstanceCPUData.BoundingSphere.CenterOffset);
			Object3DBinary.ReadFloat(InstanceCPUData.BoundingSphere.RadiusBias);
		}

		CreateInstances(vInstanceCPUData);
	}
}

void CObject3D::SaveOB3D(const std::string& OB3DFileName)
{
	static uint16_t KVersionMajor{ 0x0001 };
	static uint8_t KVersionMinor{ 0x00 };
	static uint8_t KVersionSubminor{ 0x01 };

	m_OB3DFileName = OB3DFileName;

	CBinaryData Object3DBinary{};

	// 8B (string) Signature
	Object3DBinary.WriteString("KJW_OB3D", 8);

	// 4B (in total) Version
	Object3DBinary.WriteUint16(KVersionMajor);
	Object3DBinary.WriteUint8(KVersionMinor);
	Object3DBinary.WriteUint8(KVersionSubminor);

	// <@PrefString> Object3D name
	Object3DBinary.WriteStringWithPrefixedLength(m_Name);

	// 1B (bool) bContainMeshData
	// <@PrefString> Model file name
	if (m_ModelFileName.empty())
	{
		Object3DBinary.WriteBool(true);

		// 4B (uint32_t) Mesh byte count
		// ?? (byte) Mesh bytes
		CMeshPorter MeshPorter{};
		MeshPorter.WriteMeshData(CMeshPorter::SMESHData(m_Model.vMeshes, m_Model.vMaterialData));
		vector<byte> MeshBytes{ MeshPorter.GetBytes() };

		Object3DBinary.WriteUint32((uint32_t)MeshBytes.size());
		Object3DBinary.AppendBytes(MeshBytes);
	}
	else
	{
		Object3DBinary.WriteBool(false);

		Object3DBinary.WriteStringWithPrefixedLength(m_ModelFileName);
	}

	// ### ComponentTransform ###
	{
		Object3DBinary.WriteXMVECTOR(ComponentTransform.Translation);

		Object3DBinary.WriteFloat(ComponentTransform.Pitch);
		Object3DBinary.WriteFloat(ComponentTransform.Yaw);
		Object3DBinary.WriteFloat(ComponentTransform.Roll);

		Object3DBinary.WriteXMVECTOR(ComponentTransform.Scaling);
	}
	
	// ### ComponentPhysics ###
	{
		Object3DBinary.WriteBool(ComponentPhysics.bIsPickable);
		
		Object3DBinary.WriteXMVECTOR(ComponentPhysics.BoundingSphere.CenterOffset);
		Object3DBinary.WriteFloat(ComponentPhysics.BoundingSphere.RadiusBias);
	}

	// ### ComponentRender ###
	{
		Object3DBinary.WriteBool(ComponentRender.bIsTransparent);
		Object3DBinary.WriteBool(ComponentRender.bShouldAnimate);
	}

	// ##### Animation data #####
	{
		// 1B (bool) bIsModelRigged
		Object3DBinary.WriteBool(m_Model.bIsModelRigged);

		// 4B (uint32_t) Tree node count
		Object3DBinary.WriteUint32((uint32_t)m_Model.vTreeNodes.size());
		for (const auto& Node : m_Model.vTreeNodes)
		{
			// <@PrefString> Node name
			Object3DBinary.WriteStringWithPrefixedLength(Node.Name);

			// 4B (int32_t) Node index
			Object3DBinary.WriteInt32(Node.Index);

			// 1B (bool) bIsBone
			Object3DBinary.WriteBool(Node.bIsBone);

			// 4B (uint32_t) Bone index
			Object3DBinary.WriteUint32(Node.BoneIndex);

			// 64B (XMMATRIX) Bone offset matrix
			Object3DBinary.WriteXMMATRIX(Node.MatrixBoneOffset);

			// 64B (XMMATRIX) Transformation matrix
			Object3DBinary.WriteXMMATRIX(Node.MatrixTransformation);

			// 4B (int32_t) Parent node index
			Object3DBinary.WriteInt32(Node.ParentNodeIndex);

			// 4B (uint32_t) Blend weight count
			Object3DBinary.WriteUint32((uint32_t)Node.vBlendWeights.size());
			for (const auto& BlendWeight : Node.vBlendWeights)
			{
				// 4B (uint32_t) Mesh index
				Object3DBinary.WriteUint32(BlendWeight.MeshIndex);
				
				// 4B (uint32_t) Vertex ID
				Object3DBinary.WriteUint32(BlendWeight.VertexID);

				// 4B (float) Weight
				Object3DBinary.WriteFloat(BlendWeight.Weight);
			}

			// 4B (uint32_t) Child node count
			Object3DBinary.WriteUint32((uint32_t)Node.vChildNodeIndices.size());
			for (const auto& ChildNodeIndex : Node.vChildNodeIndices)
			{
				// 4B (int32_t) Child node index
				Object3DBinary.WriteInt32(ChildNodeIndex);
			}
		}

		// 4B (uint32_t) Model bone count
		Object3DBinary.WriteUint32(m_Model.ModelBoneCount);

		// 4B (uint32_t) Animation count
		Object3DBinary.WriteUint32((uint32_t)m_Model.vAnimations.size());
		for (const auto& Animation : m_Model.vAnimations)
		{
			// <@PrefString> Animation name
			Object3DBinary.WriteStringWithPrefixedLength(Animation.Name);

			// 4B (float) Duration
			Object3DBinary.WriteFloat(Animation.Duration);

			// 4B (float) Ticks per second
			Object3DBinary.WriteFloat(Animation.TicksPerSecond);
			
			// 4B (uint32_t) Node animation count
			Object3DBinary.WriteUint32((uint32_t)Animation.vNodeAnimations.size());
			for (const auto& NodeAnimation : Animation.vNodeAnimations)
			{
				// 4B (uint32_t) Node animation index
				Object3DBinary.WriteUint32(NodeAnimation.Index);

				// <@PrefString> Node name
				Object3DBinary.WriteStringWithPrefixedLength(NodeAnimation.Name);

				// 4B (uint32_t) Position key count
				Object3DBinary.WriteUint32((uint32_t)NodeAnimation.vPositionKeys.size());
				for (const auto& PositionKey : NodeAnimation.vPositionKeys)
				{
					// 4B (float) Time
					Object3DBinary.WriteFloat(PositionKey.Time);

					// 16B (XMVECTOR) Value
					Object3DBinary.WriteXMVECTOR(PositionKey.Value);
				}

				// 4B (uint32_t) Rotation key count
				Object3DBinary.WriteUint32((uint32_t)NodeAnimation.vRotationKeys.size());
				for (const auto& RotationKey : NodeAnimation.vRotationKeys)
				{
					// 4B (float) Time
					Object3DBinary.WriteFloat(RotationKey.Time);

					// 16B (XMVECTOR) Value
					Object3DBinary.WriteXMVECTOR(RotationKey.Value);
				}

				// 4B (uint32_t) Scaling key count
				Object3DBinary.WriteUint32((uint32_t)NodeAnimation.vScalingKeys.size());
				for (const auto& ScalingKey : NodeAnimation.vScalingKeys)
				{
					// 4B (float) Time
					Object3DBinary.WriteFloat(ScalingKey.Time);

					// 16B (XMVECTOR) Value
					Object3DBinary.WriteXMVECTOR(ScalingKey.Value);
				}
			}
		}
	}

	// ### Instance ###
	{
		Object3DBinary.WriteUint32((uint32_t)m_vInstanceCPUData.size());

		for (const auto& InstanceCPUData : m_vInstanceCPUData)
		{
			Object3DBinary.WriteStringWithPrefixedLength(InstanceCPUData.Name);
			
			Object3DBinary.WriteXMVECTOR(InstanceCPUData.Translation);
			Object3DBinary.WriteFloat(InstanceCPUData.Pitch);
			Object3DBinary.WriteFloat(InstanceCPUData.Yaw);
			Object3DBinary.WriteFloat(InstanceCPUData.Roll);
			Object3DBinary.WriteXMVECTOR(InstanceCPUData.Scaling);

			Object3DBinary.WriteXMVECTOR(InstanceCPUData.BoundingSphere.CenterOffset);
			Object3DBinary.WriteFloat(InstanceCPUData.BoundingSphere.RadiusBias);
		}
	}

	Object3DBinary.SaveToFile(OB3DFileName);
}

bool CObject3D::HasAnimations()
{
	return (m_Model.vAnimations.size()) ? true : false;
}

void CObject3D::AddAnimationFromFile(const string& FileName, const string& AnimationName)
{
	if (!m_Model.bIsModelRigged) return;

	m_AssimpLoader.AddAnimationFromFile(FileName, &m_Model);
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
		MB_WARN("�ִϸ��̼��� �������� �ʽ��ϴ�.", "�ִϸ��̼� �̸� ���� ����");
		return;
	}

	if (Name.size() > KMaxAnimationNameLength)
	{
		MB_WARN(("�ִϸ��̼� �̸��� �ִ� " + to_string(KMaxAnimationNameLength) + "���Դϴ�.").c_str(), "�ִϸ��̼� �̸� ���� ����");
		return;
	}

	m_Model.vAnimations[ID].Name = Name;
}

const string& CObject3D::GetAnimationName(int ID) const
{
	return m_Model.vAnimations[ID].Name;
}

const CObject3D::SCBAnimationData& CObject3D::GetAnimationData() const
{
	return m_CBAnimationData;
}

const DirectX::XMMATRIX* CObject3D::GetAnimationBoneMatrices() const
{
	return m_AnimatedBoneMatrices;
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
			CalculateAnimatedBoneMatrices(Animation, (float)iTime, m_Model.vTreeNodes[0], XMMatrixIdentity());

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

	m_BakedAnimationTexture->SaveDDSFile(FileName);
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

void CObject3D::CreateInstances(size_t InstanceCount)
{
	if (InstanceCount <= 0) return;
	if (InstanceCount >= 100'000) return; // TOO MANY INSTANCES

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

	CreateInstanceBuffers();

	UpdateAllInstancesWorldMatrix();
}

void CObject3D::CreateInstances(const std::vector<SInstanceCPUData>& vInstanceData)
{
	if (vInstanceData.empty()) return;

	m_mapInstanceNameToIndex.clear();

	m_vInstanceCPUData = vInstanceData;

	size_t iInstance{};
	for (auto& InstanceCPUData : m_vInstanceCPUData)
	{
		float ScalingX{ XMVectorGetX(InstanceCPUData.Scaling) };
		float ScalingY{ XMVectorGetY(InstanceCPUData.Scaling) };
		float ScalingZ{ XMVectorGetZ(InstanceCPUData.Scaling) };
		float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
		InstanceCPUData.BoundingSphere.Radius = InstanceCPUData.BoundingSphere.RadiusBias * MaxScaling; // @important

		m_mapInstanceNameToIndex[InstanceCPUData.Name] = iInstance;
		++iInstance;
	}

	m_vInstanceGPUData.clear();
	m_vInstanceGPUData.resize(vInstanceData.size());

	CreateInstanceBuffers();

	UpdateAllInstancesWorldMatrix();
}

bool CObject3D::InsertInstance()
{
	size_t InstanceCount{ GetInstanceCount() };
	string AutoGeneratedName{ "inst" + to_string(InstanceCount) };

	if (m_mapInstanceNameToIndex.find(AutoGeneratedName) != m_mapInstanceNameToIndex.end())
	{
		for (size_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			AutoGeneratedName = "inst" + to_string(iInstance);
			if (m_mapInstanceNameToIndex.find(AutoGeneratedName) == m_mapInstanceNameToIndex.end()) break;
		}
	}

	if (AutoGeneratedName.length() >= SInstanceCPUData::KMaxNameLengthZeroTerminated)
	{
		MB_WARN(("�ν��Ͻ� �̸� [" + AutoGeneratedName + "] �� �ִ� ����(" +
			to_string(SInstanceCPUData::KMaxNameLengthZeroTerminated - 1) + " ��)�� �Ѿ�\n�ν��Ͻ� ������ �����߽��ϴ�.").c_str(), "�ν��Ͻ� ���� ����");
		return false;
	}

	return InsertInstance(AutoGeneratedName);
}

bool CObject3D::InsertInstance(const string& InstanceName)
{
	if (m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("�ش� �̸�(" + InstanceName + ")�� �ν��Ͻ��� �̹� �����մϴ�.").c_str(), "�ν��Ͻ� ���� ����");
		return false;
	}

	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	std::string LimitedName{ InstanceName };
	if (LimitedName.length() >= SInstanceCPUData::KMaxNameLengthZeroTerminated)
	{
		MB_WARN(("�ν��Ͻ� �̸� [" + LimitedName + "] �� �ִ� ����(" + to_string(SInstanceCPUData::KMaxNameLengthZeroTerminated - 1) +
			" ��)�� �Ѿ� �߷��� ����˴ϴ�.").c_str(), "�̸� ���� ����");

		LimitedName.resize(SInstanceCPUData::KMaxNameLengthZeroTerminated - 1);
	}

	m_vInstanceCPUData.emplace_back();
	m_vInstanceCPUData.back().Name = LimitedName;
	m_vInstanceCPUData.back().Translation = ComponentTransform.Translation;
	m_vInstanceCPUData.back().Scaling = ComponentTransform.Scaling;
	m_vInstanceCPUData.back().Pitch = ComponentTransform.Pitch;
	m_vInstanceCPUData.back().Yaw = ComponentTransform.Yaw;
	m_vInstanceCPUData.back().Roll = ComponentTransform.Roll;
	m_vInstanceCPUData.back().BoundingSphere = ComponentPhysics.BoundingSphere; // @important
	m_mapInstanceNameToIndex[LimitedName] = m_vInstanceCPUData.size() - 1;

	m_vInstanceGPUData.emplace_back();

	if (m_vInstanceCPUData.size() == 1) CreateInstanceBuffers();
	if (bShouldRecreateInstanceBuffer) CreateInstanceBuffers();

	UpdateInstanceWorldMatrix(LimitedName);

	return true;
}

bool CObject3D::ChangeInstanceName(const std::string& OldName, const std::string& NewName)
{
	if (m_mapInstanceNameToIndex.find(OldName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("���� �̸� (" + OldName + ")�� �ν��Ͻ��� �������� �ʽ��ϴ�.").c_str(), "�̸� ���� ����");
		return false;
	}
	if (m_mapInstanceNameToIndex.find(NewName) != m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("�� �̸� (" + NewName + ")�� �ν��Ͻ��� �̹� �����մϴ�.").c_str(), "�̸� ���� ����");
		return false;
	}

	string SavedOldName{ OldName };
	size_t iInstance{ m_mapInstanceNameToIndex.at(OldName) };
	m_vInstanceCPUData[iInstance].Name = NewName;
	m_mapInstanceNameToIndex.erase(OldName);
	m_mapInstanceNameToIndex[NewName] = iInstance;

	return true;
}

void CObject3D::DeleteInstance(const string& InstanceName)
{
	if (m_vInstanceCPUData.empty()) return;

	if (InstanceName.empty())
	{
		MB_WARN("�̸��� �߸��Ǿ����ϴ�.", "�ν��Ͻ� ���� ����");
		return;
	}
	if (m_mapInstanceNameToIndex.find(InstanceName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN("�ش� �ν��Ͻ��� �������� �ʽ��ϴ�.", "�ν��Ͻ� ���� ����");
		return;
	}

	string SavedName{ InstanceName };
	size_t iInstance{ m_mapInstanceNameToIndex.at(SavedName) };
	size_t iLastInstance{ GetInstanceCount() - 1 };
	if (iInstance == iLastInstance)
	{
		// End instance
		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(SavedName);

		// @important
		--iInstance;
	}
	else
	{
		// Non-end instance
		string LastInstanceName{ m_vInstanceCPUData[iLastInstance].Name };

		std::swap(m_vInstanceCPUData[iInstance], m_vInstanceCPUData[iLastInstance]);
		std::swap(m_vInstanceGPUData[iInstance], m_vInstanceGPUData[iLastInstance]);

		m_vInstanceCPUData.pop_back();
		m_vInstanceGPUData.pop_back();

		m_mapInstanceNameToIndex.erase(SavedName);
		m_mapInstanceNameToIndex[LastInstanceName] = iInstance;

		UpdateInstanceBuffers();
	}
}

void CObject3D::ClearInstances()
{
	m_vInstanceCPUData.clear();
	m_vInstanceGPUData.clear();
	m_mapInstanceNameToIndex.clear();
}

SInstanceCPUData& CObject3D::GetInstanceCPUData(const string& InstanceName)
{
	assert(m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end());
	return m_vInstanceCPUData[GetInstanceID(InstanceName)];
}

SInstanceCPUData& CObject3D::GetLastInstanceCPUData()
{
	return m_vInstanceCPUData.back();
}

const std::vector<SInstanceCPUData>& CObject3D::GetInstanceCPUDataVector() const
{
	return m_vInstanceCPUData;
}

SInstanceGPUData& CObject3D::GetInstanceGPUData(const std::string& InstanceName)
{
	assert(m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end());
	return m_vInstanceGPUData[GetInstanceID(InstanceName)];
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

void CObject3D::UpdateInstanceBuffers()
{
	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		UpdateInstanceBuffer(iMesh);
	}
}

void CObject3D::UpdateInstanceBuffer(size_t MeshIndex)
{
	if (m_vInstanceGPUData.empty()) return;
	if (m_vInstanceBuffers.empty()) return;
	if (!m_vInstanceBuffers[MeshIndex].Buffer) return;

	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vInstanceGPUData[0], sizeof(SInstanceGPUData) * m_vInstanceGPUData.size());

		m_PtrDeviceContext->Unmap(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0);
	}
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

void CObject3D::UpdateInstanceWorldMatrix(const std::string& InstanceName)
{
	SInstanceCPUData& InstanceCPUData{ m_vInstanceCPUData[GetInstanceID(InstanceName)] };
	SInstanceGPUData& InstanceGPUData{ m_vInstanceGPUData[GetInstanceID(InstanceName)] };

	// Update CPU data
	LimitFloatRotation(InstanceCPUData.Pitch, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(InstanceCPUData.Yaw, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);
	LimitFloatRotation(InstanceCPUData.Roll, CGame::KRotationMinLimit, CGame::KRotationMaxLimit);

	if (XMVectorGetX(InstanceCPUData.Scaling) < CGame::KScalingMinLimit)
		InstanceCPUData.Scaling = XMVectorSetX(InstanceCPUData.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetY(InstanceCPUData.Scaling) < CGame::KScalingMinLimit)
		InstanceCPUData.Scaling = XMVectorSetY(InstanceCPUData.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetZ(InstanceCPUData.Scaling) < CGame::KScalingMinLimit)
		InstanceCPUData.Scaling = XMVectorSetZ(InstanceCPUData.Scaling, CGame::KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(InstanceCPUData.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(InstanceCPUData.Pitch,
		InstanceCPUData.Yaw, InstanceCPUData.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(InstanceCPUData.Scaling) };

	// @important
	float ScalingX{ XMVectorGetX(InstanceCPUData.Scaling) };
	float ScalingY{ XMVectorGetY(InstanceCPUData.Scaling) };
	float ScalingZ{ XMVectorGetZ(InstanceCPUData.Scaling) };
	float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
	InstanceCPUData.BoundingSphere.Radius = ComponentPhysics.BoundingSphere.RadiusBias * MaxScaling;

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(ComponentPhysics.BoundingSphere.CenterOffset) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-ComponentPhysics.BoundingSphere.CenterOffset) };

	// Update GPU data
	InstanceGPUData.WorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;

	UpdateInstanceBuffers();
}

void CObject3D::UpdateInstanceWorldMatrix(const std::string& InstanceName, const XMMATRIX& WorldMatrix)
{
	if (InstanceName.empty()) return;

	m_vInstanceGPUData[GetInstanceID(InstanceName)].WorldMatrix = WorldMatrix;

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

void CObject3D::SetInstanceHighlight(const std::string& InstanceName, bool bShouldHighlight)
{
	m_vInstanceGPUData[GetInstanceID(InstanceName)].IsHighlighted = (bShouldHighlight) ? 1.0f : 0.0f;
}

void CObject3D::SetAllInstancesHighlightOff()
{
	for (auto& InstanceGPUData : m_vInstanceGPUData)
	{
		InstanceGPUData.IsHighlighted = 0.0f;
	}
}

size_t CObject3D::GetInstanceID(const std::string& InstanceName) const
{
	return m_mapInstanceNameToIndex.at(InstanceName);
}

void CObject3D::ShouldTessellate(bool Value)
{
	m_bShouldTesselate = Value;
}

void CObject3D::SetTessFactorData(const CObject3D::SCBTessFactorData& Data)
{
	m_CBTessFactorData = Data;
}

const CObject3D::SCBTessFactorData& CObject3D::GetTessFactorData() const
{
	return m_CBTessFactorData;
}

void CObject3D::SetDisplacementData(const CObject3D::SCBDisplacementData& Data)
{
	m_CBDisplacementData = Data;
}

const CObject3D::SCBDisplacementData& CObject3D::GetDisplacementData() const
{
	return m_CBDisplacementData;
}

CMeshPorter::SMESHData CObject3D::GetMESHData() const
{
	return CMeshPorter::SMESHData(m_Model.vMeshes, m_Model.vMaterialData, ComponentPhysics.BoundingSphere);
}

void CObject3D::SetName(const std::string& Name)
{
	m_Name = Name;
}

CMaterialTextureSet* CObject3D::GetMaterialTextureSet(size_t iMaterial)
{
	if (iMaterial >= m_vMaterialTextureSets.size()) return nullptr;
	return m_vMaterialTextureSets[iMaterial].get();
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
	}
	else
	{
		CalculateAnimatedBoneMatrices(m_Model.vAnimations[m_CurrentAnimationID], m_CurrentAnimationTick, m_Model.vTreeNodes[0], XMMatrixIdentity());
	}
}

void CObject3D::Draw(bool bIgnoreOwnTexture, bool bIgnoreInstances) const
{
	if (HasBakedAnimationTexture()) m_BakedAnimationTexture->Use();

	for (size_t iMesh = 0; iMesh < m_Model.vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_Model.vMeshes[iMesh] };
		const CMaterialData& MaterialData{ m_Model.vMaterialData[Mesh.MaterialID] };

		// per mesh
		m_PtrGame->UpdateCBMaterialData(MaterialData, (uint32_t)m_Model.vMaterialData.size());

		if (MaterialData.HasAnyTexture() && !bIgnoreOwnTexture)
		{
			if (m_Model.bUseMultipleTexturesInSingleMesh) // This bool is for CTerrain
			{
				for (const CMaterialData& MaterialDatum : m_Model.vMaterialData)
				{
					const CMaterialTextureSet* MaterialTextureSet{ m_vMaterialTextureSets[MaterialDatum.Index()].get() };
					MaterialTextureSet->UseTextures();
				}
			}
			else
			{
				const CMaterialTextureSet* MaterialTextureSet{ m_vMaterialTextureSets[Mesh.MaterialID].get() };
				MaterialTextureSet->UseTextures();
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

		if (IsRigged())
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

void CObject3D::CalculateAnimatedBoneMatrices(const SModel::SAnimation& CurrentAnimation, float AnimationTick,
	const SModel::STreeNode& Node, XMMATRIX ParentTransform)
{
	XMMATRIX MatrixTransformation{ Node.MatrixTransformation * ParentTransform };

	if (Node.bIsBone)
	{
		if (CurrentAnimation.vNodeAnimations.size())
		{
			if (CurrentAnimation.umapNodeAnimationNameToIndex.find(Node.Name) != CurrentAnimation.umapNodeAnimationNameToIndex.end())
			{
				size_t NodeAnimationIndex{ CurrentAnimation.umapNodeAnimationNameToIndex.at(Node.Name) };

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
			CalculateAnimatedBoneMatrices(CurrentAnimation, AnimationTick, m_Model.vTreeNodes[iChild], MatrixTransformation);
		}
	}
}
