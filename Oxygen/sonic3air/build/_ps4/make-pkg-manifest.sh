#!/usr/bin/env bash
# Runs orbis-compat's scripts/ps4/make-pkg.sh with one `--extra <src>:<targ>` per line of a manifest.
#
#   make-pkg-manifest.sh <make-pkg.sh> <manifest> [make-pkg.sh arguments...]
#
# ⚠ WHY A MANIFEST AND NOT EXTRA_FILES: the unpacked game data is ~850 files, and a CMake POST_BUILD
# command reaches the shell as ONE `sh -c "<command>"` argument. Linux caps a single argument at
# 128 KiB (MAX_ARG_STRLEN), which 850 absolute src:targ pairs exceed. Running make-pkg.sh from here
# passes them as separate argv entries, where only the 2 MiB total applies.
#
# ⚠ COPYRIGHT GUARD: check-pkg-no-rom.sh runs on everything that goes in (manifest, eboot, icon) BEFORE
# make-pkg.sh, and on the staged package directory AFTER it; a hit fails the step and deletes the pkg.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAKE_PKG="$1"
MANIFEST="$2"
shift 2

args=("$@")
out_dir=""
guard_files=()
for ((i = 0; i < ${#args[@]}; i++)); do
  case "${args[$i]}" in
    --out-dir) out_dir="${args[$((i + 1))]}" ;;
    --eboot|--icon) guard_files+=(--file "${args[$((i + 1))]}") ;;
  esac
done

"$HERE/check-pkg-no-rom.sh" --manifest "$MANIFEST" ${guard_files+"${guard_files[@]}"}

n=0
while IFS= read -r line || [[ -n "$line" ]]; do
  [[ -z "$line" || "$line" == \#* ]] && continue
  args+=(--extra "$line")
  n=$((n + 1))
done < "$MANIFEST"

echo "make-pkg-manifest: ${n} extra file(s) from ${MANIFEST}"
"$MAKE_PKG" "${args[@]}"

if [[ -n "$out_dir" ]]; then
  if ! "$HERE/check-pkg-no-rom.sh" --dir "$out_dir/pkg-stage"; then
    rm -f "$out_dir"/IV0000-*.pkg
    exit 1
  fi
fi
