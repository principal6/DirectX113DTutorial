#pragma once

#include "../DirectXTK/DirectXTK.h"

using namespace DirectX;

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

class CObject3D;
struct SObjectIdentifier
{
	SObjectIdentifier() {}
	SObjectIdentifier(CObject3D* _Object3D) : Object3D{ _Object3D } {}
	SObjectIdentifier(CObject3D* _Object3D, const std::string& _InstanceName) : Object3D{ _Object3D }, InstanceName{ _InstanceName } {}

	CObject3D*	Object3D{};
	std::string	InstanceName{};
};

struct SObject3DInstanceCPUData
{
	static constexpr size_t KMaxNameLengthZeroTerminated{ 32 };

	std::string				Name{};
	SComponentTransform		Transform{};
	SComponentPhysics		Physics{};
	SBoundingVolume			EditorBoundingSphere{};
	size_t					CurrAnimPlayCount{};
	EAnimationOption		eCurrAnimOption{};
};

struct SObject3DInstanceGPUData
{
	XMMATRIX	WorldMatrix{ KMatrixIdentity };
	float		IsHighlighted{};
	float		AnimTick{};
	uint32_t	CurrAnimID{};
};
