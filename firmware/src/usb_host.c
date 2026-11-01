/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Retains USB host enumeration and input contributed by recursivefieldinstruments (2026).
 * melodee-tess: from upstream keremimo/melodee c14912b; a lost device releases the MIDI notes through
 * midi_in_overflow (Tessera keeps no per-source note owners), MIDI out is midi_out.c's queue. */
/* USB MIDI host on USB0 (MELODEE_USB_HOST), included by usb.c after the SIE helpers and ep1_take.
 *
 * The port is either the computer-facing device (usb.c) or, when the menu's USB HOST is on, the host of a class
 * compliant USB-MIDI controller, alone or behind a hub, or behind a second hub on that one (USB-C hubs often have
 * two inside). The choice is never saved: every boot is a device, so
 * the update paths always work. Switching is the main loop's (uh_task); usb.up is 0 while the port is a host.
 *
 *   main loop (uh_task)   waits for a device, resets the bus, and enumerates it with blocking EP0 transfers
 *                         (hub: powers the ports, resets those with something on them, and tries each device
 *                         until one has a USB-MIDI streaming interface). Failed direct probes wait for unplug;
 *                         idle hubs are polled for downstream connection changes. What each probe found is kept
 *                         for ABOUT (uh_dg, uh_diag_line). The ISR does nothing until uh.run.
 *   TIMER5 (uh_poll)      once uh.run is 1 the SIE is the ISR's alone: optional IN and OUT endpoints share host
 *                         EP1 in separate directions. IN is asked for a packet (ReqPkt), read when RxPktRdy, and its channel / clock events
 *                         go to the same ring as USB and TRS MIDI (source 1). A stall or a run of errors sets
 *                         uh.fault; the main loop then resets the bus and looks for a device again.
 *
 * Register sequences follow the AC79 SDK's USB0 host code (usb_h_sie_init, usb_h_chirp_and_reset, usb0_h_ep_read,
 * usb_ctlXfer): EP0 data moves through ep0buf / EP0_CNT as in the device driver, the target is FADDR (USB0 has no
 * per-endpoint function address), RXMAXP is 0xFF as everywhere in this firmware. Not driven: VBUS (the port
 * needs outside power, e.g. a powered hub), low-speed devices (direct or behind a hub: no PRE packets), a third hub. OUT drains the existing MIDI track queue only
 * after the target ACKs; no SysEx/editor traffic is sent to attached devices.
 * Host OUT is tested against the SDK register definitions and a SIE model, not on hardware: see BUILDING.md. */
#if MELODEE_USB_HOST
#include "usb_host_desc.h"

enum { S_DEVCTL = 15, S_TXTYPE = 24, S_TXINTERVAL = 25, S_RXTYPE = 26, S_RXINTERVAL = 27 };
enum { UH_MSG_NONE, UH_MSG_ON, UH_MSG_OFF, UH_MSG_MIDI, UH_MSG_NOMIDI, UH_MSG_LOW, UH_MSG_LOST };
#define UH_FIRST_ADDR 1u
#define UH_MAX_ERRORS 16u
#define UH_HUBS 2u                                      /* the hub on the port and one behind it */
#define UH_HUB_PORTS 7u                                 /* ports read per hub (a 9-byte hub descriptor) */
/* What one probe found (uh_probe, uh_hub_port_probe; ABOUT shows it). UH_R_MIDI is the one success */
enum { UH_R_NONE, UH_R_EMPTY, UH_R_LOW, UH_R_RESET, UH_R_ENABLE, UH_R_DESC, UH_R_ADDR, UH_R_CFG, UH_R_NOMIDI,
       UH_R_SETCFG, UH_R_HUB, UH_R_HUBFAIL, UH_R_DEEP, UH_R_MIDI };
typedef struct {
    uint8_t addr, mps, ports;                           /* addr 0: no hub in this slot (or it stopped answering) */
    uint8_t up;                                         /* hub 1: the port of hub 0 it hangs on */
} uh_hub_t;

