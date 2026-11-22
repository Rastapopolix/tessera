/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The FM-1's ports on macOS (fm1_link.h): CoreMIDI. CoreMIDI lets every program use a port at once, so an FM-1 the DAW
 * also has open works here too (turn it off in the DAW so its keys don't also play the track the DAW routes them to).
 * The first source and destination whose display names match are taken. */
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>

#include <atomic>
#include <chrono>

#include "fm1_link.h"

namespace
{
std::string name_of(MIDIObjectRef o)
{
    CFStringRef cf = nullptr;
    if (MIDIObjectGetStringProperty(o, kMIDIPropertyDisplayName, &cf) != noErr || !cf)
        if (MIDIObjectGetStringProperty(o, kMIDIPropertyName, &cf) != noErr || !cf)
            return std::string();
    char out[256] = {};
    CFStringGetCString(cf, out, sizeof out, kCFStringEncodingUTF8);
    CFRelease(cf);
    return out;
}

struct MacPorts : Fm1Ports
{
    MIDIClientRef client = 0;
    MIDIPortRef inport = 0, outport = 0;
    MIDIEndpointRef src = 0, dst = 0;
    std::function<void(const uint8_t *, size_t)> cb;
    std::chrono::steady_clock::time_point checked{};
    bool gone = false;

    static void read(const MIDIPacketList *pl, void *ref, void *)
    {
        auto *self = static_cast<MacPorts *>(ref);
        const MIDIPacket *p = &pl->packet[0];
        for (UInt32 i = 0; i < pl->numPackets; i++)
        {
            self->cb(p->data, p->length);
            p = MIDIPacketNext(p);
        }
    }
    ~MacPorts() override
    {
        if (inport && src)
            MIDIPortDisconnectSource(inport, src);
        if (inport)
            MIDIPortDispose(inport);
        if (outport)
            MIDIPortDispose(outport);
        if (client)
            MIDIClientDispose(client);
    }
    bool send(const uint8_t *b, size_t n) override
    {
        Byte buf[512];
        auto *pl = reinterpret_cast<MIDIPacketList *>(buf);
        MIDIPacket *p = MIDIPacketListInit(pl);
        p = MIDIPacketListAdd(pl, sizeof buf, p, 0, n, b);
        return p && MIDISend(outport, dst, pl) == noErr;
    }
    bool alive() override   // (once a second: its source still listed)
    {
        auto now = std::chrono::steady_clock::now();
        if (!gone && now - checked >= std::chrono::seconds(1))
        {
            checked = now;
            bool seen = false;
            for (ItemCount i = 0, n = MIDIGetNumberOfSources(); i < n && !seen; i++)
                seen = MIDIGetSource(i) == src;
            SInt32 offline = 0;
            if (MIDIObjectGetIntegerProperty(src, kMIDIPropertyOffline, &offline) == noErr && offline)
                seen = false;
            gone = !seen;
        }
        return !gone;
    }
};
}   // namespace

std::unique_ptr<Fm1Ports> Fm1Ports::open(std::function<void(const uint8_t *, size_t)> in, bool &busy)
{
    busy = false;
    MIDIEndpointRef src = 0, dst = 0;
    for (ItemCount i = 0, n = MIDIGetNumberOfSources(); i < n && !src; i++)
        if (fm1_port_name(name_of(MIDIGetSource(i))))
            src = MIDIGetSource(i);
    for (ItemCount i = 0, n = MIDIGetNumberOfDestinations(); i < n && !dst; i++)
        if (fm1_port_name(name_of(MIDIGetDestination(i))))
            dst = MIDIGetDestination(i);
    if (!src || !dst)
        return nullptr;
    std::unique_ptr<MacPorts> p(new MacPorts);
    p->cb = std::move(in);
    p->src = src;
    p->dst = dst;
    if (MIDIClientCreate(CFSTR("Tessera"), nullptr, nullptr, &p->client) != noErr)
        return nullptr;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"   // (MIDIInputPortCreateWithProtocol: macOS 11 only)
    if (MIDIInputPortCreate(p->client, CFSTR("FM-1 in"), &MacPorts::read, p.get(), &p->inport) != noErr)
        return nullptr;
    if (MIDIOutputPortCreate(p->client, CFSTR("FM-1 out"), &p->outport) != noErr)
        return nullptr;
#pragma clang diagnostic pop
    if (MIDIPortConnectSource(p->inport, src, nullptr) != noErr)
        return nullptr;
    return p;
}
