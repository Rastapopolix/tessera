/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window's FM-1 (panel.h): web/sim/index.html's wide layout and its shells, or TESSERA's own shell, drawn
 * into a pixel buffer */
#include "panel.h"

#include <cmath>
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
    // TESSERA's own (draw_native; these colours only make its swatch among the FM-1's)
    {"tessera", "TESSERA", "YELLOW", 0xFF111111, 0xFF111111, 0xFFFFD020, 0xFF3B3007, 0xFFFFD020, 0xFF3B3007, 0xFFB9B2A0,
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
    void blend(int x, int y, uint32_t c, double a)
    {
        if (x < 0 || y < 0 || x >= Panel::W || y >= Panel::H || a <= 0)
            return;
        uint32_t &d = px[y * Panel::W + x];
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
const int KEYBED[4] = {36, 332, 928, 256};
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
    const int x0 = KEYBED[0], y0 = KEYBED[1], w = KEYBED[2], h = KEYBED[3];
    pitch = (w - 24) / 16;
    kw = (int)(pitch * 0.78);
    kh = (int)(h * 0.425);
    sw = kw * 14 / 100 > 3 ? kw * 14 / 100 : 3;
    for (int i = 0; i < 16; i++)
        keys.push_back({x0 + 12 + (int)((i + 0.5) * pitch), y0 + h * 3 / 4, WHITE_K[i]});
    for (int j = 0; j < 11; j++)
        keys.push_back({x0 + 12 + (BLACK_AFTER[j] + 1) * pitch, y0 + h / 4, BLACK_K[j]});
}

// the buttons and the files where the shell has them: the FM-1's as the web sim's wide layout; TESSERA's grid a little
// wider (room round the words), its files centred under the keys
void Panel::layout()
{
    const bool n = native();
    buttons.clear();
    buttons.push_back({60, 258, 90, 34, B_OCTDN, "OCT-", nullptr});
    buttons.push_back({170, 258, 90, 34, B_OCTUP, "OCT+", nullptr});
    const int R1[6] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO}, R2[6] = {B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC};
    const char *F1[6] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO"}, *F2[6] = {"HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"};
    const int bx = n ? 574 : 582, step = n ? 66 : 63, bw = n ? 60 : 54;
    for (int i = 0; i < 6; i++)
    {
        buttons.push_back({bx + i * step, 174, bw, 48, R1[i], F1[i], nullptr});
        buttons.push_back({bx + i * step, 174 + 62, bw, 48, R2[i], F2[i], R2[i] == B_PLAY ? "STOP" : nullptr});
    }
    files.clear();
    const int fy = KEYBED[1] + KEYBED[3] + 5;
    if (!n)
    {
        files.push_back({60, fy, 72, 24, FILE_OPEN, "OPEN"});   // (in the shell, under the keys)
        files.push_back({142, fy, 72, 24, FILE_SAVE, "SAVE"});
        banks[0] = {234, fy, 52, 24, 0, "1-4"};   // (one switch, two halves)
        banks[1] = {286, fy, 52, 24, 4, "5-8"};
        usb = {tracks_ > 4 ? 348 : 224, fy, 112, 24, 0, "FM-1 USB"};
        return;
    }
    const int total = 72 + 10 + 72 + (tracks_ > 4 ? 20 + 104 : 0);
    int x = W / 2 - total / 2;
    files.push_back({x, fy - 1, 72, 26, FILE_OPEN, "OPEN"});
    files.push_back({x + 82, fy - 1, 72, 26, FILE_SAVE, "SAVE"});
    banks[0] = {x + 174, fy - 1, 52, 26, 0, "1-4"};
    banks[1] = {x + 226, fy - 1, 52, 26, 4, "5-8"};
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
    if (fm1_known && (on ? text != before : was))
        note(on ? text : "FM-1 USB OFF");
    else if (!fm1_known && on)
        note(text);
    fm1_known = true;
}

