/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Windows only (platform/plugin): the POSIX names tests/hostsim.c mentions. The plugin never calls them (they are in
 * the host tests' runners, which the plugin does not run); these stand-ins only let the unity build compile. */
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef int pid_t;
static inline pid_t fork(void) { return -1; }
static inline int pipe(int fd[2]) { (void)fd; return -1; }
static inline int close(int a) { (void)a; return 0; }
static inline long long read(int f, void *b, size_t n) { (void)f; (void)b; (void)n; return -1; }
static inline long long write(int f, const void *b, size_t n) { (void)f; (void)b; (void)n; return -1; }
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
static inline int clock_gettime(int c, struct timespec *t) { (void)c; return timespec_get(t, TIME_UTC) ? 0 : -1; }
#endif
