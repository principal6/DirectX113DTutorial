#pragma once

#include "Object3D.h"
#include "Object3DLine.h"
#include "Object2D.h"
#include <unordered_map>

static const XMMATRIX KMatrixIdentity{ XMMatrixIdentity() };

static bool operator==(const XMVECTOR& A, const XMVECTOR& B)
{
	return XMVector3Equal(A, B);
}

static string ConvertXMVECTORToString(const XMVECTOR& Vector)
{
	float X{ XMVectorGetX(Vector) };
	float Y{ XMVectorGetY(Vector) };
	float Z{ XMVectorGetZ(Vector) };
	float W{ XMVectorGetW(Vector) };

	string Result{ to_string(X) + "#" + to_string(Y) + "#" + to_string(Z) + "#" + to_string(W) };
	return Result;
}

static void CalculateVertexNormalsFromFaceNormals(SMesh& Object3DData)
{
	using std::unordered_map;
	using std::pair;

	unordered_map<string, vector<XMVECTOR>> MapVertexToNormals{};
	for (const STriangle& Triangle : Object3DData.vTriangles)
	{
		SVertex3D& V0{ Object3DData.vVertices[Triangle.I0] };
		SVertex3D& V1{ Object3DData.vVertices[Triangle.I1] };
		SVertex3D& V2{ Object3DData.vVertices[Triangle.I2] };

		string V0Str{ ConvertXMVECTORToString(V0.Position) };
		string V1Str{ ConvertXMVECTORToString(V1.Position) };
		string V2Str{ ConvertXMVECTORToString(V2.Position) };

		vector<XMVECTOR>& V0List{ MapVertexToNormals[V0Str] };
		{
			auto found{ std::find(V0List.begin(), V0List.end(), V0.Normal) };
			if (found == V0List.end()) MapVertexToNormals[V0Str].emplace_back(V0.Normal);
		}
		
		vector<XMVECTOR>& V1List{ MapVertexToNormals[V1Str] };
		{
			auto found{ std::find(V1List.begin(), V1List.end(), V1.Normal) };
			if (found == V1List.end()) MapVertexToNormals[V1Str].emplace_back(V1.Normal);
		}
		
		vector<XMVECTOR>& V2List{ MapVertexToNormals[V2Str] };
		{
			auto found{ std::find(V2List.begin(), V2List.end(), V2.Normal) };
			if (found == V2List.end()) MapVertexToNormals[V2Str].emplace_back(V2.Normal);
		}
	}

	for (SVertex3D& Vertex : Object3DData.vVertices)
	{
		string VStr{ ConvertXMVECTORToString(Vertex.Position) };

		vector<XMVECTOR> NormalList{ MapVertexToNormals[VStr] };

		XMVECTOR Sum{};
		for (const auto& i : NormalList)
		{
			Sum += i;
		}
		XMVECTOR TriangleNormal{ Sum / (float)NormalList.size() };

		Vertex.Normal = TriangleNormal;
	}
}

static void CalculateFaceNormals(SMesh& Object3DData)
{
	for (const STriangle& Triangle : Object3DData.vTriangles)
	{
		SVertex3D& V0{ Object3DData.vVertices[Triangle.I0] };
		SVertex3D& V1{ Object3DData.vVertices[Triangle.I1] };
		SVertex3D& V2{ Object3DData.vVertices[Triangle.I2] };

		XMVECTOR Edge01{ V1.Position - V0.Position };
		XMVECTOR Edge02{ V2.Position - V0.Position };

		XMVECTOR Normal{ XMVector3Normalize(XMVector3Cross(Edge01, Edge02)) };

		V0.Normal = V1.Normal = V2.Normal = Normal;
	}
}

static vector<STriangle> GenerateContinuousFaces(int FaceCount)
{
	vector<STriangle> vTriangles{};

	for (int FaceIndex = 0; FaceIndex < FaceCount; ++FaceIndex)
	{
		vTriangles.emplace_back(FaceIndex * 4 + 0, FaceIndex * 4 + 1, FaceIndex * 4 + 2);
		vTriangles.emplace_back(FaceIndex * 4 + 1, FaceIndex * 4 + 3, FaceIndex * 4 + 2);
	}

	return vTriangles;
}

