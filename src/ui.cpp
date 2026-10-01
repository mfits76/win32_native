#include "ui.hpp"
#include "theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

D2D1_RECT_F inset(const D2D1_RECT_F& r, float x, float y)
{
    return D2D1::RectF(r.left + x, r.top + y, r.right - x, r.bottom - y);
}

} // namespace

void Ui::begin(Renderer& renderer, const InputState& input, float time)
{
    r_ = &renderer;
    in_ = &input;
    time_ = time;
    nextId_ = 1;
    hot_ = 0;
    cursor_ = CursorHint::Arrow;
    if (comboOpen_ && in_->pressed) {
        const bool onBox = contains(comboBox_, in_->x, in_->y);
        const bool onPopup = contains(comboPopup_, in_->x, in_->y);
        if (!onBox && !onPopup) {
            comboOpen_ = false;
        }
    }
    if (menuOpen_ >= 0 && in_->pressed) {
        const bool onBar = contains(menuBarR_, in_->x, in_->y);
        const bool onPopup = contains(menuPopup_, in_->x, in_->y);
        if (!onBar && !onPopup) {
            menuOpen_ = -1;
        }
    }
}

void Ui::end()
{
    if (comboOpen_) {
        r_->fillRounded(comboPopup_, 6.0f, theme::pal.popup);
        r_->strokeRounded(comboPopup_, 6.0f, theme::pal.windowStroke);
        const float rowH = theme::rowHeight;
        for (int i = 0; i < static_cast<int>(comboItems_.size()); ++i) {
            const D2D1_RECT_F row = rect(comboPopup_.left + 4, comboPopup_.top + 4 + i * rowH,
                                         comboPopup_.right - comboPopup_.left - 8, rowH);
            const bool over = hit(row);
            if (over || i == *comboSelected_) {
                r_->fillRounded(row, 4.0f, over ? theme::pal.buttonHover : theme::pal.selection);
            }
            r_->text(inset(row, 8, 0), comboItems_[i], r_->bodyFont(), theme::pal.primary);
            if (over && in_->released) {
                *comboSelected_ = i;
                comboChanged_ = true;
                comboOpen_ = false;
                active_ = 0;
            }
        }
    }

    if (menuOpen_ >= 0) {
        r_->fillRounded(menuPopup_, 6.0f, theme::pal.popup);
        r_->strokeRounded(menuPopup_, 6.0f, theme::pal.windowStroke);
        float y = menuPopup_.top + 4;
        for (const auto& item : menuItems_) {
            if (item.separator) {
                r_->line(D2D1::Point2F(menuPopup_.left + 8, y + 4),
                         D2D1::Point2F(menuPopup_.right - 8, y + 4), theme::pal.rule);
                y += 9;
                continue;
            }
            const D2D1_RECT_F row = rect(menuPopup_.left + 4, y, menuPopup_.right - menuPopup_.left - 8,
                                         24);
            const bool over = hit(row);
            if (over) {
                r_->fillRounded(row, 4.0f, theme::pal.buttonHover);
            }
            r_->text(inset(row, 10, 0), item.label, r_->bodyFont(), theme::pal.primary);
            if (over && in_->released) {
                menuCommand_ = item.command;
                menuOpen_ = -1;
                active_ = 0;
            }
            y += 24;
        }
    }

    if (in_->released) {
        pressId_ = 0;
    }
}

bool Ui::hit(const D2D1_RECT_F& r) const
{
    return contains(r, in_->x, in_->y);
}

bool Ui::hovered(const D2D1_RECT_F& r) const
{
    return hit(r);
}

std::uint32_t Ui::next()
{
    return nextId_++;
}

bool Ui::clicked(std::uint32_t id, bool over)
{
    if (over) {
        hot_ = id;
    }
    if (over && wantPress()) {
        pressId_ = id;
    }
    return in_->released && pressId_ == id && over;
}

void Ui::tile(const D2D1_RECT_F& r)
{
    r_->fillRounded(r, theme::tileRadius, theme::pal.tile);
    r_->strokeRounded(r, theme::tileRadius, theme::pal.tileStroke);
}

