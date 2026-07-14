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
| `ui/boot.html` | HTML/CSS/JS | Startup animation with a synthesized boot jingle; visuals are clocked off the audio timeline. |
| `ui/epg.html` | HTML/CSS/JS | Interactive on-screen simulation of the EPG. Open in any browser. |
| `src/epg_core.h` / `src/epg_core.c` | C | Guide-data engine: channel map, event ring, cursor/navigation state machine. |
| `src/epg_render.hpp` / `src/epg_render.cpp` | C++ | OSD compositor: draws the grid, info panel, and channel banner into the VDP framebuffer. |
| `src/hw_video.s` | MIPS assembly | Low-level init for the fictional DGY-VDP video display processor and VBlank handler. |
| `src/boot_anim.h` / `src/boot_anim.c` | C | Startup-animation sequencer; fires video keyframes off the APU sample counter. |
| `src/hw_audio.s` | MIPS assembly | DGY-APU init, voice gating, and the DAC sample counter used as the A/V master clock. |
| `data/tvguide_listings.xml` | XML | Program listings in TV Guide style (channels, events, descriptions, ratings). |
| `data/strings_en.xml` | XML | UI string table (English) referenced by string ID from the firmware. |
| `data/boot_timeline.xml` | XML | Startup A/V timeline: video keyframes and jingle notes on one sample clock. |

## Trying the UI

Open `ui/boot.html` for the startup sequence (click or press any key to power
on; **A**/Enter continues into the guide, **B**/Backspace replays), or open
`ui/epg.html` to jump straight to the guide.

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

### Startup A/V sync

The boot jingle and the boot animation share one timeline
(`data/boot_timeline.xml`), authored in APU samples at 32 kHz. The DGY-APU's
DAC sample counter is the master clock: each video frame, `boot_anim_step()`
reads the counter and fires every keyframe it has passed, starting jingle
voices from the same table. A dropped frame emits its missed keyframes on the
next step, so the picture snaps back onto the soundtrack instead of drifting.
The HTML simulation mirrors this by scheduling all notes on the Web Audio
clock and driving CSS keyframes from `AudioContext.currentTime`.
