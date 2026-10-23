#!/bin/bash
# DC-12 runner: one serial case at a time (nice 19, single thread). Usage:
#   run_case.sh <binary> <case> <root> [VAR=value ...]     cases: the six cuts, or volonly:<case> for [NativeVolumeOnlyDC12]
# Resumable: [NativeFirstRowDC12] skips a case whose JSON in <root> carries the same binary sha256, limits, runtime-input
# sha and the digest of its saved final mesh; a volume-only result is skipped when its JSON names the same binary/inputs.
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc12
BIN=$1;C=$2;ROOT=$3;shift 3
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
SHA=$(sha256sum $BIN | cut -d' ' -f1)
WP=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b;M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts
NAME=${C#volonly:}
case $NAME in
  m6_*) IN="$M6/$NAME.txt $(awk '$1=="cfg"||$1=="ref"{print $2}' $M6/$NAME.txt)";;
  annulus) A=$WP/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1;IN="$WP/annulus_case.txt ${A}_rejected_rank0.native_ref $A.cfg";;
  *) IN="$WP/$NAME.txt";;
esac
ROLE=$(awk -v c=$NAME '$1==c{print $2}' $E/fixture_roles.txt);[ -n "$ROLE" ] && set -- "$@" SU2_DC12_REFLECTIVE=$ROLE && IN="$IN $E/fixture_roles.txt"
INSHA=$(sha256sum $IN | sha256sum | cut -d' ' -f1)
mkdir -p $ROOT; cd $ROOT
if [ "${C#volonly:}" != "$C" ]; then
  if [ -f $ROOT/volonly12_$NAME.json ] && grep -q "$SHA $INSHA" $ROOT/volonly12_$NAME.key 2>/dev/null; then echo "REUSE volonly $NAME"; exit 0; fi
  TAGS="[NativeVolumeOnlyDC12]";LOG=$ROOT/volonly12_$NAME.log
else TAGS="[NativeFirstRowDC12]";LOG=$ROOT/$NAME.log; fi
echo "case=$C binary=$SHA inputs=$INSHA ($IN) root=$ROOT env=$* start=$(date -Is)" >> $E/campaign.log
( /usr/bin/time -v env "$@" SU2_DC12_INPUT_SHA=$INSHA SU2_DC12_BINARY_SHA=$SHA SU2_DC12_ROOT=$ROOT SU2_DC12_WP0B=$WP SU2_DC12_M6=$M6 SU2_DC12_CASES=$NAME \
  nice -n 19 timeout --kill-after=5s 7200s $BIN "$TAGS" --use-colour no > $LOG 2>&1 ) &
PID=$!
# resource receipt: peak RSS of the whole job sampled every 20 s
tree(){ local p=$1; echo $p; for c in $(pgrep -P $p); do tree $c; done; }
PEAK=0;while kill -0 $PID 2>/dev/null; do R1=$(for p in $(tree $PID); do ps -o rss= -p $p 2>/dev/null; done | awk '{s+=$1}END{print s+0}'); [ "$R1" -gt "$PEAK" ] && PEAK=$R1; sleep 20; done
wait $PID;code=$?
[ "${C#volonly:}" != "$C" ] && [ $code -eq 0 ] && echo "$SHA $INSHA" > $ROOT/volonly12_$NAME.key
echo "case=$C exit=$code sampled_peak_rss_kB=$PEAK end=$(date -Is) final=$(sha256sum $ROOT/dc12_$NAME.json $ROOT/dc12_${NAME}_final.mesh $ROOT/volonly12_$NAME.json 2>/dev/null | cut -c1-16 | tr '\n' ' ')" >> $E/campaign.log
exit $code
