#include "renderer.hpp"

#include <algorithm>

namespace {

ComPtr<IDWriteTextFormat> makeFont(IDWriteFactory* factory, const wchar_t* family,
                                   DWRITE_FONT_WEIGHT weight, float size)
{
    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = factory->CreateTextFormat(
        family, nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        size, L"en-us", &format);
    if (FAILED(hr)) {
        checkHr(factory->CreateTextFormat(
                    L"Segoe UI", nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &format),
                "CreateTextFormat fallback");
    }
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    return format;
}

} // namespace

void Renderer::init(HWND hwnd)
{
    hwnd_ = hwnd;
    dpi_ = static_cast<float>(GetDpiForWindow(hwnd_));

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    widthPx_ = std::max(1L, rc.right - rc.left);
    heightPx_ = std::max(1L, rc.bottom - rc.top);

    checkHr(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                reinterpret_cast<IUnknown**>(dwrite_.ReleaseAndGetAddressOf())),
            "DWriteCreateFactory");
    createFonts();
    createDevice();
    createTarget();
}

void Renderer::createFonts()
{
    const float s = fontScale_;
    titleFont_ = makeFont(dwrite_.Get(), L"Segoe UI Variable Display", DWRITE_FONT_WEIGHT_BOLD, 13.0f * s);
    headingFont_ = makeFont(dwrite_.Get(), L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_SEMI_BOLD, 10.0f * s);
    bodyFont_ = makeFont(dwrite_.Get(), L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL, 11.0f * s);
    smallFont_ = makeFont(dwrite_.Get(), L"Segoe UI Variable Text", DWRITE_FONT_WEIGHT_NORMAL, 10.0f * s);
    monoFont_ = makeFont(dwrite_.Get(), L"Cascadia Mono", DWRITE_FONT_WEIGHT_SEMI_BOLD, 12.0f * s);
}

void Renderer::setFontScale(float scale)
{
    const float next = std::clamp(scale, 0.75f, 1.75f);
    if (std::abs(next - fontScale_) < 0.001f) {
        return;
    }
    fontScale_ = next;
    if (dwrite_) {
        createFonts();
    }
}

void Renderer::createDevice()
{
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL got{};
    checkHr(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels,
                              static_cast<UINT>(sizeof(levels) / sizeof(levels[0])),
                              D3D11_SDK_VERSION, &d3d_, &got, &d3dContext_),
            "D3D11CreateDevice");

    ComPtr<IDXGIDevice1> dxgiDevice;
    checkHr(d3d_.As(&dxgiDevice), "Query IDXGIDevice1");
    dxgiDevice->SetMaximumFrameLatency(1);

    ComPtr<IDXGIAdapter> adapter;
    checkHr(dxgiDevice->GetAdapter(&adapter), "GetAdapter");
    ComPtr<IDXGIFactory2> factory;
    checkHr(adapter->GetParent(IID_PPV_ARGS(&factory)), "Get DXGI factory");

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = widthPx_;
    desc.Height = heightPx_;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    checkHr(factory->CreateSwapChainForHwnd(d3d_.Get(), hwnd_, &desc, nullptr, nullptr, &swapChain_),
            "CreateSwapChainForHwnd");
    factory->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER);

    D2D1_FACTORY_OPTIONS opts{};
#ifdef _DEBUG
    opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    checkHr(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opts,
                              reinterpret_cast<void**>(d2dFactory_.ReleaseAndGetAddressOf())),
            "D2D1CreateFactory");

    ComPtr<IDXGIDevice> dxgiDeviceBase;
    checkHr(dxgiDevice.As(&dxgiDeviceBase), "IDXGIDevice");
    checkHr(d2dFactory_->CreateDevice(dxgiDeviceBase.Get(), &d2dDevice_), "CreateDevice D2D");
    checkHr(d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc_),
            "CreateDeviceContext");
    checkHr(dc_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &brush_), "CreateSolidColorBrush");
}

void Renderer::releaseTarget()
{
    if (dc_) {
        dc_->SetTarget(nullptr);
    }
    target_.Reset();
}

void Renderer::createTarget()
{
    releaseTarget();

    ComPtr<IDXGISurface> surface;
    checkHr(swapChain_->GetBuffer(0, IID_PPV_ARGS(&surface)), "GetBuffer");

    const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        dpi_, dpi_);

    checkHr(dc_->CreateBitmapFromDxgiSurface(surface.Get(), &props, &target_),
            "CreateBitmapFromDxgiSurface");
    dc_->SetTarget(target_.Get());
    dc_->SetDpi(dpi_, dpi_);
}

void Renderer::resize(UINT pixelWidth, UINT pixelHeight, float dpi)
{
    if (!swapChain_) {
        return;
    }

    dpi_ = dpi > 0.0f ? dpi : 96.0f;
    widthPx_ = std::max(1u, pixelWidth);
    heightPx_ = std::max(1u, pixelHeight);

    releaseTarget();
    const HRESULT hr = swapChain_->ResizeBuffers(0, widthPx_, heightPx_, DXGI_FORMAT_UNKNOWN, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        d3d_.Reset();
        d3dContext_.Reset();
        swapChain_.Reset();
        d2dDevice_.Reset();
        dc_.Reset();
        brush_.Reset();
        createDevice();
    } else {
        checkHr(hr, "ResizeBuffers");
    }
    createTarget();
}

void Renderer::beginDraw()
{
    dc_->BeginDraw();
    dc_->SetTransform(D2D1::Matrix3x2F::Identity());
}

