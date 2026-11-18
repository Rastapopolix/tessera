/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* One save format everywhere: a .tsp saved by one target loads in the others. Tessera's core API
 * (platform/core/tess_core.h) built as one target builds it (tests/tsp_cross.sh: the FM-1's options, the web
 * simulator's, the plugin's, the Daisy's at 48 kHz), as a tool:
 *   tsp_cross save IN.tsp SECS OUT.tsp   IN loaded as SAVE > PROJECT loads it, played SECS seconds (the generators
 *                                        and their state move on), stopped, saved
 *   tsp_cross load IN.tsp OUT.tsp        IN loaded and saved again at once: a target that reads IN as it was written
 *                                        writes the same bytes back
 *   tsp_cross info                       the target's rate and options, one line
 * Exit 0 done; 1 IN refused (not a record this build reads); 2 the save failed; 3 usage or a file. */
#include "../platform/core/core_unit.c"

static uint8_t buf_in[65536], buf_out[65536];

static uint32_t read_file(const char *path, uint8_t *b, uint32_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f)
        return 0;
    n = fread(b, 1, cap, f);
    fclose(f);
    return (uint32_t)n;
}

static int write_file(const char *path, const uint8_t *b, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f || fwrite(b, 1, n, f) != n)
        return 0;
    return fclose(f) == 0;
}

int main(int argc, char **argv)
{
    uint32_t n, out;
    if (argc == 2 && !strcmp(argv[1], "info")) {
        printf("rate %u  TSP_MAX %u  UI %d  screen %d  access %d  usb-audio %d  usb-host %d\n", (unsigned)tess_core_rate(),
               (unsigned)tess_core_tsp_max(), TESS_CORE_UI, HOST_HAL_SCREEN, MELODEE_ACCESS, MELODEE_USB_AUDIO,
               MELODEE_USB_HOST);
        return 0;
    }
    if (argc < 4 || (strcmp(argv[1], "save") && strcmp(argv[1], "load")) || (!strcmp(argv[1], "save") && argc < 5)) {
        fprintf(stderr, "usage: tsp_cross save IN.tsp SECS OUT.tsp | load IN.tsp OUT.tsp | info\n");
        return 3;
    }
    n = read_file(argv[2], buf_in, sizeof buf_in);
    if (!n) {
        fprintf(stderr, "tsp_cross: can't read %s\n", argv[2]);
        return 3;
    }
    tess_core_boot();
    if (tess_core_load(buf_in, n)) {
        fprintf(stderr, "tsp_cross: %s refused\n", argv[2]);
        return 1;
    }
    if (!strcmp(argv[1], "save")) {
        static float l[1024], r[1024];
        uint32_t frames = (uint32_t)atoi(argv[3]) * tess_core_rate(), k;
        tess_core_transport(1);
        for (k = 0; k < frames; k += 512u)
            tess_core_render(l, r, 0, 512u);
        tess_core_transport(0);
        for (k = 0; k < 8u; k++)                  /* (the stop taken, the notes released) */
            tess_core_render(l, r, 0, 512u);
    }
    out = tess_core_save(buf_out, sizeof buf_out);
    if (!out) {
        fprintf(stderr, "tsp_cross: the save failed\n");
        return 2;
    }
    if (!write_file(argv[argc - 1], buf_out, out)) {
        fprintf(stderr, "tsp_cross: can't write %s\n", argv[argc - 1]);
        return 3;
    }
    return 0;
}