static uint8_t uh_rx[64 + 4] __attribute__((aligned(4)));
static uint8_t uh_cfg[320];                                  /* a configuration, cut short if longer */
static struct {
    uint8_t want;                                       /* the menu (main loop) */
    uint8_t on;                                         /* the port is a host */
    volatile uint8_t run;                               /* TIMER5 owns the active MIDI endpoints */
    volatile uint8_t fault;                             /* TIMER5 -> main loop: reset the bus and look again */
    uint8_t seen, next_addr, dev, tried, cls;           /* cls: the last probed device's class (uh_probe) */
    uint16_t port_st;                                   /* the last hub port status read (uh_port_reset) */
    uh_hub_t hub[UH_HUBS];
    uint8_t poll_hub, poll_port, dg_n;                  /* dg_n: counts uh_dg's changes (the ABOUT screen redraws) */
    uint8_t in_ep, out_mps, tx_count, tx_errors, line_idle;
    uint8_t msg;                                        /* one UH_MSG_* for main.c to show */
    volatile uint8_t errors;
    uint32_t t_ms, retry_ms;
    uint32_t tx_ms, hub_poll_ms;
    volatile uint32_t pkts, events;
} uh;
/* The last scan, for ABOUT: the device on the port and, per hub, its ports' UH_R_* (0: not reached), the device
 * class (an interface's when the device has none) and wPortStatus (after the reset, when there was one). ports[h]
 * 0: no hub h; nbr[h] its own port count (only the first UH_HUB_PORTS are read) */
static struct {
    uint8_t root, root_cls, ports[UH_HUBS], nbr[UH_HUBS], up[UH_HUBS];
    uint8_t res[UH_HUBS][UH_HUB_PORTS], cls[UH_HUBS][UH_HUB_PORTS];
    uint16_t st[UH_HUBS][UH_HUB_PORTS];
} uh_dg;

/* ------------------------------------------------ main loop: control --- */
static void uh_wait(uint32_t ms)                        /* main loop only */
{
    uint32_t t = fm1_ms;
    while (fm1_ms - t < ms)
        fm1_wdt_feed();
}

/* CSR0 (host) until want_set bits are all set and want_clr are all clear: >= 0 the CSR0, < 0 a stall / error / timeout */
static int uh_csr0(uint32_t want_set, uint32_t want_clr)
{
    uint32_t t = fm1_ms, v;
    for (;;) {
        v = sie_rd(S_CSR0);
        if (v & 0x04u) {                                /* RxStall */
            sie_wr(S_CSR0, 0);
            return -2;
        }
        if (v & 0x10u) {                                /* Error: no answer */
            sie_wr(S_CSR0, 0);
            return -1;
        }
        if ((v & want_set) == want_set && !(v & want_clr))
            return (int)v;
        if (fm1_ms - t > 500u)
            return -3;
        fm1_wdt_feed();
    }
}

/* One control transfer to `addr`: SETUP, the IN data stage if any, the status stage. Bytes read, or < 0.
 * Only requests without an OUT data stage (every one used here). */
static int uh_ctl(uint8_t addr, uint8_t rt, uint8_t rq, uint16_t val, uint16_t idx, uint16_t len, uint8_t *out,
                  uint8_t mps)
{
    uint32_t got = 0, n, i;
    int r;
    sie_wr(S_FADDR, addr);
    sie_wr(S_INDEX, 0);
    ep0buf[0] = rt;
    ep0buf[1] = rq;
    ep0buf[2] = (uint8_t)val;
    ep0buf[3] = (uint8_t)(val >> 8);
    ep0buf[4] = (uint8_t)idx;
    ep0buf[5] = (uint8_t)(idx >> 8);
    ep0buf[6] = (uint8_t)len;
    ep0buf[7] = (uint8_t)(len >> 8);
    fm1_usb_ep0_send(ep0buf, 8);
    sie_wr(S_CSR0, 0x0Au);                              /* SetupPkt | TxPktRdy */
    r = uh_csr0(0, 0x02u);
    if (r < 0)
        return r;
    if (len && (rt & 0x80u)) {
        while (got < len) {
            sie_wr(S_CSR0, 0x20u);                      /* ReqPkt */
            r = uh_csr0(0x01u, 0);
            if (r < 0)
                return r;
            n = sie_rd(S_COUNT0);
            if (n > 64u)
                n = 64u;
            fm1_usb_rx_sync();
            for (i = 0; i < n && got + i < len; i++)
                out[got + i] = ep0buf[i];
            sie_wr(S_CSR0, 0);                          /* RxPktRdy done */
            got += n;
            if (n < mps)
                break;
        }
        if (got > len)
            got = len;
        sie_wr(S_CSR0, 0x42u);                          /* status: StatusPkt | TxPktRdy (zero-length OUT) */
        r = uh_csr0(0, 0x02u);
    } else {
        sie_wr(S_CSR0, 0x60u);                          /* status: StatusPkt | ReqPkt (zero-length IN) */
        r = uh_csr0(0x01u, 0);
        if (r >= 0)
            sie_wr(S_CSR0, 0);
    }
    return r < 0 ? r : (int)got;
}

