# Dignity OS · Electronic Program Guide

Concept UI for the **Dignity** set-top box — *Digital Intelligence Generations Now in the Yield* — a fan-fiction Nintendo firmware design study.

A self-contained, symbol-driven program guide simulation:

- **Boot sequence** — orbit-ring logo animation with a synthesized firmware startup chime
- **Animated guide grid** — 12-hour XML listing window, 2 hours on screen, live now-line, on-air highlighting, smooth row scrolling
- **Symbols over words** — genre, broadcast-flag and control glyphs are all inline SVG; on-screen text is kept to titles and times
- **Sound design** — every interaction (move, select, back, channel zap, denied) is synthesized live via the Web Audio API, no audio files
- **Now-playing deck** — faux-broadcast canvas preview with channel-change static burst, progress bar and live badge
- **Detail overlay** — A-button program card with watch / record / remind actions

## Controls

| Input | Action |
| --- | --- |
| ← → ↑ ↓ | Navigate grid / scroll time window |
| Enter / A | Program details |
| Esc / Backspace / B | Back |
| X | Tune preview to selected channel |
| Click | Select · click again for details |

## Notes

The in-fiction firmware is written in C, C++ and assembly with XML service listings (TV-Guide-style feed schema). This simulation mirrors that: the schedule is parsed at runtime from an embedded `<epg>` XML document (`services` → `events`), and the UI layer is plain HTML/CSS/JS with no dependencies — open `index.html` in any browser.

All channels, programs and marks are fictional.