void Renderer::endDraw()
{
    const HRESULT endHr = dc_->EndDraw();
    HRESULT presentHr = swapChain_->Present(1, 0);
    if (endHr == D2DERR_RECREATE_TARGET || presentHr == DXGI_ERROR_DEVICE_REMOVED ||
        presentHr == DXGI_ERROR_DEVICE_RESET) {
        RECT rc{};
        GetClientRect(hwnd_, &rc);
        resize(static_cast<UINT>(std::max(1L, rc.right)), static_cast<UINT>(std::max(1L, rc.bottom)), dpi_);
    } else {
        checkHr(endHr, "EndDraw");
        checkHr(presentHr, "Present");
    }
}

void Renderer::fill(const D2D1_RECT_F& r, D2D1_COLOR_F c)
{
    brush_->SetColor(c);
    dc_->FillRectangle(r, brush_.Get());
}

void Renderer::fillRounded(const D2D1_RECT_F& r, float radius, D2D1_COLOR_F c)
{
    brush_->SetColor(c);
    const D2D1_ROUNDED_RECT rr{r, radius, radius};
    dc_->FillRoundedRectangle(rr, brush_.Get());
}

void Renderer::stroke(const D2D1_RECT_F& r, D2D1_COLOR_F c, float width)
{
    brush_->SetColor(c);
    dc_->DrawRectangle(r, brush_.Get(), width);
}

void Renderer::strokeRounded(const D2D1_RECT_F& r, float radius, D2D1_COLOR_F c, float width)
{
    brush_->SetColor(c);
    const D2D1_ROUNDED_RECT rr{r, radius, radius};
    dc_->DrawRoundedRectangle(rr, brush_.Get(), width);
}

void Renderer::fillGradient(const D2D1_RECT_F& r, D2D1_COLOR_F top, D2D1_COLOR_F bottom)
{
    fillGradientRounded(r, 0.0f, top, bottom);
}

void Renderer::fillGradientRounded(const D2D1_RECT_F& r, float radius, D2D1_COLOR_F top,
                                   D2D1_COLOR_F bottom)
{
    D2D1_GRADIENT_STOP stops[2]{{0.0f, top}, {1.0f, bottom}};
    ComPtr<ID2D1GradientStopCollection> stopsCol;
    checkHr(dc_->CreateGradientStopCollection(stops, 2, &stopsCol), "CreateGradientStopCollection");

    ComPtr<ID2D1LinearGradientBrush> grad;
    const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props =
        D2D1::LinearGradientBrushProperties(D2D1::Point2F(r.left, r.top),
                                            D2D1::Point2F(r.left, r.bottom));
    checkHr(dc_->CreateLinearGradientBrush(props, stopsCol.Get(), &grad),
            "CreateLinearGradientBrush");

    if (radius <= 0.0f) {
        dc_->FillRectangle(r, grad.Get());
    } else {
        const D2D1_ROUNDED_RECT rr{r, radius, radius};
        dc_->FillRoundedRectangle(rr, grad.Get());
    }
}

void Renderer::line(D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_COLOR_F c, float width)
{
    brush_->SetColor(c);
    dc_->DrawLine(a, b, brush_.Get(), width);
}

void Renderer::fillEllipse(D2D1_POINT_2F c, float rx, float ry, D2D1_COLOR_F color)
{
    brush_->SetColor(color);
    dc_->FillEllipse(D2D1::Ellipse(c, rx, ry), brush_.Get());
}

void Renderer::pushClip(const D2D1_RECT_F& r)
{
    dc_->PushAxisAlignedClip(r, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
}

void Renderer::popClip()
{
    dc_->PopAxisAlignedClip();
}

void Renderer::fillPolylineArea(const D2D1_POINT_2F* pts, int count, float baselineY, D2D1_COLOR_F c)
{
    if (!pts || count < 2) {
        return;
    }
    ComPtr<ID2D1PathGeometry> geo;
    if (FAILED(d2dFactory_->CreatePathGeometry(&geo))) {
        return;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geo->Open(&sink))) {
        return;
    }
    sink->BeginFigure(D2D1::Point2F(pts[0].x, baselineY), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(pts[0]);
    sink->AddLines(pts + 1, static_cast<UINT32>(count - 1));
    sink->AddLine(D2D1::Point2F(pts[count - 1].x, baselineY));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    brush_->SetColor(c);
    dc_->FillGeometry(geo.Get(), brush_.Get());
}

float Renderer::measure(std::wstring_view s, IDWriteTextFormat* font)
{
    if (s.empty()) {
        return 0.0f;
    }
    ComPtr<IDWriteTextLayout> layout;
    if (FAILED(dwrite_->CreateTextLayout(s.data(), static_cast<UINT32>(s.size()), font, 4096.0f,
                                         64.0f, &layout))) {
        return 0.0f;
    }
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);
    return metrics.widthIncludingTrailingWhitespace;
}

void Renderer::text(const D2D1_RECT_F& r, std::wstring_view s, IDWriteTextFormat* font,
                    D2D1_COLOR_F c, DWRITE_TEXT_ALIGNMENT align,
                    DWRITE_PARAGRAPH_ALIGNMENT valign)
{
    font->SetTextAlignment(align);
    font->SetParagraphAlignment(valign);
    brush_->SetColor(c);
    dc_->DrawTextW(s.data(), static_cast<UINT32>(s.size()), font, r, brush_.Get(),
                   D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
}