int Panel::bank_at(int x, int y) const
{
    if (tracks_ <= 4) return -1;
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

void Panel::draw(uint32_t *px, const tess_view_t &v)
{
    Canvas c{px};
    const Shell &sh = SHELLS[shell_];
    const uint32_t BODY = sh.body, TRAY = sh.tray, BTN = sh.btn, EDGE = sh.edge, BTN_INK = sh.btn_ink, SLIT = sh.slit,
                   LABEL = sh.label, LIT_INK = sh.lit_ink ? sh.lit_ink : sh.btn_ink, LIT = sh.lit ? sh.lit : 0xFFF6F6F6,
                   REC = sh.rec ? sh.rec : 0xFFC62A22;
    master_now = (int)v.master;
    knobs[0].angle = -135 + 270.0 * (input.master >= 0 ? input.master : master_now) / 1023;
    static const double kPi = 3.14159265358979;
    // the breathing LEDs: dark .. lit and back in ~1.13 s (fm1_led_breath)
    double breath = 0.5 - 0.5 * std::cos(2 * kPi * (frame++ / 30.0) / 1.13);
    if (ink_pending < 0 || (int)v.ink == ink_pending)   // (a pick stays ringed until the view has it)
    {
        ink_now = (int)v.ink;
        ink_pending = -1;
    }
    track_now = (int)v.track;
    if (native())
    {
        draw_native(px, v, breath);
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
    if (tracks_ > 4)
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
    input.keys = mouse_keys | kbd_keys;
    input.buttons = mouse_buttons | kbd_buttons;
    input.changed = true;
}

// web/sim/index.html KEYMAP: F3..A#4 on the Z (white) and A (black) rows, B4..G5 on Q..O in semitones, the twelve
// buttons on the number row in the panel's order, [ ] OCT
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
    if (code == 0x1A) b = B_OCTDN;
    if (code == 0x1B) b = B_OCTUP;
    if (code == 0x0F && tracks_ > 4)   // Tab: the other bank (T1 or T5)
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
        int m = (input.master >= 0 ? input.master : master_now) + n * 16;
        input.master = m < 0 ? 0 : m > 1023 ? 1023 : m;
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
    acc = 0;
    if (native() && menu_open)   // (a click with the menu open is the menu's, inside or out)
    {
        drag = NONE;
        return menu_click(x, y);
    }
    if (native() && x >= 30 && x < 250 && y >= 6 && y < 44)   // the mark: the menu
    {
        drag = NONE;
        menu_open = true;
        menu_sub = 0;
        menu_hover = sub_hover = -1;
        return false;
    }
    if ((drag_i = knob_at(x, y)) >= 0) { drag = KNOB; return false; }
    if ((drag_i = button_at(x, y)) >= 0)
    {
        drag = BUTTON;
        mouse_buttons |= 1u << buttons[drag_i].bit;
        merge_held();
        return true;
    }
    if ((drag_i = key_at(x, y)) >= 0)
    {
        drag = KEY;
        mouse_keys |= 1u << keys[drag_i].k;
        merge_held();
        return true;
    }
    drag = NONE;
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
    if (sw_i >= 0 && sw_i != shell_)
    {
        set_shell(sw_i);
        shell_req = sw_i;
        return false;
    }
    int in_i = ink_at(x, y);
    if (in_i >= 0 && in_i != ink_now)
    {
        ink_now = ink_pending = ink_req = in_i;   // (ringed at once; the view follows)
    }
    for (size_t i = 0; i < files.size(); i++)
        if (x >= files[i].x && x < files[i].x + files[i].w && y >= files[i].y && y < files[i].y + files[i].h)
        {
            file_held = (int)i;
            file_req = files[i].action;
        }
    if (!native() && x >= usb.x && x < usb.x + usb.w && y >= usb.y && y < usb.y + usb.h)
    {
        file_held = 100;   // (pressed in, as OPEN and SAVE)
        fm1_req = fm1_on && fm1_mine ? 0 : 1;
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
    shell_hover = drag == NONE ? swatch_at(x, y) : -1;
    ink_hover = drag == NONE ? ink_at(x, y) : -1;
    int dx = x - last_x, dy = last_y - y;   // up or right: clockwise (the larger of the two)
    last_x = x;
    last_y = y;
    if (drag == KNOB)
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
    return false;
}

bool Panel::up(int, int)
{
    bool ch = false;
    if (drag == BUTTON) { mouse_buttons &= ~(1u << buttons[drag_i].bit); merge_held(); ch = true; }
    if (drag == KEY) { mouse_keys &= ~(1u << keys[drag_i].k); merge_held(); ch = true; }
    drag = NONE;
    file_held = bank_held = -1;
    return ch;
}

bool Panel::wheel(int x, int y, int notches)
{
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
const int MENU_X = 36, MENU_Y = 46, MENU_W = 300, MENU_ROWS = 6, ROW_H = 36, SUB_X = MENU_X + MENU_W + 8, SUB_ROW = 30,
          INK_COLS = 5, INK_S = 32, INK_GAP = 12, FM1_LINE = 22;
// SHELL, INK; MODE, SIZE; FM-1 USB and its state; ABOUT (a dotted line between each group)
enum { ROW_SHELL, ROW_INK, ROW_MODE, ROW_SIZE, ROW_FM1, ROW_ABOUT };
int row_y(int i)
{
    return MENU_Y + 10 + i * ROW_H + (i >= ROW_MODE ? 18 : 0) + (i >= ROW_FM1 ? 18 : 0) +
           (i >= ROW_ABOUT ? FM1_LINE + 18 : 0);
}
const int MENU_H = 2 + 8 + MENU_ROWS * ROW_H + 3 * 18 + FM1_LINE + 8 + 2;
// FM-1 USB's two words, right to left from the row's right edge: ON, then OFF
const int SEG_RIGHT = MENU_X + MENU_W - 16;
int seg_w(const char *t) { return Canvas::ptext_w(t, 2) + 12; }
int shell_sub_y() { return row_y(ROW_SHELL) - 10; }
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

void Panel::draw_native(uint32_t *px, const tess_view_t &v, double breath)
{
    Canvas c{px};
    static const double kPi = 3.14159265358979;
    for (int i = 0; i < W * H; i++)
        px[i] = T_BODY;

    // the mark, the name, the menu's arrow, the MIDI lamp; the version at the right
    for (int ty = 0; ty < 3; ty++)
        for (int tx = 0; tx < 3; tx++)
        {
            int x = 36 + tx * 9, y = 12 + ty * 9;
            c.box(x, y, 8, 8, T_YEL);
            if (!(ty == 0 && tx == 2)) c.box(x + 2, y + 2, 4, 4, T_BODY);
        }
    c.ptext(72, 14, "TESSERA", 3, T_YEL);
    int ax = 72 + Canvas::ptext_w("TESSERA", 3) + 10;
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
        std::string ver = "PLUGIN " + version;
        for (auto &ch : ver) ch = ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
        c.ptext(W - 36 - Canvas::ptext_w(ver.c_str(), 2), 18, ver.c_str(), 2, T_WORD);
    }

    // the screen: a dim hairline, a gap, the 240 x 240 at 1:1; under it the track and its bank
    nbox(c, SCREEN_X, SCREEN_Y, SCREEN_S, SCREEN_S, T_DIM, T_BODY);
    const int inset = SCREEN_S / 32;
    for (int y = 0; y < TESS_SCREEN; y++)
        for (int x = 0; x < TESS_SCREEN; x++)
        {
            uint16_t p = v.screen[y * TESS_SCREEN + x];
            uint32_t r = (p >> 11 & 31) * 255 / 31, g = (p >> 5 & 63) * 255 / 63, b = (p & 31) * 255 / 31;
            px[(SCREEN_Y + inset + y) * W + SCREEN_X + inset + x] = 0xFF000000u | r << 16 | g << 8 | b;
        }
    char cap[8];
    std::snprintf(cap, sizeof cap, "T%d", track_now + 1);
    c.ptext(SCREEN_X + 2, SCREEN_Y + SCREEN_S + 8, cap, 2, T_WORD);
    if (tracks_ > 4)
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
        else if (v.dim_buttons & bit) border = T_YEL, ink = T_YEL;
        int y = b.y + (down ? 2 : 0), h = b.h - (down ? 2 : 0), bottom = down ? 2 : 4;
        nbox(c, b.x, y, b.w, h, border, fill, bottom);
        c.ptext_c(b.x + b.w / 2, (y + 2 + y + h - bottom) / 2 - 7, b.face, 2, ink);
    }

    // the keys: naturals a dim outline, sharps dim and solid; lit yellow, held warm white
    dotted(c, KEYBED[0], KEYBED[0] + KEYBED[2], KEYBED[1], T_DIM);
    for (size_t i = 0; i < keys.size(); i++)
    {
        const Key &k = keys[i];
        uint32_t bit = 1u << k.k;
        bool natural = i < 16;
        uint32_t base = natural ? T_BLACK : T_DIM, border = T_DIM, fill = base;
        if (input.keys & bit) border = fill = T_HELD;
        else if (v.lit_keys & bit) border = fill = T_YEL;
        else if (v.breath_keys & bit) border = T_YEL, fill = mix(base, T_YEL, 0.6 * breath);
        else if (v.dim_keys & bit) border = T_YEL;
        nbox(c, k.cx - kw / 2, k.cy - kh / 2, kw, kh, border, fill);
    }

    // OPEN, SAVE and the bank switch, centred under the keys; what the last one did beside them
    for (size_t i = 0; i < files.size(); i++)
    {
        const FileButton &f = files[i];
        bool held = file_held == (int)i;
        nbox(c, f.x, f.y, f.w, f.h, held ? T_HELD : T_YEL, held ? T_HELD : T_BODY);
        c.ptext_c(f.x + f.w / 2, f.y + f.h / 2 - 7, f.face, 2, held ? T_BLACK : T_YEL);
    }
    if (tracks_ > 4)
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
        const FileButton &last = tracks_ > 4 ? banks[1] : files.back();
        c.ptext(last.x + last.w + 16, last.y + last.h / 2 - 7, note_text.c_str(), 2, T_WORD);
    }

    if (!menu_open)
        return;

    // the menu: SHELL, INK; MODE, SIZE; ABOUT
    nbox(c, MENU_X, MENU_Y, MENU_W, MENU_H, T_DIM, T_BLACK);
    static const char *NAMES[MENU_ROWS] = {"SHELL", "INK", "MODE", "SIZE", "FM-1 USB", "ABOUT"};
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
            else if (ink_now >= 0 && ink_now < (int)inks.size())
                c.box(right - 14, y + 11, 14, 14, 0xFF000000u | inks[ink_now].rgb[0]);
        }
        else if (r == ROW_MODE)
        {
            seg("EXPANDED", false, false);   // (not there yet)
            seg("FM-1", true, true);
        }
        else if (r == ROW_SIZE)
        {
            seg("200", false, false);
            seg("150", false, false);
            seg("100", true, true);
        }
        else if (r == ROW_FM1)   // OFF / ON, and under them what the FM-1 is doing
        {
            seg("ON", fm1_on, true);
            seg("OFF", !fm1_on, true);
            if (fm1_on)
                c.ptext(MENU_X + 16, y + ROW_H + 2, fm1_text.c_str(), 2, fm1_live ? T_YEL : T_WORD);
        }
        else
        {
            std::string ver = version.empty() ? "TESSERA" : version;
            for (auto &ch : ver) ch = ch >= 'a' && ch <= 'z' ? (char)(ch - 32) : ch;
            c.ptext(right - Canvas::ptext_w(ver.c_str(), 2), y + 11, ver.c_str(), 2, T_WORD);
        }
    }
    dotted(c, MENU_X + 16, MENU_X + MENU_W - 16, row_y(ROW_INK) + ROW_H + 8, T_DIM);
    dotted(c, MENU_X + 16, MENU_X + MENU_W - 16, row_y(ROW_SIZE) + ROW_H + 8, T_DIM);
    dotted(c, MENU_X + 16, MENU_X + MENU_W - 16, row_y(ROW_FM1) + ROW_H + FM1_LINE + 8, T_DIM);

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
    if (r == ROW_SHELL) menu_sub = menu_sub == 1 ? 0 : 1;
    else if (r == ROW_INK) menu_sub = menu_sub == 2 ? 0 : 2;
    else if (r < 0) menu_open = false;   // (MODE, SIZE, ABOUT: nothing to pick yet)
    sub_hover = -1;
    return false;
}

int Panel::fm1_seg_at(int x) const
{
    int on_l = SEG_RIGHT - seg_w("ON"), off_r = on_l - 4, off_l = off_r - seg_w("OFF");
    if (x >= on_l && x < SEG_RIGHT) return 1;
    if (x >= off_l && x < off_r) return 0;
    return -1;
}
