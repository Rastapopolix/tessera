/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Andamento's home screen: the piece at a glance, the "shaping the flow" view. Included by and_ui.c after its state,
 * so it runs the same in the browser (web/andamento) and on the Daisy Seed; it reads only Tessera's portable core
 * (energy.c, planner.c, entries.c through their getters and state), never the FM-1's UI.
 *
 *   the header     the piece's SHAPE (or FREE: no LENGTH set), its time (left of a planned piece, played of a free
 *                  one), REST and its seconds while the notes rest; PLAY and the tempo as on every page
 *   the arc        ENERGY over time, full above the dim line, sparse below. A LENGTH set: the whole piece across the
 *                  screen, its SHAPE as energy_tick will follow it (dim), what was played over it (lit) and a play head.
 *                  FREE: from PLAY it grows from the left, a column every 1.5 s, then scrolls (the last 3 min 12 s,
 *                  now at the right edge); stopped, one turn of the tide ahead, dim. Under it a mark where the notes rested; above it a tick where Entries moved a track
 *   the 8 tracks   16 px each: the number (the selected one lit), a meter of how far the track is in (Entries' fade:
 *                  full in, empty out), lit while it sounds, blinking before Entries changes it, struck through when
 *                  muted; along the foot its ENERGY DEPTH. Tessera has NTRK tracks yet: the rest are dotted, empty
 *
 * Controls on HOME: encoders A..H are tracks 1..8's ENERGY DEPTH (pushed: back to 100); ENERGY, the keys, the arrows
 * and the transport as on every page. NO comes back here from any page; Andamento starts here. */

#define AND_TRACKS 8                             /* Andamento's tracks (Tessera has NTRK of them yet) */
#define AH_X 128                                 /* the arc: a column a sample */
#define AH_MID 22                                /* ENERGY 0 */
#define AH_AMP 11                                /* full (+1) at AH_MID - AH_AMP, sparse (-1) at AH_MID + AH_AMP */
#define AH_FREE_MS 1500u                         /* FREE: a column every 1.5 s, 128 of them 3 min 12 s (a tide turn) */
#define AH_LANE_Y 38                             /* the tracks' lanes */
#define AND_PID_DEPTH 0x26du
#define AND_PID_SHAPE 0x281u

enum { AH_SET = 1, AH_REST = 2, AH_MOVE = 4 };   /* a column's flags: sampled, resting, Entries moved a track */

static struct {
    int8_t e[AH_X];                              /* ENERGY at each column, -127 sparse .. 127 full */
    uint8_t f[AH_X];                             /* AH_* */
    uint8_t piece, playing, out;                 /* what the history was taken under; Entries' tracks out */
    uint32_t last_x, next_ms;                    /* a piece: the column last taken; FREE: the columns taken */
} and_home;

static void ah_clear(void)
{
    memset(and_home.e, 0, sizeof and_home.e);
    memset(and_home.f, 0, sizeof and_home.f);
    and_home.last_x = 0;
    and_home.next_ms = 0;
}

/* ENERGY as a row of the arc */
static int32_t ah_y(int32_t e) { return AH_MID - (e < -32767 ? -32767 : e > 32767 ? 32767 : e) * AH_AMP / 32767; }

/* the history: one sample a column. Taken when the screen is drawn (about 30 times a second here and on the Seed),
 * which is plenty for a column every 1.5 s or a piece of minutes across 128 columns */
static void and_home_sample(void)
{
    uint32_t at = 0, left = 0, piece = (uint32_t)tessera_piece_get(&at, &left), out, x, i;
    uint8_t f = AH_SET;
    if (piece != and_home.piece || (song.playing && !and_home.playing)) {   /* (a new PLAY, a LENGTH set or not) */
        ah_clear();
        and_home.piece = (uint8_t)piece;
    }
    and_home.playing = song.playing;
    if (!song.playing)
        return;
    out = tessera_entries_get(0, 0);
    if (energy_resting())
        f |= AH_REST;
    if (out != and_home.out)
        f |= AH_MOVE;
    and_home.out = (uint8_t)out;
    if (piece) {                                 /* the piece across the screen: the column where it is now */
        x = at * (AH_X - 1u) >> 16;
        if (x >= AH_X)
            x = AH_X - 1u;
        for (i = and_home.last_x + 1u; i < x; i++)   /* (a column skipped, a short piece: as the one before) */
            if (!and_home.f[i]) {
                and_home.e[i] = and_home.e[and_home.last_x];
                and_home.f[i] = and_home.f[and_home.last_x] & (uint8_t)~AH_MOVE;
            }
        and_home.e[x] = (int8_t)(energy_now() / 258);
        and_home.f[x] = (uint8_t)(f | (and_home.f[x] & (AH_REST | AH_MOVE)));
        and_home.last_x = x;
        return;
    }
    if ((int32_t)(fm1_ms - and_home.next_ms) < 0)   /* FREE: scrolling, a column every AH_FREE_MS */
        return;
    and_home.next_ms = fm1_ms + AH_FREE_MS;
    if (and_home.last_x >= AH_X) {               /* (full: the oldest column off the left) */
        memmove(and_home.e, and_home.e + 1, AH_X - 1u);
        memmove(and_home.f, and_home.f + 1, AH_X - 1u);
        and_home.last_x = AH_X - 1u;
    }
    and_home.e[and_home.last_x] = (int8_t)(energy_now() / 258);
    and_home.f[and_home.last_x++] = f;
}
/* FREE: the column now (the history grows from the left, then scrolls) */
static uint32_t ah_now_x(void) { return and_home.last_x ? and_home.last_x - 1u : 0u; }
/* FREE: the latest column takes the rests and moves since it was taken, so a short rest is not missed */
static void and_home_mark(void)
{
    if (song.playing && !and_home.piece && and_home.last_x) {
        uint32_t out = tessera_entries_get(0, 0);
        if (energy_resting())
            and_home.f[ah_now_x()] |= AH_REST;
        if (out != and_home.out)
            and_home.f[ah_now_x()] |= AH_MOVE;
        and_home.out = (uint8_t)out;
    }
}

