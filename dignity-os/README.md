# Dignity OS — Electronic Program Guide (EPG)

**Dignity** — *Digital Intelligence Generations Now in the Yield* — is a fictional
Nintendo set-top box concept created for this design studio. This directory contains
the concept firmware sources and UI simulation for the **Dignity OS Electronic
Program Guide**, styled after classic printed/on-screen TV Guide listings.

> This is an original fan-fiction design concept. It is not affiliated with,
> endorsed by, or produced by Nintendo Co., Ltd. or TV Guide.

## Layout

| Path | Language | Purpose |
| --- | --- | --- |
| `ui/epg.html` | HTML/CSS/JS | Interactive on-screen simulation of the EPG. Open in any browser. |
| `src/epg_core.h` / `src/epg_core.c` | C | Guide-data engine: channel map, event ring, cursor/navigation state machine. |
| `src/epg_render.hpp` / `src/epg_render.cpp` | C++ | OSD compositor: draws the grid, info panel, and channel banner into the VDP framebuffer. |
| `src/hw_video.s` | MIPS assembly | Low-level init for the fictional DGY-VDP video display processor and VBlank handler. |
| `data/tvguide_listings.xml` | XML | Program listings in TV Guide style (channels, events, descriptions, ratings). |
| `data/strings_en.xml` | XML | UI string table (English) referenced by string ID from the firmware. |

## Trying the UI

Open `ui/epg.html` in a browser.

Controls (mapped to the Dignity remote):

| Key | Remote button | Action |
| --- | --- | --- |
| Arrow keys | D-pad | Move the grid cursor |
| Enter | A | Tune to channel / show details |
| Backspace | B | Back / close details |
| `[` / `]` | L / R | Page back / forward 90 minutes |
| `D` | GUIDE | Toggle the guide overlay |

## Concept build notes

The firmware targets the fictional **DGY-1 "Yield" SoC** (MIPS R4300-class core,
DGY-VDP tile/OSD video unit). `epg_core` keeps guide data received from the
broadcast carousel in a fixed-size event ring (no heap after boot), `epg_render`
composites the OSD each VBlank, and `hw_video.s` brings the VDP out of reset and
services the VBlank interrupt. Listings and strings are authored in XML and baked
into the ROM string/data banks at build time.
