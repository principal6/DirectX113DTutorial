#pragma once

#define NOMINMAX 0

#include <string>
#include <vector>
#include <cassert>
#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <deque>
#include "../DirectXTK/DirectXTK.h"
#include "../DirectXTex/DirectXTex.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DirectXTK.lib")
#pragma comment(lib, "DirectXTex.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

static const XMMATRIX KMatrixIdentity{ XMMatrixIdentity() };
static constexpr uint32_t KVSSharedCBCount{ 1 };
static constexpr uint32_t KHSSharedCBCount{ 2 };
static constexpr uint32_t KDSSharedCBCount{ 1 };
static constexpr uint32_t KGSSharedCBCount{ 1 };
static constexpr uint32_t KPSSharedCBCount{ 3 };
static constexpr float KBoundingSphereDefaultRadius{ 1.0f };

enum class EShaderType
{
	VertexShader,
	HullShader,
	DomainShader,
	GeometryShader,
	PixelShader
};

struct SVertex3D
{
	SVertex3D() {}
	SVertex3D(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord = XMVectorSet(0, 0, 0, 0)) :
		Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
	XMVECTOR TexCoord{};
	XMVECTOR Normal{};
	XMVECTOR Tangent{};
	// Bitangent is calculated dynamically using Normal and Tangent
};

struct STriangle
{
	STriangle() {}
	STriangle(uint32_t _0, uint32_t _1, uint32_t _2) : I0{ _0 }, I1{ _1 }, I2{ _2 } {}

	uint32_t I0{};
	uint32_t I1{};
	uint32_t I2{};
};

struct SAnimationVertex
{
	static constexpr uint32_t KMaxWeightCount{ 4 };

	uint32_t	BoneIDs[KMaxWeightCount]{};
	float		Weights[KMaxWeightCount]{};
};

struct SMesh
{
	std::vector<SVertex3D>			vVertices{};
	std::vector<SAnimationVertex>	vAnimationVertices{};
	std::vector<STriangle>			vTriangles{};

	uint8_t							MaterialID{};
};

enum class EBoundingVolumeType
{
	BoundingSphere,
	AxisAlignedBoundingBox,
};

struct SBoundingSphereData
{
	float Radius{ KBoundingSphereDefaultRadius };
	float RadiusBias{ KBoundingSphereDefaultRadius };
};

struct alignas(16) SBoundingVolume
{
	union UData
	{
		XMFLOAT3			AABBHalfSizes{ 1, 1, 1 };
		SBoundingSphereData	BS;
	};

	EBoundingVolumeType	eType{ EBoundingVolumeType::BoundingSphere };
	UData				Data{};
	XMVECTOR			Center{};
};

struct SObject3DInstanceCPUData
{
	static constexpr size_t KMaxNameLengthZeroTerminated{ 32 };

	std::string				Name{};
	XMVECTOR				Translation{};
	XMVECTOR				Scaling{ XMVectorSet(1, 1, 1, 0) };
	float					Pitch{};
	float					Yaw{};
	float					Roll{};
	SBoundingVolume			EditorBoundingSphere{};
};

struct SObject3DInstanceGPUData
{
	XMMATRIX	WorldMatrix{ KMatrixIdentity };
	float		IsHighlighted{};
};

struct SVertexBufferBundle
{
	SVertexBufferBundle(size_t _Stride) : Stride{ static_cast<UINT>(_Stride) } {}

	ComPtr<ID3D11Buffer>	Buffer{};
	UINT					Stride{};
	UINT					Offset{};
};

#define ENUM_CLASS_FLAG(enum_type)\
static enum_type operator|(enum_type a, enum_type b)\
{\
	return static_cast<enum_type>(static_cast<int>(a) | static_cast<int>(b));\
}\
static enum_type& operator|=(enum_type& a, enum_type b)\
{\
	a = static_cast<enum_type>(static_cast<int>(a) | static_cast<int>(b));\
	return a;\
}\
static enum_type operator&(enum_type a, enum_type b)\
{\
	return static_cast<enum_type>(static_cast<int>(a) & static_cast<int>(b));\
}\
static enum_type& operator&=(enum_type& a, enum_type b)\
{\
	a = static_cast<enum_type>(static_cast<int>(a) & static_cast<int>(b));\
	return a;\
}\
static enum_type operator^(enum_type a, enum_type b)\
{\
	return static_cast<enum_type>(static_cast<int>(a) ^ static_cast<int>(b)); \
}\
static enum_type& operator^=(enum_type& a, enum_type b)\
{\
	a = static_cast<enum_type>(static_cast<int>(a) ^ static_cast<int>(b)); \
	return a; \
}\
static enum_type operator~(enum_type a)\
{\
	return static_cast<enum_type>(~static_cast<int>(a)); \
}

#define EFLAG_HAS(Object, eFlag) (Object & eFlag) == eFlag
#define EFLAG_HAS_NO(Object, eFlag) (Object & eFlag) != eFlag

#define MB_WARN(Text, Title) MessageBox(nullptr, Text, Title, MB_OK | MB_ICONEXCLAMATION)