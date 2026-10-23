#!/bin/bash
# DC-8: one serial case at a time, nice 19, single thread. Resumable: a case whose dc8_<case>.json exists in the run
# root and the same binary sha256 is skipped by the harness; delete it to rerun. Usage: run_dc8.sh case[,case...] tag [root]
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
B=$R/integration_evidence/build-integrated-v2
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc8
ROOT=${3:-$E/runs}
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$B/compile-tmp
BIN=$B/UnitTests/test_driver
SHA=$(sha256sum $BIN | cut -d' ' -f1)
mkdir -p $ROOT; cd $ROOT
for c in $(echo "$1" | tr ',' ' '); do
  echo "case=$c binary=$SHA root=$ROOT start=$(date -Is)" >> $E/campaign.log
  SU2_DC8_BINARY_SHA=$SHA SU2_DC8_ROOT=$ROOT SU2_DC8_WP0B=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b SU2_DC8_M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts SU2_DC8_CASES=$c \
    /usr/bin/time -v nice -n 19 timeout --kill-after=5s 3600s $BIN "[NativeFirstRowDC8]" --use-colour no > $ROOT/$c.$2.log 2>&1
  echo "case=$c exit=$? end=$(date -Is)" >> $E/campaign.log
done
