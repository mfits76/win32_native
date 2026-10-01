#pragma once

#include "renderer.hpp"

#include <string>
#include <vector>

enum class CursorHint { Arrow, IBeam, SizeWE };

struct InputState {
    float x = 0.0f;
    float y = 0.0f;
    bool down = false;
    bool pressed = false;
    bool released = false;
    bool ctrl = false;
    bool shift = false;
    float wheel = 0.0f;
    std::wstring chars;
    std::vector<int> keys;
};

struct EditState {
    std::wstring text;
    int caret = 0;
};

struct MenuItem {
    std::wstring label;
    int command = 0;
    bool separator = false;
};

struct Menu {
    std::wstring title;
    std::vector<MenuItem> items;
};

struct Candle {
    float open = 0;
    float high = 0;
    float low = 0;
    float close = 0;
};

struct TableColumn {
    std::wstring header;
    float weight = 1.0f;
    bool numeric = false;
};

class Ui {
public:
    void begin(Renderer& renderer, const InputState& input, float time);
    void end();

    bool button(const D2D1_RECT_F& r, std::wstring_view label);
    bool checkbox(const D2D1_RECT_F& r, std::wstring_view label, bool& value);
    bool toggle(const D2D1_RECT_F& r, bool& on);
    bool slider(const D2D1_RECT_F& r, float& value, D2D1_COLOR_F accent);
    bool rangeSlider(const D2D1_RECT_F& r, float& lo, float& hi, D2D1_COLOR_F accent);
    bool chip(const D2D1_RECT_F& r, std::wstring_view label, bool selected, D2D1_COLOR_F accent);
    bool captionGlyph(const D2D1_RECT_F& r, int kind); // 0 min, 1 max/restore, 2 close
    bool edit(const D2D1_RECT_F& r, EditState& state, std::wstring_view placeholder);
    int tabs(const D2D1_RECT_F& r, const std::vector<std::wstring>& labels, int selected);
    bool listBox(const D2D1_RECT_F& r, const std::vector<std::wstring>& items, int& selected,
                 float& scroll);
    bool combo(const D2D1_RECT_F& r, const std::vector<std::wstring>& items, int& selected);
    int menuBar(const D2D1_RECT_F& r, const std::vector<Menu>& menus);
    bool splitter(const D2D1_RECT_F& r, float originX, float& width, float minW, float maxW);
    int table(const D2D1_RECT_F& r, const std::vector<TableColumn>& cols,
              const std::vector<std::vector<std::wstring>>& rows, int& sortCol, bool& sortAsc,
              int& selected, float& scroll);
    void meter(const D2D1_RECT_F& r, float value, D2D1_COLOR_F accent);
    void tile(const D2D1_RECT_F& r);
    void lineChart(const D2D1_RECT_F& r, const std::vector<float>& values, D2D1_COLOR_F accent);
    void barChart(const D2D1_RECT_F& r, const std::vector<float>& values,
                  const std::vector<std::wstring>& labels, D2D1_COLOR_F accent);
    void candleChart(const D2D1_RECT_F& r, const std::vector<Candle>& candles);
    bool modal(std::wstring_view title, std::wstring_view body);

    CursorHint cursor() const { return cursor_; }
    bool hovered(const D2D1_RECT_F& r) const;
    Renderer& gfx() { return *r_; }

private:
    bool hit(const D2D1_RECT_F& r) const;
    bool wantPress() const { return in_->pressed && !eatClick_; }
    std::uint32_t next();
    bool clicked(std::uint32_t id, bool over);
    void drawThumb(float x, float cy, bool active, D2D1_COLOR_F accent);
    void applyEditKeys(EditState& state);
    void copyText(const std::wstring& text) const;
    std::wstring pasteText() const;
    int caretFromX(const D2D1_RECT_F& r, const std::wstring& text, float x);

    Renderer* r_{};
    const InputState* in_{};
    float time_{};
    std::uint32_t nextId_{1};
    std::uint32_t hot_{};
    std::uint32_t active_{};
    std::uint32_t focus_{};
    CursorHint cursor_{CursorHint::Arrow};
    bool eatClick_{};

    bool comboOpen_{};
    std::uint32_t comboId_{};
    D2D1_RECT_F comboBox_{};
    D2D1_RECT_F comboPopup_{};
    std::vector<std::wstring> comboItems_;
    int* comboSelected_{};
    bool comboChanged_{};

    int menuOpen_{-1};
    D2D1_RECT_F menuBarR_{};
    D2D1_RECT_F menuPopup_{};
    std::vector<MenuItem> menuItems_;
    int menuCommand_{};
};