/* ---------------------------------------------------- enumeration --- */
static int uh_hub_probe(uint8_t h, uint8_t up, uint8_t addr, uint8_t mps, uint8_t cfgval, uh_midi_t *m);

/* The device at address 0 (just reset), `depth` hubs down (`up`: the hub port it is on): give it an address, read its
 * configuration, and if it has a USB-MIDI streaming interface select it; a hub is followed while a slot is free.
 * A UH_R_*: UH_R_MIDI found, else what stopped it. uh.cls: the device's class (9 for a hub, after its ports). */
static int uh_probe(uint8_t depth, uint8_t up, uh_midi_t *m)
{
    uint8_t d8[8], mps, addr = uh.next_addr++;
    uint32_t total, i;
    int r;
    uh.cls = 0;
    r = uh_ctl(0, 0x80, 6, 0x0100, 0, 8, d8, 8);        /* GET_DESCRIPTOR(device), first 8 bytes */
    if (r < 8)
        return UH_R_DESC;
    mps = d8[7] == 16u || d8[7] == 32u || d8[7] == 64u ? d8[7] : 8u;
    uh.cls = d8[4];
    if (uh_ctl(0, 0x00, 5, addr, 0, 0, 0, mps) < 0)     /* SET_ADDRESS */
        return UH_R_ADDR;
    uh_wait(10);                                        /* the device settles to its new address */
    r = uh_ctl(addr, 0x80, 6, 0x0200, 0, 9, uh_cfg, mps);
    if (r < 9)
        return UH_R_CFG;
    total = (uint32_t)(uh_cfg[2] | uh_cfg[3] << 8);
    if (total > sizeof uh_cfg)
        total = sizeof uh_cfg;
    if (total > 9u) {
        r = uh_ctl(addr, 0x80, 6, 0x0200, 0, (uint16_t)total, uh_cfg, mps);
        if (r < 9)
            return UH_R_CFG;
        total = (uint32_t)r;
    }
    for (i = 0; !uh.cls && i + 5u < total && uh_cfg[i]; i += uh_cfg[i])   /* class 0: the first interface's */
        if (uh_cfg[i + 1] == 4u)
            uh.cls = uh_cfg[i + 5];
    if (d8[4] == 9u) {                                  /* a hub */
        r = depth < UH_HUBS && !uh.hub[depth].addr ? uh_hub_probe(depth, up, addr, mps, uh_cfg[5], m) : UH_R_DEEP;
        uh.cls = 9;                                     /* (its ports' probes set it too) */
        return r;
    }
    if (!uh_parse_midi(uh_cfg, total, m))
        return UH_R_NOMIDI;
    if (uh_ctl(addr, 0x00, 9, m->cfg, 0, 0, 0, mps) < 0)   /* SET_CONFIGURATION */
        return UH_R_SETCFG;
    if (m->alt && uh_ctl(addr, 0x01, 11, m->alt, m->iface, 0, 0, mps) < 0)   /* SET_INTERFACE */
        return UH_R_SETCFG;
    uh.dev = addr;
    return UH_R_MIDI;
}

