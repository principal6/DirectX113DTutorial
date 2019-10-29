#include "ParticlePool.h"

static int GetRandom(int Min, int Max)
{
	return (rand() % (Max - Min + 1)) + Min;
}

static float GetRandom(float Min, float Max)
{
	constexpr float KFreedom{ 100.0f };
	float Range{ Max - Min };
	int IntRange{ (int)(KFreedom * Range) };
	int IntRangeHalf{ (int)(IntRange / 2) };

	return static_cast<float>((rand() % (IntRange + 1) - IntRangeHalf) / KFreedom);
}

void CParticlePool::Create(size_t MaxParticleCount)
{
	m_MaxParticleCount = MaxParticleCount;

	m_vVertexParticles.reserve(m_MaxParticleCount);
	m_vParticleData.reserve(m_MaxParticleCount);

	CreateVertexBuffer();

	srand((unsigned int)GetTickCount64());
}

void CParticlePool::SetSpawningInterval(float Value)
{
	m_SpawningInterval = Value;
}

void CParticlePool::SetSphericalPositionConstraints(const SSphericalPositionConstraints& Constraints)
{
	m_SphericalPositionConstraints = Constraints;
	m_bIsSphericalPositionConstraintsSet = true;
}

void CParticlePool::SetParticleRotationSpeedFactor(float Value)
{
	m_ParticleRotationSpeedFactor = Value;
}

void CParticlePool::Update(float DeltaTime)
{
	const float KSpawningInterval{ m_SpawningInterval };
	m_SpawningTimer = min(KSpawningInterval, m_SpawningTimer + DeltaTime);
	if (m_SpawningTimer >= KSpawningInterval && m_vVertexParticles.size() < m_MaxParticleCount)
	{
		m_vVertexParticles.emplace_back();
		m_vParticleData.emplace_back();

		if (m_bIsSphericalPositionConstraintsSet)
		{
			const float& KRadius{ m_SphericalPositionConstraints.Radius };

			float X{ GetRandom(-KRadius, +KRadius) };
			
			float YZRadius{ sin(acos(X / KRadius)) * KRadius };
			float Y{ GetRandom(-YZRadius, +YZRadius) };

			float Z{ cos(asin(Y / YZRadius)) * YZRadius };
			int Sign{ GetRandom(0, 1) };
			if (Sign == 1) Z *= -1.0f;

			m_vVertexParticles.back().Position = XMVectorSet(X, Y, Z, 1) + m_SphericalPositionConstraints.Center;
		}
		
		float RandomX{ GetRandom(-1.0f, +1.0f) };
		float RandomY{ GetRandom(-1.0f, +1.0f) };
		float RandomZ{ GetRandom(-1.0f, +1.0f) };
		m_vParticleData.back().Velocity = XMVectorSet(RandomX, RandomY, RandomZ, 0);

		m_SpawningTimer = 0.0f;
	}

	int64_t ParticleCount{ static_cast<int64_t>(m_vVertexParticles.size()) };

	for (int64_t iParticle = 0; iParticle < ParticleCount; ++iParticle)
	{
		SVertexParticle& VertexParticle{ m_vVertexParticles[iParticle] };
		SParticleData& ParticleData{ m_vParticleData[iParticle] };

		ParticleData.Elapsed += DeltaTime;
		if (ParticleData.Elapsed >= ParticleData.Duration)
		{
			int64_t iLastParticle{ ParticleCount - 1 };
			if (iParticle < iLastParticle)
			{
				std::swap(m_vVertexParticles[iParticle], m_vVertexParticles[iLastParticle]);
				std::swap(m_vParticleData[iParticle], m_vParticleData[iLastParticle]);
			}
			m_vVertexParticles.pop_back();
			m_vParticleData.pop_back();

			--ParticleCount;

			continue;
		}

		ParticleData.Velocity += ParticleData.Acceleration * DeltaTime;

		if (m_bIsSphericalPositionConstraintsSet)
		{
			XMVECTOR PositionFromOrigin{ VertexParticle.Position - m_SphericalPositionConstraints.Center };
			XMMATRIX RotationMatrix{ XMMatrixRotationRollPitchYawFromVector((ParticleData.Velocity * XM_PI) * DeltaTime * m_ParticleRotationSpeedFactor) };
			PositionFromOrigin = XMVector3TransformCoord(PositionFromOrigin, RotationMatrix);
			VertexParticle.Position = PositionFromOrigin + m_SphericalPositionConstraints.Center;
		}
		else
		{
			VertexParticle.Position += ParticleData.Velocity * DeltaTime;
		}
	}

	UpdateVertexBuffer();
}

void CParticlePool::Draw() const
{
	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	m_PtrDeviceContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_VertexBufferStride, &m_VertexBufferOffset);
	
	m_PtrDeviceContext->Draw(static_cast<UINT>(m_vVertexParticles.size()), 0);
}

void CParticlePool::CreateVertexBuffer()
{
	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.ByteWidth = static_cast<UINT>(sizeof(SVertexParticle) * m_MaxParticleCount);
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	assert(SUCCEEDED(m_PtrDevice->CreateBuffer(&BufferDesc, nullptr, m_VertexBuffer.ReleaseAndGetAddressOf())));
}

void CParticlePool::UpdateVertexBuffer()
{
	D3D11_MAPPED_SUBRESOURCE MappedSubresource{};
	if (SUCCEEDED(m_PtrDeviceContext->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		if (m_vVertexParticles.size())
		{
			memcpy(MappedSubresource.pData, &m_vVertexParticles[0], sizeof(SVertexParticle) * m_vVertexParticles.size());
		}
		else
		{
			memset(MappedSubresource.pData, 0, sizeof(SVertexParticle));
		}

		m_PtrDeviceContext->Unmap(m_VertexBuffer.Get(), 0);
	}
}