/* a column of the curve from row y0 to y1 (both drawn) */
static void ah_span(int32_t x, int32_t y0, int32_t y1, uint32_t l)
{
    if (y0 > y1) {
        int32_t t = y0;
        y0 = y1;
        y1 = t;
    }
    and_fill(x, y0, 1, y1 - y0 + 1, l);
}

/* seconds as M:SS, or H:MM from an hour */
static void ah_time(char *b, uint32_t s)
{
    uint32_t a = s >= 3600u ? s / 3600u : s / 60u, c = s >= 3600u ? s % 3600u / 60u : s % 60u, n = 0;
    if (a >= 100u)
        b[n++] = (char)('0' + a / 100u % 10u);
    if (a >= 10u)
        b[n++] = (char)('0' + a / 10u % 10u);
    b[n++] = (char)('0' + a % 10u);
    b[n++] = ':';
    b[n++] = (char)('0' + c / 10u);
    b[n++] = (char)('0' + c % 10u);
    b[n] = 0;
}

/* the header's left part on HOME */
static void and_home_header(void)
{
    char b[16];
    uint32_t at = 0, left = 0, on = (uint32_t)tessera_piece_get(&at, &left), set = tessera.pl_length != 0u;
    const param_desc_t *d = pid_desc(AND_PID_SHAPE, 0);
    int32_t x;
    if (song.playing && energy_resting()) {      /* the notes resting: REST and its seconds, inverted in the bar */
        uint32_t s = energy.rest_left / EN_BPS + 1u, n = 0;
        b[n++] = 'R', b[n++] = 'E', b[n++] = 'S', b[n++] = 'T', b[n++] = ' ';
        if (s >= 10u)
            b[n++] = (char)('0' + s / 10u % 10u);
        b[n++] = (char)('0' + s % 10u);
        b[n++] = 'S';
        b[n] = 0;
        and_fill(0, 0, 4 + 6 * (int32_t)n, 9, 0);
        and_text(2, 1, b, 15, 8);
        return;
    }
    x = and_text(2, 1, set && d && d->names ? d->names[tessera.pl_shape - d->min] : "FREE", 0, 7);
    if (set && song.playing && !on)
        str_cpy(b, "END", sizeof b);
    else if (set) {                              /* a planned piece: the time left (stopped: its length) */
        ah_time(b + 1, left);
        b[0] = song.playing ? '-' : ' ';
    } else                                       /* free: the time played since PLAY */
        ah_time(b, song.playing ? plan.run / (uint32_t)FS : 0u);
    and_text(x + 8, 1, b[0] == ' ' ? b + 1 : b, 0, 7);
}

