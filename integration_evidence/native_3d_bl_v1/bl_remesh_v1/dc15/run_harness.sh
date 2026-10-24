#!/bin/bash
# DC-14 copy of the DC-13 harness runner (case directory from SU2_DC14_CASEDIR): one serial case at a time (nice 19, single thread). Usage:
#   run_case.sh <binary> <case> <root> [VAR=value ...]     cases: the six cuts, or volonly:<case> for [NativeVolumeOnlyDC13]
# Extra VAR=value are passed to the test (e.g. SU2_DC13_H0_SCALE=32, SU2_DC13_CHAIN=16,8, SU2_DC13_V_SECONDS=...).
# Resumable: [NativeFirstRowDC13] skips a case whose JSON in <root> carries the same binary sha256, limits (incl. the h0
# multiplier/chain), runtime-input sha and the digest of its saved final mesh; volume-only by its .key file.
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc15
BIN=$1;C=$2;ROOT=$3;shift 3
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
SHA=$(sha256sum $BIN | cut -d' ' -f1)
WP=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b;M6=${SU2_DC14_CASEDIR:-$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts}
NAME=${C#volonly:}
case $NAME in
  m6_*) IN="$M6/$NAME.txt $(awk '$1=="cfg"||$1=="ref"{print $2}' $M6/$NAME.txt)";;
  annulus) A=$WP/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1;IN="$WP/annulus_case.txt ${A}_rejected_rank0.native_ref $A.cfg";;
  *) IN="$WP/$NAME.txt";;
esac
ROLE=$(awk -v c=$NAME '$1==c{print $2}' $E/../dc14/fixture_roles.txt);[ -n "$ROLE" ] && set -- "$@" SU2_DC12_REFLECTIVE=$ROLE && IN="$IN $E/../dc14/fixture_roles.txt"
INSHA=$(sha256sum $IN | sha256sum | cut -d' ' -f1)
SUFFIX=$(echo "$*" | sed 's/SU2_DC1[23]_//g;s/REFLECTIVE=[a-zA-Z0-9,]*//;s/[ =,]/_/g;s/__*/_/g;s/^_//;s/_$//')
mkdir -p $ROOT; cd $ROOT
if [ "${C#volonly:}" != "$C" ]; then
  if [ -f $ROOT/volonly13_$NAME.json ] && grep -q "$SHA $INSHA" $ROOT/volonly13_$NAME.key 2>/dev/null; then echo "REUSE volonly $NAME"; exit 0; fi
  TAGS="[NativeVolumeOnlyDC13]";LOG=$ROOT/volonly13_$NAME.log
else TAGS="[NativeFirstRowDC13]";LOG=$ROOT/${NAME}${SUFFIX:+_$SUFFIX}.out; fi
echo "case=$C binary=$SHA inputs=$INSHA ($IN) root=$ROOT env=$* start=$(date -Is)" >> $E/campaign.log
( /usr/bin/time -v env "$@" SU2_DC13_INPUT_SHA=$INSHA SU2_DC13_BINARY_SHA=$SHA SU2_DC13_ROOT=$ROOT SU2_DC13_WP0B=$WP SU2_DC13_M6=$M6 SU2_DC13_CASES=$NAME \
  nice -n 19 timeout --kill-after=5s ${SU2_DC14_TIMEOUT:-14400}s $BIN "$TAGS" --use-colour no > $LOG 2>&1 ) &
PID=$!
tree(){ local p=$1; echo $p; for c in $(pgrep -P $p); do tree $c; done; }
PEAK=0;while kill -0 $PID 2>/dev/null; do R1=$(for p in $(tree $PID); do ps -o rss= -p $p 2>/dev/null; done | awk '{s+=$1}END{print s+0}'); [ "$R1" -gt "$PEAK" ] && PEAK=$R1; sleep 20; done
wait $PID;code=$?
[ "${C#volonly:}" != "$C" ] && [ $code -eq 0 ] && echo "$SHA $INSHA" > $ROOT/volonly13_$NAME.key
echo "case=$C env=$* exit=$code sampled_peak_rss_kB=$PEAK end=$(date -Is)" >> $E/campaign.log
exit $code
