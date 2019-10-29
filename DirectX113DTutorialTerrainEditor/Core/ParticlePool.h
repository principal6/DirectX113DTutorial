#pragma once

#include "SharedHeader.h"

class CParticlePool
{
public:
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
		XMFLOAT2 ScalingFactor{};
	};

	struct SParticleData
	{
		bool bShouldCollide{ false };
		float Duration{ 10.0f };
		float Elapsed{};
		XMVECTOR Acceleration{};
		XMVECTOR Velocity{}; // (SphericalConstraings) Signed speeds for spherical rotation
	};

	struct SSphericalPositionConstraints
	{
		SSphericalPositionConstraints() {}
		SSphericalPositionConstraints(float _Radius) : Radius{ _Radius } {}
		SSphericalPositionConstraints(float _Radius, const XMVECTOR& _Center) : Radius{ _Radius }, Center{ _Center } {}

		float Radius{};
		XMVECTOR Center{ 0, 0, 0, 1 };
	};

public:
	CParticlePool(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
	}
	~CParticlePool() {}

	void Create(size_t MaxParticleCount);

	void SetSpawningInterval(float Value);
	void SetSphericalPositionConstraints(const SSphericalPositionConstraints& Constraints);
	void SetParticleRotationSpeedFactor(float Value);

	void Update(float DeltaTime);

	void Draw() const;

private:
	void CreateVertexBuffer();
	void UpdateVertexBuffer();

private:
	static constexpr float	KSpawningIntervalDefault{ 1.0f };
	static constexpr float	KParticleRotationSpeedFactorDefault{ 1.0f };

private:
	ID3D11Device*			m_PtrDevice{};
	ID3D11DeviceContext*	m_PtrDeviceContext{};

private:
	ComPtr<ID3D11Buffer>	m_VertexBuffer{};
	UINT					m_VertexBufferStride{ sizeof(SVertexParticle) };
	UINT					m_VertexBufferOffset{};

private:
	size_t							m_MaxParticleCount{};
	float							m_SpawningInterval{ KSpawningIntervalDefault };
	float							m_SpawningTimer{};
	SSphericalPositionConstraints	m_SphericalPositionConstraints{};
	bool							m_bIsSphericalPositionConstraintsSet{ false };
	float							m_ParticleRotationSpeedFactor{ KParticleRotationSpeedFactorDefault };

	vector<SVertexParticle>			m_vVertexParticles{};
	vector<SParticleData>			m_vParticleData{};
};