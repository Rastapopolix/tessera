/* SPDX-License-Identifier: GPL-3.0-only */
/* SEQ tools (Tessera 2.1): the Pattern sheet's DOUBLE, HALF, EUCLID and SNAP TO SCALE (OCT+ held on NOTES, PATTERN, the drum grid;
 * ui_popup.c SHEET_PATTERN). The selected track's pattern playing. DOUBLE copies its LEN steps, recorded notes and
 * motion after themselves (motion.c pattern_double, after upstream Melodee 1.4.5's pattern dialog), HALF halves LEN
 * (what is past it stays, hidden). EUCLID spreads HITS over LEN as evenly as they go, turned ROTATE steps on, and
 * writes them into one lane (a DRUM track: the grid's lane) or one note row (the note at the cursor; none: the
 * pattern's first note; none: ROOT in octave 4): a hit adds the note to its step, a step without one loses it, the
 * other lanes and notes stay. SNAP TO SCALE moves a melodic track's notes onto its scale, once. Each is one level EDIT
 * held undoes (a doubled or snapped pattern with recorded notes: as a precise clear, note_tools.c). Included by ui.c */
static struct {
    uint8_t hits, rot;                                  /* EUCLID's, as last set (not saved); hits > LEN: LEN */
    uint16_t snap_pcs;                                  /* SNAP TO SCALE's pitch classes, for one snap */
} seqt = {4, 0, 0};

static void seqt_load(track_t *t, void (*edit)(track_t *t))
{
    motion_store_t m;
    motion_snapshot_track(t, &m);                       /* (a load clears the motion it keeps a copy of: kept here) */
    load_begin(t, UNDO_PAT);
    motion_replace_track(t, &m);
    fm1_irq_off();
    edit(t);
    fm1_irq_on();
    load_end(t);
    undo_seal();                                        /* (each its own level) */
    cursor_fix();
    ui.note_pick = 0;
    ui.force = 1;
}

/* ------------------------------------------------------ DOUBLE, HALF --- */
static void seqt_double_do(track_t *t) { pattern_double(t, t->pattern, 1); }
static void seqt_half_do(track_t *t) { pattern_half(t, t->pattern, 1); }
static void seqt_len(int up)
{
    track_t *t = TSEL;
    uint32_t len = step_pattern_len(t);
    int rc = up ? pattern_double(t, t->pattern, 0) : pattern_half(t, t->pattern, 0);
    ui.note_copy = ui.note_move = 0;
    if (!rc && live_rec_sel())
        rc = 1;
    if (rc) {
        ui_message(rc == 2 ? "PATTERN FULL" : rc == 3 ? (up ? "64 STEPS MAX" : "1 STEP MIN") : "STOP TO EDIT");
        return;
    }
    if (up && notes_have_recording(t)) {                /* (recorded notes: undone as a precise clear, LEN too) */
        fm1_irq_off();
        if (!note_transaction_begin_track(t)) {
            fm1_irq_on();
            return;
        }
        motion_snapshot_track(t, &note_transaction.motion);
        note_transaction.clear = 1;
        pattern_double(t, t->pattern, 1);
        note_transaction.slen = (uint8_t)len;
        note_transaction.len = (uint8_t)step_pattern_len(t);
        note_transaction_end();
        fm1_irq_on();
        undo_seal();
    } else {
        seqt_load(t, up ? seqt_double_do : seqt_half_do);
    }
    ui_message(up ? "PATTERN DOUBLED" : "PATTERN HALVED");
}
static void seqt_double(void) { seqt_len(1); }
static void seqt_half(void) { seqt_len(0); }
static void seqt_double_v(char *b)
{
    uint32_t n = 2u * step_pattern_len(TSEL);
    str_cpy(b, "to ", 4);
    if (n > NSTEP)
        str_cpy(b, "64 max", 8);
    else
        fmt_int(b + 3, (int32_t)n);
}
static void seqt_half_v(char *b)
{
    str_cpy(b, "to ", 4);
    fmt_int(b + 3, (int32_t)((step_pattern_len(TSEL) + 1u) / 2u));
}

/* ------------------------------------------------------------ EUCLID --- */
/* hit or not at step i of n: k hits as evenly spaced as they go (Bresenham's line, Bjorklund's patterns up to a
 * rotation), the first on step 1, then the whole turned r steps later */
