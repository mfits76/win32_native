#include "theme.hpp"
#include "renderer.hpp"
#include "ui.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cwctype>
#include <string>
#include <vector>

namespace {

constexpr wchar_t kClassName[] = L"HelloWorldDxWindow";

enum Cmd {
    CmdNone = 0,
    CmdReset,
    CmdExit,
    CmdToggleAnimate,
    CmdToggleTheme,
    CmdResetSplit,
    CmdAbout
};

int dipsToPx(float dips, UINT dpi)
{
    return static_cast<int>(std::lround(dips * static_cast<float>(dpi) / 96.0f));
}

float pxToDips(int px, UINT dpi)
{
    return static_cast<float>(px) * 96.0f / static_cast<float>(dpi);
}

bool iContains(const std::wstring& hay, const std::wstring& needle)
{
    if (needle.empty()) {
        return true;
    }
    auto fold = [](std::wstring s) {
        for (auto& c : s) {
            c = static_cast<wchar_t>(towlower(c));
        }
        return s;
    };
    return fold(hay).find(fold(needle)) != std::wstring::npos;
}

struct ProcRow {
    std::wstring name;
    float cpu{};
    float mem{};
    std::wstring disk;
    std::wstring status;
};

struct App {
    HWND hwnd{};
    Renderer gfx;
    Ui ui;
    InputState input;
    bool mouseDown{};
    bool maximized{};
    RECT restoreRect{};
    bool animate{true};
    bool darkMode{true};
    bool aboutOpen{};
    float slider{0.64f};
    float rangeLo{0.0f};
    float rangeHi{1.0f};
    float leftWidth{208.0f};
    int phosphor{2};
    int page{};
    int clicks{};
    int comboRegion{};
    int comboStatus{};
    int navSel{};
    int listSel{};
    int tableSel{};
    int sortCol{1};
    bool sortAsc{false};
    float navScroll{};
    float listScroll{};
    float tableScroll{};
    EditState nameEdit{L"Operator"};
    EditState noteEdit{};
    EditState filterEdit{};
    std::vector<std::wstring> regions{L"Local", L"Rack A", L"Rack B", L"Remote"};
    std::vector<std::wstring> statuses{L"All", L"Running", L"Stopped"};
    std::vector<std::wstring> nav{L"Dashboard", L"Controls", L"Table", L"Charts"};
    std::vector<std::wstring> events;
    std::vector<ProcRow> procs;
    std::vector<float> lineSeries;
    std::vector<float> barSeries;
    std::vector<std::wstring> barLabels{L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                                        L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
    std::vector<Candle> candles;
    std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};

    App() { resetDemo(); }

    D2D1_COLOR_F accent() const
    {
        switch (phosphor) {
        case 0: return theme::pal.phosphorGreen;
        case 1: return theme::pal.phosphorAmber;
        default: return theme::pal.phosphorCyan;
        }
    }

    float now() const
    {
        return std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
    }

    void log(const std::wstring& line)
    {
        events.insert(events.begin(), line);
        if (events.size() > 40) {
            events.pop_back();
        }
    }

