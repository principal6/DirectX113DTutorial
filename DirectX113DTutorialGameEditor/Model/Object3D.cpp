#include "Object3D.h"
#include "AssimpLoader.h"
#include "MeshPorter.h"
#include "../Core/BinaryData.h"
#include "../Core/ConstantBuffer.h"
#include "../Core/Material.h"
#include "../Core/Shader.h"

using std::max;
using std::min;
using std::vector;
using std::string;
using std::to_string;
using std::make_unique;

CObject3D::CObject3D(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CObject3D::~CObject3D()
{
}

void CObject3D::Create(const SMESHData& MESHData)
{
	m_Model = make_unique<SMESHData>(MESHData);

	m_OuterBoundingSphere = MESHData.EditorBoundingSphereData;

	InitializeModelData();

	m_bIsCreated = true;
}

void CObject3D::Create(const SMesh& Mesh, const CMaterialData& MaterialData)
{
	SMESHData Model{};
	Model.vMeshes.emplace_back(Mesh);
	Model.vMaterialData.emplace_back(MaterialData);
	Model.EditorBoundingSphereData = m_OuterBoundingSphere;
	Create(Model);
}

void CObject3D::Create(const SMesh& Mesh)
{
	Create(Mesh, CMaterialData());
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
		SMESHData MESHData{};
		MeshPorter.ImportMESH(m_ModelFileName, MESHData);
		
		Create(MESHData);
	}
	else
	{
		// non-MESH file
		CAssimpLoader AssimpLoader{};
		SMESHData Model{};
		ULONGLONG StartTimePoint{ GetTickCount64() };
		if (bIsModelRigged)
		{
			AssimpLoader.LoadAnimatedModelFromFile(FileName, &Model, m_PtrDevice, m_PtrDeviceContext);
		}
		else
		{
			AssimpLoader.LoadStaticModelFromFile(FileName, &Model, m_PtrDevice, m_PtrDeviceContext);
		}
		OutputDebugString(("- Model [" + FileName + "] loaded. [" + to_string(GetTickCount64() - StartTimePoint) + "] elapsed.\n").c_str());

		Create(Model);

		CalculateEditorBoundingSphereData();
	}

	m_Model->bIsModelRigged = bIsModelRigged;
}

void CObject3D::InitializeModelData()
{
	_CreateMeshBuffers();
	_CreateMaterialTextures();
	_CreateConstantBuffers();
	_InitializeAnimationData();

	for (const CMaterialData& Material : m_Model->vMaterialData)
	{
		// @important
		if (Material.HasTexture(ETextureType::OpacityTexture))
		{
			m_ComponentRender.bIsTransparent = true;
			break;
		}
	}
}

void CObject3D::_CreateMeshBuffers()
{
	m_vMeshBuffers.clear();
	m_vMeshBuffers.resize(m_Model->vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model->vMeshes.size(); ++iMesh)
	{
		__CreateMeshBuffer(iMesh, m_Model->bIsModelRigged);
	}
}