static int euclid_hit(uint32_t i, uint32_t n, uint32_t k, uint32_t r)
{
    i = (i + n - r % n) % n;
    return k >= n || (i * k) % n < k;
}
static uint32_t seqt_hits(void)
{
    uint32_t len = step_pattern_len(TSEL);
    return seqt.hits < len ? seqt.hits : len;
}
/* the note row EUCLID writes on a melodic track */
static uint32_t seqt_note(const track_t *t)
{
    uint32_t at = notes_manual_start(t), i;
    if (at < NSTEP && t->step[at].n)
        return t->step[at].note[notes_manual_slot(&t->step[at]) % t->step[at].n];
    for (i = 0; i < step_pattern_len(t); i++)
        if (step_on(&t->step[i]) && t->step[i].n)
            return t->step[i].note[0];
    return 60u + (uint32_t)clamp(t->p[P_ROOT], 0, 11);
}
/* note x on (1) / off (0) at step i, its ties as the note keeps them; a step full (four notes): as it was */
static void seqt_note_at(track_t *t, uint32_t i, uint32_t x, int on)
{
    step_t *s = &t->step[i];
    uint32_t j, has = NSTEP;
    if (s->time == ST_NOTE)
        for (j = 0; j < s->n && j < 4u; j++)
            if (s->note[j] == x)
                has = j;
    if (on && has == NSTEP) {
        if (s->time != ST_NOTE || !step_on(s)) {
            step_clear(s);
            s->time = ST_NOTE;
        }
        if (s->n < 4u)
            s->note[s->n++] = (uint8_t)x;
    } else if (!on && has < NSTEP) {
        if (s->n == 1u && !s->hit) {
            step_delete(t, i);                          /* (and its ties) */
            return;
        }
        for (j = has; j + 1u < s->n; j++)
            s->note[j] = s->note[j + 1u];
        s->n--;
    }
}
static void seqt_euclid_do(track_t *t)
{
    uint32_t len = step_pattern_len(t), k = seqt_hits(), i, x = drum_track(t) ? 0u : seqt_note(t);
    for (i = 0; i < len; i++) {
        int on = euclid_hit(i, len, k, seqt.rot);
        if (drum_track(t))
            grid_hit(t, i, ui.lane, (uint32_t)on);
        else
            seqt_note_at(t, i, x, on);
    }
}
static void seqt_euclid(void)
{
    ui.note_copy = ui.note_move = 0;
    if (chain_busy() || live_rec_sel()) {
        ui_message("STOP TO EDIT");
        return;
    }
    if (notes_have_recording(TSEL)) {                   /* (their steps are the recorded notes' view) */
        ui_message("RECORDED NOTES");
        return;
    }
    seqt_load(TSEL, seqt_euclid_do);
    ui_message("EUCLID");
}
static void seqt_hits_v(char *b)                        /* "5 of 16 BD", "3 of 8 C4" */
{
    char n[8];
    uint32_t i;
    fmt_int(b, (int32_t)seqt_hits());
    str_cpy(b + str_len(b), " of ", 5);
    fmt_int(b + str_len(b), (int32_t)step_pattern_len(TSEL));
    i = str_len(b);
    b[i++] = ' ';
    if (drum_track(TSEL)) {
        str_cpy(b + i, drum_lane_abbr(TSEL, ui.lane), 4);
    } else {
        note_name(n, seqt_note(TSEL));
        str_cpy(b + i, n, 6);
    }
}
static void seqt_hits_turn(int32_t s)
{
    seqt.hits = (uint8_t)clamp((int32_t)seqt_hits() + s, 0, (int32_t)step_pattern_len(TSEL));
    ui.force = 1;
}
static void seqt_rot_v(char *b)
{
    uint32_t len = step_pattern_len(TSEL);
    fmt_int(b, (int32_t)(seqt.rot % len));
    str_cpy(b + str_len(b), seqt.rot % len == 1u ? " step" : " steps", 7);
}
static void seqt_rot_turn(int32_t s)
{
    int32_t len = (int32_t)step_pattern_len(TSEL);
    seqt.rot = (uint8_t)((((int32_t)(seqt.rot % (uint32_t)len) + s) % len + len) % len);
    ui.force = 1;
}