bool Ui::button(const D2D1_RECT_F& r, std::wstring_view label)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    D2D1_COLOR_F face = theme::pal.buttonFace;
    if (over) {
        face = theme::pal.buttonHover;
    }
    if (pressId_ == id && in_->down) {
        face = theme::pal.buttonPress;
    }
    r_->fillRounded(r, theme::controlRadius, face);
    r_->strokeRounded(r, theme::controlRadius, over ? theme::pal.control : theme::pal.tileStroke);
    r_->text(r, label, r_->bodyFont(), theme::pal.primary, DWRITE_TEXT_ALIGNMENT_CENTER);
    return clicked(id, over);
}

bool Ui::checkbox(const D2D1_RECT_F& r, std::wstring_view label, bool& value)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    const float box = 16.0f;
    const float cy = (r.top + r.bottom) * 0.5f;
    const D2D1_RECT_F boxR = D2D1::RectF(r.left, cy - box * 0.5f, r.left + box, cy + box * 0.5f);
    r_->fillRounded(boxR, 3.0f, value ? theme::pal.checkFill : theme::pal.buttonFace);
    r_->strokeRounded(boxR, 3.0f, value ? theme::pal.phosphorCyan : theme::pal.tileStroke);
    if (value) {
        r_->line(D2D1::Point2F(boxR.left + 3.5f, cy), D2D1::Point2F(boxR.left + 6.5f, cy + 3.5f),
                 theme::pal.phosphorCyan, 1.6f);
        r_->line(D2D1::Point2F(boxR.left + 6.5f, cy + 3.5f),
                 D2D1::Point2F(boxR.right - 3.5f, cy - 3.5f), theme::pal.phosphorCyan, 1.6f);
    }
    r_->text(D2D1::RectF(boxR.right + 8.0f, r.top, r.right, r.bottom), label, r_->bodyFont(),
             theme::pal.primary);
    if (clicked(id, over)) {
        value = !value;
        return true;
    }
    return false;
}

bool Ui::toggle(const D2D1_RECT_F& r, bool& on)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    const float cy = (r.top + r.bottom) * 0.5f;
    const float trackW = 38.0f;
    const float trackH = 18.0f;
    const D2D1_RECT_F track = D2D1::RectF(r.right - trackW, cy - trackH * 0.5f, r.right, cy + trackH * 0.5f);
    r_->text(D2D1::RectF(r.left, r.top, track.left - 6.0f, r.bottom), on ? L"Dark" : L"Light",
             r_->headingFont(), theme::pal.control, DWRITE_TEXT_ALIGNMENT_TRAILING);
    r_->fillRounded(track, 9.0f, on ? theme::pal.phosphorCyan : theme::pal.unlit);
    r_->strokeRounded(track, 9.0f, over ? theme::pal.control : theme::pal.tileStroke);
    const float thumbR = 6.5f;
    const float thumbX = on ? track.right - 9.5f : track.left + 9.5f;
    r_->fillEllipse(D2D1::Point2F(thumbX, cy), thumbR, thumbR, theme::pal.title);
    if (clicked(id, over)) {
        on = !on;
        return true;
    }
    return false;
}

void Ui::drawThumb(float x, float cy, bool active, D2D1_COLOR_F accent)
{
    const float thumbR = active ? 8.0f : 7.0f;
    const D2D1_RECT_F thumbBox = D2D1::RectF(x - thumbR, cy - thumbR, x + thumbR, cy + thumbR);
    r_->fillRounded(thumbBox, thumbR, theme::pal.title);
    r_->strokeRounded(thumbBox, thumbR, D2D1::ColorF(accent.r, accent.g, accent.b, 0.6f), 1.0f);
}

bool Ui::slider(const D2D1_RECT_F& r, float& value, D2D1_COLOR_F accent)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    if (over && wantPress()) {
        active_ = id;
    }
    const float cy = (r.top + r.bottom) * 0.5f;
    const D2D1_RECT_F track = D2D1::RectF(r.left, cy - 2.5f, r.right, cy + 2.5f);
    r_->fillRounded(track, 2.5f, theme::pal.unlit);
    if (active_ == id && in_->down) {
        value = clamp01((in_->x - r.left) / std::max(1.0f, r.right - r.left));
    }
    r_->fillRounded(D2D1::RectF(track.left, track.top, lerp(track.left, track.right, value), track.bottom),
                    2.5f, D2D1::ColorF(accent.r, accent.g, accent.b, 0.85f));
    drawThumb(lerp(r.left, r.right, value), cy, active_ == id, accent);
    const bool changed = (active_ == id && in_->down);
    if (in_->released && active_ == id) {
        active_ = 0;
    }
    return changed;
}

