#!/bin/sh
# Run a command in WSL Ubuntu, in the checkout that matches the current directory.
#   tools/wsl.sh ./build.sh          tools/wsl.sh gh pr list          tools/wsl.sh --copy tests/run_tests.sh
# From Git Bash. From PowerShell: .\tools\w.ps1 <command> (same thing).
# A Windows checkout or worktree (C:/Users/.../.claude/worktrees/<name>) runs at its /mnt/c path. Set
# TESSERA_WSL_DIR=/home/<you>/src/<wsl-worktree> to use a WSL worktree instead. gh, node and the JieLi
# toolchain exist only in WSL. git does not work there on a Windows worktree (its .git file points to a
# Windows path): use Windows git for git, and this for build, tests and gh (GH_REPO is set for you).
# tests/run_tests.sh needs FIFOs, which /mnt/c cannot hold: use --copy, which rsyncs the checkout (without
# .git and build/) to ~/wsl-copies/<name>, makes a throwaway git repo there (the release tests want one)
# and runs there. Output files stay in that copy.
copy=0
[ "$1" = "--copy" ] && { copy=1; shift; }
[ $# -gt 0 ] || { echo "usage: tools/wsl.sh [--copy] <command> [args...]" >&2; exit 2; }
dir=${TESSERA_WSL_DIR:-}
if [ -z "$dir" ]; then
  here=$(pwd -W 2>/dev/null || pwd)
  dir=$(printf '%s' "$here" | tr '\\' '/')
  case $dir in
    [A-Za-z]:*) drive=$(printf '%s' "${dir%%:*}" | tr 'A-Z' 'a-z'); dir=/mnt/$drive${dir#?:} ;;
  esac
fi
repo=${GH_REPO:-Rastapopolix/tessera}
pre=:
if [ $copy = 1 ]; then
  name=$(basename "$dir")
  pre="mkdir -p ~/wsl-copies/$name && rsync -a --delete --exclude .git --exclude build '$dir/' ~/wsl-copies/$name/ && cd ~/wsl-copies/$name && { [ -d .git ] || { git init -q && git add -A && git -c user.name=wsl -c user.email=wsl@localhost commit -qm copy; }; }"
fi
MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' exec wsl.exe -e bash -lc "cd '$dir' && $pre && export GH_REPO='$repo' && $*"
