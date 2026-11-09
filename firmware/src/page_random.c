/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Page randomize (issue #529): on a generator track's GEN and TWEAK pages, OCT- held and KNOB 1 turned right draws
 * new values for the page's knobs, each inside its range (pid.c); turned left puts back the values from before the
 * last draw (one level, as ROLL ALL). GEN: the engine's KNOB 2..4 (midi_cc.c GEN_KNOB_TX), TWEAK: SHIFT DROP HUMAN.
 * Never MODE, the seed, FOLLOW, a link to other tracks (QUAD LINK) or an automation (CONSTELLATION AUTO): those stay
 * as set. A STEP or ARP track has no generator: NO GENERATOR, nothing changes. Included by melodee.c after ui_graph.c; its columns: ui_draw.c */

#define PR_N 3u
static struct {
    uint8_t tr;                  /* track + 1 of the values kept, 0 = none */
    uint8_t mode;                /* its MODE then (another MODE since: nothing to put back) */
    uint8_t tx[PR_N];            /* the ids drawn (0xFF none) */
    int16_t v[PR_N];             /* their values before the draw */
    uint32_t n;                  /* draws so far (the stream's position) */
} pr;

/* GEN or TWEAK with OCT- held (OCT+ not) */
static int pr_layer_on(void)
{
    const page_t *pg = cur_page();
    return !ui.home && !ui.menu && !ui.confirm && (pg->graph == GR_TWEAK || (pg->graph == GR_STEPS && pg->fam == FAM_GEN)) &&
           ((fm1_in.buttons >> panel.btn[B_OCTDN]) & 1u) && !((fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u);
}
static int pr_gen_track(uint32_t tr) { return tr < TESSERA_TRACKS && tessera.mode[tr] != TESS_ENG_OFF; }
static int pr_back_ok(uint32_t tr) { return pr.tr == tr + 1u && pr.mode == tessera.mode[tr]; }

/* the page's ids on track tr (0xFF: none, or one never drawn) */
static void pr_ids(uint32_t tr, uint8_t *tx)
{
    uint32_t k;
    if (cur_page()->graph == GR_TWEAK) {
        tx[0] = TX_TW_SHIFT, tx[1] = TX_TW_DROP, tx[2] = TX_TW_HUMAN;
        return;
    }
    for (k = 0; k < PR_N; k++) {
        uint8_t x = GEN_KNOB_TX[tessera.mode[tr] % TESS_ENG_COUNT][k];
        tx[k] = x == TX_QD_LINK || x == TX_CS_AUTO ? 0xFFu : x;
    }
}

/* KNOB 1 with OCT- held: dir > 0 new values, < 0 the ones before back. 1 something changed */
static int page_random(int32_t dir)
{
    uint32_t tr = song.sel, k;
    if (!pr_gen_track(tr)) {
        ui_message("NO GENERATOR");
        return 0;
    }
    if (dir < 0) {
        if (!pr_back_ok(tr)) {
            ui_message("NOTHING TO UNDO");
            return 0;
        }
        for (k = 0; k < PR_N; k++)
            if (pr.tx[k] != 0xFFu) tessera_param_set(tr, pr.tx[k], pr.v[k]);
        pr.tr = 0;
        ui_message("PUT BACK");
        return 1;
    }
    pr_ids(tr, pr.tx);
    pr.tr = (uint8_t)(tr + 1u);
    pr.mode = tessera.mode[tr];
    pr.n++;
    for (k = 0; k < PR_N; k++) {                        /* (in order: Euclid's PULSES and ROTATE inside its new STEPS) */
        const param_desc_t *d;
        int32_t lo, hi;
        if (pr.tx[k] == 0xFFu || !(d = pid_desc(PID_TESS(pr.tx[k]), tr))) {
            pr.tx[k] = 0xFFu;
            continue;
        }
        pr.v[k] = (int16_t)tessera_param_get(tr, pr.tx[k]);
        lo = d->min, hi = d->max;
        if (pr.tx[k] == TX_EUC_PULSES) hi = tessera.euclid[tr].steps;
        if (pr.tx[k] == TX_EUC_ROT) hi = tessera.euclid[tr].steps > 1u ? tessera.euclid[tr].steps - 1 : 0;
        if (hi < lo) hi = lo;
        tessera_param_set(tr, pr.tx[k], lo + (int32_t)(gen_stream(TESS_ENTROPY() + pr.n * 0x9E3779B9u + k * 0x85EBCA6Bu) %
                                                       (uint32_t)(hi - lo + 1)));
    }
    ui_message("RANDOMIZED");
    return 1;
}