void CObject3D::__CreateMeshBuffer(size_t MeshIndex, bool IsAnimated)
{
	const SMesh& Mesh{ m_Model->vMeshes[MeshIndex] };

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

void CObject3D::_CreateMaterialTextures()
{
	m_vMaterialTextureSets.clear();

	for (CMaterialData& MaterialData : m_Model->vMaterialData)
	{
		// @important
		m_vMaterialTextureSets.emplace_back(make_unique<CMaterialTextureSet>(m_PtrDevice, m_PtrDeviceContext));

		if (MaterialData.HasAnyTexture())
		{
			m_vMaterialTextureSets.back()->CreateTextures(MaterialData);
		}
	}
}

void CObject3D::__CreateMaterialTexture(size_t Index)
{
	if (Index == m_vMaterialTextureSets.size())
	{
		m_vMaterialTextureSets.emplace_back(make_unique<CMaterialTextureSet>(m_PtrDevice, m_PtrDeviceContext));
	}
	else
	{
		m_vMaterialTextureSets[Index] = make_unique<CMaterialTextureSet>(m_PtrDevice, m_PtrDeviceContext);
	}
	m_vMaterialTextureSets[Index]->CreateTextures(m_Model->vMaterialData[Index]);
}

void CObject3D::_CreateConstantBuffers()
{
	m_CBMaterial = make_unique<CConstantBuffer>(m_PtrDevice, m_PtrDeviceContext, &m_CBMaterialData, sizeof(m_CBMaterialData));
	m_CBMaterial->Create();
}

void CObject3D::CalculateEditorBoundingSphereData()
{
	size_t VertexCount{};
	XMVECTOR VertexCenter{};
	for (const auto& Mesh : m_Model->vMeshes)
	{
		for (const auto& Vertex : Mesh.vVertices)
		{
			VertexCenter += Vertex.Position;
			++VertexCount;
		}
	}
	VertexCenter /= static_cast<float>(VertexCount);

	float MaxLengthSqaure{};
	for (const auto& Mesh : m_Model->vMeshes)
	{
		for (const auto& Vertex : Mesh.vVertices)
		{
			float LengthSquare{ XMVectorGetX(XMVector3LengthSq(Vertex.Position - VertexCenter)) };
			if (LengthSquare > MaxLengthSqaure) MaxLengthSqaure = LengthSquare;
		}
	}

	m_OuterBoundingSphere.Data.BS.RadiusBias = m_Model->EditorBoundingSphereData.Data.BS.RadiusBias = sqrt(MaxLengthSqaure);
	m_OuterBoundingSphere.Center = m_Model->EditorBoundingSphereData.Center = VertexCenter;
}

void CObject3D::_InitializeAnimationData()
{
	if (m_Model->vAnimations.empty())
	{
		m_vAnimationBehaviorStartTicks.clear();
		return;
	}

	m_vAnimationBehaviorStartTicks.resize(m_Model->vAnimations.size());
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

	// 1B (bool) bIsPickable
	if (Version >= 0x10005) Object3DBinary.ReadBool(m_bIsPickable);

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
		SMESHData MeshData{};
		MeshPorter.ReadMESHData(MeshData);

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
		Object3DBinary.ReadXMVECTOR(m_ComponentTransform.Translation);

		Object3DBinary.ReadFloat(m_ComponentTransform.Pitch);
		Object3DBinary.ReadFloat(m_ComponentTransform.Yaw);
		Object3DBinary.ReadFloat(m_ComponentTransform.Roll);

		Object3DBinary.ReadXMVECTOR(m_ComponentTransform.Scaling);
	}

	// ### ComponentPhysics ###
	{
		if (Version < 0x10005) Object3DBinary.ReadBool(); // ComponentPhysics.bIsPickable

		Object3DBinary.ReadXMVECTOR(m_OuterBoundingSphere.Center);
		Object3DBinary.ReadFloat(m_OuterBoundingSphere.Data.BS.RadiusBias);

		if (Version >= 0x10002)
		{
			size_t BoundingVolumeCount{ Object3DBinary.ReadUint32() };
			m_vInnerBoundingVolumes.resize(BoundingVolumeCount);

			for (auto& BoundingVolume : m_vInnerBoundingVolumes)
			{
				Object3DBinary.ReadXMVECTOR(BoundingVolume.Center);

				BoundingVolume.eType = (EBoundingVolumeType)Object3DBinary.ReadUint8();

				// @important: union data
				Object3DBinary.ReadFloat(BoundingVolume.Data.AABBHalfSizes.x); // BoundingVolume.Data.BS.Radius;
				Object3DBinary.ReadFloat(BoundingVolume.Data.AABBHalfSizes.y); // BoundingVolume.Data.BS.RadiusBias;
				Object3DBinary.ReadFloat(BoundingVolume.Data.AABBHalfSizes.z);
			}
		}
	}

	// ### ComponentRender ###
	{
		Object3DBinary.ReadBool(m_ComponentRender.bIsTransparent);
		if (Version < 0x10005) Object3DBinary.ReadBool(); // ComponentRender.bShouldAnimate
	}

	// ### Instance ###
	{
		size_t InstanceCount{ Object3DBinary.ReadUint32() };

		vector<SObject3DInstanceCPUData> vInstanceCPUData{};
		vector<SObject3DInstanceGPUData> vInstanceGPUData{};
		vInstanceCPUData.resize(InstanceCount);
		vInstanceGPUData.resize(InstanceCount);
		for (size_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
		{
			auto& InstanceCPUData{ vInstanceCPUData[iInstance] };
			auto& InstanceGPUData{ vInstanceGPUData[iInstance] };

			Object3DBinary.ReadStringWithPrefixedLength(InstanceCPUData.Name);

			Object3DBinary.ReadXMVECTOR(InstanceCPUData.Transform.Translation);
			Object3DBinary.ReadFloat(InstanceCPUData.Transform.Pitch);
			Object3DBinary.ReadFloat(InstanceCPUData.Transform.Yaw);
			Object3DBinary.ReadFloat(InstanceCPUData.Transform.Roll);
			Object3DBinary.ReadXMVECTOR(InstanceCPUData.Transform.Scaling);

			if (Version >= 0x10005)
			{
				Object3DBinary.ReadFloat(InstanceCPUData.Physics.InverseMass);
				Object3DBinary.ReadXMVECTOR(InstanceCPUData.Physics.LinearAcceleration);
				Object3DBinary.ReadXMVECTOR(InstanceCPUData.Physics.LinearVelocity);

				Object3DBinary.ReadUint32(InstanceGPUData.CurrAnimID);
			}

			Object3DBinary.ReadXMVECTOR(InstanceCPUData.EditorBoundingSphere.Center);
			Object3DBinary.ReadFloat(InstanceCPUData.EditorBoundingSphere.Data.BS.RadiusBias);
		}

		CreateInstances(vInstanceCPUData, vInstanceGPUData);
	}

	// ### Animation ###
	{
		if (Version >= 0x10001)
		{
			if (Version < 0x10005)
			{
				// 4B (int32_t) Current animation ID
				m_CurrentAnimationID = (uint32_t)Object3DBinary.ReadInt32();
			}
			else
			{
				// 4B (uint32_t) Current animation ID
				Object3DBinary.ReadUint32(m_CurrentAnimationID);
			}
		}
		if (Version >= 0x10003)
		{
			if (Version >= 0x10005)
			{
				// <@PrefString> Baked animation texture file name
				string BakedAnimationTextureFileName{};
				Object3DBinary.ReadStringWithPrefixedLength(BakedAnimationTextureFileName);
				LoadBakedAnimationTexture(BakedAnimationTextureFileName);
			}

			uint32_t RegisteredAnimationType{};
			for (size_t iAnimation = 0; iAnimation < GetAnimationCount(); ++iAnimation)
			{
				// 4B (uint32_t, enum) Registered animation type
				Object3DBinary.ReadUint32(RegisteredAnimationType);

				EAnimationRegistrationType eRegisteredAnimationType{ (EAnimationRegistrationType)RegisteredAnimationType };
				RegisterAnimation(static_cast<int32_t>(iAnimation), eRegisteredAnimationType);

				if (Version >= 0x10004)
				{
					// 4B (float) Behavior start tick
					m_vAnimationBehaviorStartTicks[iAnimation] = Object3DBinary.ReadFloat();
				}
				
				// 4B (float) Ticks per second (overriding the data in MESH)
				if (Version >= 0x10005)
				{
					Object3DBinary.ReadFloat(m_Model->vAnimations[iAnimation].TicksPerSecond);
				}
			}
		}
	}
}

void CObject3D::SaveOB3D(const std::string& OB3DFileName)
{
	static constexpr uint16_t KVersionMajor{ 0x0001 };
	static constexpr uint8_t KVersionMinor{ 0x00 };
	static constexpr uint8_t KVersionSubminor{ 0x05 };
	uint32_t Version{ (uint32_t)(KVersionSubminor | (KVersionMinor << 8) | (KVersionMajor << 16)) };

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

	// 1B (bool) bIsPickable
	if (Version >= 0x10005) Object3DBinary.WriteBool(m_bIsPickable);

	// 1B (bool) bContainMeshData
	// <@PrefString> Model file name
	{
		size_t ExtensionDot{ m_ModelFileName.find_last_of('.') };
		string Extension{ m_ModelFileName.substr(ExtensionDot + 1) };
		for (char& ch : Extension)
		{
			ch = toupper(ch);
		}
		if (m_ModelFileName.empty())
		{
			Object3DBinary.WriteBool(true);

			// 4B (uint32_t) Mesh byte count
			// ?? (byte) Mesh bytes
			CMeshPorter MeshPorter{};
			MeshPorter.WriteMESHData(*m_Model);
			vector<byte> MeshBytes{ MeshPorter.GetBytes() };

			Object3DBinary.WriteUint32((uint32_t)MeshBytes.size());
			Object3DBinary.AppendBytes(MeshBytes);
		}
		else
		{
			Object3DBinary.WriteBool(false);

			Object3DBinary.WriteStringWithPrefixedLength(m_ModelFileName);
		}
	}

	// ### ComponentTransform ###
	{
		Object3DBinary.WriteXMVECTOR(m_ComponentTransform.Translation);

		Object3DBinary.WriteFloat(m_ComponentTransform.Pitch);
		Object3DBinary.WriteFloat(m_ComponentTransform.Yaw);
		Object3DBinary.WriteFloat(m_ComponentTransform.Roll);

		Object3DBinary.WriteXMVECTOR(m_ComponentTransform.Scaling);
	}
	
	// ### ComponentPhysics ###
	{
		if (Version < 0x10005) Object3DBinary.WriteBool(true); // ComponentPhysics.bIsPickable
		
		Object3DBinary.WriteXMVECTOR(m_OuterBoundingSphere.Center);
		Object3DBinary.WriteFloat(m_OuterBoundingSphere.Data.BS.RadiusBias);

		if (Version >= 0x10002)
		{
			Object3DBinary.WriteUint32((uint32_t)m_vInnerBoundingVolumes.size());

			for (const auto& BoundingVolume : m_vInnerBoundingVolumes)
			{
				Object3DBinary.WriteXMVECTOR(BoundingVolume.Center);

				Object3DBinary.WriteUint8((uint8_t)BoundingVolume.eType);
				
				// @important: union data
				Object3DBinary.WriteFloat(BoundingVolume.Data.AABBHalfSizes.x); // BoundingVolume.Data.BS.Radius;
				Object3DBinary.WriteFloat(BoundingVolume.Data.AABBHalfSizes.y); // BoundingVolume.Data.BS.RadiusBias;
				Object3DBinary.WriteFloat(BoundingVolume.Data.AABBHalfSizes.z);
			}
		}
	}

	// ### ComponentRender ###
	{
		Object3DBinary.WriteBool(m_ComponentRender.bIsTransparent);
		if (Version < 0x10005) Object3DBinary.WriteBool(false); // ComponentRender.bShouldAnimate
	}

	// ### Instance ###
	{
		Object3DBinary.WriteUint32((uint32_t)m_vInstanceCPUData.size());

		for (size_t iInstance = 0; iInstance < GetInstanceCount(); ++iInstance)
		{
			auto& InstanceCPUData{ m_vInstanceCPUData[iInstance] };
			auto& InstanceGPUData{ m_vInstanceGPUData[iInstance] };

			Object3DBinary.WriteStringWithPrefixedLength(InstanceCPUData.Name);
			
			Object3DBinary.WriteXMVECTOR(InstanceCPUData.Transform.Translation);
			Object3DBinary.WriteFloat(InstanceCPUData.Transform.Pitch);
			Object3DBinary.WriteFloat(InstanceCPUData.Transform.Yaw);
			Object3DBinary.WriteFloat(InstanceCPUData.Transform.Roll);
			Object3DBinary.WriteXMVECTOR(InstanceCPUData.Transform.Scaling);

			if (Version >= 0x10005)
			{
				Object3DBinary.WriteFloat(InstanceCPUData.Physics.InverseMass);
				Object3DBinary.WriteXMVECTOR(InstanceCPUData.Physics.LinearAcceleration);
				Object3DBinary.WriteXMVECTOR(InstanceCPUData.Physics.LinearVelocity);

				Object3DBinary.WriteUint32(InstanceGPUData.CurrAnimID);
			}

			Object3DBinary.WriteXMVECTOR(InstanceCPUData.EditorBoundingSphere.Center);
			Object3DBinary.WriteFloat(InstanceCPUData.EditorBoundingSphere.Data.BS.RadiusBias);
		}
	}

	// ### Animation ###
	{
		if (Version >= 0x10001)
		{
			if (Version < 0x10005)
			{
				// 4B (int32_t) Current animation ID
				Object3DBinary.WriteInt32((int32_t)m_CurrentAnimationID);
			}
			else
			{
				// 4B (uint32_t) Current animation ID
				Object3DBinary.WriteUint32(m_CurrentAnimationID);
			}
			
		}
		if (Version >= 0x10003)
		{
			if (Version >= 0x10005)
			{
				// <@PrefString> Baked animation texture file name
				Object3DBinary.WriteStringWithPrefixedLength((m_BakedAnimationTexture) ? m_BakedAnimationTexture->GetFileName() : "");
			}

			for (size_t iAnimation = 0; iAnimation < GetAnimationCount(); ++iAnimation)
			{
				EAnimationRegistrationType eRegisteredAnimationType{ GetRegisteredAnimationType(static_cast<int32_t>(iAnimation)) };

				// 4B (uint32_t, enum) Registered animation type
				Object3DBinary.WriteUint32((uint32_t)eRegisteredAnimationType);

				if (Version >= 0x10004)
				{
					// 4B (float) Behavior start tick
					Object3DBinary.WriteFloat(m_vAnimationBehaviorStartTicks[iAnimation]);
				}

				// 4B (float) Ticks per second (overriding the data in MESH)
				if (Version >= 0x10005)
				{
					Object3DBinary.WriteFloat(m_Model->vAnimations[iAnimation].TicksPerSecond);
				}
			}
		}
	}

	Object3DBinary.SaveToFile(OB3DFileName);
}

void CObject3D::ExportEmbeddedTextures(const std::string& Directory)
{
	if (!m_Model) return;

	size_t iMaterial{};
	for (const auto& MaterialTextureSet : m_vMaterialTextureSets)
	{
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::BaseColorTexture, Directory);
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::NormalTexture, Directory);
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::OpacityTexture, Directory);
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::RoughnessTexture, Directory);
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::MetalnessTexture, Directory);
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::AmbientOcclusionTexture, Directory);
		ExportEmbeddedTexture(MaterialTextureSet.get(), m_Model->vMaterialData[iMaterial], ETextureType::DisplacementTexture, Directory);
		
		++iMaterial;
	}
}

