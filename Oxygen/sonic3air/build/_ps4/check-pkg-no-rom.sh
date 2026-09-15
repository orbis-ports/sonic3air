#!/usr/bin/env bash
# Copyright guard: the Sonic 3 & Knuckles ROM must NEVER end up in the .pkg.
#
#   check-pkg-no-rom.sh [--manifest <extra-files.txt>]... [--file <path>]... [--dir <dir>]...
#
# Fails (exit 1, clear message) when any checked file
#   * is exactly 4194304 bytes (0x400000 - the size of the S3&K ROM; oxygenproject.json RomCheck "size=0x400000"), or
#   * has a name matching *Sonic*Knuckles* (case-insensitive), or ends in .md / .gen / .smd (Mega Drive ROM dumps).
# The RomCheck checksum (Murmur2_64 0x344983ffcfeff8cb) is NOT computed: it is taken over the ROM AFTER the
# engine's overwrites, so the raw Steam file never matches it, and every file that could match has 0x400000
# bytes anyway - the size rule already covers it.
#
# Manifest lines are "<source>:<target in /app0>"; both the source file and the target name are checked.
# Used before make-pkg.sh (manifest, eboot, icon) and after it (the staged pkg directory).
set -euo pipefail

ROM_SIZE=4194304
bad=0

check_name() {	# <display> <name>
  local base="${2##*/}"
  shopt -s nocasematch
  if [[ "$base" == *sonic*knuckles* || "$base" == *.md || "$base" == *.gen || "$base" == *.smd ]]; then
    echo "!! check-pkg-no-rom: '$1' looks like a ROM by its name ($base) - a ROM must never be packaged" >&2
    bad=1
  fi
  shopt -u nocasematch
}

check_file() {	# <path>
  [[ -f "$1" ]] || return 0
  check_name "$1" "$1"
  local size
  size="$(stat -L -c %s "$1")"
  if [[ "$size" == "$ROM_SIZE" ]]; then
    echo "!! check-pkg-no-rom: '$1' is exactly ${ROM_SIZE} bytes (S3&K ROM size) - a ROM must never be packaged" >&2
    bad=1
  fi
}

n=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --manifest)
      [[ -f "$2" ]] || { echo "!! check-pkg-no-rom: no manifest $2" >&2; exit 1; }
      while IFS= read -r line || [[ -n "$line" ]]; do
        [[ -z "$line" || "$line" == \#* ]] && continue
        src="${line%%:*}"
        targ="${line#*:}"
        check_file "$src"
        check_name "$line" "$targ"
        n=$((n + 1))
      done < "$2"
      shift 2 ;;
    --file)
      check_file "$2"; n=$((n + 1)); shift 2 ;;
    --dir)
      if [[ -d "$2" ]]; then
        while IFS= read -r -d '' f; do
          check_file "$f"
          n=$((n + 1))
        done < <(find "$2" -type f -print0)
      fi
      shift 2 ;;
    *) echo "!! check-pkg-no-rom: unknown argument $1" >&2; exit 2 ;;
  esac
done

if [[ "$bad" != 0 ]]; then
  echo "!! check-pkg-no-rom: REFUSING to build the pkg (copyright: the ROM is uploaded by the user to /data/sonic3air/, never shipped)" >&2
  exit 1
fi
echo "check-pkg-no-rom: ${n} file(s) checked, no ROM"
