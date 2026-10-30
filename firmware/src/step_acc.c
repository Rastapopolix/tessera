/* SPDX-License-Identifier: GPL-3.0-only */
/* The pitch accumulator (Tessera, as the OXI One's): a STEP track's step with an Accumulate amount (step_climb,
 * -7..+7 scale degrees) plays its notes that many degrees further each pass of the pattern (t->seq_pass), up to the
 * track's LIMIT in degrees from the notes as written; there it WRAPs back to them, turns round (PING) or stays
 * (CLAMP). PLAY and a pattern change start it again. Every note of the step moves, in the track's scale (ROOT / SCL;
 * QNT OFF: semitones); a TIE holds what played before it; the drum hits and a recorded step's notes stay.
 * Included by seq.c */

/* the degrees a step of amount climb has moved by on pass `pass` of track tr */
static int32_t acc_offset(uint32_t tr, int32_t climb, uint32_t pass)
{
    uint32_t a = (uint32_t)(climb < 0 ? -climb : climb), lim, x;
    if (!climb || !pass || tr >= TESSERA_TRACKS)
        return 0;
    lim = tessera.acc_limit[tr] ? tessera.acc_limit[tr] : TESS_ACC_LIMIT_DEF;
    switch (tessera.acc_mode[tr]) {
    case TESS_ACC_PING:                                 /* 0 .. LIMIT .. 0, and so on */
        x = a * (pass % (2u * lim)) % (2u * lim);
        if (x > lim) x = 2u * lim - x;
        break;
    case TESS_ACC_CLAMP:
        x = pass >= lim ? lim : a * pass;
        if (x > lim) x = lim;
        break;
    default:                                            /* WRAP: 0 .. LIMIT, then from 0 again */
        x = a * (pass % (lim + 1u)) % (lim + 1u);
        break;
    }
    return climb < 0 ? -(int32_t)x : (int32_t)x;
}

/* note n moved d degrees in track t's scale (QNT OFF: d semitones); one off the scale counts its next one as the first */
static uint8_t acc_note(const track_t *t, uint32_t n, int32_t d)
{
    int32_t k = (int32_t)n, dir = d < 0 ? -1 : 1;
    uint32_t mask = t->p[P_QUANT] == Q_OFF ? 0xFFFu : scale_mask(t) & 0xFFFu;
    if (!mask)
        mask = 0xFFFu;
    while (d && k + dir >= 0 && k + dir <= 127) {
        k += dir;
        if ((mask >> (uint32_t)(((k - t->p[P_ROOT]) % 12 + 12) % 12)) & 1u)
            d -= dir;
    }
    return (uint8_t)clamp(k, 0, 127);
}

/* step s as it plays on pass `pass`: s itself, or *out with its notes moved (out may be s already) */
static const step_t *step_acc(const track_t *t, const step_t *s, step_t *out, uint32_t pass)
{
    int32_t d;
    if (!s->climb || s->time != ST_NOTE || !s->n || (s->flags & SF_RECORDED))
        return s;
    if (!(d = acc_offset(trk_index(t), step_climb(s), pass)))
        return s;
    if (s != out)
        *out = *s;
    for (uint32_t i = 0; i < out->n && i < 4u; i++)
        out->note[i] = acc_note(t, out->note[i], d);
    return out;
}
