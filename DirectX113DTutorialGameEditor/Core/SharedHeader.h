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
#include "../DirectXTK/DirectXTK.h"
#include "../DirectXTex/DirectXTex.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DirectXTK.lib")
#pragma comment(lib, "DirectXTex.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

enum class EShaderType
{
	VertexShader,
	HullShader,
	DomainShader,
	GeometryShader,
	PixelShader
};

struct STriangle
{
	STriangle() {}
	STriangle(uint32_t _0, uint32_t _1, uint32_t _2) : I0{ _0 }, I1{ _1 }, I2{ _2 } {}

	uint32_t I0{};
	uint32_t I1{};
	uint32_t I2{};
};

static const XMMATRIX KMatrixIdentity{ XMMatrixIdentity() };

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