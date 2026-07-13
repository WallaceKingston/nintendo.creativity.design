# hw_audio.s — DGY-APU bring-up and voice control for Dignity OS.
# Concept firmware for the fictional Dignity set-top box (DGY-1 "Yield" SoC,
# MIPS R4300-class core). Assembles with gas, -march=vr4300 -mabi=32.
#
# The APU is a 4-voice wavetable synth with hardware pluck/decay envelopes,
# mixed to a 32 kHz stereo DAC. SAMPLE_CNT increments once per DAC sample
# and is the master clock for boot-time A/V sync: boot_anim.c fires video
# keyframes when this counter passes their authored timestamps, so picture
# and jingle can never drift apart.

        .set    noreorder

# ---- APU register file (KSEG1) ---------------------------------------------
        .equ    APU_BASE,       0xA4500000
        .equ    APU_CTRL,       0x00        # bit0 enable, bit1 DAC enable
        .equ    APU_SAMPLE_CNT, 0x04        # r/o, samples since counter reset
        .equ    APU_CNT_RESET,  0x08        # write 1 to zero SAMPLE_CNT
        .equ    APU_V0_FREQ,    0x10        # voice regs, stride 0x10
        .equ    APU_V0_LEVEL,   0x14        #   level 0..63, bit7 = gate
        .equ    APU_V0_DUR,     0x18        #   envelope duration, ms
        .equ    APU_VOICE_STRIDE, 0x10

        .equ    CTRL_ENABLE,    0x1
        .equ    CTRL_DAC_ON,    0x2
        .equ    LEVEL_GATE,     0x80

        .text
        .align  2

# ----------------------------------------------------------------------------
# void apu_init(void)
# Enable the APU and DAC, silence all four voices.
# ----------------------------------------------------------------------------
        .globl  apu_init
        .ent    apu_init
apu_init:
        lui     $t0, %hi(APU_BASE)
        ori     $t0, $t0, %lo(APU_BASE)

        li      $t1, CTRL_ENABLE | CTRL_DAC_ON
        sw      $t1, APU_CTRL($t0)

        li      $t2, 4                      # silence voices 0..3
        addiu   $t3, $t0, APU_V0_LEVEL
1:      sw      $zero, 0($t3)
        addiu   $t2, $t2, -1
        bnez    $t2, 1b
        addiu   $t3, $t3, APU_VOICE_STRIDE  # (delay slot) next voice

        jr      $ra
        nop
        .end    apu_init

# ----------------------------------------------------------------------------
# void apu_counter_reset(void)
# Zero the DAC sample counter. Called by boot_anim_start() at power-on so
# timeline timestamps are relative to the start of the animation.
# ----------------------------------------------------------------------------
        .globl  apu_counter_reset
        .ent    apu_counter_reset
apu_counter_reset:
        lui     $t0, %hi(APU_BASE)
        ori     $t0, $t0, %lo(APU_BASE)
        li      $t1, 1
        jr      $ra
        sw      $t1, APU_CNT_RESET($t0)     # (delay slot)
        .end    apu_counter_reset

# ----------------------------------------------------------------------------
# uint32_t apu_sample_count(void)
# Read the master A/V clock. Two reads guard against catching the counter
# mid-increment on the bus (the APU latches on the second read).
# ----------------------------------------------------------------------------
        .globl  apu_sample_count
        .ent    apu_sample_count
apu_sample_count:
        lui     $t0, %hi(APU_BASE)
        ori     $t0, $t0, %lo(APU_BASE)
        lw      $v0, APU_SAMPLE_CNT($t0)
        lw      $v0, APU_SAMPLE_CNT($t0)
        jr      $ra
        nop
        .end    apu_sample_count

# ----------------------------------------------------------------------------
# void apu_voice_on(uint8_t voice, uint16_t hz, uint8_t level, uint16_t dur_ms)
# Gate one voice: a0=voice(0..3) a1=hz a2=level(0..63) a3=envelope ms.
# The hardware envelope releases the gate itself after dur_ms.
# ----------------------------------------------------------------------------
        .globl  apu_voice_on
        .ent    apu_voice_on
apu_voice_on:
        lui     $t0, %hi(APU_BASE)
        ori     $t0, $t0, %lo(APU_BASE)

        andi    $a0, $a0, 3                 # clamp voice index
        sll     $t1, $a0, 4                 # voice * APU_VOICE_STRIDE
        addu    $t0, $t0, $t1

        sw      $a1, APU_V0_FREQ($t0)
        sw      $a3, APU_V0_DUR($t0)

        andi    $t2, $a2, 0x3F
        ori     $t2, $t2, LEVEL_GATE        # set level + open the gate
        jr      $ra
        sw      $t2, APU_V0_LEVEL($t0)      # (delay slot)
        .end    apu_voice_on
