/* SPDX-License-Identifier: GPL-3.0-only */
/* The filter is UI state; the selected scale retains its stable project ID. */
_Static_assert(SCALE_TOTAL <= SCALE_FAV_CAP, "scale favorite capacity");
static int scale_settings_page(const page_t *pg)
{
    return pg->fam == FAM_SCL && pg->graph == GR_SCALE && pg->id[1] == 0xFF;
}
/* The picker's ids: the catalogue's (P_SCALE), then SCALE_TOTAL + k the user tuning in slot k (tune.h: TUNING, the
 * project's), listed in ALL and in USER, the family after the favourites, once the web editor sent them */
static uint32_t scale_picker_selected(void)         /* the id playing: TUNING's slot, else SCL's */
{
    return tune_scale() ? SCALE_TOTAL + tune_sel - 1u : (uint32_t)clamp(TSEL->p[P_SCALE], 0, SCALE_TOTAL - 1u);
}
static int scale_pick_fav(uint32_t id) { return id < SCALE_TOTAL && scale_favorite(id); }
static const char *scale_pick_name(uint32_t id)     /* "53EDO", a tuning's own name */
{
    return id < SCALE_TOTAL ? N_SCALE[id] : tune_name(id - SCALE_TOTAL);
}
static const char *scale_pick_title(uint32_t id)    /* "53-tone equal division", a tuning's title (else its name) */
{
    return id < SCALE_TOTAL ? SCALE_TITLE[id] : tune_title(id - SCALE_TOTAL)[0] ? tune_title(id - SCALE_TOTAL) :
                                                                                   tune_name(id - SCALE_TOTAL);
}
static uint32_t scale_pick_degrees(uint32_t id)
{
    return id < SCALE_TOTAL ? SCALE_DEGREES[id] : tune_micro[(id - SCALE_TOTAL) % TUNE_SLOTS].count;
}
static int scale_pick_nonoct(uint32_t id)           /* a period other than the octave */
{
    return id < SCALE_TOTAL ? SCALE_FAMILY[id] == 6u :
                              tune_slot[(id - SCALE_TOTAL) % TUNE_SLOTS].pitch[scale_pick_degrees(id)] != 120000;
}
static const char *scale_family_short(uint32_t f) { return f == SCALE_FAM_USER ? "USER" : SCALE_FAMILY_SHORT[f]; }
static void scale_picker_mark(int on)
{
    if (!tune_scale() && scale_favorite_set((uint32_t)TSEL->p[P_SCALE], on)) settings_save();
    ui.force = 1;                   /* marking a favorite keeps the active tuning (a user tuning: none to mark) */
}
static int scale_picker_match(uint32_t id)
{
    if (id >= SCALE_TOTAL)
        return id < SCALE_PICK_TOTAL && tune_used(id - SCALE_TOTAL) && (!ui.scale_family || ui.scale_family == SCALE_FAM_USER);
    return !ui.scale_family || (ui.scale_family == SCALE_FAMILIES + 1u ? scale_favorite(id) :
        SCALE_FAMILY[id] + 1u == ui.scale_family);
}
static uint32_t scale_picker_count(void)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < SCALE_PICK_TOTAL; i++) n += scale_picker_match(i);
    return n;
}
static uint32_t scale_picker_at(uint32_t rank)
{
    for (uint32_t i = 0; i < SCALE_PICK_TOTAL; i++)
        if (scale_picker_match(i) && !rank--) return i;
    return SCALE_PICK_TOTAL;
}
static uint32_t scale_picker_rank(void)
{
    uint32_t n = 0, selected = scale_picker_selected();
    for (uint32_t i = 0; i < SCALE_PICK_TOTAL; i++) {
        if (!scale_picker_match(i)) continue;
        if (i == selected) return n;
        n++;
    }
    return n;
}
static void scale_picker_select(uint32_t id)
{
    if (id >= SCALE_PICK_TOTAL) return;
    if (id >= SCALE_TOTAL) {                        /* a user tuning: TUNING, SCL kept underneath */
        tune_sel = (uint8_t)(id - SCALE_TOTAL + 1u);
    } else {
        tune_sel = 0;
        TSEL->p[P_SCALE] = (int16_t)id;
        motion_capture(TSEL, P_SCALE, (int16_t)id);
    }
    scale_share(TSEL);
    ui.force = 1;
}
static void scale_picker_step(int32_t steps)
{
    uint32_t count = scale_picker_count(), rank = scale_picker_rank();
    if (!count) return;
    scale_picker_select(scale_picker_at(rank >= count ? (steps > 0 ? 0u : count - 1u) :
        (uint32_t)clamp((int32_t)rank + steps, 0, count - 1u)));
}
static void scale_picker_edit(uint32_t slot, int32_t steps)
{
    if (slot == 0u) {
        ui.scale_family = (uint8_t)clamp((int32_t)ui.scale_family + steps, 0, SCALE_FAM_USER);
        if (!scale_picker_match(scale_picker_selected())) scale_picker_select(scale_picker_at(0));
    } else if (slot == 1u) {
        scale_picker_step(steps);
    } else if (slot == 2u) {
        TSEL->p[P_ROOT] = (int16_t)clamp(TSEL->p[P_ROOT] + steps, TP[P_ROOT].min, TP[P_ROOT].max);
        motion_capture(TSEL, P_ROOT, TSEL->p[P_ROOT]);
        if (tessera.key_link) scale_share(TSEL);       /* (KEY LINK: every melodic track's) */
    } else {
        TSEL->p[P_QUANT] = (int16_t)clamp(TSEL->p[P_QUANT] + steps, TP[P_QUANT].min, TP[P_QUANT].max);
        motion_capture(TSEL, P_QUANT, TSEL->p[P_QUANT]);
        scale_share(TSEL);
    }
    ui.force = 1;
}
