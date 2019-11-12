#include "ParticlePool.h"
#include "Math.h"

using std::max;
using std::min;
using std::string;
using std::make_unique;

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

void CParticlePool::SetTexture(const string& FileName)
{
	m_ParticleTexture = make_unique<CTexture>(m_PtrDevice, m_PtrDeviceContext);
	m_ParticleTexture->CreateTextureFromFile(FileName, false);
	m_ParticleTexture->SetSlot(0);

	m_bUseTexture = true;
}

void CParticlePool::SetUniversalScalingFactor(const XMFLOAT2& Factor)
{
	m_ParticleScalingFactor = Factor;
}

void CParticlePool::SetUniversalDuration(float Value)
{
	m_ParticleDuration = Value;
}

void CParticlePool::SpawnParticle()
{
	m_vVertexParticles.emplace_back();
	m_vParticleData.emplace_back();

	m_vParticleData.back().Duration = m_ParticleDuration;

	SetLastParticleTexColor();
	SetLastParticleScalingFactor();
}

void CParticlePool::Update(float DeltaTime)
{
	// Spawn particle
	const float KSpawningInterval{ m_SpawningInterval };
	m_SpawningTimer = min(KSpawningInterval, m_SpawningTimer + DeltaTime);
	if (m_SpawningTimer >= KSpawningInterval && m_vVertexParticles.size() < m_MaxParticleCount)
	{
		SpawnParticle();

		m_SpawningTimer = 0.0f;
	}

	// Elapse time and destroy particle
	int64_t ParticleCount{ static_cast<int64_t>(m_vVertexParticles.size()) };
	for (int64_t iParticle = 0; iParticle < ParticleCount; ++iParticle)
	{
		SVertexParticle& VertexParticle{ m_vVertexParticles[iParticle] };
		SParticleData& ParticleData{ m_vParticleData[iParticle] };

		ParticleData.Elapsed += DeltaTime;

		float NormalizedElapsed{ ParticleData.Elapsed / ParticleData.Duration };
		NormalizedElapsed = max(min(NormalizedElapsed, 1.0f), 0.0f);
		VertexParticle.TexColor.TexCoord = XMVectorSetZ(VertexParticle.TexColor.TexCoord, sin(NormalizedElapsed * XM_PI));

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
		VertexParticle.Position += ParticleData.Velocity * DeltaTime;
	}

	UpdateVertexBuffer();
}

void CParticlePool::Draw() const
{
	if (m_bUseTexture)
	{
		if (m_ParticleTexture) m_ParticleTexture->Use();
	}

	m_PtrDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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

void CParticlePool::SetLastParticleTexColor()
{
	if (!m_bUseTexture)
	{
		float R{ GetRandom(0.0f, 1.0f) };
		float G{ GetRandom(0.0f, 1.0f) };
		float B{ GetRandom(0.0f, 1.0f) };
		m_vVertexParticles.back().TexColor.Color = XMVectorSet(R, G, B, 1);
	}
	else
	{
		m_vVertexParticles.back().TexColor.TexCoord = XMVectorSet(0, 0, 1, -1);
	}
}

void CParticlePool::SetLastParticleScalingFactor()
{
	m_vVertexParticles.back().ScalingFactor = m_ParticleScalingFactor;
}
