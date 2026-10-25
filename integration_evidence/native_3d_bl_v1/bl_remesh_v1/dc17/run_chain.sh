#!/bin/bash
# DC-17 full-M6 chain (resumable through run_event.sh receipts): event 0 from Coarse.su2, then events 1.. from the
# previous published event while each event is valid/published and takes < 60 min. Census + pictures per event.
# Usage: run_chain.sh <binary> <root dir> <last event>
P=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc17
B=$1;ROOT=$2;LAST=${3:-5};H0=(6.4e-05 3.2e-05 1.6e-05 8e-06 4e-06 2e-06)
export OMP_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/build-integrated-v2/compile-tmp
IN=$P/../dc7/m6_field/Coarse.su2
for K in $(seq 0 $LAST); do
  D=$ROOT/event$K
  T0=$(date +%s)
  $P/run_event.sh $B $K $IN $D 5400; code=$?
  T1=$(date +%s)
  echo "chain event=$K exit=$code seconds=$((T1-T0)) dir=$D" >> $P/campaign.log
  if [ $code -ne 0 ] || [ ! -f $D/event$K.su2 ]; then echo "chain stop: event $K not published (exit $code)" >> $P/campaign.log; exit 1; fi
  [ -f $D/layer_census_event$K.txt ] || (cd $D && nice -n 19 python3 $P/layer_census.py event${K}_final.mesh ${H0[$K]} > layer_census_event$K.txt 2>&1)
  [ -d $ROOT/png_event$K ] || nice -n 19 python3 $P/pictures.py $D/event${K}_final.mesh $D/event${K}_bl_report.json $ROOT/png_event$K "DC-17 full M6 event $K ($(basename $B))" > $ROOT/pictures_event$K.out 2>&1
  S=$(python3 -c "import json;d=json.load(open('$D/event${K}_bl_report.json'));print(d.get('total_seconds',0))")
  if python3 -c "import sys;sys.exit(0 if float('$S')>3600 else 1)"; then echo "chain stop: event $K took $S s (> 60 min)" >> $P/campaign.log; exit 1; fi
  IN=$D/event$K.su2
done
echo CHAIN COMPLETE