void CObject3D::ExportEmbeddedTexture(CMaterialTextureSet* const MaterialTextureSet, CMaterialData& MaterialData,
	ETextureType eTextureType, const std::string& Directory)
{
	if (MaterialTextureSet->HasTexture(eTextureType))
	{
		const auto& Texture{ MaterialTextureSet->GetTexture(eTextureType) };
		const auto& FileName{ Texture.GetFileName() };
		if (FileName.empty())
		{
			const string& InternalFileName{ MaterialData.GetTextureData(eTextureType).InternalFileName };
			Texture.SaveWICFile(Directory + InternalFileName);
			MaterialData.SetTextureFileName(eTextureType, Directory + InternalFileName);
		}
	}
}

void CObject3D::AddAnimationFromFile(const string& FileName, const string& AnimationName)
{
	if (!m_Model->bIsModelRigged) return;

	CAssimpLoader AssimpLoader{};
	bool bWasTreeEmpty{ m_Model->vTreeNodes.empty() };

	AssimpLoader.AddAnimationFromFile(FileName, m_Model.get());
	m_Model->vAnimations.back().Name = AnimationName;

	if (bWasTreeEmpty)
	{
		InitializeModelData();
	}
	else
	{
		_InitializeAnimationData();
	}
}

void CObject3D::RegisterAnimation(uint32_t AnimationID, EAnimationRegistrationType eRegisteredType)
{
	if (eRegisteredType == EAnimationRegistrationType::NotRegistered) return;

	if (m_umapRegisteredAnimationTypeToIndex.find(eRegisteredType) == m_umapRegisteredAnimationTypeToIndex.end())
	{
		m_vRegisteredAnimationIDs.emplace_back(AnimationID);
		m_umapRegisteredAnimationTypeToIndex[eRegisteredType] = m_vRegisteredAnimationIDs.size() - 1;
		m_umapRegisteredAnimationIndexToType[AnimationID] = eRegisteredType;
	}
	else
	{
		size_t At{ m_umapRegisteredAnimationTypeToIndex.at(eRegisteredType) };
		m_vRegisteredAnimationIDs[At] = AnimationID;
		m_umapRegisteredAnimationIndexToType[AnimationID] = eRegisteredType;
	}
}

void CObject3D::SetAnimationName(uint32_t AnimationID, const string& Name)
{
	if (m_Model->vAnimations.empty())
	{
		MB_WARN("애니메이션이 존재하지 않습니다.", "애니메이션 이름 지정 실패");
		return;
	}

	if (Name.size() > KMaxAnimationNameLength)
	{
		MB_WARN(("애니메이션 이름은 최대 " + to_string(KMaxAnimationNameLength) + "자입니다.").c_str(), "애니메이션 이름 지정 실패");
		return;
	}

	m_Model->vAnimations[AnimationID].Name = Name;
}

void CObject3D::SetAnimationTicksPerSecond(uint32_t AnimationID, float TPS)
{
	if (m_Model->vAnimations.empty())
	{
		MB_WARN("애니메이션이 존재하지 않습니다.", "애니메이션 이름 지정 실패");
		return;
	}

	m_Model->vAnimations[AnimationID].TicksPerSecond = TPS;
}

void CObject3D::SetAnimationBehaviorStartTick(uint32_t AnimationID, float BehaviorStartTick)
{
	m_vAnimationBehaviorStartTicks[AnimationID] = BehaviorStartTick;
}

bool CObject3D::HasAnimations() const
{
	return (m_Model->vAnimations.size()) ? true : false;
}

size_t CObject3D::GetAnimationCount() const
{
	return m_Model->vAnimations.size();
}

const string& CObject3D::GetAnimationName(uint32_t AnimationID) const
{
	return m_Model->vAnimations[AnimationID].Name;
}

const CObject3D::SCBAnimationData& CObject3D::GetAnimationData() const
{
	return m_CBAnimationData;
}

EAnimationRegistrationType CObject3D::GetRegisteredAnimationType(uint32_t AnimationID) const
{
	if (m_umapRegisteredAnimationIndexToType.find(AnimationID) != m_umapRegisteredAnimationIndexToType.end())
	{
		return m_umapRegisteredAnimationIndexToType.at(AnimationID);
	}
	return EAnimationRegistrationType::NotRegistered;
}

float CObject3D::GetAnimationTicksPerSecond(uint32_t AnimationID) const
{
	return m_Model->vAnimations[AnimationID].TicksPerSecond;
}

float CObject3D::GetAnimationBehaviorStartTick(uint32_t AnimationID) const
{
	return m_vAnimationBehaviorStartTicks[AnimationID];
}

float CObject3D::GetAnimationDuration(uint32_t AnimationID) const
{
	return m_Model->vAnimations[AnimationID].Duration;
}

const DirectX::XMMATRIX* CObject3D::GetAnimationBoneMatrices() const
{
	return m_AnimatedBoneMatrices;
}

bool CObject3D::IsCurrentAnimationRegisteredAs(const SObjectIdentifier& Identifier, EAnimationRegistrationType eRegistrationType) const
{
	if (Identifier.InstanceName.size())
	{
		return IsInstanceCurrentAnimationRegisteredAs(Identifier.InstanceName, eRegistrationType);
	}
	return IsObjectCurrentAnimationRegisteredAs(eRegistrationType);
}

float CObject3D::GetAnimationTick(const SObjectIdentifier& Identifier) const
{
	if (Identifier.InstanceName.size())
	{
		return GetInstanceAnimationTick(Identifier.InstanceName);
	}
	else
	{
		return GetObjectAnimationTick();
	}
}

uint32_t CObject3D::GetAnimationID(const SObjectIdentifier& Identifier) const
{
	if (Identifier.InstanceName.size())
	{
		return GetInstanceAnimationID(Identifier.InstanceName);
	}
	return GetObjectAnimationID();
}

size_t CObject3D::GetAnimationPlayCount(const SObjectIdentifier& Identifier) const
{
	if (Identifier.InstanceName.size())
	{
		return GetInstanceAnimationPlayCount(Identifier.InstanceName);
	}
	return GetObjectAnimationPlayCount();
}

float CObject3D::GetCurrentAnimationBehaviorStartTick(const SObjectIdentifier& Identifier) const
{
	return GetAnimationBehaviorStartTick(GetAnimationID(Identifier));
}

bool CObject3D::IsObjectCurrentAnimationRegisteredAs(EAnimationRegistrationType eRegistrationType) const
{
	return GetRegisteredAnimationType(m_CurrentAnimationID) == eRegistrationType;
}

float CObject3D::GetObjectAnimationTick() const
{
	return m_AnimationTick;
}

uint32_t CObject3D::GetObjectAnimationID() const
{
	return m_CurrentAnimationID;
}

size_t CObject3D::GetObjectAnimationPlayCount() const
{
	return m_CurrentAnimationPlayCount;
}

float CObject3D::GetObjectCurrentAnimationBehaviorStartTick() const
{
	return GetAnimationBehaviorStartTick(GetObjectAnimationID());
}

bool CObject3D::IsInstanceCurrentAnimationRegisteredAs(const std::string& InstanceName, EAnimationRegistrationType eRegistrationType) const
{
	return GetRegisteredAnimationType(GetInstanceGPUData(InstanceName).CurrAnimID) == eRegistrationType;
}

float CObject3D::GetInstanceAnimationTick(const std::string& InstanceName) const
{
	return GetInstanceGPUData(InstanceName).AnimTick;
}

uint32_t CObject3D::GetInstanceAnimationID(const std::string& InstanceName) const
{
	return GetInstanceGPUData(InstanceName).CurrAnimID;
}

size_t CObject3D::GetInstanceAnimationPlayCount(const std::string& InstanceName) const
{
	return GetInstanceCPUData(InstanceName).CurrAnimPlayCount;
}

float CObject3D::GetInstanceCurrentAnimationBehaviorStartTick(const std::string& InstanceName) const
{
	return GetAnimationBehaviorStartTick(GetInstanceAnimationID(InstanceName));
}

void CObject3D::SetAnimation(const SObjectIdentifier& Identifier, uint32_t AnimationID, 
	EAnimationOption eAnimationOption, bool bShouldIgnoreCurrentAnimation)
{
	if (Identifier.InstanceName.size())
	{
		SetInstanceAnimation(Identifier.InstanceName, AnimationID, eAnimationOption, bShouldIgnoreCurrentAnimation);
	}
	else
	{
		SetObjectAnimation(AnimationID, eAnimationOption, bShouldIgnoreCurrentAnimation);
	}
}

void CObject3D::SetAnimation(const SObjectIdentifier& Identifier, EAnimationRegistrationType eRegisteredType, 
	EAnimationOption eAnimationOption, bool bShouldIgnoreCurrentAnimation)
{
	if (Identifier.InstanceName.size())
	{
		SetInstanceAnimation(Identifier.InstanceName, eRegisteredType, eAnimationOption, bShouldIgnoreCurrentAnimation);
	}
	else
	{
		SetObjectAnimation(eRegisteredType, eAnimationOption, bShouldIgnoreCurrentAnimation);
	}
}

