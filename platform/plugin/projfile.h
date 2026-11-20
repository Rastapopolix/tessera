/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The plugin's project files, apart from any window system: a .tsp (one project, a TSP1 record: what the editor's
 * "project file" and SAVE > PROJECT hold) and an FM-1 backup (.json, from the backup page or the Hub: the music
 * playing and the project slots A..D, among the device's other objects). */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace projfile
{
/* a whole TSP1 record (its magic, length and checksum): its NAME ("" none); false if it is not one */
bool tsp_name(const std::vector<uint8_t> &rec, std::string &name);
/* a file name for a project called name: "UNDERTOW.tsp", "Tessera.tsp" for none */
std::string tsp_file_name(const std::string &name);

/* an FM-1 backup (the backup page's JSON, "felucca-backup" version 1): the music playing and slots A..D (empty: no
 * project there), each object's size and CRC-32 checked. false and why in err if it is not one, or damaged */
struct Backup
{
    std::vector<uint8_t> music;
    std::vector<uint8_t> slot[4];
};
bool backup_read(const std::string &json, Backup &out, std::string &err);

/* whole files (path in UTF-8) */
bool read_file(const std::string &path, std::vector<uint8_t> &out, size_t max);
bool write_file(const std::string &path, const std::vector<uint8_t> &data);
}   // namespace projfile
