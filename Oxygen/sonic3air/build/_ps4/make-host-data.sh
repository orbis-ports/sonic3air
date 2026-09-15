#!/usr/bin/env bash
# The release data the PS4 pkg ships (build.sh --data <dir>), made on a Linux host WITHOUT the ROM.
#
#   Oxygen/sonic3air/build/_ps4/make-host-data.sh --out <dir> [--work <dir>] [--host-bin <sonic3air_linux>] [--jobs N]
#
# <out> receives enginedata.bin, gamedata.bin, audiodata.bin, metadata.json and scripts.bin (+ SHA256SUMS).
#
#   1. The host tool: Oxygen/sonic3air/build/_cmake built with clang into <work> (default
#      ~/.cache/sonic3air-host-data), unless --host-bin names an existing sonic3air_linux.
#      ⚠ clang, not GCC: GCC 16 -O3 builds a lemonscript compiler that crashes on the game's scripts.
#   2. `sonic3air_linux -pack` in a staging copy of the data (it writes into its working directory
#      and reads ../oxygenengine/data) -> the three data packages + data/metadata.json.
#   3. `sonic3air_linux -compilescripts` in a copy laid out like /app0 -> saves/scripts.bin.
#      That mode skips the ROM entirely (GameLoader) and always compiles from source; the result
#      was verified byte-identical to a scripts.bin written by a normal run WITH the ROM.
#   4. check-pkg-no-rom.sh over <out>: nothing ROM-like may leave this script.
#
# Nothing is written into the repository - except that configuring _cmake renames
# framework/external/zlib/zlib/zconf.h (zlib's own CMakeLists does that); it is put back on exit.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/../../../.." && pwd)"
OXY="${ROOT}/Oxygen"
WORK="${HOME}/.cache/sonic3air-host-data"
OUT=""
HOST_BIN=""
JOBS="$(nproc)"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out) OUT="$2"; shift 2 ;;
    --work) WORK="$2"; shift 2 ;;
    --host-bin) HOST_BIN="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"; shift 2 ;;
    --jobs) JOBS="$2"; shift 2 ;;
    -h|--help) sed -n '2,21p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
[[ -n "${OUT}" ]] || { echo "!! --out <dir> is required" >&2; exit 2; }
mkdir -p "${OUT}" "${WORK}"
OUT="$(cd "${OUT}" && pwd)"
WORK="$(cd "${WORK}" && pwd)"

# ---------------------------------------------------------------- 1. host tool
ZCONF="${ROOT}/framework/external/zlib/zlib/zconf.h"
restore_zconf() {
  if [[ -f "${ZCONF}.included" && ! -f "${ZCONF}" ]]; then
    mv "${ZCONF}.included" "${ZCONF}"
  fi
}
trap restore_zconf EXIT

if [[ -z "${HOST_BIN}" ]]; then
  # _cmake puts the executable at ../../../../sonic3air relative to the build directory, hence the depth
  BUILD="${WORK}/b/u/i/build"
  HOST_BIN="${WORK}/sonic3air/sonic3air_linux"
  mkdir -p "${BUILD}"
  GEN=()
  command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)
  echo "== configuring host tool in ${BUILD}"
  # -D_LARGEFILE64_SOURCE: minizip uses fopen64 & co., which clang otherwise rejects as implicit declarations
  CC="${CC:-clang}" CXX="${CXX:-clang++}" cmake -S "${OXY}/sonic3air/build/_cmake" -B "${BUILD}" "${GEN[@]}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DUSE_DISCORD=OFF \
    -DBUILD_OXYGEN_ENGINEAPP=OFF \
    -DCMAKE_C_FLAGS="-D_LARGEFILE64_SOURCE -Wno-error=implicit-function-declaration -Wno-error=int-conversion"
  restore_zconf
  echo "== building host tool"
  cmake --build "${BUILD}" --target Sonic3AIR -j "${JOBS}"
fi
[[ -x "${HOST_BIN}" ]] || { echo "!! no host tool at ${HOST_BIN}" >&2; exit 1; }
echo "== host tool: ${HOST_BIN}"

# ---------------------------------------------------------------- 2. -pack
STAGE="${WORK}/stage"
rm -rf "${STAGE}"
mkdir -p "${STAGE}/sonic3air" "${STAGE}/oxygenengine"
cp -a --reflink=auto "${OXY}/sonic3air/data" "${STAGE}/sonic3air/data"
cp -a --reflink=auto "${OXY}/oxygenengine/data" "${STAGE}/oxygenengine/data"
ln -s "${HOST_BIN}" "${STAGE}/sonic3air/sonic3air_linux"
echo "== packing data"
(cd "${STAGE}/sonic3air" && HOME="${STAGE}/home" XDG_DATA_HOME="${STAGE}/home/.local/share" ./sonic3air_linux -pack)
for f in enginedata.bin gamedata.bin audiodata.bin data/metadata.json; do
  [[ -s "${STAGE}/sonic3air/${f}" ]] || { echo "!! -pack produced no ${f}" >&2; exit 1; }
done

# ---------------------------------------------------------------- 3. -compilescripts
RUN="${WORK}/compile"
rm -rf "${RUN}"
mkdir -p "${RUN}/data" "${RUN}/home"
cp "${STAGE}/sonic3air/"{enginedata,gamedata,audiodata}.bin "${STAGE}/sonic3air/data/metadata.json" "${RUN}/data/"
cp -a --reflink=auto "${OXY}/sonic3air/scripts" "${RUN}/scripts"
cp "${OXY}/sonic3air/_master_image_template/config.json" "${RUN}/config.json"
cp "${OXY}/sonic3air/oxygenproject.json" "${RUN}/oxygenproject.json"
ln -s "${HOST_BIN}" "${RUN}/sonic3air_linux"
echo "== compiling scripts (no ROM)"
rc=0
(cd "${RUN}" && HOME="${RUN}/home" XDG_DATA_HOME="${RUN}/home/.local/share" \
   SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}" \
   timeout 600 ./sonic3air_linux -compilescripts) || rc=$?
if [[ "${rc}" != 0 || ! -s "${RUN}/saves/scripts.bin" ]]; then
  echo "!! -compilescripts failed (exit ${rc}); engine log:" >&2
  find "${RUN}/home" -name logfile.txt -exec tail -n 60 {} \; >&2 || true
  exit 1
fi

# ---------------------------------------------------------------- 4. output + ROM guard
cp "${STAGE}/sonic3air/"{enginedata,gamedata,audiodata}.bin "${STAGE}/sonic3air/data/metadata.json" "${RUN}/saves/scripts.bin" "${OUT}/"
(cd "${OUT}" && sha256sum enginedata.bin gamedata.bin audiodata.bin metadata.json scripts.bin > SHA256SUMS)
"${HERE}/check-pkg-no-rom.sh" --dir "${OUT}"
echo "== data for build.sh --data ${OUT}:"
(cd "${OUT}" && ls -l enginedata.bin gamedata.bin audiodata.bin metadata.json scripts.bin && cat SHA256SUMS)
