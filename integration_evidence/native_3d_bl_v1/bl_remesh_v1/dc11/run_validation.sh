#!/bin/bash
# DC-11 validation: one serial job at a time (nice 19, single thread). Resumable: a stage with a .done marker carrying
# the same binary sha is skipped. Usage: run_validation.sh <binary> <tag> stage...
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc11
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
BIN=$1;TAG=$2;shift 2;SHA=$(sha256sum $BIN | cut -d' ' -f1)
stage(){ name=$1;shift;
  if [ -f $E/$TAG/$name.done ] && grep -q "$SHA" $E/$TAG/$name.done; then echo "REUSE $name";return; fi
  mkdir -p $E/$TAG/run_$name; cd $E/$TAG/run_$name
  echo "stage=$TAG/$name binary=$SHA start=$(date -Is)" >> $E/campaign.log
  /usr/bin/time -v "$@" > $E/$TAG/$name.log 2>&1; code=$?
  echo "stage=$TAG/$name exit=$code end=$(date -Is)" >> $E/campaign.log
  echo "$name exit=$code binary=$SHA" > $E/$TAG/$name.status
  [ $code -eq 0 ] && echo "$SHA" > $E/$TAG/$name.done; }
mkdir -p $E/$TAG
for s in "$@"; do case $s in
  unit3d) stage unit3d nice -n 19 timeout 3600 $BIN "[NativeFirstLayer3D],[NativeComposite3D],[NativeChordEnvelope],[NativeFirstLayerEngine3D],[NativeWallReconstruction3D],[NativeVolumeBL3D],[NativeCurvedBL3D]" --use-colour no;;
  unit2d) stage unit2d nice -n 19 timeout 3600 $BIN "[NativeEngine2D],[NativeMesh2D],[NativeField2D],[NativeDistributed2D],[NativeRemesher]" --use-colour no;;
  mesh3d) stage mesh3d nice -n 19 timeout 3600 $BIN "[NativeMesh3D],[NativeDistributed3D]" --use-colour no;;
  metric) stage metric nice -n 19 timeout 3600 $BIN "[NativeD1Law],[Adaptation],[BoundaryLayerMetric],[MetricRobustness],[NativeExactFma],[NativeBalancedSplit3D],[NativeMetricMove3D],[NativeCurvedBL3D],[NativeChordEnvelope],[NativeMetricSplitRule3D]" --use-colour no;;
  airfoil) stage airfoil env SU2_NATIVE_AIRFOIL_CONFIG=$R/integration_evidence/airfoil_inputs/baseline.cfg nice -n 19 timeout 1800 $BIN "[NativeAirfoil2D]" --use-colour no;;
esac; done
echo campaign-finished