static void and_home_draw(void)
{
    uint32_t at = 0, left = 0, piece, i, tr, blink = (fm1_ms / 125u) & 1u, warn = 0;
    int32_t arc = tessera.en_arc, lv = tessera.en_level * 32767 / 100, py = 0, x;
    and_home_sample();
    and_home_mark();
    piece = (uint32_t)tessera_piece_get(&at, &left);
    (void)tessera_entries_get(&warn, 0);

    /* the arc: the line ENERGY swings round (0), the plan or the tide ahead dim, what was played lit */
    for (x = 0; x < AH_X; x += 2)
        and_px(x, AH_MID, 3);
    if (tessera.pl_length || !song.playing) {
        for (x = 0; x < AH_X; x++) {             /* (a LENGTH set: the SHAPE; FREE and stopped: a tide's turn) */
            int32_t e = (tessera.pl_length ? 2 * tessera_piece_shape((uint32_t)x * 65536u / (AH_X - 1u)) - 32767
                                           : en_sin((uint32_t)x, AH_X)) * arc / 100 + lv;
            int32_t y = ah_y(e);
            ah_span(x, x ? py : y, y, arc || lv ? 5 : 3);
            py = y;
        }
    }
    if (!arc && !lv && !tessera.pl_length)
        and_text(2, 12, "ARC OFF", 4, 7);
    for (i = 0, py = -1; i < AH_X; i++) {
        int32_t y;
        if (!(and_home.f[i] & AH_SET)) {
            py = -1;
            continue;
        }
        y = ah_y(and_home.e[i] * 258);
        ah_span((int32_t)i, py < 0 ? y : py, y, and_home.f[i] & AH_REST ? 7 : 15);
        py = y;
        if (and_home.f[i] & AH_REST)
            and_fill((int32_t)i, 35, 1, 2, 12);  /* (a rest: marked under the arc) */
        if (and_home.f[i] & AH_MOVE)
            and_fill((int32_t)i, 10, 1, 2, 9);   /* (Entries moved a track: a tick over it) */
    }
    if (song.playing && (piece || !tessera.pl_length)) {   /* now: the play head (a piece) and a disc on the curve */
        int32_t nx = piece ? (int32_t)(at * (AH_X - 1u) >> 16) : (int32_t)ah_now_x(), ny = ah_y(energy_now());
        if (piece)
            for (i = 10; i < 37u; i += 2)
                and_px(nx, (int32_t)i, 8);
        and_fill(nx - 1, ny - 1, 3, 3, 15);
        and_px(nx - 2, ny, 15);
        and_px(nx + 2, ny, 15);
        and_px(nx, ny - 2, 15);
        and_px(nx, ny + 2, 15);
    }

    /* the 8 tracks, 16 px each */
    for (tr = 0; tr < AND_TRACKS; tr++) {
        int32_t x0 = (int32_t)tr * 16, sel = tr == song.sel;
        char s[2] = {(char)('1' + tr), 0};
        if (tr >= NTRK) {                        /* (not yet: Tessera's tracks end at NTRK) */
            and_text(x0 + 6, AH_LANE_Y + 1, s, 4, 1);
            for (i = 0; i < 13u; i += 2) {
                and_px(x0 + 2 + (int32_t)i, AH_LANE_Y + 10, 3);
                and_px(x0 + 2 + (int32_t)i, AH_LANE_Y + 22, 3);
                and_px(x0 + 2, AH_LANE_Y + 10 + (int32_t)i, 3);
                and_px(x0 + 14, AH_LANE_Y + 10 + (int32_t)i, 3);
            }
            continue;
        }
        if (sel)
            and_fill(x0 + 1, AH_LANE_Y, 15, 9, 15);
        and_text(x0 + 6, AH_LANE_Y + 1, s, sel ? 0 : 12, 1);
        {
            uint32_t k, on = 0, muted = pid_get(0x028, tr) != 0, w = (warn >> tr & 1u) && blink;
            int32_t gain = muted ? 0 : tr < TESSERA_TRACKS ? entry_gain[tr] : 32768, h;
            int32_t depth = pid_get(AND_PID_DEPTH, tr);
            for (k = 0; k < NVOICE; k++)
                on += trk[tr].v[k].active;
            and_rect(x0 + 2, AH_LANE_Y + 10, 13, 13, w ? 13 : 4);
            h = gain * 11 / 32768;
            if (h)
                and_fill(x0 + 3, AH_LANE_Y + 22 - h, 11, h, on ? 15 : 7);
            if (muted)                           /* (muted: struck through) */
                for (i = 0; i < 13u; i++)
                    and_px(x0 + 2 + (int32_t)i, AH_LANE_Y + 22 - (int32_t)i, 11);
            and_fill(x0 + 2, AH_LANE_Y + 24, 1, 2, 3);
            and_fill(x0 + 2, AH_LANE_Y + 24, 13 * depth / 100, 2, depth ? 8 : 3);
        }
    }
}

/* encoder i on HOME: track i's ENERGY DEPTH (pushed: back to its default) */
static void and_home_enc(uint32_t i, int32_t n, int push)
{
    const param_desc_t *d;
    char b[16];
    if (i >= AND_TRACKS)
        return;
    if (i >= NTRK) {
        b[0] = 'T', b[1] = (char)('1' + i);
        str_cpy(b + 2, " NOT YET", sizeof b - 2);
        and_ui_say(b);
        return;
    }
    if (!(d = pid_desc(AND_PID_DEPTH, i)))
        return;
    pid_set(AND_PID_DEPTH, i, push ? d->def : pid_get(AND_PID_DEPTH, i) + n);
    and_ui.hot = (int8_t)i;
    and_ui.hot_pid = (uint16_t)AND_PID_DEPTH;
    and_ui.hot_ms = fm1_ms | 1u;
    and_ui.say_ms = 0;
}
