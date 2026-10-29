/* SPDX-License-Identifier: GPL-3.0-only */
/* ORACLE's pages (params.c PAGES, scope SC_ORACLE): the patch (eng_oracle.c or_patch) as values one turns. A value
 * is a view of the patch: most are one byte of it, some fold the Prophet's switches into one choice (WAVE A: SAW
 * and PULSE; B MODE: KEY and LOW). The pages read and write through or_view_get / or_view_put; nothing else here
 * holds a value */
enum {
    OV_FREQ_A, OV_WAVE_A, OV_PW_A, OV_SYNC,                       /* OSC A */
    OV_FREQ_B, OV_FINE_B, OV_WAVE_B, OV_PW_B,                     /* OSC B */
    OV_LVL_A, OV_LVL_B, OV_NOISE, OV_B_MODE,                      /* OSC MIX */
    OV_CUT, OV_RES, OV_ENV, OV_KEY,                               /* FILTER */
    OV_FA, OV_FD, OV_FS, OV_FR,                                   /* FILT ENV */
    OV_AA, OV_AD, OV_AS, OV_AR,                                   /* AMP ENV */
    OV_PM_ENV, OV_PM_B, OV_PM_DEST, OV_VEL,                       /* POLY MOD */
    OV_RATE, OV_AMT, OV_LWAVE, OV_NMIX,                           /* LFO MOD */
    OV_L_PITCH, OV_L_PW, OV_L_FILT,                               /* LFO ROUTE */
    OV_MODEL, OV_VINTAGE, OV_GLIDE,                               /* CHARACTER */
    OV_COUNT
};
static const char *const N_OR_WAVE_A[] = {"OFF", "SAW", "PLS", "S+P"};
static const char *const N_OR_WAVE_B[] = {"OFF", "SAW", "TRI", "S+T", "PLS", "S+P", "T+P", "ALL"};
static const char *const N_OR_LWAVE[] = {"OFF", "SAW", "TRI", "S+T", "SQR", "S+Q", "T+Q", "ALL"};
static const char *const N_OR_DEST[] = {"OFF", "FREQ", "PW", "FQ+PW", "FILT", "FQ+FL", "PW+FL", "ALL"};
static const char *const N_OR_AB[] = {"OFF", "A", "B", "A+B"};
static const char *const N_OR_BMODE[] = {"KEY", "FIXED", "LO KEY", "LO FIX"};
static const char *const N_OR_KEY[] = {"OFF", "HALF", "FULL"};
static const char *const N_OR_VEL[] = {"OFF", "FILT", "AMP", "BOTH"};
static const char *const N_OR_MODEL[] = {"EARLY", "LATE"};
static const param_desc_t OR_VD[OV_COUNT] = {
    [OV_FREQ_A] = PD("FREQ A", F_SEMI, -12, 36, 0), [OV_WAVE_A] = PE("WAVE A", N_OR_WAVE_A, 1),
    [OV_PW_A] = PD("PW A", F_INT, 0, 127, 64), [OV_SYNC] = PE("SYNC", N_ONOFF, 0),
    [OV_FREQ_B] = PD("FREQ B", F_SEMI, -12, 36, 0), [OV_FINE_B] = PD("FINE B", F_INT, 0, 127, 0),
    [OV_WAVE_B] = PE("WAVE B", N_OR_WAVE_B, 0), [OV_PW_B] = PD("PW B", F_INT, 0, 127, 64),
    [OV_LVL_A] = PD("OSC A", F_INT, 0, 127, 127), [OV_LVL_B] = PD("OSC B", F_INT, 0, 127, 0),
    [OV_NOISE] = PD("NOISE", F_INT, 0, 127, 0), [OV_B_MODE] = PE("B MODE", N_OR_BMODE, 0),
    [OV_CUT] = PD("CUTOFF", F_INT, 0, 127, 127), [OV_RES] = PD("RES", F_INT, 0, 127, 0),
    [OV_ENV] = PD("ENV AMT", F_INT, 0, 127, 0), [OV_KEY] = PE("KEY", N_OR_KEY, 0),
    [OV_FA] = PD("ATK", F_INT, 0, 127, 0), [OV_FD] = PD("DEC", F_INT, 0, 127, 60),
    [OV_FS] = PD("SUS", F_INT, 0, 127, 127), [OV_FR] = PD("REL", F_INT, 0, 127, 40),
    [OV_AA] = PD("ATK", F_INT, 0, 127, 0), [OV_AD] = PD("DEC", F_INT, 0, 127, 60),
    [OV_AS] = PD("SUS", F_INT, 0, 127, 127), [OV_AR] = PD("REL", F_INT, 0, 127, 40),
    [OV_PM_ENV] = PD("FENV", F_INT, 0, 127, 0), [OV_PM_B] = PD("OSC B", F_INT, 0, 127, 0),
    [OV_PM_DEST] = PE("TO", N_OR_DEST, 0), [OV_VEL] = PE("VEL", N_OR_VEL, 0),
    [OV_RATE] = PD("RATE", F_INT, 0, 127, 60), [OV_AMT] = PD("AMOUNT", F_INT, 0, 127, 0),
    [OV_LWAVE] = PE("WAVE", N_OR_LWAVE, 2), [OV_NMIX] = PD("NOISE", F_INT, 0, 127, 0),
    [OV_L_PITCH] = PE("PITCH", N_OR_AB, 0), [OV_L_PW] = PE("PW", N_OR_AB, 0), [OV_L_FILT] = PE("FILT", N_ONOFF, 0),
    [OV_MODEL] = PE("MODEL", N_OR_MODEL, 1), [OV_VINTAGE] = PD("VINTAGE", F_INT, 0, 127, 0),
    [OV_GLIDE] = PD("GLIDE", F_INT, 0, 127, 0),
};
/* FREQ B with B's keyboard off (B MODE FIXED / LO FIX): the knob as it is, not semitones */
static const param_desc_t OR_FREQ_FIXED = PD("FREQ B", F_INT, 0, 127, 24);
/* the views that are one byte of the patch, as it is */
static const uint8_t OR_BYTE[OV_COUNT] = {
    [OV_PW_A] = OR_PW_A + 1u, [OV_FINE_B] = OR_FINE_B + 1u, [OV_PW_B] = OR_PW_B + 1u, [OV_SYNC] = OR_SYNC + 1u,
    [OV_LVL_A] = OR_LEVEL_A + 1u, [OV_LVL_B] = OR_LEVEL_B + 1u, [OV_NOISE] = OR_NOISE + 1u,
    [OV_CUT] = OR_CUTOFF + 1u, [OV_RES] = OR_RESONANCE + 1u, [OV_ENV] = OR_ENV_FILTER + 1u, [OV_KEY] = OR_KEY_FILTER + 1u,
    [OV_FA] = OR_ATTACK_FILTER + 1u, [OV_FD] = OR_DECAY_FILTER + 1u, [OV_FS] = OR_SUSTAIN_FILTER + 1u,
    [OV_FR] = OR_RELEASE_FILTER + 1u, [OV_AA] = OR_ATTACK_AMP + 1u, [OV_AD] = OR_DECAY_AMP + 1u,
    [OV_AS] = OR_SUSTAIN_AMP + 1u, [OV_AR] = OR_RELEASE_AMP + 1u, [OV_PM_ENV] = OR_POLY_ENV + 1u,
    [OV_PM_B] = OR_POLY_B + 1u, [OV_RATE] = OR_LFO_RATE + 1u, [OV_AMT] = OR_LFO_INITIAL + 1u,
    [OV_NMIX] = OR_WHEEL_MIX + 1u, [OV_L_FILT] = OR_WHEEL_FILTER + 1u, [OV_MODEL] = OR_FILTER_REV + 1u,
    [OV_VINTAGE] = OR_VINTAGE + 1u, [OV_GLIDE] = OR_GLIDE + 1u,
};
static int16_t or_cell[4];
static int or_bit(const uint8_t *p, uint32_t i) { return p[i] != 0; }
static int32_t or_bits2(const uint8_t *p, uint32_t a, uint32_t b) { return or_bit(p, a) | or_bit(p, b) << 1; }
static int32_t or_bits3(const uint8_t *p, uint32_t a, uint32_t b, uint32_t c)
{
    return or_bit(p, a) | or_bit(p, b) << 1 | or_bit(p, c) << 2;
}
/* a keyed oscillator's FREQ: semitones (two raw values a detent, 24 = 0, up to 96) */
static int32_t or_semi(uint32_t raw) { return (int32_t)(raw > 96u ? 96u : raw) / 2 - 12; }
static int or_b_keyed(const uint8_t *p) { return p[OR_KEY_B] != 0; }
static int32_t or_view_get(const uint8_t *p, uint32_t id)
{
    if (id >= OV_COUNT) return 0;
    if (OR_BYTE[id]) return p[OR_BYTE[id] - 1u];
    switch (id) {
    case OV_FREQ_A: return or_semi(p[OR_FREQ_A]);
    case OV_FREQ_B: return or_b_keyed(p) ? or_semi(p[OR_FREQ_B]) : p[OR_FREQ_B];
    case OV_WAVE_A: return or_bits2(p, OR_SAW_A, OR_PULSE_A);
    case OV_WAVE_B: return or_bits3(p, OR_SAW_B, OR_TRI_B, OR_PULSE_B);
    case OV_B_MODE: return !or_b_keyed(p) | or_bit(p, OR_LOW_B) << 1;
    case OV_PM_DEST: return or_bits3(p, OR_POLY_FREQ, OR_POLY_PW, OR_POLY_FILTER);
    case OV_VEL: return or_bits2(p, OR_VEL_FILTER, OR_VEL_AMP);
    case OV_LWAVE: return or_bits3(p, OR_LFO_SAW, OR_LFO_TRI, OR_LFO_PULSE);
    case OV_L_PITCH: return or_bits2(p, OR_WHEEL_FREQ_A, OR_WHEEL_FREQ_B);
    case OV_L_PW: return or_bits2(p, OR_WHEEL_PW_A, OR_WHEEL_PW_B);
    default: return 0;
    }
}
static const param_desc_t *or_view_desc(const uint8_t *p, uint32_t id)
{
    if (id >= OV_COUNT) return 0;
    return id == OV_FREQ_B && !or_b_keyed(p) ? &OR_FREQ_FIXED : &OR_VD[id];
}
/* value v of view id -> p (v inside the view's range) */
static void or_view_put(uint8_t *p, uint32_t id, int32_t v)
{
    const param_desc_t *d = or_view_desc(p, id);
    if (!d) return;
    v = clamp(v, d->min, d->max);
    if (OR_BYTE[id]) { p[OR_BYTE[id] - 1u] = (uint8_t)v; return; }
    switch (id) {
    case OV_FREQ_A: p[OR_FREQ_A] = (uint8_t)((v + 12) * 2); break;
    case OV_FREQ_B:
        if (or_b_keyed(p)) p[OR_FREQ_B] = (uint8_t)((v + 12) * 2);
        else p[OR_FREQ_B] = (uint8_t)v;
        break;
    case OV_WAVE_A: p[OR_SAW_A] = (uint8_t)(v & 1); p[OR_PULSE_A] = (uint8_t)(v >> 1 & 1); break;
    case OV_WAVE_B: p[OR_SAW_B] = (uint8_t)(v & 1); p[OR_TRI_B] = (uint8_t)(v >> 1 & 1); p[OR_PULSE_B] = (uint8_t)(v >> 2 & 1); break;
    case OV_B_MODE: p[OR_KEY_B] = (uint8_t)!(v & 1); p[OR_LOW_B] = (uint8_t)(v >> 1 & 1); break;
    case OV_PM_DEST: p[OR_POLY_FREQ] = (uint8_t)(v & 1); p[OR_POLY_PW] = (uint8_t)(v >> 1 & 1); p[OR_POLY_FILTER] = (uint8_t)(v >> 2 & 1); break;
    case OV_VEL: p[OR_VEL_FILTER] = (uint8_t)(v & 1); p[OR_VEL_AMP] = (uint8_t)(v >> 1 & 1); break;
    case OV_LWAVE: p[OR_LFO_SAW] = (uint8_t)(v & 1); p[OR_LFO_TRI] = (uint8_t)(v >> 1 & 1); p[OR_LFO_PULSE] = (uint8_t)(v >> 2 & 1); break;
    case OV_L_PITCH: p[OR_WHEEL_FREQ_A] = (uint8_t)(v & 1); p[OR_WHEEL_FREQ_B] = (uint8_t)(v >> 1 & 1); break;
    case OV_L_PW: p[OR_WHEEL_PW_A] = (uint8_t)(v & 1); p[OR_WHEEL_PW_B] = (uint8_t)(v >> 1 & 1); break;
    default: break;
    }
}
/* the value in column slot of an ORACLE page (a copy in or_cell) and its descriptor; 0 = none */
static const param_desc_t *or_page_desc(const page_t *pg, uint32_t slot, int16_t **valp)
{
    uint32_t id = pg->id[slot];
    const uint8_t *p;
    *valp = 0;
    if (!MELODEE_ORACLE || id == 0xFFu || id >= OV_COUNT || TSEL->eng_req != ENGI_ORACLE)
        return 0;
    p = or_patch_of(TSEL);
    or_cell[slot & 3u] = (int16_t)or_view_get(p, id);
    *valp = &or_cell[slot & 3u];
    return or_view_desc(p, id);
}
/* .. and a new value there, into the track's patch (the sounding notes follow at the next block) */
static void or_page_put(const page_t *pg, uint32_t slot, int32_t val)
{
    uint32_t id = pg->id[slot];
    uint8_t p[OR_N];
    if (!MELODEE_ORACLE || id >= OV_COUNT || TSEL->eng_req != ENGI_ORACLE) return;
    memcpy(p, or_patch_of(TSEL), OR_N);
    or_view_put(p, id, val);
    memcpy(or_patch[song.sel % NTRK], p, OR_N);
}
