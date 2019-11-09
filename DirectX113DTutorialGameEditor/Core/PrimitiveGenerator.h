#pragma once

#include "Object3D.h"
#include "Object3DLine.h"
#include "Object2D.h"
#include <unordered_map>

static const XMVECTOR KColorWhite{ XMVectorSet(1, 1, 1 ,1) };

static std::string ConvertXMVECTORToString(const XMVECTOR& Vector);
static void CalculateNormals(SMesh& Mesh);
static void AverageNormals(SMesh& Mesh);
static void CalculateTangents(SMesh& Mesh);
static std::vector<STriangle> GenerateContinuousQuads(int QuadCount);
static SMesh GenerateTriangle(const XMVECTOR& V0, const XMVECTOR& V1, const XMVECTOR& V2, const XMVECTOR& Color = KColorWhite);
static SMesh GenerateTriangle(const XMVECTOR& V0, const XMVECTOR& V1, const XMVECTOR& V2, const XMVECTOR& Color0, const XMVECTOR& Color1, const XMVECTOR& Color2);
static SMesh GenerateSquareXYPlane(const XMVECTOR& Color = KColorWhite);
static SMesh GenerateSquareXZPlane(const XMVECTOR& Color = KColorWhite);
static SMesh GenerateSquareYZPlane(const XMVECTOR& Color = KColorWhite);
static SMesh GenerateTerrainBase(const XMFLOAT2& Size, bool bSubdivideTexCoord = false, const XMVECTOR& Color = KColorWhite);
static SMesh GenerateCircleXZPlane(uint32_t SideCount = 16, const XMVECTOR& Color = KColorWhite);
static SMesh GeneratePyramid(const XMVECTOR& Color = KColorWhite);
static SMesh GenerateCube(const XMVECTOR& Color = KColorWhite);
static SMesh GenerateCone(float RadiusRatio = 0.0f, float Radius = 1.0f, float Height = 1.0f, uint32_t SideCount = 16, const XMVECTOR& Color = KColorWhite);
static SMesh GenerateCylinder(float Radius = 1.0f, float Height = 1.0f, uint32_t SideCount = 16, const XMVECTOR& Color = KColorWhite);
static SMesh GenerateSphere(uint32_t SegmentCount, const XMVECTOR& ColorTop, const XMVECTOR& ColorBottom);
static SMesh GenerateSphere(uint32_t SegmentCount = 16, const XMVECTOR& Color = KColorWhite);
static SMesh GenerateTorus(const XMVECTOR& Color = KColorWhite, float InnerRadius = 0.2f, uint32_t SideCount = 16, uint32_t SegmentCount = 24);
static void TranslateMesh(SMesh& Mesh, const XMVECTOR& Translation);
static void RotateMesh(SMesh& Mesh, float Pitch, float Yaw, float Roll);
static void ScaleMesh(SMesh& Mesh, const XMVECTOR& Scaling);
static void ScaleMeshTexCoord(SMesh& Mesh, const XMVECTOR& Scaling);
static SMesh MergeStaticMeshes(const SMesh& MeshA, const SMesh& MeshB);
static std::vector<SVertex3DLine> Generate3DLineCircleYZ(const XMVECTOR& Color = KColorWhite, uint32_t SegmentCount = 32);
static std::vector<SVertex3DLine> Generate3DGrid(int GuidelineCount = 10, float Interval = 1.0f);
static CObject2D::SData Generate2DRectangle(const XMFLOAT2& RectangleSize);

static bool operator==(const XMVECTOR& A, const XMVECTOR& B)
{
	return XMVector3Equal(A, B);
}

static std::string ConvertXMVECTORToString(const XMVECTOR& Vector)
{
	using std::to_string;

	float X{ XMVectorGetX(Vector) };
	float Y{ XMVectorGetY(Vector) };
	float Z{ XMVectorGetZ(Vector) };
	float W{ XMVectorGetW(Vector) };

	std::string Result{ to_string(X) + "#" + to_string(Y) + "#" + to_string(Z) + "#" + to_string(W) };
	return Result;
}

