#!/bin/bash
# DC-15 c3: cut regressions (root_te, tip_te at x32), then the validation suites, one serial job at a time.
P=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc15
export TMPDIR=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/build-integrated-v2/compile-tmp
$P/run_harness.sh $P/bin/test_driver_c3 m6_root_te $P/cuts_c3 SU2_DC13_H0_SCALE=32
$P/run_harness.sh $P/bin/test_driver_c3 m6_tip_te $P/cuts_c3 SU2_DC13_H0_SCALE=32
$P/run_validation.sh $P/bin/test_driver_c3 validation_c3 pipeline growth unit3d unit2d metric mesh3d airfoil
echo ALL DONE