    void resetDemo()
    {
        animate = true;
        slider = 0.64f;
        rangeLo = 0.0f;
        rangeHi = 1.0f;
        leftWidth = 208.0f;
        phosphor = 2;
        page = 0;
        navSel = 0;
        clicks = 0;
        comboRegion = 0;
        comboStatus = 0;
        listSel = 0;
        tableSel = 0;
        sortCol = 1;
        sortAsc = false;
        nameEdit = {L"Operator", 8};
        noteEdit = {};
        filterEdit = {};
        events = {L"Session started", L"Direct3D 11 device ready", L"DirectWrite fonts loaded"};
        procs = {
            {L"hello.exe", 12.4f, 48.2f, L"0.1 MB/s", L"Running"},
            {L"Task Manager.exe", 4.8f, 92.0f, L"0.4 MB/s", L"Running"},
            {L"explorer.exe", 1.2f, 140.5f, L"0.0 MB/s", L"Running"},
            {L"dwm.exe", 3.6f, 88.1f, L"0.0 MB/s", L"Running"},
            {L"svchost.exe", 0.8f, 22.4f, L"0.0 MB/s", L"Running"},
            {L"chrome.exe", 18.2f, 512.0f, L"1.6 MB/s", L"Running"},
            {L"code.exe", 9.1f, 380.4f, L"0.7 MB/s", L"Running"},
            {L"msedge.exe", 6.4f, 210.8f, L"0.3 MB/s", L"Running"},
            {L"steam.exe", 0.4f, 96.0f, L"0.2 MB/s", L"Stopped"},
            {L"discord.exe", 2.2f, 175.3f, L"0.5 MB/s", L"Running"},
            {L"nvcontainer.exe", 1.1f, 64.0f, L"0.0 MB/s", L"Running"},
            {L"SearchHost.exe", 0.3f, 40.2f, L"0.1 MB/s", L"Running"},
            {L"RuntimeBroker.exe", 0.2f, 18.7f, L"0.0 MB/s", L"Running"},
            {L"powershell.exe", 5.5f, 72.9f, L"0.2 MB/s", L"Running"},
            {L"cmd.exe", 0.1f, 4.2f, L"0.0 MB/s", L"Stopped"},
            {L"notepad.exe", 0.0f, 8.8f, L"0.0 MB/s", L"Stopped"},
            {L"spotify.exe", 1.7f, 155.0f, L"0.8 MB/s", L"Running"},
            {L"obs64.exe", 11.0f, 260.1f, L"2.4 MB/s", L"Running"},
        };
        lineSeries.resize(48);
        for (int i = 0; i < 48; ++i) {
            const float t = static_cast<float>(i) / 47.0f;
            lineSeries[i] = 40.0f + 18.0f * std::sin(t * 6.2f) + 8.0f * std::sin(t * 14.0f);
        }
        barSeries = {42, 55, 61, 48, 70, 66, 80, 74, 69, 58, 50, 63};
        candles.resize(24);
        float price = 100.0f;
        for (int i = 0; i < 24; ++i) {
            const float drift = ((i * 17) % 13 - 6) * 0.8f;
            const float open = price;
            const float close = price + drift;
            const float high = std::max(open, close) + 1.4f + (i % 3);
            const float low = std::min(open, close) - 1.1f - (i % 2);
            candles[i] = {open, high, low, close};
            price = close;
        }
        log(L"Demo data reset");
    }

    float cpuValue() const
    {
        if (!animate) {
            return slider;
        }
        const float t = now();
        return clamp01(0.38f + 0.22f * std::sin(t * 1.7f) + 0.08f * std::sin(t * 4.3f));
    }

    void applyTheme(bool dark)
    {
        darkMode = dark;
        theme::setDark(dark);
        if (hwnd) {
            BOOL immersive = dark ? TRUE : FALSE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &immersive, sizeof(immersive));
        }
        log(dark ? L"Dark mode" : L"Light mode");
    }

    void applyMenu(int cmd)
    {
        switch (cmd) {
        case CmdReset: resetDemo(); break;
        case CmdExit: DestroyWindow(hwnd); break;
        case CmdToggleAnimate:
            animate = !animate;
            log(animate ? L"Animation on" : L"Animation off");
            break;
        case CmdToggleTheme:
            applyTheme(!darkMode);
            break;
        case CmdResetSplit: leftWidth = 208.0f; break;
        case CmdAbout: aboutOpen = true; break;
        default: break;
        }
    }

    std::vector<Menu> menus() const
    {
        return {
            {L"File", {{L"Reset demo", CmdReset}, {L"", 0, true}, {L"Exit", CmdExit}}},
            {L"View",
             {{animate ? L"Pause meters" : L"Animate meters", CmdToggleAnimate},
              {darkMode ? L"Light mode" : L"Dark mode", CmdToggleTheme},
              {L"Reset splitter", CmdResetSplit}}},
            {L"Help", {{L"About", CmdAbout}}},
        };
    }

    void toggleMax()
    {
        if (!maximized) {
            GetWindowRect(hwnd, &restoreRect);
            MONITORINFO mi{sizeof(mi)};
            GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
            SetWindowPos(hwnd, nullptr, mi.rcWork.left, mi.rcWork.top,
                         mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top,
                         SWP_NOZORDER);
            maximized = true;
        } else {
            SetWindowPos(hwnd, nullptr, restoreRect.left, restoreRect.top,
                         restoreRect.right - restoreRect.left, restoreRect.bottom - restoreRect.top,
                         SWP_NOZORDER);
            maximized = false;
        }
    }