bool Ui::rangeSlider(const D2D1_RECT_F& r, float& lo, float& hi, D2D1_COLOR_F accent)
{
    const std::uint32_t idLo = next();
    const std::uint32_t idHi = next();
    const float cy = (r.top + r.bottom) * 0.5f;
    const float span = std::max(1.0f, r.right - r.left);
    const float xLo = lerp(r.left, r.right, lo);
    const float xHi = lerp(r.left, r.right, hi);
    const bool over = hit(r);
    if (over && wantPress()) {
        active_ = (std::abs(in_->x - xLo) <= std::abs(in_->x - xHi)) ? idLo : idHi;
    }
    const D2D1_RECT_F track = D2D1::RectF(r.left, cy - 2.5f, r.right, cy + 2.5f);
    r_->fillRounded(track, 2.5f, theme::pal.unlit);
    r_->fillRounded(D2D1::RectF(xLo, track.top, xHi, track.bottom), 2.5f,
                    D2D1::ColorF(accent.r, accent.g, accent.b, 0.85f));
    if (active_ == idLo && in_->down) {
        lo = clamp01((in_->x - r.left) / span);
        if (lo > hi) {
            lo = hi;
        }
    }
    if (active_ == idHi && in_->down) {
        hi = clamp01((in_->x - r.left) / span);
        if (hi < lo) {
            hi = lo;
        }
    }
    drawThumb(lerp(r.left, r.right, lo), cy, active_ == idLo, accent);
    drawThumb(lerp(r.left, r.right, hi), cy, active_ == idHi, accent);
    const bool changed = (active_ == idLo || active_ == idHi) && in_->down;
    if (in_->released && (active_ == idLo || active_ == idHi)) {
        active_ = 0;
    }
    return changed;
}

bool Ui::chip(const D2D1_RECT_F& r, std::wstring_view label, bool selected, D2D1_COLOR_F accent)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    const D2D1_COLOR_F face = selected ? theme::pal.chipSelected : (over ? theme::pal.buttonHover : theme::pal.buttonFace);
    r_->fillRounded(r, 11.0f, face);
    r_->strokeRounded(r, 11.0f, selected ? D2D1::ColorF(accent.r, accent.g, accent.b, 0.85f)
                                         : theme::pal.tileStroke);
    r_->text(r, label, r_->headingFont(), selected ? accent : theme::pal.secondary,
             DWRITE_TEXT_ALIGNMENT_CENTER);
    if (over && wantPress()) {
        return true;
    }
    return clicked(id, over);
}

bool Ui::captionGlyph(const D2D1_RECT_F& r, int kind)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    if (over) {
        r_->fill(r, kind == 2 ? theme::pal.closeHover : theme::pal.captionHover);
    }
    const float cx = (r.left + r.right) * 0.5f;
    const float cy = (r.top + r.bottom) * 0.5f;
    const D2D1_COLOR_F ink = over && kind == 2 ? rgb(0xFFFFFF) : theme::pal.control;
    if (kind == 2) {
        r_->line(D2D1::Point2F(cx - 4.5f, cy - 4.5f), D2D1::Point2F(cx + 4.5f, cy + 4.5f), ink, 1.2f);
        r_->line(D2D1::Point2F(cx + 4.5f, cy - 4.5f), D2D1::Point2F(cx - 4.5f, cy + 4.5f), ink, 1.2f);
    } else if (kind == 0) {
        r_->line(D2D1::Point2F(cx - 5.0f, cy), D2D1::Point2F(cx + 5.0f, cy), ink, 1.2f);
    } else {
        r_->stroke(D2D1::RectF(cx - 5.0f, cy - 5.0f, cx + 5.0f, cy + 5.0f), ink, 1.1f);
    }
    return clicked(id, over);
}

void Ui::copyText(const std::wstring& text) const
{
    if (!OpenClipboard(nullptr)) {
        return;
    }
    EmptyClipboard();
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (mem) {
        if (auto* dst = static_cast<wchar_t*>(GlobalLock(mem))) {
            memcpy(dst, text.c_str(), bytes);
            GlobalUnlock(mem);
            SetClipboardData(CF_UNICODETEXT, mem);
        }
    }
    CloseClipboard();
}

