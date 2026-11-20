/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window on Windows (gui.h): a child window of the host's, a timer at ~30 Hz, the panel drawn into a DIB */
#include "gui.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include <vector>

namespace
{
const wchar_t *CLASS_NAME = L"TesseraPluginWindow";
extern "C" IMAGE_DOS_HEADER __ImageBase;   // this module (the class belongs to it, not to the host)

class GuiWin32 : public Gui
{
  public:
    explicit GuiWin32(GuiLink *l) : link(l) {}
    GuiLink *link;   // this window's instance
    HWND wnd = nullptr;
    Panel panel;
    std::vector<uint32_t> px = std::vector<uint32_t>(Panel::W * Panel::H);
    tess_view_t view{};

    ~GuiWin32() override
    {
        if (wnd)
        {
            KillTimer(wnd, 1);
            DestroyWindow(wnd);
        }
    }

    static LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        auto *g = reinterpret_cast<GuiWin32 *>(GetWindowLongPtrW(h, GWLP_USERDATA));
        if (!g)
            return DefWindowProcW(h, msg, wp, lp);
        int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        switch (msg)
        {
        case WM_TIMER:
            g->tick();
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            g->paint(dc);
            EndPaint(h, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_LBUTTONDOWN:
            SetCapture(h);
            SetFocus(h);
            g->input(g->panel.down(x, y));
            return 0;
        case WM_MOUSEMOVE:
            g->input(g->panel.move(x, y));
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            g->input(g->panel.up(x, y));
            return 0;
        case WM_CAPTURECHANGED:
            g->input(g->panel.up(0, 0));
            return 0;
        case WM_MOUSEWHEEL:
        {
            POINT p{x, y};   // (screen coordinates)
            ScreenToClient(h, &p);
            g->input(g->panel.wheel(p.x, p.y, GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA));
            return 0;
        }
        }
        return DefWindowProcW(h, msg, wp, lp);
    }

    void input(bool changed)
    {
        if (changed)
        {
            link->input(panel.input);
            InvalidateRect(wnd, nullptr, FALSE);
        }
    }

    void paint(HDC dc)
    {
        panel.draw(px.data(), view);
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof bi.bmiHeader;
        bi.bmiHeader.biWidth = Panel::W;
        bi.bmiHeader.biHeight = -Panel::H;   // top row first
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        SetDIBitsToDevice(dc, 0, 0, Panel::W, Panel::H, 0, 0, 0, Panel::H, px.data(), &bi, DIB_RGB_COLORS);
    }

    bool set_parent(const clap_window_t *w) override
    {
        HINSTANCE inst = reinterpret_cast<HINSTANCE>(&__ImageBase);
        WNDCLASSEXW wc{};
        if (!GetClassInfoExW(inst, CLASS_NAME, &wc))
        {
            wc = {};
            wc.cbSize = sizeof wc;
            wc.lpfnWndProc = proc;
            wc.hInstance = inst;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.lpszClassName = CLASS_NAME;
            RegisterClassExW(&wc);
        }
        wnd = CreateWindowExW(0, CLASS_NAME, L"Tessera", WS_CHILD | WS_CLIPSIBLINGS, 0, 0, Panel::W, Panel::H,
                              static_cast<HWND>(w->win32), nullptr, inst, nullptr);
        if (!wnd)
            return false;
        SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetTimer(wnd, 1, 33, nullptr);
        return true;
    }
    bool show() override { return wnd && (ShowWindow(wnd, SW_SHOW), true); }
    bool hide() override { return wnd && (ShowWindow(wnd, SW_HIDE), true); }
    void tick() override
    {
        link->view(view);
        InvalidateRect(wnd, nullptr, FALSE);
    }
};
}   // namespace

const char *Gui::api() { return CLAP_WINDOW_API_WIN32; }
std::unique_ptr<Gui> Gui::make(const clap_host_t *, GuiLink *link) { return std::make_unique<GuiWin32>(link); }
