#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <memory>
#include <d3d11.h>
#include <wrl.h>
#include "../DirectXTK/DirectXTK.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DirectXTK.lib")

using namespace Microsoft::WRL;
using namespace DirectX;

using std::string;
using std::wstring;
using std::vector;
using std::unique_ptr;
using std::make_unique;

struct STriangle
{
	STriangle() {}
	STriangle(uint32_t _0, uint32_t _1, uint32_t _2) : I0{ _0 }, I1{ _1 }, I2{ _2 } {}

	uint32_t I0{};
	uint32_t I1{};
	uint32_t I2{};
};
