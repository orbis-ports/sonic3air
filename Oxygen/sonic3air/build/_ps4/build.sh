#!/usr/bin/env bash
# Cross-build Sonic 3 A.I.R. for the PlayStation 4 (OpenOrbis toolchain, Mesa zink/RADV for GL).
#
#   Oxygen/sonic3air/build/_ps4/build.sh [--work <dir>] [--jobs N] [--orbis-compat <dir>]
#                                         [--data <dir>] [--sdl-orbis-audio] [--sdl-orbis-joystick]
#                                         [--sdl-orbis-video] [--sdl-orbis-all] [--no-pkg] [--clean]
#                                         [--with-remaster [--remaster-bin <audioremaster.bin>]]
#                                         [--mesa-bundle <dir>]
#
# Builds THIS checkout in place; everything the build produces lives under --work
# (default ~/.cache/sonic3air-ps4), nothing under the repository.
#
# Outputs (under <work>/build/sonic3air/):
#   Sonic3AIR.elf              the linked executable
#   eboot.bin                  create-fself output (checked: create-fself exits 0 on failure)
#   IV0000-SAIR00001_00-SONIC3AIR0000000.pkg  (+ SAIR00001.pkg symlink) when PkgTool.Core runs
#
# --data <dir> packages the release data found there into /app0/data/ (enginedata.bin, gamedata.bin,
# audiodata.bin, metadata.json, scripts.bin). Those come from a host build, without a ROM:
# make-host-data.sh (`sonic3air_linux -pack` + `-compilescripts`). The ROM is never packaged
# (check-pkg-no-rom.sh fails the pkg step on anything ROM-like).
#
# The remastered soundtrack (audioremaster.bin, ~126 MB) is NOT packaged by default: the game also loads
# it from /data/sonic3air/audioremaster.bin (FTP upload). --with-remaster ships it at /app0/data/ -
# taken from --remaster-bin, else <data>/audioremaster.bin, else ~/.cache/sonic3air-host/pack/sonic3air/.
# Without --data, --with-remaster ships the repository's unpacked data/audio/remastered instead.
#
# The pkg is its own target (Sonic3AIR_pkg): it is rebuilt whenever eboot.bin, the file list, a packaged
# file or the icon changes, and a second run with nothing changed does nothing.
#
# --sdl-orbis-* switch SDL's platform drivers on (agents B/C, phase F3). Default: SDL dummy drivers.
#
# Mesa: a mesa-ps4 checkout with build-orbis/ (found by orbis-env.sh), or a release bundle
# (orbis-ports/mesa-ps4 orbis-mesa-<sha>.tar.gz: include/ + build-orbis/src/...) given by --mesa-bundle <dir>
# or by ORBIS_MESA_SRC + ORBIS_MESA_BUILD in the environment (what .github/actions/orbis-toolchain exports).
# Release data without a ROM: make-host-data.sh --out <dir>, then --data <dir>.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
SRC="${ROOT}/Oxygen/sonic3air/build/_ps4"
WORK="${HOME}/.cache/sonic3air-ps4"
JOBS="$(nproc)"
DATA_DIR=""
SDL_AUDIO=OFF
SDL_JOY=OFF
SDL_VIDEO=OFF
BUILD_PKG=ON
CLEAN=0
REMASTER=OFF
REMASTER_BIN=""
# orbis-env.sh only knows mesa-ps4 checkouts and overwrites ORBIS_MESA_BUILD - keep a bundle given by the environment
MESA_BUNDLE_SRC=""
MESA_BUNDLE_BUILD=""
if [[ -n "${ORBIS_MESA_SRC:-}" && -n "${ORBIS_MESA_BUILD:-}" ]]; then
  MESA_BUNDLE_SRC="${ORBIS_MESA_SRC}"
  MESA_BUNDLE_BUILD="${ORBIS_MESA_BUILD}"
fi

# ⚠ The lines that cannot be shared - see orbis-compat/scripts/ps4/orbis-env.sh. Sibling directory of
# this repository first, then the personal default.
# ⚠ TWO REPOSITORIES SINCE 2026-09-18. orbis-compat is include/ and the archive; the porting kit is
# the toolchain file, the loader shim and scripts/ps4/. The overlay is found by a HEADER it owns -
# probing for scripts/ps4/orbis-env.sh would find the KIT and call it the overlay, which is a wrong
# build rather than an error.
for _c in "${ORBIS_COMPAT_DIR:-}" "${ROOT}/../orbis-compat" "${HOME}/src-ps4/orbis-compat"; do
  [[ -n "$_c" && -f "$_c/include/orbis_prefix.h" ]] && { ORBIS_COMPAT_DIR="$_c"; break; }