static void CalculateNormals(SMesh& Mesh)
{
	for (const STriangle& Triangle : Mesh.vTriangles)
	{
		SVertex3D& V0{ Mesh.vVertices[Triangle.I0] };
		SVertex3D& V1{ Mesh.vVertices[Triangle.I1] };
		SVertex3D& V2{ Mesh.vVertices[Triangle.I2] };

		XMVECTOR Edge01{ V1.Position - V0.Position };
		XMVECTOR Edge02{ V2.Position - V0.Position };

		XMVECTOR Normal{ XMVector3Normalize(XMVector3Cross(Edge01, Edge02)) };

		V0.Normal = V1.Normal = V2.Normal = Normal;
	}
}

static void AverageNormals(SMesh& Mesh)
{
	std::unordered_map<std::string, std::vector<XMVECTOR>> mapVertexToNormals{};
	for (const STriangle& Triangle : Mesh.vTriangles)
	{
		const SVertex3D& V0{ Mesh.vVertices[Triangle.I0] };
		const SVertex3D& V1{ Mesh.vVertices[Triangle.I1] };
		const SVertex3D& V2{ Mesh.vVertices[Triangle.I2] };

		std::string V0PositionStr{ ConvertXMVECTORToString(V0.Position) };
		std::string V1PositionStr{ ConvertXMVECTORToString(V1.Position) };
		std::string V2PositionStr{ ConvertXMVECTORToString(V2.Position) };

		const std::vector<XMVECTOR>& vV0Normals{ mapVertexToNormals[V0PositionStr] };
		{
			auto found{ std::find(vV0Normals.begin(), vV0Normals.end(), V0.Normal) };
			if (found == vV0Normals.end()) mapVertexToNormals[V0PositionStr].emplace_back(V0.Normal);
		}

		const std::vector<XMVECTOR>& vV1Normals{ mapVertexToNormals[V1PositionStr] };
		{
			auto found{ std::find(vV1Normals.begin(), vV1Normals.end(), V1.Normal) };
			if (found == vV1Normals.end()) mapVertexToNormals[V1PositionStr].emplace_back(V1.Normal);
		}

		const std::vector<XMVECTOR>& vV2Normals{ mapVertexToNormals[V2PositionStr] };
		{
			auto found{ std::find(vV2Normals.begin(), vV2Normals.end(), V2.Normal) };
			if (found == vV2Normals.end()) mapVertexToNormals[V2PositionStr].emplace_back(V2.Normal);
		}
	}

	for (SVertex3D& Vertex : Mesh.vVertices)
	{
		std::string VStr{ ConvertXMVECTORToString(Vertex.Position) };

		const std::vector<XMVECTOR>& vNormals{ mapVertexToNormals[VStr] };

		XMVECTOR NormalSum{};
		for (const auto& Normal : vNormals)
		{
			NormalSum += Normal;
		}
		XMVECTOR TriangleNormal{ NormalSum / (float)vNormals.size() };

		Vertex.Normal = TriangleNormal;
	}
}

static void CalculateTangents(SMesh& Mesh)
{
	for (STriangle& Triangle : Mesh.vTriangles)
	{
		SVertex3D& Vert0{ Mesh.vVertices[Triangle.I0] };
		SVertex3D& Vert1{ Mesh.vVertices[Triangle.I1] };
		SVertex3D& Vert2{ Mesh.vVertices[Triangle.I2] };

		XMVECTOR Edge01{ Vert1.Position - Vert0.Position };
		XMVECTOR Edge02{ Vert2.Position - Vert0.Position };

		XMVECTOR UV01{ Vert1.TexCoord - Vert0.TexCoord };
		XMVECTOR UV02{ Vert2.TexCoord - Vert0.TexCoord };

		float U01{ XMVectorGetX(UV01) };
		float V01{ XMVectorGetY(UV01) };

		float U02{ XMVectorGetX(UV02) };
		float V02{ XMVectorGetY(UV02) };

		// Edge01 = U01 * Tangent + V01 * Bitangent
		// Edge02 = U02 * Tangent + V02 * Bitangent

		// | Edge01 x y z | = | U01 V01 | * | Tangent	x y z |
		// | Edge02 x y z |   | U02 V02 |   | Bitangent	x y z |

		// ( Determinant == U01 * V02 - V01 * U02 )

		//       1		  *  |  V02 -V01 | * | Edge01 x y z | = | Tangent	x y z |
		//	Determinant		 | -U02  U01 | * | Edge02 x y z | = | Bitangent	x y z |

		float InverseDeterminant{ 1 / (U01 * V02 - V01 * U02) };
		Vert2.Tangent = Vert1.Tangent = Vert0.Tangent = XMVector3Normalize(InverseDeterminant * V02 * Edge01 - V01 * Edge02);
		//Vert2.Bitangent = Vert1.Bitangent = Vert0.Bitangent = XMVector3Normalize(InverseDeterminant * -U02 * Edge01 + U01 * Edge02);
	}
}

