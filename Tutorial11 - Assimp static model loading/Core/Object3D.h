#pragma once

#include "SharedHeader.h"
#include "Texture.h"

class CGameWindow;

struct SVertex3D
{
	SVertex3D() {}
	SVertex3D(const XMVECTOR& _Position, const XMVECTOR& _Color, const XMVECTOR& _TexCoord = XMVectorSet(0, 0, 0, 0)) :
		Position{ _Position }, Color{ _Color }, TexCoord{ _TexCoord } {}

	XMVECTOR Position{};
	XMVECTOR Color{};
	XMVECTOR TexCoord{};
	XMVECTOR Normal{};
};

struct SMesh
{
	vector<SVertex3D>	vVertices{};
	vector<STriangle>	vTriangles{};
	size_t				MaterialID{};
};

struct SMeshBuffers
{
	ComPtr<ID3D11Buffer>	VertexBuffer{};
	UINT					VertexBufferStride{ sizeof(SVertex3D) };
	UINT					VertexBufferOffset{};
	ComPtr<ID3D11Buffer>	IndexBuffer{};
};

struct SMaterial
{
	SMaterial() {}
	SMaterial(const XMFLOAT3& UniversalColor) : MaterialAmbient{ UniversalColor }, MaterialDiffuse{ UniversalColor }, MaterialSpecular{ UniversalColor } {}

	XMFLOAT3			MaterialAmbient{};
	float				SpecularExponent{ 1 };
	XMFLOAT3			MaterialDiffuse{};
	float				SpecularIntensity{ 0 };
	XMFLOAT3			MaterialSpecular{};

	bool				bHasTexture{ false };
	bool				bHasEmbeddedTexture{ false };
	string				TextureFileName{};
	size_t				TextureID{};

	vector<uint8_t>		vEmbeddedTextureRawData{};
};

struct SModel
{
	vector<SMesh>		vMeshes{};
	vector<SMaterial>	vMaterials{};
};

class CObject3D
{
	friend class CGameWindow;

public:
	CObject3D(ID3D11Device* PtrDevice, ID3D11DeviceContext* PtrDeviceContext, CGameWindow* PtrGameWindow) :
		m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }, m_PtrGameWindow{ PtrGameWindow }
	{
		assert(m_PtrDevice);
		assert(m_PtrDeviceContext);
		assert(m_PtrGameWindow);
	}
	~CObject3D() {}

	void Create(const SMesh& Mesh, const SMaterial& Material = SMaterial());
	void Create(const vector<SMesh>& vMeshes, const vector<SMaterial>& vMaterials);
	void Create(const SModel& Model);

private:
	void CreateMeshBuffers(size_t MeshIndex);

	void Draw() const;

	void DrawNormals() const;

private:
	ID3D11Device*					m_PtrDevice{};
	ID3D11DeviceContext*			m_PtrDeviceContext{};
	CGameWindow*					m_PtrGameWindow{};

private:
	vector<SMesh>					m_vMeshes{};
	vector<SMaterial>				m_vMaterials{};
	vector<SMeshBuffers>			m_vMeshBuffers{};
	vector<unique_ptr<CTexture>>	m_vTextures{};
};