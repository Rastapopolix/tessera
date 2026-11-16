/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include "scales.h"
#include "tune.h"
#include "uscale.h"
/* the USER scale playing (uscale.h), 0: none (SCL's scale, or TUNING's tuning) */
static uint32_t uscale_on(void) { return uscale_sel && !tune_scale() ? uscale_mask[(uscale_sel - 1u) % USCALE_SLOTS] : 0u; }

/* Microtonal notes use the existing 7-bit note identity as a scale-degree address:
 * 60 is degree zero at C4 + ROOT + TRN. This keeps independent releases, chords,
 * recordings and projects intact even when two degrees lie within one semitone.
 * Pitch tables are hundredths of a cent; their last entry is the repeat period. */
static const micro_scale_t *micro_scale(const track_t *t)
{
    int32_t s = t->p[P_SCALE];
    if (tune_scale()) return tune_scale();          /* (TUNE: a user tuning stands for SCL, tune.h) */
    if (uscale_on()) return 0;                      /* (a USER scale: 12 notes, uscale.h) */
    return s >= (int32_t)SCALE_LEGACY && s < (int32_t)SCALE_TOTAL ? &MICRO_SCALE[s - SCALE_LEGACY] : 0;
}
/* the key a degree tunes on a piano: the semitone nearest it */
#define MICRO_KEY(p) (((p) + 5000) / 10000 % 12)
/* QNT OFF on a scale of up to 12 degrees with an octave period, each degree near a key of its own (the maqamat, the
 * just and meantone scales, the Japanese pentatonics): the keys stay a piano's, every key its own note, and a key
 * nearest a degree takes the degree's tuning (RAST on C: E and B half flat), the others 12-TET. Its notes are
 * semitones, not degree numbers: micro_active is 0 and only the voice's pitch (micro_chrom_pitch) is tuned */
static int micro_chrom(const track_t *t)
{
    const micro_scale_t *s;
    uint32_t i, mask = 0, n = 0;
    if (t->p[P_QUANT] != Q_OFF || t->eng_req == 10u || !(s = micro_scale(t)) || s->count > 12u || s->pitch[s->count] != 120000)
        return 0;
    for (i = 0; i < s->count; i++) {
        n += !(mask >> MICRO_KEY(s->pitch[i]) & 1u);
        mask |= 1u << MICRO_KEY(s->pitch[i]);
    }
    return n == s->count;
}
/* a microtonal scale playing its degrees as the notes: QNT on; QNT OFF only where the keys cannot stay a piano's
 * (more than 12 degrees, two near one key, a period other than the octave: OFF plays every degree in a row, as ALL) */
static int micro_active(const track_t *t)
{
    return t->eng_req != 10u && micro_scale(t) != 0 && !micro_chrom(t);
}
/* micro_chrom: note's pitch (1/10000 semitone): the degree's on a key nearest one, else the key's own */
static int32_t micro_chrom_pitch(const track_t *t, uint32_t note)
{
    const micro_scale_t *s = micro_scale(t);
    int32_t pc = ((int32_t)note - t->p[P_ROOT] - t->p[P_TRANS]) % 12, i;
    pc += pc < 0 ? 12 : 0;
    for (i = 0; i < (int32_t)s->count; i++)
        if (MICRO_KEY(s->pitch[i]) == pc) return (int32_t)note * 10000 + s->pitch[i] - pc * 10000;
    return (int32_t)note * 10000;
}
static int32_t micro_floor(int32_t n, int32_t d)
{
    int32_t q = n / d;
    return q - (n % d < 0);
}
static int32_t micro_pitch(const track_t *t, int32_t note)
{
    const micro_scale_t *s = micro_scale(t);
    int32_t d = note - 60, cycle = micro_floor(d, s->count);
    return (60 + t->p[P_ROOT] + t->p[P_TRANS]) * 10000 +
           cycle * s->pitch[s->count] + s->pitch[d - cycle * s->count];
}
static uint32_t micro_degree_map(const track_t *t, int32_t degree, int32_t offset, int strict)
{
    const micro_scale_t *s = micro_scale(t);
    int32_t note = 60 + degree + offset / 12 * s->count;
    int32_t pitch = micro_pitch(t, note);
    if (note < 0 || note > 127 || (strict && (pitch < 0 || pitch > 1270000)))
        return 255u;
    return (uint32_t)note;
}
static uint32_t micro_snap(const track_t *t, int32_t pitch)
{
    const micro_scale_t *s = micro_scale(t);
    int32_t relative = (pitch - 60 - t->p[P_ROOT] - t->p[P_TRANS]) * 10000;
    int32_t cycle = micro_floor(relative, s->pitch[s->count]), degree = 0;
    relative -= cycle * s->pitch[s->count];
    while (degree + 1 < s->count && s->pitch[degree + 1] <= relative)
        degree++;
    if (s->pitch[degree + 1] - relative < relative - s->pitch[degree])
        degree++;                                       /* (the nearest degree, a tie the lower: upstream 18df862) */
    return micro_degree_map(t, cycle * s->count + degree, 0, 1);
}
static uint32_t scale_note_period(const track_t *t)
{
    return micro_active(t) ? micro_scale(t)->count : 12u;
}
