/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* The FM-1's ports on Linux (fm1_link.h): ALSA's raw MIDI devices (/dev/snd/midiC<card>D<device>), no library. A
 * device's names are its card's (/proc/asound/cards: id, short and long name) and its own (/proc/asound/card<N>/
 * midi<D>, the first line). One device is both the FM-1's input and its output. A reader thread hands what it sends
 * to the link. */
#include "fm1_link.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <sys/stat.h>
#include <thread>
#include <atomic>
#include <unistd.h>

namespace
{
std::string first_line(const std::string &path)
{
    std::ifstream f(path);
    std::string s;
    std::getline(f, s);
    return s;
}

// card N's lines in /proc/asound/cards (" 1 [FM1    ]: USB-Audio - FM-1" and "   M-VAVE FM-1 at usb-...")
std::string card_names(int card)
{
    std::ifstream f("/proc/asound/cards");
    std::string line, out;
    bool in = false;
    while (std::getline(f, line))
    {
        int n = -1;
        if (std::sscanf(line.c_str(), " %d [", &n) == 1)
            in = n == card;
        if (in)
            out += line + "\n";
    }
    return out;
}

struct LinuxPorts : Fm1Ports
{
    int fd = -1;
    std::string dev;
    std::function<void(const uint8_t *, size_t)> cb;
    std::thread reader;
    std::atomic<bool> run{true}, dead{false};

    ~LinuxPorts() override
    {
        run = false;
        if (reader.joinable())
            reader.join();
        if (fd >= 0)
            close(fd);
    }
    bool send(const uint8_t *b, size_t n) override
    {
        for (int tries = 0; n && tries < 100; tries++)
        {
            ssize_t w = write(fd, b, n);
            if (w > 0)
                b += w, n -= (size_t)w;
            else if (w < 0 && errno != EAGAIN && errno != EINTR)
            {
                dead = true;
                return false;
            }
            else
                usleep(1000);
        }
        return n == 0;
    }
    bool alive() override
    {
        struct stat st;
        return !dead && stat(dev.c_str(), &st) == 0;
    }
    void read_loop()
    {
        uint8_t buf[256];
        while (run)
        {
            pollfd p{fd, POLLIN, 0};
            int r = ::poll(&p, 1, 100);
            if (r < 0 && errno != EINTR)
                break;
            if (r <= 0)
                continue;
            if (p.revents & (POLLERR | POLLHUP | POLLNVAL))
                break;
            ssize_t n = read(fd, buf, sizeof buf);
            if (n > 0)
                cb(buf, (size_t)n);
            else if (n < 0 && errno != EAGAIN && errno != EINTR)
                break;
        }
        if (run)
            dead = true;   // (unplugged)
    }
};
}   // namespace

std::unique_ptr<Fm1Ports> Fm1Ports::open(std::function<void(const uint8_t *, size_t)> in, bool &busy)
{
    busy = false;
    DIR *d = opendir("/dev/snd");
    if (!d)
        return nullptr;
    std::unique_ptr<LinuxPorts> found;
    while (dirent *e = readdir(d))
    {
        int card, device;
        char tail;
        if (std::sscanf(e->d_name, "midiC%dD%d%c", &card, &device, &tail) != 2)
            continue;
        std::string own = first_line("/proc/asound/card" + std::to_string(card) + "/midi" + std::to_string(device));
        if (!fm1_port_name(own) && !fm1_port_name(card_names(card)))
            continue;
        std::string dev = std::string("/dev/snd/") + e->d_name;
        int fd = ::open(dev.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
        {
            if (errno == EBUSY)
                busy = true;
            continue;
        }
        found.reset(new LinuxPorts);
        found->fd = fd;
        found->dev = dev;
        found->cb = std::move(in);
        break;
    }
    closedir(d);
    if (!found)
        return nullptr;
    busy = false;
    LinuxPorts *p = found.get();
    p->reader = std::thread([p] { p->read_loop(); });
    return found;
}
