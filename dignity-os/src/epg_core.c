/*
 * epg_core.c — Dignity OS EPG guide-data engine.
 * Concept firmware for the fictional Dignity set-top box.
 *
 * All storage is static: the DGY-1 shell forbids heap use after boot so a
 * corrupt carousel can never fragment memory. Sections that do not fit are
 * dropped and re-acquired on the next carousel cycle.
 */

#include "epg_core.h"

#include <string.h>

static epg_channel_t s_channels[EPG_MAX_CHANNELS];
static epg_event_t   s_events[EPG_MAX_EVENTS];
static uint16_t      s_channel_count;
static uint16_t      s_event_count;
static epg_view_t    s_view;

/* mklistings section layout (little-endian, packed by the build tools):
 *   u16 channel_number   u16 start_min   u16 dur_min   u8 rating
 *   u8  callsign[8]      char title[48]  char desc[160]
 */
#define SEC_LEN (2u + 2u + 2u + 1u + EPG_CALLSIGN_LEN + EPG_TITLE_LEN + EPG_DESC_LEN)

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static int channel_lookup(uint16_t number, const uint8_t *callsign) {
    uint16_t i;
    for (i = 0; i < s_channel_count; i++) {
        if (s_channels[i].number == number)
            return (int)i;
    }
    if (s_channel_count == EPG_MAX_CHANNELS)
        return -1;
    i = s_channel_count++;
    s_channels[i].number = number;
    memcpy(s_channels[i].callsign, callsign, EPG_CALLSIGN_LEN);
    s_channels[i].callsign[EPG_CALLSIGN_LEN - 1] = '\0';
    s_channels[i].first_event = 0xFFFF;
    s_channels[i].event_count = 0;
    s_channels[i].favorite = 0;
    return (int)i;
}

void epg_init(void) {
    memset(s_channels, 0, sizeof s_channels);
    memset(s_events, 0, sizeof s_events);
    s_channel_count = 0;
    s_event_count = 0;

    memset(&s_view, 0, sizeof s_view);
    s_view.window_min = 19 * 60;   /* guide opens on primetime */
}

int epg_ingest_section(const uint8_t *section, uint32_t len) {
    const uint8_t *p = section;
    epg_event_t *ev;
    int ch;

    if (len < SEC_LEN || s_event_count == EPG_MAX_EVENTS)
        return -1;

    ch = channel_lookup(rd16(p), p + 7);
    if (ch < 0)
        return -1;

    ev = &s_events[s_event_count];
    ev->channel   = (uint16_t)ch;
    ev->start_min = rd16(p + 2);
    ev->dur_min   = rd16(p + 4);
    ev->rating    = (epg_rating_t)(p[6] <= EPG_RATING_TVMA ? p[6] : EPG_RATING_NONE);
    memcpy(ev->title, p + 7 + EPG_CALLSIGN_LEN, EPG_TITLE_LEN);
    ev->title[EPG_TITLE_LEN - 1] = '\0';
    memcpy(ev->desc, p + 7 + EPG_CALLSIGN_LEN + EPG_TITLE_LEN, EPG_DESC_LEN);
    ev->desc[EPG_DESC_LEN - 1] = '\0';

    if (s_channels[ch].first_event == 0xFFFF)
        s_channels[ch].first_event = s_event_count;
    s_channels[ch].event_count++;
    s_event_count++;
    return 0;
}

const epg_event_t *epg_event_at(uint16_t channel, uint16_t at) {
    const epg_channel_t *c;
    uint16_t i;

    if (channel >= s_channel_count)
        return 0;
    c = &s_channels[channel];
    for (i = 0; i < c->event_count; i++) {
        const epg_event_t *ev = &s_events[c->first_event + i];
        if (at >= ev->start_min && at < (uint16_t)(ev->start_min + ev->dur_min))
            return ev;
    }
    return 0;
}

const epg_event_t *epg_cursor_event(void) {
    uint16_t ch = (uint16_t)(s_view.top_row + s_view.cur_row);
    uint16_t at = (uint16_t)(s_view.window_min + s_view.cur_col * EPG_SLOT_MINUTES);
    return epg_event_at(ch, at);
}

int epg_handle_key(epg_key_t key, uint16_t now_min) {
    (void)now_min;

    switch (key) {
    case EPG_KEY_UP:
        if (s_view.cur_row > 0)
            s_view.cur_row--;
        else if (s_view.top_row > 0)
            s_view.top_row--;
        else
            return 0;
        return 1;

    case EPG_KEY_DOWN:
        if (s_view.top_row + s_view.cur_row + 1 >= s_channel_count)
            return 0;
        if (s_view.cur_row < EPG_GRID_ROWS - 1)
            s_view.cur_row++;
        else
            s_view.top_row++;
        return 1;

    case EPG_KEY_LEFT:
        if (s_view.cur_col > 0) {
            s_view.cur_col--;
        } else if (s_view.window_min >= EPG_SLOT_MINUTES) {
            s_view.window_min -= EPG_SLOT_MINUTES;
        } else {
            return 0;
        }
        return 1;

    case EPG_KEY_RIGHT:
        if (s_view.cur_col < EPG_GRID_COLS - 1)
            s_view.cur_col++;
        else if (s_view.window_min + EPG_GRID_COLS * EPG_SLOT_MINUTES < 24 * 60)
            s_view.window_min += EPG_SLOT_MINUTES;
        else
            return 0;
        return 1;

    case EPG_KEY_L:
        if (s_view.window_min < 90)
            s_view.window_min = 0;
        else
            s_view.window_min -= 90;
        return 1;

    case EPG_KEY_R:
        if (s_view.window_min + 90 + EPG_GRID_COLS * EPG_SLOT_MINUTES <= 24 * 60)
            s_view.window_min += 90;
        return 1;

    case EPG_KEY_A:
        s_view.details_open = 1;
        return 1;

    case EPG_KEY_B:
        if (!s_view.details_open)
            return 0;
        s_view.details_open = 0;
        return 1;

    case EPG_KEY_GUIDE:
    default:
        return 0;   /* GUIDE toggling is owned by the OS shell */
    }
}

const epg_channel_t *epg_channel(uint16_t index) {
    return index < s_channel_count ? &s_channels[index] : 0;
}

uint16_t epg_channel_count(void) {
    return s_channel_count;
}

const epg_view_t *epg_view(void) {
    return &s_view;
}
