/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's 16 keys (2 rows of 8, bottom row 0..7, top row 8..15) and their RGB lights. Included by and_ui.c, after
 * its state (it reads and_ui.held), so it runs the same in the browser (web/andamento) and on the Daisy Seed. It uses
 * only the portable core (seq.c, eng_drum.c, tessera.c's land_feed.c), never the FM-1's UI.
 *
 * Four modes, FUNC + a top-row key to pick one (the keys show the choice while FUNC is held):
 *   SCALE  (FUNC + key 9)   16 scale degrees in a row from the root at C4: the bottom row first, then the top
 *   CHROM  (FUNC + key 10)  a keyboard: the bottom row white keys C..C, the top row each one's sharp (none above E, B)
 *   STEP   (FUNC + key 11)  the selected track's pattern, 16 steps at a time: a key turns its step on or off
 *   MUTE   (FUNC + key 12)  the top row mutes a track, the bottom row selects one
 * FUNC + bottom-row key: that track. FUNC + UP / DOWN: the octave (SCALE, CHROM) or the 16 steps shown (STEP).
 * A drum track's keys are its 8 lanes, the bottom row as played and the top row accented; STEP turns the lane last
 * played on or off. An external track (5..8, and_ext.c) plays its line out (MIDI, CV / gate) in track 1's scale and its
 * own OCT, the key taking over from its generator while held; it has no pattern for STEP.
 *
 * The lights, a colour per track: what the keys play, dim, the root brighter; a key flashes when the track plays its
 * note, a generator's notes included (Tessera's note feed, land_feed.c); STEP shows the steps set, the step playing in
 * white and a trail where the track played; MUTE each track's activity, red when muted. Turning ENERGY shows its level
 * across the 16 keys for a moment. and_keys.rgb holds 0..255 a channel; the board's LED driver caps the brightness. */

enum { AK_SCALE, AK_CHROM, AK_STEP, AK_MUTE, AK_MODES };
static const char *const AK_NAME[AK_MODES] = {"SCALE", "CHROM", "STEP", "MUTE"};
static const char *const AK_SHORT[AK_MODES] = {"SCL", "CHR", "STP", "MUT"};   /* (the screen's header) */
#define AK_NONE 0xFFu
#define AK_FLASH_MS 450u                             /* a played note's flash */
#define AK_TRAIL_MS 900u                             /* STEP: where the track played */
#define AK_ENERGY_MS 1200u                           /* ENERGY shown after a turn */

static struct {
    uint8_t mode, bank;
    uint8_t note[16], vel[16], trk[16];          /* what each held key started (note AK_NONE: nothing) */
    uint8_t step_note[NTRK], lane[NTRK];         /* STEP's note (the last played on the track), a drum track's lane */
    uint8_t feed;                                /* the note feed read up to here (land_ev_at's count) */
    uint32_t held;                               /* keys down */
    uint32_t flash_ms[16];                       /* when the selected track last played key k's note (0: never) */
    uint8_t flash_lv[16];                        /* .. how bright: its own note, or the note in another octave */
    uint32_t trail_ms[NTRK][NSTEP];              /* when each track last played on each step */
    uint32_t track_ms[AND_TRACKS];               /* when each track last played */
    uint32_t ext_ms[AE_N];                       /* an external track's last note seen (its note_ms) */
    uint32_t energy_ms;
    uint8_t rgb[16][3];
} and_keys;

/* the tracks' colours, Andamento's 8 */
static const uint8_t AK_TRACK_RGB[8][3] = {
    {255, 140, 16}, {24, 190, 255}, {255, 40, 150}, {110, 255, 40},
    {150, 90, 255}, {255, 70, 30}, {30, 255, 170}, {230, 220, 190},
};
static const uint8_t AK_WHITE_RGB[3] = {255, 255, 255};
/* CHROM: the semitones above C of the bottom row (white keys) and of the top row (their sharps; -1: no key) */
static const int8_t AK_SEMI[16] = {0, 2, 4, 5, 7, 9, 11, 12, 1, 3, -1, 6, 8, 10, -1, 13};
/* an engine with a key map of its own (a GM kit, slices): the FM-1's white keys, as it plays them */
static const uint8_t AK_FM1_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

/* the milliseconds since ms (0: never); a time stamped fm1_ms | 1 on an even millisecond is 1 ahead: now */
static uint32_t ak_ago(uint32_t ms) { return !ms ? 0xFFFFFFFFu : fm1_ms - ms == 0xFFFFFFFFu ? 0u : fm1_ms - ms; }
static uint32_t ak_len(const track_t *t) { return (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP); }
static int ak_step_on(const step_t *s) { return s->time == ST_NOTE && (s->n || s->hit); }

