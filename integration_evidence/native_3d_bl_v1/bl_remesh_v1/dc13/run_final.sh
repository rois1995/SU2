#!/bin/bash
# DC-13 final campaign on one pinned binary (coordinator order 2026-10-24), serial, resumable per case:
# (1) main result: the M6 cuts at x32 with the halving chain to x16 (per-marker h0 multiplier);
# (2) TEST-ONLY cut-slide diagnostic x32->x16 m6_root_te (labelled _DIAG_cutslide, never the main result);
# (3) true-h0 reference: tube_root_x, two_tube_y, annulus, m6_root_te, volume-only tube_root_x and m6_root_te, m6_le,
#     m6_tip_te. (Tubes/annulus are not scaled: their h0 x32 exceeds the BL thickness and the body size.)
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc13
BIN=$1
for c in m6_root_te m6_tip_te m6_le; do $E/run_case.sh $BIN $c $E/runs_x32 SU2_DC13_H0_SCALE=32 SU2_DC13_CHAIN=16; done
$E/run_case.sh $BIN m6_root_te $E/runs_diag SU2_DC13_H0_SCALE=32 SU2_DC13_CHAIN=16 SU2_DC13_CUT_SLIDE=1
for c in tube_root_x two_tube_y annulus volonly:tube_root_x m6_root_te volonly:m6_root_te m6_tip_te m6_le; do $E/run_case.sh $BIN $c $E/runs_x1; done
echo FINAL-DONE
