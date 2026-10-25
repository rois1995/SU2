#!/bin/bash
# DC-16 c12: validation suites, the new DC-16 tests, then the cut references (tip_te x32, tubes, annulus); one serial job at a time.
P=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc16
export TMPDIR=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/build-integrated-v2/compile-tmp OMP_NUM_THREADS=1 MALLOC_ARENA_MAX=2
B=$P/bin/test_driver_c12
mkdir -p $P/validation_c12
( cd $P/validation_c12 && nice -n 19 $B "[NativeDeform16],[NativeRoom16]" --use-colour no > deform.log 2>&1; echo "deform exit=$?" > deform.status )
$P/run_validation.sh $B validation_c12 pipeline growth unit3d unit2d metric mesh3d airfoil
$P/run_harness.sh $B m6_tip_te $P/cuts_c12 SU2_DC13_H0_SCALE=32
$P/run_harness.sh $B m6_root_te $P/cuts_c12_prod SU2_DC13_H0_SCALE=32 SU2_DC16_VALIDITY_ONLY=1 SU2_DC16_PRODUCTION_V=1
echo ALL DONE
