#!/bin/bash
# D2 candidate fix 2 (envelope-pinned ownership), sequential single job; plus copy-faithfulness recheck on this binary.
set -u
B=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/build-integrated-v2
D=$B/../native_3d_bl_v1/construction_strategy_v1/d2
cd $B
run(){ /usr/bin/time -v env OMP_NUM_THREADS=1 MALLOC_ARENA_MAX=2 SU2_D2_CASE=$D/../dc2/m6/input SU2_D2_WITNESSES=$D/$1.i64 \
  SU2_D2_OUT=$D/$2.jsonl SU2_D2_LAW=copy SU2_D2_TIE=$3 SU2_D2_PIN=$4 SU2_D2_NATIVE=0 nice -n 19 ./UnitTests/test_driver "[NativeD2Replay]" > $D/$2.log 2>&1; }
run controls run3_copy_tie0_controls 0 0
run witnesses pin_witnesses 0 1
run controls pin_controls 0 1
echo campaign-finished
