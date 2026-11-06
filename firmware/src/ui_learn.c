/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio): MIDI LEARN, upstream Melodee 7bd7e4f
 * Copyright (C) 2026 Theo Dainis (melodee-tess): its Tessera form */
/* MIDI LEARN's screen side (midi_learn.c): while learning, a knob turned (it still edits) picks its parameter as the
 * help card finds it (ui_help.c help_find) and waits for a CC; the next CC binds to it ("CC 21 =", then "T1 CUTOFF"). A CC
 * leaves the knob it had, a knob the CC it had. Included by ui_pages.c after ui_modsc.c. */
static struct { uint8_t on, track; uint16_t pid; } mlearn_want;   /* on: a knob picked, waiting for its CC */
static uint8_t mlearn_ask;                              /* the menu's MIDI LEARN row: FORGET ALL? asked (ui_menu.c) */

/* "T1 CUTOFF" for parameter pid of track tr */
static void mlearn_name(char *b, uint32_t n, uint32_t tr, uint32_t pid)
{
    const param_desc_t *d = pid_desc(pid, tr);
    str_cpy(b, "T1 ", n);
    b[1] = (char)('1' + tr % NTRK);
    str_cpy(b + 3, d && d->label ? d->label : "", n - 3u);
}
/* ui_input.c page_knob: KNOB k turned while learning */
static void mlearn_take(uint32_t k)
{
    __typeof__(help) keep = help;                       /* (the help card's own state left as it was) */
    uint32_t keep_tr = help_tr, pid, tr;
    const pid_entry_t *e;
    char b[28];
    help_find(k);
    pid = help.pid;
    tr = help_tr % NTRK;                                /* (MIXER: KNOB k is track k's) */
    help = keep;
    help_tr = keep_tr;
    if (pid == PID_NONE || !(e = pid_find(pid)) || (e->flags & (PID_F_ACTION | PID_F_DEVICE)))
        return;                                         /* (no parameter, or not one a CC may set: midi_cc.c) */
    mlearn_want.on = 1;
    mlearn_want.track = (uint8_t)tr;
    mlearn_want.pid = (uint16_t)pid;
    mlearn_name(b, sizeof b, tr, pid);
    ui_say(b, "");                                      /* (the header has 15 cells: the name, then the ask) */
    str_cpy(ui.msg2, "SEND A CC", sizeof ui.msg2);
}
static void mlearn_bind(uint32_t cc)
{
    uint32_t k;
    char b[40];
    fm1_irq_off();
    for (k = 0; k < MLEARN_N; k++)                      /* the CC leaves its old knob, the knob its old CC */
        if (mlearn[k].cc == cc || (mlearn[k].cc && mlearn[k].track == mlearn_want.track && mlearn[k].pid == mlearn_want.pid))
            mlearn[k].cc = 0;
    for (k = 0; k < MLEARN_N && mlearn[k].cc; k++) ;
    if (k < MLEARN_N) {
        mlearn[k].track = mlearn_want.track;
        mlearn[k].pid = mlearn_want.pid;
        mlearn[k].cc = (uint8_t)cc;                     /* (the MIDI handler matches it from here) */
    }
    fm1_irq_on();
    mlearn_dirty = 1;                                   /* (settings_poll saves it) */
    if (k == MLEARN_N) {
        ui_message("LEARN FULL");
        return;
    }
    mlearn_want.on = 0;
    str_cpy(b, "CC ", sizeof b);
    fmt_int(b + 3, (int32_t)cc);
    str_cpy(b + str_len(b), " =", sizeof b - str_len(b));
    ui_say(b, "");                                      /* "CC 85 =", then "T1 CUTOFF" */
    mlearn_name(ui.msg2, sizeof ui.msg2, mlearn_want.track, mlearn_want.pid);
}
/* each UI pass (ui_input.c): a CC sent while a knob waits binds it */
static void mlearn_poll(void)
{
    int32_t cc = mlearn_seen;
    if (cc < 0)
        return;
    mlearn_seen = -1;
    if (mlearn_on && mlearn_want.on)
        mlearn_bind((uint32_t)cc);
}
/* each UI pass: while learning, the header says so once any other message is over (the knob waiting for its CC, else
 * MIDI LEARN ON); OCT- ends learning (ui_input.c, before the page sees it) */
static void mlearn_show(void)
{
    if (!mlearn_on || ui.msg_t || ui.menu)
        return;
    if (mlearn_want.on) {
        char b[28];
        mlearn_name(b, sizeof b, mlearn_want.track, mlearn_want.pid);
        ui_say(b, "");
        str_cpy(ui.msg2, "SEND A CC", sizeof ui.msg2);
    } else {
        ui_say("MIDI LEARN ON", "");
        str_cpy(ui.msg2, "OCT- ENDS IT", sizeof ui.msg2);
    }
}
static int mlearn_end_key(uint32_t pressed)             /* OCT- pressed while learning: learning ends. 1 if so */
{
    if (!mlearn_on || ui.menu || !((pressed >> panel.btn[B_OCTDN]) & 1u))
        return 0;
    mlearn_on = 0;
    mlearn_want.on = 0;
    ui.msg_t = 0;
    ui_message("LEARN ENDED");
    return 1;
}
/* the menu: learning starts (closing the menu), or every CC forgotten */
static void mlearn_start(void)
{
    mlearn_want.on = 0;
    mlearn_seen = -1;
    mlearn_on = 1;
    ui_message("LEARN: A KNOB");
}
static void mlearn_forget(void)
{
    fm1_irq_off();
    memset(mlearn, 0, sizeof mlearn);
    fm1_irq_on();
    mlearn_dirty = 1;
}