/* Reset downstream port p of hub h: 0 when it is enabled at full speed, else UH_R_RESET, UH_R_ENABLE, UH_R_LOW */
static int uh_port_reset(uint8_t h, uint8_t p)
{
    uint8_t b[4], addr = uh.hub[h].addr, mps = uh.hub[h].mps;
    uint32_t t;
    if (uh_ctl(addr, 0x23, 3, 4, p, 0, 0, mps) < 0)    /* SET_FEATURE(PORT_RESET) */
        return UH_R_RESET;
    t = fm1_ms;
    for (;;) {
        if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) >= 4 && (b[2] & 0x10u))   /* C_PORT_RESET */
            break;
        if (fm1_ms - t > 500u)
            return UH_R_RESET;
        uh_wait(10);
    }
    uh_ctl(addr, 0x23, 1, 20, p, 0, 0, mps);            /* CLEAR_FEATURE(C_PORT_RESET) */
    if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) < 4)
        return UH_R_ENABLE;
    uh.port_st = (uint16_t)(b[0] | b[1] << 8);
    if (!(b[0] & 0x02u))                                /* enabled? */
        return UH_R_ENABLE;
    if (b[1] & 0x02u)                                   /* low speed, known for sure after the reset */
        return UH_R_LOW;
    uh_wait(20);                                        /* reset recovery */
    return 0;
}

/* Probe only port p of hub h (status st); keep the hubs and the other devices configured, and note what was found.
 * Each port reuses its address on reconnect (hub 0's ports 2..8, hub 1's 9..15), so repeated hot-plugs cannot
 * exhaust the USB address space. A low-speed device is not probed: behind a hub it needs PRE packets, which the
 * SIE does not send. A UH_R_*. */
static int uh_hub_port_probe(uint8_t h, uint8_t p, uint32_t st, uh_midi_t *m)
{
    int r;
    uh.cls = 0;
    uh.port_st = (uint16_t)st;
    if (!h && uh_dg.ports[1] && uh_dg.up[1] == p) {     /* hub 1 hung here: it is gone, or reset below */
        uh.hub[1].addr = 0;
        uh_dg.ports[1] = 0;
    }
    r = !(st & 0x0001u) ? UH_R_EMPTY : st & 0x0200u ? UH_R_LOW : uh_port_reset(h, p);
    if (!r) {
        uh.next_addr = (uint8_t)(UH_FIRST_ADDR + h * UH_HUB_PORTS + p);
        r = uh_probe((uint8_t)(h + 1u), p, m);
    }
    uh_dg.res[h][p - 1u] = (uint8_t)(uh.cls == 9u && r == UH_R_MIDI ? UH_R_HUB : r);
    uh_dg.cls[h][p - 1u] = uh.cls;
    uh_dg.st[h][p - 1u] = uh.port_st;
    uh.dg_n++;
    return r;
}

/* Read and acknowledge connection changes, including disconnects and unsupported devices. An unresponsive
 * hub is left alone (with the hub behind it) until root unplug/replug, rather than repeating a blocking control
 * timeout forever. */
static int uh_hub_status(uint8_t h, uint8_t p, uint8_t *b)
{
    uint8_t addr = uh.hub[h].addr, mps = uh.hub[h].mps;
    if (uh_ctl(addr, 0xA3, 0, 0, p, 4, b, mps) < 4 ||
        ((b[2] & 1u) && uh_ctl(addr, 0x23, 1, 16, p, 0, 0, mps) < 0)) {
        uh.hub[h].addr = 0;
        if (!h)
            uh.hub[1].addr = 0;
        return 0;
    }
    return 1;
}

/* Configure and power hub h once (hub 1 on hub 0's port up) and probe its ports until a MIDI device: UH_R_MIDI,
 * UH_R_HUB (none found) or UH_R_HUBFAIL. After the scan, idle polling uses connection-change bits. */
static int uh_hub_probe(uint8_t h, uint8_t up, uint8_t addr, uint8_t mps, uint8_t cfgval, uh_midi_t *m)
{
    uint8_t b[16], ports, p;                            /* (the hub descriptor is 9 bytes with <= 7 ports) */
    uint32_t good;
    if (uh_ctl(addr, 0x00, 9, cfgval, 0, 0, 0, mps) < 0)
        return UH_R_HUBFAIL;
    if (uh_ctl(addr, 0xA0, 6, 0x2900, 0, 9, b, mps) < 7)   /* hub descriptor */
        return UH_R_HUBFAIL;
    ports = b[2] > UH_HUB_PORTS ? (uint8_t)UH_HUB_PORTS : b[2];
    good = (uint32_t)b[5] * 2u;                         /* bPwrOn2PwrGood, in 2 ms */
    for (p = 1; p <= ports; p++)
        uh_ctl(addr, 0x23, 3, 8, p, 0, 0, mps);         /* SET_FEATURE(PORT_POWER) */
    uh_wait((good < 100u ? 100u : good) + 100u);        /* power good, then the devices announce themselves */
    uh.hub[h] = (uh_hub_t){addr, mps, ports, up};
    uh.hub_poll_ms = fm1_ms;
    uh_dg.ports[h] = ports;
    uh_dg.nbr[h] = b[2];
    uh_dg.up[h] = up;
    memset(uh_dg.res[h], 0, sizeof uh_dg.res[h]);
    for (p = 1; p <= ports; p++) {
        if (!uh_hub_status(h, p, b))
            return UH_R_HUBFAIL;
        if (uh_hub_port_probe(h, p, (uint32_t)(b[0] | b[1] << 8), m) == UH_R_MIDI)
            return UH_R_MIDI;
    }
    return UH_R_HUB;
}

