/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Tessera's presets in a VST3 host's browser, as a minimal VST3 host over the built Smalti.vst3 (Linux), with HOME
 * in a folder of its own. clap-wrapper publishes the CLAP preset discovery (presets.cpp) as a program list on the
 * root unit, with a program-change parameter that loads one:
 *   - the root unit has a program list; once the plugin is active and the crawl is done it holds the eight demos and
 *     the .tsp put in the user's folder Documents/Tessera/Presets (the host is told the list changed);
 *   - its parameter is a program-change list with a step per preset;
 *   - choosing a preset through that parameter loads it: two different demos leave the parameters different, the
 *     same demo twice leaves them the same.
 * usage: tessera-vst3-presets-test path/to/Smalti.vst3 path/to/a.tsp */
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstunits.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

// the interface IDs this host asks for (the SDK's base library, which defines them, would also bring a plugin's entry)
DEF_CLASS_IID(FUnknown)
DEF_CLASS_IID(IPluginBase)
DEF_CLASS_IID(IPluginFactory)
DEF_CLASS_IID(IComponent)
DEF_CLASS_IID(IAudioProcessor)
DEF_CLASS_IID(IEditController)
DEF_CLASS_IID(IUnitInfo)
DEF_CLASS_IID(IHostApplication)
DEF_CLASS_IID(IComponentHandler)
DEF_CLASS_IID(IUnitHandler)
DEF_CLASS_IID(IParamValueQueue)
DEF_CLASS_IID(IParameterChanges)

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

static std::string utf8(const String128 s)
{
    std::string o;
    for (int i = 0; i < 128 && s[i]; i++) o += (char)s[i];   // (the names are ASCII)
    return o;
}

struct Host : U::Implements<U::Directly<IHostApplication>>
{
    tresult PLUGIN_API getName(String128 n) override
    {
        const char *h = "tessera-vst3-presets-test";
        int i = 0;
        for (; h[i]; i++) n[i] = h[i];
        n[i] = 0;
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID, TUID, void **o) override { *o = nullptr; return kNotImplemented; }
};

static int list_changes = 0;
struct Handler : U::Implements<U::Directly<IComponentHandler, IUnitHandler>>
{
    tresult PLUGIN_API beginEdit(ParamID) override { return kResultOk; }
    tresult PLUGIN_API performEdit(ParamID, ParamValue) override { return kResultOk; }
    tresult PLUGIN_API endEdit(ParamID) override { return kResultOk; }
    tresult PLUGIN_API restartComponent(int32) override { return kResultOk; }
    tresult PLUGIN_API notifyUnitSelection(UnitID) override { return kResultOk; }
    tresult PLUGIN_API notifyProgramListChange(ProgramListID, int32) override { list_changes++; return kResultOk; }
};

// one parameter's one point, at the block's start
struct Queue : U::Implements<U::Directly<IParamValueQueue>>
{
    ParamID id = 0;
    ParamValue v = 0;
    ParamID PLUGIN_API getParameterId() override { return id; }
    int32 PLUGIN_API getPointCount() override { return 1; }
    tresult PLUGIN_API getPoint(int32, int32 &offset, ParamValue &value) override
    {
        offset = 0;
        value = v;
        return kResultOk;
    }
    tresult PLUGIN_API addPoint(int32, ParamValue, int32 &) override { return kResultFalse; }
};
struct Changes : U::Implements<U::Directly<IParameterChanges>>
{
    Queue *q = nullptr;
    int32 PLUGIN_API getParameterCount() override { return q ? 1 : 0; }
    IParamValueQueue *PLUGIN_API getParameterData(int32) override { return q; }
    IParamValueQueue *PLUGIN_API addParameterData(const ParamID &, int32 &) override { return nullptr; }
};

