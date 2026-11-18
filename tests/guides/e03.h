/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 3: change the sound: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* Season 1, episode 3: change the sound */
static const gstep_t G_SOUND[] = {
    {0, 0, 0, 0, 0, "HOME: the four tracks, each with its sound"},
    {3500, 6500, 600, "ALGORITHM", "PICK TRACK 2", "Turn ALGORITHM to pick a track: here track 2", 1},
    {11000, 14000, 2400, "PRESETS", "NEXT SOUNDS", "Turn PRESETS: track 2 steps through the next sounds (here to WARM PAD)", 3},
    {20500, 23500, 2400, "PRESETS", "BACK", "Turn PRESETS back: the sound you had (KOTO WARM) returns", -3},
    {30000, 0, 0, 0, 0, "Hold SAVE to keep the project with its sounds"},
};

static void guide_e03(const char *out, FILE *tsv)
{
    G_RUN("e03-sound", "CHANGE THE SOUND", "MOSAIC", 43, 30, demo_cedar, G_SOUND);   /* koto (demo: to replace) */
}
