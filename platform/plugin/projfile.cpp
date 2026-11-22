/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's project files (projfile.h). TSP1 as firmware/src/project_tsp.c writes it (tools/demo_backup.py reads it
 * the same way); the backup as the backup page writes it (web/editor.html, tools/demo_backup.py). */
#include "projfile.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace projfile
{
namespace
{
uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
uint32_t le16(const uint8_t *p) { return p[0] | p[1] << 8; }

uint32_t fnv1a(const uint8_t *p, size_t n)   // project.c proj_hash: over the record before its checksum
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

uint32_t crc32(const uint8_t *p, size_t n)   // zlib's CRC-32 (the backup's objects)
{
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++)
    {
        c ^= p[i];
        for (int k = 0; k < 8; k++) c = c >> 1 ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}

bool base64(const std::string &in, std::vector<uint8_t> &out)
{
    out.clear();
    uint32_t acc = 0;
    int bits = 0;
    for (char ch : in)
    {
        int v = ch >= 'A' && ch <= 'Z' ? ch - 'A' : ch >= 'a' && ch <= 'z' ? ch - 'a' + 26 : ch >= '0' && ch <= '9' ? ch - '0' + 52
              : ch == '+' ? 62 : ch == '/' ? 63 : ch == '=' ? -2 : -1;
        if (v == -2) break;
        if (v < 0) return false;
        acc = acc << 6 | (uint32_t)v;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back((uint8_t)(acc >> bits));
        }
    }
    return true;
}

// a small JSON reader: enough for the backup (objects, arrays, strings, numbers, true / false / null)
struct Json
{
    enum Kind { NUL, NUM, STR, ARR, OBJ, BOOL } kind = NUL;
    double num = 0;
    std::string str;
    std::vector<Json> items;                                // ARR, or OBJ's values
    std::vector<std::string> keys;                          // OBJ
    const Json *get(const char *k) const
    {
        for (size_t i = 0; i < keys.size(); i++)
            if (keys[i] == k) return &items[i];
        return nullptr;
    }
};
struct Parser
{
    const std::string &s;
    size_t i = 0;
    int depth = 0;
    void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) i++; }
    bool str(std::string &o)
    {
        if (i >= s.size() || s[i] != '"') return false;
        for (i++; i < s.size(); i++)
        {
            char c = s[i];
            if (c == '"') { i++; return true; }
            if (c == '\\')
            {
                if (++i >= s.size()) return false;
                char e = s[i];
                if (e == 'u') { if (i + 4 >= s.size()) return false; o += '?'; i += 4; }   // (not in a backup's fields)
                else o += e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r' : e == 'b' ? '\b' : e == 'f' ? '\f' : e;
            }
            else
                o += c;
        }
        return false;
    }
    bool val(Json &v)
    {
        ws();
        if (i >= s.size() || ++depth > 32) return false;
        bool ok = true;
        char c = s[i];
        if (c == '{')
        {
            v.kind = Json::OBJ;
            i++;
            ws();
            if (i < s.size() && s[i] == '}') i++;
            else
                for (;;)
                {
                    std::string k;
                    ws();
                    if (!str(k)) { ok = false; break; }
                    ws();
                    if (i >= s.size() || s[i++] != ':') { ok = false; break; }
                    v.keys.push_back(k);
                    v.items.emplace_back();
                    if (!val(v.items.back())) { ok = false; break; }
                    ws();
                    if (i < s.size() && s[i] == ',') { i++; continue; }
                    if (i < s.size() && s[i] == '}') { i++; break; }
                    ok = false;
                    break;
                }
        }
        else if (c == '[')
        {
            v.kind = Json::ARR;
            i++;
            ws();
            if (i < s.size() && s[i] == ']') i++;
            else
                for (;;)
                {
                    v.items.emplace_back();
                    if (!val(v.items.back())) { ok = false; break; }
                    ws();
                    if (i < s.size() && s[i] == ',') { i++; continue; }
                    if (i < s.size() && s[i] == ']') { i++; break; }
                    ok = false;
                    break;
                }
        }
        else if (c == '"')
        {
            v.kind = Json::STR;
            ok = str(v.str);
        }
        else if (!s.compare(i, 4, "true") || !s.compare(i, 5, "false") || !s.compare(i, 4, "null"))
        {
            v.kind = s[i] == 'n' ? Json::NUL : Json::BOOL;
            v.num = s[i] == 't';
            i += s[i] == 'f' ? 5 : 4;
        }
        else
        {
            size_t at = i;
            while (i < s.size() && (std::strchr("+-.eE", s[i]) || (s[i] >= '0' && s[i] <= '9'))) i++;
            v.kind = Json::NUM;
            ok = i > at;
            if (ok) v.num = std::strtod(s.substr(at, i - at).c_str(), nullptr);
        }
        depth--;
        return ok;
    }
};
}   // namespace