/* key k's note on track t in this mode (AK_NONE: the key is silent) */
static uint32_t ak_note(const track_t *t, uint32_t k, uint32_t mode)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    int32_t n;
    if (drum_track(t))
        return DRUM_LANE_NOTE[k % 8u];
    if (e->keys && (n = e->keys(t, AK_FM1_WHITE[k])) >= 0)
        return (uint32_t)n;
    if (mode == AK_CHROM) {
        if (AK_SEMI[k] < 0)
            return AK_NONE;
        n = 60 + AK_SEMI[k] + 12 * song.octave + t->p[P_TRANS];
        return n < 0 || n > 127 ? AK_NONE : (uint32_t)n;
    }
    n = (int32_t)scale_degree_map(t, (int32_t)k, 12 * song.octave, 1);
    return n == KB_SILENT ? AK_NONE : (uint32_t)n;
}

/* key k's note on Andamento's track tr: Tessera's own (ak_note), or an external track's: SCALE in track 1's scale,
 * CHROM, each from C4 and moved by the keys' octave and the track's OCT */
static uint32_t ak_key_note(uint32_t tr, uint32_t k, uint32_t mode)
{
    const track_t *t = &trk[0];
    int32_t n, oct;
    if (tr < NTRK)
        return ak_note(&trk[tr], k, mode);
    oct = 12 * (song.octave + and_ext.t[(tr - NTRK) % AE_N].oct);
    if (mode == AK_CHROM) {
        if (AK_SEMI[k] < 0)
            return AK_NONE;
        n = 60 + AK_SEMI[k] + oct;
        return n < 0 || n > 127 ? AK_NONE : (uint32_t)n;
    }
    n = (int32_t)scale_degree_map(t, (int32_t)k, oct, 1);
    return n == KB_SILENT || n < 0 || n > 127 ? AK_NONE : (uint32_t)n;
}

/* a key's light: 0..255 of a colour */
static void ak_set(uint32_t k, const uint8_t *c, uint32_t level)
{
    uint32_t i;
    if (level > 255u)
        level = 255u;
    for (i = 0; i < 3u; i++)
        and_keys.rgb[k][i] = (uint8_t)(c[i] * level / 255u);
}
static void ak_max(uint32_t k, const uint8_t *c, uint32_t level)   /* (brighter only) */
{
    uint32_t i;
    if (level > 255u)
        level = 255u;
    for (i = 0; i < 3u; i++) {
        uint32_t v = c[i] * level / 255u;
        if (v > and_keys.rgb[k][i])
            and_keys.rgb[k][i] = (uint8_t)v;
    }
}
static uint32_t ak_fade(uint32_t ago, uint32_t span, uint32_t top)   /* top at once, 0 after span */
{
    return ago >= span ? 0u : top * (span - ago) / span;
}

/* ------------------------------------------------------------- input --- */
/* STEP: step i of track t on or off. A synth step takes the track's last note (else the root at C4); a drum step the
 * lane last played. As ui.c's grid_hit and the FM-1's step entry, without their undo */
static void ak_step_toggle(track_t *t, uint32_t i)
{
    uint32_t tr = trk_index(t);
    step_t *s = &t->step[i % NSTEP];
    TESS_IRQ_OFF();
    if (drum_track(t)) {
        uint32_t l = and_keys.lane[tr] % NLANE, b = 1u << l, k, j = 0;
        s->flags &= (uint8_t)~SF_RECORDED;
        if (s->time != ST_NOTE) {
            memset(s, 0, sizeof *s);
            s->time = ST_NOTE;
        }
        step_to_grid(s);
        for (k = 0; k < s->n; k++)                   /* (the lane's own note becomes its hit: ui.c grid_own) */
            if (drum_lane(s->note[k]) == l)
                s->hit |= (uint8_t)b;
            else
                s->note[j++] = s->note[k];
        s->n = (uint8_t)j;
        if (s->hit & b) {
            s->hit &= (uint8_t)~b;
            s->acc &= (uint8_t)~b;
        } else {
            s->hit |= (uint8_t)b;
        }
        if (!s->n && !s->hit) {
            memset(s, 0, sizeof *s);
            s->time = ST_REST;
        }
    } else if (ak_step_on(s)) {
        uint32_t len = ak_len(t), k;
        memset(s, 0, sizeof *s);                     /* (and its TIE steps: seq_edit.c step_delete) */
        s->time = ST_REST;
        for (k = 1; k < len; k++) {
            step_t *tie = &t->step[(i + k) % len];
            if (tie->time != ST_TIE)
                break;
            memset(tie, 0, sizeof *tie);
            tie->time = ST_REST;
        }
    } else {
        memset(s, 0, sizeof *s);
        s->time = ST_NOTE;
        s->note[0] = and_keys.step_note[tr] ? and_keys.step_note[tr]
                                            : (uint8_t)scale_degree_map(t, 0, 12 * song.octave, 0);
        s->n = 1;
        s->vel = 100;
    }
    t->seq_active = 1;
    TESS_IRQ_ON();
}