void CObject3D::SetObjectAnimation(uint32_t AnimationID, EAnimationOption eAnimationOption, bool bShouldIgnoreCurrentAnimation)
{
	size_t AnimationCount{ GetAnimationCount() };
	if (AnimationCount == 0) return;

	AnimationID = min(AnimationID, static_cast<uint32_t>(AnimationCount - 1));

	if (!bShouldIgnoreCurrentAnimation)
	{
		if (m_CurrentAnimationID == AnimationID) return;
		if (m_CurrentAnimationPlayCount == 0) return;
	}

	m_AnimationTick = 0; // @important
	m_CurrentAnimationID = AnimationID;
	m_CurrentAnimationPlayCount = 0;
	m_eCurrentAnimationOption = eAnimationOption;
}

void CObject3D::SetObjectAnimation(EAnimationRegistrationType eRegisteredType, EAnimationOption eAnimationOption, bool bShouldIgnoreCurrentAnimation)
{
	if (m_umapRegisteredAnimationTypeToIndex.find(eRegisteredType) == m_umapRegisteredAnimationTypeToIndex.end()) return;
	size_t RegisteredAnimationIndex{ m_umapRegisteredAnimationTypeToIndex.at(eRegisteredType) };
	uint32_t AnimationID{ m_vRegisteredAnimationIDs[RegisteredAnimationIndex] };
	SetObjectAnimation(AnimationID, eAnimationOption, bShouldIgnoreCurrentAnimation);
}

void CObject3D::SetInstanceAnimation(const std::string& InstanceName, uint32_t AnimationID, EAnimationOption eAnimationOption, bool bShouldIgnoreCurrentAnimation)
{
	size_t AnimationCount{ GetAnimationCount() };
	if (AnimationCount == 0) return;

	AnimationID = min(AnimationID, static_cast<uint32_t>(AnimationCount - 1));

	auto& InstanceCPUData{ GetInstanceCPUData(InstanceName) };
	auto& InstanceGPUData{ GetInstanceGPUData(InstanceName) };
	if (!bShouldIgnoreCurrentAnimation)
	{
		if (InstanceGPUData.CurrAnimID == AnimationID) return;
		if (InstanceCPUData.CurrAnimPlayCount == 0) return;
	}

	InstanceGPUData.AnimTick = 0; // @important
	InstanceGPUData.CurrAnimID = AnimationID;
	InstanceCPUData.CurrAnimPlayCount = 0;
	InstanceCPUData.eCurrAnimOption = eAnimationOption;
}

void CObject3D::SetInstanceAnimation(const std::string& InstanceName, EAnimationRegistrationType eRegisteredType, EAnimationOption eAnimationOption, bool bShouldIgnoreCurrentAnimation)
{
	if (m_umapRegisteredAnimationTypeToIndex.find(eRegisteredType) == m_umapRegisteredAnimationTypeToIndex.end()) return;
	size_t RegisteredAnimationIndex{ m_umapRegisteredAnimationTypeToIndex.at(eRegisteredType) };
	uint32_t AnimationID{ m_vRegisteredAnimationIDs[RegisteredAnimationIndex] };
	SetInstanceAnimation(InstanceName, AnimationID, eAnimationOption, bShouldIgnoreCurrentAnimation);
}

const SComponentTransform& CObject3D::GetTransform(const SObjectIdentifier& Identifier) const
{
	if (Identifier.InstanceName.size())
	{
		return GetInstanceTransform(Identifier.InstanceName);
	}
	return GetTransform();
}

const SComponentPhysics& CObject3D::GetPhysics(const SObjectIdentifier& Identifier) const
{
	if (Identifier.InstanceName.size())
	{
		return GetInstancePhysics(Identifier.InstanceName);
	}
	return GetPhysics();
}

CObject3D::EFlagsRendering CObject3D::GetRenderingFlags() const
{
	return m_eFlagsRendering;
}

const SBoundingVolume& CObject3D::GetOuterBoundingSphere(const SObjectIdentifier& Identifier) const
{
	if (Identifier.InstanceName.size())
	{
		return GetInstanceOuterBoundingSphere(Identifier.InstanceName);
	}
	return GetOuterBoundingSphere();
}

void CObject3D::TranslateTo(const SObjectIdentifier& Identifier, const XMVECTOR& Prime)
{
	if (Identifier.InstanceName.size())
	{
		TranslateInstanceTo(Identifier.InstanceName, Prime);
		return;
	}
	TranslateTo(Prime);
}

void CObject3D::RotatePitchTo(const SObjectIdentifier& Identifier, float Prime)
{
	if (Identifier.InstanceName.size())
	{
		RotateInstancePitchTo(Identifier.InstanceName, Prime);
		return;
	}
	RotatePitchTo(Prime);
}

void CObject3D::RotateYawTo(const SObjectIdentifier& Identifier, float Prime)
{
	if (Identifier.InstanceName.size())
	{
		RotateInstanceYawTo(Identifier.InstanceName, Prime);
		return;
	}
	RotateYawTo(Prime);
}

void CObject3D::RotateRollTo(const SObjectIdentifier& Identifier, float Prime)
{
	if (Identifier.InstanceName.size())
	{
		RotateInstanceRollTo(Identifier.InstanceName, Prime);
		return;
	}
	RotateRollTo(Prime);
}

void CObject3D::ScaleTo(const SObjectIdentifier& Identifier, const XMVECTOR& Prime)
{
	if (Identifier.InstanceName.size())
	{
		ScaleInstanceTo(Identifier.InstanceName, Prime);
		return;
	}
	ScaleTo(Prime);
}

void CObject3D::Translate(const SObjectIdentifier& Identifier, const XMVECTOR& Delta)
{
	if (Identifier.InstanceName.size())
	{
		TranslateInstance(Identifier.InstanceName, Delta);
		return;
	}
	Translate(Delta);
}

void CObject3D::RotatePitch(const SObjectIdentifier& Identifier, float Delta)
{
	if (Identifier.InstanceName.size())
	{
		RotateInstancePitch(Identifier.InstanceName, Delta);
		return;
	}
	RotatePitch(Delta);
}

void CObject3D::RotateYaw(const SObjectIdentifier& Identifier, float Delta)
{
	if (Identifier.InstanceName.size())
	{
		RotateInstanceYaw(Identifier.InstanceName, Delta);
		return;
	}
	RotateYaw(Delta);
}

void CObject3D::RotateRoll(const SObjectIdentifier& Identifier, float Delta)
{
	if (Identifier.InstanceName.size())
	{
		RotateInstanceRoll(Identifier.InstanceName, Delta);
		return;
	}
	RotateRoll(Delta);
}

void CObject3D::Scale(const SObjectIdentifier& Identifier, const XMVECTOR& Delta)
{
	if (Identifier.InstanceName.size())
	{
		ScaleInstance(Identifier.InstanceName, Delta);
		return;
	}
	Scale(Delta);
}

void CObject3D::SetLinearAcceleration(const SObjectIdentifier& Identifier, const XMVECTOR& Prime)
{
	if (Identifier.InstanceName.size())
	{
		GetInstanceCPUData(Identifier.InstanceName).Physics.LinearAcceleration = Prime;
		return;
	}
	SetLinearAcceleration(Prime);
}

void CObject3D::SetLinearVelocity(const SObjectIdentifier& Identifier, const XMVECTOR& Prime)
{
	if (Identifier.InstanceName.size())
	{
		GetInstanceCPUData(Identifier.InstanceName).Physics.LinearVelocity = Prime;
		return;
	}
	SetLinearVelocity(Prime);
}

void CObject3D::AddLinearAcceleration(const SObjectIdentifier& Identifier, const XMVECTOR& Delta)
{
	if (Identifier.InstanceName.size())
	{
		GetInstanceCPUData(Identifier.InstanceName).Physics.LinearAcceleration += Delta;
		return;
	}
	AddLinearAcceleration(Delta);
}

void CObject3D::AddLinearVelocity(const SObjectIdentifier& Identifier, const XMVECTOR& Delta)
{
	if (Identifier.InstanceName.size())
	{
		GetInstanceCPUData(Identifier.InstanceName).Physics.LinearVelocity += Delta;
		return;
	}
	AddLinearVelocity(Delta);
}

void CObject3D::SetTransform(const SComponentTransform& NewValue)
{
	m_ComponentTransform = NewValue;
}

void CObject3D::SetPhysics(const SComponentPhysics& NewValue)
{
	m_ComponentPhysics = NewValue;
}

void CObject3D::SetRender(const SComponentRender& NewValue)
{
	m_ComponentRender = NewValue;
}

const SComponentTransform& CObject3D::GetTransform() const
{
	return m_ComponentTransform;
}

const SComponentPhysics& CObject3D::GetPhysics() const
{
	return m_ComponentPhysics;
}

const SComponentRender& CObject3D::GetRender() const
{
	return m_ComponentRender;
}

bool CObject3D::HasInnerBoundingVolumes() const
{
	return (m_vInnerBoundingVolumes.size() ? true : false);
}

const std::vector<SBoundingVolume>& CObject3D::GetInnerBoundingVolumeVector() const
{
	return m_vInnerBoundingVolumes;
}

std::vector<SBoundingVolume>& CObject3D::GetInnerBoundingVolumeVector()
{
	return m_vInnerBoundingVolumes;
}

void CObject3D::TranslateTo(const XMVECTOR& Prime)
{
	m_ComponentTransform.Translation = Prime;
}

void CObject3D::RotatePitchTo(float Prime)
{
	m_ComponentTransform.Pitch = Prime;
}

void CObject3D::RotateYawTo(float Prime)
{
	m_ComponentTransform.Yaw = Prime;
}

void CObject3D::RotateRollTo(float Prime)
{
	m_ComponentTransform.Roll = Prime;
}