done
[[ -n "${ORBIS_COMPAT_DIR:-}" ]] || {
  echo "!! orbis-compat not found - clone it next to this repository, or set ORBIS_COMPAT_DIR" >&2
  exit 1
}
export ORBIS_COMPAT_DIR

# The kit. Its last candidate is the overlay, which carried these scripts until that date, so a pin
# older than the move still works - and that is the arm this repository uses until its pin moves.
for _k in "${ORBIS_KIT_DIR:-}" "${ROOT}/../orbis-porting-kit" "${HOME}/src-ps4/orbis-porting-kit" "${ORBIS_COMPAT_DIR}"; do
  [[ -n "$_k" && -f "$_k/scripts/ps4/orbis-env.sh" ]] && { ORBIS_KIT_DIR="$_k"; break; }
done
[[ -n "${ORBIS_KIT_DIR:-}" ]] || {
  echo "!! orbis-porting-kit not found - clone it next to this repository, or set ORBIS_KIT_DIR" >&2
  exit 1
}
export ORBIS_KIT_DIR

while [[ $# -gt 0 ]]; do
  case "$1" in
    --work) WORK="$2"; shift 2 ;;
    --orbis-compat) ORBIS_COMPAT_DIR="$2"; shift 2 ;;
    --jobs) JOBS="$2"; shift 2 ;;
    --data) DATA_DIR="$(cd "$2" && pwd)"; shift 2 ;;
    --sdl-orbis-audio) SDL_AUDIO=ON; shift ;;
    --sdl-orbis-joystick) SDL_JOY=ON; shift ;;
    --sdl-orbis-video) SDL_VIDEO=ON; shift ;;
    --sdl-orbis-all) SDL_AUDIO=ON; SDL_JOY=ON; SDL_VIDEO=ON; shift ;;
    --no-pkg) BUILD_PKG=OFF; shift ;;
    --with-remaster) REMASTER=ON; shift ;;
    --remaster-bin) REMASTER=ON; REMASTER_BIN="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"; shift 2 ;;
    --mesa-bundle) MESA_BUNDLE_SRC="$(cd "$2" && pwd)"; MESA_BUNDLE_BUILD="${MESA_BUNDLE_SRC}/build-orbis"; shift 2 ;;
    --clean) CLEAN=1; shift ;;
    -h|--help) sed -n '2,37p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

export ORBIS_COMPAT_DIR
. "${ORBIS_KIT_DIR}/scripts/ps4/orbis-env.sh"

if [[ -n "${MESA_BUNDLE_SRC}" ]]; then
  [[ -f "${MESA_BUNDLE_SRC}/include/EGL/egl.h" ]] || orbis_die "Mesa bundle ${MESA_BUNDLE_SRC} has no include/EGL/egl.h"
  [[ -f "${MESA_BUNDLE_BUILD}/src/amd/vulkan/libvulkan_radeon.a" ]] || orbis_die "Mesa bundle has no ${MESA_BUNDLE_BUILD}/src/amd/vulkan/libvulkan_radeon.a"
  export ORBIS_MESA_DIR="${MESA_BUNDLE_SRC}"
  export ORBIS_MESA_BUILD="${MESA_BUNDLE_BUILD}"
  export ORBIS_RADV_ARCHIVE="${MESA_BUNDLE_BUILD}/src/amd/vulkan/libvulkan_radeon.a"
  [[ -f "${MESA_BUNDLE_SRC}/manifest.txt" ]] && orbis_note "Mesa bundle: $(grep -E '^(bundle|mesa-commit)=' "${MESA_BUNDLE_SRC}/manifest.txt" | tr '\n' ' ')"
fi

[[ -f "${ORBIS_COMPAT_DIR}/build/liborbis-compat.a" ]] || \
  orbis_die "no ${ORBIS_COMPAT_DIR}/build/liborbis-compat.a - build orbis-compat first (orbis-compat/build.sh)"