std::wstring Ui::pasteText() const
{
    std::wstring out;
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(nullptr)) {
        return out;
    }
    if (HANDLE mem = GetClipboardData(CF_UNICODETEXT)) {
        if (const auto* src = static_cast<const wchar_t*>(GlobalLock(mem))) {
            out = src;
            GlobalUnlock(mem);
        }
    }
    CloseClipboard();
    return out;
}

int Ui::caretFromX(const D2D1_RECT_F& r, const std::wstring& text, float x)
{
    const float local = x - (r.left + 8.0f);
    if (local <= 0) {
        return 0;
    }
    int best = static_cast<int>(text.size());
    for (int i = 0; i <= static_cast<int>(text.size()); ++i) {
        const float w = r_->measure(std::wstring_view(text.c_str(), i), r_->bodyFont());
        if (w >= local) {
            best = i;
            break;
        }
    }
    return best;
}

void Ui::applyEditKeys(EditState& state)
{
    auto clampCaret = [&] {
        state.caret = std::clamp(state.caret, 0, static_cast<int>(state.text.size()));
    };
    for (wchar_t ch : in_->chars) {
        if (ch < 32) {
            continue;
        }
        state.text.insert(state.text.begin() + state.caret, ch);
        ++state.caret;
    }
    for (int vk : in_->keys) {
        if (vk == VK_LEFT) {
            state.caret = std::max(0, state.caret - 1);
        } else if (vk == VK_RIGHT) {
            state.caret = std::min(static_cast<int>(state.text.size()), state.caret + 1);
        } else if (vk == VK_HOME) {
            state.caret = 0;
        } else if (vk == VK_END) {
            state.caret = static_cast<int>(state.text.size());
        } else if (vk == VK_BACK && state.caret > 0) {
            state.text.erase(state.text.begin() + (state.caret - 1));
            --state.caret;
        } else if (vk == VK_DELETE && state.caret < static_cast<int>(state.text.size())) {
            state.text.erase(state.text.begin() + state.caret);
        } else if (in_->ctrl && (vk == 'A')) {
            // caret to end is enough for this demo
            state.caret = static_cast<int>(state.text.size());
        } else if (in_->ctrl && (vk == 'C')) {
            copyText(state.text);
        } else if (in_->ctrl && (vk == 'V')) {
            auto pasted = pasteText();
            pasted.erase(std::remove(pasted.begin(), pasted.end(), L'\r'), pasted.end());
            pasted.erase(std::remove(pasted.begin(), pasted.end(), L'\n'), pasted.end());
            state.text.insert(state.caret, pasted);
            state.caret += static_cast<int>(pasted.size());
        }
    }
    clampCaret();
}

bool Ui::edit(const D2D1_RECT_F& r, EditState& state, std::wstring_view placeholder)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    if (over) {
        cursor_ = CursorHint::IBeam;
    }
    if (over && wantPress()) {
        focus_ = id;
        state.caret = caretFromX(r, state.text, in_->x);
    } else if (wantPress() && !over && focus_ == id) {
        focus_ = 0;
    }
    const bool focused = focus_ == id;
    r_->fillRounded(r, theme::controlRadius, theme::pal.field);
    r_->strokeRounded(r, theme::controlRadius, focused ? theme::pal.phosphorCyan : theme::pal.tileStroke,
                      focused ? 1.4f : 1.0f);

    r_->pushClip(inset(r, 1, 1));
    if (state.text.empty() && !focused) {
        r_->text(inset(r, 8, 0), placeholder, r_->bodyFont(), theme::pal.secondary);
    } else {
        r_->text(inset(r, 8, 0), state.text, r_->bodyFont(), theme::pal.primary);
    }
    if (focused) {
        applyEditKeys(state);
        if (std::fmod(time_, 1.0f) < 0.55f) {
            const float caretX =
                r.left + 8.0f + r_->measure(std::wstring_view(state.text.c_str(), state.caret),
                                            r_->bodyFont());
            r_->line(D2D1::Point2F(caretX, r.top + 6), D2D1::Point2F(caretX, r.bottom - 6),
                     theme::pal.title, 1.0f);
        }
    }
    r_->popClip();
    return focused && (!in_->chars.empty() || !in_->keys.empty());
}

