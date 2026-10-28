/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis (melodee-tess) */
/* PIDS entry i's help text (from PID_HELP_N on: ui_help.c HELP_PAGE's) out of build/gen/pid_help.h (tools/gen_help.py): each
 * canonical Huffman code read to its symbol (by the code lengths alone: PID_HELP_CNT a length's count, PID_HELP_LEN each
 * symbol's), then expanded on a small stack, a symbol from PID_HELP_LO up being two symbols (PID_HELP_PAIR). Symbol 0 ends
 * the text. out takes PID_HELP_MAX + 1 bytes. */
#include "pid_help.h"

static uint32_t pid_help_pair(uint32_t k)       /* PID_HELP_PAIR's 10-bit field k */
{
    uint32_t b = k * 10u;
    return ((uint32_t)PID_HELP_PAIR[b >> 3] << 8 | PID_HELP_PAIR[(b >> 3) + 1u]) >> (6u - (b & 7u)) & 1023u;
}

static uint32_t pid_help_sym(const uint8_t *p, uint32_t *bit)
{
    uint32_t code = 0, first = 0, n, s;
    for (n = 1; n <= PID_HELP_MAXL; n++) {
        code |= (uint32_t)p[*bit >> 3] >> (7u - (*bit & 7u)) & 1u;
        ++*bit;
        if (code - first < PID_HELP_CNT[n]) {
            uint32_t k = code - first;
            for (s = 0; s < PID_HELP_NSYM; s++)
                if ((PID_HELP_LEN[s >> 1] >> (s & 1u ? 0u : 4u) & 15u) == n && !k--)
                    return s;
            return 0;
        }
        first = (first + PID_HELP_CNT[n]) << 1;
        code <<= 1;
    }
    return 0;
}

static void pid_help_unpack(uint32_t i, char *out)
{
    const uint8_t *p = PID_HELP_DATA + PID_HELP_AT[i % (PID_HELP_N + HELP_PAGE_N)];
    uint16_t st[PID_HELP_DEPTH];
    uint32_t n = 0, bit = 0, s;
    while ((s = pid_help_sym(p, &bit)) != 0) {
        uint32_t sp = 0;
        st[sp++] = (uint16_t)s;
        while (sp) {
            uint32_t c = st[--sp];
            if (c >= PID_HELP_LO && sp + 2u <= PID_HELP_DEPTH) {
                st[sp++] = (uint16_t)pid_help_pair(2u * (c - PID_HELP_LO) + 1u);
                st[sp++] = (uint16_t)pid_help_pair(2u * (c - PID_HELP_LO));
            } else if (c < PID_HELP_LO && n < PID_HELP_MAX) {
                out[n++] = (char)c;
            }
        }
    }
    out[n] = 0;
}