/* the next hub port the idle poll reads (uh.poll_hub, uh.poll_port), 0 when no hub answers */
static int uh_poll_next(void)
{
    uint32_t k;
    for (k = 0; k < UH_HUBS * UH_HUB_PORTS; k++) {
        if (++uh.poll_port > UH_HUB_PORTS) {
            uh.poll_port = 1;
            uh.poll_hub = (uint8_t)((uh.poll_hub + 1u) % UH_HUBS);
        }
        if (uh.hub[uh.poll_hub].addr && uh.poll_port <= uh.hub[uh.poll_hub].ports)
            return 1;
    }
    return 0;
}

static void uh_hubs_off(void)
{
    uint32_t h;
    for (h = 0; h < UH_HUBS; h++)
        uh.hub[h].addr = 0;
}

static int uh_dg_low(void)                              /* the last scan met a low-speed device */
{
    uint32_t h, p;
    if (uh_dg.root == UH_R_LOW)
        return 1;
    for (h = 0; h < UH_HUBS; h++)
        for (p = 0; p < uh_dg.ports[h]; p++)
            if (uh_dg.res[h][p] == UH_R_LOW)
                return 1;
    return 0;
}

static char *uh_put(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
    return d;
}

static char *uh_hex(char *d, uint32_t v, uint32_t digits)
{
    static const char HEX[] = "0123456789ABCDEF";
    while (digits--)
        *d++ = HEX[v >> digits * 4u & 15u];
    *d = 0;
    return d;
}

/* ABOUT's USB HOST lines (ui_menu.c): line i of the last scan into b (24 bytes), 0 past the last. The device on the
 * port ("ROOT FS HUB 09"), each hub ("HUB 1: 4 PORTS", "HUB 2 (P2): 4 PORTS": the second on hub 1's port 2) and
 * after it its ports: the number, wPortStatus in hex (1 connected, 2 enabled, 100 powered, 200 low speed, 400 high
 * speed), the result and, once a descriptor was read, the class in hex: "P1 0100 EMPTY", "P2 0103 NO MIDI 08",
 * "P3 0301 LOW SPEED", "P4 0103 MIDI 01". Ports not reached (a MIDI device was found before them) are left out. */
static int uh_diag_line(uint32_t i, char *b)
{
    static const char *const RES[] = {"", "EMPTY", "LOW SPEED", "RESET FAIL", "NOT ENABLED", "NO ANSWER",
                                      "ADDRESS FAIL", "CONFIG FAIL", "NO MIDI", "SET CFG FAIL", "HUB", "HUB FAIL",
                                      "HUB SKIPPED", "MIDI", "?", "?"};
    uint32_t h, p, r = uh_dg.root, cls = uh_dg.root_cls;
    char *d = uh_put(b, r == UH_R_LOW ? "ROOT " : "ROOT FS ");
    if (!r)
        return 0;
    for (h = 0; i && h < UH_HUBS; h++) {
        if (!uh_dg.ports[h])
            continue;
        if (!--i) {                                     /* the hub's own line */
            d = uh_put(b, "HUB ");
            *d++ = (char)('1' + h);
            if (h) {
                d = uh_put(d, " (P");
                *d++ = (char)('0' + uh_dg.up[h]);
                *d++ = ')';
            }
            d = uh_put(d, ": ");
            if (uh_dg.nbr[h] >= 100u)
                *d++ = (char)('0' + uh_dg.nbr[h] / 100u);
            if (uh_dg.nbr[h] >= 10u)
                *d++ = (char)('0' + uh_dg.nbr[h] / 10u % 10u);
            *d++ = (char)('0' + uh_dg.nbr[h] % 10u);
            uh_put(d, uh_dg.nbr[h] == 1u ? " PORT" : " PORTS");
            return 1;
        }
        for (p = 0; i && p < uh_dg.ports[h]; p++)
            if (uh_dg.res[h][p] && !--i) {
                d = uh_put(b, "P");
                *d++ = (char)('1' + p);
                *d++ = ' ';
                d = uh_hex(d, uh_dg.st[h][p], 4);
                d = uh_put(d, " ");
                r = uh_dg.res[h][p];
                cls = uh_dg.cls[h][p];
            }
    }
    if (i)
        return 0;
    d = uh_put(d, RES[r & 15u]);
    if (r >= UH_R_ADDR) {                               /* (the device descriptor was read) */
        *d++ = ' ';
        uh_hex(d, cls, 2);
    }
    return 1;
}