int Ui::tabs(const D2D1_RECT_F& r, const std::vector<std::wstring>& labels, int selected)
{
    if (labels.empty()) {
        return 0;
    }
    const float w = (r.right - r.left) / static_cast<float>(labels.size());
    int nextSel = selected;
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        const D2D1_RECT_F tab = rect(r.left + i * w, r.top, w, r.bottom - r.top);
        const std::uint32_t id = next();
        const bool over = hit(tab);
        const bool on = i == selected;
        if (on) {
            r_->fill(tab, theme::pal.overlayStrong);
        } else if (over) {
            r_->fill(tab, theme::pal.overlay);
        }
        r_->text(tab, labels[i], r_->headingFont(), on ? theme::pal.title : theme::pal.secondary,
                 DWRITE_TEXT_ALIGNMENT_CENTER);
        if (on) {
            r_->fill(D2D1::RectF(tab.left + 12, tab.bottom - 2, tab.right - 12, tab.bottom),
                     theme::pal.phosphorCyan);
        }
        if ((over && wantPress()) || clicked(id, over)) {
            nextSel = i;
        }
    }
    r_->line(D2D1::Point2F(r.left, r.bottom - 0.5f), D2D1::Point2F(r.right, r.bottom - 0.5f),
             theme::pal.rule);
    return nextSel;
}

bool Ui::listBox(const D2D1_RECT_F& r, const std::vector<std::wstring>& items, int& selected,
                 float& scroll)
{
    const bool over = hit(r);
    r_->fillRounded(r, 6.0f, theme::pal.field);
    r_->strokeRounded(r, 6.0f, theme::pal.tileStroke);
    const float rowH = theme::rowHeight;
    const float view = r.bottom - r.top - 8;
    const float content = static_cast<float>(items.size()) * rowH;
    if (over && in_->wheel != 0.0f) {
        scroll -= in_->wheel * 40.0f;
    }
    const float maxScroll = std::max(0.0f, content - view);
    scroll = std::clamp(scroll, 0.0f, maxScroll);

    r_->pushClip(inset(r, 1, 1));
    bool changed = false;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const float y = r.top + 4 + i * rowH - scroll;
        if (y + rowH < r.top || y > r.bottom) {
            continue;
        }
        const D2D1_RECT_F row = D2D1::RectF(r.left + 4, y, r.right - 4, y + rowH);
        const bool rowOver = hit(row);
        if (i == selected) {
            r_->fillRounded(row, 4.0f, theme::pal.selection);
        } else if (rowOver) {
            r_->fillRounded(row, 4.0f, theme::pal.buttonHover);
        }
        r_->text(inset(row, 8, 0), items[i], r_->bodyFont(), theme::pal.primary);
        if (rowOver && wantPress()) {
            selected = i;
            changed = true;
        }
    }
    r_->popClip();
    return changed;
}

bool Ui::combo(const D2D1_RECT_F& r, const std::vector<std::wstring>& items, int& selected)
{
    const std::uint32_t id = next();
    const bool changed = comboChanged_ && comboId_ == id;
    if (changed) {
        comboChanged_ = false;
    }
    const bool over = hit(r);
    const bool open = comboOpen_ && comboId_ == id;
    r_->fillRounded(r, theme::controlRadius, theme::pal.buttonFace);
    r_->strokeRounded(r, theme::controlRadius, open || over ? theme::pal.control : theme::pal.tileStroke);
    const std::wstring& label =
        (selected >= 0 && selected < static_cast<int>(items.size())) ? items[selected] : L"";
    r_->text(inset(r, 10, 0), label, r_->bodyFont(), theme::pal.primary);
    const float cx = r.right - 14;
    const float cy = (r.top + r.bottom) * 0.5f;
    r_->line(D2D1::Point2F(cx - 4, cy - 2), D2D1::Point2F(cx, cy + 2), theme::pal.secondary, 1.2f);
    r_->line(D2D1::Point2F(cx, cy + 2), D2D1::Point2F(cx + 4, cy - 2), theme::pal.secondary, 1.2f);

    if (clicked(id, over)) {
        if (open) {
            comboOpen_ = false;
        } else {
            comboOpen_ = true;
            comboId_ = id;
            comboBox_ = r;
            comboItems_ = items;
            comboSelected_ = &selected;
            const float h = 8.0f + theme::rowHeight * static_cast<float>(items.size());
            comboPopup_ = rect(r.left, r.bottom + 4, r.right - r.left, h);
        }
    } else if (open) {
        comboBox_ = r;
        comboItems_ = items;
        comboSelected_ = &selected;
        const float h = 8.0f + theme::rowHeight * static_cast<float>(items.size());
        comboPopup_ = rect(r.left, r.bottom + 4, r.right - r.left, h);
    }
    return changed;
}

