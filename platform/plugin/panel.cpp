/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window's FM-1 (panel.h): web/sim/index.html's wide layout and its shells, or TESSERA's own shell, drawn
 * into a pixel buffer */
#include "panel.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

extern "C" {
#include "mo_font.h"
}

namespace
{
// ---- colours (web/sim/index.html SHELLS; the LED levels mixed as shellVars does, in linear light)
struct Rgb { double r, g, b; };
Rgb hex(uint32_t c)
{
    auto lin = [](uint32_t v) { return std::pow(v / 255.0, 2.2); };
    return {lin(c >> 16 & 255), lin(c >> 8 & 255), lin(c & 255)};
}
uint32_t pack(Rgb c)
{
    auto enc = [](double v) { return (uint32_t)std::lround(255 * std::pow(v < 0 ? 0 : v > 1 ? 1 : v, 1 / 2.2)); };
    return 0xFF000000u | enc(c.r) << 16 | enc(c.g) << 8 | enc(c.b);
}
uint32_t mix(uint32_t a, uint32_t b, double s)
{
    Rgb x = hex(a), y = hex(b);
    return pack({x.r + (y.r - x.r) * s, x.g + (y.g - x.g) * s, x.b + (y.b - x.b) * s});
}
const uint32_t WELL = 0xFF141415, PART = 0xFF2A2A2D, KEDGE = 0xFF3A3A3E, GREEN = 0xFF5FE08A, GREEN_INK = 0xFF0D2414,
               BEZEL = 0xFF0B0B0C;

// the FM-1's shells, as web/sim/index.html SHELLS has them (keep the two alike), each with the Mono ink made for it:
// body, trays, buttons and keys, their edge, their ink, the slits, the labels on the body, the ink on a lit button,
// a lit button, REC's ink (0: the defaults, the button ink, 0xF6F6F6, 0xC62A22). (mint's LED levels: mixed here, the
// web sim's tuned ones are within a shade)
struct Shell
{
    const char *id, *name, *ink;
    uint32_t body, tray, btn, edge, btn_ink, slit, label, lit_ink, lit, rec;
};
const Shell SHELLS[] = {
    // TESSERA's own (draw_native; these colours only make its swatch among the FM-1's); its screen in MOSAIC, a colour
    // a track, T1 the yellow (Theo, 2026-11-24: Cosmati's inlaid stones)
    {"tessera", "TESSERA", "MOSAIC", 0xFF111111, 0xFF111111, 0xFFFFD020, 0xFF3B3007, 0xFFFFD020, 0xFF3B3007, 0xFFB9B2A0,
     0xFF000000, 0xFFFFD020, 0xFFFFD020},
    {"black", "BLACK", "YELLOW", 0xFF1B1B1D, 0xFF29292C, 0xFF3B3B40, 0xFF505056, 0xFFE4E4E6, 0xFF121214, 0xFFD6D6D8,
     0xFF1B1B1D, 0, 0},
    {"mint", "BLACK / MINT", "GREEN", 0xFF1D1D1F, 0xFF5FC9AD, 0xFF7CD9C0, 0xFF4BB398, 0xFF0F2E27, 0xFF1E3A33, 0xFFD6D6D8,
     0xFF0F2E27, 0xFFF2FFFA, 0},
    {"lavender", "LAVENDER", "LILAC", 0xFFB8ACDF, 0xFFA394D4, 0xFFCBC1EE, 0xFF9283C8, 0xFF2A2240, 0xFF463C6C, 0xFF2A2240,
     0, 0, 0},
    {"orange", "ORANGE", "ORANGE", 0xFFEE8A30, 0xFFDC7720, 0xFFF6A656, 0xFFC86A16, 0xFF3A1E05, 0xFF6A3A10, 0xFF2E1804,
     0, 0, 0},
    {"cream-grey", "CREAM / GREY", "WHITE", 0xFFEBE5D5, 0xFF9B9B9E, 0xFFB6B6B9, 0xFF88888C, 0xFF1E1E20, 0xFF3A3A3D,
     0xFF3A3630, 0, 0, 0},
    {"cream-blue", "CREAM / BLUE", "BLUE", 0xFFEBE5D5, 0xFF4F7FC4, 0xFF6F99D7, 0xFF3E6CB0, 0xFF0D1D36, 0xFF1C3460,
     0xFF3A3630, 0, 0, 0},
    {"cream-brick", "CREAM / BRICK", "RED", 0xFFEBE5D5, 0xFFB4523C, 0xFFC96B53, 0xFF9A4230, 0xFF2B0D06, 0xFF4C1A10,
     0xFF3A3630, 0, 0, 0},
    {"hot-pink", "HOT PINK", "HOT PINK", 0xFFD6346C, 0xFFC42A60, 0xFFCF2F67, 0xFFA91F50, 0xFFFDE8EF, 0xFF5E0F2C,
     0xFFFDE8EF, 0xFF3A0C20, 0, 0xFFFFE08A},
};
const int NSHELLS = (int)(sizeof SHELLS / sizeof SHELLS[0]);

// ---- drawing: anti-aliased by distance at the edges
struct Canvas
{
    uint32_t *px;
    int w;   // the panel's width (its rows' length)
    void blend(int x, int y, uint32_t c, double a)
    {
        if (x < 0 || y < 0 || x >= w || y >= Panel::H || a <= 0)
            return;
        uint32_t &d = px[y * w + x];
        if (a >= 1) { d = c; return; }
        auto ch = [&](int s) {
            double v = (d >> s & 255) * (1 - a) + (c >> s & 255) * a;
            return (uint32_t)(v + 0.5) << s;
        };
        d = 0xFF000000u | ch(16) | ch(8) | ch(0);
    }
    // a rounded rectangle (r = 0: square)
    void rrect(double x, double y, double w, double h, double r, uint32_t c)
    {
        for (int py = (int)std::floor(y); py < (int)std::ceil(y + h); py++)
            for (int px_ = (int)std::floor(x); px_ < (int)std::ceil(x + w); px_++)
            {
                double cx = px_ + 0.5, cy = py + 0.5;
                double dx = std::fmax(std::fmax(x + r - cx, cx - (x + w - r)), 0.0);
                double dy = std::fmax(std::fmax(y + r - cy, cy - (y + h - r)), 0.0);
                double edge = std::fmin(std::fmin(cx - x, x + w - cx), std::fmin(cy - y, y + h - cy)) + 0.5;
                double a = r > 0 && dx > 0 && dy > 0 ? r - std::sqrt(dx * dx + dy * dy) + 0.5 : edge;
                blend(px_, py, c, a > 1 ? 1 : a);
            }
    }
    void circle(double cx, double cy, double r, uint32_t c) { rrect(cx - r, cy - r, 2 * r, 2 * r, r, c); }
    // a disc in four quarters (a multi-ink palette's track colours, clockwise from the top left)
    void quad(double cx, double cy, double r, const uint32_t c[4])
    {
        for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++)
            for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++)
            {
                double dx = x + 0.5 - cx, dy = y + 0.5 - cy, a = r + 0.5 - std::sqrt(dx * dx + dy * dy);
                if (a <= 0) continue;
                int q = dy < 0 ? (dx < 0 ? 0 : 1) : (dx < 0 ? 3 : 2);
                blend(x, y, c[q], a > 1 ? 1 : a);
            }
    }
    void line(double x0, double y0, double x1, double y1, double wd, uint32_t c)
    {
        double len = std::hypot(x1 - x0, y1 - y0);
        int n = (int)(len * 2) + 1;
        for (int i = 0; i <= n; i++)
            circle(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, wd / 2, c);
    }
    // a plain rectangle, no edge blending
    void box(int x, int y, int w, int h, uint32_t c)
    {
        for (int py = y; py < y + h; py++)
            for (int px_ = x; px_ < x + w; px_++)
                blend(px_, py, c, 1);
    }
    // text in the Mono face spaced by each letter's ink (TESSERA's shell): a dot between letters, three for a space
    static void glyph_cols(int ch, int &lo, int &hi)
    {
        lo = 5; hi = -1;
        if (ch < 32 || ch > 'z') return;
        const uint8_t *g = MO_FONT[ch - 32];
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (g[row] >> (4 - col) & 1) { if (col < lo) lo = col; if (col > hi) hi = col; }
    }
    static int ptext_w(const char *t, int s)
    {
        int w = 0;
        for (const char *q = t; *q; q++)
        {
            int lo, hi;
            glyph_cols((unsigned char)*q, lo, hi);
            w += (hi < lo ? 3 : hi - lo + 1) + (q[1] ? 1 : 0);
        }
        return w * s;
    }
    void ptext(int x, int y, const char *t, int s, uint32_t c)   // (x: the left edge)
    {
        for (; *t; t++)
        {
            int ch = (unsigned char)*t, lo, hi;
            glyph_cols(ch, lo, hi);
            if (hi < lo) { x += 4 * s; continue; }
            const uint8_t *g = MO_FONT[ch - 32];
            for (int row = 0; row < 7; row++)
                for (int col = lo; col <= hi; col++)
                    if (g[row] >> (4 - col) & 1)
                        box(x + (col - lo) * s, y + row * s, s, s, c);
            x += (hi - lo + 2) * s;
        }
    }
    void ptext_c(int cx, int y, const char *t, int s, uint32_t c) { ptext(cx - ptext_w(t, s) / 2, y, t, s, c); }
    // text in the Mono face, centred on cx, its top at y; s = pixels per font dot
    static int text_w(const char *t, int s) { return (int)std::strlen(t) * 6 * s - s; }
    void text(int cx, int y, const char *t, int s, uint32_t c)
    {
        int x = cx - text_w(t, s) / 2;
        for (; *t; t++, x += 6 * s)
        {
            int ch = (unsigned char)*t;
            if (ch < 32 || ch > 'z')
                continue;
            const uint8_t *g = MO_FONT[ch - 32];
            for (int row = 0; row < 7; row++)
                for (int col = 0; col < 5; col++)
                    if (g[row] >> (4 - col) & 1)
                        for (int dy = 0; dy < s; dy++)
                            for (int dx = 0; dx < s; dx++)
                                blend(x + col * s + dx, y + row * s + dy, c, 1);
        }
    }
};

