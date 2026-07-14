/*
 * epg_render.hpp — Dignity OS EPG on-screen-display compositor.
 * Concept firmware for the fictional Dignity set-top box.
 *
 * Draws the guide grid, clock bar, info panel and channel banner into the
 * DGY-VDP OSD framebuffer (640x480, RGB5551, double-buffered). The active
 * back buffer is flipped by the VBlank handler in hw_video.s.
 */

#ifndef DGY_EPG_RENDER_HPP
#define DGY_EPG_RENDER_HPP

#include <cstdint>

#include "epg_core.h"

namespace dgy {

using Pixel = std::uint16_t;   // RGB5551

struct Rect {
    int x, y, w, h;
};

// Palette entries live in the theme bank so regional builds can reskin the
// guide without touching code. Indices, not colors, are compiled in.
enum class Ink : std::uint8_t {
    GridBg,          // deep navy field behind the grid
    GridLine,
    CellText,
    CursorFill,      // TV Guide yellow highlight
    CursorText,
    HeaderFill,      // Dignity red header band
    HeaderText,
    PanelFill,
    PanelText,
    Dim
};

class Surface {
public:
    Surface(Pixel *pixels, int width, int height);

    void fill(const Rect &r, Ink ink);
    void frame(const Rect &r, Ink ink);
    // 8x16 ROM font; clips to `r`, ellipsizes on overflow.
    void text(const Rect &r, const char *utf8, Ink ink);

private:
    Pixel *m_px;
    int    m_w;
    int    m_h;
};

class GuideRenderer {
public:
    explicit GuideRenderer(Surface &surface);

    // Recomposite the full guide OSD. `now_min` drives the clock bar and
    // the NOW marker; strings come from ROM string bank 0 (strings_en.xml).
    void compose(const epg_view_t &view, std::uint16_t now_min);

private:
    void drawHeader(std::uint16_t now_min);
    void drawTimeBar(std::uint16_t window_min);
    void drawChannelColumn(const epg_view_t &view);
    void drawGrid(const epg_view_t &view);
    void drawInfoPanel(const epg_event_t *ev);
    void drawKeyHints(bool details_open);

    Rect cellRect(int row, int col, int span_cols) const;

    Surface &m_surf;
};

} // namespace dgy

#endif // DGY_EPG_RENDER_HPP
