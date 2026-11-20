/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window's FM-1 (panel.h): web/sim/index.html's wide layout and black shell, drawn into a pixel buffer */
#include "panel.h"

#include <cmath>
#include <cstring>

extern "C" {
#include "mo_font.h"
}

namespace
{
// ---- colours (web/sim/index.html SHELLS black; the LED levels mixed as shellVars does, in linear light)
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
const uint32_t BODY = 0xFF1B1B1D, TRAY = 0xFF29292C, BTN = 0xFF3B3B40, EDGE = 0xFF505056, BTN_INK = 0xFFE4E4E6,
               SLIT = 0xFF121214, LABEL = 0xFFD6D6D8, LIT = 0xFFF6F6F6, LIT_INK = 0xFF1B1B1D, REC = 0xFFC62A22,
               WELL = 0xFF141415, PART = 0xFF2A2A2D, KEDGE = 0xFF3A3A3E, GREEN = 0xFF5FE08A, GREEN_INK = 0xFF0D2414,
               BEZEL = 0xFF0B0B0C;

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
    void line(double x0, double y0, double x1, double y1, double wd, uint32_t c)
    {
        double len = std::hypot(x1 - x0, y1 - y0);
        int n = (int)(len * 2) + 1;
        for (int i = 0; i <= n; i++)
            circle(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, wd / 2, c);
    }
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
    buttons.push_back({60, 258, 90, 34, B_OCTDN, "OCT-", nullptr});
    buttons.push_back({170, 258, 90, 34, B_OCTUP, "OCT+", nullptr});
    const int R1[6] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO}, R2[6] = {B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC};
    const char *F1[6] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO"}, *F2[6] = {"HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC"};
    for (int i = 0; i < 6; i++)
    {
        buttons.push_back({582 + i * 63, 174, 54, 48, R1[i], F1[i], nullptr});
        buttons.push_back({582 + i * 63, 174 + 62, 54, 48, R2[i], F2[i], R2[i] == B_PLAY ? "STOP" : nullptr});
    }
    const int x0 = KEYBED[0], y0 = KEYBED[1], w = KEYBED[2], h = KEYBED[3];
    pitch = (w - 24) / 16;
    kw = (int)(pitch * 0.78);
    kh = (int)(h * 0.425);
    sw = kw * 14 / 100 > 3 ? kw * 14 / 100 : 3;
    for (int i = 0; i < 16; i++)
        keys.push_back({x0 + 12 + (int)((i + 0.5) * pitch), y0 + h * 3 / 4, WHITE_K[i]});
    for (int j = 0; j < 11; j++)
        keys.push_back({x0 + 12 + (BLACK_AFTER[j] + 1) * pitch, y0 + h / 4, BLACK_K[j]});
    files.push_back({60, KEYBED[1] + KEYBED[3] + 5, 72, 24, FILE_OPEN, "OPEN"});   // (in the shell, under the keys)
    files.push_back({142, KEYBED[1] + KEYBED[3] + 5, 72, 24, FILE_SAVE, "SAVE"});
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
    master_now = (int)v.master;
    knobs[0].angle = -135 + 270.0 * (input.master >= 0 ? input.master : master_now) / 1023;
    static const double kPi = 3.14159265358979;
    // the breathing LEDs: dark .. lit and back in ~1.13 s (fm1_led_breath)
    double breath = 0.5 - 0.5 * std::cos(2 * kPi * (frame++ / 30.0) / 1.13);
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
    if (note_frames > 0)
    {
        note_frames--;
        const FileButton &last = files.back();
        int x = last.x + last.w + 16;
        c.text(x + Canvas::text_w(note_text.c_str(), 2) / 2, last.y + last.h / 2 - 7, note_text.c_str(), 2, LABEL);
    }
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
    if (code == 0x1A) b = B_OCTDN;
    if (code == 0x1B) b = B_OCTUP;
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
    input.changed = true;
}

bool Panel::down(int x, int y)
{
    last_x = x;
    last_y = y;
    acc = 0;
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
    for (size_t i = 0; i < files.size(); i++)
        if (x >= files[i].x && x < files[i].x + files[i].w && y >= files[i].y && y < files[i].y + files[i].h)
        {
            file_held = (int)i;
            file_req = files[i].action;
        }
    return false;
}

bool Panel::move(int x, int y)
{
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
    file_held = -1;
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