// web/sim/index.html: B, EN, ROW1 / ROW2, KNOBS and LAYOUTS.wide
enum { B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC, B_OCTDN, B_OCTUP };
const int PLAY_GREEN = 14;
const int WHITE_K[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
const int BLACK_K[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
const int BLACK_AFTER[11] = {0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14};
const int SCREEN_X = 292, SCREEN_Y = 44, SCREEN_S = 256;
const int KEYBED[4] = {36, 332, 928, 256};   // (EXPANDED: as wide as the window less 72, Panel::kb_w)
// EXPANDED: the TRACKS screen beside the FM-1's (its frame as the FM-1's: 8 px round 480 x 240), and KNOB 1..4 and the
// twelve buttons right of it
const int T2_X = 560, T2_Y = SCREEN_Y, T2_W = TESS_TRACKS_W + 16, T2_H = TESS_TRACKS_H + 16, T2_IN = 8;
const int XK_X = 1124, XK_PITCH = 96, XB_X = 1080, XB_STEP = 64, XB_W = 56;
// TESSERA's shell: OCT+ over OCT- at the keys' left (Theo 2026-11-24), the keys that much narrower
const int OCT_W = 60, OCT_H = 46, OCT_COL = OCT_W + 16;
// its own buttons past the FM-1's (tess_core.h TESS_BTN_*): bit numbers in PanelInput.buttons
const int BIT_SHIFT = 16, BIT_OCT_DOWN = 17, BIT_OCT_UP = 18, BIT_HELP = 19;
}   // namespace

Panel::Panel()
{
    const int K[8][3] = {{105, 100, 24}, {215, 100, 24}, {105, 205, 24}, {215, 205, 24},
                         {610, 100, 22}, {714, 100, 22}, {818, 100, 22}, {922, 100, 22}};
    const char *KN[8] = {"MASTER", "SELECT", "PRESETS", "ALGORITHM", "KNOB 1", "KNOB 2", "KNOB 3", "KNOB 4"};
    const int ROLE[8] = {-1, 0, 2, 1, 3, 4, 5, 6};
    for (int i = 0; i < 8; i++)
        knobs.push_back({K[i][0], K[i][1], K[i][2], KN[i], ROLE[i], 0});
    knobs[0].angle = -135 + 270.0 * master_now / 1023;
    layout();
}

// the buttons and the files where the shell has them: the FM-1's as the web sim's wide layout; TESSERA's grid a little
// wider (room round the words), its files centred under the keys
void Panel::layout()
{
    const bool n = native(), x = expanded();
    for (int i = 4; i < 8; i++)   // KNOB 1..4: over the buttons (EXPANDED: right of TRACKS)
        knobs[i].x = x ? XK_X + (i - 4) * XK_PITCH : 610 + (i - 4) * 104;
    // the keys: the FM-1's 27 across the keybed (EXPANDED: wider, the pitch with it)
    kb_w = x ? width() - 2 * KEYBED[0] : KEYBED[2];
    const int x0 = KEYBED[0] + (n ? OCT_COL : 0), y0 = KEYBED[1], h = KEYBED[3];   // (TESSERA's: OCT+ / OCT- at their left)
    if (n) kb_w -= OCT_COL;
    pitch = (kb_w - 24) / 16;
    kw = (int)(pitch * 0.78);
    kh = (int)(h * 0.425);
    sw = kw * 14 / 100 > 3 ? kw * 14 / 100 : 3;
    keys.clear();
    for (int i = 0; i < 16; i++)
        keys.push_back({x0 + 12 + (int)((i + 0.5) * pitch), y0 + h * 3 / 4, WHITE_K[i]});
    for (int j = 0; j < 11; j++)
        keys.push_back({x0 + 12 + (BLACK_AFTER[j] + 1) * pitch, y0 + h / 4, BLACK_K[j]});
    buttons.clear();
    const int R1[6] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO}, R2[6] = {B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC};
    const char *F1[6] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO"}, *F2[6] = {"HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"};
    const int bx = x ? XB_X : n ? 574 : 582, step = x ? XB_STEP : n ? 66 : 63, bw = x ? XB_W : n ? 60 : 54;
    if (n)   // TESSERA's shell: SHIFT, BACK and OK of their own under the left knobs (OCT-, OCT- held, OCT+ on the FM-1),
    {        // and a third row of the grid, the octave a step down and up (the FM-1's OCT- / OCT+ on Stage)
        buttons.push_back({50, 258, 62, 34, BIT_SHIFT, "SHIFT", nullptr, "CLICK: THE KNOBS' SECOND SET, AGAIN: OFF   KEY: SHIFT"});   // (Theo:
        buttons.push_back({118, 258, 56, 34, B_OCTDN, "BACK", nullptr, "BACK  (THE FM-1'S OCT-)   KEY: ESC"});        // SHIFT, BACK, OK)
        buttons.push_back({180, 258, 50, 34, B_OCTUP, "OK", nullptr, "OK  HOLD: THE PAGE'S SHEET   KEY: ENTER"});
        buttons.push_back({236, 258, 34, 34, BIT_HELP, "?", nullptr, "HELP: TURN A KNOB  (OR RIGHT-CLICK)   KEY: F1"});
        static const char *K1[6] = {"KEY: 1", "KEY: 2", "KEY: 3", "KEY: 4", "KEY: 5", "KEY: 6"},   // (panel.h key)
                          *K2[6] = {"KEY: 7", "KEY: 8", "KEY: 9", "KEY: 0", "KEY: -", "KEY: ="};
        for (int i = 0; i < 6; i++)
        {
            buttons.push_back({bx + i * step, 174, bw, 48, R1[i], F1[i], nullptr, K1[i]});
            buttons.push_back({bx + i * step, 174 + 62, bw, 48, R2[i], F2[i], nullptr,
                               R2[i] == B_PLAY && standalone_ ? "KEY: SPACE  (OR -)" : K2[i]});
        }
        const int oy = KEYBED[1] + KEYBED[3] / 2;   // (OCT+ over OCT-, at the keys' left, round the keybed's middle)
        buttons.push_back({KEYBED[0], oy - 5 - OCT_H, OCT_W, OCT_H, BIT_OCT_UP, "OCT+", nullptr, "THE KEYS AN OCTAVE UP   KEY: PAGE UP"});
        buttons.push_back({KEYBED[0], oy + 5, OCT_W, OCT_H, BIT_OCT_DOWN, "OCT-", nullptr, "THE KEYS AN OCTAVE DOWN   KEY: PAGE DOWN"});
    }
    else
    {
        buttons.push_back({60, 258, 90, 34, B_OCTDN, "OCT-", nullptr});
        buttons.push_back({170, 258, 90, 34, B_OCTUP, "OCT+", nullptr});
        for (int i = 0; i < 6; i++)
        {
            buttons.push_back({bx + i * step, 174, bw, 48, R1[i], F1[i], nullptr});
            buttons.push_back({bx + i * step, 174 + 62, bw, 48, R2[i], F2[i], R2[i] == B_PLAY ? "STOP" : nullptr});
        }
    }
    key_words();
    files.clear();
    const int fy = KEYBED[1] + KEYBED[3] + 5;
    if (!n)
    {
        files.push_back({60, fy, 72, 24, FILE_OPEN, "OPEN"});   // (in the shell, under the keys)
        files.push_back({142, fy, 72, 24, FILE_SAVE, "SAVE"});
        files.push_back({224, fy, 72, 24, FILE_MIDI, "MIDI"});
        banks[0] = {316, fy, 52, 24, 0, "1-4"};   // (one switch, two halves)
        banks[1] = {368, fy, 52, 24, 4, "5-8"};
        usb = {tracks_ > 4 ? 430 : 306, fy, 112, 24, 0, "FM-1 USB"};
        return;
    }
    const int total = 3 * 72 + 2 * 10 + (banked() ? 20 + 104 : 0);   // (EXPANDED: no bank switch, TRACKS shows all)
    int fx = width() / 2 - total / 2;
    files.push_back({fx, fy - 1, 72, 26, FILE_OPEN, "OPEN"});
    files.push_back({fx + 82, fy - 1, 72, 26, FILE_SAVE, "SAVE"});
    files.push_back({fx + 164, fy - 1, 72, 26, FILE_MIDI, "MIDI"});
    banks[0] = {fx + 256, fy - 1, 52, 26, 0, "1-4"};
    banks[1] = {fx + 308, fy - 1, 52, 26, 4, "5-8"};
}

// TESSERA's shell shows BACK, OK and SHIFT of its own: the screen says so too (tess_core_key_words); not while an FM-1
// on USB plays the instance (its OCT- / OCT+ as ever)
void Panel::key_words()
{
    const bool words = native() && !(fm1_on && fm1_mine);
    if (words != input.key_words)
    {
        input.key_words = words;
        input.changed = true;
    }
}

void Panel::set_midi(uint32_t count)
{
    if (midi_known && count != midi_count)
        midi_frame = frame + 1;   // (lit from the next frame for a few)
    midi_count = count;
    midi_known = true;
}

void Panel::set_fm1(bool on, bool mine, bool live, const std::string &text)
{
    bool was = fm1_on;
    std::string before = fm1_text;
    fm1_on = on;
    fm1_mine = mine;
    fm1_live = live;
    fm1_text = text;
    key_words();   // (an FM-1 playing it keeps its OCT- / OCT+)
    if (fm1_known && (on ? text != before : was))
        note(on ? text : "FM-1 USB OFF");
    else if (!fm1_known && on)
        note(text);
    fm1_known = true;
}

int Panel::bank_at(int x, int y) const
{
    if (!banked()) return -1;
    for (int i = 0; i < 2; i++)
        if (x >= banks[i].x && x < banks[i].x + banks[i].w && y >= banks[i].y && y < banks[i].y + banks[i].h)
            return i;
    return -1;
}

void Panel::note(const std::string &text)
{
    note_text.clear();
    for (char ch : text)   // (the Mono face: capitals)
        note_text += ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
    note_frames = 150;     // (5 s at the window's 30 frames a second)
}

void Panel::draw(uint32_t *px, const tess_view_t &v, const tess_tracks_t *t2)
{
    for (int k = 0; k < TESS_KEYS; k++)   // (the keys' jobs on the page: their labels and hover cards)
    {
        std::memcpy(key_short[k], v.key_short[k], sizeof key_short[k]);
        std::memcpy(key_long[k], v.key_long[k], sizeof key_long[k]);
        key_short[k][sizeof key_short[k] - 1] = key_long[k][sizeof key_long[k] - 1] = 0;
    }
    zones.assign(v.zones, v.zones + (v.zone_count < (uint32_t)TESS_ZONES ? v.zone_count : (uint32_t)TESS_ZONES));
    if (expanded() && t2 && t2->w == TESS_TRACKS_W)
        zones2.assign(t2->zones, t2->zones + (t2->zone_count < (uint32_t)TESS_TRACKS_ZONES ? t2->zone_count
                                                                                          : (uint32_t)TESS_TRACKS_ZONES));
    else
        zones2.clear();
    draw_body(px, v, t2);
    draw_hover(px);
}