void CObject3D::ScaleTo(const XMVECTOR& Prime)
{
	m_ComponentTransform.Scaling = Prime;
}

void CObject3D::Translate(const XMVECTOR& Delta)
{
	m_ComponentTransform.Translation += Delta;
}

void CObject3D::RotatePitch(float Delta)
{
	m_ComponentTransform.Pitch += Delta;
}

void CObject3D::RotateYaw(float Delta)
{
	m_ComponentTransform.Yaw += Delta;
}

void CObject3D::RotateRoll(float Delta)
{
	m_ComponentTransform.Roll += Delta;
}

void CObject3D::Scale(const XMVECTOR& Delta)
{
	m_ComponentTransform.Scaling += Delta;
}

void CObject3D::SetLinearAcceleration(const XMVECTOR& Prime)
{
	m_ComponentPhysics.LinearAcceleration = Prime;
}

void CObject3D::SetLinearVelocity(const XMVECTOR& Prime)
{
	m_ComponentPhysics.LinearVelocity = Prime;
}

void CObject3D::AddLinearAcceleration(const XMVECTOR& Delta)
{
	m_ComponentPhysics.LinearAcceleration += Delta;
}

void CObject3D::AddLinearVelocity(const XMVECTOR& Delta)
{
	m_ComponentPhysics.LinearVelocity += Delta;
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
	if (m_Model->vAnimations.empty()) return;

	// TODO: corret sign-ness ??

	int32_t AnimationCount{ (int32_t)GetAnimationCount() };
	int32_t TextureHeight{ KAnimationTextureReservedHeight };
	vector<int32_t> vAnimationHeights{};
	for (const auto& Animation : m_Model->vAnimations)
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
			&m_Model->vAnimations[iAnimation].Duration, sizeof(float));
		memcpy(&vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].B,
			&m_Model->vAnimations[iAnimation].TicksPerSecond, sizeof(float));
		//A

		// RGBA = 4 floats = 16 chars!
		memcpy(&vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].R,
			m_Model->vAnimations[iAnimation].Name.c_str(), sizeof(char) * 16);

		AnimationHeightSum += vAnimationHeights[iAnimation];
	}

	for (int32_t iAnimation = 0; iAnimation < (int32_t)m_Model->vAnimations.size(); ++iAnimation)
	{
		float fAnimationOffset{};
		memcpy(&fAnimationOffset, &vRawData[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].R, sizeof(float));

		const int32_t KAnimationYOffset{ (int32_t)fAnimationOffset };
		const int32_t KAnimationOffset{ (int32_t)((int64_t)KAnimationYOffset * KAnimationTextureWidth) };
		const SMeshAnimation& Animation{ m_Model->vAnimations[iAnimation] };
		
		int32_t Duration{ (int32_t)Animation.Duration + 1 };
		for (int32_t iTime = 0; iTime < Duration; ++iTime)
		{
			const int32_t KTimeOffset{ (int32_t)((int64_t)iTime * KAnimationTextureWidth) };
			CalculateAnimatedBoneMatrices(Animation, (float)iTime, m_Model->vTreeNodes[0], XMMatrixIdentity());

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
	if (FileName.empty()) return;

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
		m_Model->vAnimations.clear();
		m_Model->vAnimations.resize((size_t)vPixels[1].R);

		for (int32_t iAnimation = 0; iAnimation < (int32_t)m_Model->vAnimations.size(); ++iAnimation)
		{
			m_Model->vAnimations[iAnimation].Duration = vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].G;
			m_Model->vAnimations[iAnimation].TicksPerSecond = vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 0].B;

			float NameR{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].R };
			float NameG{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].G };
			float NameB{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].B };
			float NameA{ vPixels[(int64_t)KAnimationTextureReservedFirstPixelCount + (int64_t)iAnimation * 2 + 1].A };

			char Name[16]{};
			memcpy(&Name[0], &NameR, 4);
			memcpy(&Name[4], &NameG, 4);
			memcpy(&Name[8], &NameB, 4);
			memcpy(&Name[12], &NameA, 4);

			m_Model->vAnimations[iAnimation].Name = Name;
		}

		m_PtrDeviceContext->Unmap(ReadableAnimationTexture.Get(), 0);
	}

	m_BakedAnimationTexture->SetShaderType(EShaderType::VertexShader);

	m_bIsBakedAnimationLoaded = true;

	_InitializeAnimationData();
}

void CObject3D::AddMaterial(const CMaterialData& MaterialData)
{
	m_Model->vMaterialData.emplace_back(MaterialData);
	m_Model->vMaterialData.back().Index(m_Model->vMaterialData.size() - 1);

	__CreateMaterialTexture(m_Model->vMaterialData.size() - 1);
}

void CObject3D::SetMaterial(size_t Index, const CMaterialData& MaterialData)
{
	assert(Index < m_Model->vMaterialData.size());

	m_Model->vMaterialData[Index] = MaterialData;
	m_Model->vMaterialData[Index].Index(Index);

	__CreateMaterialTexture(Index);
}

size_t CObject3D::GetMaterialCount() const
{
	return m_Model->vMaterialData.size();
}

void CObject3D::ShouldIgnoreSceneMaterial(bool bShouldIgnore)
{
	if (m_Model) m_Model->bIgnoreSceneMaterial = bShouldIgnore;
}

bool CObject3D::ShouldIgnoreSceneMaterial() const
{
	return m_Model->bIgnoreSceneMaterial;
}

void CObject3D::CreateInstances(const std::vector<SObject3DInstanceCPUData>& vInstanceCPUData, const std::vector<SObject3DInstanceGPUData>& vInstanceGPUData)
{
	if (vInstanceCPUData.empty()) return;
	if (vInstanceGPUData.empty())
	{
		m_vInstanceGPUData.resize(vInstanceCPUData.size());
	}
	if (vInstanceCPUData.size() != vInstanceGPUData.size()) return;

	m_vInstanceCPUData = vInstanceCPUData;
	m_vInstanceGPUData = vInstanceGPUData;

	m_mapInstanceNameToIndex.clear();
	for (size_t iInstance = 0; iInstance < m_vInstanceCPUData.size(); ++iInstance)
	{
		auto& InstanceCPUData{ m_vInstanceCPUData[iInstance] };
		float ScalingX{ XMVectorGetX(InstanceCPUData.Transform.Scaling) };
		float ScalingY{ XMVectorGetY(InstanceCPUData.Transform.Scaling) };
		float ScalingZ{ XMVectorGetZ(InstanceCPUData.Transform.Scaling) };
		float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
		InstanceCPUData.EditorBoundingSphere.Data.BS.Radius = InstanceCPUData.EditorBoundingSphere.Data.BS.RadiusBias * MaxScaling; // @important

		m_mapInstanceNameToIndex[InstanceCPUData.Name] = iInstance;
	}

	CreateInstanceBuffers();

	UpdateAllInstances();
}

void CObject3D::CreateInstances(const std::vector<SObject3DInstanceCPUData>& vInstanceCPUData)
{
	CreateInstances(vInstanceCPUData, m_vInstanceGPUData);
}

void CObject3D::CreateInstances(size_t InstanceCount)
{
	if (InstanceCount <= 0) return;
	if (InstanceCount >= 100'000) return; // TOO MANY INSTANCES

	m_vInstanceCPUData.resize(InstanceCount);
	for (size_t iInstance = 0; iInstance < InstanceCount; ++iInstance)
	{
		auto& InstanceCPUData{ m_vInstanceCPUData[iInstance] };
		InstanceCPUData.Transform.Scaling = m_ComponentTransform.Scaling;
		InstanceCPUData.Name = "instance" + to_string(iInstance);
	}

	CreateInstances(m_vInstanceCPUData);
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

	if (AutoGeneratedName.length() >= SObject3DInstanceCPUData::KMaxNameLengthZeroTerminated)
	{
		MB_WARN(("인스턴스 이름 [" + AutoGeneratedName + "] 이 최대 길이(" +
			to_string(SObject3DInstanceCPUData::KMaxNameLengthZeroTerminated - 1) + " 자)를 넘어\n인스턴스 생성에 실패했습니다.").c_str(), "인스턴스 생성 실패");
		return false;
	}

	return InsertInstance(AutoGeneratedName);
}

bool CObject3D::InsertInstance(const string& InstanceName)
{
	if (m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("해당 이름(" + InstanceName + ")의 인스턴스가 이미 존재합니다.").c_str(), "인스턴스 생성 실패");
		return false;
	}

	bool bShouldRecreateInstanceBuffer{ m_vInstanceCPUData.size() == m_vInstanceCPUData.capacity() };

	std::string LimitedName{ InstanceName };
	if (LimitedName.length() >= SObject3DInstanceCPUData::KMaxNameLengthZeroTerminated)
	{
		MB_WARN(("인스턴스 이름 [" + LimitedName + "] 이 최대 길이(" + to_string(SObject3DInstanceCPUData::KMaxNameLengthZeroTerminated - 1) +
			" 자)를 넘어 잘려서 저장됩니다.").c_str(), "이름 길이 제한");

		LimitedName.resize(SObject3DInstanceCPUData::KMaxNameLengthZeroTerminated - 1);
	}

	m_vInstanceCPUData.emplace_back();
	m_vInstanceCPUData.back().Name = LimitedName;
	m_vInstanceCPUData.back().Transform.Translation = m_ComponentTransform.Translation;
	m_vInstanceCPUData.back().Transform.Scaling = m_ComponentTransform.Scaling;
	m_vInstanceCPUData.back().Transform.Pitch = m_ComponentTransform.Pitch;
	m_vInstanceCPUData.back().Transform.Yaw = m_ComponentTransform.Yaw;
	m_vInstanceCPUData.back().Transform.Roll = m_ComponentTransform.Roll;
	m_vInstanceCPUData.back().EditorBoundingSphere = m_OuterBoundingSphere; // @important
	m_mapInstanceNameToIndex[LimitedName] = m_vInstanceCPUData.size() - 1;

	m_vInstanceGPUData.emplace_back();

	if (m_vInstanceCPUData.size() == 1) CreateInstanceBuffers();
	if (bShouldRecreateInstanceBuffer) CreateInstanceBuffers();

	UpdateInstanceWorldMatrix(LimitedName);

	return true;
}

