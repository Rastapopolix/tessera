/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Theo Dainis */
/* Windows only (platform/plugin): see ../unistd.h */
#pragma once
#include "../unistd.h"
static inline pid_t waitpid(pid_t p, int *st, int o) { (void)p; (void)o; if (st) *st = 0; return -1; }
#define WIFEXITED(s) 1
#define WEXITSTATUS(s) (s)
#define WIFSIGNALED(s) 0
#define WTERMSIG(s) 0
