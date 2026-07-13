/*
 * epg_core.h — Dignity OS Electronic Program Guide, guide-data engine.
 *
 * Concept firmware for the Dignity set-top box (DGY-1 "Yield" SoC).
 * Fictional design study; not a Nintendo product.
 *
 * The engine owns the channel map and a fixed-size ring of guide events
 * parsed from the broadcast listings carousel (see data/tvguide_listings.xml
 * for the authoring format). No heap allocation occurs after epg_init().
 */

#ifndef DGY_EPG_CORE_H
#define DGY_EPG_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EPG_MAX_CHANNELS   64
#define EPG_MAX_EVENTS     1024
#define EPG_TITLE_LEN      48
#define EPG_DESC_LEN       160
#define EPG_CALLSIGN_LEN   8

/* Grid geometry: 6 channel rows visible, 3 half-hour columns. */
#define EPG_GRID_ROWS      6
#define EPG_GRID_COLS      3
#define EPG_SLOT_MINUTES   30

typedef enum {
    EPG_RATING_NONE = 0,
    EPG_RATING_TVY,
    EPG_RATING_TVY7,
    EPG_RATING_TVG,
    EPG_RATING_TVPG,
    EPG_RATING_TV14,
    EPG_RATING_TVMA
} epg_rating_t;

typedef struct {
    uint16_t     channel;              /* index into channel table       */
    uint16_t     start_min;            /* minutes since 00:00 local      */
    uint16_t     dur_min;
    epg_rating_t rating;
    char         title[EPG_TITLE_LEN];
    char         desc[EPG_DESC_LEN];
} epg_event_t;

typedef struct {
    uint16_t number;                   /* display channel number         */
    char     callsign[EPG_CALLSIGN_LEN];
    uint16_t first_event;              /* index of first event, or 0xFFFF */
    uint16_t event_count;
    uint8_t  favorite;
} epg_channel_t;

/* Cursor / view state driven by the remote. Rendered by epg_render.cpp. */
typedef struct {
    uint16_t top_row;                  /* first visible channel row      */
    uint8_t  cur_row;                  /* 0..EPG_GRID_ROWS-1             */
    uint8_t  cur_col;                  /* 0..EPG_GRID_COLS-1             */
    uint16_t window_min;               /* left edge of the time window   */
    uint8_t  details_open;             /* B-panel expanded               */
} epg_view_t;

typedef enum {
    EPG_KEY_UP, EPG_KEY_DOWN, EPG_KEY_LEFT, EPG_KEY_RIGHT,
    EPG_KEY_A,      /* tune / details  */
    EPG_KEY_B,      /* back            */
    EPG_KEY_L,      /* page -90 min    */
    EPG_KEY_R,      /* page +90 min    */
    EPG_KEY_GUIDE
} epg_key_t;

/* Lifecycle ---------------------------------------------------------- */

/* Reset tables and view state. Called once from the OS shell at boot. */
void epg_init(void);

/* Feed one carousel section (mklistings binary record) into the ring.
 * Returns 0 on success, -1 if the ring or channel map is full. */
int epg_ingest_section(const uint8_t *section, uint32_t len);

/* Input / queries ---------------------------------------------------- */

/* Advance the state machine for one remote keypress.
 * Returns nonzero if the OSD must be recomposited. */
int epg_handle_key(epg_key_t key, uint16_t now_min);

/* Event under the cursor, or NULL for an empty cell. */
const epg_event_t *epg_cursor_event(void);

/* Event airing on `channel` at absolute minute `at`, or NULL. */
const epg_event_t *epg_event_at(uint16_t channel, uint16_t at);

const epg_channel_t *epg_channel(uint16_t index);
uint16_t             epg_channel_count(void);
const epg_view_t    *epg_view(void);

#ifdef __cplusplus
}
#endif

#endif /* DGY_EPG_CORE_H */