void Panel::draw_body(uint32_t *px, const tess_view_t &v, const tess_tracks_t *t2)
{
    Canvas c{px, width()};
    const Shell &sh = SHELLS[shell_];
    const uint32_t BODY = sh.body, TRAY = sh.tray, BTN = sh.btn, EDGE = sh.edge, BTN_INK = sh.btn_ink, SLIT = sh.slit,
                   LABEL = sh.label, LIT_INK = sh.lit_ink ? sh.lit_ink : sh.btn_ink, LIT = sh.lit ? sh.lit : 0xFFF6F6F6,
                   REC = sh.rec ? sh.rec : 0xFFC62A22;
    master_now = (int)v.master;
    if (master_mine >= 0 && !(drag == KNOB && knobs[drag_i].role < 0) && frame - master_mine_at > 15)
        master_mine = -1;   // (let go half a second ago: the view has it, or the DAW moved it since)
    knobs[0].angle = -135 + 270.0 * (master_mine >= 0 ? master_mine : master_now) / 1023;
    static const double kPi = 3.14159265358979;
    // the breathing LEDs: dark .. lit and back in ~1.13 s (fm1_led_breath)
    double breath = 0.5 - 0.5 * std::cos(2 * kPi * (frame++ / 30.0) / 1.13);
    if (ink_pending < 0 || (int)v.ink == ink_pending)   // (a pick stays ringed until the view has it)
    {
        ink_now = (int)v.ink;
        ink_pending = -1;
    }
    typing_ = v.typing != 0;
    closable_ = v.closable != 0;
    sheet_ = v.sheet != 0;
    view_hover = v.hover;
    view_help.assign(v.hover_help, strnlen(v.hover_help, sizeof v.hover_help));
    track_now = (int)v.track;
    if (native())
    {
        draw_native(px, v, breath, t2);
        return;
    }
    for (int i = 0; i < W * H; i++)
        px[i] = 0xFF101011;
    c.rrect(4, 4, W - 8, H - 8, 40, BODY);
    c.rrect(50, 250, 220, 50, 14, TRAY);
    c.rrect(568, 160, 400, 140, 14, TRAY);

    // the screen, 1:1 inside its bezel
    c.rrect(SCREEN_X, SCREEN_Y, SCREEN_S, SCREEN_S, 18, BEZEL);
    const int inset = SCREEN_S / 32;
    for (int y = 0; y < TESS_SCREEN; y++)
        for (int x = 0; x < TESS_SCREEN; x++)
        {
            uint16_t p = v.screen[y * TESS_SCREEN + x];
            uint32_t r = (p >> 11 & 31) * 255 / 31, g = (p >> 5 & 63) * 255 / 63, b = (p & 31) * 255 / 31;
            px[(SCREEN_Y + inset + y) * W + SCREEN_X + inset + x] = 0xFF000000u | r << 16 | g << 8 | b;
        }

    // knobs: MASTER shows its position; the encoders turn freely (a mark per detent, 20 a turn)
    for (auto &k : knobs)
    {
        c.circle(k.x, k.y, k.r + 5, WELL);
        c.circle(k.x, k.y, k.r, KEDGE);
        c.circle(k.x, k.y, k.r - 2, PART);
        double a = (k.angle - 90) * kPi / 180;
        c.line(k.x + std::cos(a) * (k.r - 14), k.y + std::sin(a) * (k.r - 14), k.x + std::cos(a) * (k.r - 5),
               k.y + std::sin(a) * (k.r - 5), 3, LABEL);
        c.text(k.x, k.y - k.r - 24, k.name, 2, LABEL);
    }

    // buttons: lit, PLAY green, dim (what is there), breathing (what can be pressed)
    uint32_t held = input.buttons;
    for (auto &b : buttons)
    {
        uint32_t bit = 1u << b.bit;
        bool green = b.bit == B_PLAY && (v.lit_buttons >> PLAY_GREEN & 1);
        uint32_t fill = BTN, ink = b.bit == B_REC ? REC : BTN_INK;
        if (green) { fill = GREEN; ink = GREEN_INK; }
        else if (v.lit_buttons & bit) { fill = LIT; ink = b.bit == B_REC ? REC : LIT_INK; }
        else if (v.breath_buttons & bit) fill = mix(BTN, LIT, 0.75 * breath);
        else if (v.dim_buttons & bit) fill = mix(BTN, LIT, 0.2);
        int dy = held & bit ? 1 : 0;
        c.rrect(b.x, b.y + dy, b.w, b.h, 8, EDGE);
        c.rrect(b.x + 1.5, b.y + 1.5 + dy, b.w - 3, b.h - 3, 7, fill);
        int s = Canvas::text_w(b.face, 2) <= b.w - 8 ? 2 : 1;
        if (b.sub)
        {
            c.text(b.x + b.w / 2, b.y + b.h / 2 - 13 + dy, b.face, s, ink);
            c.text(b.x + b.w / 2, b.y + b.h / 2 + 4 + dy, b.sub, 1, ink);
        }
        else
            c.text(b.x + b.w / 2, b.y + b.h / 2 - 7 * s / 2 + dy, b.face, s, ink);
    }

    // the keys: pills with a slit that lights
    c.rrect(KEYBED[0], KEYBED[1], KEYBED[2], KEYBED[3], 18, TRAY);
    for (auto &k : keys)
    {
        uint32_t bit = 1u << k.k;
        int y = k.cy - kh / 2, dy = input.keys & bit ? 1 : 0;
        c.rrect(k.cx - kw / 2.0 - 1, y - 1 + dy, kw + 2, kh + 2, kw / 2.0 + 1, EDGE);
        c.rrect(k.cx - kw / 2.0, y + dy, kw, kh, kw / 2.0, BTN);
        uint32_t s = SLIT;
        if (v.lit_keys & bit) s = 0xFFFFFFFF;
        else if (v.breath_keys & bit) s = mix(SLIT, 0xFFFFFFFF, 0.74 * breath);
        else if (v.dim_keys & bit) s = mix(SLIT, 0xFFFFFFFF, 0.08);
        c.rrect(k.cx - sw / 2.0, y + kw * 0.4 + dy, sw, kh - kw * 0.8, sw / 2.0, s);
    }
    draw_key_labels(px, mix(TRAY, LABEL, 0.55));   // (smaller and dimmer than TESSERA's: the device has none)

    // the project files: OPEN, SAVE, and what the last one did
    for (size_t i = 0; i < files.size(); i++)
    {
        const FileButton &f = files[i];
        int dy = file_held == (int)i ? 1 : 0;
        c.rrect(f.x, f.y + dy, f.w, f.h, 6, EDGE);
        c.rrect(f.x + 1.5, f.y + 1.5 + dy, f.w - 3, f.h - 3, 5, BTN);
        c.text(f.x + f.w / 2, f.y + f.h / 2 - 7 + dy, f.face, 2, BTN_INK);
    }
    // the bank switch (8 tracks): the half of the selected track's bank lit
    if (banked())
    {
        const FileButton &a = banks[0];
        c.rrect(a.x, a.y, banks[1].x + banks[1].w - a.x, a.h, 6, EDGE);
        for (int i = 0; i < 2; i++)
        {
            const FileButton &b = banks[i];
            bool lit = (track_now & 4) == b.action;
            int dy = bank_held == i ? 1 : 0;
            c.rrect(b.x + 1.5, b.y + 1.5 + dy, b.w - 3, b.h - 3, 5, lit ? LIT : BTN);
            c.text(b.x + b.w / 2, b.y + b.h / 2 - 7 + dy, b.face, 2, lit ? LIT_INK : BTN_INK);
        }
    }
    {   // FM-1 USB: lit while this instance listens to an FM-1 on USB
        const FileButton &u = usb;
        bool lit = fm1_on && fm1_mine;
        int dy = file_held == 100 ? 1 : 0;
        c.rrect(u.x, u.y + dy, u.w, u.h, 6, EDGE);
        c.rrect(u.x + 1.5, u.y + 1.5 + dy, u.w - 3, u.h - 3, 5, lit ? LIT : BTN);
        c.text(u.x + u.w / 2, u.y + u.h / 2 - 7 + dy, u.face, 2, lit ? LIT_INK : BTN_INK);
    }
    if (note_frames > 0)
    {
        note_frames--;
        const FileButton &last = usb;
        int x = last.x + last.w + 16, sx, sy;
        swatch_xy(0, sx, sy);
        std::string t = note_text;   // (cut short before the shells' swatches)
        while (!t.empty() && x + Canvas::text_w(t.c_str(), 2) > sx - SWATCH_R - 12)
            t.pop_back();
        c.text(x + Canvas::text_w(t.c_str(), 2) / 2, last.y + last.h / 2 - 7, t.c_str(), 2, LABEL);
    }

    // the shells: a swatch each (its body round its buttons), the one shown ringed; its name while the mouse is on it
    for (int i = 0; i < NSHELLS; i++)
    {
        int x, y;
        swatch_xy(i, x, y);
        if (i == shell_ || i == shell_hover)
            c.circle(x, y, SWATCH_R + 3, i == shell_ ? LABEL : mix(BODY, LABEL, 0.4));
        c.circle(x, y, SWATCH_R + 1, BODY);
        c.circle(x, y, SWATCH_R, SHELLS[i].body);
        c.circle(x, y, SWATCH_R * 0.5, SHELLS[i].btn);
    }
    {   // TESSERA's own swatch set apart from the FM-1's eight by a line
        int x0, x1, y;
        swatch_xy(0, x0, y);
        swatch_xy(1, x1, y);
        int lx = (x0 + SWATCH_R + x1 - SWATCH_R) / 2;
        c.rrect(lx, y - SWATCH_R, 1, 2 * SWATCH_R, 0, mix(BODY, LABEL, 0.5));
    }
    int sx, sy;
    swatch_xy(0, sx, sy);
    const char *what = shell_hover >= 0 ? SHELLS[shell_hover].name : note_frames > 0 ? "" : "SHELL";   // (a note there)
    c.text(sx - SWATCH_R - 14 - Canvas::text_w(what, 2) / 2, sy - 7, what, 2, LABEL);

    // the screen's inks: a swatch each (its ink on the screen's black), the one shown ringed; its name while the mouse
    // is on it
    for (int i = 0; i < (int)inks.size(); i++)
    {
        int x, y;
        ink_xy(i, x, y);
        if (i == ink_now || i == ink_hover)
            c.circle(x, y, INK_R + 3, i == ink_now ? LABEL : mix(BODY, LABEL, 0.4));
        c.circle(x, y, INK_R + 1, BODY);
        c.circle(x, y, INK_R, 0xFF000000);
        const uint32_t *k = inks[i].rgb;
        uint32_t q[4] = {0xFF000000u | k[0], 0xFF000000u | k[1], 0xFF000000u | k[2], 0xFF000000u | k[3]};
        if (k[0] == k[1] && k[0] == k[2] && k[0] == k[3])
            c.circle(x, y, INK_R * 0.55, q[0]);
        else
            c.quad(x, y, INK_R * 0.6, q);
    }
    if (!inks.empty())
    {
        int ix, iy;
        ink_xy(0, ix, iy);
        const char *name = ink_hover >= 0 ? inks[ink_hover].name.c_str() : "INK";
        c.text(ix - INK_R - 12 - Canvas::text_w(name, 2) / 2, iy - 7, name, 2, LABEL);
    }
}

// ---- the shells

int Panel::shell_count() { return NSHELLS; }
const char *Panel::shell_id(int i) { return i >= 0 && i < NSHELLS ? SHELLS[i].id : ""; }
const char *Panel::shell_ink(int i) { return i >= 0 && i < NSHELLS ? SHELLS[i].ink : ""; }
int Panel::shell_by_id(const std::string &id)
{
    for (int i = 0; i < NSHELLS; i++)
        if (id == SHELLS[i].id)
            return i;
    return -1;
}
void Panel::set_shell(int i)
{
    if (i >= 0 && i < NSHELLS)
    {
        shell_ = i;
        layout();
        menu_open = false;
    }
}
// swatch i's centre: a row under the keys, at the right, as OPEN and SAVE are at the left
void Panel::swatch_xy(int i, int &x, int &y) const
{
    x = KEYBED[0] + KEYBED[2] - SWATCH_R - 2 - (NSHELLS - 1 - i) * (2 * SWATCH_R + 10) - (i == 0 ? SHELL_GAP : 0);
    y = KEYBED[1] + KEYBED[3] + 5 + 12;
}
// ink i's centre: a row between the buttons and the keys, at the right
void Panel::ink_xy(int i, int &x, int &y) const
{
    x = 568 + 400 - INK_R - 6 - ((int)inks.size() - 1 - i) * (2 * INK_R + 6);
    y = (300 + KEYBED[1]) / 2;
}
int Panel::ink_at(int x, int y) const
{
    if (native()) return -1;   // (TESSERA's shell: in its menu)
    for (int i = 0; i < (int)inks.size(); i++)
    {
        int ix, iy;
        ink_xy(i, ix, iy);
        if (std::hypot(x - ix, y - iy) <= INK_R + 3)
            return i;
    }
    return -1;
}
int Panel::swatch_at(int x, int y) const
{
    if (native()) return -1;
    for (int i = 0; i < NSHELLS; i++)
    {
        int sx, sy;
        swatch_xy(i, sx, sy);
        if (std::hypot(x - sx, y - sy) <= SWATCH_R + 4)
            return i;
    }
    return -1;
}

// ---- the mouse

int Panel::knob_at(int x, int y) const
{
    for (size_t i = 0; i < knobs.size(); i++)
        if (std::hypot(x - knobs[i].x, y - knobs[i].y) <= knobs[i].r + 12)
            return (int)i;
    return -1;
}
int Panel::button_at(int x, int y) const
{
    for (size_t i = 0; i < buttons.size(); i++)
        if (x >= buttons[i].x && x < buttons[i].x + buttons[i].w && y >= buttons[i].y && y < buttons[i].y + buttons[i].h)
            return (int)i;
    return -1;
}
int Panel::key_at(int x, int y) const
{
    for (size_t i = 0; i < keys.size(); i++)   // each key's target: its pitch wide, about half the bed tall
        if (std::abs(x - keys[i].cx) <= pitch / 2 && std::abs(y - keys[i].cy) <= KEYBED[3] * 24 / 100)
            return (int)i;
    return -1;
}

void Panel::merge_held()
{
    input.keys = mouse_keys | kbd_keys | latch_keys;
    input.buttons = mouse_buttons | kbd_buttons | latch_buttons;
    input.changed = true;
}

// web/sim/index.html KEYMAP: F3..A#4 on the Z (white) and A (black) rows, B4..G5 on Q..O in semitones, the twelve
// buttons on the number row in the panel's order, [ ] OCT
bool Panel::text(uint32_t ch)
{
    if (!typing_ || !keys_on_ || menu_open || !ch) return false;
    input.text.push_back(ch);
    input.changed = true;
    return true;
}

bool Panel::key(int code, bool down)
{
    static const int WHITE_SC[10] = {0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35};   // Z X C V B N M , . /
    static const int BLACK_SC[8][2] = {{0x1F, 1}, {0x20, 3}, {0x21, 5}, {0x23, 8}, {0x24, 10}, {0x26, 13}, {0x27, 15}, {0x28, 17}};
    static const int TOP_SC[9] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};             // Q W E R T Y U I O
    static const int BTN_SC[12] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};   // 1 .. 0 - =
    static const int BTN_BIT[12] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC};
    int k = -1, b = -1;
    for (int i = 0; i < 10; i++) if (code == WHITE_SC[i]) k = WHITE_K[i];
    for (int i = 0; i < 8; i++) if (code == BLACK_SC[i][0]) k = BLACK_SC[i][1];
    for (int i = 0; i < 9; i++) if (code == TOP_SC[i]) k = 18 + i;
    for (int i = 0; i < 12; i++) if (code == BTN_SC[i]) b = BTN_BIT[i];
    if (code == 0x01 && menu_open)   // Esc: the menu shut
    {
        if (down) menu_open = false;
        return false;
    }
    if (code == 0x01 && gen_list)    // Esc: GENERATE's style list shut
    {
        if (down) gen_list = false;
        return false;
    }
    if (code == 0x01 && (latch_keys || latch_buttons))   // Esc: every latch let go
    {
        if (!down) return false;
        latch_keys = latch_buttons = 0;
        merge_held();
        return true;
    }
    if (!keys_on_)   // KEYS OFF: every key the DAW's
        return false;
    if (code == 0x1A || code == 0x01) b = B_OCTDN;   // [ and Esc: OCT- (TESSERA's shell: BACK)
    if (code == 0x1B || code == 0x1C) b = B_OCTUP;   // ] and Enter: OCT+ (OK)
    if (code == 0x39 && standalone_) b = B_PLAY;     // Space: PLAY (the standalone app; in a DAW, the DAW's)
    if (native())   // TESSERA's shell: Shift its SHIFT (held while the key is; not while NAME takes capitals), Page Up /
    {               // Page Down its OCT+ / OCT-, F1 its ?
        if ((code == 0x2A || code == 0x36) && (!typing_ || !down)) b = BIT_SHIFT;
        if (code == KEY_PGUP) b = BIT_OCT_UP;
        if (code == KEY_PGDN) b = BIT_OCT_DOWN;
        if (code == 0x3B) b = BIT_HELP;   // F1: ?
    }
    if (code == 0x0F && banked())   // Tab: the other bank (T1 or T5)
    {
        if (!down) return false;
        input.track = (track_now & 4) ? 0 : 4;
        track_now = input.track;   // (a second Tab before the view catches up: back again)
        input.changed = true;
        return true;
    }
    if (code == KEY_UP || code == KEY_DOWN || code == KEY_LEFT || code == KEY_RIGHT)
    {
        if (!down) return false;
        int role = code == KEY_UP || code == KEY_DOWN ? 0 : 1;   // SELECT, ALGORITHM
        for (size_t i = 0; i < knobs.size(); i++)
            if (knobs[i].role == role) turn((int)i, code == KEY_UP || code == KEY_RIGHT ? 1 : -1);
        return true;
    }
    uint32_t was_k = kbd_keys, was_b = kbd_buttons;
    if (k >= 0) kbd_keys = down ? kbd_keys | 1u << k : kbd_keys & ~(1u << k);
    if (b >= 0) kbd_buttons = down ? kbd_buttons | 1u << b : kbd_buttons & ~(1u << b);
    if (kbd_keys == was_k && kbd_buttons == was_b) return false;   // (not ours, or a key's repeat)
    merge_held();
    return true;
}

