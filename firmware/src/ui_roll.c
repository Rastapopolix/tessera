/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* NOTES under a mouse (TESS_HAS_POINTER: the plugin): the steps as a piano roll. mo_pic_notes draws a step a column
 * and a note's height its pitch (mo_note_h over the page's c0 and span, kept in uz_roll) and puts one zone, UZ_ROLL,
 * over them; the core (tess_core.c, TESS_ZONE_ROLL) hands a click's place on the screen to roll_do, which turns it back
 * into a step and a pitch and edits the step as the keys would:
 *   - ROLL_ADD: a note on that step at that pitch (a step with notes: one more, a chord of up to 4; a MONO voice: the
 *     note replaced), in key when the track quantises; a click on one of the step's notes picks it;
 *   - ROLL_DEL: the note under the mouse removed (a chord's tick: that note; the last one: the step empty);
 *   - ROLL_GRAB, then ROLL_MOVE n: the note under the mouse n rows up (- down), in key when the track quantises, over
 *     the chord's other notes rather than onto them; ROLL_MOVE_ALL: the whole chord, keeping its shape, stopping
 *     whole at the range's edge.
 * The same rules as the keys: STOP TO EDIT while a chain plays or the track records, a recorded step's notes are the
 * knobs' (SELECT A NOTE). One step's note list changes, as the firmware stores it: no change to the pattern or the
 * save format. Every edit is one undo step (seq_undo.c). Included by core_ui_sources.h and melodee.c after
 * ui_input.c; without TESS_HAS_POINTER nothing: the FM-1's image is unchanged */
#if TESS_HAS_POINTER
enum { ROLL_ADD = 1, ROLL_DEL, ROLL_GRAB, ROLL_MOVE, ROLL_MOVE_ALL };
enum { ROLL_BASE = 92 };                                /* (mo_pic_notes' base: a bar's foot, in cells) */
static struct { int16_t step; int8_t slot; } roll_grab = {-1, 0};

/* the column under x (cells): the page's step i (0..15), -1 none */
static int32_t roll_col(int32_t cx)
{
    int32_t i;
    for (i = 0; i < 16; i++) {
        int32_t x = 3 + i * 7 + i / 4 * 2;
        if (cx >= x - 1 && cx <= x + 5)
            return i;
    }
    return -1;
}
/* the pitch a bar reaching row cy would have on step st (as drawn: mo_note_h over c0 and span); one pitch or none on
 * the page (span 127, or no note): around that pitch (or the last note entered), a semitone every 2 cells */
static int32_t roll_pitch(const step_t *st, int32_t cy)
{
    int32_t h = ROLL_BASE - cy, x;
    if (step_on(st) && st->n && mo_note_h((int32_t)st->note[0] - uz_roll.c0, uz_roll.span) == h)
        return st->note[0];                             /* (the step's bar, clicked at its top: its note) */
    if (uz_roll.span == 127 || uz_roll.lo > uz_roll.hi) {
        int32_t p = uz_roll.lo <= uz_roll.hi ? uz_roll.lo : (int32_t)last_note;
        int32_t h0 = uz_roll.lo <= uz_roll.hi ? mo_note_h(p, 127) : 14;
        return clamp(p + (h - h0) / 2, 1, 127);
    }
    for (x = 0; uz_roll.c0 + x < 127 && mo_note_h(x, uz_roll.span) < h; x++)
        ;
    return clamp(uz_roll.c0 + x, 1, 127);
}
/* the note of step st under row cy: 0 its bar, k a chord's k-th tick above it; -1 none (above them all) */
static int32_t roll_slot(const step_t *st, int32_t cy)
{
    int32_t top, k;
    if (!step_on(st) || !st->n)
        return -1;
    top = ROLL_BASE - mo_note_h((int32_t)st->note[0] - uz_roll.c0, uz_roll.span);
    if (cy >= top - 1)
        return 0;
    k = (top - cy + 1) / 2;
    return k < (int32_t)st->n && k < 4 ? k : -1;
}
static int roll_has(const step_t *st, int32_t n, int32_t skip)
{
    int32_t k;
    for (k = 0; k < (int32_t)st->n; k++)
        if (k != skip && st->note[k] == n)
            return 1;
    return 0;
}
/* pitch n moved one row (dir), in key when the track quantises (as KNOB 2 turns a note); 0: off the range */
static int32_t roll_row(int32_t n, int32_t dir)
{
    int32_t m = tessera_pitch_turn(TSEL, n, dir, 0);
    return m < 1 || m > 127 ? 0 : m;
}

/* what the mouse did at (x, y), screen pixels, over NOTES' roll: op ROLL_*, n rows for a move (+ up) */
static void roll_do(uint32_t op, int32_t x, int32_t y, int32_t n)
{
    track_t *t = TSEL;
    int32_t col, si, cy = y / 2, j, k, heard = 0;
    step_t *st;
    if (ui.home || cur_page()->graph != GR_ROLL || grid_on() || ui.menu || ui.confirm || pop.on)
        return;
    col = roll_col(x / 2);
    si = col < 0 ? -1 : uz_roll.first + col;
    if (op == ROLL_MOVE || op == ROLL_MOVE_ALL) {       /* (a drag: the note grabbed where it began) */
        if (roll_grab.step < 0)
            return;
        si = roll_grab.step;
    }
    if (si < 0 || si >= uz_roll.len || si >= (int32_t)NSTEP)
        return;
    if (op == ROLL_GRAB) {                              /* (nothing changes yet) */
        uint32_t at = step_note_start(t, (uint32_t)si);
        roll_grab.step = -1;
        if (at < NSTEP && at != (uint32_t)si)           /* (a tie: its note's) */
            roll_grab.step = (int16_t)at, roll_grab.slot = 0;
        else if ((k = roll_slot(&t->step[si], cy)) >= 0)
            roll_grab.step = (int16_t)si, roll_grab.slot = (int8_t)k;
        return;
    }
    if (chain_busy() || live_rec_sel()) {
        ui_message("STOP TO EDIT");
        return;
    }
    st = &t->step[si];
    if (st->flags & SF_RECORDED) {                      /* (a recorded note: the knobs' to edit) */
        cursor_set(si);
        ui_message("SELECT A NOTE");
        return;
    }
    step_history_sync();                                /* (one undo step: seq_undo.c) */
    fm1_irq_off();
    switch (op) {
    case ROLL_ADD:
        if (!step_on(st) && st->time == ST_TIE && step_note_start(t, (uint32_t)si) < NSTEP) {
            fm1_irq_on();                               /* (a tie: its note picked) */
            cursor_set((int32_t)step_note_start(t, (uint32_t)si));
            return;
        }
        {
            int32_t p = roll_pitch(st, cy), q = tessera_pitch_turn(t, p - 1, 1, 0);   /* (in key: the scale's next) */
            p = q >= 1 && q <= 127 ? q : p;
            if (!step_on(st) || !st->n || (t->p[P_VOICE] && !ENGINES[t->engine]->oneshot)) {
                if (!step_on(st)) step_clear(st);
                st->note[0] = (uint8_t)p, st->n = 1, st->time = ST_NOTE;
                j = 0;
            } else {
                for (j = 0; j < (int32_t)st->n && st->note[j] != p; j++)
                    ;
                if (j == (int32_t)st->n) {
                    if (st->n >= 4u) {
                        fm1_irq_on();
                        ui_message("CHORD FULL");
                        return;
                    }
                    st->note[st->n++] = (uint8_t)p;
                }
            }
            last_note = (uint8_t)p;
            heard = 1;
        }
        break;
    case ROLL_DEL:
        if (!step_on(st)) {                             /* (a tie: its whole note, as EDIT) */
            if (step_note_start(t, (uint32_t)si) < NSTEP) step_delete(t, (uint32_t)si);
            j = -1;
            break;
        }
        if ((j = roll_slot(st, cy)) < 0) {
            fm1_irq_on();
            return;
        }
        if (st->n > 1u) {
            for (k = j; k + 1 < (int32_t)st->n; k++) st->note[k] = st->note[k + 1];
            st->n--;
        } else {
            step_delete(t, (uint32_t)si);
        }
        j = -1;
        break;
    default:                                            /* ROLL_MOVE, ROLL_MOVE_ALL */
        j = roll_grab.slot < (int32_t)st->n ? roll_grab.slot : 0;
        if (!step_on(st) || !st->n || !n) {
            fm1_irq_on();
            return;
        }
        for (k = 0; k < (n < 0 ? -n : n) && k < 64; k++) {
            int32_t dir = n < 0 ? -1 : 1, i;
            if (op == ROLL_MOVE_ALL) {                  /* the chord whole: every note a row, or none */
                uint8_t to[4];
                for (i = 0; i < (int32_t)st->n && i < 4; i++)
                    if (!(to[i] = (uint8_t)roll_row(st->note[i], dir)))
                        break;
                if (i < (int32_t)st->n)
                    break;
                for (i = 0; i < (int32_t)st->n; i++) st->note[i] = to[i];
            } else {                                    /* one note, over its chord's others */
                int32_t q = roll_row(st->note[j], dir);
                while (q && roll_has(st, q, j))
                    q = roll_row(q, dir);
                if (!q)
                    break;
                st->note[j] = (uint8_t)q;
            }
            heard = 1;
        }
        last_note = st->note[j];
        break;
    }
    fm1_irq_on();
    step_history_end();
    if (op != ROLL_DEL) {
        cursor_set(op == ROLL_ADD ? si : roll_grab.step);
        ui.note_slot = (uint8_t)(j > 0 ? j : 0);
    }
    ui.force = 1;
    if (heard && settings_preview) notes_preview_start(1);
}
#endif
