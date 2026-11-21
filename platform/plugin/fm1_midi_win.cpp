/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The FM-1's ports on Windows (fm1_link.h): WinMM. One program holds a port at a time, so an FM-1 the DAW (or the
 * editor's browser) has open answers busy: turn it off in the DAW's MIDI settings. Windows can list one FM-1 twice
 * (PR #738): the first input and the first output that match are taken, nothing more. Channel messages come through
 * the callback; SysEx through buffers the link's thread hands back (poll: WinMM's functions must not be called from
 * the callback). */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

#include <chrono>
#include <thread>

#include "fm1_link.h"

namespace
{
std::string utf8(const wchar_t *w)
{
    char out[256];
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, sizeof out, nullptr, nullptr);
    return n > 0 ? std::string(out) : std::string();
}

std::string in_name(UINT i)
{
    MIDIINCAPSW c{};
    return midiInGetDevCapsW(i, &c, sizeof c) == MMSYSERR_NOERROR ? utf8(c.szPname) : std::string();
}

std::string out_name(UINT i)
{
    MIDIOUTCAPSW c{};
    return midiOutGetDevCapsW(i, &c, sizeof c) == MMSYSERR_NOERROR ? utf8(c.szPname) : std::string();
}

struct WinPorts : Fm1Ports
{
    static constexpr int NBUF = 4, BUF = 256;
    HMIDIIN hin = nullptr;
    HMIDIOUT hout = nullptr;
    UINT in_id = 0;
    std::string name;
    std::function<void(const uint8_t *, size_t)> cb;
    MIDIHDR hdr[NBUF] = {};
    char buf[NBUF][BUF];
    bool started = false, gone = false;
    std::chrono::steady_clock::time_point checked{};

    static void CALLBACK proc(HMIDIIN, UINT msg, DWORD_PTR inst, DWORD_PTR p1, DWORD_PTR)
    {
        auto *self = reinterpret_cast<WinPorts *>(inst);
        if (msg != MIM_DATA)
            return;
        uint8_t b[3] = {(uint8_t)(p1 & 0xFF), (uint8_t)((p1 >> 8) & 0xFF), (uint8_t)((p1 >> 16) & 0xFF)};
        uint8_t t = b[0] & 0xF0;
        size_t n = b[0] >= 0xF8 ? 1 : t == 0xC0 || t == 0xD0 ? 2 : 3;
        self->cb(b, n);
    }
    void queue(int i)
    {
        MIDIHDR &h = hdr[i];
        h = MIDIHDR{};
        h.lpData = buf[i];
        h.dwBufferLength = BUF;
        if (midiInPrepareHeader(hin, &h, sizeof h) == MMSYSERR_NOERROR)
            midiInAddBuffer(hin, &h, sizeof h);
    }
    ~WinPorts() override
    {
        if (hin)
        {
            midiInStop(hin);
            midiInReset(hin);   // (the buffers back, marked done)
            for (auto &h : hdr)
                if (h.lpData)
                    midiInUnprepareHeader(hin, &h, sizeof h);
            midiInClose(hin);
        }
        if (hout)   // (no midiOutReset: its notes off on every channel would reach the FM-1 too)
            midiOutClose(hout);
    }
    bool send(const uint8_t *b, size_t n) override
    {
        MIDIHDR h{};
        h.lpData = (LPSTR)b;
        h.dwBufferLength = (DWORD)n;
        if (midiOutPrepareHeader(hout, &h, sizeof h) != MMSYSERR_NOERROR)
            return false;
        bool ok = midiOutLongMsg(hout, &h, sizeof h) == MMSYSERR_NOERROR;
        for (int i = 0; ok && !(h.dwFlags & MHDR_DONE) && i < 500; i++)
            Sleep(1);
        while (midiOutUnprepareHeader(hout, &h, sizeof h) == MIDIERR_STILLPLAYING)
            Sleep(1);
        if (!ok)
            gone = true;
        return ok;
    }
    void poll() override
    {
        for (int i = 0; i < NBUF; i++)
            if (hdr[i].dwFlags & MHDR_DONE)
            {
                if (hdr[i].dwBytesRecorded)
                    cb((const uint8_t *)buf[i], hdr[i].dwBytesRecorded);
                midiInUnprepareHeader(hin, &hdr[i], sizeof hdr[i]);
                queue(i);
            }
    }
    bool alive() override   // (once a second: still listed under its id, with its name)
    {
        auto now = std::chrono::steady_clock::now();
        if (!gone && now - checked >= std::chrono::seconds(1))
        {
            checked = now;
            gone = in_id >= midiInGetNumDevs() || in_name(in_id) != name;
        }
        return !gone;
    }
};
}   // namespace

std::unique_ptr<Fm1Ports> Fm1Ports::open(std::function<void(const uint8_t *, size_t)> in, bool &busy)
{
    busy = false;
    std::unique_ptr<WinPorts> p(new WinPorts);
    p->cb = std::move(in);
    UINT ni = midiInGetNumDevs(), no = midiOutGetNumDevs();
    for (UINT i = 0; i < ni && !p->hin; i++)
    {
        std::string n = in_name(i);
        if (!fm1_port_name(n))
            continue;
        HMIDIIN h = nullptr;
        MMRESULT r = midiInOpen(&h, i, (DWORD_PTR)&WinPorts::proc, (DWORD_PTR)p.get(), CALLBACK_FUNCTION);
        if (r == MMSYSERR_ALLOCATED)
            busy = true;
        if (r != MMSYSERR_NOERROR)
            continue;
        p->hin = h;
        p->in_id = i;
        p->name = n;
    }
    for (UINT i = 0; i < no && p->hin && !p->hout; i++)
    {
        if (!fm1_port_name(out_name(i)))
            continue;
        HMIDIOUT h = nullptr;
        MMRESULT r = midiOutOpen(&h, i, 0, 0, CALLBACK_NULL);
        if (r == MMSYSERR_ALLOCATED)
            busy = true;
        if (r == MMSYSERR_NOERROR)
            p->hout = h;
    }
    if (!p->hin || !p->hout)
        return nullptr;   // (the destructor closes what opened)
    busy = false;
    for (int i = 0; i < WinPorts::NBUF; i++)
        p->queue(i);
    midiInStart(p->hin);
    p->started = true;
    return p;
}