/* key k down: plays (SCALE, CHROM), sets a step (STEP), mutes or selects (MUTE); FUNC held: the mode or the track */
static void and_keys_down(uint32_t k)
{
    track_t *t = TSEL;
    uint32_t n, sel = AND_SEL;
    if (k >= 16u)
        return;
    and_keys.held |= 1u << k;
    and_keys.note[k] = AK_NONE;
    if (and_ui.held[AB_FUNC]) {
        if (k < 8u) {
            and_select(k);
        } else if (k - 8u < AK_MODES) {
            and_keys.mode = (uint8_t)(k - 8u);
            and_ui_say(AK_NAME[and_keys.mode]);
        }
        return;
    }
    if (and_keys.mode == AK_MUTE) {
        if (k >= 8u)
            and_mute(k - 8u, !and_muted(k - 8u));
        else
            and_select(k);
        return;
    }
    if (sel >= NTRK) {                               /* an external track: its line out */
        if (and_keys.mode == AK_STEP) {
            and_ui_say("NO STEPS: EXT");
            return;
        }
        if ((n = ak_key_note(sel, k, and_keys.mode)) == AK_NONE)
            return;
        and_keys.note[k] = (uint8_t)n;
        and_keys.trk[k] = (uint8_t)sel;
        and_ext_key(sel - NTRK, n, 100);
        return;
    }
    if (and_keys.mode == AK_STEP) {
        uint32_t i = and_keys.bank * 16u + k;
        if (i < ak_len(t))
            ak_step_toggle(t, i);
        if (song.sel < TESSERA_TRACKS && tessera.mode[song.sel] != TESS_ENG_OFF)
            and_ui_say("SET MODE: STEP");            /* (a generator plays the track: the pattern waits) */
        return;
    }
    if ((n = ak_note(t, k, and_keys.mode)) == AK_NONE)
        return;
    and_keys.note[k] = (uint8_t)n;
    and_keys.vel[k] = (uint8_t)(drum_track(t) && k >= 8u ? 127u : 100u);
    and_keys.trk[k] = song.sel;
    if (drum_track(t))
        and_keys.lane[song.sel] = (uint8_t)(k % 8u);
    else
        and_keys.step_note[song.sel] = (uint8_t)n;
    TESS_IRQ_OFF();                                  /* (seq.c key_on, in the key's own slot: its chord, MIDI out) */
    kb_note[k] = (uint8_t)n;
    kb_trk[k] = song.sel;
    {
        uint32_t c = chord_build(t, n, kb_chord[k]), i;
        kb_chn[k] = 0;
        for (i = 0; i < c; i++) {
            uint32_t x = kb_chord[k][i];
            if (midi_local_held(t, x))
                continue;
            input_on(t, x, and_keys.vel[k]);
            midi_out_key(t, x, and_keys.vel[k]);
        }
        kb_chn[k] = (uint8_t)c;
        last_note = (uint8_t)n;
    }
    TESS_IRQ_ON();
}

/* key k up: the notes it started end (on the track they started on) */
static void and_keys_up(uint32_t k)
{
    if (k >= 16u)
        return;
    and_keys.held &= ~(1u << k);
    if (and_keys.note[k] == AK_NONE)
        return;
    if (and_keys.trk[k] >= NTRK) {                   /* (an external track's: its note off, if still its own) */
        and_ext_key(and_keys.trk[k] - NTRK, and_keys.note[k], 0);
        and_keys.note[k] = AK_NONE;
        return;
    }
    and_keys.note[k] = AK_NONE;
    TESS_IRQ_OFF();
    key_off(k, &trk[and_keys.trk[k] % NTRK]);
    TESS_IRQ_ON();
}