static void AverageTangents(SMesh& Mesh)
{
	std::unordered_map<std::string, std::vector<XMVECTOR>> mapVertexToTangents{};
	for (const STriangle& Triangle : Mesh.vTriangles)
	{
		const SVertex3D& V0{ Mesh.vVertices[Triangle.I0] };
		const SVertex3D& V1{ Mesh.vVertices[Triangle.I1] };
		const SVertex3D& V2{ Mesh.vVertices[Triangle.I2] };

		std::string V0PositionStr{ ConvertXMVECTORToString(V0.Position) };
		std::string V1PositionStr{ ConvertXMVECTORToString(V1.Position) };
		std::string V2PositionStr{ ConvertXMVECTORToString(V2.Position) };

		const std::vector<XMVECTOR>& vV0Tangents{ mapVertexToTangents[V0PositionStr] };
		{
			auto found{ std::find(vV0Tangents.begin(), vV0Tangents.end(), V0.Tangent) };
			if (found == vV0Tangents.end()) mapVertexToTangents[V0PositionStr].emplace_back(V0.Tangent);
		}

		const std::vector<XMVECTOR>& vV1Tangents{ mapVertexToTangents[V1PositionStr] };
		{
			auto found{ std::find(vV1Tangents.begin(), vV1Tangents.end(), V1.Tangent) };
			if (found == vV1Tangents.end()) mapVertexToTangents[V1PositionStr].emplace_back(V1.Tangent);
		}

		const std::vector<XMVECTOR>& vV2Tangents{ mapVertexToTangents[V2PositionStr] };
		{
			auto found{ std::find(vV2Tangents.begin(), vV2Tangents.end(), V2.Tangent) };
			if (found == vV2Tangents.end()) mapVertexToTangents[V2PositionStr].emplace_back(V2.Tangent);
		}
	}

	for (SVertex3D& Vertex : Mesh.vVertices)
	{
		std::string VStr{ ConvertXMVECTORToString(Vertex.Position) };

		const std::vector<XMVECTOR>& vTangents{ mapVertexToTangents[VStr] };

		XMVECTOR TangentSum{};
		for (const auto& Tangent : vTangents)
		{
			TangentSum += Tangent;
		}
		XMVECTOR TriangleTangent{ TangentSum / (float)vTangents.size() };

		Vertex.Tangent = TriangleTangent;
	}
}

static std::vector<STriangle> GenerateContinuousQuads(int QuadCount)
{
	std::vector<STriangle> vTriangles{};

	for (int iQuad = 0; iQuad < QuadCount; ++iQuad)
	{
		vTriangles.emplace_back(iQuad * 4 + 0, iQuad * 4 + 1, iQuad * 4 + 2);
		vTriangles.emplace_back(iQuad * 4 + 1, iQuad * 4 + 3, iQuad * 4 + 2);
	}

	return vTriangles;
}

static SMesh GenerateTriangle(const XMVECTOR& V0, const XMVECTOR& V1, const XMVECTOR& V2, const XMVECTOR& Color)
{
	return GenerateTriangle(V0, V1, V2, Color, Color, Color);
}

