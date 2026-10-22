#!/bin/bash
# WP-A (D1) + WP-B/C deferred items: focused validation on ONE binary, one serial job at a time (nice 19, single
# thread). Resumable: a stage whose .done marker holds the current binary sha is skipped.
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
B=$R/integration_evidence/build-integrated-v2
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/wpa_m1
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$B/compile-tmp
BIN=$B/UnitTests/test_driver
SHA=$(sha256sum $BIN | cut -d' ' -f1)
stage(){ name=$1;shift;
  if [ -f $E/$name.done ] && grep -q "$SHA" $E/$name.done; then echo "REUSE $name";return; fi
  mkdir -p $E/run_$name; cd $E/run_$name
  /usr/bin/time -v "$@" > $E/$name.log 2>&1; code=$?
  echo "$name exit=$code binary=$SHA" | tee $E/$name.status
  [ $code -eq 0 ] && echo "$SHA" > $E/$name.done; }
stage d1law nice -n 19 timeout 3600 $BIN "[NativeD1Law],[NativeReferenceIO],[NativeReference3D],[NativeMoveConstraint3D]" --use-colour no
stage bl2d nice -n 19 timeout 3600 $BIN "[Adaptation],[BoundaryLayerMetric],[NativeComposite2D],[MetricRobustness],[NativeBL2D],[NativeProducedBL2D],[NativeBLBudgetProfile],[NativeBLBudgetPolicy]" --use-colour no
stage unit3d nice -n 19 timeout 3600 $BIN "[NativeFirstLayer3D],[NativeComposite3D],[NativeChordEnvelope],[NativeFirstLayerEngine3D],[NativeWallReconstruction3D],[NativeVolumeBL3D],[NativeCurvedBL3D]" --use-colour no
stage unit2d nice -n 19 timeout 3600 $BIN "[NativeEngine2D],[NativeMesh2D],[NativeField2D],[NativeDistributed2D],[NativeRemesher]" --use-colour no
stage mesh3d nice -n 19 timeout 3600 $BIN "[NativeMesh3D],[NativeDistributed3D]" --use-colour no
stage airfoil env SU2_NATIVE_AIRFOIL_CONFIG=$R/integration_evidence/airfoil_inputs/baseline.cfg nice -n 19 timeout 1800 $BIN "[NativeAirfoil2D]" --use-colour no
# Serial M6 is refused by the unchanged 2 GiB donor-import admission (before any D1 data); one 2-rank MPI job.
stage m6_d1_500k_np2 env SU2_NATIVE_FROZEN_CONFIG=$E/run_m6_d1_500k_np2/run.cfg taskset -c 0-1 nice -n 19 timeout --kill-after=5s 5400s mpirun -np 2 --bind-to none $BIN "[NativeSavedMetric3D]" --abort --reporter compact
echo campaign-finished
