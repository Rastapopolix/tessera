/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio): MIDI LEARN, upstream Melodee 7bd7e4f
 * Copyright (C) 2026 Theo Dainis (melodee-tess): its Tessera form */
/* MIDI LEARN: a CC onto one knob's parameter (its central id, pid.h) of one track, whatever channel the CC comes on and
 * whatever page is shown. The MIDI handler sets it as the CC map does (midi_cc.c midi_pid: MOTION records it, cords add
 * on top); a learned CC no longer does what the fixed map gave it, MIDI CC IN OFF included (it was chosen). While
 * learning (MENU > MIDI LEARN) every CC only marks itself (mlearn_seen) for ui_learn.c. Up to MLEARN_N; kept until
 * power-off, not saved. Included by seq.c after midi_cc.c. */
#define MLEARN_N 16u
typedef struct { uint8_t cc, track; uint16_t pid; } mlearn_t;   /* cc 0: free */
static mlearn_t mlearn[MLEARN_N];
static volatile uint8_t mlearn_on;                       /* learning (the menu's MIDI LEARN until the menu again) */
static volatile int16_t mlearn_seen = -1;                /* learning: the last CC sent */
static int mlearn_ok(uint32_t cc)                        /* not the CCs that play (wheel, data entry, pedal, RPN ..) */
{
    return cc != 0u && cc != 1u && cc != 6u && cc != 11u && cc != 32u && cc != 38u && cc != 64u &&
           (cc < 96u || cc > 101u) && cc < 120u;
}
/* midi_control.c: CC cc = value from any channel; 1 = taken */
static int mlearn_receive(uint32_t cc, uint32_t value)
{
    uint32_t k;
    if (!mlearn_ok(cc))
        return 0;
    if (mlearn_on) {
        mlearn_seen = (int16_t)cc;
        return 1;
    }
    for (k = 0; k < MLEARN_N; k++)
        if (mlearn[k].cc == cc) {
            midi_pid(&trk[mlearn[k].track % NTRK], mlearn[k].pid, value, 127u);
            return 1;
        }
    return 0;
}
static uint32_t mlearn_count(void)
{
    uint32_t k, n = 0;
    for (k = 0; k < MLEARN_N; k++)
        n += mlearn[k].cc != 0u;
    return n;
}