// Ctrl+O / Ctrl+S (Cmd on a Mac): OPEN and SAVE, as the buttons under the keys; the standalone app's only (a DAW keeps
// its own). true: the window should take the file request now
bool Panel::shortcut(char c)
{
    if (!standalone_ || (c != 'O' && c != 'S'))
        return false;
    file_req = c == 'O' ? FILE_OPEN : FILE_SAVE;
    return true;
}

bool Panel::release_keys()
{
    if (!kbd_keys && !kbd_buttons) return false;
    kbd_keys = kbd_buttons = 0;
    merge_held();
    return true;
}

void Panel::turn(int i, int n)
{
    Knob &k = knobs[i];
    if (k.role < 0)
    {
        int m = (master_mine >= 0 ? master_mine : master_now) + n * 16;
        input.master = master_mine = m < 0 ? 0 : m > 1023 ? 1023 : m;
        master_mine_at = frame;
        k.angle = -135 + 270.0 * input.master / 1023;
    }
    else
    {
        input.enc[k.role] += n;
        k.angle = std::fmod(k.angle + n * 18, 360);
    }
    turned[i] = frame ? frame : 1;
    input.changed = true;
}

bool Panel::down(int x, int y)
{
    last_x = x;
    last_y = y;
    acc = acc_y = 0;
    if (native() && menu_open)   // (a click with the menu open is the menu's, inside or out)
    {
        drag = NONE;
        return menu_click(x, y);
    }
    if (gen_list)   // GENERATE's style list: a style picked, or (anywhere else) the list shut
    {
        drag = NONE;
        int j = gen_list_at(x, y);
        gen_list = false;
        if (j >= 0 && j != gen.style)
        {
            gen.style = j;
            gen_ask(GEN_SET);
        }
        return false;
    }
    switch (gen_part_at(x, y))   // GENERATE's strip
    {
    case GP_BUTTON:
        drag = GEN_BUTTON;
        gen_down_frame = frame;
        gen_swapped = false;
        return false;
    case GP_PREV: drag = NONE; gen_style_step(-1); return false;
    case GP_NEXT: drag = NONE; gen_style_step(1); return false;
    case GP_STYLE: drag = NONE; gen_list = true; gen_list_hover = -1; return false;
    case GP_VAR:
    {
        double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        if (now - gen_var_click < 0.4) gen_var_step(20);   // (a double-click: 100)
        gen_var_click = now;
        drag = GEN_VAR;
        return false;
    }
    case GP_SEED: drag = NONE; return false;
    default: break;
    }
    if (native() && x >= 30 && x < 250 && y >= 6 && y < 44)   // the mark: the menu
    {
        drag = NONE;
        menu_open = true;
        menu_sub = 0;
        menu_hover = sub_hover = -1;
        return false;
    }
    int z2 = zone2_at(x, y);
    if (z2 >= 0)   // TRACKS (EXPANDED): a row, M, S (a click each; the level: the wheel)
    {
        drag = NONE;
        if (zones2[z2].kind == TESS_ZONE_LEVEL) return false;
        if (zones2[z2].kind == TESS_ZONE_TEMPO)   // (the tempo: dragged as a value, a double-click 120)
        {
            double t = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
            bool dbl = row2_last == -2 && t - row2_t < 0.4;
            row2_last = dbl ? -1 : -2, row2_t = t;
            if (dbl) { input.tzones.push_back({zones2[z2], TESS_ZA_DOUBLE, 0}); input.changed = true; return true; }
            drag = ZONE2;
            drag_zone = zones2[z2];
            acc = 0;
            return false;
        }
        double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        bool dbl = zones2[z2].kind == TESS_ZONE_TRACK && zones2[z2].row == row2_last && now - row2_t < 0.4;   // (a row
        row2_last = dbl ? -1 : zones2[z2].kind == TESS_ZONE_TRACK ? zones2[z2].row : -1;   // twice: its EDIT page)
        row2_t = now;
        input.tzones.push_back({zones2[z2], dbl ? (uint32_t)TESS_ZA_DOUBLE : (uint32_t)TESS_ZA_CLICK, 0});
        input.changed = true;
        return true;
    }
    int zi = zone_at(x, y);
    if (zi >= 0)   // the screen: a zone (a double-click: the same zone, soon, where the first was)
    {
        const tess_zone_t z = zones[zi];
        double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        bool dbl = click_known && zone_like(click_zone) == zi && now - click_t < 0.4 && std::abs(x - click_x) < 6 &&
                   std::abs(y - click_y) < 6;
        click_known = !dbl;
        click_zone = z;
        click_t = now, click_x = x, click_y = y;
        if (z.kind == TESS_ZONE_ROLL)   // NOTES' steps: the note there grabbed; dragged up or down, its pitch; let go
        {                               // unmoved, a click (a note there)
            drag = ZONE;
            drag_zone = zone_here(zi, x, y);
            zone_still = true;
            acc = 0;
            zone_act(drag_zone, TESS_ZA_GRAB, 0);
            return true;
        }
        if (z.kind == TESS_ZONE_KNOB || z.kind == TESS_ZONE_LEVEL || z.kind == TESS_ZONE_PAN ||
            z.kind == TESS_ZONE_SCROLL || z.kind == TESS_ZONE_TEMPO || z.kind == TESS_ZONE_POINT ||
            z.kind == TESS_ZONE_CHORD)   // (a value, a text, the tempo, a marker, CHORD USER's card: its root)
        {
            drag = ZONE;
            drag_zone = z;
            zone_still = !dbl;   // (let go unturned: a click, which closes a pinned help card)
            if (dbl) zone_act(z, TESS_ZA_DOUBLE, 0);
            return dbl;
        }
        if (z.kind == TESS_ZONE_ROW && z.vrole >= 0)   // (a row with a value, a sheet's or MENU's: click there,
        {                                             // then held and dragged up or down, its value turns)
            zone_act(z, dbl ? TESS_ZA_DOUBLE : TESS_ZA_CLICK, 0);
            drag = ZONE;
            drag_zone = z;
            acc = 0;
            return true;
        }
        drag = NONE;
        if (dbl && z.kind != TESS_ZONE_ROW && z.kind != TESS_ZONE_TRACK && z.kind != TESS_ZONE_CELL &&
            !(z.kind == TESS_ZONE_MUTE && z.role == 1))
            return false;   // (OK, YES: once, however often clicked; MIXER's MUTE word: double-click SOLO; HOME's track:
                            // double-click its sounds; a cell: the browser's sound kept)
        zone_act(z, dbl ? TESS_ZA_DOUBLE : TESS_ZA_CLICK, 0);
        return true;
    }
    if ((drag_i = knob_at(x, y)) >= 0) { drag = KNOB; return false; }
    if ((drag_i = button_at(x, y)) >= 0 && (latch_buttons >> buttons[drag_i].bit & 1u))   // a latched one: let go
    {
        drag = NONE;
        latch_buttons &= ~(1u << buttons[drag_i].bit);
        merge_held();
        return true;
    }
    if (drag_i >= 0 && buttons[drag_i].bit == BIT_SHIFT)   // SHIFT: a click latches it (the mouse then free for a
    {                                                      // knob), a second lets go (above)
        drag = NONE;
        latch_buttons |= 1u << BIT_SHIFT;
        merge_held();
        return true;
    }
    if (drag_i >= 0)
    {
        drag = BUTTON;
        mouse_buttons |= 1u << buttons[drag_i].bit;
        merge_held();
        return true;
    }
    if ((drag_i = key_at(x, y)) >= 0 && (latch_keys >> keys[drag_i].k & 1u))   // a latched one: let go
    {
        drag = NONE;
        latch_keys &= ~(1u << keys[drag_i].k);
        merge_held();
        return true;
    }
    if (drag_i >= 0)
    {
        drag = KEY;
        mouse_keys |= 1u << keys[drag_i].k;
        merge_held();
        return true;
    }
    drag = NONE;
    bool hit = false;   // (anything below: not a click away)
    int bk = bank_at(x, y);
    if (bk >= 0)
    {
        bank_held = bk;
        if ((track_now & 4) == banks[bk].action) return false;   // (that bank already)
        input.track = banks[bk].action;
        track_now = input.track;
        input.changed = true;
        return true;
    }
    int sw_i = swatch_at(x, y);
    hit = sw_i >= 0;
    if (sw_i >= 0 && sw_i != shell_)
    {
        set_shell(sw_i);
        shell_req = sw_i;
        return false;
    }
    int in_i = ink_at(x, y);
    hit = hit || in_i >= 0;
    if (in_i >= 0 && in_i != ink_now)
    {
        ink_now = ink_pending = ink_req = in_i;   // (ringed at once; the view follows)
    }
    for (size_t i = 0; i < files.size(); i++)
        if (x >= files[i].x && x < files[i].x + files[i].w && y >= files[i].y && y < files[i].y + files[i].h)
        {
            file_held = (int)i;
            file_req = files[i].action;
            hit = true;
        }
    if (!native() && x >= usb.x && x < usb.x + usb.w && y >= usb.y && y < usb.y + usb.h)
    {
        file_held = 100;   // (pressed in, as OPEN and SAVE)
        fm1_req = fm1_on && fm1_mine ? 0 : 1;
        hit = true;
    }
    if (!hit && closable_)   // a click on nothing (the body, the screen off its zones): a picker, a sheet or a pinned
    {                        // help card closes, as BACK
        input.away = true;
        input.changed = true;
        return true;
    }
    return false;
}

