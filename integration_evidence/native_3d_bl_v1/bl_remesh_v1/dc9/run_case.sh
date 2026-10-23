#!/bin/bash
# DC-9 runner: one serial case at a time (nice 19, single thread). Usage:
#   run_case.sh <binary> <DC8|DC9> <case> <root> [VAR=value ...]   (extra env, e.g. SU2_DC9_V_SECONDS=600)
# Resumable: the harness skips a case whose JSON in <root> carries the same binary sha256 and a total time.
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc9
BIN=$1;H=$2;C=$3;ROOT=$4;shift 4
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
SHA=$(sha256sum $BIN | cut -d' ' -f1)
mkdir -p $ROOT; cd $ROOT
echo "case=$C harness=$H binary=$SHA root=$ROOT env=$* start=$(date -Is)" >> $E/campaign.log
env "$@" SU2_${H}_BINARY_SHA=$SHA SU2_${H}_ROOT=$ROOT SU2_${H}_WP0B=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b \
  SU2_${H}_M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts SU2_${H}_CASES=$C \
  /usr/bin/time -v nice -n 19 timeout --kill-after=5s 7200s $BIN "[NativeFirstRow${H}]" --use-colour no > $ROOT/$C.log 2>&1
code=$?
echo "case=$C harness=$H exit=$code end=$(date -Is)" >> $E/campaign.log
exit $code
