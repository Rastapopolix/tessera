/* SPDX-License-Identifier: GPL-3.0-only */
/* Tessera MIDI's project storage (TESSERA_MIDI_ONLY, storage.c): twelve sectors a copy, the extra two from the sound's
 * idle storage. Each copy round-trips a full 46,048-byte record; no sector shared between copies or with an object
 * still in use (settings, MIDI LEARN); every sector inside the HAL's storage areas (hal/fm1_flash.h FL_STORE_OK), never
 * the app area or the OTA staging, its tail left erased (the update loader's scan); the first ten sectors of each copy
 * the standard build's (a record of ten sectors or fewer reads the same in both); the sound's objects never load and
 * a save of one (a restored backup) writes nothing. Built and run by tests/run_tests.sh. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define TESSERA_MIDI_ONLY 1
static uint8_t nor[0x100000];
static uint32_t writes;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { writes++; memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    writes++;
    for (uint32_t i = 0; i < n; i++) nor[off + i] &= s[i];
    return 0;
}
#include "../firmware/src/storage.c"
#define FL_IN(off, n, lo, hi) ((uint32_t)(off) >= (lo) && (uint32_t)(off) <= (hi) && (uint32_t)(n) <= (hi) - (uint32_t)(off))
#define FL_STORE_OK(off, n) (FL_IN(off, n, 0x93000u, 0x97000u) || FL_IN(off, n, 0x97000u, 0xE0000u) || \
                             FL_IN(off, n, 0xFC000u, 0xFF000u) || FL_IN(off, n, 0xE5000u, 0xFB000u))   /* (hal/fm1_flash.h) */
static int check(const char *what, int ok)
{
    printf("%-64s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}
int main(void)
{
    static uint8_t payload[ST_PROJ_MAX], got[ST_PROJ_MAX];
    static int16_t owner[0x100000 / ST_SECTOR];
    /* the standard build's first ten sectors of each copy (storage.c's other ST_PROJ_RUN) */
    static const uint32_t STD[8][2][2] = {{{0xA0000u, 10}}, {{0xAA000u, 10}}, {{0xB4000u, 10}}, {{0xBE000u, 10}},
                                          {{0xC8000u, 10}}, {{0xEA000u, 10}}, {{0x93000u, 10}}, {{0xD2000u, 6}, {0xF4000u, 4}}};
    int bad = 0, ok = 1, inside = 1, tails = 1, clash = 0, same = 1;
    uint32_t i, k, copy, o;
    for (i = 0; i < sizeof owner / sizeof owner[0]; i++) owner[i] = -1;
    for (i = 0; i < sizeof payload; i++) payload[i] = (uint8_t)(i * 37u + 11u);
    memset(nor, 0xFF, sizeof nor);
    bad += check("a project copy holds 46,048 B (twelve sectors)", ST_PROJ_SECTORS == 12u && st_capacity(OBJ_PROJ0) == 46048u);
    for (k = 0; k < 4u; k++)
        for (copy = 0; copy < 2u; copy++) {
            ok &= st_save(OBJ_PROJ0 + k, payload, sizeof payload) == 0;
            ok &= st_load(OBJ_PROJ0 + k, got, sizeof got) == (int)sizeof got && !memcmp(got, payload, sizeof got);
            for (i = 0; i < ST_PROJ_SECTORS; i++) {
                uint32_t sec = st_proj_sector(OBJ_PROJ0 + k, copy, i), c = k * 2u + copy, std;
                inside &= FL_STORE_OK(sec, ST_SECTOR) && !(sec < 0xE5000u && sec + ST_SECTOR > 0xE0000u) && sec >= 0x93000u &&
                          sec < 0xFC000u;
                for (uint32_t j = ST_SECTOR - ST_TAIL; j < ST_SECTOR; j++) tails &= nor[sec + j] == 0xFFu;
                clash += owner[sec / ST_SECTOR] >= 0;
                owner[sec / ST_SECTOR] = (int16_t)c;
                if (i < 10u) {
                    std = i < STD[c][0][1] ? STD[c][0][0] + i * ST_SECTOR : STD[c][1][0] + (i - STD[c][0][1]) * ST_SECTOR;
                    same &= sec == std;
                }
            }
        }
    bad += check("four slots, both copies, a full record round trip each", ok);
    bad += check("every sector inside the storage areas (no app area, OTA, settings)", inside);
    bad += check("every sector's last 256 B erased (update records)", tails);
    bad += check("no sector in two project copies", !clash);
    bad += check("the first ten sectors of each copy are the standard build's", same);
    for (o = 0; o < 2u; o++) {                         /* settings, MIDI LEARN: still in use, their sectors their own */
        uint32_t obj = o ? OBJ_LEARN : OBJ_SETTINGS;
        for (copy = 0; copy < 2u; copy++) clash += owner[st_sector(obj, copy) / ST_SECTOR] >= 0;
    }
    bad += check("settings and MIDI LEARN keep their own sectors", !clash);
    ok = 1;
    for (o = 0; o < OBJ_COUNT; o++)
        if (st_gone(o)) {
            uint32_t w = writes;
            ok &= st_save(o, payload, 64u) == 0 && writes == w && st_load(o, got, sizeof got) < 0 && !st_exists(o);
        }
    bad += check("the sound's objects: a save writes nothing, nothing loads", ok);
    ok = st_load(OBJ_PROJ0, got, sizeof got) == (int)sizeof got && !memcmp(got, payload, sizeof got);
    bad += check("the projects untouched by those saves", ok);
    printf("storage_midi_test %s\n", bad ? "FAILED" : "passed");
    return !!bad;
}
