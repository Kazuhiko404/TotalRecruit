#!/usr/bin/env bash
# Build, test and deploy TotalRecruit (Git Bash on Windows).
#
#   scripts/deploy.sh [--dry-run]
#
# Real run: refuse if Kenshi is running -> build Release|x64 -> run rules_tests ->
# copy the DLL (repo root, package folder, game) and gui/ -> add only the MISSING
# keys to the deployed config.json / messages.json (backups first) -> verify md5.
# --dry-run builds nothing and writes nothing: it reports what would happen.
#
# config.json / messages.json in the game folder are the user's. They are never
# overwritten: scripts/add_missing_keys.py only adds keys that are absent, at any
# depth, and the one value it changes is easyHoursCaged 12 -> 6 (the old default);
# any other value the user set is kept.
set -u

DRY=0
case "${1:-}" in
  "") ;;
  --dry-run) DRY=1 ;;
  *) echo "usage: scripts/deploy.sh [--dry-run]" >&2; exit 3 ;;
esac

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MSBUILD="/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
GAME="${TR_GAME_DIR:-/d/SteamLibrary/steamapps/common/Kenshi}"
DST="$GAME/mods/TotalRecruit"
PKG="$REPO/TotalRecruit"                 # the mod package folder, as shipped
BUILT="$REPO/x64/Release/TotalRecruit.dll"
TESTS_EXE="$REPO/tests/rules_tests/x64/Release/rules_tests.exe"
MERGE="$REPO/scripts/add_missing_keys.py"
STAMP="$(date +%Y%m%d-%H%M%S)"

say() { echo "[deploy] $*"; }
die() { echo "[deploy] ERROR: $*" >&2; exit 1; }
[ "$DRY" = 1 ] && say "DRY RUN: nothing will be built, copied or written"

# 1. Never touch a running game. kenshi_x64 only: KenshiPatcher.exe also matches "kenshi".
if tasklist 2>/dev/null | grep -qi 'kenshi_x64'; then
  if [ "$DRY" = 1 ]; then
    say "NOTE: kenshi_x64.exe is running. A real run would refuse (exit 1); this dry run only reports."
  else
    die "kenshi_x64.exe is running. Close Kenshi completely (the main menu is not enough) and run again. Nothing was changed."
  fi
fi

PY=""
for c in python python3; do command -v "$c" >/dev/null 2>&1 && "$c" --version >/dev/null 2>&1 && { PY="$c"; break; }; done
[ -n "$PY" ] || die "python is needed for the key merge and was not found"
[ -f "$MERGE" ] || die "missing $MERGE"

# 2. Build and test.
if [ "$DRY" = 1 ]; then
  say "would build:  MSBuild TotalRecruit.vcxproj Release|x64"
  say "would build and run: tests/rules_tests (a failing test aborts the deploy)"
  [ -f "$BUILT" ] || say "NOTE: $BUILT does not exist yet; the real run builds it first"
else
  [ -x "$MSBUILD" ] || [ -f "$MSBUILD" ] || die "MSBuild not found at $MSBUILD"
  cd "$REPO" || exit 1
  say "building TotalRecruit.dll (Release|x64)"
  "$MSBUILD" TotalRecruit.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo || die "DLL build failed"
  say "building rules_tests"
  "$MSBUILD" tests/rules_tests/rules_tests.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo || die "tests build failed"
  say "running rules_tests"
  "$TESTS_EXE" || die "rules_tests FAILED: nothing was deployed"
fi

# 3. DLL + gui.
say "DLL copies (source: x64/Release/TotalRecruit.dll):"
for target in "$REPO/TotalRecruit.dll" "$PKG/TotalRecruit.dll" "$DST/TotalRecruit.dll"; do
  say "  -> $target"
done
say "gui copy: $PKG/gui/ -> $DST/gui/ (recursive)"
for f in RE_Kenshi.json TotalRecruit.mod; do
  if [ -f "$DST/$f" ]; then say "  $f: present in the game folder, left alone"
  else say "  $f: absent in the game folder, would be copied (never overwritten once present)"; fi
done
[ -d "$DST" ] || say "NOTE: $DST does not exist: this is a fresh install, it would be created"

if [ "$DRY" = 0 ]; then
  [ -f "$BUILT" ] || die "$BUILT missing after the build"
  mkdir -p "$DST" || die "cannot create $DST"
  cp "$BUILT" "$REPO/TotalRecruit.dll" || die "copy to repo root failed"
  cp "$BUILT" "$PKG/TotalRecruit.dll" || die "copy to package folder failed"
  cp "$BUILT" "$DST/TotalRecruit.dll" || die "copy to the game folder failed (is Kenshi still locking it?)"
  mkdir -p "$DST/gui" && cp -r "$PKG/gui/." "$DST/gui/" || die "gui copy failed"
  for f in RE_Kenshi.json TotalRecruit.mod; do
    [ -f "$DST/$f" ] || cp "$PKG/$f" "$DST/$f" || die "copy of $f failed"
  done
fi

# 4. config.json / messages.json: add missing keys only.
merge_file() {  # name, extra args...
  local name="$1"; shift
  local shipped="$PKG/$name" deployed="$DST/$name"
  say "$name:"
  if [ ! -f "$deployed" ]; then
    say "  not in the game folder: the shipped file would be copied as is"
    [ "$DRY" = 1 ] || cp "$shipped" "$deployed" || die "copy of $name failed"
    return 0
  fi
  if [ "$DRY" = 1 ]; then
    "$PY" "$MERGE" "$shipped" "$deployed" --dry-run "$@" || die "merge of $name failed (nothing written)"
  else
    cp "$deployed" "$deployed.bak-$STAMP" || die "cannot back up $name"
    say "  backup: $name.bak-$STAMP"
    "$PY" "$MERGE" "$shipped" "$deployed" "$@" || die "merge of $name failed; the backup is $deployed.bak-$STAMP"
  fi
}
merge_file config.json --set-if easyHoursCaged 12 6.0
merge_file messages.json

# 5. md5 of the three DLLs.
say "md5:"
H1=""; H2=""; H3=""
[ -f "$BUILT" ] && H1="$(md5sum "$BUILT" | cut -d' ' -f1)"
[ -f "$REPO/TotalRecruit.dll" ] && H2="$(md5sum "$REPO/TotalRecruit.dll" | cut -d' ' -f1)"
[ -f "$PKG/TotalRecruit.dll" ] && HP="$(md5sum "$PKG/TotalRecruit.dll" | cut -d' ' -f1)" || HP=""
[ -f "$DST/TotalRecruit.dll" ] && H3="$(md5sum "$DST/TotalRecruit.dll" | cut -d' ' -f1)"
say "  x64/Release : ${H1:-(missing)}"
say "  repo root   : ${H2:-(missing)}"
say "  package     : ${HP:-(missing)}"
say "  game        : ${H3:-(missing)}"
if [ "$DRY" = 1 ]; then
  say "dry run finished: nothing was written."
  exit 0
fi
if [ -n "$H1" ] && [ "$H1" = "$H2" ] && [ "$H1" = "$HP" ] && [ "$H1" = "$H3" ]; then
  say "OK: all four copies match."
else
  die "md5 mismatch: you would be testing an old build"
fi
