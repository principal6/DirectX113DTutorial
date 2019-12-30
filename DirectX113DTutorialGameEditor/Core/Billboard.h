#pragma once

#include "SharedHeader.h"

class CConstantBuffer;
class CShader;
class CTexture;

class CBillboard
{
public:
	static constexpr D3D11_INPUT_ELEMENT_DESC KInputElementDescs[]
	{
		{ "POSITION"		, 0, DXGI_FORMAT_R32G32B32A32_FLOAT	, 0,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "ROTATION"		, 0, DXGI_FORMAT_R32_FLOAT			, 0, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "SCALING"			, 0, DXGI_FORMAT_R32G32_FLOAT		, 0, 20, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "IS_HIGHLIGHTED"	, 0, DXGI_FORMAT_R32_FLOAT			, 0, 28, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};

	struct SCBBillboardData
	{
		XMFLOAT2	PixelSize{};
		XMFLOAT2	ScreenSize{};
		XMFLOAT2	WorldSpaceSize{ 1.0f, 1.0f };
		BOOL		bIsScreenSpace{ FALSE };
		float		Reserved{};
	};

private:
	struct SInstanceCPUDataBillboard
	{
		SInstanceCPUDataBillboard(const std::string& _Name) : Name{ _Name } {}

		std::string				Name{};
		SBoundingVolume			EditorBoundingSphere{};
	};

	struct alignas(16) SInstanceGPUDataBillboard
	{
		XMVECTOR				Position{ 0, 0, 0, 1 }; // @important
		float					Rotation{};
		XMFLOAT2				Scaling{ 1, 1 };
		float					IsHighlighted{ 0.0f };
	};

	struct SInstanceBuffer
	{
		ComPtr<ID3D11Buffer>	Buffer{};
		UINT					Stride{ sizeof(SInstanceGPUDataBillboard) }; // @important
		UINT					Offset{};
	};

public:
	CBillboard(const std::string& Name, ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CBillboard();

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
	void CreateScreenSpace(const std::string& TextureFileName, const XMFLOAT2& ScreenSize, const CConstantBuffer* const CBEditorTime);
	void CreateWorldSpace(const std::string& TextureFileName, const XMFLOAT2& WorldSpaceSize, const CConstantBuffer* const CBEditorTime);

private:
	void CreateShaders(const CConstantBuffer* const CBEditorTime);

public:
	void InsertInstance(const std::string& InstanceName);
	void DeleteInstance(const std::string& InstanceName);
	void ClearInstances();

private:
	void CreateInstanceBuffer();

public:
	void UpdateInstanceBuffer();

public:
	void SetInstancePosition(const std::string& InstanceName, const XMVECTOR& Position);
	const XMVECTOR& GetInstancePosition(const std::string& InstanceName) const;

	void SetInstanceHighlight(const std::string& InstanceName, bool bShouldHighlight);
	void SetAllInstancesHighlightOff();

	size_t GetInstanceCount() const;

private:
	size_t GetInstanceID(const std::string& InstanceName) const;

public:
	void Draw();

private:
	ID3D11Device* const						m_PtrDevice{};
	ID3D11DeviceContext* const				m_PtrDeviceContext{};

private:
	std::unique_ptr<CommonStates>			m_CommonStates{};

private:
	std::string								m_Name{};
	std::unique_ptr<CTexture>				m_Texture{};

private:
	SCBBillboardData						m_CBBillboardData{};
	std::unique_ptr<CConstantBuffer>		m_CBBillboard{};

	std::unique_ptr<CShader>				m_VSBillboard{};
	std::unique_ptr<CShader>				m_HSBillboard{};
	std::unique_ptr<CShader>				m_DSBillboard{};
	std::unique_ptr<CShader>				m_PSBillboard{};

private:
	SInstanceBuffer							m_InstanceBuffer{};
	std::vector<SInstanceCPUDataBillboard>	m_vInstanceCPUData{};
	std::vector<SInstanceGPUDataBillboard>	m_vInstanceGPUData{};
	std::map<std::string, size_t>			m_mapInstanceNameToIndex{};
};