#!/bin/bash
# DC-7 (BL_FIRST_ROW_DESIGN rev 2.1): one serial case at a time, nice 19, single thread. Resumable: a case whose
# dc7_<case>.json exists is skipped by the harness; delete it to rerun. Usage: run_dc7.sh case[,case...] tag
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
B=$R/integration_evidence/build-integrated-v2
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$B/compile-tmp
BIN=$B/UnitTests/test_driver
SHA=$(sha256sum $BIN | cut -d' ' -f1)
mkdir -p $E/runs; cd $E/runs
for c in $(echo "$1" | tr ',' ' '); do
  echo "case=$c binary=$SHA start=$(date -Is)" >> $E/campaign.log
  SU2_DC7_ROOT=$E/runs SU2_DC7_WP0B=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b SU2_DC7_M6=$E/m6_cuts SU2_DC7_CASES=$c \
    /usr/bin/time -v nice -n 19 timeout --kill-after=5s 3600s $BIN "[NativeFirstRowDC7]" --use-colour no > $E/runs/$c.$2.log 2>&1
  echo "case=$c exit=$? end=$(date -Is)" >> $E/campaign.log
done