bool Panel::move(int x, int y)
{
    if (native() && menu_open)
    {
        menu_hover = menu_row_at(x, y);
        sub_hover = sub_at(x, y);
        last_x = x;
        last_y = y;
        return false;
    }
    if (gen_list)
    {
        gen_list_hover = gen_list_at(x, y);
        last_x = x;
        last_y = y;
        return false;
    }
    shell_hover = drag == NONE ? swatch_at(x, y) : -1;
    ink_hover = drag == NONE ? ink_at(x, y) : -1;
    hover_in = true, hover_x = x, hover_y = y;
    bool hover_moved = false;
    if (hover_help_ && drag == NONE)   // HOVER HELP: the zone under the mouse, to the core when it changes
    {
        int hz = zone_at(x, y);
        tess_zone_t was = hover_now;
        hover_send(hz >= 0 ? &zones[hz] : nullptr);
        hover_moved = std::memcmp(&was, &hover_now, sizeof was) != 0;
    }
    int dx = x - last_x, dy = last_y - y;   // up or right: clockwise (the larger of the two)
    last_x = x;
    last_y = y;
    if (drag == ZONE2)   // TRACKS' tempo: a detent every 8 px
    {
        acc += std::abs(dx) > std::abs(dy) ? dx : dy;
        int n = acc / 8;
        if (n) { acc -= n * 8; input.tzones.push_back({drag_zone, TESS_ZA_TURN, n}); input.changed = true; return true; }
        return false;
    }
    if (drag == ZONE && drag_zone.kind == TESS_ZONE_POINT)   // a picture's marker: each way its own knob, a detent
    {                                                        // every 3 px (+ right, + up)
        acc += dx;
        acc_y += dy;
        int nx = acc / 3, ny = acc_y / 3;
        if (nx) { acc -= nx * 3; zone_act(drag_zone, TESS_ZA_TURN, nx); }
        if (ny) { acc_y -= ny * 3; zone_act(drag_zone, TESS_ZA_TURN_Y, ny); }
        if (nx || ny) { zone_still = click_known = false; return true; }
        return false;
    }
    if (drag == ZONE && drag_zone.kind == TESS_ZONE_ROLL)   // a note: a row every 6 px up or down; with SHIFT (held,
    {                                                      // latched or the Shift key) its whole chord
        acc += dy;
        int n = acc / 6;
        if (n)
        {
            acc -= n * 6;
            zone_still = click_known = false;
            zone_act(drag_zone, input.buttons >> BIT_SHIFT & 1u ? (uint32_t)TESS_ZA_TURN_CHORD : (uint32_t)TESS_ZA_TURN, n);
            return true;
        }
    }
    else if (drag == ZONE)   // a value's column: as a knob, a detent every 8 px
    {
        acc += std::abs(dx) > std::abs(dy) ? dx : dy;
        int n = acc / 8;
        if (n) { acc -= n * 8; zone_still = click_known = false; zone_act(drag_zone, TESS_ZA_TURN, n); return true; }   // (turned: no click, nor half a double-click)
    }
    else if (drag == GEN_VAR)   // VAR: 5 a detent, a detent every 8 px
    {
        acc += std::abs(dx) > std::abs(dy) ? dx : dy;
        int n = acc / 8;
        if (n) { acc -= n * 8; gen_var_step(n); }
        return false;
    }
    else if (drag == KNOB)
    {
        acc += std::abs(dx) > std::abs(dy) ? dx : dy;
        int step = knobs[drag_i].role < 0 ? 3 : 8, n = acc / step;
        if (n) { acc -= n * step; turn(drag_i, n); return true; }
    }
    else if (drag == KEY)   // a glide across the keys
    {
        int k = key_at(x, y);
        if (k >= 0 && k != drag_i)
        {
            mouse_keys = (mouse_keys & ~(1u << keys[drag_i].k)) | 1u << keys[k].k;
            drag_i = k;
            merge_held();
            return true;
        }
    }
    return hover_moved;
}

// HOVER HELP: z (null: none) the zone the mouse rests on now; the core is told when it changes
void Panel::hover_send(const tess_zone_t *z)
{
    tess_zone_t n = z ? *z : tess_zone_t{};
    if (!z) n.kind = 0;
    if (!std::memcmp(&n, &hover_now, sizeof n))
        return;
    hover_now = n;
    hover_t = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    input.hover = n;
    input.hover_changed = true;
    input.changed = true;
}

// a right-click: the button or key under it latched (held), or let go if it was
bool Panel::latch(int x, int y)
{
    if (native() && menu_open)
        return false;
    int zi = zone_at(x, y);
    if (zi >= 0)   // the screen: a value's column or a list's row, its help card (tess_core_zone_do TESS_ZA_HELP)
    {
        const tess_zone_t &z = zones[zi];
        if (z.kind == TESS_ZONE_ROLL)   // NOTES' steps: the note there removed
        {
            zone_act(zone_here(zi, x, y), TESS_ZA_HELP, 0);
            return true;
        }
        if (z.kind != TESS_ZONE_KNOB && !(z.kind == TESS_ZONE_ROW && z.vrole >= 0) &&
            !(z.kind == TESS_ZONE_MUTE && z.role == 1) && !(z.kind == TESS_ZONE_POINT && (z.row & 0x200)))
            return false;   // (MIXER's MUTE word: its solo; BOUNCE's picture: a ball less)
        zone_act(z, TESS_ZA_HELP, 0);
        return true;
    }
    int i;
    if ((i = button_at(x, y)) >= 0)
        latch_buttons ^= 1u << buttons[i].bit;
    else if ((i = key_at(x, y)) >= 0)
        latch_keys ^= 1u << keys[i].k;
    else
        return false;
    merge_held();
    return true;
}

bool Panel::up(int, int)
{
    bool ch = false;
    if (drag == GEN_BUTTON && !gen_swapped)   // GENERATE let go before half a second: a new piece
        gen_ask(GEN_MAKE);
    gen_swapped = false;
    if (drag == BUTTON) { mouse_buttons &= ~(1u << buttons[drag_i].bit); merge_held(); ch = true; }
    if (drag == KEY) { mouse_keys &= ~(1u << keys[drag_i].k); merge_held(); ch = true; }
    if (drag == ZONE && zone_still) { zone_act(drag_zone, TESS_ZA_CLICK, 0); ch = true; }   // (a help card's text, a
    zone_still = false;                                                                       // knob's column: a click)
    drag = NONE;
    file_held = bank_held = -1;
    return ch;
}

bool Panel::wheel(int x, int y, int notches)
{
    switch (notches ? gen_part_at(x, y) : -1)   // GENERATE's strip: the style, VAR, the pieces made before
    {
    case GP_PREV: case GP_STYLE: case GP_NEXT: gen_style_step(-notches); return false;   // (up: the one before)
    case GP_VAR: gen_var_step(notches); return false;
    case GP_SEED: gen_ask(GEN_RECALL, notches > 0 ? -1 : 1); return false;   // (up: back to an older piece)
    default: break;
    }
    int z2 = zone2_at(x, y);
    if (z2 >= 0 && notches)   // TRACKS: a level turned (elsewhere on a row: nothing)
    {
        if (zones2[z2].kind != TESS_ZONE_LEVEL && zones2[z2].kind != TESS_ZONE_TEMPO) return false;
        input.tzones.push_back({zones2[z2], TESS_ZA_TURN, notches});
        input.changed = true;
        return true;
    }
    int zi = zone_at(x, y);
    if (zi >= 0 && notches)   // a value: as its knob turned; a list: it moves (a row a notch)
    {
        zone_act(zone_here(zi, x, y), TESS_ZA_WHEEL, notches);   // (NOTES' steps: the note under the mouse)
        return true;
    }
    int i = knob_at(x, y);
    if (i < 0 || !notches)
        return false;
    turn(i, notches);
    return true;
}

// ---- TESSERA's shell: the project page's look (docs/index.html), flat and 1-bit: black and #111 body, Tessera yellow
// for what is live or lit, a dim yellow for lines, warm white for what is held, warm grey for words; corners cut a
// dot, buttons with a lip that sit down when lit or held; the menu under the mark

namespace
{
const uint32_t T_YEL = 0xFFFFD020, T_DIM = 0xFF3B3007, T_HELD = 0xFFE6E0D0, T_WORD = 0xFFB9B2A0, T_OFF = 0xFF5F5A4C,
               T_BLACK = 0xFF000000, T_BODY = 0xFF111111;
const int MENU_X = 36, MENU_Y = 46, MENU_W = 300, MENU_ROWS = 10, ROW_H = 36, SUB_X = MENU_X + MENU_W + 8, SUB_ROW = 30,
          INK_COLS = 5, INK_S = 32, INK_GAP = 12, FM1_LINE = 22;
// SHELL, INK; MODE, SIZE; FM-1 USB and its state, KEYS, LISTEN, SPLIT; ABOUT (a dotted line between each group)
enum { ROW_SHELL, ROW_INK, ROW_MODE, ROW_SIZE, ROW_FM1, ROW_KEYS, ROW_HOVER, ROW_LISTEN, ROW_SPLIT, ROW_ABOUT };
int row_y(int i)
{
    return MENU_Y + 10 + i * ROW_H + (i >= ROW_MODE ? 18 : 0) + (i >= ROW_FM1 ? 18 : 0) + (i >= ROW_KEYS ? FM1_LINE : 0) +
           (i >= ROW_ABOUT ? 18 : 0);
}
const int MENU_H = 2 + 8 + MENU_ROWS * ROW_H + 3 * 18 + FM1_LINE + 8 + 2;
// FM-1 USB's two words, right to left from the row's right edge: ON, then OFF
const int SEG_RIGHT = MENU_X + MENU_W - 16;
int seg_w(const char *t) { return Canvas::ptext_w(t, 2) + 12; }
// SIZE's choices, right to left (0: no more): FM-1 mode 200 150 100, EXPANDED 125 100
int size_choice(bool expanded, int i)
{
    static const int FM1[3] = {200, 150, 100}, EXP[3] = {125, 100, 0};
    return i < 0 || i > 2 ? 0 : expanded ? EXP[i] : FM1[i];
}
// LISTEN's < value >: each one's left and right, from SEG_RIGHT leftwards
const int LISTEN_SEG[3][2] = {{108, 136}, {32, 104}, {0, 28}};
int shell_sub_y() { return row_y(ROW_SHELL) - 10; }

// GENERATE's strip, between the MIDI lamp and the version: the button, the style (its arrows at the ends), VAR, the
// seed; the style's list under its name
const int GEN_Y = 10, GEN_H = 28, GB_X = 316, GB_W = 112, GS_X = 436, GS_W = 168, GS_ARROW = 24, GV_X = 612, GV_W = 88,
          GD_X = 708, GD_W = 112, GL_Y = GEN_Y + GEN_H + 6, GL_ROW = 26;

int ink_sub_y() { return row_y(ROW_INK) - 10; }

// a box with its corners cut a dot (2 px): border, then the fill inside, `bottom` its lower edge's width
void nbox(Canvas &c, int x, int y, int w, int h, uint32_t border, uint32_t fill, int bottom = 2)
{
    c.box(x, y, w, h, border);
    c.box(x + 2, y + 2, w - 4, h - 2 - bottom, fill);
    c.box(x, y, 2, 2, T_BODY);
    c.box(x + w - 2, y, 2, 2, T_BODY);
    c.box(x, y + h - 2, 2, 2, T_BODY);
    c.box(x + w - 2, y + h - 2, 2, 2, T_BODY);
}
void dotted(Canvas &c, int x0, int x1, int y, uint32_t col)
{
    for (int x = x0; x < x1; x += 4)
        c.box(x, y, 2, 2, col);
}
}   // namespace