    void onResize()
    {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        const UINT w = static_cast<UINT>(std::max(1L, rc.right - rc.left));
        const UINT h = static_cast<UINT>(std::max(1L, rc.bottom - rc.top));
        if (w == 0 || h == 0) {
            return;
        }
        gfx.resize(w, h, static_cast<float>(GetDpiForWindow(hwnd)));
    }

    void setMouse(float x, float y, bool down)
    {
        input.pressed = down && !mouseDown;
        input.released = !down && mouseDown;
        input.down = down;
        input.x = x;
        input.y = y;
        mouseDown = down;
    }

    void drawChrome(float w, float h)
    {
        gfx.fillGradient(rect(0, 0, w, h), theme::pal.windowTop, theme::pal.windowBottom);
        gfx.strokeRounded(D2D1::RectF(0.5f, 0.5f, w - 0.5f, h - 0.5f), theme::windowRadius,
                          theme::pal.windowStroke, 1.0f);

        gfx.fillGradient(rect(0, 0, w, theme::headerHeight), theme::pal.headerTop, theme::pal.headerBottom);
        gfx.line(D2D1::Point2F(0, theme::headerHeight - 0.5f),
                 D2D1::Point2F(w, theme::headerHeight - 0.5f), theme::pal.rule);
        gfx.fillRounded(rect(12, 14, 16, 16), 4.0f, accent());
        gfx.text(rect(34, 4, 420, 22), L"HELLO WORLD", gfx.titleFont(), theme::pal.title);
        gfx.text(rect(34, 22, 420, 18), L"Direct3D 11  ·  Direct2D  ·  DirectWrite",
                 gfx.headingFont(), theme::pal.secondary);

        const float cap = theme::captionBtn * 3.0f;
        const D2D1_RECT_F decR = rect(w - cap - theme::themeToggleW - theme::fontBtnW * 2.0f - 8.0f, 11,
                                      theme::fontBtnW, 22);
        const D2D1_RECT_F incR = rect(w - cap - theme::themeToggleW - theme::fontBtnW - 4.0f, 11,
                                      theme::fontBtnW, 22);
        if (ui.button(decR, L"A-")) {
            const float before = gfx.fontScale();
            gfx.setFontScale(before - 0.1f);
            if (gfx.fontScale() != before) {
                log(L"Font smaller");
            }
        }
        if (ui.button(incR, L"A+")) {
            const float before = gfx.fontScale();
            gfx.setFontScale(before + 0.1f);
            if (gfx.fontScale() != before) {
                log(L"Font larger");
            }
        }

        const D2D1_RECT_F themeR = rect(w - cap - theme::themeToggleW, 11, theme::themeToggleW - 8, 22);
        if (ui.toggle(themeR, darkMode)) {
            applyTheme(darkMode);
        }

        const D2D1_RECT_F minR = rect(w - theme::captionBtn * 3.0f, 0, theme::captionBtn, theme::headerHeight);
        const D2D1_RECT_F maxR = rect(w - theme::captionBtn * 2.0f, 0, theme::captionBtn, theme::headerHeight);
        const D2D1_RECT_F closeR = rect(w - theme::captionBtn, 0, theme::captionBtn, theme::headerHeight);
        if (ui.captionGlyph(minR, 0)) {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
        if (ui.captionGlyph(maxR, 1)) {
            toggleMax();
        }
        if (ui.captionGlyph(closeR, 2)) {
            DestroyWindow(hwnd);
        }
    }

    void drawDashboard(const D2D1_RECT_F& r)
    {
        const float innerW = r.right - r.left;
        const D2D1_COLOR_F acc = accent();
        ui.tile(rect(r.left, r.top, innerW, 92));
        gfx.text(rect(r.left + 12, r.top + 8, 80, 16), L"CPU", gfx.headingFont(), theme::pal.secondary);
        const float cpu = cpuValue();
        wchar_t buf[32];
        swprintf_s(buf, L"%d%%", static_cast<int>(cpu * 100 + 0.5f));
        gfx.text(rect(r.right - 72, r.top + 8, 60, 16), buf, gfx.monoFont(), acc,
                 DWRITE_TEXT_ALIGNMENT_TRAILING);
        ui.meter(rect(r.left + 12, r.top + 36, innerW - 24, 36), cpu, acc);

        ui.tile(rect(r.left, r.top + 100, innerW, 92));
        gfx.text(rect(r.left + 12, r.top + 108, 80, 16), L"GPU", gfx.headingFont(), theme::pal.secondary);
        swprintf_s(buf, L"%d%%", static_cast<int>(slider * 100 + 0.5f));
        gfx.text(rect(r.right - 72, r.top + 108, 60, 16), buf, gfx.monoFont(), acc,
                 DWRITE_TEXT_ALIGNMENT_TRAILING);
        ui.meter(rect(r.left + 12, r.top + 136, innerW - 24, 36), slider, acc);

        gfx.text(rect(r.left, r.top + 204, 200, 16), L"LOAD HISTORY", gfx.headingFont(), theme::pal.secondary);
        ui.lineChart(rect(r.left, r.top + 224, innerW, r.bottom - r.top - 224), lineSeries, acc);
    }

    void drawControls(const D2D1_RECT_F& r)
    {
        const D2D1_COLOR_F acc = accent();
        const float w = r.right - r.left;
        gfx.text(rect(r.left, r.top, 200, 16), L"BUTTONS", gfx.headingFont(), theme::pal.secondary);
        if (ui.button(rect(r.left, r.top + 22, 88, 28), L"Hello")) {
            ++clicks;
            log(L"Hello clicked");
        }
        if (ui.button(rect(r.left + 96, r.top + 22, 88, 28), L"Apply")) {
            log(L"Apply " + nameEdit.text);
        }
        if (ui.button(rect(r.left + 192, r.top + 22, 88, 28), L"Refresh")) {
            log(L"Refresh");
        }
        if (ui.button(rect(r.left + 288, r.top + 22, 88, 28), L"Reset")) {
            resetDemo();
        }
        wchar_t clickText[48];
        swprintf_s(clickText, L"%d click%s", clicks, clicks == 1 ? L"" : L"s");
        gfx.text(rect(r.left + 388, r.top + 22, 140, 28), clickText, gfx.bodyFont(), theme::pal.primary);
        ui.checkbox(rect(r.left + 520, r.top + 22, 160, 28), L"Animate meters", animate);

        gfx.text(rect(r.left, r.top + 64, 80, 16), L"EDITS", gfx.headingFont(), theme::pal.secondary);
        gfx.text(rect(r.left, r.top + 84, 70, 26), L"Name", gfx.bodyFont(), theme::pal.secondary);
        ui.edit(rect(r.left + 70, r.top + 82, 220, 28), nameEdit, L"Name");
        gfx.text(rect(r.left + 310, r.top + 84, 70, 26), L"Notes", gfx.bodyFont(), theme::pal.secondary);
        ui.edit(rect(r.left + 370, r.top + 82, w - 370, 28), noteEdit, L"Type a note");

        gfx.text(rect(r.left, r.top + 124, 80, 16), L"COMBOS", gfx.headingFont(), theme::pal.secondary);
        gfx.text(rect(r.left, r.top + 144, 70, 26), L"Region", gfx.bodyFont(), theme::pal.secondary);
        ui.combo(rect(r.left + 70, r.top + 142, 180, 28), regions, comboRegion);
        gfx.text(rect(r.left + 270, r.top + 144, 70, 26), L"Status", gfx.bodyFont(), theme::pal.secondary);
        ui.combo(rect(r.left + 330, r.top + 142, 160, 28), statuses, comboStatus);

        gfx.text(rect(r.left, r.top + 186, 120, 16), L"SLIDERS", gfx.headingFont(), theme::pal.secondary);
        gfx.text(rect(r.left, r.top + 208, 70, 22), L"Load", gfx.bodyFont(), theme::pal.secondary);
        ui.slider(rect(r.left + 70, r.top + 208, w - 140, 22), slider, acc);
        wchar_t pct[16];
        swprintf_s(pct, L"%d%%", static_cast<int>(slider * 100 + 0.5f));
        gfx.text(rect(r.right - 60, r.top + 208, 60, 22), pct, gfx.monoFont(), acc,
                 DWRITE_TEXT_ALIGNMENT_TRAILING);

        gfx.text(rect(r.left, r.top + 238, 120, 22), L"CPU range", gfx.bodyFont(), theme::pal.secondary);
        ui.rangeSlider(rect(r.left + 90, r.top + 238, w - 200, 22), rangeLo, rangeHi, acc);
        wchar_t range[32];
        swprintf_s(range, L"%d-%d%%", static_cast<int>(rangeLo * 100 + 0.5f),
                   static_cast<int>(rangeHi * 100 + 0.5f));
        gfx.text(rect(r.right - 90, r.top + 238, 90, 22), range, gfx.monoFont(), acc,
                 DWRITE_TEXT_ALIGNMENT_TRAILING);

        gfx.text(rect(r.left, r.top + 274, 80, 16), L"PHOSPHOR", gfx.headingFont(), theme::pal.secondary);
        const float chipW = (w - 16.0f) / 3.0f;
        if (ui.chip(rect(r.left, r.top + 294, chipW, 24), L"Green", phosphor == 0, theme::pal.phosphorGreen)) {
            phosphor = 0;
        }
        if (ui.chip(rect(r.left + chipW + 8, r.top + 294, chipW, 24), L"Amber", phosphor == 1,
                    theme::pal.phosphorAmber)) {
            phosphor = 1;
        }
        if (ui.chip(rect(r.left + (chipW + 8) * 2, r.top + 294, chipW, 24), L"Cyan", phosphor == 2,
                    theme::pal.phosphorCyan)) {
            phosphor = 2;
        }

        gfx.text(rect(r.left, r.top + 332, 80, 16), L"LIST", gfx.headingFont(), theme::pal.secondary);
        ui.listBox(rect(r.left, r.top + 350, w, r.bottom - r.top - 350), events, listSel, listScroll);
    }

    void drawTable(const D2D1_RECT_F& r)
    {
        const D2D1_COLOR_F acc = accent();
        const float w = r.right - r.left;
        gfx.text(rect(r.left, r.top, 70, 26), L"Filter", gfx.bodyFont(), theme::pal.secondary);
        ui.edit(rect(r.left + 50, r.top, 240, 28), filterEdit, L"Search processes");
        gfx.text(rect(r.left + 304, r.top, 70, 26), L"Status", gfx.bodyFont(), theme::pal.secondary);
        ui.combo(rect(r.left + 354, r.top, 140, 28), statuses, comboStatus);
        gfx.text(rect(r.left + 508, r.top, 90, 26), L"CPU band", gfx.bodyFont(), theme::pal.secondary);
        ui.rangeSlider(rect(r.left + 590, r.top + 4, w - 590, 20), rangeLo, rangeHi, acc);

        std::vector<ProcRow> filtered;
        for (const auto& p : procs) {
            const float cpu01 = p.cpu / 100.0f;
            if (cpu01 < rangeLo || cpu01 > rangeHi) {
                continue;
            }
            if (comboStatus == 1 && p.status != L"Running") {
                continue;
            }
            if (comboStatus == 2 && p.status != L"Stopped") {
                continue;
            }
            if (!iContains(p.name, filterEdit.text) && !iContains(p.status, filterEdit.text) &&
                !iContains(p.disk, filterEdit.text)) {
                continue;
            }
            filtered.push_back(p);
        }
        std::sort(filtered.begin(), filtered.end(), [&](const ProcRow& a, const ProcRow& b) {
            auto less = [&](const ProcRow& lhs, const ProcRow& rhs) {
                switch (sortCol) {
                case 1: return lhs.cpu < rhs.cpu;
                case 2: return lhs.mem < rhs.mem;
                case 3: return lhs.disk < rhs.disk;
                case 4: return lhs.status < rhs.status;
                default: return lhs.name < rhs.name;
                }
            };
            return sortAsc ? less(a, b) : less(b, a);
        });

        std::vector<std::vector<std::wstring>> rows;
        rows.reserve(filtered.size());
        for (const auto& p : filtered) {
            wchar_t cpu[32], mem[32];
            swprintf_s(cpu, L"%.1f", p.cpu);
            swprintf_s(mem, L"%.1f MB", p.mem);
            rows.push_back({p.name, cpu, mem, p.disk, p.status});
        }
        const std::vector<TableColumn> cols{{L"Process", 1.4f, false},
                                            {L"CPU %", 0.6f, true},
                                            {L"Memory", 0.8f, true},
                                            {L"Disk", 0.8f, false},
                                            {L"Status", 0.7f, false}};
        wchar_t count[48];
        swprintf_s(count, L"%d of %d rows", static_cast<int>(rows.size()),
                   static_cast<int>(procs.size()));
        gfx.text(rect(r.left, r.top + 34, 220, 16), count, gfx.headingFont(), theme::pal.secondary);
        ui.table(rect(r.left, r.top + 52, w, r.bottom - r.top - 52), cols, rows, sortCol, sortAsc,
                 tableSel, tableScroll);
    }

    void drawCharts(const D2D1_RECT_F& r)
    {
        const D2D1_COLOR_F acc = accent();
        const float w = r.right - r.left;
        const float h = r.bottom - r.top;
        const float topH = (h - 8) * 0.48f;
        const float botH = h - topH - 8;
        gfx.text(rect(r.left, r.top, 120, 16), L"LINE", gfx.headingFont(), theme::pal.secondary);
        ui.lineChart(rect(r.left, r.top + 18, w, topH - 18), lineSeries, acc);
        const float half = (w - 8) * 0.5f;
        gfx.text(rect(r.left, r.top + topH + 8, 120, 16), L"CANDLESTICK", gfx.headingFont(),
                 theme::pal.secondary);
        ui.candleChart(rect(r.left, r.top + topH + 26, half, botH - 26), candles);
        gfx.text(rect(r.left + half + 8, r.top + topH + 8, 120, 16), L"BAR", gfx.headingFont(),
                 theme::pal.secondary);
        ui.barChart(rect(r.left + half + 8, r.top + topH + 26, half, botH - 26), barSeries, barLabels,
                    acc);
    }

    void frame()
    {
        gfx.beginDraw();
        const float w = gfx.widthDips();
        const float h = gfx.heightDips();
        ui.begin(gfx, input, now());
        drawChrome(w, h);

        if (aboutOpen) {
            if (ui.modal(L"Hello World",
                         L"C++20 x64 sample. Custom DirectX controls, no GDI widgets or Qt.")) {
                aboutOpen = false;
            }
            ui.end();
            gfx.endDraw();
            input.pressed = false;
            input.released = false;
            input.wheel = 0;
            input.chars.clear();
            input.keys.clear();
            return;
        }

        const D2D1_RECT_F menuR = rect(0, theme::headerHeight, w, theme::menuHeight);
        applyMenu(ui.menuBar(menuR, menus()));

        const float footer = 24.0f;
        const float contentTop = theme::headerHeight + theme::menuHeight;
        const float contentH = h - contentTop - footer;
        const float originX = 0.0f;
        const float splitX = leftWidth;
        ui.listBox(rect(8, contentTop + 8, leftWidth - 12, contentH - 16), nav, navSel, navScroll);
        page = navSel;
        ui.splitter(rect(splitX - 3, contentTop, theme::splitterHit, contentH), originX, leftWidth,
                    150.0f, std::min(360.0f, w - 420.0f));

        const float rightX = splitX + 8;
        const float rightW = w - rightX - 8;
        const D2D1_RECT_F tabR = rect(rightX, contentTop + 4, rightW, theme::tabHeight);
        page = ui.tabs(tabR, nav, page);
        navSel = page;

        const D2D1_RECT_F body = rect(rightX, contentTop + theme::tabHeight + 8, rightW,
                                      contentH - theme::tabHeight - 12);
        if (page == 0) {
            drawDashboard(body);
        } else if (page == 1) {
            drawControls(body);
        } else if (page == 2) {
            drawTable(body);
        } else {
            drawCharts(body);
        }

        gfx.text(rect(10, h - footer, w - 20, footer - 4),
                 L"C++20  ·  x64  ·  custom DirectX controls, no GDI widgets", gfx.headingFont(),
                 theme::pal.secondary);

        ui.end();
        gfx.endDraw();
        input.pressed = false;
        input.released = false;
        input.wheel = 0;
        input.chars.clear();
        input.keys.clear();
    }

    LRESULT hitTest(LPARAM lParam) const
    {
        POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd, &pt);
        const UINT dpi = GetDpiForWindow(hwnd);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        const float w = pxToDips(rc.right, dpi);
        const float h = pxToDips(rc.bottom, dpi);
        const float x = pxToDips(pt.x, dpi);
        const float y = pxToDips(pt.y, dpi);
        const float e = theme::resizeEdge;

        if (!maximized) {
            const bool left = x < e;
            const bool right = x >= w - e;
            const bool top = y < e;
            const bool bottom = y >= h - e;
            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
        }

        if (y < theme::headerHeight) {
            if (x >= w - theme::captionBtn * 3.0f - theme::themeToggleW - theme::fontBtnW * 2.0f - 12.0f) {
                return HTCLIENT;
            }
            return HTCAPTION;
        }
        return HTCLIENT;
    }

