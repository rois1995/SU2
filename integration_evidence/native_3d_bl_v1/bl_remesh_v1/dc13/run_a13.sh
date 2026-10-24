#!/bin/bash
# DC-13 reruns on a13 (fixes after a12: halving registers only column faces, placement starts outside the target
# coverage are no proposal): the x32 -> x16 chains of m6_root_te and m6_tip_te, the TEST-ONLY cut-slide diagnostic,
# and the annulus (the only a12 run with an optimizer exception). Other runs stay a12 evidence.
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc13
BIN=$1
for c in m6_root_te m6_tip_te; do $E/run_case.sh $BIN $c $E/runs_x32_a13 SU2_DC13_H0_SCALE=32 SU2_DC13_CHAIN=16; done
$E/run_case.sh $BIN m6_root_te $E/runs_diag_a13 SU2_DC13_H0_SCALE=32 SU2_DC13_CHAIN=16 SU2_DC13_CUT_SLIDE=1
$E/run_case.sh $BIN annulus $E/runs_x1_a13
echo A13-DONE