static SMesh GenerateTriangle(const XMVECTOR& V0, const XMVECTOR& V1, const XMVECTOR& V2, const XMVECTOR& Color0, const XMVECTOR& Color1, const XMVECTOR& Color2)
{
	SMesh Mesh{};

	Mesh.vVertices.emplace_back(V0, Color0, XMVectorSet(0.5f, 0.0f, 0, 0));
	Mesh.vVertices.emplace_back(V1, Color1, XMVectorSet(1.0f, 1.0f, 0, 0));
	Mesh.vVertices.emplace_back(V2, Color2, XMVectorSet(0.0f, 1.0f, 0, 0));

	Mesh.vTriangles.emplace_back(0, 1, 2);

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateSquareXYPlane(const XMVECTOR& Color)
{
	constexpr float KHalfLengthX{ 0.5f };
	constexpr float KHalfLengthY{ 0.5f };

	SMesh Mesh{};

	Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, 0.0f, 1), Color, XMVectorSet(0, 0, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, 0.0f, 1), Color, XMVectorSet(1, 0, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, 0.0f, 1), Color, XMVectorSet(0, 1, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, 0.0f, 1), Color, XMVectorSet(1, 1, 0, 0));

	Mesh.vTriangles = GenerateContinuousQuads(1);

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateSquareXZPlane(const XMVECTOR& Color)
{
	constexpr float KHalfLengthX{ 0.5f };
	constexpr float KHalfLengthZ{ 0.5f };

	SMesh Mesh{};

	Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, 0.0f, +KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, 0.0f, +KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, 0.0f, -KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, 0.0f, -KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

	Mesh.vTriangles = GenerateContinuousQuads(1);

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateSquareYZPlane(const XMVECTOR& Color)
{
	constexpr float KHalfLengthY{ 0.5f };
	constexpr float KHalfLengthZ{ 0.5f };

	SMesh Mesh{};

	Mesh.vVertices.emplace_back(XMVectorSet(0.0f, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(0.0f, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(0.0f, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
	Mesh.vVertices.emplace_back(XMVectorSet(0.0f, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

	Mesh.vTriangles = GenerateContinuousQuads(1);

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateTerrainBase(const XMFLOAT2& Size, bool bSubdivideTexCoord, const XMVECTOR& Color)
{
	using std::max;

	int SizeX{ max((int)Size.x, 2) };
	int SizeZ{ max((int)Size.y, 2) };
	if (SizeX % 2) ++SizeX;
	if (SizeZ % 2) ++SizeZ;

	SMesh Mesh{};
	float U0{}, V0{}, U1{}, V1{};
	for (int z = 0; z < SizeZ; ++z)
	{
		for (int x = 0; x < SizeX; ++x)
		{
			if (bSubdivideTexCoord)
			{
				U0 = (float)(x)		/ (float)SizeX;
				U1 = (float)(x + 1)	/ (float)SizeX;

				V0 = (float)(z)		/ (float)SizeZ;
				V1 = (float)(z + 1)	/ (float)SizeZ;
			}
			else
			{
				U0 = V0 = 0.0f;
				U1 = V1 = 1.0f;
			}
			Mesh.vVertices.emplace_back(XMVectorSet(static_cast<float>(x + 0), +0.0f, static_cast<float>(-z + 0), 1), Color, XMVectorSet(U0, V0, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(static_cast<float>(x + 1), +0.0f, static_cast<float>(-z + 0), 1), Color, XMVectorSet(U1, V0, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(static_cast<float>(x + 0), +0.0f, static_cast<float>(-z - 1), 1), Color, XMVectorSet(U0, V1, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(static_cast<float>(x + 1), +0.0f, static_cast<float>(-z - 1), 1), Color, XMVectorSet(U1, V1, 0, 0));
		}
	}

	Mesh.vTriangles = GenerateContinuousQuads(SizeX * SizeZ);

	CalculateNormals(Mesh);

	CalculateTangents(Mesh);

	TranslateMesh(Mesh, XMVectorSet(static_cast<float>(-SizeX / 2), 0, static_cast<float>(SizeZ / 2), 1));
	
	return Mesh;
}

static SMesh GenerateCircleXZPlane(uint32_t SideCount, const XMVECTOR& Color)
{
	using std::max;

	constexpr float KRadius{ 1.0f };

	SideCount = max(SideCount, (uint32_t)8);
	const float KThetaUnit{ XM_2PI / SideCount };

	SMesh Mesh{};
	for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
	{
		float Theta0{ KThetaUnit * iSide };
		float Theta1{ KThetaUnit * (iSide + 1) };

		float Cos0{ cosf(Theta0) };
		float Cos1{ cosf(Theta1) };

		float Sin0{ sinf(Theta0) };
		float Sin1{ sinf(Theta1) };

		Mesh.vVertices.emplace_back(XMVectorSet(+0.0f, +0.0f, +0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+Cos1, +0.0f, +Sin1, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+Cos0, +0.0f, +Sin0, 1), Color);
	}
	
	for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
	{
		Mesh.vTriangles.emplace_back(iSide * 3 + 0, iSide * 3 + 1, iSide * 3 + 2);
	}

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GeneratePyramid(const XMVECTOR& Color)
{
	constexpr float KHalfLengthX{ 0.5f };
	constexpr float KHalfLengthY{ 0.5f };
	constexpr float KHalfLengthZ{ 0.5f };

	SMesh Mesh{};
	{
		// Front
		Mesh.vVertices.emplace_back(XMVectorSet(0.0f, +KHalfLengthY, 0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color);

		// Left
		Mesh.vVertices.emplace_back(XMVectorSet(0.0f, +KHalfLengthY, 0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color);

		// Right
		Mesh.vVertices.emplace_back(XMVectorSet(0.0f, +KHalfLengthY, 0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color);

		// Back
		Mesh.vVertices.emplace_back(XMVectorSet(0.0f, +KHalfLengthY, 0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color);

		// Bottom
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color);
	}
	
	{
		Mesh.vTriangles.emplace_back(0, 1, 2);
		Mesh.vTriangles.emplace_back(3, 4, 5);
		Mesh.vTriangles.emplace_back(6, 7, 8);
		Mesh.vTriangles.emplace_back(9, 10, 11);
		Mesh.vTriangles.emplace_back(12, 13, 14);
		Mesh.vTriangles.emplace_back(13, 15, 14);
	}

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateCube(const XMVECTOR& Color)
{
	constexpr float KHalfLengthX{ 0.5f };
	constexpr float KHalfLengthY{ 0.5f };
	constexpr float KHalfLengthZ{ 0.5f };

	SMesh Mesh{};
	{
		// Front face
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Left face
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Right face
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Back face
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Top face
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, +KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Bottom face
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, -KHalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(-KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Mesh.vVertices.emplace_back(XMVectorSet(+KHalfLengthX, -KHalfLengthY, +KHalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));
	}

	Mesh.vTriangles = GenerateContinuousQuads(6);
	
	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateCone(float RadiusRatio, float Radius, float Height, uint32_t SideCount, const XMVECTOR& Color)
{
	using std::max;

	SideCount = max(SideCount, (uint32_t)3);
	const float KHalfHeight{ Height / 2.0f };
	const float KThetaUnit{ XM_2PI / SideCount };

	SMesh Mesh{};

	for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
	{
		float Theta0{ KThetaUnit * iSide };
		float Theta1{ KThetaUnit * (iSide + 1) };

		float Cos0{ cosf(Theta0) };
		float Cos1{ cosf(Theta1) };

		float Sin0{ sinf(Theta0) };
		float Sin1{ sinf(Theta1) };

		// Bottom
		Mesh.vVertices.emplace_back(XMVectorSet(+0.0f, -KHalfHeight, +0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+Cos0 * Radius, -KHalfHeight, +Sin0 * Radius, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+Cos1 * Radius, -KHalfHeight, +Sin1 * Radius, 1), Color);

		// Top
		Mesh.vVertices.emplace_back(XMVectorSet(+0.0f, +KHalfHeight, +0.0f, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+Cos1 * Radius * RadiusRatio, +KHalfHeight, +Sin1 * Radius * RadiusRatio, 1), Color);
		Mesh.vVertices.emplace_back(XMVectorSet(+Cos0 * Radius * RadiusRatio, +KHalfHeight, +Sin0 * Radius * RadiusRatio, 1), Color);
	}

	for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
	{
		// Bottom
		Mesh.vTriangles.emplace_back(iSide * 6 + 0, iSide * 6 + 1, iSide * 6 + 2);

		// Top
		Mesh.vTriangles.emplace_back(iSide * 6 + 3, iSide * 6 + 4, iSide * 6 + 5);

		// Side
		Mesh.vTriangles.emplace_back(iSide * 6 + 5, iSide * 6 + 4, iSide * 6 + 1);
		Mesh.vTriangles.emplace_back(iSide * 6 + 1, iSide * 6 + 4, iSide * 6 + 2);
	}

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateCylinder(float Radius, float Height, uint32_t SideCount, const XMVECTOR& Color)
{
	return GenerateCone(1.0f, Radius, Height, SideCount, Color);
}

static SMesh GenerateSphere(uint32_t SegmentCount, const XMVECTOR& ColorTop, const XMVECTOR& ColorBottom)
{
	using std::max;

	SegmentCount = max(SegmentCount, (uint32_t)4);
	if (SegmentCount % 2) ++SegmentCount;
	const float KThetaUnit{ XM_2PI / SegmentCount };
	const float KPhiUnit{ XM_2PI / SegmentCount };

	SMesh Mesh{};
	for (uint32_t PolarStep = 0; PolarStep < SegmentCount / 2; ++PolarStep) // phi
	{
		float Phi0{ KPhiUnit * PolarStep };
		float Phi1{ KPhiUnit * (PolarStep + 1) };

		float Y0{ cosf(Phi0) };
		float Y1{ cosf(Phi1) };

		float XZLengthY0{ sinf(Phi0) };
		float XZLengthY1{ sinf(Phi1) };

		float PolarRatio0{ static_cast<float>(PolarStep) / (static_cast<float>(SegmentCount) / 2.0f) };
		float PolarRatio1{ static_cast<float>(PolarStep + 1) / (static_cast<float>(SegmentCount) / 2.0f) };
		XMVECTOR Color0{ ColorBottom * PolarRatio0 + ColorTop * (1.0f - PolarRatio0) };
		XMVECTOR Color1{ ColorBottom * PolarRatio1 + ColorTop * (1.0f - PolarRatio1) };

		for (uint32_t AzimuthStep = 0; AzimuthStep < SegmentCount; ++AzimuthStep) // theta
		{
			float Theta0{ KThetaUnit * AzimuthStep };
			float Theta1{ KThetaUnit * (AzimuthStep + 1) };

			float X0{ cosf(Theta0) };
			float X1{ cosf(Theta1) };

			float Z0{ sinf(Theta0) };
			float Z1{ sinf(Theta1) };

			float AzimuthRatio0{ static_cast<float>(AzimuthStep) / (static_cast<float>(SegmentCount)) };
			float AzimuthRatio1{ static_cast<float>(AzimuthStep + 1) / (static_cast<float>(SegmentCount)) };

			// Side 0
			Mesh.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY0, +Y0, +Z0 * XZLengthY0, 1), Color0, XMVectorSet(AzimuthRatio0, PolarRatio0, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY0, +Y0, +Z1 * XZLengthY0, 1), Color0, XMVectorSet(AzimuthRatio1, PolarRatio0, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY1, +Y1, +Z0 * XZLengthY1, 1), Color1, 
				XMVectorSet((PolarRatio1 == 1.0f) ? 0.5f : AzimuthRatio0, PolarRatio1, 0, 0));

			// Side 1
			Mesh.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY0, +Y0, +Z1 * XZLengthY0, 1), Color0,
				XMVectorSet((PolarRatio0 == 0.0f) ? 0.5f : AzimuthRatio1, PolarRatio0, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY1, +Y1, +Z1 * XZLengthY1, 1), Color1, XMVectorSet(AzimuthRatio1, PolarRatio1, 0, 0));
			Mesh.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY1, +Y1, +Z0 * XZLengthY1, 1), Color1, XMVectorSet(AzimuthRatio0, PolarRatio1, 0, 0));
		}
	}

	for (uint32_t iSide = 0; iSide < SegmentCount * SegmentCount / 2; ++iSide)
	{
		// Side 0
		Mesh.vTriangles.emplace_back(iSide * 6 + 0, iSide * 6 + 1, iSide * 6 + 2);

		// Side 1
		Mesh.vTriangles.emplace_back(iSide * 6 + 3, iSide * 6 + 4, iSide * 6 + 5);
	}

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static SMesh GenerateSphere(uint32_t SegmentCount, const XMVECTOR& Color)
{
	return GenerateSphere(SegmentCount, Color, Color);
}

static SMesh GenerateTorus(const XMVECTOR& Color, float InnerRadius, uint32_t SideCount, uint32_t SegmentCount)
{
	using std::max;

	SegmentCount = max(SegmentCount, (uint32_t)3);
	SideCount = max(SideCount, (uint32_t)3);
	const float KThetaUnit{ XM_2PI / SegmentCount };
	const float KPhiUnit{ XM_2PI / SideCount };

	SMesh Mesh{};
	for (uint32_t iSegment = 0; iSegment < SegmentCount; ++iSegment)
	{
		float Theta0{ KThetaUnit * iSegment };
		float Theta1{ KThetaUnit * (iSegment + 1) };

		float OuterX0{ cos(Theta0) };
		float OuterX1{ cos(Theta1) };

		float OuterZ0{ sin(Theta0) };
		float OuterZ1{ sin(Theta1) };

		for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
		{
			float Phi0{ KPhiUnit * iSide };
			float Phi1{ KPhiUnit * (iSide + 1) };

			float InnerXLow{ InnerRadius * cos(Phi0) };
			float InnerXHigh{ InnerRadius * cos(Phi1) };

			float InnerYLow{ InnerRadius * sin(Phi0) };
			float InnerYHigh{ InnerRadius * sin(Phi1) };

			Mesh.vVertices.emplace_back(XMVectorSet(OuterX0 + OuterX0 * InnerXHigh, +InnerYHigh, +OuterZ0 + OuterZ0 * InnerXHigh, 1), Color);
			Mesh.vVertices.emplace_back(XMVectorSet(OuterX1 + OuterX1 * InnerXHigh, +InnerYHigh, +OuterZ1 + OuterZ1 * InnerXHigh, 1), Color);
			Mesh.vVertices.emplace_back(XMVectorSet(OuterX0 + OuterX0 * InnerXLow, +InnerYLow, +OuterZ0 + OuterZ0 * InnerXLow, 1), Color);
			Mesh.vVertices.emplace_back(XMVectorSet(OuterX1 + OuterX1 * InnerXLow, +InnerYLow, +OuterZ1 + OuterZ1 * InnerXLow, 1), Color);
		}
	}
	
	for (uint32_t iSegment = 0; iSegment < SegmentCount; ++iSegment)
	{
		for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
		{
			// Side 0
			Mesh.vTriangles.emplace_back(
				iSegment * SideCount * 4 + iSide * 4 + 0,
				iSegment * SideCount * 4 + iSide * 4 + 1,
				iSegment * SideCount * 4 + iSide * 4 + 2);

			// Side 1
			Mesh.vTriangles.emplace_back(
				iSegment * SideCount * 4 + iSide * 4 + 1, 
				iSegment * SideCount * 4 + iSide * 4 + 3, 
				iSegment * SideCount * 4 + iSide * 4 + 2);
		}
	}

	CalculateNormals(Mesh);

	AverageNormals(Mesh);

	return Mesh;
}

static void TranslateMesh(SMesh& Mesh, const XMVECTOR& Translation)
{
	XMMATRIX Matrix{ XMMatrixTranslationFromVector(Translation) };
	for (auto& Vertex : Mesh.vVertices)
	{
		Vertex.Position = XMVector3TransformCoord(Vertex.Position, Matrix);
		Vertex.Normal = XMVector3TransformNormal(Vertex.Normal, Matrix);
	}
}

static void RotateMesh(SMesh& Mesh, float Pitch, float Yaw, float Roll)
{
	XMMATRIX Matrix{ XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll) };
	for (auto& Vertex : Mesh.vVertices)
	{
		Vertex.Position = XMVector3TransformCoord(Vertex.Position, Matrix);
		Vertex.Normal = XMVector3TransformNormal(Vertex.Normal, Matrix);
	}
}

static void ScaleMesh(SMesh& Mesh, const XMVECTOR& Scaling)
{
	XMMATRIX Matrix{ XMMatrixScalingFromVector(Scaling) };
	for (auto& Vertex : Mesh.vVertices)
	{
		Vertex.Position = XMVector3TransformCoord(Vertex.Position, Matrix);
		Vertex.Normal = XMVector3TransformNormal(Vertex.Normal, Matrix);
	}
}

static void ScaleMeshTexCoord(SMesh& Mesh, const XMVECTOR& Scaling)
{
	XMMATRIX Matrix{ XMMatrixScalingFromVector(Scaling) };
	for (auto& Vertex : Mesh.vVertices)
	{
		Vertex.TexCoord = XMVector3TransformCoord(Vertex.TexCoord, Matrix);
	}
}

static SMesh MergeStaticMeshes(const SMesh& MeshA, const SMesh& MeshB)
{
	SMesh MergedMesh{ MeshA };

	size_t MeshAVertexCount{ MeshA.vVertices.size() };

	for (auto& Vertex : MeshB.vVertices)
	{
		MergedMesh.vVertices.emplace_back(Vertex);
	}

	for (auto& Triangle : MeshB.vTriangles)
	{
		STriangle NewTriangle
		{
			static_cast<uint32_t>(MeshAVertexCount + Triangle.I0),
			static_cast<uint32_t>(MeshAVertexCount + Triangle.I1),
			static_cast<uint32_t>(MeshAVertexCount + Triangle.I2)
		};

		MergedMesh.vTriangles.emplace_back(NewTriangle);
	}

	return MergedMesh;
}

static std::vector<SVertex3DLine> Generate3DLineCircleYZ(const XMVECTOR& Color, uint32_t SegmentCount)
{
	std::vector<SVertex3DLine> vVertices{};

	for (uint32_t iSegment = 0; iSegment < SegmentCount; ++iSegment)
	{
		float Theta0{ XM_2PI * (static_cast<float>(iSegment) / SegmentCount) };
		float Theta1{ XM_2PI * (static_cast<float>(iSegment + 1) / SegmentCount) };

		vVertices.emplace_back(SVertex3DLine(XMVectorSet(0, sin(Theta0), cos(Theta0), 1), Color));
		vVertices.emplace_back(SVertex3DLine(XMVectorSet(0, sin(Theta1), cos(Theta1), 1), Color));
	}

	return vVertices;
}

static std::vector<SVertex3DLine> Generate3DGrid(int GuidelineCount, float Interval)
{
	using std::max;

	static constexpr float KAxisHalfLength{ 1000.0f };
	static const XMVECTOR KColorAxisX{ XMVectorSet(1, 0, 0, 1) };
	static const XMVECTOR KColorAxisY{ XMVectorSet(0, 1, 0, 1) };
	static const XMVECTOR KColorAxisZ{ XMVectorSet(0, 0, 1, 1) };

	GuidelineCount = max(GuidelineCount, 0);
	Interval = max(Interval, 0.1f);
	float EndPosition{ -GuidelineCount * Interval };
	
	std::vector<SVertex3DLine> vVertices{};

	for (int z = 0; z < GuidelineCount * 2 + 1; ++z)
	{
		if (z == GuidelineCount)
		{
			vVertices.emplace_back(SVertex3DLine(XMVectorSet(0, 0, +KAxisHalfLength, 1), KColorAxisZ));
			vVertices.emplace_back(SVertex3DLine(XMVectorSet(0, 0, -KAxisHalfLength, 1), KColorAxisZ));
			continue;
		}
		vVertices.emplace_back(SVertex3DLine(XMVectorSet(+EndPosition + Interval * z, 0, +EndPosition, 1), KColorWhite));
		vVertices.emplace_back(SVertex3DLine(XMVectorSet(+EndPosition + Interval * z, 0, -EndPosition, 1), KColorWhite));
	}

	for (int x = 0; x < GuidelineCount * 2 + 1; ++x)
	{
		if (x == GuidelineCount)
		{
			vVertices.emplace_back(SVertex3DLine(XMVectorSet(0, +KAxisHalfLength, 0, 1), KColorAxisY));
			vVertices.emplace_back(SVertex3DLine(XMVectorSet(0, -KAxisHalfLength, 0, 1), KColorAxisY));
			continue;
		}
		vVertices.emplace_back(SVertex3DLine(XMVectorSet(+EndPosition, 0, +EndPosition + Interval * x, 1), KColorWhite));
		vVertices.emplace_back(SVertex3DLine(XMVectorSet(-EndPosition, 0, +EndPosition + Interval * x, 1), KColorWhite));
	}

	vVertices.emplace_back(SVertex3DLine(XMVectorSet(+KAxisHalfLength, 0, 0, 1), KColorAxisX));
	vVertices.emplace_back(SVertex3DLine(XMVectorSet(-KAxisHalfLength, 0, 0, 1), KColorAxisX));

	return vVertices;
}

static CObject2D::SData Generate2DRectangle(const XMFLOAT2& RectangleSize)
{
	const float KHalfWidth{ RectangleSize.x / 2 };
	const float KHalfHeight{ RectangleSize.y / 2 };

	CObject2D::SData Result{};

	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(-KHalfWidth, +KHalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(0.0f, 0.0f, 0, 0)));
	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(+KHalfWidth, +KHalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(1.0f, 0.0f, 0, 0)));
	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(-KHalfWidth, -KHalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(0.0f, 1.0f, 0, 0)));
	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(+KHalfWidth, -KHalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(1.0f, 1.0f, 0, 0)));

	Result.vTriangles.emplace_back(STriangle(0, 1, 2));
	Result.vTriangles.emplace_back(STriangle(1, 3, 2));

	return Result;
}