int Ui::menuBar(const D2D1_RECT_F& r, const std::vector<Menu>& menus)
{
    const int cmd = menuCommand_;
    menuCommand_ = 0;
    menuBarR_ = r;
    r_->fill(r, theme::pal.headerBottom);
    r_->line(D2D1::Point2F(r.left, r.bottom - 0.5f), D2D1::Point2F(r.right, r.bottom - 0.5f),
             theme::pal.rule);

    float x = r.left + 8;
    for (int i = 0; i < static_cast<int>(menus.size()); ++i) {
        const float w = r_->measure(menus[i].title, r_->headingFont()) + 24.0f;
        const D2D1_RECT_F item = rect(x, r.top, w, r.bottom - r.top);
        const std::uint32_t id = next();
        const bool over = hit(item);
        const bool open = menuOpen_ == i;
        if (over || open) {
            r_->fillRounded(inset(item, 2, 3), 4.0f, theme::pal.buttonHover);
        }
        r_->text(item, menus[i].title, r_->headingFont(), theme::pal.control,
                 DWRITE_TEXT_ALIGNMENT_CENTER);
        if (clicked(id, over)) {
            if (open) {
                menuOpen_ = -1;
            } else {
                menuOpen_ = i;
                menuItems_ = menus[i].items;
                float maxW = 140.0f;
                float h = 8.0f;
                for (const auto& mi : menuItems_) {
                    if (mi.separator) {
                        h += 9;
                    } else {
                        maxW = std::max(maxW, r_->measure(mi.label, r_->bodyFont()) + 36.0f);
                        h += 24;
                    }
                }
                menuPopup_ = rect(item.left, r.bottom + 2, maxW, h);
            }
        } else if (open) {
            menuItems_ = menus[i].items;
        }
        x += w;
    }
    return cmd;
}

bool Ui::splitter(const D2D1_RECT_F& r, float originX, float& width, float minW, float maxW)
{
    const std::uint32_t id = next();
    const bool over = hit(r);
    if (over) {
        cursor_ = CursorHint::SizeWE;
    }
    if (over && wantPress()) {
        active_ = id;
    }
    const bool dragging = active_ == id && in_->down;
    if (dragging) {
        cursor_ = CursorHint::SizeWE;
        width = std::clamp(in_->x - originX, minW, maxW);
    }
    r_->fill(D2D1::RectF((r.left + r.right) * 0.5f - 0.5f, r.top + 10, (r.left + r.right) * 0.5f + 0.5f,
                         r.bottom - 10),
             over || dragging ? theme::pal.phosphorCyan : theme::pal.rule);
    if (in_->released && active_ == id) {
        active_ = 0;
    }
    return dragging;
}

