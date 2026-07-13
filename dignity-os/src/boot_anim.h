/*
 * boot_anim.h — Dignity OS startup animation sequencer.
 * Concept firmware for the fictional Dignity set-top box.
 *
 * A/V sync design: the DGY-APU DAC sample counter is the master clock.
 * Both the jingle notes and the video keyframes are authored in APU
 * samples (data/boot_timeline.xml), so a frame fires exactly when the
 * DAC has consumed that many samples — the picture can lag a frame but
 * can never drift relative to the audio.
 */

#ifndef DGY_BOOT_ANIM_H
#define DGY_BOOT_ANIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_RATE_HZ    32000u
#define BOOT_LETTERS    7u          /* D I G N I T Y */

typedef enum {
    BOOT_OP_NONE = 0,
    BOOT_OP_CRT_FLASH,
    BOOT_OP_LETTER,                 /* arg = letter index               */
    BOOT_OP_SWEEP,
    BOOT_OP_TAGLINE,
    BOOT_OP_OSLINE,
    BOOT_OP_DONE
} boot_op_t;

typedef struct {
    uint32_t  sample;               /* APU samples since power-on       */
    boot_op_t op;
    uint8_t   arg;
} boot_key_t;

/* Arm the sequencer: reset the keyframe cursor, start the APU jingle
 * voices and zero the DAC sample counter. */
void boot_anim_start(void);

/* Called once per video frame by the shell. Emits every keyframe whose
 * timestamp the DAC counter has passed since the previous call, invoking
 * `emit` for each. Returns 0 while running, 1 once BOOT_OP_DONE fired. */
int boot_anim_step(void (*emit)(boot_op_t op, uint8_t arg));

#ifdef __cplusplus
}
#endif

#endif /* DGY_BOOT_ANIM_H */