void Panel::draw_native(uint32_t *px, const tess_view_t &v, double breath, const tess_tracks_t *t2)
{
    Canvas c{px, width()};
    static const double kPi = 3.14159265358979;
    const int PW = width();
    for (int i = 0; i < PW * H; i++)
        px[i] = T_BODY;

    // the mark, the name, the menu's arrow, the MIDI lamp; the version at the right
    for (int ty = 0; ty < 3; ty++)
        for (int tx = 0; tx < 3; tx++)
        {
            int x = 36 + tx * 9, y = 12 + ty * 9;
            c.box(x, y, 8, 8, T_YEL);
            if (!(ty == 0 && tx == 2)) c.box(x + 2, y + 2, 4, 4, T_BODY);
        }
    c.ptext(72, 14, "COSMATI", 3, T_YEL);   // (the plugin's name beside Tessera's mark)
    int ax = 72 + Canvas::ptext_w("COSMATI", 3) + 10;
    c.box(ax, 22, 12, 2, T_YEL);
    c.box(ax + 2, 24, 8, 2, T_YEL);
    c.box(ax + 4, 26, 4, 2, T_YEL);
    int lx = ax + 12 + 20;
    bool midi_lit = midi_frame && frame - midi_frame < 8;
    c.box(lx, 20, 10, 10, T_YEL);
    if (!midi_lit) c.box(lx + 2, 22, 6, 6, T_BLACK);
    c.ptext(lx + 16, 18, "MIDI", 2, T_WORD);
    if (!version.empty())
    {
        std::string ver = (gen_on() ? "" : "PLUGIN ") + version;   // (GENERATE's strip: the version alone)
        for (auto &ch : ver) ch = ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
        c.ptext(PW - 36 - Canvas::ptext_w(ver.c_str(), 2), 18, ver.c_str(), 2, T_WORD);
    }

    // the screen: a dim hairline, a gap, the 240 x 240 at 1:1; under it the track and its bank
    nbox(c, SCREEN_X, SCREEN_Y, SCREEN_S, SCREEN_S, T_DIM, T_BODY);
    const int inset = SCREEN_S / 32;
    for (int y = 0; y < TESS_SCREEN; y++)
        for (int x = 0; x < TESS_SCREEN; x++)
        {
            uint16_t p = v.screen[y * TESS_SCREEN + x];
            uint32_t r = (p >> 11 & 31) * 255 / 31, g = (p >> 5 & 63) * 255 / 63, b = (p & 31) * 255 / 31;
            px[(SCREEN_Y + inset + y) * PW + SCREEN_X + inset + x] = 0xFF000000u | r << 16 | g << 8 | b;
        }
    char cap[8];
    std::snprintf(cap, sizeof cap, "T%d", track_now + 1);
    c.ptext(SCREEN_X + 2, SCREEN_Y + SCREEN_S + 8, cap, 2, T_WORD);
    if (expanded())   // EXPANDED: PAGE under the page; TRACKS beside it, 1-8 (or 1-4) under it
    {
        c.ptext(SCREEN_X + SCREEN_S - 2 - Canvas::ptext_w("PAGE", 2), SCREEN_Y + SCREEN_S + 8, "PAGE", 2, T_WORD);
        nbox(c, T2_X, T2_Y, T2_W, T2_H, T_DIM, T_BODY);
        c.box(T2_X + T2_IN, T2_Y + T2_IN, TESS_TRACKS_W, TESS_TRACKS_H, T_BLACK);
        if (t2 && t2->w == TESS_TRACKS_W && t2->h == TESS_TRACKS_H)
            for (int y = 0; y < TESS_TRACKS_H; y++)
                for (int x = 0; x < TESS_TRACKS_W; x++)
                {
                    uint16_t p = t2->screen[y * TESS_TRACKS_W + x];
                    uint32_t r = (p >> 11 & 31) * 255 / 31, g = (p >> 5 & 63) * 255 / 63, b = (p & 31) * 255 / 31;
                    px[(T2_Y + T2_IN + y) * PW + T2_X + T2_IN + x] = 0xFF000000u | r << 16 | g << 8 | b;
                }
        const char *all = tracks_ > 4 ? "1-8" : "1-4";
        c.ptext(T2_X + 2, T2_Y + T2_H + 8, "TRACKS", 2, T_WORD);
        c.ptext(T2_X + T2_W - 2 - Canvas::ptext_w(all, 2), T2_Y + T2_H + 8, all, 2, T_WORD);
    }
    else if (tracks_ > 4)
    {
        const char *bank = track_now & 4 ? "5-8" : "1-4";
        c.ptext(SCREEN_X + SCREEN_S - 2 - Canvas::ptext_w(bank, 2), SCREEN_Y + SCREEN_S + 8, bank, 2, T_WORD);
    }

    // knobs: a ring like the screen's dials; MASTER keeps its pointer, an encoder shows one (white) while it turns; the
    // pointer runs into the ring (no gap)
    for (size_t i = 0; i < knobs.size(); i++)
    {
        const Knob &k = knobs[i];
        bool turning = turned[i] && frame - turned[i] < 15;
        uint32_t ring = turning ? T_HELD : T_YEL;
        c.ptext_c(k.x, k.y - 48, k.name, 2, T_WORD);
        c.circle(k.x, k.y, k.r, ring);
        c.circle(k.x, k.y, k.r - 4, T_BLACK);
        if (k.role < 0 || turning)
        {
            double a = (k.angle - 90) * kPi / 180;
            c.line(k.x + std::cos(a) * 2, k.y + std::sin(a) * 2, k.x + std::cos(a) * (k.r - 4), k.y + std::sin(a) * (k.r - 4),
                   4, ring);
        }
    }

    // buttons: lit (and PLAY green) solid yellow, held warm white, both sat down; breathing pulses; what is there a
    // yellow outline, the rest dim
    for (auto &b : buttons)
    {
        uint32_t bit = 1u << b.bit;
        bool lit = (v.lit_buttons & bit) || (b.bit == B_PLAY && (v.lit_buttons >> PLAY_GREEN & 1));
        bool held = input.buttons & bit, down = lit || held;
        uint32_t border = T_DIM, fill = T_BLACK, ink = T_WORD;
        if (held) border = fill = T_HELD, ink = T_BLACK;
        else if (lit) border = fill = T_YEL, ink = T_BLACK;
        else if (v.breath_buttons & bit) border = T_YEL, fill = mix(T_BLACK, T_YEL, 0.35 * breath), ink = T_YEL;
        else if ((v.dim_buttons & bit) || b.bit == BIT_OCT_DOWN || b.bit == BIT_OCT_UP || b.bit == BIT_HELP)   // (there)
            border = T_YEL, ink = T_YEL;
        int y = b.y + (down ? 2 : 0), h = b.h - (down ? 2 : 0), bottom = down ? 2 : 4;
        nbox(c, b.x, y, b.w, h, border, fill, bottom);
        c.ptext_c(b.x + b.w / 2, (y + 2 + y + h - bottom) / 2 - 7, b.face, 2, ink);
    }

    // the keys: naturals a dim outline, sharps dim and solid; held warm white; lit, an LED on the key (Theo
    // 2026-11-23: the whole key yellow was too much while they blink): its outline yellow and a bar across its top
    dotted(c, KEYBED[0], KEYBED[0] + OCT_COL + kb_w, KEYBED[1], T_DIM);
    for (size_t i = 0; i < keys.size(); i++)
    {
        const Key &k = keys[i];
        uint32_t bit = 1u << k.k;
        bool natural = i < 16;
        uint32_t base = natural ? T_BLACK : T_DIM, border = T_DIM, fill = base, led = 0;
        if (input.keys & bit) border = fill = T_HELD;
        else if (v.lit_keys & bit) border = T_YEL, led = T_YEL;
        else if (v.breath_keys & bit) border = T_YEL, led = mix(base, T_YEL, 0.6 * breath);
        else if (v.dim_keys & bit) border = T_YEL;
        nbox(c, k.cx - kw / 2, k.cy - kh / 2, kw, kh, border, fill);
        if (led)
        {
            int lw = kw * 2 / 3, lh = kh / 8 < 4 ? 4 : kh / 8;
            c.box(k.cx - lw / 2, k.cy - kh / 2 + 6, lw, lh, led);
        }
    }
    draw_key_labels(px, mix(T_BODY, T_YEL, 0.62));
    for (auto &b : buttons)   // the page has a sheet: say so under OK (held: the sheet; the mouse holds it as long)
        if (b.bit == B_OCTUP && sheet_ && !(fm1_on && fm1_mine))
            c.ptext_c(b.x + b.w / 2, b.y + b.h + 6, "HOLD: MORE", 1, T_YEL);

    // OPEN, SAVE and the bank switch, centred under the keys; what the last one did beside them
    for (size_t i = 0; i < files.size(); i++)
    {
        const FileButton &f = files[i];
        bool held = file_held == (int)i;
        nbox(c, f.x, f.y, f.w, f.h, held ? T_HELD : T_YEL, held ? T_HELD : T_BODY);
        c.ptext_c(f.x + f.w / 2, f.y + f.h / 2 - 7, f.face, 2, held ? T_BLACK : T_YEL);
    }
    if (banked())
        for (int i = 0; i < 2; i++)
        {
            const FileButton &b = banks[i];
            bool lit = (track_now & 4) == b.action;
            nbox(c, b.x, b.y, b.w, b.h, T_YEL, lit ? T_YEL : T_BODY);
            c.ptext_c(b.x + b.w / 2, b.y + b.h / 2 - 7, b.face, 2, lit ? T_BLACK : T_YEL);
        }
    if (note_frames > 0)
    {
        note_frames--;
        const FileButton &last = banked() ? banks[1] : files.back();
        c.ptext(last.x + last.w + 16, last.y + last.h / 2 - 7, note_text.c_str(), 2, T_WORD);
    }

    draw_gen(px);
    if (!menu_open)
        return;

    // the menu: SHELL, INK; MODE, SIZE; ABOUT
    nbox(c, MENU_X, MENU_Y, MENU_W, MENU_H, T_DIM, T_BLACK);
    static const char *NAMES[MENU_ROWS] = {"SHELL", "INK", "MODE", "SIZE", "FM-1 USB", "KEYS", "HOVER HELP",
                                                "LISTEN", "SPLIT", "ABOUT"};
    for (int r = 0; r < MENU_ROWS; r++)
    {
        int y = row_y(r);
        bool on = menu_hover == r || (r == ROW_SHELL && menu_sub == 1) || (r == ROW_INK && menu_sub == 2);
        bool active = r == ROW_SHELL || r == ROW_INK;
        if (on && active) c.box(MENU_X + 2, y, MENU_W - 4, ROW_H, T_DIM);
        c.ptext(MENU_X + 16, y + 11, NAMES[r], 2, on && active ? T_YEL : T_WORD);
        int right = MENU_X + MENU_W - 16;
        // a choice of words, right to left: the chosen one solid yellow, the ones not there yet greyed
        auto seg = [&](const char *t, bool chosen, bool there) {
            int w = Canvas::ptext_w(t, 2) + 12;
            right -= w;
            if (chosen) c.box(right, y + 7, w, 22, T_YEL);
            c.ptext(right + 6, y + 11, t, 2, chosen ? T_BLACK : there ? T_WORD : T_OFF);
            right -= 4;
        };
        if (r == ROW_SHELL || r == ROW_INK)
        {
            c.ptext(right - Canvas::ptext_w(">", 2), y + 11, ">", 2, T_YEL);
            right -= Canvas::ptext_w(">", 2) + 10;
            if (r == ROW_SHELL)
                c.ptext(right - Canvas::ptext_w(SHELLS[shell_].name, 2), y + 11, SHELLS[shell_].name, 2, T_YEL);
            else if (ink_now >= 0 && ink_now < (int)inks.size())   // (as its swatch: a four-ink palette in quarters)
            {
                const uint32_t *k = inks[ink_now].rgb;
                c.box(right - 14, y + 11, 7, 7, 0xFF000000u | k[0]);
                c.box(right - 7, y + 11, 7, 7, 0xFF000000u | k[1]);
                c.box(right - 7, y + 18, 7, 7, 0xFF000000u | k[2]);
                c.box(right - 14, y + 18, 7, 7, 0xFF000000u | k[3]);
            }
        }
        else if (r == ROW_MODE)   // FM-1: the FM-1's panel; EXPANDED: and TRACKS beside its screen
        {
            seg("EXPANDED", expanded_, true);
            seg("FM-1", !expanded_, true);
        }
        else if (r == ROW_SIZE)   // the window's size (EXPANDED: 100 or 125)
            for (int i = 0; i < 3; i++)
            {
                int pct = size_choice(expanded_, i);
                if (!pct) break;
                char t[8];
                std::snprintf(t, sizeof t, "%d", pct);
                seg(t, size_now == pct, true);
            }
        else if (r == ROW_FM1)   // OFF / ON, and under them what the FM-1 is doing
        {
            seg("ON", fm1_on, true);
            seg("OFF", !fm1_on, true);
            if (fm1_on)
                c.ptext(MENU_X + 16, y + ROW_H + 2, fm1_text.c_str(), 2, fm1_live ? T_YEL : T_WORD);
        }
        else if (r == ROW_KEYS)   // the computer keyboard: Tessera's (ON) or the DAW's (OFF)
        {
            seg("ON", keys_on_, true);
            seg("OFF", !keys_on_, true);
        }
        else if (r == ROW_HOVER)   // the help text on hover (OFF: the name and value only)
        {
            seg("ON", hover_help_, true);
            seg("OFF", !hover_help_, true);
        }
        else if (r == ROW_LISTEN || r == ROW_SPLIT)   // < OFF > / < CH 16 >, < 48 >: fixed widths (LISTEN_SEG)
        {
            char t[8];
            bool sp = r == ROW_SPLIT;
            if (sp) std::snprintf(t, sizeof t, "%d", split_note);
            else std::snprintf(t, sizeof t, listen_ch ? "CH %d" : "OFF", listen_ch);
            const char *words[3] = {"<", t, ">"};
            for (int i = 0; i < 3; i++)
            {
                int l = SEG_RIGHT - LISTEN_SEG[i][1], w = LISTEN_SEG[i][1] - LISTEN_SEG[i][0];
                bool chosen = i == 1 && !sp && listen_ch > 0,
                     there = sp ? (i == 1 && listen_ch > 0) || (i == 0 && split_note > 12) || (i == 2 && split_note < 108)
                                : i == 1 || (i == 0 ? listen_ch > 0 : listen_ch < 16);
                if (chosen) c.box(l, y + 7, w, 22, T_YEL);
                c.ptext_c(l + w / 2, y + 11, words[i], 2, chosen ? T_BLACK : there ? T_WORD : T_OFF);
            }
        }
        else
        {
            std::string ver = version.empty() ? "COSMATI" : version;
            for (auto &ch : ver) ch = ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
            c.ptext(right - Canvas::ptext_w(ver.c_str(), 2), y + 11, ver.c_str(), 2, T_WORD);
        }
    }
    dotted(c, MENU_X + 16, MENU_X + MENU_W - 16, row_y(ROW_INK) + ROW_H + 8, T_DIM);
    dotted(c, MENU_X + 16, MENU_X + MENU_W - 16, row_y(ROW_SIZE) + ROW_H + 8, T_DIM);
    dotted(c, MENU_X + 16, MENU_X + MENU_W - 16, row_y(ROW_SPLIT) + ROW_H + 8, T_DIM);

    if (menu_sub == 1)   // the shells, the one shown marked
    {
        int y0 = shell_sub_y(), h = 20 + NSHELLS * SUB_ROW;
        nbox(c, SUB_X, y0, 232, h, T_DIM, T_BLACK);
        for (int j = 0; j < NSHELLS; j++)
        {
            int y = y0 + 10 + j * SUB_ROW;
            if (sub_hover == j) c.box(SUB_X + 2, y, 228, SUB_ROW, T_DIM);
            if (j == shell_) c.box(SUB_X + 16, y + 11, 8, 8, T_YEL);
            c.ptext(SUB_X + 34, y + 8, SHELLS[j].name, 2, sub_hover == j ? T_YEL : T_WORD);
        }
    }
    else if (menu_sub == 2 && !inks.empty())   // the inks as swatches (a four-ink palette in quarters), the name below
    {
        int n = (int)inks.size(), rows = (n + INK_COLS - 1) / INK_COLS, y0 = ink_sub_y();
        int w = 36 + INK_COLS * INK_S + (INK_COLS - 1) * INK_GAP, gh = rows * INK_S + (rows - 1) * INK_GAP;
        nbox(c, SUB_X, y0, w, 18 + gh + 12 + 14 + 18, T_DIM, T_BLACK);
        for (int j = 0; j < n; j++)
        {
            int sx = SUB_X + 18 + j % INK_COLS * (INK_S + INK_GAP), sy = y0 + 18 + j / INK_COLS * (INK_S + INK_GAP);
            uint32_t ring = j == ink_now ? T_HELD : j == sub_hover ? T_YEL : T_DIM;
            nbox(c, sx, sy, INK_S, INK_S, ring, T_BLACK);
            const uint32_t *k = inks[j].rgb;
            const int half = (INK_S - 8) / 2;
            c.box(sx + 4, sy + 4, half, half, 0xFF000000u | k[0]);
            c.box(sx + 4 + half, sy + 4, half, half, 0xFF000000u | k[1]);
            c.box(sx + 4 + half, sy + 4 + half, half, half, 0xFF000000u | k[2]);
            c.box(sx + 4, sy + 4 + half, half, half, 0xFF000000u | k[3]);
        }
        int shown = sub_hover >= 0 ? sub_hover : ink_now;
        if (shown >= 0 && shown < n)
            c.ptext(SUB_X + 18, y0 + 18 + gh + 12, inks[shown].name.c_str(), 2, T_YEL);
    }
}

