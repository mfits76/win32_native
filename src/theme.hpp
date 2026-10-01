#pragma once

#include "common.hpp"

// Compact instrument chrome in the same family as TMOG. Colors live in `pal`
// so the window can switch light/dark without GDI widgets.
namespace theme {

inline constexpr float windowWidth = 1120.0f;
inline constexpr float windowHeight = 740.0f;
inline constexpr float headerHeight = 44.0f;
inline constexpr float menuHeight = 28.0f;
inline constexpr float pad = 12.0f;
inline constexpr float gap = 8.0f;
inline constexpr float windowRadius = 12.0f;
inline constexpr float tileRadius = 9.0f;
inline constexpr float controlRadius = 6.0f;
inline constexpr float resizeEdge = 6.0f;
inline constexpr float captionBtn = 36.0f;
inline constexpr float themeToggleW = 92.0f;
inline constexpr float fontBtnW = 28.0f;
inline constexpr float splitterHit = 8.0f;
inline constexpr float rowHeight = 26.0f;
inline constexpr float tabHeight = 30.0f;

struct Palette {
    bool dark = true;
    D2D1_COLOR_F windowTop{};
    D2D1_COLOR_F windowBottom{};
    D2D1_COLOR_F windowStroke{};
    D2D1_COLOR_F headerTop{};
    D2D1_COLOR_F headerBottom{};
    D2D1_COLOR_F tile{};
    D2D1_COLOR_F tileStroke{};
    D2D1_COLOR_F title{};
    D2D1_COLOR_F primary{};
    D2D1_COLOR_F secondary{};
    D2D1_COLOR_F control{};
    D2D1_COLOR_F rule{};
    D2D1_COLOR_F closeHover{};
    D2D1_COLOR_F buttonFace{};
    D2D1_COLOR_F buttonHover{};
    D2D1_COLOR_F buttonPress{};
    D2D1_COLOR_F unlit{};
    D2D1_COLOR_F phosphorGreen{};
    D2D1_COLOR_F phosphorAmber{};
    D2D1_COLOR_F phosphorCyan{};
    D2D1_COLOR_F selection{};
    D2D1_COLOR_F candleUp{};
    D2D1_COLOR_F candleDown{};
    D2D1_COLOR_F popup{};
    D2D1_COLOR_F dim{};
    D2D1_COLOR_F checkFill{};
    D2D1_COLOR_F chipSelected{};
    D2D1_COLOR_F captionHover{};
    D2D1_COLOR_F field{};
    D2D1_COLOR_F overlay{};
    D2D1_COLOR_F overlayStrong{};
};

inline Palette makeDark()
{
    Palette p{};
    p.dark = true;
    p.windowTop = rgb(0x101418);
    p.windowBottom = rgb(0x05070A);
    p.windowStroke = rgb(0x43505C, 0.85f);
    p.headerTop = rgb(0x141A20);
    p.headerBottom = rgb(0x080C10);
    p.tile = rgb(0x070A0D, 0.92f);
    p.tileStroke = rgb(0xFFFFFF, 0.08f);
    p.title = rgb(0xE6EDF3);
    p.primary = rgb(0xDCE6EF);
    p.secondary = rgb(0x7D8B99);
    p.control = rgb(0xC9D6E2);
    p.rule = rgb(0xFFFFFF, 0.08f);
    p.closeHover = rgb(0xC42B1C);
    p.buttonFace = rgb(0x1A222B);
    p.buttonHover = rgb(0x24303A);
    p.buttonPress = rgb(0x12181F);
    p.unlit = rgb(0xFFFFFF, 0.06f);
    p.phosphorGreen = rgb(0x1AFF1C);
    p.phosphorAmber = rgb(0xE4DF6A);
    p.phosphorCyan = rgb(0x4FD1C5);
    p.selection = rgb(0x4FD1C5, 0.16f);
    p.candleUp = rgb(0x3DDC97);
    p.candleDown = rgb(0xF07178);
    p.popup = rgb(0x12181F, 0.98f);
    p.dim = rgb(0x000000, 0.45f);
    p.checkFill = rgb(0x14332F);
    p.chipSelected = rgb(0x13241F);
    p.captionHover = rgb(0xFFFFFF, 0.08f);
    p.field = rgb(0x0C1014);
    p.overlay = rgb(0xFFFFFF, 0.025f);
    p.overlayStrong = rgb(0xFFFFFF, 0.04f);
    return p;
}

inline Palette makeLight()
{
    Palette p{};
    p.dark = false;
    p.windowTop = rgb(0xF4F6F8);
    p.windowBottom = rgb(0xE6EBEF);
    p.windowStroke = rgb(0x8A9AAB, 0.85f);
    p.headerTop = rgb(0xEEF2F6);
    p.headerBottom = rgb(0xE2E8EE);
    p.tile = rgb(0xFFFFFF, 0.94f);
    p.tileStroke = rgb(0x1B2430, 0.10f);
    p.title = rgb(0x1B2430);
    p.primary = rgb(0x243040);
    p.secondary = rgb(0x5C6B78);
    p.control = rgb(0x3D4A57);
    p.rule = rgb(0x1B2430, 0.10f);
    p.closeHover = rgb(0xC42B1C);
    p.buttonFace = rgb(0xE8EEF3);
    p.buttonHover = rgb(0xDDE5EC);
    p.buttonPress = rgb(0xD0D9E2);
    p.unlit = rgb(0x1B2430, 0.08f);
    p.phosphorGreen = rgb(0x1A9E32);
    p.phosphorAmber = rgb(0xC49A14);
    p.phosphorCyan = rgb(0x1A8F86);
    p.selection = rgb(0x1A8F86, 0.18f);
    p.candleUp = rgb(0x1A9E62);
    p.candleDown = rgb(0xD04A55);
    p.popup = rgb(0xFFFFFF, 0.98f);
    p.dim = rgb(0x1B2430, 0.35f);
    p.checkFill = rgb(0xD7F1EE);
    p.chipSelected = rgb(0xD7F1EE);
    p.captionHover = rgb(0x1B2430, 0.06f);
    p.field = rgb(0xFFFFFF);
    p.overlay = rgb(0x1B2430, 0.04f);
    p.overlayStrong = rgb(0x1B2430, 0.06f);
    return p;
}

inline Palette pal = makeDark();

inline void setDark(bool dark)
{
    pal = dark ? makeDark() : makeLight();
}

} // namespace theme