void CObject3D::DeleteInstance(const string& InstanceName)
{
	if (m_vInstanceCPUData.empty()) return;

	if (InstanceName.empty())
	{
		MB_WARN("이름이 잘못되었습니다.", "인스턴스 삭제 실패");
		return;
	}
	if (m_mapInstanceNameToIndex.find(InstanceName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN("해당 인스턴스는 존재하지 않습니다.", "인스턴스 삭제 실패");
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

bool CObject3D::ChangeInstanceName(const std::string& OldName, const std::string& NewName)
{
	if (m_mapInstanceNameToIndex.find(OldName) == m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("기존 이름 (" + OldName + ")의 인스턴스가 존재하지 않습니다.").c_str(), "이름 변경 실패");
		return false;
	}
	if (m_mapInstanceNameToIndex.find(NewName) != m_mapInstanceNameToIndex.end())
	{
		MB_WARN(("새 이름 (" + NewName + ")의 인스턴스가 이미 존재합니다.").c_str(), "이름 변경 실패");
		return false;
	}

	string SavedOldName{ OldName };
	size_t iInstance{ m_mapInstanceNameToIndex.at(OldName) };
	m_vInstanceCPUData[iInstance].Name = NewName;
	m_mapInstanceNameToIndex.erase(OldName);
	m_mapInstanceNameToIndex[NewName] = iInstance;

	return true;
}

void CObject3D::TranslateInstanceTo(const std::string& InstanceName, const XMVECTOR& Prime)
{
	GetInstanceCPUData(InstanceName).Transform.Translation = Prime;
}

void CObject3D::RotateInstancePitchTo(const std::string& InstanceName, float Prime)
{
	GetInstanceCPUData(InstanceName).Transform.Pitch = Prime;
}

void CObject3D::RotateInstanceYawTo(const std::string& InstanceName, float Prime)
{
	GetInstanceCPUData(InstanceName).Transform.Yaw = Prime;
}

void CObject3D::RotateInstanceRollTo(const std::string& InstanceName, float Prime)
{
	GetInstanceCPUData(InstanceName).Transform.Roll = Prime;
}

void CObject3D::ScaleInstanceTo(const std::string& InstanceName, const XMVECTOR& Prime)
{
	GetInstanceCPUData(InstanceName).Transform.Scaling = Prime;
}

void CObject3D::TranslateInstance(const std::string& InstanceName, const XMVECTOR& Delta)
{
	GetInstanceCPUData(InstanceName).Transform.Translation += Delta;
}

void CObject3D::RotateInstancePitch(const std::string& InstanceName, float Delta)
{
	GetInstanceCPUData(InstanceName).Transform.Pitch += Delta;
}

void CObject3D::RotateInstanceYaw(const std::string& InstanceName, float Delta)
{
	GetInstanceCPUData(InstanceName).Transform.Yaw += Delta;
}

void CObject3D::RotateInstanceRoll(const std::string& InstanceName, float Delta)
{
	GetInstanceCPUData(InstanceName).Transform.Roll += Delta;
}

void CObject3D::ScaleInstance(const std::string& InstanceName, const XMVECTOR& Delta)
{
	GetInstanceCPUData(InstanceName).Transform.Scaling += Delta;
}

void CObject3D::SetInstanceLinearAcceleration(const std::string& InstanceName, const XMVECTOR& Prime)
{
	GetInstanceCPUData(InstanceName).Physics.LinearAcceleration = Prime;
}

void CObject3D::SetInstanceLinearVelocity(const std::string& InstanceName, const XMVECTOR& Prime)
{
	GetInstanceCPUData(InstanceName).Physics.LinearVelocity = Prime;
}

void CObject3D::AddInstanceLinearAcceleration(const std::string& InstanceName, const XMVECTOR& Delta)
{
	GetInstanceCPUData(InstanceName).Physics.LinearAcceleration += Delta;
}

void CObject3D::AddInstanceLinearVelocity(const std::string& InstanceName, const XMVECTOR& Delta)
{
	GetInstanceCPUData(InstanceName).Physics.LinearVelocity += Delta;
}

const SComponentTransform& CObject3D::GetInstanceTransform(const std::string& InstanceName) const
{
	return GetInstanceCPUData(InstanceName).Transform;
}

const SComponentPhysics& CObject3D::GetInstancePhysics(const std::string& InstanceName) const
{
	return GetInstanceCPUData(InstanceName).Physics;
}

const SBoundingVolume& CObject3D::GetInstanceOuterBoundingSphere(const std::string& InstanceName) const
{
	return GetInstanceCPUData(InstanceName).EditorBoundingSphere;
}

SObject3DInstanceCPUData& CObject3D::GetInstanceCPUData(const string& InstanceName)
{
	return m_vInstanceCPUData[GetInstanceIndex(InstanceName)];
}

void CObject3D::SetInstanceCPUData(const std::string& InstanceName, const SObject3DInstanceCPUData& Prime)
{
	string SavedName{ InstanceName };
	auto& InstanceCPUData{ GetInstanceCPUData(InstanceName) };
	InstanceCPUData = Prime;
	InstanceCPUData.Name = SavedName; // @important
}

const SObject3DInstanceCPUData& CObject3D::GetInstanceCPUData(const std::string& InstanceName) const
{
	return m_vInstanceCPUData[GetInstanceIndex(InstanceName)];
}

const SObject3DInstanceGPUData& CObject3D::GetInstanceGPUData(const std::string& InstanceName) const
{
	return m_vInstanceGPUData[GetInstanceIndex(InstanceName)];
}

const std::vector<SObject3DInstanceCPUData>& CObject3D::GetInstanceCPUDataVector() const
{
	return m_vInstanceCPUData;
}

SObject3DInstanceGPUData& CObject3D::GetInstanceGPUData(const std::string& InstanceName)
{
	return m_vInstanceGPUData[GetInstanceIndex(InstanceName)];
}

size_t CObject3D::GetInstanceIndex(const std::string& InstanceName) const
{
	assert(m_mapInstanceNameToIndex.find(InstanceName) != m_mapInstanceNameToIndex.end());
	return m_mapInstanceNameToIndex.at(InstanceName);
}

const XMMATRIX& CObject3D::GetInstanceWorldMatrix(const std::string& InstanceName) const
{
	return GetInstanceGPUData(InstanceName).WorldMatrix;
}

const std::string& CObject3D::GetLastInstanceName() const
{
	return m_vInstanceCPUData.back().Name;
}

void CObject3D::_CreateInstanceBuffer(size_t MeshIndex)
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SObject3DInstanceGPUData) * m_vInstanceGPUData.capacity()); // @important
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	D3D11_SUBRESOURCE_DATA SubresourceData{};
	SubresourceData.pSysMem = &m_vInstanceGPUData[0];
	m_PtrDevice->CreateBuffer(&BufferDesc, &SubresourceData, &m_vInstanceBuffers[MeshIndex].Buffer);
}

void CObject3D::CreateInstanceBuffers()
{
	if (m_vInstanceCPUData.empty()) return;

	m_vInstanceBuffers.clear();
	m_vInstanceBuffers.resize(m_Model->vMeshes.size());
	for (size_t iMesh = 0; iMesh < m_Model->vMeshes.size(); ++iMesh)
	{
		_CreateInstanceBuffer(iMesh);
	}
}

void CObject3D::_UpdateInstanceBuffer(size_t MeshIndex)
{
	if (m_vInstanceGPUData.empty()) return;
	if (m_vInstanceBuffers.empty()) return;
	if (!m_vInstanceBuffers[MeshIndex].Buffer) return;

	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_vInstanceGPUData[0], sizeof(SObject3DInstanceGPUData) * m_vInstanceGPUData.size());

		m_PtrDeviceContext->Unmap(m_vInstanceBuffers[MeshIndex].Buffer.Get(), 0);
	}
}

void CObject3D::UpdateInstanceBuffers()
{
	for (size_t iMesh = 0; iMesh < m_Model->vMeshes.size(); ++iMesh)
	{
		_UpdateInstanceBuffer(iMesh);
	}
}

void CObject3D::UpdateQuadUV(const XMFLOAT2& UVOffset, const XMFLOAT2& UVSize)
{
	float U0{ UVOffset.x };
	float V0{ UVOffset.y };
	float U1{ U0 + UVSize.x };
	float V1{ V0 + UVSize.y };

	m_Model->vMeshes[0].vVertices[0].TexCoord = XMVectorSet(U0, V0, 0, 0);
	m_Model->vMeshes[0].vVertices[1].TexCoord = XMVectorSet(U1, V0, 0, 0);
	m_Model->vMeshes[0].vVertices[2].TexCoord = XMVectorSet(U0, V1, 0, 0);
	m_Model->vMeshes[0].vVertices[3].TexCoord = XMVectorSet(U1, V1, 0, 0);

	UpdateMeshBuffer();
}

