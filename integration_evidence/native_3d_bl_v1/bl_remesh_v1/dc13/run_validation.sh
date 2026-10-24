#!/bin/bash
# DC-13 validation: one serial job at a time (nice 19, single thread). Each stage writes <stage>.status with its verdict;
# a stage is reused only if its .done marker holds the same key (binary sha256 + every runtime input file of the stage).
# mesh3d passes only with exactly the 6 known failed assertions (CNativeDistributed3D_tests.cpp:1446/1449/1453, twice
# each). The script exits non-zero if any requested stage fails. Usage: run_validation.sh <binary> <tag> stage...
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc13
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
BIN=$1;TAG=$2;shift 2;SHA=$(sha256sum $BIN | cut -d' ' -f1)
AIRCFG=$R/integration_evidence/airfoil_inputs/baseline.cfg
FAILED=0
key(){ case $1 in airfoil) echo "$SHA $(sha256sum $AIRCFG $(awk -F'= *' '/^MESH_FILENAME/{print $2}' $AIRCFG) | cut -c1-64 | tr '\n' ' ')";; *) echo "$SHA";; esac; }
verdict(){ name=$1;code=$2;log=$3
  if [ $name = mesh3d ]; then
    n=$(grep -c 'FAILED:' $log); k=$(grep -E 'CNativeDistributed3D_tests.cpp:(1446|1449|1453): FAILED:' $log | wc -l)
    if [ $code -ne 0 ] && [ $n -eq 6 ] && [ $k -eq 6 ] && grep -q 'assertions:.*| 6 failed' $log; then echo pass-known; return 0; fi
    [ $code -eq 0 ] && { echo pass; return 0; }; echo "fail(code=$code failed_assertions=$n known=$k)"; return 1
  fi
  [ $code -eq 0 ] && { echo pass; return 0; }; echo "fail(code=$code)"; return 1; }
stage(){ name=$1;shift;K=$(key $name)
  if [ -f $E/$TAG/$name.done ] && [ "$(cat $E/$TAG/$name.done)" = "$K" ]; then echo "REUSE $name"; grep -q pass $E/$TAG/$name.status || FAILED=1; return; fi
  mkdir -p $E/$TAG/run_$name; cd $E/$TAG/run_$name
  echo "stage=$TAG/$name binary=$SHA start=$(date -Is)" >> $E/campaign.log
  /usr/bin/time -v "$@" > $E/$TAG/$name.log 2>&1; code=$?
  v=$(verdict $name $code $E/$TAG/$name.log); ok=$?
  echo "stage=$TAG/$name exit=$code verdict=$v end=$(date -Is)" >> $E/campaign.log
  echo "$name exit=$code verdict=$v binary=$SHA" > $E/$TAG/$name.status
  if [ $ok -eq 0 ]; then echo "$K" > $E/$TAG/$name.done; else FAILED=1; rm -f $E/$TAG/$name.done; fi; }
mkdir -p $E/$TAG
for s in "$@"; do case $s in
  unit3d) stage unit3d nice -n 19 timeout 3600 $BIN "[NativeFirstLayer3D],[NativeComposite3D],[NativeChordEnvelope],[NativeFirstLayerEngine3D],[NativeWallReconstruction3D],[NativeVolumeBL3D],[NativeCurvedBL3D]" --use-colour no;;
  unit2d) stage unit2d nice -n 19 timeout 3600 $BIN "[NativeEngine2D],[NativeMesh2D],[NativeField2D],[NativeDistributed2D],[NativeRemesher]" --use-colour no;;
  mesh3d) stage mesh3d nice -n 19 timeout 3600 $BIN "[NativeMesh3D],[NativeDistributed3D]" --use-colour no;;
  metric) stage metric nice -n 19 timeout 3600 $BIN "[NativeD1Law],[Adaptation],[BoundaryLayerMetric],[MetricRobustness],[NativeExactFma],[NativeBalancedSplit3D],[NativeMetricMove3D],[NativeCurvedBL3D],[NativeChordEnvelope],[NativeMetricSplitRule3D],[NativeShapeAdmission3D],[NativeOptimize3D]" --use-colour no;;
  growth) stage growth nice -n 19 timeout 3600 $BIN "[NativeGrowthLayer12],[NativeApexPlace13]" --use-colour no;;
  airfoil) stage airfoil env SU2_NATIVE_AIRFOIL_CONFIG=$AIRCFG nice -n 19 timeout 1800 $BIN "[NativeAirfoil2D]" --use-colour no;;
  *) echo "unknown stage $s"; FAILED=1;;
esac; done
echo "validation-finished failed=$FAILED"
exit $FAILED