    LRESULT handle(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_NCCALCSIZE:
            if (wParam) {
                return 0;
            }
            break;
        case WM_NCACTIVATE:
            return TRUE;
        case WM_NCHITTEST:
            return hitTest(lParam);
        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                LPCWSTR cur = IDC_ARROW;
                if (ui.cursor() == CursorHint::IBeam) {
                    cur = IDC_IBEAM;
                } else if (ui.cursor() == CursorHint::SizeWE) {
                    cur = IDC_SIZEWE;
                }
                SetCursor(LoadCursorW(nullptr, cur));
                return TRUE;
            }
            break;
        case WM_MOUSEMOVE: {
            const UINT dpi = GetDpiForWindow(hwnd);
            setMouse(pxToDips(GET_X_LPARAM(lParam), dpi), pxToDips(GET_Y_LPARAM(lParam), dpi),
                     mouseDown);
            TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE:
            setMouse(input.x, input.y, false);
            return 0;
        case WM_LBUTTONDOWN: {
            const UINT dpi = GetDpiForWindow(hwnd);
            setMouse(pxToDips(GET_X_LPARAM(lParam), dpi), pxToDips(GET_Y_LPARAM(lParam), dpi), true);
            SetCapture(hwnd);
            return 0;
        }
        case WM_LBUTTONUP: {
            const UINT dpi = GetDpiForWindow(hwnd);
            setMouse(pxToDips(GET_X_LPARAM(lParam), dpi), pxToDips(GET_Y_LPARAM(lParam), dpi), false);
            ReleaseCapture();
            return 0;
        }
        case WM_MOUSEWHEEL:
            input.wheel = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            return 0;
        case WM_CHAR:
            if (wParam >= 32) {
                input.chars.push_back(static_cast<wchar_t>(wParam));
            }
            return 0;
        case WM_KEYDOWN:
            input.keys.push_back(static_cast<int>(wParam));
            input.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            input.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            return 0;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                onResize();
            }
            return 0;
        case WM_DPICHANGED: {
            const auto* suggested = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left, suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            onResize();
            return 0;
        }
        case WM_GETMINMAXINFO: {
            const UINT dpi = hwnd ? GetDpiForWindow(hwnd) : GetDpiForSystem();
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = dipsToPx(880, dpi);
            mmi->ptMinTrackSize.y = dipsToPx(600, dpi);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }
        return DefWindowProcW(window, msg, wParam, lParam);
    }
};

