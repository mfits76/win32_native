#pragma once

#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <dwmapi.h>
#include <wrl/client.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

using Microsoft::WRL::ComPtr;

inline void checkHr(HRESULT hr, const char* what)
{
    if (FAILED(hr)) {
        throw std::runtime_error(what);
    }
}

inline D2D1_COLOR_F rgb(std::uint32_t hex, float a = 1.0f)
{
    return D2D1::ColorF(
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f,
        a);
}

inline D2D1_RECT_F rect(float x, float y, float w, float h)
{
    return D2D1::RectF(x, y, x + w, y + h);
}

inline bool contains(const D2D1_RECT_F& r, float x, float y)
{
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

inline float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}
