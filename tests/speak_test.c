/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* The screen reader (MELODEE_ACCESS, firmware/src/ui_speak.c, fm1-reader's protocol in announce.c) on
 * tests/ui_test.c's stubs, in the Mono look as shipped: silent until a reader's HELLO; HELLO answered with the
 * firmware's name; the first HELLO says the whole screen; a page change says the page and its knobs once it
 * settles; a knob turning says its name and value once it is still; the menu's rows with their place; the
 * header's messages; PLAY and STOP; WHERE says the screen again; 5 s without a HELLO: silent again.
 * Built with -DMELODEE_ACCESS=1 -DMELODEE_LOOK=1; run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void reader(uint32_t cmd)                  /* a SysEx from the reader, as editor.c's ed_service hands it on */
{
    const uint8_t f[5] = {ANN_H0, ANN_H1, ANN_H2, (uint8_t)cmd, 1};
    ann_frame(f, cmd == ANN_HELLO ? 5u : 4u, fm1_ms);
}
static void said_clear(void) { host_said_n = 0; host_said[0] = 0; host_said_frames = 0; }
static int said(const char *s) { return strstr(host_said, s) != 0; }
static void settle(void) { uint32_t i; for (i = 0; i < 20u; i++) frame(); }   /* 320 ms: what waits is said */

int main(void)
{
    int bad = 0;
    ui_power_on();
    usb.config = 1;
    go_title("EVOLVE");
    settle();
    turn(EN_K1 + 2u, 2);
    settle();
    bad += check("speak: silent until a reader says HELLO (a DAW on the port sees nothing)", host_said_frames == 0u);

    reader(ANN_HELLO);
    frame();
    bad += check("speak: HELLO answered with the firmware's name", said("H Tessera TEST|"));
    bad += check("speak: the first HELLO says the screen at once: page, track, knobs by their whole names",
                 said("N EVOLVE. T1. ") && said("AMOUNT"));
    printf("  said: %s", host_said);

    said_clear();
    turn(EN_K1 + 2u, 3);
    bad += check("speak: a knob turning is not said at once", !said("AMOUNT"));
    settle();
    bad += check("speak: ..once it is still: its name and its value", said("N STEP EVOLVE AMOUNT ") && said(" %"));
    printf("  said: %s", host_said);

    said_clear();
    go_title("TIMING");
    settle();
    bad += check("speak: another page: its name and knobs, once it settles", said("N TIMING. T1. "));
    printf("  said: %s", host_said);

    said_clear();
    ui_message("SAVED");
    bad += check("speak: a message in the header is said at once", said("N SAVED"));

    said_clear();
    song.playing = 1;
    frame();
    bad += check("speak: PLAY", said("N PLAY"));
    song.playing = 0;
    frame();
    bad += check("speak: STOP", said("N STOP"));

    said_clear();
    hold(B_HOME);
    settle();
    printf("  said: %s", host_said);
    bad += check("speak: the menu: its first row, with its place", said("N MENU. COLOR, 1 OF "));
    said_clear();
    turn(EN_K1, 1);
    settle();
    bad += check("speak: ..KNOB 1 to the next row: its name, value and place", said(", 2 OF "));
    printf("  said: %s", host_said);
    press(B_OCTDN);
    settle();

    said_clear();
    reader(ANN_WHERE);
    frame();
    bad += check("speak: WHERE says the whole screen again", host_said_n > 0u && host_said[0] == 'N');

    said_clear();
    fm1_ms += 6000u;
    frame();
    turn(EN_K1 + 2u, 1);
    settle();
    bad += check("speak: 5 s without a HELLO: silent again", host_said_frames == 0u);
    printf(bad ? "speak: FAILED\n" : "speak: all passed\n");
    return bad != 0;
}