/* FUNC + UP / DOWN: the octave, or STEP's 16 steps */
static void and_keys_shift(int32_t d)
{
    char s[16];
    if (and_keys.mode == AK_STEP && AND_SEL >= NTRK)
        return;
    if (and_keys.mode == AK_STEP) {                  /* "STEPS 17-32" */
        uint32_t banks = (ak_len(TSEL) + 15u) / 16u, b = (and_keys.bank + (d > 0 ? 1u : banks - 1u)) % banks;
        uint32_t lo = b * 16u + 1u, hi = lo + 15u, n = 6;
        and_keys.bank = (uint8_t)b;
        memcpy(s, "STEPS ", 6);
        if (lo >= 10u)
            s[n++] = (char)('0' + lo / 10u);
        s[n++] = (char)('0' + lo % 10u);
        s[n++] = '-';
        s[n++] = (char)('0' + hi / 10u);
        s[n++] = (char)('0' + hi % 10u);
        s[n] = 0;
        and_ui_say(s);
        return;
    }
    song.octave = (int8_t)clamp(song.octave + d, -3, 3);
    memcpy(s, "OCTAVE ", 7);
    s[7] = song.octave < 0 ? '-' : '+';
    s[8] = (char)('0' + (song.octave < 0 ? -song.octave : song.octave));
    s[9] = 0;
    and_ui_say(song.octave ? s : "OCTAVE 0");
}

/* ------------------------------------------------------------ lights --- */
/* flashes on the keys that play note on track tr (none: the keys of its pitch class, dimmer) */
static void ak_flash(uint32_t tr, uint32_t note, uint32_t now)
{
    uint32_t n[16], exact = 0, mode = and_keys.mode == AK_CHROM ? AK_CHROM : AK_SCALE, k;
    for (k = 0; k < 16u; k++)
        exact |= (uint32_t)((n[k] = ak_key_note(tr, k, mode)) == note) << k;
    for (k = 0; k < 16u; k++)
        if (exact ? (exact >> k) & 1u : n[k] != AK_NONE && n[k] % 12u == note % 12u) {
            and_keys.flash_ms[k] = now;
            and_keys.flash_lv[k] = exact ? 255u : 130u;
        }
}

/* the note feed (land_feed.c: every note a track starts, a generator's too) and the external tracks' notes: flashes
 * and the trails */
static void ak_feed(void)
{
    uint8_t at = land_ev_at;
    uint32_t x;
    for (x = 0; x < AE_N; x++)
        if (and_ext.t[x].note_ms != and_keys.ext_ms[x]) {
            and_keys.ext_ms[x] = and_ext.t[x].note_ms;
            and_keys.track_ms[NTRK + x] = fm1_ms | 1u;
            if (NTRK + x == AND_SEL && !and_ext.t[x].key)
                ak_flash(NTRK + x, and_ext.t[x].note, fm1_ms | 1u);
        }
    land_open = 1;                                   /* (Andamento reads the feed always) */
    if ((uint8_t)(at - and_keys.feed) > LAND_EV)
        and_keys.feed = (uint8_t)(at - LAND_EV);
    for (; and_keys.feed != at; and_keys.feed++) {
        uint32_t i = and_keys.feed & (LAND_EV - 1u), tr = land_ev_tr[i] % NTRK, note = land_ev_note[i];
        const track_t *t = &trk[tr];
        uint32_t now = fm1_ms | 1u;
        and_keys.track_ms[tr] = now;
        and_keys.trail_ms[tr][t->seq_idx % NSTEP] = now;
        if (tr == AND_SEL)                           /* (the key that plays it; none: its pitch class, dimmer) */
            ak_flash(tr, note, now);
    }
}

static void ak_play_lights(uint32_t tr, const uint8_t *c)
{
    const track_t *t = &trk[tr < NTRK ? tr : 0u];
    uint32_t k, mask = scale_mask(t), drum = tr < NTRK && drum_track(t);
    for (k = 0; k < 16u; k++) {
        uint32_t n = ak_key_note(tr, k, and_keys.mode), level;
        if (n == AK_NONE) {
            ak_set(k, c, 0);
            continue;
        }
        if (drum)                                    /* (the lane last played brighter) */
            level = k % 8u == and_keys.lane[tr] ? 90u : k < 8u ? 34u : 18u;
        else if (and_keys.mode == AK_CHROM) {
            uint32_t pc = (n - (uint32_t)t->p[P_ROOT] + 120u) % 12u;
            level = !pc ? 90u : ((mask >> pc) & 1u) ? 34u : 6u;
        } else
            level = k % scale_count(t) ? 30u : 90u;
        ak_set(k, c, level);
        ak_max(k, c, ak_fade(ak_ago(and_keys.flash_ms[k]), AK_FLASH_MS, and_keys.flash_lv[k]));
    }
}