void CObject3D::UpdateMeshBuffer(size_t MeshIndex)
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_vMeshBuffers[MeshIndex].VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &m_Model->vMeshes[MeshIndex].vVertices[0], sizeof(SVertex3D) * m_Model->vMeshes[MeshIndex].vVertices.size());

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
	LimitFloatRotation(m_ComponentTransform.Pitch, KRotationMinLimit, KRotationMaxLimit);
	LimitFloatRotation(m_ComponentTransform.Yaw, KRotationMinLimit, KRotationMaxLimit);
	LimitFloatRotation(m_ComponentTransform.Roll, KRotationMinLimit, KRotationMaxLimit);

	if (XMVectorGetX(m_ComponentTransform.Scaling) < KScalingMinLimit)
		m_ComponentTransform.Scaling = XMVectorSetX(m_ComponentTransform.Scaling, KScalingMinLimit);
	if (XMVectorGetY(m_ComponentTransform.Scaling) < KScalingMinLimit)
		m_ComponentTransform.Scaling = XMVectorSetY(m_ComponentTransform.Scaling, KScalingMinLimit);
	if (XMVectorGetZ(m_ComponentTransform.Scaling) < KScalingMinLimit)
		m_ComponentTransform.Scaling = XMVectorSetZ(m_ComponentTransform.Scaling, KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(m_ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(m_ComponentTransform.Pitch,
		m_ComponentTransform.Yaw, m_ComponentTransform.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(m_ComponentTransform.Scaling) };
	
	// @important
	float ScalingX{ XMVectorGetX(m_ComponentTransform.Scaling) };
	float ScalingY{ XMVectorGetY(m_ComponentTransform.Scaling) };
	float ScalingZ{ XMVectorGetZ(m_ComponentTransform.Scaling) };
	float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
	m_OuterBoundingSphere.Data.BS.Radius = m_OuterBoundingSphere.Data.BS.RadiusBias * MaxScaling;

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(m_OuterBoundingSphere.Center) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-m_OuterBoundingSphere.Center) };

	m_WorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;
}

void CObject3D::UpdateInstanceWorldMatrix(const std::string& InstanceName, bool bUpdateInstanceBuffer)
{
	auto& InstanceCPUData{ GetInstanceCPUData(InstanceName) };
	auto& InstanceGPUData{ GetInstanceGPUData(InstanceName) };

	// Update CPU data
	LimitFloatRotation(InstanceCPUData.Transform.Pitch, KRotationMinLimit, KRotationMaxLimit);
	LimitFloatRotation(InstanceCPUData.Transform.Yaw, KRotationMinLimit, KRotationMaxLimit);
	LimitFloatRotation(InstanceCPUData.Transform.Roll, KRotationMinLimit, KRotationMaxLimit);

	if (XMVectorGetX(InstanceCPUData.Transform.Scaling) < KScalingMinLimit)
		InstanceCPUData.Transform.Scaling = XMVectorSetX(InstanceCPUData.Transform.Scaling, KScalingMinLimit);
	if (XMVectorGetY(InstanceCPUData.Transform.Scaling) < KScalingMinLimit)
		InstanceCPUData.Transform.Scaling = XMVectorSetY(InstanceCPUData.Transform.Scaling, KScalingMinLimit);
	if (XMVectorGetZ(InstanceCPUData.Transform.Scaling) < KScalingMinLimit)
		InstanceCPUData.Transform.Scaling = XMVectorSetZ(InstanceCPUData.Transform.Scaling, KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(InstanceCPUData.Transform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(
		InstanceCPUData.Transform.Pitch, InstanceCPUData.Transform.Yaw, InstanceCPUData.Transform.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(InstanceCPUData.Transform.Scaling) };

	// @important
	float ScalingX{ XMVectorGetX(InstanceCPUData.Transform.Scaling) };
	float ScalingY{ XMVectorGetY(InstanceCPUData.Transform.Scaling) };
	float ScalingZ{ XMVectorGetZ(InstanceCPUData.Transform.Scaling) };
	float MaxScaling{ max(ScalingX, max(ScalingY, ScalingZ)) };
	InstanceCPUData.EditorBoundingSphere.Data.BS.Radius = m_OuterBoundingSphere.Data.BS.RadiusBias * MaxScaling;

	XMMATRIX BoundingSphereTranslation{ XMMatrixTranslationFromVector(m_OuterBoundingSphere.Center) };
	XMMATRIX BoundingSphereTranslationOpposite{ XMMatrixTranslationFromVector(-m_OuterBoundingSphere.Center) };

	// Update GPU data
	InstanceGPUData.WorldMatrix = Scaling * BoundingSphereTranslationOpposite * Rotation * Translation * BoundingSphereTranslation;

	if (bUpdateInstanceBuffer) UpdateInstanceBuffers();
}

void CObject3D::UpdateInstanceWorldMatrix(const std::string& InstanceName, const XMMATRIX& WorldMatrix)
{
	if (InstanceName.empty()) return;

	GetInstanceGPUData(InstanceName).WorldMatrix = WorldMatrix;

	UpdateInstanceBuffers();
}

void CObject3D::UpdateAllInstances(bool bUpdateWorldMatrix)
{
	if (bUpdateWorldMatrix)
	{
		for (const auto& InstanceCPUData : m_vInstanceCPUData)
		{
			UpdateInstanceWorldMatrix(InstanceCPUData.Name, false);
		}
	}
	UpdateInstanceBuffers();
}

void CObject3D::SetInstanceHighlight(const std::string& InstanceName, bool bShouldHighlight)
{
	GetInstanceGPUData(InstanceName).IsHighlighted = (bShouldHighlight) ? 1.0f : 0.0f;
}

void CObject3D::SetAllInstancesHighlightOff()
{
	for (auto& InstanceGPUData : m_vInstanceGPUData)
	{
		InstanceGPUData.IsHighlighted = 0.0f;
	}

	UpdateInstanceBuffers();
}

void CObject3D::UpdateCBMaterial(const CMaterialData& MaterialData, uint32_t TotalMaterialCount) const
{
	//m_CBMaterialData.AmbientColor = MaterialData.AmbientColor();
	m_CBMaterialData.DiffuseColor = MaterialData.DiffuseColor();
	//m_CBMaterialData.SpecularColor = MaterialData.SpecularColor();
	//m_CBMaterialData.SpecularExponent = MaterialData.SpecularExponent();
	//m_CBMaterialData.SpecularIntensity = MaterialData.SpecularIntensity();
	m_CBMaterialData.Roughness = MaterialData.Roughness();
	m_CBMaterialData.Metalness = MaterialData.Metalness();

	uint32_t FlagsHasTexture{};
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::DiffuseTexture) ? 0x01 : 0;
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::NormalTexture) ? 0x02 : 0;
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::OpacityTexture) ? 0x04 : 0;
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::SpecularIntensityTexture) ? 0x08 : 0;
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::RoughnessTexture) ? 0x10 : 0;
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::MetalnessTexture) ? 0x20 : 0;
	FlagsHasTexture += MaterialData.HasTexture(ETextureType::AmbientOcclusionTexture) ? 0x40 : 0;
	// @empty_slot: Displacement texture is usually not used in PS
	FlagsHasTexture += m_Model->bIgnoreSceneMaterial ? 0x2000 : 0;
	m_CBMaterialData.FlagsHasTexture = FlagsHasTexture;

	uint32_t FlagsIsTextureSRGB{};
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::DiffuseTexture) ? 0x01 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::NormalTexture) ? 0x02 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::OpacityTexture) ? 0x04 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::SpecularIntensityTexture) ? 0x08 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::RoughnessTexture) ? 0x10 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::MetalnessTexture) ? 0x20 : 0;
	FlagsIsTextureSRGB += MaterialData.IsTextureSRGB(ETextureType::AmbientOcclusionTexture) ? 0x40 : 0;
	// @empty_slot: Displacement texture is usually not used in PS
	m_CBMaterialData.FlagsIsTextureSRGB = FlagsIsTextureSRGB;

	m_CBMaterialData.TotalMaterialCount = TotalMaterialCount;

	m_CBMaterial->Update();
	m_CBMaterial->Use(EShaderType::PixelShader, 0); // @important
}

