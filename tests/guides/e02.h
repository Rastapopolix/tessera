/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 2: play the demo project: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* the demos in their slots, as the web editor's "Demo projects" leaves them: EMBER FIELD in slot A (loaded, playing),
 * NIGHT FERRY in slot B. A slot is saved from a fresh project each, as power_on's NEW but keeping the flash */
static void g_fresh(void)
{
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    pattern_init();
    host_tracks_init();
    fm6_init();
    tessera_reset();
    project_new_blank();
    song.master_q12 = 2048;
}
static void g_slots_scene(void)
{
    demo_ferry();
    project_save_as(1, "NIGHT FERRY");
    g_fresh();
    demo_ember();
    project_save_as(0, "EMBER FIELD");
    project_load(0);
}
static const gstep_t G_SLOT_LOAD[] = {
    {0, 0, 0, 0, 0, "Projects live in four slots. This is EMBER FIELD, in slot A"},
    {3500, 6500, 200, "SAVE", "OPEN SAVE", "Tap SAVE: USER, where your own sounds go"},
    {10000, 13000, 200, "SAVE", "PROJECT", "Tap SAVE again: PROJECT, the slots"},
    {17000, 20000, 600, "KNOB 1", "PICK B", "Turn KNOB 1 to slot B, NIGHT FERRY", 1},
    {24000, 27000, 200, "OCT+", "OPEN SLOT", "Tap OCT+: what to do with it"},
    {31000, 34000, 200, "OCT+", "LOAD", "Tap OCT+ on LOAD: NIGHT FERRY loads, and playing stops"},
    {38000, 41000, 200, "HOME", "GO HOME", "Tap HOME: its four tracks"},
    {45000, 48000, 200, "PLAY", "PLAY", "Tap PLAY: NIGHT FERRY plays from the top"},
    {55000, 0, 0, 0, 0, "The web editor's Demo projects (Projects tab) puts the demos into the slots"},
};
static void guide_e02(const char *out, FILE *tsv)
{
    G_RUN("e02-load", "PLAY A DEMO", "YELLOW", 64, 20, g_slots_scene, G_SLOT_LOAD);   /* the demos themselves */
}
