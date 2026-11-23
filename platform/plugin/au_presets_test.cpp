/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's presets in an Audio Unit host's browser (macOS), over the installed Smalti.component (aumu Tsra Tess:
 * CI copies it to ~/Library/Audio/Plug-Ins/Components first). clap-wrapper publishes the CLAP preset discovery
 * (presets.cpp) as the AU's factory presets:
 *   - once the unit is initialised and the crawl is done, the factory presets hold the eight demos;
 *   - choosing one (kAudioUnitProperty_PresentPreset) loads it: two different demos leave the parameters different,
 *     the same demo twice leaves them the same.
 * usage: tessera-au-presets-test */
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <cstdio>
#include <string>
#include <vector>

static int fails = 0;
#define CHECK(c, ...)                          \
    do                                         \
    {                                          \
        if (!(c))                              \
        {                                      \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                 \
            fails++;                           \
        }                                      \
    } while (0)

static std::string str(CFStringRef s)
{
    char buf[256] = "";
    if (s) CFStringGetCString(s, buf, sizeof buf, kCFStringEncodingUTF8);
    return buf;
}

int main()
{
    AudioComponentDescription desc{kAudioUnitType_MusicDevice, 'Tsra', 'Tess', 0, 0};
    AudioComponent c = AudioComponentFindNext(nullptr, &desc);
    CHECK(c, "Smalti.component found (aumu Tsra Tess)");
    if (!c) return 1;
    AudioUnit au = nullptr;
    CHECK(AudioComponentInstanceNew(c, &au) == noErr && au, "an instance");
    if (!au) return 1;
    CHECK(AudioUnitInitialize(au) == noErr, "initialised");

    // the factory presets: up to 5 s for the crawl (the wrapper's idle runs on the main run loop)
    std::vector<std::pair<int, std::string>> presets;
    for (int t = 0; t < 100 && presets.size() < 8; t++)
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.05, false);
        CFArrayRef list = nullptr;
        UInt32 size = sizeof list;
        presets.clear();
        if (AudioUnitGetProperty(au, kAudioUnitProperty_FactoryPresets, kAudioUnitScope_Global, 0, &list, &size) ==
                noErr &&
            list)
        {
            for (CFIndex i = 0; i < CFArrayGetCount(list); i++)
            {
                auto *p = (const AUPreset *)CFArrayGetValueAtIndex(list, i);
                presets.push_back({(int)p->presetNumber, str(p->presetName)});
            }
            CFRelease(list);
        }
    }
    const char *demos[] = {"CEDAR RAIN", "EMBER FIELD", "KOTO GARDEN", "LOW HARBOUR",
                           "PAPER KITES", "SLOW LIGHT", "STILL TIDE", "UNDERTOW"};
    CHECK(presets.size() >= 8, "the eight demos listed (%d)", (int)presets.size());
    for (const char *d : demos)
    {
        bool in = false;
        for (auto &p : presets) in |= p.second == d;
        CHECK(in, "%s listed", d);
    }

    // choosing a preset loads it
    UInt32 size = 0;
    AudioUnitGetPropertyInfo(au, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, &size, nullptr);
    std::vector<AudioUnitParameterID> ids(size / sizeof(AudioUnitParameterID));
    if (size) AudioUnitGetProperty(au, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0, ids.data(), &size);
    auto params = [&]() {
        std::vector<AudioUnitParameterValue> v;
        for (auto id : ids)
        {
            AudioUnitParameterValue x = 0;
            AudioUnitGetParameter(au, id, kAudioUnitScope_Global, 0, &x);
            v.push_back(x);
        }
        return v;
    };
    auto choose = [&](const char *name) {
        for (auto &p : presets)
            if (p.second == name)
            {
                AUPreset pr{p.first, CFStringCreateWithCString(nullptr, name, kCFStringEncodingUTF8)};
                OSStatus e = AudioUnitSetProperty(au, kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0,
                                                  &pr, sizeof pr);
                CHECK(e == noErr, "%s chosen (%d)", name, (int)e);
                CFRelease(pr.presetName);
                CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, false);   // (the load: on the wrapper's idle)
                return;
            }
    };
    CHECK(!ids.empty(), "the parameters (%d)", (int)ids.size());
    choose("EMBER FIELD");
    auto a = params();
    choose("UNDERTOW");
    auto b = params();
    choose("EMBER FIELD");
    auto a2 = params();
    int differ = 0, again = 0;
    for (size_t i = 0; i < a.size() && i < b.size() && i < a2.size(); i++)
    {
        differ += a[i] != b[i];
        again += a[i] != a2[i];
    }
    CHECK(differ > 0, "EMBER FIELD then UNDERTOW: the parameters change");
    CHECK(again == 0, "EMBER FIELD again: the same parameters as the first time (%d differ)", again);

    AudioUnitUninitialize(au);
    AudioComponentInstanceDispose(au);
    std::printf("%s: %d factory presets, %d failure(s)\n", fails ? "FAIL" : "ok", (int)presets.size(), fails);
    return fails ? 1 : 0;
}
