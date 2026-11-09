#!/bin/sh
# Run a command in WSL Ubuntu, in the checkout that matches the current directory.
#   tools/wsl.sh ./build.sh          tools/wsl.sh tests/run_tests.sh      tools/wsl.sh gh pr list
# From Git Bash. From PowerShell: .\tools\w.ps1 <command> (same thing).
# A Windows checkout or worktree (C:/Users/.../.claude/worktrees/<name>) runs at its /mnt/c path. Set
# TESSERA_WSL_DIR=/home/<you>/src/<wsl-worktree> to use a WSL worktree instead. gh, node and the JieLi
# toolchain exist only in WSL. git does not work there on a Windows worktree (its .git file points to a
# Windows path): use Windows git for git, and this for build, tests and gh (GH_REPO is set for you).
[ $# -gt 0 ] || { echo "usage: tools/wsl.sh <command> [args...]" >&2; exit 2; }
dir=${TESSERA_WSL_DIR:-}
if [ -z "$dir" ]; then
  here=$(pwd -W 2>/dev/null || pwd)
  dir=$(printf '%s' "$here" | tr '\\' '/')
  case $dir in
    [A-Za-z]:*) drive=$(printf '%s' "${dir%%:*}" | tr 'A-Z' 'a-z'); dir=/mnt/$drive${dir#?:} ;;
  esac
fi
repo=${GH_REPO:-Rastapopolix/tessera}
MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' exec wsl.exe -e bash -lc "cd '$dir' && export GH_REPO='$repo' && $*"
