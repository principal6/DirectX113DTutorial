#pragma once

#include "SharedHeader.h"
#include "Material.h"

class CParticlePool
{
protected:
	struct SVertexParticle
	{
		union UTexColor
		{
			XMVECTOR TexCoord;
			XMVECTOR Color{ 1, 1, 1, 1 };
		};

		XMVECTOR Position{};
		UTexColor TexColor{};
		float Rotation{};
		XMFLOAT2 ScalingFactor{ 1.0f, 1.0f };
	};

	struct SParticleData
	{
		bool bShouldCollide{ false };
		float Duration{};
		float Elapsed{};
		XMVECTOR Acceleration{};
		XMVECTOR Velocity{}; // (SphericalConstraings) Signed speeds for spherical rotation
	};

public:
	CParticlePool(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	virtual ~CParticlePool() {}

public:
	virtual void Create(size_t MaxParticleCount);

	virtual void SetSpawningInterval(float Value);
	virtual void SetTexture(const string& FileName);
	virtual void SetUniversalScalingFactor(const XMFLOAT2& Factor);
	virtual void SetUniversalDuration(float Value);

	virtual void SpawnParticle();
	virtual void Update(float DeltaTime);
	virtual void Draw() const;

protected:
	virtual void CreateVertexBuffer();
	virtual void UpdateVertexBuffer();
	virtual void SetLastParticleTexColor();
	virtual void SetLastParticleScalingFactor();

protected:
	static constexpr float	KSpawningIntervalDefault{ 1.0f };

protected:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

protected:
	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertexParticle) };
	UINT					m_VertexBufferOffset{};

protected:
	size_t							m_MaxParticleCount{};
	float							m_SpawningInterval{ KSpawningIntervalDefault };
	float							m_SpawningTimer{};

	vector<SVertexParticle>			m_vVertexParticles{};
	vector<SParticleData>			m_vParticleData{};

	bool							m_bUseTexture{ false };
	unique_ptr<CMaterial::CTexture>	m_ParticleTexture{};

	XMFLOAT2						m_ParticleScalingFactor{ 1.0f, 1.0f };
	float							m_ParticleDuration{ 1.0f };
};