/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's project files (projfile.h): the demos' names; a damaged record refused; a backup (made here as the
 * backup page writes one) read back slot by slot, and a damaged one refused; a .tsp opened into the core and saved out
 * again under its name. Run from the repository's top (demos/). Exit 0: all passed. */
#include <cstdio>
#include <string>
#include <vector>

#include "projfile.h"
#include "tess_core.h"

static int fails = 0;
#define CHECK(c, ...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(c))                                                                                                      \
        {                                                                                                              \
            std::printf("FAIL: " __VA_ARGS__);                                                                         \
            std::printf("\n");                                                                                         \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

static std::string b64(const std::vector<uint8_t> &b)
{
    static const char *T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o;
    size_t i = 0;
    for (; i + 2 < b.size(); i += 3)
    {
        uint32_t v = b[i] << 16 | b[i + 1] << 8 | b[i + 2];
        o += T[v >> 18], o += T[v >> 12 & 63], o += T[v >> 6 & 63], o += T[v & 63];
    }
    if (i < b.size())
    {
        uint32_t v = b[i] << 16 | (i + 1 < b.size() ? b[i + 1] << 8 : 0);
        o += T[v >> 18], o += T[v >> 12 & 63], o += i + 1 < b.size() ? T[v >> 6 & 63] : '=', o += '=';
    }
    return o;
}
static uint32_t crc32(const std::vector<uint8_t> &b)
{
    uint32_t c = 0xFFFFFFFFu;
    for (uint8_t x : b)
    {
        c ^= x;
        for (int k = 0; k < 8; k++) c = c >> 1 ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}
static std::string backup_json(const std::vector<uint8_t> &music, const std::vector<uint8_t> slot[4], bool damage)
{
    std::string j = "{\"format\":\"felucca-backup\",\"version\":1,\"objects\":[";
    for (int id = 0; id < 30; id++)
    {
        std::vector<uint8_t> d = id == 0 ? music : id >= 2 && id <= 5 ? slot[id - 2] : std::vector<uint8_t>(id % 3, (uint8_t)id);
        uint32_t c = crc32(d) ^ (damage && id == 2 ? 1u : 0u);   // (damaged: slot A)
        j += (id ? "," : "") + std::string("{\"id\":") + std::to_string(id) + ",\"size\":" + std::to_string(d.size()) +
             ",\"crc\":" + std::to_string(c) + ",\"data\":\"" + b64(d) + "\"}";
    }
    return j + "]}";
}

int main()
{
    const char *names[] = {"still-tide", "koto-garden", "undertow"};
    std::vector<uint8_t> rec[3];
    std::string nm[3];
    for (int i = 0; i < 3; i++)
    {
        CHECK(projfile::read_file(std::string("demos/") + names[i] + ".tsp", rec[i], 1u << 20), "read demos/%s.tsp", names[i]);
        CHECK(projfile::tsp_name(rec[i], nm[i]) && !nm[i].empty(), "%s: a TSP1 record with a name", names[i]);
        std::printf("%s.tsp: %zu bytes, \"%s\" -> %s\n", names[i], rec[i].size(), nm[i].c_str(),
                    projfile::tsp_file_name(nm[i]).c_str());
    }
    if (fails) return 1;
    CHECK(projfile::tsp_file_name("UNDERTOW") == "UNDERTOW.tsp" && projfile::tsp_file_name("") == "Tessera.tsp" &&
              projfile::tsp_file_name("A/B:C") == "A_B_C.tsp", "file names");
    std::vector<uint8_t> bad = rec[2];
    bad[bad.size() / 2] ^= 0x40;
    std::string x;
    CHECK(!projfile::tsp_name(bad, x), "a damaged record is refused (its checksum)");
    bad = rec[2];
    bad.pop_back();
    CHECK(!projfile::tsp_name(bad, x), "a cut-off record is refused");

    // a backup: the music playing UNDERTOW; slots A STILL TIDE, B empty, C KOTO GARDEN, D empty
    std::vector<uint8_t> slots[4] = {rec[0], {}, rec[1], {}};
    projfile::Backup bk;
    std::string err;
    CHECK(projfile::backup_read(backup_json(rec[2], slots, false), bk, err), "a backup reads (%s)", err.c_str());
    CHECK(bk.music == rec[2] && bk.slot[0] == rec[0] && bk.slot[1].empty() && bk.slot[2] == rec[1] && bk.slot[3].empty(),
          "the backup's music and slots A..D");
    CHECK(!projfile::backup_read(backup_json(rec[2], slots, true), bk, err), "a damaged backup is refused");
    std::printf("damaged backup: %s\n", err.c_str());
    CHECK(!projfile::backup_read("{\"format\":\"other\"}", bk, err), "another JSON is refused");
    CHECK(!projfile::backup_read("not json", bk, err), "not JSON is refused");

    // a .tsp into the core and out again: the same project (its name), a file to write
    tess_core_boot();
    CHECK(tess_core_load(rec[2].data(), (uint32_t)rec[2].size()) == 0, "UNDERTOW.tsp loads into the core");
    std::vector<uint8_t> out(tess_core_tsp_max());
    out.resize(tess_core_save(out.data(), (uint32_t)out.size()));
    std::string back;
    CHECK(projfile::tsp_name(out, back) && back == nm[2], "saved out again: \"%s\"", back.c_str());
    std::string path = "projfile-test-" + projfile::tsp_file_name(back);
    std::vector<uint8_t> again;
    CHECK(projfile::write_file(path, out) && projfile::read_file(path, again, 1u << 20) && again == out, "written and read back");
    std::remove(path.c_str());

    std::printf(fails ? "projfile test: %d FAILED\n" : "projfile test: ok\n", fails);
    return fails ? 1 : 0;
}
