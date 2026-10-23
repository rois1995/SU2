#!/bin/bash
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
B=$R/integration_evidence/build-integrated-v2
ROOT=${2:?root}; c=${1:?case}; BIN=${3:-$B/UnitTests/test_driver}
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$B/compile-tmp
SHA=$(sha256sum $BIN | cut -d' ' -f1)
mkdir -p $ROOT; cd $ROOT
SU2_DC8_BINARY_SHA=$SHA SU2_DC8_ROOT=$ROOT SU2_DC8_WP0B=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b SU2_DC8_M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts SU2_DC8_CASES=$c \
  nice -n 19 perf record -F ${PERF_F:-199} --call-graph dwarf,16384 -o $ROOT/perf.data $BIN "[NativeFirstRowDC8]" --use-colour no > $ROOT/$c.log 2>&1
echo exit=$?