static SMesh GenerateTriangle(const XMVECTOR& V0, const XMVECTOR& V1, const XMVECTOR& V2, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	Data.vVertices.emplace_back(V0, Color);
	Data.vVertices.emplace_back(V1, Color);
	Data.vVertices.emplace_back(V2, Color);
	
	Data.vTriangles.emplace_back(0, 1, 2);

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateSquareXZPlane(const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	constexpr float HalfLengthX{ 0.5f };
	constexpr float HalfLengthZ{ 0.5f };

	Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +0.0f, +HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
	Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +0.0f, +HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
	Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +0.0f, -HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
	Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +0.0f, -HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

	Data.vTriangles = GenerateContinuousFaces(1);

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateSquareYZPlane(const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	constexpr float HalfLengthY{ 0.5f };
	constexpr float HalfLengthZ{ 0.5f };

	Data.vVertices.emplace_back(XMVectorSet(0.0f, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
	Data.vVertices.emplace_back(XMVectorSet(0.0f, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
	Data.vVertices.emplace_back(XMVectorSet(0.0f, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
	Data.vVertices.emplace_back(XMVectorSet(0.0f, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

	Data.vTriangles = GenerateContinuousFaces(1);

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateCircleXZPlane(uint32_t SideCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	SideCount = max(SideCount, (uint32_t)8);

	constexpr float Radius{ 1.0f };
	const float ThetaUnit{ XM_2PI / SideCount };
	for (uint32_t i = 0; i < SideCount; ++i)
	{
		float Theta0{ ThetaUnit * i };
		float Theta1{ ThetaUnit * (i + 1) };

		float Cos0{ cosf(Theta0) };
		float Cos1{ cosf(Theta1) };

		float Sin0{ sinf(Theta0) };
		float Sin1{ sinf(Theta1) };

		Data.vVertices.emplace_back(XMVectorSet(+0.0f, +0.0f, +0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos1, +0.0f, +Sin1, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos0, +0.0f, +Sin0, 1), Color);
	}
	
	for (uint32_t i = 0; i < SideCount; ++i)
	{
		Data.vTriangles.emplace_back(i * 3 + 0, i * 3 + 1, i * 3 + 2);
	}

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GeneratePyramid(const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	constexpr float HalfLengthX{ 0.5f };
	constexpr float HalfLengthY{ 0.5f };
	constexpr float HalfLengthZ{ 0.5f };

	{
		// Front
		Data.vVertices.emplace_back(XMVectorSet(0.0f, +HalfLengthY, 0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color);

		// Left
		Data.vVertices.emplace_back(XMVectorSet(0.0f, +HalfLengthY, 0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color);

		// Right
		Data.vVertices.emplace_back(XMVectorSet(0.0f, +HalfLengthY, 0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color);

		// Back
		Data.vVertices.emplace_back(XMVectorSet(0.0f, +HalfLengthY, 0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color);

		// Bottom
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color);
	}
	
	{
		Data.vTriangles.emplace_back(0, 1, 2);
		Data.vTriangles.emplace_back(3, 4, 5);
		Data.vTriangles.emplace_back(6, 7, 8);
		Data.vTriangles.emplace_back(9, 10, 11);
		Data.vTriangles.emplace_back(12, 13, 14);
		Data.vTriangles.emplace_back(13, 15, 14);
	}

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateCube(const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	constexpr float HalfLengthX{ 0.5f };
	constexpr float HalfLengthY{ 0.5f };
	constexpr float HalfLengthZ{ 0.5f };

	{
		// Front face
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Left face
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Right face
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Back face
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Top face
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, +HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));

		// Bottom face
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(0, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, -HalfLengthZ, 1), Color, XMVectorSet(1, 0, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(-HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(0, 1, 0, 0));
		Data.vVertices.emplace_back(XMVectorSet(+HalfLengthX, -HalfLengthY, +HalfLengthZ, 1), Color, XMVectorSet(1, 1, 0, 0));
	}

	Data.vTriangles = GenerateContinuousFaces(6);
	
	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateCone(float RadiusRatio = 0.0f, float Radius = 1.0f, float Height = 1.0f, 
	uint32_t SideCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	SideCount = max(SideCount, (uint32_t)3);

	const float KHalfHeight{ Height / 2.0f };
	const float KThetaUnit{ XM_2PI / SideCount };
	for (uint32_t i = 0; i < SideCount; ++i)
	{
		float Theta0{ KThetaUnit * i };
		float Theta1{ KThetaUnit * (i + 1) };

		float Cos0{ cosf(Theta0) };
		float Cos1{ cosf(Theta1) };

		float Sin0{ sinf(Theta0) };
		float Sin1{ sinf(Theta1) };

		// Bottom
		Data.vVertices.emplace_back(XMVectorSet(+0.0f, -KHalfHeight, +0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos0 * Radius, -KHalfHeight, +Sin0 * Radius, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos1 * Radius, -KHalfHeight, +Sin1 * Radius, 1), Color);

		// Top
		Data.vVertices.emplace_back(XMVectorSet(+0.0f, +KHalfHeight, +0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos1 * Radius * RadiusRatio, +KHalfHeight, +Sin1 * Radius * RadiusRatio, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos0 * Radius * RadiusRatio, +KHalfHeight, +Sin0 * Radius * RadiusRatio, 1), Color);
	}

	for (uint32_t i = 0; i < SideCount; ++i)
	{
		// Bottom
		Data.vTriangles.emplace_back(i * 6 + 0, i * 6 + 1, i * 6 + 2);

		// Top
		Data.vTriangles.emplace_back(i * 6 + 3, i * 6 + 4, i * 6 + 5);

		// Side
		Data.vTriangles.emplace_back(i * 6 + 5, i * 6 + 4, i * 6 + 1);
		Data.vTriangles.emplace_back(i * 6 + 1, i * 6 + 4, i * 6 + 2);
	}

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateCylinder(float Radius = 1.0f, float Height = 1.0f, uint32_t SideCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	return GenerateCone(1.0f, Radius, Height, SideCount, Color);
}

static SMesh GenerateSphere(uint32_t SegmentCount, const XMVECTOR& ColorTop, const XMVECTOR& ColorBottom)
{
	SMesh Data{};

	SegmentCount = max(SegmentCount, (uint32_t)4);
	if (SegmentCount % 2) ++SegmentCount;

	const float PhiUnit{ XM_2PI / SegmentCount };
	const float ThetaUnit{ XM_2PI / SegmentCount };
	for (uint32_t polar_step = 0; polar_step < SegmentCount / 2; ++polar_step) // phi
	{
		float Phi0{ PhiUnit * polar_step };
		float Phi1{ PhiUnit * (polar_step + 1) };

		float Y0{ cosf(Phi0) };
		float Y1{ cosf(Phi1) };

		float XZLengthY0{ sinf(Phi0) };
		float XZLengthY1{ sinf(Phi1) };

		float Interpolation0{ static_cast<float>(polar_step) / (static_cast<float>(SegmentCount) / 2.0f) };
		float Interpolation1{ static_cast<float>(polar_step + 1) / (static_cast<float>(SegmentCount) / 2.0f) };
		XMVECTOR Color0{ ColorBottom * Interpolation0 + ColorTop * (1.0f - Interpolation0) };
		XMVECTOR Color1{ ColorBottom * Interpolation1 + ColorTop * (1.0f - Interpolation1) };

		for (uint32_t azimuth_step = 0; azimuth_step < SegmentCount; ++azimuth_step) // theta
		{
			float Theta0{ ThetaUnit * azimuth_step };
			float Theta1{ ThetaUnit * (azimuth_step + 1) };

			float X0{ cosf(Theta0) };
			float X1{ cosf(Theta1) };

			float Z0{ sinf(Theta0) };
			float Z1{ sinf(Theta1) };

			// Side 0
			Data.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY0, +Y0, +Z0 * XZLengthY0, 1), Color0);
			Data.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY0, +Y0, +Z1 * XZLengthY0, 1), Color0);
			Data.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY1, +Y1, +Z0 * XZLengthY1, 1), Color1);

			// Side 1
			Data.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY0, +Y0, +Z1 * XZLengthY0, 1), Color0);
			Data.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY1, +Y1, +Z1 * XZLengthY1, 1), Color1);
			Data.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY1, +Y1, +Z0 * XZLengthY1, 1), Color1);
		}
	}

	for (uint32_t i = 0; i < SegmentCount * SegmentCount / 2; ++i)
	{
		// Side 0
		Data.vTriangles.emplace_back(i * 6 + 0, i * 6 + 1, i * 6 + 2);

		// Side 1
		Data.vTriangles.emplace_back(i * 6 + 3, i * 6 + 4, i * 6 + 5);
	}

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh GenerateSphere(uint32_t SegmentCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	return GenerateSphere(SegmentCount, Color, Color);
}

static SMesh GenerateTorus(const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1), float InnerRadius = 0.2f, uint32_t SideCount = 16, uint32_t SegmentCount = 24)
{
	SMesh Data{};
	SegmentCount = max(SegmentCount, (uint32_t)3);
	SideCount = max(SideCount, (uint32_t)3);

	const float ThetaUnit{ XM_2PI / SegmentCount };
	const float PhiUnit{ XM_2PI / SideCount };
	for (uint32_t iSegment = 0; iSegment < SegmentCount; ++iSegment)
	{
		float Theta0{ ThetaUnit * iSegment };
		float Theta1{ ThetaUnit * (iSegment + 1) };

		float OuterX0{ cos(Theta0) };
		float OuterX1{ cos(Theta1) };

		float OuterZ0{ sin(Theta0) };
		float OuterZ1{ sin(Theta1) };

		for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
		{
			float Phi0{ PhiUnit * iSide };
			float Phi1{ PhiUnit * (iSide + 1) };

			float InnerXLow{ InnerRadius * cos(Phi0) };
			float InnerXHigh{ InnerRadius * cos(Phi1) };

			float InnerYLow{ InnerRadius * sin(Phi0) };
			float InnerYHigh{ InnerRadius * sin(Phi1) };

			Data.vVertices.emplace_back(XMVectorSet(OuterX0 + OuterX0 * InnerXHigh, +InnerYHigh, +OuterZ0 + OuterZ0 * InnerXHigh, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(OuterX1 + OuterX1 * InnerXHigh, +InnerYHigh, +OuterZ1 + OuterZ1 * InnerXHigh, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(OuterX0 + OuterX0 * InnerXLow, +InnerYLow, +OuterZ0 + OuterZ0 * InnerXLow, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(OuterX1 + OuterX1 * InnerXLow, +InnerYLow, +OuterZ1 + OuterZ1 * InnerXLow, 1), Color);
		}
	}
	
	for (uint32_t iSegment = 0; iSegment < SegmentCount; ++iSegment)
	{
		for (uint32_t iSide = 0; iSide < SideCount; ++iSide)
		{
			// Side 0
			Data.vTriangles.emplace_back(
				iSegment * SideCount * 4 + iSide * 4 + 0,
				iSegment * SideCount * 4 + iSide * 4 + 1,
				iSegment * SideCount * 4 + iSide * 4 + 2);

			// Side 1
			Data.vTriangles.emplace_back(
				iSegment * SideCount * 4 + iSide * 4 + 1, 
				iSegment * SideCount * 4 + iSide * 4 + 3, 
				iSegment * SideCount * 4 + iSide * 4 + 2);
		}
	}

	CalculateFaceNormals(Data);

	CalculateVertexNormalsFromFaceNormals(Data);

	return Data;
}

static SMesh MergeStaticMeshes(const SMesh& MeshA, const SMesh& MeshB)
{
	SMesh Result{ MeshA };

	size_t MeshAVertexCount{ MeshA.vVertices.size() };

	for (auto& Vertex : MeshB.vVertices)
	{
		Result.vVertices.emplace_back(Vertex);
	}

	for (auto& Triangle : MeshB.vTriangles)
	{
		STriangle NewTriangle
		{
			static_cast<uint32_t>(MeshAVertexCount + Triangle.I0), 
			static_cast<uint32_t>(MeshAVertexCount + Triangle.I1),
			static_cast<uint32_t>(MeshAVertexCount + Triangle.I2)
		};

		Result.vTriangles.emplace_back(NewTriangle);
	}

	return Result;
}

static vector<SVertex3DLine> Generate3DLineCircleYZ(const XMVECTOR& Color, uint32_t SegmentCount = 32)
{
	vector<SVertex3DLine> Vertices{};

	for (uint32_t iSegment = 0; iSegment < SegmentCount; ++iSegment)
	{
		float Theta0{ XM_2PI * (static_cast<float>(iSegment) / SegmentCount) };
		float Theta1{ XM_2PI * (static_cast<float>(iSegment + 1) / SegmentCount) };

		Vertices.emplace_back(SVertex3DLine(XMVectorSet(0, sin(Theta0), cos(Theta0), 1), Color));
		Vertices.emplace_back(SVertex3DLine(XMVectorSet(0, sin(Theta1), cos(Theta1), 1), Color));
	}

	return Vertices;
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

static vector<SVertex3DLine> Generate3DGrid(int GuidelineCount = 10, float Interval = 1.0f)
{
	GuidelineCount = max(GuidelineCount, 10);
	Interval = max(Interval, 0.1f);

	vector<SVertex3DLine> vResult{};
	static const XMVECTOR KColorWhite{ XMVectorSet(1, 1, 1, 1) };
	static const XMVECTOR KColorAxisX{ XMVectorSet(1, 0, 0, 1) };
	static const XMVECTOR KColorAxisY{ XMVectorSet(0, 1, 0, 1) };
	static const XMVECTOR KColorAxisZ{ XMVectorSet(0, 0, 1, 1) };
	float EndPosition{ -GuidelineCount * Interval };
	XMVECTOR Color{};
	for (int z = 0; z < GuidelineCount * 2 + 1; ++z)
	{
		Color = KColorWhite;
		if (z == GuidelineCount) Color = KColorAxisZ;
		vResult.emplace_back(SVertex3DLine(XMVectorSet(+EndPosition + Interval * z, 0, +EndPosition, 1), Color));
		vResult.emplace_back(SVertex3DLine(XMVectorSet(+EndPosition + Interval * z, 0, -EndPosition, 1), Color));
	}

	for (int x = 0; x < GuidelineCount * 2 + 1; ++x)
	{
		Color = KColorWhite;
		if (x == GuidelineCount) Color = KColorAxisX;
		vResult.emplace_back(SVertex3DLine(XMVectorSet(+EndPosition, 0, +EndPosition + Interval * x, 1), Color));
		vResult.emplace_back(SVertex3DLine(XMVectorSet(-EndPosition, 0, +EndPosition + Interval * x, 1), Color));
	}

	Color = KColorAxisY;
	vResult.emplace_back(SVertex3DLine(XMVectorSet(0, -EndPosition, 0, 1), Color));
	vResult.emplace_back(SVertex3DLine(XMVectorSet(0, +EndPosition, 0, 1), Color));

	return vResult;
}

static SObject2DData Generate2DRectangle(const XMFLOAT2& RectangleSize)
{
	SObject2DData Result{};

	float HalfWidth{ RectangleSize.x / 2 };
	float HalfHeight{ RectangleSize.y / 2 };

	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(-HalfWidth, +HalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(0.0f, 0.0f, 0, 0)));
	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(+HalfWidth, +HalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(1.0f, 0.0f, 0, 0)));
	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(-HalfWidth, -HalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(0.0f, 1.0f, 0, 0)));
	Result.vVertices.emplace_back(SVertex2D(XMVectorSet(+HalfWidth, -HalfHeight, 0, 1), XMVectorSet(1, 1, 1, 1), XMVectorSet(1.0f, 1.0f, 0, 0)));

	Result.vTriangles.emplace_back(STriangle(0, 1, 2));
	Result.vTriangles.emplace_back(STriangle(1, 3, 2));

	return Result;
}