bool CObject3D::ShouldTessellate() const
{
	return m_bShouldTesselate;
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

bool CObject3D::IsCreated() const
{
	return m_bIsCreated;
}

bool CObject3D::IsRigged() const
{
	return m_Model->bIsModelRigged;
}

bool CObject3D::IsInstanced() const
{
	return (m_vInstanceCPUData.size() > 0) ? true : false;
}

bool CObject3D::IsPickable() const
{
	return m_bIsPickable;
}

void CObject3D::IsPickable(bool NewValue)
{
	m_bIsPickable = NewValue;
}

bool CObject3D::IsTransparent() const
{
	return m_ComponentRender.bIsTransparent;
}

void CObject3D::IsTransparent(bool NewValue)
{
	m_ComponentRender.bIsTransparent = NewValue;
}

bool CObject3D::IsGPUSkinned() const
{
	return HasBakedAnimationTexture();
}

size_t CObject3D::GetInstanceCount() const
{
	return m_vInstanceCPUData.size();
}

const SMESHData& CObject3D::GetModel() const
{
	return *m_Model;
}

SMESHData& CObject3D::GetModel()
{
	return *m_Model;
}

void CObject3D::SetName(const std::string& Name)
{
	m_Name = Name;
}

const std::string& CObject3D::GetName() const
{
	return m_Name;
}

void CObject3D::SetModelFileName(const std::string& FileName)
{
	m_ModelFileName = FileName;
}

const std::string& CObject3D::GetModelFileName() const
{
	return m_ModelFileName;
}

const std::string& CObject3D::GetOB3DFileName() const
{
	return m_OB3DFileName;
}

const std::map<std::string, size_t>& CObject3D::GetInstanceNameToIndexMap() const
{
	return m_mapInstanceNameToIndex;
}

CMaterialTextureSet* CObject3D::GetMaterialTextureSet(size_t iMaterial)
{
	if (iMaterial >= m_vMaterialTextureSets.size()) return nullptr;
	return m_vMaterialTextureSets[iMaterial].get();
}

void CObject3D::SetOuterBoundingSphereCenterOffset(const XMVECTOR& Center)
{
	m_OuterBoundingSphere.Center = Center;
	if (m_Model) m_Model->EditorBoundingSphereData.Center = m_OuterBoundingSphere.Center;
}

void CObject3D::SetOuterBoundingSphereRadiusBias(float Radius)
{
	m_OuterBoundingSphere.Data.BS.RadiusBias = Radius;
	if (m_Model) m_Model->EditorBoundingSphereData.Data.BS.RadiusBias = m_OuterBoundingSphere.Data.BS.RadiusBias;
}

const XMVECTOR& CObject3D::GetOuterBoundingSphereCenterOffset() const
{
	return m_OuterBoundingSphere.Center;
}

float CObject3D::GetOuterBoundingSphereRadius() const
{
	return m_OuterBoundingSphere.Data.BS.Radius;
}

float CObject3D::GetOuterBoundingSphereRadiusBias() const
{
	return m_OuterBoundingSphere.Data.BS.RadiusBias;
}

const SBoundingVolume& CObject3D::GetOuterBoundingSphere() const
{
	return m_OuterBoundingSphere;
}

const XMMATRIX& CObject3D::GetWorldMatrix() const
{
	return m_WorldMatrix;
}

void CObject3D::Animate(float DeltaTime)
{
	if (!HasAnimations()) return;

	if (IsInstanced())
	{
		for (const auto& InstanceCPUData : m_vInstanceCPUData)
		{
			AnimateInstance(InstanceCPUData.Name, DeltaTime);
		}
		UpdateInstanceBuffers(); // @important
	}
	else
	{
		if ((m_eCurrentAnimationOption == EAnimationOption::PlayToFirstFrame || m_eCurrentAnimationOption == EAnimationOption::PlayToLastFrame) &&
			m_CurrentAnimationPlayCount >= 1)
		{
			return;
		}

		const SMeshAnimation& CurrentAnimation{ m_Model->vAnimations[m_CurrentAnimationID] };
		m_AnimationTick += CurrentAnimation.TicksPerSecond * DeltaTime;
		if (m_AnimationTick > CurrentAnimation.Duration)
		{
			++m_CurrentAnimationPlayCount;

			if (m_eCurrentAnimationOption == EAnimationOption::Repeat || m_eCurrentAnimationOption == EAnimationOption::PlayToFirstFrame)
			{
				m_AnimationTick = 0.0f;
			}
		}
	}

	m_CBAnimationData.bIsInstanced = IsInstanced();
	m_CBAnimationData.AnimationID = m_CurrentAnimationID;
	m_CBAnimationData.AnimationTick = m_AnimationTick;
	if (m_BakedAnimationTexture)
	{
		m_CBAnimationData.bUseGPUSkinning = TRUE;
	}
	else
	{
		m_CBAnimationData.bUseGPUSkinning = FALSE;
		CalculateAnimatedBoneMatrices(m_Model->vAnimations[m_CurrentAnimationID], m_AnimationTick, m_Model->vTreeNodes[0], XMMatrixIdentity());
	}
}

void CObject3D::AnimateInstance(const std::string& InstanceName, float DeltaTime)
{
	auto& InstanceCPUData{ GetInstanceCPUData(InstanceName) };
	auto& InstanceGPUData{ GetInstanceGPUData(InstanceName) };

	if ((InstanceCPUData.eCurrAnimOption == EAnimationOption::PlayToFirstFrame || InstanceCPUData.eCurrAnimOption == EAnimationOption::PlayToLastFrame) &&
		InstanceCPUData.CurrAnimPlayCount >= 1)
	{
		return;
	}

	const SMeshAnimation& CurrentAnimation{ m_Model->vAnimations[InstanceGPUData.CurrAnimID] };
	InstanceGPUData.AnimTick += CurrentAnimation.TicksPerSecond * DeltaTime;
	if (InstanceGPUData.AnimTick > CurrentAnimation.Duration)
	{
		++InstanceCPUData.CurrAnimPlayCount;

		if (InstanceCPUData.eCurrAnimOption == EAnimationOption::Repeat || InstanceCPUData.eCurrAnimOption == EAnimationOption::PlayToFirstFrame)
		{
			InstanceGPUData.AnimTick = 0.0f;
		}
	}
}

void CObject3D::CalculateAnimatedBoneMatrices(const SMeshAnimation& CurrentAnimation, float AnimationTick,
	const SMeshTreeNode& Node, XMMATRIX ParentTransform)
{
	XMMATRIX MatrixTransformation{ Node.MatrixTransformation * ParentTransform };

	if (Node.bIsBone)
	{
		if (CurrentAnimation.vNodeAnimations.size())
		{
			if (CurrentAnimation.umapNodeAnimationNameToIndex.find(Node.Name) != CurrentAnimation.umapNodeAnimationNameToIndex.end())
			{
				size_t NodeAnimationIndex{ CurrentAnimation.umapNodeAnimationNameToIndex.at(Node.Name) };

				const SMeshAnimation::SNodeAnimation& NodeAnimation{ CurrentAnimation.vNodeAnimations[NodeAnimationIndex] };

				XMMATRIX MatrixPosition{ XMMatrixIdentity() };
				XMMATRIX MatrixRotation{ XMMatrixIdentity() };
				XMMATRIX MatrixScaling{ XMMatrixIdentity() };

				{
					const vector<SMeshAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vPositionKeys };
					SMeshAnimation::SNodeAnimation::SKey KeyA{};
					SMeshAnimation::SNodeAnimation::SKey KeyB{};
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
					const vector<SMeshAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vRotationKeys };
					SMeshAnimation::SNodeAnimation::SKey KeyA{};
					SMeshAnimation::SNodeAnimation::SKey KeyB{};
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
					const vector<SMeshAnimation::SNodeAnimation::SKey>& vKeys{ NodeAnimation.vScalingKeys };
					SMeshAnimation::SNodeAnimation::SKey KeyA{};
					SMeshAnimation::SNodeAnimation::SKey KeyB{};
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
			CalculateAnimatedBoneMatrices(CurrentAnimation, AnimationTick, m_Model->vTreeNodes[iChild], MatrixTransformation);
		}
	}
}

void CObject3D::Draw(EFlagsObject3DRendering eFlagsRendering, size_t OneInstanceIndex) const
{
	bool bIgnoreOwnTexture{ EFLAG_HAS(eFlagsRendering, EFlagsObject3DRendering::IgnoreOwnTextures) };
	bool bDrawOneInstance{ EFLAG_HAS(eFlagsRendering, EFlagsObject3DRendering::DrawOneInstance) };

	if (HasBakedAnimationTexture()) m_BakedAnimationTexture->Use();

	for (size_t iMesh = 0; iMesh < m_Model->vMeshes.size(); ++iMesh)
	{
		const SMesh& Mesh{ m_Model->vMeshes[iMesh] };
		const CMaterialData& MaterialData{ m_Model->vMaterialData[Mesh.MaterialID] };

		// per mesh
		UpdateCBMaterial(MaterialData, (uint32_t)m_Model->vMaterialData.size());

		if (MaterialData.HasAnyTexture() && !bIgnoreOwnTexture)
		{
			if (m_Model->bUseMultipleTexturesInSingleMesh) // This bool is for CTerrain
			{
				for (const CMaterialData& MaterialDatum : m_Model->vMaterialData)
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

		if (IsInstanced())
		{
			m_PtrDeviceContext->IASetVertexBuffers(2, 1, m_vInstanceBuffers[iMesh].Buffer.GetAddressOf(),
				&m_vInstanceBuffers[iMesh].Stride, &m_vInstanceBuffers[iMesh].Offset);

			if (bDrawOneInstance)
			{
				m_PtrDeviceContext->DrawIndexedInstanced(static_cast<UINT>(Mesh.vTriangles.size() * 3),
					1, 0, 0, static_cast<UINT>(OneInstanceIndex));
			}
			else
			{
				m_PtrDeviceContext->DrawIndexedInstanced(static_cast<UINT>(Mesh.vTriangles.size() * 3),
					static_cast<UINT>(m_vInstanceCPUData.size()), 0, 0, 0);
			}
		}
		else
		{
			m_PtrDeviceContext->DrawIndexed(static_cast<UINT>(Mesh.vTriangles.size() * 3), 0, 0);
		}
	}
}
