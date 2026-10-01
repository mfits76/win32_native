#pragma once

#include "common.hpp"

class Renderer {
public:
    void init(HWND hwnd);
    void resize(UINT pixelWidth, UINT pixelHeight, float dpi);
    void beginDraw();
    void endDraw();

    ID2D1DeviceContext* dc() const { return dc_.Get(); }
    IDWriteFactory* dwrite() const { return dwrite_.Get(); }
    ID2D1Factory1* factory() const { return d2dFactory_.Get(); }
    IDWriteTextFormat* titleFont() const { return titleFont_.Get(); }
    IDWriteTextFormat* headingFont() const { return headingFont_.Get(); }
    IDWriteTextFormat* bodyFont() const { return bodyFont_.Get(); }
    IDWriteTextFormat* smallFont() const { return smallFont_.Get(); }
    IDWriteTextFormat* monoFont() const { return monoFont_.Get(); }

    float fontScale() const { return fontScale_; }
    void setFontScale(float scale);

    float dpi() const { return dpi_; }
    float widthDips() const { return widthPx_ * 96.0f / dpi_; }
    float heightDips() const { return heightPx_ * 96.0f / dpi_; }

    void fill(const D2D1_RECT_F& r, D2D1_COLOR_F c);
    void fillRounded(const D2D1_RECT_F& r, float radius, D2D1_COLOR_F c);
    void stroke(const D2D1_RECT_F& r, D2D1_COLOR_F c, float width = 1.0f);
    void strokeRounded(const D2D1_RECT_F& r, float radius, D2D1_COLOR_F c, float width = 1.0f);
    void fillGradient(const D2D1_RECT_F& r, D2D1_COLOR_F top, D2D1_COLOR_F bottom);
    void fillGradientRounded(const D2D1_RECT_F& r, float radius, D2D1_COLOR_F top, D2D1_COLOR_F bottom);
    void line(D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_COLOR_F c, float width = 1.0f);
    void fillEllipse(D2D1_POINT_2F c, float rx, float ry, D2D1_COLOR_F color);
    void pushClip(const D2D1_RECT_F& r);
    void popClip();
    void fillPolylineArea(const D2D1_POINT_2F* pts, int count, float baselineY, D2D1_COLOR_F c);
    float measure(std::wstring_view s, IDWriteTextFormat* font);
    void text(const D2D1_RECT_F& r, std::wstring_view s, IDWriteTextFormat* font,
              D2D1_COLOR_F c, DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
              DWRITE_PARAGRAPH_ALIGNMENT valign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

private:
    void createDevice();
    void createTarget();
    void releaseTarget();
    void createFonts();

    HWND hwnd_{};
    float dpi_{96.0f};
    float fontScale_{1.0f};
    UINT widthPx_{1};
    UINT heightPx_{1};

    ComPtr<ID3D11Device> d3d_;
    ComPtr<ID3D11DeviceContext> d3dContext_;
    ComPtr<IDXGISwapChain1> swapChain_;
    ComPtr<ID2D1Factory1> d2dFactory_;
    ComPtr<ID2D1Device> d2dDevice_;
    ComPtr<ID2D1DeviceContext> dc_;
    ComPtr<ID2D1Bitmap1> target_;
    ComPtr<ID2D1SolidColorBrush> brush_;
    ComPtr<IDWriteFactory> dwrite_;
    ComPtr<IDWriteTextFormat> titleFont_;
    ComPtr<IDWriteTextFormat> headingFont_;
    ComPtr<IDWriteTextFormat> bodyFont_;
    ComPtr<IDWriteTextFormat> smallFont_;
    ComPtr<IDWriteTextFormat> monoFont_;
};