// ---- GENERATE's strip (TESSERA's shell)

void Panel::set_gen(const GenState &g)
{
    int style = gen.style, var = gen.variation;
    gen = g;
    if (gen_req.what == GEN_SET)   // (a pick not yet taken: the panel's, until it is)
        gen.style = style, gen.variation = var;
}

int Panel::gen_part_at(int x, int y) const
{
    if (!gen_on() || y < GEN_Y || y >= GEN_Y + GEN_H)
        return -1;
    if (x >= GB_X && x < GB_X + GB_W) return GP_BUTTON;
    if (x >= GS_X && x < GS_X + GS_ARROW) return GP_PREV;
    if (x >= GS_X + GS_W - GS_ARROW && x < GS_X + GS_W) return GP_NEXT;
    if (x >= GS_X && x < GS_X + GS_W) return GP_STYLE;
    if (x >= GV_X && x < GV_X + GV_W) return GP_VAR;
    if (x >= GD_X && x < GD_X + GD_W) return GP_SEED;
    return -1;
}

int Panel::gen_list_at(int x, int y) const
{
    int j = (y - GL_Y - 10) / GL_ROW;
    if (!gen_list || x < GS_X || x >= GS_X + GS_W || y < GL_Y + 10 || j < 0 || j >= (int)gen_names.size())
        return -1;
    return j;
}

void Panel::gen_ask(int what, int dir)
{
    if (gen_req.what != GEN_NONE && gen_req.what != GEN_SET && what == GEN_SET)
        return;   // (a press waiting: the pick rides along with it)
    gen_req.what = what;
    gen_req.style = gen.style;
    gen_req.variation = gen.variation;
    gen_req.dir = dir;
}

void Panel::gen_style_step(int n)
{
    int k = (int)gen_names.size();
    if (!k || !n) return;
    gen.style = ((gen.style + n) % k + k) % k;
    gen_ask(GEN_SET);
}

void Panel::gen_var_step(int n)
{
    int v = gen.variation + 5 * n;
    v = v < 0 ? 0 : v > 100 ? 100 : v;
    if (v == gen.variation) return;
    gen.variation = v;
    gen_ask(GEN_SET);
}

void Panel::draw_gen(uint32_t *px)
{
    Canvas c{px, width()};
    if (!gen_on())
        return;
    // held half a second: BEFORE / AFTER, once a press
    if (drag == GEN_BUTTON && !gen_swapped && frame - gen_down_frame >= 15)
    {
        gen_swapped = true;
        gen_ask(GEN_SWAP);
    }
    bool held = drag == GEN_BUTTON;
    int lip = held ? 2 : 0;
    nbox(c, GB_X, GEN_Y + lip, GB_W, GEN_H - lip, held ? T_HELD : T_YEL, held ? T_HELD : T_BODY, held ? 2 : 4);
    c.ptext_c(GB_X + GB_W / 2, GEN_Y + lip + 6, gen_swapped && held ? (gen.before ? "BEFORE" : "AFTER") : "GENERATE", 2,
              held ? T_BLACK : T_YEL);

    // the style: < NAME >
    nbox(c, GS_X, GEN_Y, GS_W, GEN_H, gen_list ? T_YEL : T_DIM, T_BLACK);
    c.ptext(GS_X + 8, GEN_Y + 7, "<", 2, T_YEL);
    c.ptext(GS_X + GS_W - 8 - Canvas::ptext_w(">", 2), GEN_Y + 7, ">", 2, T_YEL);
    const char *name = gen.style >= 0 && gen.style < (int)gen_names.size() ? gen_names[gen.style].c_str() : "";
    c.ptext_c(GS_X + GS_W / 2, GEN_Y + 7, name, 2, T_YEL);

    // VAR 0..100
    char b[24];
    std::snprintf(b, sizeof b, "VAR %d", gen.variation);
    nbox(c, GV_X, GEN_Y, GV_W, GEN_H, drag == GEN_VAR ? T_YEL : T_DIM, T_BLACK);
    c.ptext_c(GV_X + GV_W / 2, GEN_Y + 7, b, 2, drag == GEN_VAR ? T_HELD : T_WORD);

    // the seed: the piece's (BEFORE while the project before it plays), - none yet
    if (gen.before) std::snprintf(b, sizeof b, "BEFORE");
    else if (gen.made) std::snprintf(b, sizeof b, "%08X", (unsigned)gen.seed);
    else std::snprintf(b, sizeof b, "-");
    nbox(c, GD_X, GEN_Y, GD_W, GEN_H, T_DIM, T_BLACK);
    c.ptext_c(GD_X + GD_W / 2, GEN_Y + 7, b, 2, gen.before ? T_HELD : T_WORD);

    if (!gen_list)
        return;
    int n = (int)gen_names.size(), h = 20 + n * GL_ROW;
    nbox(c, GS_X, GL_Y, GS_W, h, T_DIM, T_BLACK);
    for (int j = 0; j < n; j++)
    {
        int y = GL_Y + 10 + j * GL_ROW;
        if (gen_list_hover == j) c.box(GS_X + 2, y, GS_W - 4, GL_ROW, T_DIM);
        if (j == gen.style) c.box(GS_X + 14, y + 9, 8, 8, T_YEL);
        c.ptext(GS_X + 30, y + 6, gen_names[j].c_str(), 2, gen_list_hover == j ? T_YEL : T_WORD);
    }
}

int Panel::menu_row_at(int x, int y) const
{
    if (x < MENU_X || x >= MENU_X + MENU_W)
        return -1;
    for (int r = 0; r < MENU_ROWS; r++)
        if (y >= row_y(r) && y < row_y(r) + ROW_H)
            return r;
    return -1;
}

int Panel::sub_at(int x, int y) const
{
    if (menu_sub == 1)
    {
        int j = (y - shell_sub_y() - 10) / SUB_ROW;
        if (x >= SUB_X && x < SUB_X + 232 && y >= shell_sub_y() + 10 && j >= 0 && j < NSHELLS)
            return j;
    }
    else if (menu_sub == 2)
        for (int j = 0; j < (int)inks.size(); j++)
        {
            int sx = SUB_X + 18 + j % INK_COLS * (INK_S + INK_GAP), sy = ink_sub_y() + 18 + j / INK_COLS * (INK_S + INK_GAP);
            if (x >= sx && x < sx + INK_S && y >= sy && y < sy + INK_S)
                return j;
        }
    return -1;
}

// a click with the menu open: a shell or an ink picked (the menu shuts), SHELL or INK opens its list, outside shuts it
bool Panel::menu_click(int x, int y)
{
    int j = sub_at(x, y);
    if (j >= 0 && menu_sub == 1)
    {
        menu_open = false;
        if (j != shell_)
        {
            set_shell(j);
            shell_req = j;
        }
        return false;
    }
    if (j >= 0 && menu_sub == 2)
    {
        menu_open = false;
        if (j != ink_now)
            ink_now = ink_pending = ink_req = j;
        return false;
    }
    int r = menu_row_at(x, y);
    if (r == ROW_FM1)   // OFF or ON (a click elsewhere on the row: the other one); ON also takes it from another
    {
        int sg = fm1_seg_at(x);
        fm1_req = sg >= 0 ? sg : fm1_on && fm1_mine ? 0 : 1;
        menu_sub = 0;
        return false;
    }
    if (r == ROW_KEYS)   // OFF or ON (elsewhere on the row: the other one)
    {
        int sg = fm1_seg_at(x);
        bool on = sg >= 0 ? sg == 1 : !keys_on_;
        if (on != keys_on_)
        {
            set_keys(on);
            keys_req = on;
        }
        menu_sub = 0;
        return false;
    }
    if (r == ROW_HOVER)   // OFF or ON (elsewhere on the row: the other one)
    {
        int sg = fm1_seg_at(x);
        bool on = sg >= 0 ? sg == 1 : !hover_help_;
        if (on != hover_help_)
        {
            set_hover_help(on);
            hover_req = on;
        }
        menu_sub = 0;
        return false;
    }
    if (r == ROW_LISTEN)   // < and > step the channel, the value turns it OFF or on again
    {
        int sg = listen_seg_at(x), ch = listen_ch;
        if (sg == 0) ch = ch > 0 ? ch - 1 : 0;
        else if (sg == 2) ch = ch < 16 ? ch + 1 : 16;
        else if (sg == 1) ch = ch ? 0 : listen_last;
        if (sg >= 0 && ch != listen_ch)
        {
            set_listen(ch);
            listen_req = ch;
        }
        menu_sub = 0;
        return false;
    }
    if (r == ROW_SPLIT)   // < and > move it an octave (a C, 12..108)
    {
        int sg = listen_seg_at(x), n = split_note + (sg == 0 ? -12 : sg == 2 ? 12 : 0);
        if (n >= 12 && n <= 108 && n != split_note)
        {
            split_note = n;
            split_req = n;
        }
        menu_sub = 0;
        return false;
    }
    if (r == ROW_MODE)   // FM-1 or EXPANDED (elsewhere on the row: nothing)
    {
        int m = mode_seg_at(x);
        if (m >= 0 && m != (int)expanded_)
        {
            set_expanded(m == 1);   // (at once; the window follows)
            mode_req = m;
        }
        if (m >= 0)
            menu_open = false;
        menu_sub = 0;
        return false;
    }
    if (r == ROW_SIZE)   // 100, 150 or 200; EXPANDED 100 or 125 (elsewhere on the row: nothing)
    {
        int pct = size_seg_at(x);
        if (pct > 0 && pct != size_now)
            size_req = pct;
        if (pct > 0)
            menu_open = false;   // (picked: the window changes size)
        menu_sub = 0;
        return false;
    }
    if (r == ROW_SHELL) menu_sub = menu_sub == 1 ? 0 : 1;
    else if (r == ROW_INK) menu_sub = menu_sub == 2 ? 0 : 2;
    else if (r < 0) menu_open = false;   // (ABOUT: nothing to pick)
    sub_hover = -1;
    return false;
}

