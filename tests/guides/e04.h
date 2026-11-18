/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* Season 1, episode 4: keep it in key: the guided clips (tests/demo_projects.c guide; tools/guide_video.py) */
/* Season 1, episode 4: keep it in key */
static const gstep_t G_KEY[] = {
    {0, 0, 0, 0, 0, "Every track plays in the project's key. SEL sets it"},
    {3500, 6500, 4500, "SEL", "SET THE KEY", "Hold SEL and press a key: that note is the new root (A becomes D)"},
    {0, 8000, 300, "D4", "ROOT D", 0},
    {13500, 16500, 4500, "SEL", "SET THE KEY", "Still holding SEL, turn KNOB 2: another scale (DORIAN becomes MIXOLYDIAN)"},
    {0, 18000, 1500, "KNOB 2", "NEXT SCALE", 0, 1},
    {24000, 0, 0, 0, 0, "Let go of SEL: every melodic track follows (drums keep their own)"},
};

static void guide_e04(const char *out, FILE *tsv)
{
    G_RUN("e04-key", "KEEP IT IN KEY", "BLUE", 47, 60, demo_dub, G_KEY);             /* dub (demo: to replace) */
}
