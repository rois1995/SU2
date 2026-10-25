#!/bin/bash
# DC-17 frozen full-M6 FIRST_ROW event through the production driver backend ([NativeFirstRowDC14 frozen], serial,
# nice 19, single thread). Usage: run_event.sh <binary> <event k> <input mesh .su2> <run dir> [timeout s]
# Resume only accepted receipts with unchanged inputs, helper/runtime hashes and published outputs.
set -euo pipefail
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc17
BIN=$1;K=$2;MESH=$3;DIR=$4;TO=${5:-9000}
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
mkdir -p "$DIR"
DIR="$(cd "$DIR" && pwd)"
cd "$DIR"
SHA=$(sha256sum $BIN|cut -d' ' -f1)
sed -e "s#^MESH_FILENAME=.*#MESH_FILENAME= $MESH#" -e "s#^RESTART_SOL=.*#RESTART_SOL= NO#" -e "s#^ADAP_NATIVE_REPARTITION=.*#ADAP_NATIVE_REPARTITION= NO#" -e "s#^ADAP_LOOP=.*#ADAP_LOOP= YES#" \
    -e "s#^ADAP_NATIVE_MAX_TIME=.*#ADAP_NATIVE_MAX_TIME= 7200#" -e "s#^MESH_OUT_FILENAME=.*#MESH_OUT_FILENAME= event${K}_out#" $E/../dc14/m6_full/run_serial.cfg > event$K.cfg.pending
printf 'ADAP_BL_METHOD= FIRST_ROW\nADAP_BL_H0_RAMP= 5\nADAP_SIZES= (500000, 500000, 500000, 500000, 500000, 500000)\n' >> event$K.cfg.pending
IN=$( (echo "dc17 env V=${SU2_DC17_V_SECONDS:-} old=${SU2_DC17_OLD_HALVING:-} probe=${SU2_DC17_PROBE:-}"; ldd "$BIN" | awk '/=> \// {print $3}' | xargs -r sha256sum; cat "$E/run_event.sh" "$E/check_event.py" event$K.cfg.pending "$MESH" "$E/../dc14/m6_full/coarse_sensor_donors.bin"; for sidecar in "$MESH.native_ref" "$MESH.native_rows"; do if [ -f "$sidecar" ]; then cat "$sidecar"; else echo "absent $sidecar"; fi; done ) | sha256sum | cut -d' ' -f1)
if [ -f event${K}_receipt.json ]; then
  if [ "$(cat event${K}.key 2>/dev/null || true)" = "$SHA $IN" ] &&
      python3 "$E/check_event.py" "$DIR/event$K" && sha256sum -c "event${K}.published.sha256"; then
    rm -f event$K.cfg.pending
    echo "REUSE event $K"; exit 0
  fi
  rm -f event$K.cfg.pending
  echo "Existing event is rejected or its provenance/publication is unverified; preserve it and use a new run directory." >&2
  exit 1
fi
mv event$K.cfg.pending event$K.cfg
echo "event=$K env=V${SU2_DC17_V_SECONDS:-}/old${SU2_DC17_OLD_HALVING:-} binary=$SHA inputs=$IN mesh=$MESH dir=$DIR start=$(date -Is)" >> $E/campaign.log
( /usr/bin/time -v env SU2_DC14_CONFIG=$DIR/event$K.cfg SU2_DC14_DONORS=$E/../dc14/m6_full/coarse_sensor_donors.bin SU2_DC14_OUT=$DIR/event${K} \
   SU2_DC14_EVENT=$K SU2_DC14_OUTER_PARAMETER=0.75 nice -n 19 timeout --kill-after=10s ${TO}s ${PERF:+perf record --no-buildid --no-buildid-cache -F 19 -g -o $DIR/perf.data} $BIN "[NativeFrozenBL14]" --use-colour no > event$K.out 2>&1 ) &
PID=$!
tree(){ local p=$1; echo $p; for c in $(pgrep -P $p || true); do tree $c; done; }
PEAK=0;while kill -0 $PID 2>/dev/null; do R1=$(for p in $(tree $PID); do ps -o rss= -p $p 2>/dev/null || true; done | awk '{s+=$1}END{print s+0}'); if [ "$R1" -gt "$PEAK" ]; then PEAK=$R1; fi; sleep 20; done
code=0
wait $PID || code=$?
if [ "$code" -eq 0 ]; then python3 "$E/check_event.py" "$DIR/event$K" || code=1; fi
if [ "$code" -eq 0 ]; then
  sha256sum event${K}_receipt.json event${K}_bl_report.json event${K}.su2 event${K}.su2.native_ref event${K}.su2.native_rows > event${K}.published.sha256
  echo "$SHA $IN" > event$K.key
fi
echo "event=$K exit=$code sampled_peak_rss_kB=$PEAK end=$(date -Is)" >> $E/campaign.log
exit $code
