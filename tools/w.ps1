# Run a command in WSL Ubuntu, in the checkout that matches the current directory (see tools/wsl.sh).
#   .\tools\w.ps1 ./build.sh        .\tools\w.ps1 gh pr list
# TESSERA_WSL_DIR=/home/<you>/src/<wsl-worktree> uses a WSL worktree instead of the /mnt/c path.
if ($args.Count -eq 0) { Write-Error "usage: tools\w.ps1 <command> [args...]"; exit 2 }
$dir = $env:TESSERA_WSL_DIR
if (-not $dir) {
    $here = (Get-Location).ProviderPath.Replace([string][char]92, '/')
    $dir = '/mnt/' + $here.Substring(0, 1).ToLower() + $here.Substring(2)
}
$repo = if ($env:GH_REPO) { $env:GH_REPO } else { 'Rastapopolix/tessera' }
wsl.exe -e bash -lc "cd '$dir' && export GH_REPO='$repo' && $($args -join ' ')"
exit $LASTEXITCODE