# PkgTool.Core links libssl.so.1.1 and refuses OpenSSL 3. make-pkg.sh probes /nix/store itself;
# say up front whether it will find one, so a missing .pkg is not a surprise at the very end.
if [[ "${BUILD_PKG}" == ON && -z "${PS4_PKGTOOL_OPENSSL_LIB:-}" ]]; then
  _ssl=""
  for cand in /nix/store/*-openssl-1.1.1*/lib/libssl.so.1.1; do
    [[ -e "$cand" ]] && { _ssl="$(dirname "$cand")"; break; }
  done
  if [[ -n "$_ssl" ]]; then
    orbis_note "PkgTool OpenSSL 1.1: ${_ssl}"
  else
    echo "!! no OpenSSL 1.1 found for PkgTool.Core (set PS4_PKGTOOL_OPENSSL_LIB) - the .pkg step will likely fail" >&2
  fi
fi

if [[ "${REMASTER}" == ON && -n "${DATA_DIR}" && -z "${REMASTER_BIN}" ]]; then
  for cand in "${DATA_DIR}/audioremaster.bin" "${HOME}/.cache/sonic3air-host/pack/sonic3air/audioremaster.bin"; do
    [[ -f "$cand" ]] && { REMASTER_BIN="$cand"; break; }
  done
  [[ -n "${REMASTER_BIN}" ]] || orbis_die "--with-remaster: no audioremaster.bin in ${DATA_DIR} or ~/.cache/sonic3air-host/pack/sonic3air/ (use --remaster-bin)"
fi

BUILD="${WORK}/build"
[[ "${CLEAN}" == 1 ]] && rm -rf "${BUILD}"
mkdir -p "${BUILD}"

orbis_announce_driver
orbis_note "orbis-compat: ${ORBIS_COMPAT_DIR}"
orbis_note "mesa:         ${ORBIS_MESA_DIR} (${ORBIS_MESA_BUILD})"
orbis_note "SDL orbis drivers: audio=${SDL_AUDIO} joystick=${SDL_JOY} video=${SDL_VIDEO}"
orbis_note "remastered soundtrack in pkg: ${REMASTER}${REMASTER_BIN:+ (${REMASTER_BIN})}"

GEN=()
command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)

echo "== configuring ${BUILD}"
cmake -S "${SRC}" -B "${BUILD}" "${GEN[@]}" \
      -DCMAKE_TOOLCHAIN_FILE="${ORBIS_CMAKE_TOOLCHAIN}" \
      -DORBIS_COMPAT_DIR="${ORBIS_COMPAT_DIR}" \
      -DORBIS_MESA_BUILD="${ORBIS_MESA_BUILD}" \
      -DORBIS_MESA_SRC="${ORBIS_MESA_DIR}" \
      -DCMAKE_BUILD_TYPE=Release \
      -DPS4_BUILD_PKG="${BUILD_PKG}" \
      -DPS4_DATA_DIR="${DATA_DIR}" \
      -DPS4_PKG_REMASTERED_AUDIO="${REMASTER}" \
      -DPS4_REMASTER_BIN="${REMASTER_BIN}" \
      -DPS4_SDL_ORBIS_AUDIO="${SDL_AUDIO}" \
      -DPS4_SDL_ORBIS_JOYSTICK="${SDL_JOY}" \
      -DPS4_SDL_ORBIS_VIDEO="${SDL_VIDEO}" \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5

echo "== building"
TARGET=Sonic3AIR
[[ "${BUILD_PKG}" == ON ]] && grep -q 'Sonic3AIR_pkg' "${BUILD}/build.ninja" "${BUILD}/Makefile" 2>/dev/null && TARGET=Sonic3AIR_pkg
cmake --build "${BUILD}" --target "${TARGET}" -j "${JOBS}"

OUT="${BUILD}/sonic3air"
[[ -s "${OUT}/Sonic3AIR.elf" ]] || orbis_die "no ELF at ${OUT}/Sonic3AIR.elf"
[[ -s "${OUT}/eboot.bin" ]]     || orbis_die "no eboot.bin at ${OUT}/eboot.bin (create-fself exits 0 on failure)"

echo
echo "ELF:   ${OUT}/Sonic3AIR.elf ($(stat -c %s "${OUT}/Sonic3AIR.elf") bytes)"
echo "eboot: ${OUT}/eboot.bin ($(stat -c %s "${OUT}/eboot.bin") bytes)"
PKG="$(ls -1 "${OUT}"/IV0000-*.pkg 2>/dev/null | head -n1 || true)"
if [[ -n "${PKG}" ]]; then
  echo "pkg:   ${PKG} ($(du -h "${PKG}" | cut -f1))"
elif [[ "${BUILD_PKG}" == ON ]]; then
  echo "!! no .pkg produced - check the pkg step output above (PkgTool.Core / OpenSSL 1.1)" >&2
  exit 1
fi
