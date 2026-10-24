#!/bin/bash
# DC-13 a14 (R2 retry off in chain steps): the m6_tip_te x32 -> x16 chain (crashed on a12/a13), then the validation.
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc13
BIN=$1
$E/run_case.sh $BIN m6_tip_te $E/runs_x32_a14 SU2_DC13_H0_SCALE=32 SU2_DC13_CHAIN=16
$E/run_validation.sh $BIN validation_a14 metric growth unit3d unit2d mesh3d airfoil
echo A14-DONE