static void copy_file(const char *from, const std::string &to)
{
    FILE *a = std::fopen(from, "rb"), *b = std::fopen(to.c_str(), "wb");
    CHECK(a && b, "copy %s to %s", from, to.c_str());
    if (a && b)
    {
        char buf[4096];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, a)) > 0) std::fwrite(buf, 1, n, b);
    }
    if (a) std::fclose(a);
    if (b) std::fclose(b);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: tessera-vst3-presets-test path/to/Smalti.vst3 path/to/a.tsp\n");
        return 2;
    }
    char tmpl[] = "/tmp/tessera-vst3-presets-XXXXXX";
    const char *home = mkdtemp(tmpl);
    CHECK(home, "a home of its own");
    if (!home) return 1;
    setenv("HOME", home, 1);
    unsetenv("XDG_CONFIG_HOME");
    std::string user = std::string(home) + "/Documents";
    mkdir(user.c_str(), 0755);
    user += "/Tessera";
    mkdir(user.c_str(), 0755);
    user += "/Presets";
    mkdir(user.c_str(), 0755);
    copy_file(argv[2], user + "/mine.tsp");

    std::string so = std::string(argv[1]) + "/Contents/x86_64-linux/Tessera.so";
    void *lib = dlopen(so.c_str(), RTLD_NOW | RTLD_LOCAL);
    CHECK(lib, "dlopen %s: %s", so.c_str(), dlerror());
    if (!lib) return 1;
    if (auto module_entry = (bool (*)(void *))dlsym(lib, "ModuleEntry")) module_entry(lib);
    auto get_factory = (IPluginFactory * (*)()) dlsym(lib, "GetPluginFactory");
    IPluginFactory *factory = get_factory ? get_factory() : nullptr;
    CHECK(factory, "GetPluginFactory");
    if (!factory) return 1;
    TUID cid = {};
    bool found = false;
    for (int32 i = 0; i < factory->countClasses() && !found; i++)
    {
        PClassInfo ci;
        factory->getClassInfo(i, &ci);
        if (!std::strcmp(ci.category, kVstAudioEffectClass)) { std::memcpy(cid, ci.cid, sizeof cid); found = true; }
    }
    CHECK(found, "an audio module class");
    if (!found) return 1;

    Host *host = new Host;
    Handler *handler = new Handler;
    IComponent *comp = nullptr;
    CHECK(factory->createInstance(cid, IComponent::iid, (void **)&comp) == kResultOk && comp, "the component made");
    if (!comp) return 1;
    comp->initialize(host);
    IEditController *ec = nullptr;
    if (comp->queryInterface(IEditController::iid, (void **)&ec) != kResultOk || !ec)   // (a separate controller)
    {
        TUID ccid;
        comp->getControllerClassId(ccid);
        factory->createInstance(ccid, IEditController::iid, (void **)&ec);
        if (ec) ec->initialize(host);
    }
    CHECK(ec, "the controller");
    if (!ec) return 1;
    ec->setComponentHandler(handler);
    IUnitInfo *units = nullptr;
    CHECK(ec->queryInterface(IUnitInfo::iid, (void **)&units) == kResultOk && units, "IUnitInfo");
    IAudioProcessor *ap = nullptr;
    CHECK(comp->queryInterface(IAudioProcessor::iid, (void **)&ap) == kResultOk && ap, "IAudioProcessor");
    if (!units || !ap) return 1;

    // active and processing, as in a DAW: the wrapper's idle (which publishes the finished crawl) runs from then on
    ProcessSetup setup{kRealtime, kSample32, 512, 48000.0};
    ap->setupProcessing(setup);
    comp->setActive(true);
    ap->setProcessing(true);

    UnitInfo root{};
    for (int32 i = 0; i < units->getUnitCount(); i++)
    {
        UnitInfo u;
        units->getUnitInfo(i, u);
        if (u.id == kRootUnitId) root = u;
    }
    CHECK(root.programListId != kNoProgramListId, "the root unit has a program list");
    ProgramListInfo list{};
    for (int t = 0; t < 100; t++)   // (up to 5 s for the crawl and the idle that publishes it)
    {
        for (int32 i = 0; i < units->getProgramListCount(); i++)
        {
            ProgramListInfo pi;
            units->getProgramListInfo(i, pi);
            if (pi.id == root.programListId) list = pi;
        }
        if (list.programCount > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::vector<std::string> names;
    for (int32 p = 0; p < list.programCount; p++)
    {
        String128 n;
        units->getProgramName(list.id, p, n);
        names.push_back(utf8(n));
    }
    const char *demos[] = {"CEDAR RAIN", "EMBER FIELD", "KOTO GARDEN", "LOW HARBOUR",
                           "PAPER KITES", "SLOW LIGHT", "STILL TIDE", "UNDERTOW"};
    CHECK(list.programCount == 9, "eight demos and the user's preset listed (%d)", list.programCount);
    for (const char *d : demos)
    {
        int n = 0;
        for (auto &s : names) n += s == d;
        CHECK(n >= 1, "%s listed", d);
    }
    CHECK(list_changes >= 1, "the host told the list changed");

    // the list's parameter: a program change, a step per preset
    ParameterInfo sel{};
    bool have_sel = false;
    const int32 np = ec->getParameterCount();
    for (int32 i = 0; i < np; i++)
    {
        ParameterInfo pi;
        ec->getParameterInfo(i, pi);
        if (pi.id == (ParamID)list.id) { sel = pi; have_sel = true; }
    }
    CHECK(have_sel && (sel.flags & ParameterInfo::kIsProgramChange) && sel.stepCount == list.programCount - 1,
          "the list's program-change parameter (flags %d, %d steps)", sel.flags, sel.stepCount);

    // choosing a preset loads it
    auto params = [&]() {
        std::vector<double> v;
        for (int32 i = 0; i < np; i++)
        {
            ParameterInfo pi;
            ec->getParameterInfo(i, pi);
            if (pi.id != sel.id) v.push_back(ec->getParamNormalized(pi.id));
        }
        return v;
    };
    std::vector<float> silence(512);
    float *ch[16];
    for (auto &c : ch) c = silence.data();
    auto choose = [&](int index) {
        Queue *q = new Queue;
        q->id = sel.id;
        q->v = sel.stepCount ? (double)index / sel.stepCount : 0;
        Changes *changes = new Changes;
        changes->q = q;
        AudioBusBuffers out{};
        out.numChannels = 2;
        out.channelBuffers32 = ch;
        ProcessData d{};
        d.processMode = kRealtime;
        d.symbolicSampleSize = kSample32;
        d.numSamples = 512;
        d.numOutputs = 1;
        d.outputs = &out;
        d.inputParameterChanges = changes;
        ap->process(d);
        d.inputParameterChanges = nullptr;
        for (int k = 0; k < 40; k++)   // (the load happens on the wrapper's idle: give it a second)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            ap->process(d);
        }
    };
    int a = -1, b = -1;
    for (int i = 0; i < (int)names.size(); i++)
    {
        if (names[i] == "EMBER FIELD") a = i;
        if (names[i] == "UNDERTOW") b = i;
    }
    if (have_sel && a >= 0 && b >= 0)
    {
        choose(a);
        auto pa = params();
        choose(b);
        auto pb = params();
        choose(a);
        auto pa2 = params();
        int differ = 0, again = 0;
        for (size_t i = 0; i < pa.size(); i++)
        {
            differ += pa[i] != pb[i];
            again += pa[i] != pa2[i];
        }
        CHECK(differ > 0, "EMBER FIELD then UNDERTOW: the parameters change");
        CHECK(again == 0, "EMBER FIELD again: the same parameters as the first time (%d differ)", again);
        CHECK(ec->getParamNormalized(sel.id) == (double)a / sel.stepCount, "the list shows the loaded preset");
    }

    std::printf("%s: %d programs (%s ... %s), %d failure(s)\n", fails ? "FAIL" : "ok", list.programCount,
                names.empty() ? "-" : names.front().c_str(), names.empty() ? "-" : names.back().c_str(), fails);
    // closed as a DAW closes it
    ap->setProcessing(false);
    comp->setActive(false);
    ec->setComponentHandler(nullptr);
    units->release();
    ap->release();
    if ((void *)ec != (void *)comp)
    {
        ec->terminate();
        ec->release();
    }
    comp->terminate();
    comp->release();
    factory->release();
    if (auto module_exit = (bool (*)())dlsym(lib, "ModuleExit")) module_exit();
    dlclose(lib);
    return fails ? 1 : 0;
}