int Ui::table(const D2D1_RECT_F& r, const std::vector<TableColumn>& cols,
              const std::vector<std::vector<std::wstring>>& rows, int& sortCol, bool& sortAsc,
              int& selected, float& scroll)
{
    tile(r);
    const float headerH = 28.0f;
    float totalW = 0;
    for (const auto& c : cols) {
        totalW += c.weight;
    }
    totalW = std::max(totalW, 0.001f);
    const float inner = r.right - r.left - 16;
    std::vector<float> xs(cols.size());
    float x = r.left + 8;
    for (size_t i = 0; i < cols.size(); ++i) {
        xs[i] = x;
        x += inner * (cols[i].weight / totalW);
    }

    r_->fill(D2D1::RectF(r.left + 1, r.top + 1, r.right - 1, r.top + headerH), theme::pal.overlayStrong);
    for (int i = 0; i < static_cast<int>(cols.size()); ++i) {
        const float x1 = xs[i];
        const float x2 = (i + 1 < static_cast<int>(xs.size())) ? xs[i + 1] : r.right - 8;
        const D2D1_RECT_F cell = D2D1::RectF(x1, r.top, x2, r.top + headerH);
        std::wstring label = cols[i].header;
        if (sortCol == i) {
            label += sortAsc ? L"  ^" : L"  v";
        }
        r_->text(inset(cell, 6, 0), label, r_->headingFont(), theme::pal.secondary);
        if (clicked(next(), hit(cell))) {
            if (sortCol == i) {
                sortAsc = !sortAsc;
            } else {
                sortCol = i;
                sortAsc = true;
            }
        }
    }
    r_->line(D2D1::Point2F(r.left, r.top + headerH), D2D1::Point2F(r.right, r.top + headerH),
             theme::pal.rule);

    const float rowH = theme::rowHeight;
    const D2D1_RECT_F body = D2D1::RectF(r.left + 1, r.top + headerH, r.right - 1, r.bottom - 1);
    if (hit(body) && in_->wheel != 0.0f) {
        scroll -= in_->wheel * 48.0f;
    }
    const float maxScroll = std::max(0.0f, static_cast<float>(rows.size()) * rowH - (body.bottom - body.top));
    scroll = std::clamp(scroll, 0.0f, maxScroll);

    r_->pushClip(body);
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const float y = body.top + i * rowH - scroll;
        if (y + rowH < body.top || y > body.bottom) {
            continue;
        }
        const D2D1_RECT_F rowR = D2D1::RectF(body.left, y, body.right, y + rowH);
        const bool over = hit(rowR);
        if (i == selected) {
            r_->fill(rowR, theme::pal.selection);
        } else if (over) {
            r_->fill(rowR, theme::pal.overlayStrong);
        } else if (i % 2) {
            r_->fill(rowR, theme::pal.overlay);
        }
        const auto& row = rows[i];
        for (int c = 0; c < static_cast<int>(cols.size()) && c < static_cast<int>(row.size()); ++c) {
            const float x1 = xs[c];
            const float x2 = (c + 1 < static_cast<int>(xs.size())) ? xs[c + 1] : r.right - 8;
            r_->text(D2D1::RectF(x1 + 6, y, x2 - 4, y + rowH), row[c], r_->smallFont(), theme::pal.primary,
                     cols[c].numeric ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);
        }
        if (over && wantPress()) {
            selected = i;
        }
    }
    r_->popClip();
    return selected;
}

void Ui::meter(const D2D1_RECT_F& r, float value, D2D1_COLOR_F accent)
{
    constexpr int kSeg = 28;
    const float gap = 2.0f;
    const float segW = std::max(1.0f, (r.right - r.left - gap * (kSeg - 1)) / kSeg);
    const float litUntil = clamp01(value) * kSeg;
    for (int i = 0; i < kSeg; ++i) {
        const float x = r.left + i * (segW + gap);
        const D2D1_RECT_F seg = D2D1::RectF(x, r.top, x + segW, r.bottom);
        r_->fillRounded(seg, 1.2f, theme::pal.unlit);
        const float cover = clamp01(litUntil - static_cast<float>(i));
        if (cover > 0.0f) {
            const float h = (r.bottom - r.top) * cover;
            const float t = static_cast<float>(i) / (kSeg - 1);
            r_->fillRounded(D2D1::RectF(x, r.bottom - h, x + segW, r.bottom), 1.2f,
                            D2D1::ColorF(lerp(accent.r * 0.55f, accent.r, t),
                                         lerp(accent.g * 0.55f, accent.g, t),
                                         lerp(accent.b * 0.55f, accent.b, t), 0.45f + 0.55f * cover));
        }
    }
}