bool tsp_name(const std::vector<uint8_t> &r, std::string &name)
{
    name.clear();
    size_t n = r.size();
    if (n < 16 || std::memcmp(r.data(), "TSP1", 4) || le32(&r[4]) != n || le32(&r[n - 4]) != fnv1a(r.data(), n - 4))
        return false;
    size_t pos = 12, parts = le16(&r[8]);
    for (size_t k = 0; k < parts && pos + 4 <= n - 4; k++)
    {
        size_t len = le16(&r[pos + 2]);
        if (pos + 4 + len > n - 4) return false;
        if (r[pos] == 10)   // TSP_NAME
        {
            for (size_t j = 0; j < len && r[pos + 4 + j]; j++)
            {
                char c = (char)r[pos + 4 + j];
                name += c >= 32 && c < 127 ? c : '?';
            }
            break;
        }
        pos += 4 + len;
    }
    return true;
}

std::string tsp_file_name(const std::string &name)
{
    std::string f;
    for (char c : name)
        f += (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ? c : c == ' ' ? ' ' : '_';
    while (!f.empty() && f.back() == ' ') f.pop_back();
    while (!f.empty() && f.front() == ' ') f.erase(f.begin());
    return (f.empty() ? std::string("Tessera") : f) + ".tsp";
}

bool backup_read(const std::string &json, Backup &out, std::string &err)
{
    Json root;
    Parser p{json};
    if (!p.val(root) || root.kind != Json::OBJ)
    {
        err = "not a backup (not JSON)";
        return false;
    }
    const Json *fmt = root.get("format"), *ver = root.get("version"), *objs = root.get("objects");
    if (!fmt || fmt->str != "felucca-backup" || !ver || ver->num != 1 || !objs || objs->kind != Json::ARR)
    {
        err = "not a backup from the backup page";
        return false;
    }
    bool seen[6] = {};
    for (const Json &o : objs->items)
    {
        const Json *id = o.get("id"), *size = o.get("size"), *crc = o.get("crc"), *data = o.get("data");
        if (!id || id->kind != Json::NUM) continue;
        int k = (int)id->num;
        if (k != 0 && (k < 2 || k > 5)) continue;   // (the music playing, the slots A..D; the rest is the device's)
        std::vector<uint8_t> b;
        if (!size || !crc || (size->num > 0 && (!data || !base64(data->str, b))) || b.size() != (size_t)size->num ||
            (size->num > 0 && crc32(b.data(), b.size()) != (uint32_t)crc->num))
        {
            err = "object " + std::to_string(k) + " is damaged (size or checksum): take a new backup";
            return false;
        }
        std::string name;
        if (!b.empty() && !tsp_name(b, name))
        {
            err = "its projects are not TSP1 records: back up again with Tessera 2.0 or later";
            return false;
        }
        (k == 0 ? out.music : out.slot[k - 2]) = b;
        seen[k] = true;
    }
    if (!seen[0])
    {
        err = "no project in it (a backup of another firmware?)";
        return false;
    }
    return true;
}

bool read_file(const std::string &path, std::vector<uint8_t> &out, size_t max)
{
    std::ifstream f(std::filesystem::u8path(path), std::ios::binary);
    if (!f) return false;
    out.clear();
    char buf[65536];
    while (f.read(buf, sizeof buf) || f.gcount())
    {
        out.insert(out.end(), buf, buf + f.gcount());
        if (out.size() > max) return false;
    }
    return true;
}

bool write_file(const std::string &path, const std::vector<uint8_t> &data)
{
    std::ofstream f(std::filesystem::u8path(path), std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char *>(data.data()), (std::streamsize)data.size());
    return (bool)f;
}

static std::filesystem::path settings_dir()
{
    namespace fs = std::filesystem;
#ifdef _WIN32
    const wchar_t *app = _wgetenv(L"APPDATA");
    return app && *app ? fs::path(app) / "Tessera" : fs::path();
#else
    const char *home = std::getenv("HOME");
#ifdef __APPLE__
    return home && *home ? fs::path(home) / "Library" / "Application Support" / "Tessera" : fs::path();
#else
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return fs::path(xdg) / "tessera";
    return home && *home ? fs::path(home) / ".config" / "tessera" : fs::path();
#endif
#endif
}

std::string setting_get(const char *name)
{
    std::filesystem::path d = settings_dir();
    if (d.empty()) return "";
    std::ifstream f(d / name);
    std::string v;
    if (!f || !std::getline(f, v)) return "";
    while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) v.pop_back();
    return v;
}

bool setting_put(const char *name, const std::string &value)
{
    std::filesystem::path d = settings_dir();
    std::error_code ec;
    if (d.empty() || (std::filesystem::create_directories(d, ec), ec)) return false;
    std::ofstream f(d / name, std::ios::trunc);
    f << value << "\n";
    return (bool)f;
}
std::string presets_dir()
{
    namespace fs = std::filesystem;
#ifdef _WIN32
    const wchar_t *home = _wgetenv(L"USERPROFILE");
    fs::path d = home && *home ? fs::path(home) / "Documents" / "Tessera" / "Presets" : fs::path();
#else
    const char *home = std::getenv("HOME");
    fs::path d = home && *home ? fs::path(home) / "Documents" / "Tessera" / "Presets" : fs::path();
#endif
    std::error_code ec;
    if (d.empty() || (fs::create_directories(d, ec), ec)) return "";
    return d.u8string();   // (C++17: UTF-8 in a std::string)
}
}   // namespace projfile