int Panel::size_seg_at(int x) const
{
    int right = SEG_RIGHT;
    for (int i = 0; i < 3 && size_choice(expanded_, i); i++)   // (right to left, as drawn)
    {
        char t[8];
        std::snprintf(t, sizeof t, "%d", size_choice(expanded_, i));
        int l = right - seg_w(t);
        if (x >= l && x < right) return size_choice(expanded_, i);
        right = l - 4;
    }
    return -1;
}

int Panel::mode_seg_at(int x) const
{
    int ex_l = SEG_RIGHT - seg_w("EXPANDED"), fm_r = ex_l - 4, fm_l = fm_r - seg_w("FM-1");
    if (x >= ex_l && x < SEG_RIGHT) return 1;
    if (x >= fm_l && x < fm_r) return 0;
    return -1;
}

void Panel::scale(const uint32_t *px, uint32_t *out, int w, int pct)
{
    const int ow = w * pct / 100, oh = H * pct / 100;
    if (pct == 100)
    {
        std::memcpy(out, px, sizeof(uint32_t) * w * H);
        return;
    }
    // each window pixel: the 1:1 pixels under its two quarter points, across and down
    auto q = [pct](int o, int k) { return (4 * o + k) * 100 / (4 * pct); };
    std::vector<int> xa(ow), xb(ow);
    for (int x = 0; x < ow; x++) xa[x] = q(x, 1), xb[x] = q(x, 3);
    for (int y = 0; y < oh; y++)
    {
        const uint32_t *ra = px + q(y, 1) * w, *rb = px + q(y, 3) * w;
        uint32_t *o = out + y * ow;
        for (int x = 0; x < ow; x++)
        {
            uint32_t p0 = ra[xa[x]], p1 = ra[xb[x]], p2 = rb[xa[x]], p3 = rb[xb[x]];
            if (p0 == p1 && p0 == p2 && p0 == p3) { o[x] = p0; continue; }
            uint32_t r = ((p0 >> 16 & 255) + (p1 >> 16 & 255) + (p2 >> 16 & 255) + (p3 >> 16 & 255) + 2) / 4;
            uint32_t g = ((p0 >> 8 & 255) + (p1 >> 8 & 255) + (p2 >> 8 & 255) + (p3 >> 8 & 255) + 2) / 4;
            uint32_t b = ((p0 & 255) + (p1 & 255) + (p2 & 255) + (p3 & 255) + 2) / 4;
            o[x] = 0xFF000000u | r << 16 | g << 8 | b;
        }
    }
}

int Panel::listen_seg_at(int x) const
{
    for (int i = 0; i < 3; i++)
        if (x >= SEG_RIGHT - LISTEN_SEG[i][1] && x < SEG_RIGHT - LISTEN_SEG[i][0]) return i;
    return -1;
}

int Panel::fm1_seg_at(int x) const
{
    int on_l = SEG_RIGHT - seg_w("ON"), off_r = on_l - 4, off_l = off_r - seg_w("OFF");
    if (x >= on_l && x < SEG_RIGHT) return 1;
    if (x >= off_l && x < off_r) return 0;
    return -1;
}

// the keys' jobs on this page (tess_view_t.key_short): a tiny label in the Mono face just over each key that has one
void Panel::draw_key_labels(uint32_t *px, uint32_t col)
{
    Canvas c{px, width()};
    for (const Key &k : keys)
    {
        const char *t = key_short[k.k];
        if (t[0]) c.ptext_c(k.cx, k.cy - kh / 2 - 9, t, 1, col);
    }
}

// ---- the screen's zones (tess_view_t.zones): where the page put its values, rows and keys; TRACKS' (EXPANDED)
int Panel::zone2_at(int x, int y) const
{
    if (!expanded() || menu_open) return -1;
    int sx = x - (T2_X + T2_IN), sy = y - (T2_Y + T2_IN);
    if (sx < 0 || sy < 0 || sx >= TESS_TRACKS_W || sy >= TESS_TRACKS_H) return -1;
    for (size_t i = 0; i < zones2.size(); i++)
    {
        const tess_zone_t &z = zones2[i];
        if (sx >= z.x && sx < z.x + z.w && sy >= z.y && sy < z.y + z.h) return (int)i;
    }
    return -1;
}

int Panel::zone_at(int x, int y) const
{
    if (native() && menu_open) return -1;
    int sx = x - (SCREEN_X + SCREEN_S / 32), sy = y - (SCREEN_Y + SCREEN_S / 32);
    if (sx < 0 || sy < 0 || sx >= TESS_SCREEN || sy >= TESS_SCREEN) return -1;
    for (size_t i = 0; i < zones.size(); i++)
    {
        const tess_zone_t &z = zones[i];
        if (sx >= z.x && sx < z.x + z.w && sy >= z.y && sy < z.y + z.h) return (int)i;
    }
    return -1;
}

int Panel::zone_like(const tess_zone_t &z) const
{
    for (size_t i = 0; i < zones.size(); i++)
        if (zones[i].kind == z.kind && zones[i].role == z.role && zones[i].vrole == z.vrole && zones[i].row == z.row)
            return (int)i;
    return -1;
}

// NOTES' roll (TESS_ZONE_ROLL): the core reads the step and the pitch from the place, so it goes in the zone's x, y (in
// screen pixels); any other zone as it is
tess_zone_t Panel::zone_here(int zi, int x, int y) const
{
    tess_zone_t z = zones[zi];
    if (z.kind == TESS_ZONE_ROLL)
        z.x = (uint16_t)(x - (SCREEN_X + SCREEN_S / 32)), z.y = (uint16_t)(y - (SCREEN_Y + SCREEN_S / 32));
    return z;
}

void Panel::zone_act(const tess_zone_t &z, uint32_t action, int n)
{
    input.zones.push_back({z, action, n});
    input.changed = true;
}

// the zone under the mouse (or being dragged): a hairline round it on the screen, and under the screen a card with its
// name and its value large (the readability the FM-1's 5 x 7 text lacks), black with Tessera yellow
void Panel::draw_hover(uint32_t *px)
{
    int zi = drag == ZONE ? zone_like(drag_zone) : hover_in && drag == NONE ? zone_at(hover_x, hover_y) : -1;
    int z2 = zi < 0 && hover_in && drag == NONE ? zone2_at(hover_x, hover_y) : -1;   // (TRACKS, EXPANDED)
    int bi = zi < 0 && z2 < 0 && hover_in && drag == NONE && !(native() && menu_open) ? button_at(hover_x, hover_y) : -1;
    if (bi >= 0 && buttons[bi].tip)   // a button of TESSERA's own: what it does, in a card over it
    {
        Canvas c{px, width()};
        const Button &b = buttons[bi];
        int w = Canvas::ptext_w(b.tip, 2) + 20, h = 30, x = b.x + b.w / 2 - w / 2, y = b.y - h - 6;
        if (x < 8) x = 8;
        if (x + w > width() - 8) x = width() - 8 - w;
        c.box(x, y, w, h, 0xFF000000);
        c.box(x, y, w, 2, 0xFFFFD020), c.box(x, y + h - 2, w, 2, 0xFFFFD020);
        c.box(x, y, 2, h, 0xFFFFD020), c.box(x + w - 2, y, 2, h, 0xFFFFD020);
        c.ptext(x + 10, y + 8, b.tip, 2, 0xFFB9B2A0);
        return;
    }
    int ki = zi < 0 && z2 < 0 && bi < 0 && hover_in && drag == NONE && !(native() && menu_open) ? key_at(hover_x, hover_y)
                                                                                                 : -1;
    if (ki >= 0 && key_long[keys[ki].k][0])   // a key with a job on this page: its meaning, in a card over it
    {
        Canvas c{px, width()};
        const Key &k = keys[ki];
        const char *t = key_long[k.k];
        int w = Canvas::ptext_w(t, 2) + 20, h = 30, x = k.cx - w / 2, y = k.cy - kh / 2 - h - 14;
        if (x < 8) x = 8;
        if (x + w > width() - 8) x = width() - 8 - w;
        c.box(x, y, w, h, 0xFF000000);
        c.box(x, y, w, 2, 0xFFFFD020), c.box(x, y + h - 2, w, 2, 0xFFFFD020);
        c.box(x, y, 2, h, 0xFFFFD020), c.box(x + w - 2, y, 2, h, 0xFFFFD020);
        c.ptext(x + 10, y + 8, t, 2, 0xFFFFD020);
        return;
    }
    if (zi < 0 && z2 < 0) return;
    const tess_zone_t &z = z2 >= 0 ? zones2[z2] : zones[zi];
    Canvas c{px, width()};
    const uint32_t YEL = 0xFFFFD020, WORD = 0xFFB9B2A0, BLACK = 0xFF000000;
    const int SX = z2 >= 0 ? T2_X : SCREEN_X, SW = z2 >= 0 ? T2_W : SCREEN_S;   // (the screen it is on)
    int ox = z2 >= 0 ? T2_X + T2_IN : SCREEN_X + SCREEN_S / 32, oy = z2 >= 0 ? T2_Y + T2_IN : SCREEN_Y + SCREEN_S / 32;
    c.box(ox + z.x, oy + z.y, z.w, 1, YEL);
    c.box(ox + z.x, oy + z.y + z.h - 1, z.w, 1, YEL);
    c.box(ox + z.x, oy + z.y, 1, z.h, YEL);
    c.box(ox + z.x + z.w - 1, oy + z.y, 1, z.h, YEL);
    if (!z.name[0] && !z.value[0]) return;
    // the card: under the screen (it never covers it), centred on the zone, one line: the name, then the value large
    std::string name = z.name, value = z.value;
    for (auto &ch : name) ch = ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
    if (input.key_words && z.kind == TESS_ZONE_KEY)   // (BACK / OK of its own: named as the screen names them)
        value = value == "OCT+" ? "OK" : value == "OCT-" ? "BACK" : value;
    int nw = Canvas::ptext_w(name.c_str(), 2), vw = value.empty() ? 0 : Canvas::ptext_w(value.c_str(), 3);
    int w = nw + (vw ? vw + 14 : 0) + 20, h = 34;
    int x = ox + z.x + z.w / 2 - w / 2, y = SCREEN_Y + SCREEN_S + 3;
    if (x < SX - 80) x = SX - 80;
    if (x + w > SX + SW + 80) x = SX + SW + 80 - w;
    c.box(x, y, w, h, BLACK);
    c.box(x, y, w, 2, YEL), c.box(x, y + h - 2, w, 2, YEL);
    c.box(x, y, 2, h, YEL), c.box(x + w - 2, y, 2, h, YEL);
    c.ptext(x + 10, y + 11, name.c_str(), 2, WORD);
    if (vw) c.ptext(x + 10 + nw + 14, y + 7, value.c_str(), 3, YEL);
    // HOVER HELP: resting half a second, the zone's help text in a box under the card (over the keys, never the screen)
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (!hover_help_ || z2 >= 0 || drag != NONE || view_help.empty() || now - hover_t < 0.5 ||
        view_hover.kind != z.kind || view_hover.role != z.role || view_hover.vrole != z.vrole || view_hover.row != z.row)
        return;
    const int cw = Canvas::ptext_w("M", 2) + 1, hw = 480, per = (hw - 24) / (cw > 0 ? cw : 12), lh = 20;
    std::vector<std::string> lines;   // the text in words, wrapped to the box, eight lines at most
    {
        std::string word, line;
        auto put = [&](const std::string &w) {
            if (!line.empty() && (int)(line.size() + 1 + w.size()) > per) { lines.push_back(line); line.clear(); }
            line += (line.empty() ? "" : " ") + w;
        };
        for (char ch : view_help + " ")
        {
            char u = ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
            if (u == ' ' || u == '\n') { if (!word.empty()) put(word); word.clear(); if (u == '\n' && !line.empty()) { lines.push_back(line); line.clear(); } }
            else word += u;
        }
        if (!line.empty()) lines.push_back(line);
        if (lines.size() > 8) { lines.resize(8); lines.back() += " ..."; }
    }
    int bh = 16 + (int)lines.size() * lh, bx = x + w / 2 - hw / 2, by = y + h + 4;
    if (bx < 8) bx = 8;
    if (bx + hw > width() - 8) bx = width() - 8 - hw;
    if (by + bh > H - 4) by = H - 4 - bh;
    c.box(bx, by, hw, bh, BLACK);
    c.box(bx, by, hw, 2, YEL), c.box(bx, by + bh - 2, hw, 2, YEL);
    c.box(bx, by, 2, bh, YEL), c.box(bx + hw - 2, by, 2, bh, YEL);
    for (size_t i = 0; i < lines.size(); i++)
        c.ptext(bx + 12, by + 9 + (int)i * lh, lines[i].c_str(), 2, WORD);
}
