#pragma once

#include "Object3D.h"
#include <unordered_map>

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

static SMesh GenerateCone(float RadiusRatio = 0.0f, uint32_t SideCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	SideCount = max(SideCount, (uint32_t)3);

	constexpr float HalfHeight{ 0.5f };
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

		// Bottom
		Data.vVertices.emplace_back(XMVectorSet(+0.0f, -HalfHeight, +0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos0, -HalfHeight, +Sin0, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos1, -HalfHeight, +Sin1, 1), Color);

		// Top
		Data.vVertices.emplace_back(XMVectorSet(+0.0f, +HalfHeight, +0.0f, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos1 * RadiusRatio, +HalfHeight, +Sin1 * RadiusRatio, 1), Color);
		Data.vVertices.emplace_back(XMVectorSet(+Cos0 * RadiusRatio, +HalfHeight, +Sin0 * RadiusRatio, 1), Color);
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

static SMesh GenerateCylinder(uint32_t SideCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	return GenerateCone(1.0f, SideCount, Color);
}

static SMesh GenerateSphere(uint32_t SegmentCount = 16, const XMVECTOR& Color = XMVectorSet(1, 1, 1, 1))
{
	SMesh Data{};

	SegmentCount = max(SegmentCount, (uint32_t)4);
	if (SegmentCount % 2) ++SegmentCount;

	constexpr float Radius{ 1.0f };
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

		for (uint32_t azimuth_step = 0; azimuth_step < SegmentCount; ++azimuth_step) // theta
		{
			float Theta0{ ThetaUnit * azimuth_step };
			float Theta1{ ThetaUnit * (azimuth_step + 1) };

			float X0{ cosf(Theta0) };
			float X1{ cosf(Theta1) };

			float Z0{ sinf(Theta0) };
			float Z1{ sinf(Theta1) };

			// Side 0
			Data.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY0, +Y0, +Z0 * XZLengthY0, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY0, +Y0, +Z1 * XZLengthY0, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY1, +Y1, +Z0 * XZLengthY1, 1), Color);

			// Side 1
			Data.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY0, +Y0, +Z1 * XZLengthY0, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(+X1 * XZLengthY1, +Y1, +Z1 * XZLengthY1, 1), Color);
			Data.vVertices.emplace_back(XMVectorSet(+X0 * XZLengthY1, +Y1, +Z0 * XZLengthY1, 1), Color);
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