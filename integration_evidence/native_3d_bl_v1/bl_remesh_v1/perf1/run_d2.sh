#!/bin/bash
# D2 witnesses + controls through the production owner-pinned edge integration, with long-double length dump and
# metric-balanced split timing. Usage: run_d2.sh <binary> <tag>. One serial job, nice 19, single thread.
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/perf1
D2=$R/integration_evidence/native_3d_bl_v1/construction_strategy_v1/d2
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
BIN=$1;TAG=$2;SHA=$(sha256sum $BIN | cut -d' ' -f1)
mkdir -p $E/d2_$TAG; cd $E/d2_$TAG
for set in witnesses controls; do
  echo "set=$set binary=$SHA start=$(date -Is)" >> $E/campaign.log
  SU2_D2_CASE=$D2/../dc2/m6/input SU2_D2_WITNESSES=$D2/$set.i64 SU2_D2_PIN_RECORD=$D2/pin_$set.jsonl SU2_D2_DUMP=$E/d2_$TAG/$set.dump SU2_D2_SPLIT=1 \
    /usr/bin/time -v nice -n 19 timeout 3600 $BIN "[NativeD2Production]" --use-colour no > $E/d2_$TAG/$set.log 2>&1
  echo "set=$set exit=$? end=$(date -Is)" >> $E/campaign.log
done
