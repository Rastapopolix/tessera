/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* LANDSCAPE's feed (ui_land.c, the page of the Mono look): the notes the tracks play, a few at a time for the scene's
 * drops. Written where a track's note starts (voice.c trk_note_on) only while the page is open (land_open): the page
 * changes nothing else, sound or saved. The UI reads from its own place in the ring (ui_land.c land_step), so a full
 * ring loses the oldest notes, never a frame's worth of time. Included by tessera.c */
#define LAND_EV 16u                                     /* (a power of two) */
static volatile uint8_t land_open;                      /* the page shown (ui_mono.c mono_page) */
static volatile uint8_t land_ev_at;                     /* the next place written, counting on past LAND_EV */
static uint8_t land_ev_tr[LAND_EV], land_ev_note[LAND_EV], land_ev_vel[LAND_EV];

static void land_note(const track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t i;
    if (!land_open)
        return;
    i = land_ev_at & (LAND_EV - 1u);
    land_ev_tr[i] = (uint8_t)((uint32_t)(t - trk) % NTRK);
    land_ev_note[i] = (uint8_t)note;
    land_ev_vel[i] = (uint8_t)vel;
    land_ev_at = (uint8_t)(land_ev_at + 1u);            /* (after the note: the UI reads only what is whole) */
}
