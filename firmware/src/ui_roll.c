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
 *   - ROLL_PAINT after ROLL_GRAB (a drag across the steps): each column the mouse crosses painted, a note at the mouse's
 *     pitch on an empty step if the drag began off a note, else erased (the note gone with its ties); ROLL_LEN n (Alt
 *     and a drag across): the grabbed note n steps longer (- shorter), over empty steps only; ROLL_VEL n (Alt and a drag
 *     up or down): its velocity 4 a row; ROLL_DROP (let go): the stroke one undo step.
 * A DRUM track's NOTES shows the grid (mo_grid_panel): lane_do does the same there, a hit for a note: a click toggles it,
 * a right-click clears it, a drag across a lane paints or erases its hits, a drag up or down on a hit (or the wheel)
 * accents it or not (a hit's only dynamics in the grid).
 * The same rules as the keys: STOP TO EDIT while a chain plays or the track records, a recorded step's notes are the
 * knobs' (SELECT A NOTE). One step's note list changes, as the firmware stores it: no change to the pattern or the
 * save format. Every edit is one undo step (seq_undo.c). Included by core_ui_sources.h and melodee.c after
 * ui_input.c; without TESS_HAS_POINTER nothing: the FM-1's image is unchanged */
#if TESS_HAS_POINTER
enum { ROLL_ADD = 1, ROLL_DEL, ROLL_GRAB, ROLL_MOVE, ROLL_MOVE_ALL, ROLL_PAINT, ROLL_LEN, ROLL_VEL, ROLL_DROP };
enum { ROLL_BASE = 92 };                                /* (mo_pic_notes' base: a bar's foot, in cells) */
static struct { int16_t step; int8_t slot, col, on; } roll_grab = {-1, 0, -1, 0};   /* (col: where the press was, on: a
                                                         * note or hit there) */
static struct { int8_t mode, col; } roll_paint;         /* a stroke: mode 0 none yet, 1 drawing, 2 erasing; the column
                                                         * painted last */

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
/* the note of step st under row cy: k a chord's k-th note, drawn at its own pitch (mo_pic_notes), within a row of its
 * line; else 0 its bar (the first note); -1 none (above them all) */
static int32_t roll_slot(const step_t *st, int32_t cy)
{
    int32_t top, k, best = -1, bd = 2;
    if (!step_on(st) || !st->n)
        return -1;
    for (k = 1; k < (int32_t)st->n && k < 4; k++) {
        int32_t d = cy - (ROLL_BASE - mo_note_h((int32_t)st->note[k] - uz_roll.c0, uz_roll.span));
        d = d < 0 ? -d : d;
        if (d < bd)
            bd = d, best = k;
    }
    if (best >= 0)
        return best;
    top = ROLL_BASE - mo_note_h((int32_t)st->note[0] - uz_roll.c0, uz_roll.span);
    return cy >= top - 1 ? 0 : -1;
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

/* one column of NOTES' roll painted (step uz_roll.first + c): drawing, a note at the pitch of row cy on an empty step,
 * in key when the track quantises; erasing, the note there gone with its ties. A recorded note is the knobs' */
static void roll_cell(track_t *t, int32_t c, int32_t cy)
{
    int32_t si = uz_roll.first + c, p, q;
    step_t *st;
    if (si < 0 || si >= uz_roll.len || si >= (int32_t)NSTEP)
        return;
    st = &t->step[si];
    if (st->flags & SF_RECORDED)
        return;
    if (roll_paint.mode == 2) {
        if (step_on(st) || (st->time == ST_TIE && step_note_start(t, (uint32_t)si) < NSTEP))
            step_delete(t, (uint32_t)si);
        return;
    }
    if (step_on(st) || st->time == ST_TIE)
        return;
    p = roll_pitch(st, cy), q = tessera_pitch_turn(t, p - 1, 1, 0);
    p = q >= 1 && q <= 127 ? q : p;
    step_clear(st);
    st->note[0] = (uint8_t)p, st->n = 1, st->time = ST_NOTE;
    last_note = (uint8_t)p;
}
/* a stroke on to column col (0..15): every column from the last one painted (the first time: the press's, drawing if it
 * was off a note, else erasing) to col, each by cell; the whole stroke one undo step (seq_undo.c roll_stroke) */
static void roll_stroke_to(track_t *t, int32_t col, int32_t cy, void (*cell)(track_t *, int32_t, int32_t))
{
    int32_t was = roll_paint.col;
    step_history_sync();
    fm1_irq_off();
    if (!roll_paint.mode) {
        roll_paint.mode = (int8_t)(roll_grab.on ? 2 : 1), roll_paint.col = roll_grab.col;
        roll_stroke = 1;
        cell(t, roll_paint.col, cy);
    }
    while (roll_paint.col != col) {
        roll_paint.col = (int8_t)(roll_paint.col + (col > roll_paint.col ? 1 : -1));
        cell(t, roll_paint.col, cy);
    }
    fm1_irq_on();
    step_history_end();
    if (was != roll_paint.col || roll_paint.col == roll_grab.col) {
        ui.force = 1;
        if (roll_paint.mode == 1 && cell == roll_cell && settings_preview) notes_preview_start(1);
    }
}
/* the mouse let go after a drag: a stroke's edits kept as one undo step */
static void roll_drop(void)
{
    roll_paint.mode = 0;
    if (roll_stroke) {
        roll_stroke = 0;
        step_history_end();
    }
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
    if (op == ROLL_GRAB)
        roll_grab.step = -1, roll_grab.col = (int8_t)col, roll_grab.on = 0, roll_paint.mode = 0;
    if (op == ROLL_PAINT) {                             /* (a stroke: on from the press's column) */
        if (col >= 0 && roll_grab.col >= 0 && !(chain_busy() || live_rec_sel()))
            roll_stroke_to(t, col, cy, roll_cell);
        return;
    }
    if (op == ROLL_MOVE || op == ROLL_MOVE_ALL || op == ROLL_LEN || op == ROLL_VEL) {   /* (a drag: the note grabbed
                                                         * where it began) */
        if (roll_grab.step < 0)
            return;
        si = roll_grab.step;
    }
    if (si < 0 || si >= uz_roll.len || si >= (int32_t)NSTEP)
        return;
    if (op == ROLL_GRAB) {                              /* (nothing changes yet) */
        uint32_t at = step_note_start(t, (uint32_t)si);
        if (at < NSTEP && at != (uint32_t)si)           /* (a tie: its note's) */
            roll_grab.step = (int16_t)at, roll_grab.slot = 0;
        else if ((k = roll_slot(&t->step[si], cy)) >= 0)
            roll_grab.step = (int16_t)si, roll_grab.slot = (int8_t)k;
        roll_grab.on = roll_grab.step >= 0;
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
            if (step_on(st) && st->n > 1u && (q = roll_slot(st, cy)) > 0)
                p = st->note[q];                        /* (on a chord's note, as drawn: that one picked) */
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
    case ROLL_LEN:                                      /* (Alt and a drag across: its length) */
        j = -1;
        if (step_on(st) && n) {
            step_note_resize(t, (uint32_t)si, (int32_t)step_note_length(t, (uint32_t)si) + n);
            roll_stroke = 1;
        }
        break;
    case ROLL_VEL:                                      /* (Alt and a drag up or down: its velocity, as VEL shows it) */
        j = -1;
        if (step_on(st) && n) {
            st->vel = (uint8_t)clamp((st->flags & SF_ACCENT ? 127 : st->vel ? st->vel : 96) + 4 * n, 1, 127);
            st->flags &= (uint8_t)~SF_ACCENT;
            roll_stroke = 1;
        }
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
    if (op == ROLL_LEN || op == ROLL_VEL) {
        cursor_set(si);
    } else if (op != ROLL_DEL) {
        cursor_set(op == ROLL_ADD ? si : roll_grab.step);
        ui.note_slot = (uint8_t)(j > 0 ? j : 0);
    }
    ui.force = 1;
    if (heard && settings_preview) notes_preview_start(1);
}

/* a DRUM track's NOTES, the drum grid (mo_grid_panel: lane l at row LANE_Y + 10 l; step i's cell at 18 + 6 i + i / 4):
 * the column under cx (0..15), -1 none */
enum { LANE_Y = PV_GRID_Y / 2 + 4, LANE_DY = 10 };
static int32_t lane_col(int32_t cx)
{
    int32_t i;
    for (i = 0; i < 16; i++) {
        int32_t x = 18 + i * 6 + i / 4;
        if (cx >= x - 1 && cx <= x + 5)
            return i;
    }
    return -1;
}
static int8_t lane_now;                                 /* (the lane a stroke paints) */
static void lane_cell(track_t *t, int32_t c, int32_t cy)
{
    uint32_t si = ui.bank * 16u + (uint32_t)c;
    (void)cy;
    if (si < (uint32_t)t->p[P_SLEN] && si < NSTEP && !(t->step[si].flags & SF_RECORDED))
        grid_hit(t, si, (uint32_t)lane_now, roll_paint.mode == 1);
}
/* what the mouse did over the grid: op ROLL_* as on the roll, a hit for a note */
static void lane_do(uint32_t op, int32_t x, int32_t y, int32_t n)
{
    track_t *t = TSEL;
    int32_t cx = x / 2, cy = y / 2, col = lane_col(cx), l = cy >= LANE_Y ? (cy - LANE_Y) / LANE_DY : -1, si;
    if (ui.home || cur_page()->graph != GR_ROLL || cur_page()->scope != SC_STEP || !grid_on() || ui.menu || ui.confirm ||
        pop.on)
        return;
    si = col < 0 ? -1 : (int32_t)ui.bank * 16 + col;
    if (si >= t->p[P_SLEN] || si >= (int32_t)NSTEP || l < 0 || l >= NLANE)
        si = -1;
    if (op == ROLL_GRAB) {
        roll_grab.step = (int16_t)si, roll_grab.col = (int8_t)(si < 0 ? -1 : col), roll_paint.mode = 0;
        roll_grab.on = (int8_t)(si >= 0 && step_lanes(&t->step[si]) >> l & 1u);
        lane_now = (int8_t)l;
        return;
    }
    if (chain_busy() || live_rec_sel()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (op == ROLL_PAINT) {
        if (col >= 0 && roll_grab.col >= 0)
            roll_stroke_to(t, col, cy, lane_cell);
        return;
    }
    if (op == ROLL_MOVE || op == ROLL_MOVE_ALL || op == ROLL_VEL)   /* (up or down on the hit pressed: its accent) */
        si = roll_grab.on ? roll_grab.step : -1, l = lane_now;
    if (si < 0 || (op != ROLL_ADD && op != ROLL_DEL && op != ROLL_MOVE && op != ROLL_MOVE_ALL && op != ROLL_VEL))
        return;
    if (t->step[si].flags & SF_RECORDED) {
        cursor_set(si);
        ui_message("SELECT A NOTE");
        return;
    }
    step_history_sync();
    fm1_irq_off();
    if (op == ROLL_ADD) grid_hit(t, (uint32_t)si, (uint32_t)l, 2u);
    else if (op == ROLL_DEL) grid_hit(t, (uint32_t)si, (uint32_t)l, 0u);
    else if (n) grid_acc(t, (uint32_t)si, (uint32_t)l, n > 0), roll_stroke = 1;
    fm1_irq_on();
    step_history_end();
    cursor_set(si);
    ui.lane = (uint8_t)l;
    ui.force = 1;
    if (op != ROLL_DEL && settings_preview) notes_preview_start(1);
}
#endif
