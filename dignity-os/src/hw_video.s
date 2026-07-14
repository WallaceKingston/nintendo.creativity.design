# hw_video.s — DGY-VDP bring-up and VBlank service for Dignity OS.
# Concept firmware for the fictional Dignity set-top box (DGY-1 "Yield" SoC,
# MIPS R4300-class core). Assembles with gas, -march=vr4300 -mabi=32.
#
# The VDP exposes a small register file in KSEG1 (uncached) space. The OSD
# plane is a 640x480 RGB5551 surface, double-buffered; epg_render.cpp draws
# into the back buffer and vdp_vblank_isr flips it during vertical blanking.

        .set    noreorder

# ---- VDP register file (KSEG1) --------------------------------------------
        .equ    VDP_BASE,       0xA4400000
        .equ    VDP_CTRL,       0x00        # bit0 enable, bit1 OSD enable
        .equ    VDP_STATUS,     0x04        # bit0 vblank, bit4 dma busy
        .equ    VDP_OSD_FB,     0x08        # OSD plane base (physical)
        .equ    VDP_OSD_STRIDE, 0x0C
        .equ    VDP_INTR_EN,    0x10        # bit0 vblank interrupt enable
        .equ    VDP_INTR_ACK,   0x14

        .equ    CTRL_ENABLE,    0x1
        .equ    CTRL_OSD_ON,    0x2

        .equ    FB0_PADDR,      0x00200000  # OSD buffers in low RDRAM
        .equ    FB1_PADDR,      0x00296000
        .equ    OSD_STRIDE,     1280        # 640 px * 2 bytes

        .data
        .align  2
        .globl  vdp_front                    # 0 -> FB0 front, 1 -> FB1 front
vdp_front:
        .word   0

        .text
        .align  2

# ----------------------------------------------------------------------------
# void vdp_init(void)
# Bring the VDP out of reset, point the OSD plane at FB0, unmask VBlank.
# Called once from the boot shell before epg_init().
# ----------------------------------------------------------------------------
        .globl  vdp_init
        .ent    vdp_init
vdp_init:
        lui     $t0, %hi(VDP_BASE)
        ori     $t0, $t0, %lo(VDP_BASE)

        sw      $zero, VDP_CTRL($t0)        # hold disabled while configuring

        li      $t1, FB0_PADDR
        sw      $t1, VDP_OSD_FB($t0)
        li      $t1, OSD_STRIDE
        sw      $t1, VDP_OSD_STRIDE($t0)

        la      $t1, vdp_front              # front buffer = FB0
        sw      $zero, 0($t1)

        li      $t1, 1
        sw      $t1, VDP_INTR_ACK($t0)      # clear any stale vblank latch
        sw      $t1, VDP_INTR_EN($t0)

        li      $t1, CTRL_ENABLE | CTRL_OSD_ON
        jr      $ra
        sw      $t1, VDP_CTRL($t0)          # (delay slot) go live
        .end    vdp_init

# ----------------------------------------------------------------------------
# uint16_t *vdp_backbuffer(void)
# Return a KSEG1 pointer to the buffer the compositor may draw into.
# ----------------------------------------------------------------------------
        .globl  vdp_backbuffer
        .ent    vdp_backbuffer
vdp_backbuffer:
        la      $t0, vdp_front
        lw      $t1, 0($t0)
        li      $v0, 0xA0000000 | FB1_PADDR # front==FB0 -> draw into FB1
        bnez    $t1, 1f
        nop
        jr      $ra
        nop
1:      li      $v0, 0xA0000000 | FB0_PADDR
        jr      $ra
        nop
        .end    vdp_backbuffer

# ----------------------------------------------------------------------------
# void vdp_vblank_isr(void)
# Runs from the IP2 exception dispatcher. Flips front/back and acks the VDP.
# The OS shell repaints via epg_render only when epg_handle_key reported a
# dirty view, so most frames this is just an ack.
# ----------------------------------------------------------------------------
        .globl  vdp_vblank_isr
        .ent    vdp_vblank_isr
vdp_vblank_isr:
        lui     $t0, %hi(VDP_BASE)
        ori     $t0, $t0, %lo(VDP_BASE)

        la      $t1, vdp_front
        lw      $t2, 0($t1)
        xori    $t2, $t2, 1                 # toggle front buffer index
        sw      $t2, 0($t1)

        li      $t3, FB0_PADDR
        beqz    $t2, 1f
        nop
        li      $t3, FB1_PADDR
1:      sw      $t3, VDP_OSD_FB($t0)        # latch new front for next scanout

        li      $t3, 1
        jr      $ra
        sw      $t3, VDP_INTR_ACK($t0)      # (delay slot) clear the latch
        .end    vdp_vblank_isr

# ----------------------------------------------------------------------------
# void vdp_fill_rect(uint16_t *fb, int fbw, int x, int y, int w, int h,
#                    uint16_t color)
# Software fill used by dgy::Surface. a0=fb a1=fbw a2=x a3=y,
# 16(sp)=w 20(sp)=h 24(sp)=color (o32).
# ----------------------------------------------------------------------------
        .globl  vdp_fill_rect
        .ent    vdp_fill_rect
vdp_fill_rect:
        lw      $t0, 16($sp)                # w
        lw      $t1, 20($sp)                # h
        lw      $t2, 24($sp)                # color
        blez    $t0, 3f
        nop
        blez    $t1, 3f
        nop

        mult    $a3, $a1                    # row offset = y * fbw
        mflo    $t3
        addu    $t3, $t3, $a2               # + x
        sll     $t3, $t3, 1                 # pixels -> bytes
        addu    $t3, $a0, $t3               # row start pointer
        sll     $t4, $a1, 1                 # stride in bytes

1:      move    $t5, $t3                    # pixel pointer
        move    $t6, $t0                    # column count
2:      sh      $t2, 0($t5)
        addiu   $t6, $t6, -1
        bnez    $t6, 2b
        addiu   $t5, $t5, 2                 # (delay slot) next pixel

        addiu   $t1, $t1, -1
        bnez    $t1, 1b
        addu    $t3, $t3, $t4               # (delay slot) next row

3:      jr      $ra
        nop
        .end    vdp_fill_rect
