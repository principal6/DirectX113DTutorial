#pragma once

#include "SharedHeader.h"
#include "Material.h"

class CBillboard
{
public:
	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "INSTANCE_POSITION"	, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_ROTATION"	, 0, DXGI_FORMAT_R32_FLOAT			, 0, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INSTANCE_SCALING"	, 0, DXGI_FORMAT_R32G32B32_FLOAT	, 0, 20, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	struct SCBBillboardData
	{
		XMFLOAT2 PixelSize{};
		XMFLOAT2 ScreenSize{}; // @important: this will be updated in CGame
		XMFLOAT2 WorldSpaceSize{ 1.0f, 1.0f };
		BOOL bIsScreenSpace{ FALSE };
		float Reserved{};
	};

private:
	struct SInstanceCPUDataBillboard
	{
		SInstanceCPUDataBillboard(const std::string& _Name) : Name{ _Name } {}

		std::string				Name{};
		SBoundingSphere			BoundingSphere{};
	};

	struct alignas(16) SInstanceGPUDataBillboard
	{
		XMVECTOR				Position{ 0, 0, 0, 1 }; // @important
		float					Rotation{};
		XMFLOAT3				Scaling{ 1, 1, 1 };
	};

	struct SInstanceBuffer
	{
		ComPtr<ID3D11Buffer>	Buffer{};
		UINT					Stride{ sizeof(SInstanceGPUDataBillboard) }; // @important
		UINT					Offset{};
	};

public:
	CBillboard(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
		m_Name{ Name }, m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CBillboard() {}

public:
	void* operator new(size_t Size)
	{
		return _aligned_malloc(Size, 16);
	}

	void operator delete(void* Pointer)
	{
		_aligned_free(Pointer);
	}

public:
	void CreateScreenSpace(const std::string& TextureFileName);
	void CreateWorldSpace(const std::string& TextureFileName, const XMFLOAT2& WorldSpaceSize);

public:
	void InsertInstance(const std::string& InstanceName);
	void DeleteInstance(const std::string& InstanceName);

private:
	void CreateInstanceBuffer();
	void UpdateInstanceBuffer();

public:
	void SetInstancePosition(const std::string& InstanceName, const XMVECTOR& Position);
	void SetInstancePosition(size_t InstanceID, const XMVECTOR& Position);
	const XMVECTOR& GetInstancePosition(size_t InstanceID) const;

	const CBillboard::SCBBillboardData& GetCBBillboard() const;

public:
	void Draw();

private:
	ID3D11Device* const						m_PtrDevice{};
	ID3D11DeviceContext* const				m_PtrDeviceContext{};

private:
	std::string								m_Name{};
	CTexture								m_Texture{ m_PtrDevice, m_PtrDeviceContext };
	SCBBillboardData						m_CBBillboardData{};

private:
	SInstanceBuffer							m_InstanceBuffer{};
	std::vector<SInstanceCPUDataBillboard>	m_vInstanceCPUData{};
	std::vector<SInstanceGPUDataBillboard>	m_vInstanceGPUData{};
	std::map<std::string, size_t>			m_mapInstanceNameToIndex{};
};