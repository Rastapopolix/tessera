/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Portions Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; Theo Dainis (melodee-tess) */
/* Power-on without the FM-1 (moved here from tests/ui_test.c): main.c melodee_init's state, as the host tests start
 * (track defaults, the GRAY palette, empty slots); the web simulator, the plugin and the Daisy then set the device's
 * own start over it (platform/core/tess_core.c tess_core_boot). After core_ui_sources.h. */
/* the host screen blanked: the diff (gfx.c) forgets what it sent, as after a fill */
static void screen_clear(void)
{
#if HOST_HAL_SCREEN
    memset(host_screen, 0, sizeof host_screen);
#endif
    memset(df_key, 0, sizeof df_key);
#ifdef UI_REF_SCREEN
    memset(ref_screen, 0, sizeof ref_screen);
#endif
}


/* main.c melodee_init */
static void ui_power_on(void)
{
    uint32_t i;
    fx_clear_all();
    for(uint32_t k=0;k<FX_STATE_COUNT;k++)resource_release(RES_FX0+k);
    fx_bus_result=fx_bus_pending=0;fx_bus_mix=fx_bus_target=0;
    memset(fx_state,0,sizeof fx_state);
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    pattern_init();
    memset(&ui, 0, sizeof ui);
    memset(&boot_work, 0, sizeof boot_work);
    memset(&step_history, 0, sizeof step_history);
    memset(&sx_import,0,sizeof sx_import);memset(&bulk,0,sizeof bulk);resource_release(RES_NOTE_HISTORY);note_transaction_ptr=0;
    midi_sync_active=0;memset(&midi_clock,0,sizeof midi_clock);
    seq_midi_reset();
    memset(&nm, 0, sizeof nm);                    /* NAME closed, no project name */
    memset(&nw, 0, sizeof nw);                    /* NEW SONG closed */
    memset(&pop, 0, sizeof pop);                  /* no popup (ui_popup.c) */
    smap.on = 0; sec.ok = 0;                      /* no map, the sections built again (ui_sections.c) */
    oct_eat = oct_deferred = 0;
    proj_name[0] = 0;
    proj_cur = PROJ_NO_SLOT;
    memset(&favorites, 0, sizeof favorites);
    memset(&brw, 0, sizeof brw);                  /* the browser: nothing pending, LIST ALL, RECENT empty */
    list_recent = 0;
    list_forget();
    recent_n = 0;
    undo_clear();
    memset(pat_last, 0, sizeof pat_last);
    memset(proj_bank_len, 0, sizeof proj_bank_len);
    up_cache_reset(); native_cache_reset();
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    undo_depth++;
    fm6_init();                                   /* every track's FM6 patch: the init voice (as main.c; else all 0,
                                                   * and a saved project's names came back as spaces) */
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        set_engine_of(t, TRK_DEF[i][0]);
        apply_preset_to(t, TRK_DEF[i][1]);
        t->engine = t->eng_req;
        track_defaults_steps(t);
        pat_sig[i] = steps_sig(t);
        pat_last[i] = 0;
    }
    undo_depth--;
    song.sel = 0;
    ui.home = 1;
    ui.force = 1;
    panel = PANEL_DEFAULT;
    settings.palette = UI_GRAY_INDEX;
    palette_set(UI_GRAY_INDEX);
    transport_req = 0;
    panic_req = 0;
    mi_r = mi_w = midi_in_overflow = 0;
    memset(midi_sel_on, 0, sizeof midi_sel_on);
    memset(midi_ch, 0, sizeof midi_ch);
    memset(midi_owners, 0, sizeof midi_owners);
    memset(midi_sel_on, 0, sizeof midi_sel_on);
    kb_prev = 0;
    memset(live_held, 0, sizeof live_held);
    memset(live_last, 0, sizeof live_last);
    memset(midi_bend_q8, 0, sizeof midi_bend_q8);
    memset(midi_bend_target, 0, sizeof midi_bend_target);
    memset(kb_chn, 0, sizeof kb_chn);                   /* chord.c: no key, MIDI chord or last chord */
    memset(mchord, 0, sizeof mchord);
    memset(chord_last, 0, sizeof chord_last);
    kb_layer = 0; kb_lock = 0;
    kb_mask = layer_bits();
    track_solo = 0;
    memset(&lys, 0, sizeof lys);
    settings_hold = HOLD_DEF;
    memset(&sim, 0, sizeof sim);                        /* SIM CONTROL off, as every boot */
    kb_remote = 0;
    fm1_in.buttons = fm1_in.notes = 0;
}