void Ui::lineChart(const D2D1_RECT_F& r, const std::vector<float>& values, D2D1_COLOR_F accent)
{
    tile(r);
    if (values.size() < 2) {
        return;
    }
    float mn = values[0], mx = values[0];
    for (float v : values) {
        mn = std::min(mn, v);
        mx = std::max(mx, v);
    }
    if (mx - mn < 0.001f) {
        mx = mn + 1;
    }
    const D2D1_RECT_F plot = inset(r, 36, 18);
    for (int g = 0; g <= 4; ++g) {
        const float y = lerp(plot.bottom, plot.top, g / 4.0f);
        r_->line(D2D1::Point2F(plot.left, y), D2D1::Point2F(plot.right, y), theme::pal.overlayStrong);
    }
    std::vector<D2D1_POINT_2F> pts(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(values.size() - 1);
        pts[i] = D2D1::Point2F(lerp(plot.left, plot.right, t),
                               lerp(plot.bottom, plot.top, (values[i] - mn) / (mx - mn)));
    }
    r_->fillPolylineArea(pts.data(), static_cast<int>(pts.size()), plot.bottom,
                         D2D1::ColorF(accent.r, accent.g, accent.b, 0.16f));
    for (size_t i = 1; i < pts.size(); ++i) {
        r_->line(pts[i - 1], pts[i], accent, 1.6f);
    }
}

void Ui::barChart(const D2D1_RECT_F& r, const std::vector<float>& values,
                  const std::vector<std::wstring>& labels, D2D1_COLOR_F accent)
{
    tile(r);
    if (values.empty()) {
        return;
    }
    float mx = 0.001f;
    for (float v : values) {
        mx = std::max(mx, v);
    }
    const D2D1_RECT_F plot = inset(r, 16, 24);
    const float slot = (plot.right - plot.left) / static_cast<float>(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        const float h = (values[i] / mx) * (plot.bottom - plot.top - 14);
        const float x = plot.left + i * slot + slot * 0.18f;
        const float w = slot * 0.64f;
        r_->fillRounded(D2D1::RectF(x, plot.bottom - 14 - h, x + w, plot.bottom - 14), 3.0f, accent);
        if (i < labels.size()) {
            r_->text(D2D1::RectF(x - 4, plot.bottom - 14, x + w + 4, plot.bottom + 4), labels[i],
                     r_->smallFont(), theme::pal.secondary, DWRITE_TEXT_ALIGNMENT_CENTER);
        }
    }
}

void Ui::candleChart(const D2D1_RECT_F& r, const std::vector<Candle>& candles)
{
    tile(r);
    if (candles.empty()) {
        return;
    }
    float mn = candles[0].low, mx = candles[0].high;
    for (const auto& c : candles) {
        mn = std::min(mn, c.low);
        mx = std::max(mx, c.high);
    }
    if (mx - mn < 0.001f) {
        mx = mn + 1;
    }
    const D2D1_RECT_F plot = inset(r, 16, 16);
    const float slot = (plot.right - plot.left) / static_cast<float>(candles.size());
    auto mapY = [&](float v) { return lerp(plot.bottom, plot.top, (v - mn) / (mx - mn)); };
    for (size_t i = 0; i < candles.size(); ++i) {
        const auto& c = candles[i];
        const float cx = plot.left + (static_cast<float>(i) + 0.5f) * slot;
        const D2D1_COLOR_F col = c.close >= c.open ? theme::pal.candleUp : theme::pal.candleDown;
        r_->line(D2D1::Point2F(cx, mapY(c.high)), D2D1::Point2F(cx, mapY(c.low)), col, 1.0f);
        const float y1 = mapY(c.open);
        const float y2 = mapY(c.close);
        const float top = std::min(y1, y2);
        const float bot = std::max(y1, y2);
        const float w = std::max(3.0f, slot * 0.45f);
        r_->fillRounded(D2D1::RectF(cx - w * 0.5f, top, cx + w * 0.5f, std::max(bot, top + 1.0f)), 1.0f,
                        col);
    }
}

bool Ui::modal(std::wstring_view title, std::wstring_view body)
{
    const float w = r_->widthDips();
    const float h = r_->heightDips();
    r_->fill(rect(0, 0, w, h), theme::pal.dim);
    const D2D1_RECT_F panel = rect((w - 360) * 0.5f, (h - 180) * 0.5f, 360, 180);
    r_->fillRounded(panel, 10.0f, theme::pal.popup);
    r_->strokeRounded(panel, 10.0f, theme::pal.windowStroke);
    r_->text(rect(panel.left + 20, panel.top + 16, 320, 24), title, r_->titleFont(), theme::pal.title);
    r_->text(rect(panel.left + 20, panel.top + 48, 320, 70), body, r_->bodyFont(), theme::pal.secondary,
             DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    return button(rect(panel.right - 108, panel.bottom - 46, 88, 28), L"OK");
}
