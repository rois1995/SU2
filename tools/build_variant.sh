#!/usr/bin/env bash
# Build one tag of the P1 flux variant series into its own install folder.
#
#   tools/build_variant.sh <tag> [build_dir]
#
#   <tag>        any git ref, for example p1v/3-modnormals. The name is used as the folder name.
#   [build_dir]  meson build folder. Reusing one folder across tags saves work and disk.
#                Default: /media/rausa/4TB/SU2_Versions/SU2_P1FluxVariants_variant_build
#
# The binary lands in
#   /media/rausa/4TB/SU2_Versions/SU2_P1FluxVariants_bin/<tag>/bin/SU2_CFD
#
# NOTE: the tip binary contains all options, so one binary runs every variant through the
# config file (BC_FLUX_CLOSURE, CENTRAL_GROUP_FLUX, DISSIPATION_MOD_NORMALS, VISCOUS_SCHEME,
# EDGE_FORM_P1, SPLIT_EDGE_FLUX, and FLUX_CORRECTION with MOD_CENTROIDS for variant D).
# All options default to off. Tagged binaries exist only for isolation.
#
# The tag is built from a private detached worktree, so this tree is left alone. Resource
# limits: nice 19, three workers, single-threaded libraries, temporary files on the 4TB disk.
# Override the worker count with SU2_BUILD_WORKERS. Normal (non-AD) build, no tecio or cgns.
# Nothing is pushed and no ref is changed.
set -euo pipefail

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
  echo "usage: $0 <tag> [build_dir]" >&2
  exit 2
fi

TAG=$1
ROOT=/media/rausa/4TB/SU2_Versions
BUILD=${2:-$ROOT/SU2_P1FluxVariants_variant_build}
SRC=$(git -C "$(dirname "$0")/.." rev-parse --show-toplevel)
BIN=$ROOT/SU2_P1FluxVariants_bin/$TAG
TREE=$ROOT/SU2_P1FluxVariants_src
export TMPDIR=$ROOT/SU2_P1FluxVariants_tmp
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1
WORKERS=${SU2_BUILD_WORKERS:-3}

SHA=$(git -C "$SRC" rev-parse --verify "$TAG^{commit}")
mkdir -p "$TMPDIR" "$BIN"

# One private source tree, moved to the requested commit. Files that keep their content keep
# their time stamp, so a shared build folder only rebuilds what really differs between tags.
if [ ! -e "$TREE/.git" ]; then
  git -C "$SRC" worktree add --detach "$TREE" "$SHA"
else
  git -C "$TREE" checkout --detach --quiet "$SHA"
fi

# Normal builds need only these submodules. They are copied once, with no git metadata.
for m in eigen meson mel; do
  if [ ! -f "$TREE/externals/$m/.su2_copied" ]; then
    rm -rf "${TREE:?}/externals/$m"
    cp -a "$SRC/externals/$m" "$TREE/externals/$m"
    touch "$TREE/externals/$m/.su2_copied"
  fi
done
cp -a "$SRC/ninja" "$TREE/ninja"
touch "$TREE/su2preconfig.timestamp"

export NINJA=$TREE/ninja
MESON=("$(command -v python3)" "$TREE/externals/meson/meson.py")
if [ ! -f "$BUILD/build.ninja" ]; then
  nice -n 19 "${MESON[@]}" setup "$BUILD" "$TREE" --prefix="$BIN" -Dbuildtype=release \
    -Denable-tecio=false -Denable-cgns=false -Dwith-omp=false -Dwith-mpi=enabled
else
  nice -n 19 "${MESON[@]}" configure "$BUILD" --prefix="$BIN"
fi

nice -n 19 "$NINJA" -C "$BUILD" -j "$WORKERS" install
echo "$SHA $TAG" > "$BIN/BUILT_FROM"
echo "built $TAG ($SHA) -> $BIN/bin/SU2_CFD"