/* ----------------------------------------------------- SNAP TO SCALE --- */
/* a track whose keys play notes: not a kit's or slices' (the sheet has no row there; seq.c kb_map) */
static int seqt_melodic(const track_t *t)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    return !drum_track(t) && !(e->keys && e->keys(t, 0) >= 0);
}
/* SCL from ROOT as pitch classes; QNT CHRD with a CHORD track playing: its chord's */
static uint32_t seqt_snap_scale(const track_t *t)
{
    uint32_t mask = scale_mask(t), pcs = 0, i;
    if (t->p[P_QUANT] == Q_CHRD && tessera_chord_pcs())
        return tessera_chord_pcs();
    for (i = 0; i < 12u; i++)
        if ((mask >> i) & 1u)
            pcs |= 1u << (((int32_t)i + t->p[P_ROOT] + 120) % 12);
    return pcs;
}
/* A note as written is the note as it sounds (TRN moved it on its way in, seq.c kb_map; seq_step adds nothing), so it
 * snaps as it is: to the nearest note, a tie going down. The live QNT rounds down (tessera_snap_note), but a written
 * note moved once should move as little as it can */
static uint32_t seqt_snap_note(uint32_t note) { return tessera_chord_snap(seqt.snap_pcs, (int32_t)note); }
/* the notes of the pattern, all of it (what lies hidden past LEN comes back on the scale); *off: those it would move */
static uint32_t seqt_snap_scan(const track_t *t, uint32_t *off)
{
    uint32_t all = 0, i, k;
    *off = 0;
    for (i = 0; i < NSTEP; i++)
        if (!(t->step[i].flags & SF_RECORDED) && step_on(&t->step[i]))
            for (k = 0; k < t->step[i].n && k < 4u; k++, all++)
                *off += seqt_snap_note(t->step[i].note[k]) != t->step[i].note[k];
    for (i = recording_head[recording_owner(t)]; i < RECORD_MAX; i = recording_next[i], all++)
        *off += seqt_snap_note(recording[i].note) != recording[i].note;
    return all;
}
static void seqt_snap_do(track_t *t)
{
    uint32_t len = step_pattern_len(t), i, j, k, n;
    for (i = 0; i < NSTEP; i++) {
        step_t *s = &t->step[i];
        if (s->time != ST_NOTE)
            continue;
        for (k = n = 0; k < s->n && k < 4u; k++) {      /* (two notes of a chord on one: one note) */
            uint8_t x = (uint8_t)seqt_snap_note(s->note[k]);
            for (j = 0; j < n && s->note[j] != x; j++)
                ;
            if (j == n)
                s->note[n++] = x;
        }
        s->n = (uint8_t)n;
    }
    for (i = recording_head[recording_owner(t)]; i < RECORD_MAX; i = recording_next[i])
        recording[i].note = (uint8_t)seqt_snap_note(recording[i].note);
    recording_reindex();
    for (i = 0; i < len; i++)                           /* (a recorded step shows its notes) */
        if (t->step[i].flags & SF_RECORDED)
            notes_rebuild(t, i);
}
static void seqt_snap(void)
{
    track_t *t = TSEL;
    uint32_t off, all;
    char b[8];
    ui.note_copy = ui.note_move = 0;
    if (chain_busy() || live_rec_sel()) {
        ui_message("STOP TO EDIT");
        return;
    }
    seqt.snap_pcs = (uint16_t)seqt_snap_scale(t);
    all = seqt_snap_scan(t, &off);
    if (!all) {
        ui_message("NOTHING TO SNAP");
        return;
    }
    if (micro_active(t) || !off) {                      /* (a microtonal scale's notes are its degrees: microtonal.h) */
        ui_message("ALL IN SCALE");
        return;
    }
    if (recording_head[recording_owner(t)] < RECORD_MAX) {   /* (recorded notes: undone as a precise clear) */
        fm1_irq_off();
        recording_stop(t);                              /* (what sounds lets go under the note it started as) */
        if (!note_transaction_begin_track(t)) {
            fm1_irq_on();
            return;
        }
        motion_snapshot_track(t, &note_transaction.motion);
        note_transaction.clear = 1;
        seqt_snap_do(t);
        note_transaction_end();
        fm1_irq_on();
        undo_seal();
        ui.note_pick = 0;
    } else {
        seqt_load(t, seqt_snap_do);
    }
    fmt_int(b, (int32_t)off);
    ui_say("SNAPPED ", b);
}