/* Host EP1 has independent RX/TX DMA slots. Both target endpoints belong to this device and alternate setting. */
static void uh_start_ep(const uh_midi_t *m)
{
    sie_wr(S_FADDR, uh.dev);
    sie_wr(S_INDEX, 1);
    uh.in_ep = m->in_ep;
    uh.out_mps = (uint8_t)m->out_mps;
    uh.tx_count = uh.tx_errors = 0;
    uh.line_idle = 0;
    if (m->in_ep) {
        fm1_usb_ep_rxbuf(1, uh_rx);
        sie_wr(S_RXMAXP, 0xFF);
        sie_wr(S_RXCSR1, 0x90);                         /* clear data toggle, flush */
        sie_wr(S_RXCSR2, 0);
        sie_wr(S_RXTYPE, (uint32_t)(m->in_type << 4 | m->in_ep));
        sie_wr(S_RXINTERVAL, m->in_type == 3u ? m->in_interval : 0u);
        sie_wr(S_RXCSR1, 0x20u);                        /* ReqPkt */
    }
    if (m->out_ep) {
        fm1_usb_ep_txbuf(1, ep1tx);
        sie_wr(S_TXMAXP, 0xFF);                         /* AC79 USB0's DMA maximum, as in the device driver */
        sie_wr(S_TXCSR1, 0x48);                         /* ClrDataTog | FlushFIFO, only at endpoint start */
        sie_wr(S_TXCSR2, 0x20);                         /* MODE: TX half of a bidirectional endpoint */
        sie_wr(S_TXTYPE, (uint32_t)(m->out_type << 4 | m->out_ep));
        sie_wr(S_TXINTERVAL, m->out_type == 3u ? m->out_interval : 0u);
    }
    fm1_usb_ep_enable(1u << 1);
    uh.errors = 0;
    uh.fault = 0;
    RING_PUBLISH();
    uh.run = 1;
}

/* Stop admitting output before discarding queued notes. The device cannot send the missing note-offs: the audio
 * ISR releases every MIDI-held note as after an input overflow (events_block; TRS-held ones too, which is rare). */
static void uh_session_end(void)
{
    uh.run = 0;
    uh.out_mps = 0;
    uh.tx_count = 0;
    mo_r = mo_w;
    RING_PUBLISH();
    midi_in_overflow = 1;
}

/* the controller's port as a host with nothing attached yet */
static void uh_host_up(void)
{
    usb.timeouts = 0;
    fm1_usb_reset();
    uh_wait(25);
    fm1_usb_host_attach(ep0buf);
    uh_wait(25);
    sie_wr(S_INTRUSBE, 0);
    sie_wr(S_INTRTX1E, 0);
    sie_wr(S_INTRTX2E, 0);
    sie_wr(S_INTRRX1E, 0);
    sie_wr(S_INTRRX2E, 0);
    fm1_usb_ep_enable(0x1Fu);
    sie_wr(S_FADDR, 0);
    sie_wr(S_DEVCTL, 0x03u);                            /* session + host request */
    uh.seen = 0;
    uh.line_idle = 0;
    uh_hubs_off();
}

