/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin window on Windows (gui.h): a child window of the host's, a timer at ~30 Hz, the panel drawn into a DIB.
 * OPEN / SAVE under the keys use the system's file dialogs; a .tsp or a backup dropped on the window opens too. */
#include "gui.h"

#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>

#include <string>
#include <vector>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

namespace
{
const wchar_t *CLASS_NAME = L"TesseraPluginWindow";
extern "C" IMAGE_DOS_HEADER __ImageBase;   // this module (the class belongs to it, not to the host)

class GuiWin32 : public Gui
{
  public:
    explicit GuiWin32(GuiLink *l) : link(l)
    {
        panel.set_shell(l->shell());
        panel.set_keys(l->keys_on());
        panel.set_hover_help(l->hover_help_on());
        panel.set_inks(l->inks());
        panel.set_tracks(l->tracks());
        panel.set_version(tess_version());
        panel.set_expanded(l->expanded());
        pw = panel.width();
    }
    GuiLink *link;   // this window's instance
    HWND wnd = nullptr;
    Panel panel;
    int wheel_acc = 0, hwheel_acc = 0;   // (WM_MOUSEWHEEL's, WM_MOUSEHWHEEL's deltas not yet a notch)
    std::vector<uint32_t> px = std::vector<uint32_t>(Panel::W_MAX * Panel::H);
    int pct = 100;          // SIZE in % (75 to 200): the panel drawn at 1:1, stretched to the window
    int pw = Panel::W;      // the panel's width the window has (FM-1 or EXPANDED: panel.width())
    tess_view_t view{};
    std::unique_ptr<tess_tracks_t> tracks{new tess_tracks_t()};   // TRACKS (EXPANDED)
    int at(int v) const { return v * 100 / pct; }   // a window pixel at 1:1

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
        int x = g->at(GET_X_LPARAM(lp)), y = g->at(GET_Y_LPARAM(lp));
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
            g->panel.set_mods((wp & MK_CONTROL) != 0, (wp & MK_SHIFT) != 0, GetKeyState(VK_MENU) < 0);   // (Ctrl: type a value, Shift: fine)
            g->input(g->panel.down(x, y));
            return 0;
        case WM_RBUTTONDOWN:   // a button or key latched
            SetFocus(h);
            g->input(g->panel.latch(x, y));
            return 0;
        case WM_MOUSEMOVE:
            g->panel.set_mods((wp & MK_CONTROL) != 0, (wp & MK_SHIFT) != 0, GetKeyState(VK_MENU) < 0);
            g->input(g->panel.move(x, y));
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            g->input(g->panel.up(x, y));
            g->files();   // (OPEN or SAVE clicked: its dialog, once the button is up)
            return 0;
        case WM_XBUTTONDOWN:   // the mouse's back / forward buttons: BACK and OK
        case WM_XBUTTONUP:
            g->input(g->panel.side_button(GET_XBUTTON_WPARAM(wp) == XBUTTON2, msg == WM_XBUTTONDOWN));
            return TRUE;
        case WM_MOUSEHWHEEL:   // the wheel sideways: the next page (+ right)
        {
            g->hwheel_acc += GET_WHEEL_DELTA_WPARAM(wp);
            int n = g->hwheel_acc / WHEEL_DELTA;
            g->hwheel_acc -= n * WHEEL_DELTA;
            if (n)
                g->input(g->panel.hwheel(n));
            return 0;
        }
        case WM_DROPFILES:
        {
            HDROP d = reinterpret_cast<HDROP>(wp);
            UINT n = DragQueryFileW(d, 0, nullptr, 0);
            std::wstring w(n + 1, L'\0');
            DragQueryFileW(d, 0, &w[0], n + 1);
            w.resize(n);
            DragFinish(d);
            g->panel.note(g->link->open_file(utf8(w)));
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        case WM_CAPTURECHANGED:
            g->input(g->panel.up(0, 0));
            return 0;
        case WM_SYSKEYDOWN:   // Alt+Up / Alt+Down: TRACK's move (panel.h shortcut); any other, the host's
            if ((wp == VK_UP || wp == VK_DOWN) && GetKeyState(VK_CONTROL) >= 0 && g->panel.shortcut(wp == VK_UP ? '^' : '_'))
                return 0;
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            // the computer keyboard (panel.h key): by scan code, as the web simulator does; Ctrl / Alt are the host's
            if (msg == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) >= 0 && (wp == 'O' || wp == 'S') &&
                g->panel.shortcut((char)wp))   // (the standalone app: Ctrl+O / Ctrl+S, OPEN / SAVE)
            {
                g->files();
                return 0;
            }
            if (msg == WM_KEYDOWN && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) >= 0 && (wp == 'C' || wp == 'V') &&
                g->panel.shortcut(wp == 'V' && GetKeyState(VK_SHIFT) < 0 ? 'v' : (char)wp))
                return 0;   // (TRACK: Ctrl+C copies, Ctrl+V pastes, Ctrl+Shift+V the sound alone)
            if (GetKeyState(VK_CONTROL) < 0 || GetKeyState(VK_MENU) < 0)
                break;
            int sc = (int)(lp >> 16 & 0xFF), code = sc;
            if (msg == WM_KEYDOWN && g->panel.typing())   // NAME open: the character the layout gives, typed
            {
                uint32_t ch = wp == VK_BACK ? TESS_TEXT_BACK : wp == VK_RETURN ? TESS_TEXT_OK : wp == VK_ESCAPE ? TESS_TEXT_CANCEL
                            : wp == VK_LEFT ? TESS_TEXT_LEFT : wp == VK_RIGHT ? TESS_TEXT_RIGHT : wp == VK_DELETE ? TESS_TEXT_DELETE : 0;
                if (!ch)
                {
                    BYTE state[256];
                    WCHAR w[4];
                    if (GetKeyboardState(state) && ToUnicode((UINT)wp, (UINT)sc, state, w, 4, 4) == 1 && w[0] >= 32)
                        ch = w[0];   // (flag 4: the keyboard's dead-key state left alone)
                }
                if (ch && g->panel.text(ch))
                {
                    g->input(true);
                    return 0;
                }
            }
            if (wp == VK_UP) code = Panel::KEY_UP;
            else if (wp == VK_DOWN) code = Panel::KEY_DOWN;
            else if (wp == VK_LEFT) code = Panel::KEY_LEFT;
            else if (wp == VK_RIGHT) code = Panel::KEY_RIGHT;
            else if (wp == VK_PRIOR && (lp >> 24 & 1)) code = Panel::KEY_PGUP;   // (not the keypad's 9 / 3)
            else if (wp == VK_NEXT && (lp >> 24 & 1)) code = Panel::KEY_PGDN;
            else if (lp >> 24 & 1) break;   // (another extended key: not a panel key)
            if (g->panel.key(code, msg == WM_KEYDOWN))
            {
                g->input(true);
                return 0;
            }
            break;
        }
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS | DLGC_WANTARROWS;
        case WM_KILLFOCUS:
            g->input(g->panel.release_keys());
            return 0;
        case WM_MOUSEWHEEL:
        {
            POINT p{x, y};   // (screen coordinates)
            ScreenToClient(h, &p);
            p.x = g->at(p.x), p.y = g->at(p.y);
            g->wheel_acc += GET_WHEEL_DELTA_WPARAM(wp);   // (a touchpad's small deltas gathered into notches)
            int n = g->wheel_acc / WHEEL_DELTA;
            g->wheel_acc -= n * WHEEL_DELTA;
            g->panel.set_mods((GET_KEYSTATE_WPARAM(wp) & MK_CONTROL) != 0, (GET_KEYSTATE_WPARAM(wp) & MK_SHIFT) != 0,
                              GetKeyState(VK_MENU) < 0);
            if (n)
                g->input(g->panel.wheel(p.x, p.y, n));
            return 0;
        }
        }
        return DefWindowProcW(h, msg, wp, lp);
    }

    static std::string utf8(const std::wstring &w)
    {
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string s(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
        return s;
    }
    static std::wstring wide(const std::string &s)
    {
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
        return w;
    }

    // OPEN or SAVE, if one was clicked: the system's dialog, then the instance opens or saves
    void files()
    {
        int kr = panel.take_keys_request();    // (KEYS ON or OFF: every window's)
        if (kr >= 0)
            link->keys_set(kr == 1);
        int hr = panel.take_hover_request();   // (HOVER HELP ON or OFF: every window's)
        if (hr >= 0)
            link->hover_help_set(hr == 1);
        int sh = panel.take_shell_request();   // (a shell picked: its ink)
        if (sh >= 0)
            link->shell_picked(sh);
        int ink = panel.take_ink_request();    // (an ink picked)
        if (ink >= 0)
            link->ink_picked(ink);
        int req = panel.take_file_request();
        if (req == Panel::FILE_NONE)
            return;
        if (req == Panel::FILE_MIDI)   // (no dialog: the clip goes to Documents/Tessera/MIDI)
        {
            panel.note(link->export_midi());
            return;
        }
        wchar_t path[MAX_PATH * 4] = {};
        OPENFILENAMEW of{};
        of.lStructSize = sizeof of;
        of.hwndOwner = GetAncestor(wnd, GA_ROOT);
        of.lpstrFile = path;
        of.nMaxFile = (DWORD)(sizeof path / sizeof path[0]);
        std::wstring presets = wide(projfile::presets_dir());   // (the DAW's preset browser lists what is saved there)
        if (!presets.empty())
            of.lpstrInitialDir = presets.c_str();
        if (req == Panel::FILE_OPEN)
        {
            of.lpstrFilter = L"Tessera project or FM-1 backup (*.tsp, *.json)\0*.tsp;*.json\0All files\0*.*\0";
            of.lpstrTitle = L"Open a Tessera project";
            of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetOpenFileNameW(&of))
                panel.note(link->open_file(utf8(path)));
        }
        else
        {
            std::wstring name = wide(link->file_name());
            wcsncpy(path, name.c_str(), sizeof path / sizeof path[0] - 1);
            of.lpstrFilter = L"Tessera project (*.tsp)\0*.tsp\0";
            of.lpstrDefExt = L"tsp";
            of.lpstrTitle = L"Save the Tessera project";
            of.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetSaveFileNameW(&of))
                panel.note(link->save_file(utf8(path)));
        }
        InvalidateRect(wnd, nullptr, FALSE);
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
        bool t2 = panel.expanded() && link->tracks_view(*tracks);
        panel.draw(px.data(), view, t2 ? tracks.get() : nullptr);
        BITMAPINFO bi{};
        bi.bmiHeader.biSize = sizeof bi.bmiHeader;
        bi.bmiHeader.biWidth = pw;
        bi.bmiHeader.biHeight = -Panel::H;   // top row first
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        // 100 %: as it is; 200 %: each pixel doubled; any other: GDI's HALFTONE, each pixel the mean of what it covers
        bool soft = pct % 100 != 0;
        SetStretchBltMode(dc, soft ? HALFTONE : COLORONCOLOR);
        if (soft) SetBrushOrgEx(dc, 0, 0, nullptr);
        StretchDIBits(dc, 0, 0, pw * pct / 100, Panel::H * pct / 100, 0, 0, pw, Panel::H, px.data(), &bi,
                      DIB_RGB_COLORS, SRCCOPY);
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
        wnd = CreateWindowExW(0, CLASS_NAME, L"Cosmati", WS_CHILD | WS_CLIPSIBLINGS, 0, 0, pw * pct / 100,
                              Panel::H * pct / 100,
                              static_cast<HWND>(w->win32), nullptr, inst, nullptr);
        if (!wnd)
            return false;
        SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        DragAcceptFiles(wnd, TRUE);
        SetTimer(wnd, 1, 33, nullptr);
        return true;
    }
    void set_scale(int n) override
    {
        if (n < Panel::PCT_MIN || n > Panel::PCT_MAX)
            return;
        pct = n;
        resize();
    }
    // the window at the panel's width (FM-1 or EXPANDED) and SIZE
    void resize()
    {
        panel.set_expanded(link->expanded());
        pw = panel.width();
        if (wnd)
        {
            SetWindowPos(wnd, nullptr, 0, 0, pw * pct / 100, Panel::H * pct / 100,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(wnd, nullptr, FALSE);
        }
    }
    bool show() override { return wnd && (ShowWindow(wnd, SW_SHOW), true); }
    bool hide() override { return wnd && (ShowWindow(wnd, SW_HIDE), true); }
    // the screen's ink changed since the last frame (not at the window's first): the next window's
    int ink_last = -1;
    void ink_check()
    {
        if (ink_last >= 0 && (int)view.ink != ink_last)
            link->ink_seen((int)view.ink);
        ink_last = (int)view.ink;
    }
    void tick() override
    {
        link->view(view);
        panel.set_midi(link->midi_count());
        panel.set_standalone(link->standalone());
        gui_fm1_tick(panel, link);
        gui_size_tick(panel, link);
        if (panel.width() != pw)   // (MODE, or a shell picked: the window follows)
            resize();
        gui_listen_tick(panel, link);
        gui_gen_tick(panel, link);
        ink_check();
        InvalidateRect(wnd, nullptr, FALSE);
    }
};
}   // namespace

const char *Gui::api() { return CLAP_WINDOW_API_WIN32; }
int Gui::screen_width()
{
    RECT r{};   // (the primary screen's work area: without the taskbar)
    if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &r, 0)) return (int)(r.right - r.left);
    return GetSystemMetrics(SM_CXSCREEN);
}
std::unique_ptr<Gui> Gui::make(const clap_host_t *, GuiLink *link) { return std::make_unique<GuiWin32>(link); }