App* gApp{};

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        gApp = static_cast<App*>(cs->lpCreateParams);
        gApp->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(gApp));
    }
    if (auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
        return app->handle(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    checkHr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "CoInitializeEx");

    try {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = wndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = kClassName;
        if (!RegisterClassExW(&wc)) {
            throw std::runtime_error("RegisterClassExW");
        }

        App app;
        const UINT dpi = GetDpiForSystem();
        const int width = dipsToPx(theme::windowWidth, dpi);
        const int height = dipsToPx(theme::windowHeight, dpi);
        const int x = std::max(0, (GetSystemMetrics(SM_CXSCREEN) - width) / 2);
        const int y = std::max(0, (GetSystemMetrics(SM_CYSCREEN) - height) / 2);

        HWND hwnd = CreateWindowExW(
            WS_EX_APPWINDOW, kClassName, L"Hello World",
            WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_VISIBLE, x, y, width,
            height, nullptr, nullptr, instance, &app);
        if (!hwnd) {
            throw std::runtime_error("CreateWindowExW");
        }

        const DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_ROUND;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
        const BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

        app.gfx.init(hwnd);
        ShowWindow(hwnd, show);
        UpdateWindow(hwnd);

        MSG msg{};
        while (true) {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    CoUninitialize();
                    return static_cast<int>(msg.wParam);
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (!IsWindow(hwnd)) {
                break;
            }
            if (IsIconic(hwnd)) {
                WaitMessage();
                continue;
            }
            app.frame();
        }
    } catch (const std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "Hello World", MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    CoUninitialize();
    return 0;
}