/* a device is on the line: reset the bus and find the MIDI endpoint */
static void uh_connect(void)
{
    uh_midi_t m;
    int r;
    memset(&uh_dg, 0, sizeof uh_dg);
    uh.dg_n++;
    if (sie_rd(S_DEVCTL) & 0x20u) {                     /* low speed (FSDev is bit 6, LSDev bit 5) */
        uh_dg.root = UH_R_LOW;
        if (!uh.tried)
            uh.msg = UH_MSG_LOW;                        /* said once per attach, as NO MIDI is */
        uh.tried = 1;
        return;
    }
    sie_wr(S_POWER, 0x08u);                             /* bus reset */
    uh_wait(30);
    sie_wr(S_POWER, 0);
    uh_wait(30);
    uh.next_addr = UH_FIRST_ADDR;
    uh_hubs_off();
    r = uh_probe(0, 0, &m);
    uh_dg.root = (uint8_t)(uh.cls == 9u && r == UH_R_MIDI ? UH_R_HUB : r);
    uh_dg.root_cls = uh.cls;
    if (r == UH_R_MIDI) {
        uh_start_ep(&m);
        uh.msg = UH_MSG_MIDI;
        uh.tried = 0;
    } else {
        if (!uh.tried)                                  /* said once per attach; LOW SPEED when that was behind */
            uh.msg = uh_dg_low() ? UH_MSG_LOW : UH_MSG_NOMIDI;   /* a hub too */
        uh.tried = 1;
    }
}

/* Main loop. The menu's USB HOST (uh.want) switches the port; as a host this finds and keeps a controller. */
static void uh_task(uint32_t now)
{
    if (uh.want != uh.on) {
        usb.up = 0;                                     /* TIMER5 leaves the SIE to this context */
        usb.config = 0;
        usb.suspended = 0;
        uh_session_end();
        uh.fault = 0;
        uh.tried = 0;
        if (uh.want) {
#if MELODEE_USB_AUDIO
            ua_hw_stop();
#endif
            usb.sx_on = 0;
            fm1_usb_off();
            uh.on = 1;
            uh_host_up();
            uh.retry_ms = now;
            uh.msg = UH_MSG_ON;
        } else {
            uh.on = 0;
            fm1_usb_off();
            usb.retry_ms = now - 1000u;                 /* usb_retry makes the port a device again at once */
            uh.msg = UH_MSG_OFF;
        }
        return;
    }
    if (!uh.on)
        return;
    if (uh.fault) {                                     /* stall, or the controller stopped answering (TIMER5 cleared run) */
        uh_session_end();
        uh.fault = 0;
        uh_host_up();
        uh.retry_ms = now + 500u;
        uh.msg = UH_MSG_LOST;
        return;
    }
    if (uh.run)
        return;
    if ((int32_t)(now - uh.t_ms) < 20 || (int32_t)(now - uh.retry_ms) < 0)
        return;
    uh.t_ms = now;
    if (!fm1_usb_line()) {
        uh.seen = 0;
        if (uh.line_idle < 5u) uh.line_idle++;
        if (uh.line_idle >= 5u) {                       /* debounce root disconnect while idle too */
            uh.tried = 0;
            uh_hubs_off();
        }
        return;
    }
    uh.line_idle = 0;
    if (uh.tried) {
        if ((uint32_t)(now - uh.hub_poll_ms) >= 100u && uh_poll_next()) {   /* one hub port each 100 ms */
            uint8_t b[4], h = uh.poll_hub, p = uh.poll_port;
            uh_midi_t m;
            int r;
            uh.hub_poll_ms = now;
            if (uh_hub_status(h, p, b) && (b[2] & 1u)) {   /* a connection change: the port again */
                if (b[0] & 1u) {
                    uh_wait(100);                      /* newly attached device settles before port reset */
                    if (!uh_hub_status(h, p, b))
                        return;
                }
                r = uh_hub_port_probe(h, p, (uint32_t)(b[0] | b[1] << 8), &m);
                if (r == UH_R_MIDI) {
                    uh_start_ep(&m);
                    uh.msg = UH_MSG_MIDI;
                    uh.tried = 0;
                } else if (r != UH_R_EMPTY) {
                    uh.msg = r == UH_R_LOW ? UH_MSG_LOW : UH_MSG_NOMIDI;
                }
            }
        }
        return;                                        /* direct failures wait for root unplug/replug */
    }
    if (uh.seen < 5u) {                                 /* 100 ms of a steady line before the bus reset */
        uh.seen++;
        return;
    }
    uh.seen = 0;
    uh_connect();
}