static void ak_step_lights(const track_t *t, const uint8_t *c)
{
    uint32_t k, tr = trk_index(t), len = ak_len(t), drum = (uint32_t)drum_track(t);
    uint32_t ph = song.playing ? t->seq_idx : 0xFFFFu, lane = 1u << (and_keys.lane[tr] % NLANE);
    if (and_keys.bank * 16u >= len)
        and_keys.bank = 0;
    for (k = 0; k < 16u; k++) {
        uint32_t i = and_keys.bank * 16u + k;
        const step_t *s = &t->step[i];
        if (i >= len) {
            ak_set(k, c, 0);
            continue;
        }
        ak_set(k, AK_WHITE_RGB, k % 4u ? 0u : 10u);  /* (the beats, faint) */
        if (drum ? (step_lanes(s) & lane) != 0u : ak_step_on(s))
            ak_set(k, c, drum ? ((step_accents(s) & lane) ? 230u : 120u) : (s->flags & SF_ACCENT) ? 230u : 120u);
        else if (s->time == ST_TIE)
            ak_set(k, c, 40);
        ak_max(k, c, ak_fade(ak_ago(and_keys.trail_ms[tr][i]), AK_TRAIL_MS, 200u));
        if (i == ph)
            ak_max(k, AK_WHITE_RGB, 150);
    }
}

static void ak_mute_lights(void)
{
    uint32_t k;
    static const uint8_t RED[3] = {255, 0, 0};
    for (k = 0; k < 16u; k++) {
        uint32_t tr = k % 8u;
        const uint8_t *c = AK_TRACK_RGB[tr];
        if (k < 8u) {
            ak_set(k, c, tr == AND_SEL ? 230u : 30u);
        } else if (and_muted(tr)) {
            ak_set(k, RED, 70);
        } else {
            ak_set(k, c, 60);
            ak_max(k, c, ak_fade(ak_ago(and_keys.track_ms[tr]), AK_FLASH_MS, 255u));
        }
    }
}

static void ak_func_lights(void)                     /* FUNC held: the tracks below, the modes above */
{
    uint32_t k;
    for (k = 0; k < 16u; k++)
        if (k < 8u)
            ak_set(k, AK_TRACK_RGB[k], k == AND_SEL ? 230u : 40u);
        else
            ak_set(k, AK_WHITE_RGB, k - 8u >= AK_MODES ? 0u : k - 8u == and_keys.mode ? 230u : 30u);
}

static void ak_energy_lights(void)                   /* ENERGY's level, -100..100, as 0..16 keys lit, cool to hot */
{
    int32_t en = pid_get(0x26c, 0);
    uint32_t k, lit = (uint32_t)((en + 100) * 16 + 100) / 200u;
    for (k = 0; k < 16u; k++) {
        uint8_t c[3];
        c[0] = (uint8_t)(40u + k * 215u / 15u);
        c[1] = (uint8_t)(k < 8u ? 90u + k * 15u : 195u - (k - 8u) * 25u);
        c[2] = (uint8_t)(k < 6u ? 255u - k * 40u : 20u);
        ak_set(k, c, k < lit ? 200u : 8u);
    }
}

/* the 16 lights now, into and_keys.rgb (run each screen frame) */
static void and_keys_lights(void)
{
    const track_t *t = TSEL;
    uint32_t sel = AND_SEL;
    const uint8_t *c = AK_TRACK_RGB[sel % 8u];
    uint32_t k;
    ak_feed();
    if (and_ui.held[AB_FUNC])
        ak_func_lights();
    else if (and_keys.energy_ms && ak_ago(and_keys.energy_ms) < AK_ENERGY_MS)
        ak_energy_lights();
    else if (and_keys.mode == AK_MUTE)
        ak_mute_lights();
    else if (and_keys.mode == AK_STEP && sel >= NTRK)   /* (an external track: no pattern) */
        memset(and_keys.rgb, 0, sizeof and_keys.rgb);
    else if (and_keys.mode == AK_STEP)
        ak_step_lights(t, c);
    else
        ak_play_lights(sel, c);
    for (k = 0; k < 16u; k++)                        /* a key held: white */
        if ((and_keys.held >> k) & 1u && !and_ui.held[AB_FUNC])
            ak_max(k, AK_WHITE_RGB, 200);
}
