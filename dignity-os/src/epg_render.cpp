/*
 * epg_render.cpp — Dignity OS EPG on-screen-display compositor.
 * Concept firmware for the fictional Dignity set-top box.
 */

#include "epg_render.hpp"

#include <cstdio>
#include <cstring>

// Generated from data/strings_en.xml by tools/mkstrings.
extern "C" const char *str_get(unsigned id);
#define STR_GUIDE_TITLE   2u
#define STR_GUIDE_SOURCE  3u
#define STR_NO_INFO       8u
#define STR_TUNE          9u
#define STR_DETAILS       10u
#define STR_BACK          11u
#define STR_PAGE_TIME     12u

// VDP blit primitives, implemented in hw_video.s on real hardware.
extern "C" void vdp_fill_rect(std::uint16_t *fb, int fbw,
                              int x, int y, int w, int h, dgy::Pixel c);
extern "C" void vdp_draw_glyphs(std::uint16_t *fb, int fbw,
                                int x, int y, const char *utf8,
                                int clip_w, dgy::Pixel c);
extern "C" dgy::Pixel theme_ink(std::uint8_t index);

namespace dgy {

namespace {

// Screen metrics (title-safe area of the 640x480 OSD plane).
constexpr int kScreenW   = 640;
constexpr int kHeaderH   = 44;
constexpr int kTimeBarH  = 24;
constexpr int kChanColW  = 96;
constexpr int kRowH      = 42;
constexpr int kPanelY    = kHeaderH + kTimeBarH + EPG_GRID_ROWS * kRowH;
constexpr int kPanelH    = 92;
constexpr int kSlotW     = (kScreenW - kChanColW) / EPG_GRID_COLS;

void format_clock(char *out, std::size_t cap, unsigned minutes) {
    unsigned h24 = (minutes / 60) % 24;
    unsigned h12 = h24 % 12 ? h24 % 12 : 12;
    std::snprintf(out, cap, "%u:%02u %s", h12, minutes % 60,
                  h24 < 12 ? "AM" : "PM");
}

const char *rating_label(epg_rating_t r) {
    switch (r) {
    case EPG_RATING_TVY:  return "TV-Y";
    case EPG_RATING_TVY7: return "TV-Y7";
    case EPG_RATING_TVG:  return "TV-G";
    case EPG_RATING_TVPG: return "TV-PG";
    case EPG_RATING_TV14: return "TV-14";
    case EPG_RATING_TVMA: return "TV-MA";
    default:              return "";
    }
}

} // namespace

Surface::Surface(Pixel *pixels, int width, int height)
    : m_px(pixels), m_w(width), m_h(height) {}

void Surface::fill(const Rect &r, Ink ink) {
    vdp_fill_rect(m_px, m_w, r.x, r.y, r.w, r.h,
                  theme_ink(static_cast<std::uint8_t>(ink)));
}

void Surface::frame(const Rect &r, Ink ink) {
    fill({r.x, r.y, r.w, 1}, ink);
    fill({r.x, r.y + r.h - 1, r.w, 1}, ink);
    fill({r.x, r.y, 1, r.h}, ink);
    fill({r.x + r.w - 1, r.y, 1, r.h}, ink);
}

void Surface::text(const Rect &r, const char *utf8, Ink ink) {
    vdp_draw_glyphs(m_px, m_w, r.x + 4, r.y + (r.h - 16) / 2, utf8,
                    r.w - 8, theme_ink(static_cast<std::uint8_t>(ink)));
}

GuideRenderer::GuideRenderer(Surface &surface) : m_surf(surface) {}

Rect GuideRenderer::cellRect(int row, int col, int span_cols) const {
    return {kChanColW + col * kSlotW,
            kHeaderH + kTimeBarH + row * kRowH,
            span_cols * kSlotW,
            kRowH};
}

void GuideRenderer::compose(const epg_view_t &view, std::uint16_t now_min) {
    m_surf.fill({0, 0, kScreenW, kPanelY + kPanelH + 24}, Ink::GridBg);
    drawHeader(now_min);
    drawTimeBar(view.window_min);
    drawChannelColumn(view);
    drawGrid(view);
    drawInfoPanel(epg_cursor_event());
    drawKeyHints(view.details_open != 0);
}

void GuideRenderer::drawHeader(std::uint16_t now_min) {
    char clock[16];
    format_clock(clock, sizeof clock, now_min);

    m_surf.fill({0, 0, kScreenW, kHeaderH}, Ink::HeaderFill);
    m_surf.text({12, 0, 280, kHeaderH}, str_get(STR_GUIDE_TITLE), Ink::HeaderText);
    m_surf.text({kScreenW - 240, 0, 130, kHeaderH}, str_get(STR_GUIDE_SOURCE),
                Ink::HeaderText);
    m_surf.text({kScreenW - 100, 0, 90, kHeaderH}, clock, Ink::HeaderText);
}

void GuideRenderer::drawTimeBar(std::uint16_t window_min) {
    m_surf.fill({0, kHeaderH, kScreenW, kTimeBarH}, Ink::PanelFill);
    for (int col = 0; col < EPG_GRID_COLS; ++col) {
        char label[16];
        format_clock(label, sizeof label,
                     window_min + static_cast<unsigned>(col) * EPG_SLOT_MINUTES);
        m_surf.text({kChanColW + col * kSlotW, kHeaderH, kSlotW, kTimeBarH},
                    label, Ink::PanelText);
    }
}

void GuideRenderer::drawChannelColumn(const epg_view_t &view) {
    for (int row = 0; row < EPG_GRID_ROWS; ++row) {
        const epg_channel_t *ch =
            epg_channel(static_cast<std::uint16_t>(view.top_row + row));
        Rect r{0, kHeaderH + kTimeBarH + row * kRowH, kChanColW, kRowH};
        m_surf.frame(r, Ink::GridLine);
        if (!ch)
            continue;
        char label[24];
        std::snprintf(label, sizeof label, "%u %s", ch->number, ch->callsign);
        m_surf.text(r, label, Ink::CellText);
    }
}

void GuideRenderer::drawGrid(const epg_view_t &view) {
    for (int row = 0; row < EPG_GRID_ROWS; ++row) {
        auto ch = static_cast<std::uint16_t>(view.top_row + row);

        for (int col = 0; col < EPG_GRID_COLS; ) {
            auto at = static_cast<std::uint16_t>(view.window_min +
                                                 col * EPG_SLOT_MINUTES);
            const epg_event_t *ev = epg_event_at(ch, at);

            // Merge the run of slots covered by this event into one cell.
            int span = 1;
            while (col + span < EPG_GRID_COLS &&
                   ev && ev == epg_event_at(ch, static_cast<std::uint16_t>(
                                 at + span * EPG_SLOT_MINUTES)))
                ++span;

            bool hot = row == view.cur_row &&
                       view.cur_col >= col && view.cur_col < col + span;
            Rect r = cellRect(row, col, span);
            if (hot)
                m_surf.fill(r, Ink::CursorFill);
            m_surf.frame(r, Ink::GridLine);
            m_surf.text(r, ev ? ev->title : str_get(STR_NO_INFO),
                        hot ? Ink::CursorText : (ev ? Ink::CellText : Ink::Dim));
            col += span;
        }
    }
}

void GuideRenderer::drawInfoPanel(const epg_event_t *ev) {
    Rect panel{0, kPanelY, kScreenW, kPanelH};
    m_surf.fill(panel, Ink::PanelFill);
    m_surf.frame(panel, Ink::GridLine);

    if (!ev) {
        m_surf.text({12, kPanelY + 8, kScreenW - 24, 20}, str_get(STR_NO_INFO),
                    Ink::Dim);
        return;
    }

    char line[96];
    char from[16], to[16];
    format_clock(from, sizeof from, ev->start_min);
    format_clock(to, sizeof to, ev->start_min + ev->dur_min);
    std::snprintf(line, sizeof line, "%s  |  %s - %s  |  %s",
                  ev->title, from, to, rating_label(ev->rating));

    m_surf.text({12, kPanelY + 8, kScreenW - 24, 20}, line, Ink::PanelText);
    m_surf.text({12, kPanelY + 34, kScreenW - 24, 48}, ev->desc, Ink::CellText);
}

void GuideRenderer::drawKeyHints(bool details_open) {
    char hints[128];
    std::snprintf(hints, sizeof hints, "A %s   B %s   L/R %s",
                  str_get(details_open ? STR_TUNE : STR_DETAILS),
                  str_get(STR_BACK), str_get(STR_PAGE_TIME));
    m_surf.text({0, kPanelY + kPanelH, kScreenW, 24}, hints, Ink::Dim);
}

} // namespace dgy