/* ------------------------------------------------------ TIMER5 ISR --- */
static void uh_fail(void)
{
    uh.run = 0;
    uh.fault = 1;
}

/* Retain ring slots and the DMA buffer until ACK. A NAK or transient error retries the same data toggle;
 * a stall, repeated errors or 500 ms without completion hands recovery to the main loop. Never wait here. */
static void uh_tx_poll(void)
{
    uint32_t csr = sie_rd(S_TXCSR1), n = 0;
    if (uh.tx_count) {
        if ((csr & 0x20u) || fm1_ms - uh.tx_ms >= 500u) {
            uh_fail();
            return;
        }
        if (csr & 0x84u) {                              /* Error / NAK timeout */
            if ((csr & 4u) && ++uh.tx_errors >= UH_MAX_ERRORS) {
                uh_fail();
                return;
            }
            sie_wr(S_TXCSR1, 0x01u);                    /* retry retained FIFO, preserving data toggle */
            return;
        }
        if (csr & 0x03u) return;                        /* TxPktRdy / FIFONotEmpty: no ACK yet */
        RING_PUBLISH();
        mo_r += uh.tx_count;
        uh.tx_count = uh.tx_errors = 0;
    }
    while (mo_r + n / 4u != mo_w && n + 4u <= uh.out_mps) {
        uint32_t p = midi_out_q[(mo_r + n / 4u) % MQ];
        ep1tx[n] = (uint8_t)p;
        ep1tx[n + 1] = (uint8_t)(p >> 8);
        ep1tx[n + 2] = (uint8_t)(p >> 16);
        ep1tx[n + 3] = (uint8_t)(p >> 24);
        n += 4u;
    }
    if (!n) return;
    uh.tx_count = (uint8_t)(n / 4u);
    uh.tx_ms = fm1_ms;
    fm1_usb_ep_send(1, ep1tx, n);
    sie_wr(S_TXCSR1, 0x01u);
}

static void uh_poll(void)
{
    uint32_t csr, n, i;
    if (!fm1_usb_line()) {
        /* EOP also drives SE0 briefly. Require 8 ms of idle samples, not one sample during a packet. */
        if (++uh.line_idle >= 16u) uh_fail();
        return;
    }
    uh.line_idle = 0;
    sie_wr(S_INDEX, 1);
    if (uh.out_mps) uh_tx_poll();
    if (!uh.run || !uh.in_ep) return;
    csr = sie_rd(S_RXCSR1);
    if (csr & 0x40u) {                                  /* RxStall: the endpoint is gone */
        uh_fail();
        return;
    }
    if (csr & 0x0Cu) {                                  /* Error / DataError: no usable answer */
        if (++uh.errors >= UH_MAX_ERRORS) {
            uh_fail();
            return;
        }
        sie_wr(S_RXCSR1, 0x20u);                        /* the flags clear (written 0) and the request goes again; the
                                                         * data toggle stays: it is reset when the endpoint starts only */
        return;
    }
    if (csr & 0x01u) {                                  /* a packet is in uh_rx */
        n = sie_rd(S_RXCOUNT1) | (sie_rd(S_RXCOUNT2) << 8);
        if (n > 64u)
            n = 64u;
        fm1_usb_rx_sync();
#ifndef MELODEE_LOADER
        if (MQ - (mi_w - mi_r) < EP1_ROOM)
            return;                                     /* ring too full: the packet stays, retried next poll */
#endif
        for (i = 0; i + 3u < n; i += 4u) {
            uint32_t p = uh_midi_ok((uint32_t)uh_rx[i] | (uint32_t)uh_rx[i + 1] << 8 |
                                    (uint32_t)uh_rx[i + 2] << 16 | (uint32_t)uh_rx[i + 3] << 24);
            if (p) {
                midi_enqueue(p, 1u);
                uh.events++;
            }
        }
        uh.pkts++;
        uh.errors = 0;
        sie_wr(S_RXCSR1, 0x10u);                        /* release the packet */
        sie_wr(S_RXCSR1, 0x20u);                        /* and ask for the next */
        return;
    }
    if (!(csr & 0x20u))                                 /* nothing pending: ask */
        sie_wr(S_RXCSR1, 0x20u);
}
#endif /* MELODEE_USB_HOST */
