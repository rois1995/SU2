#!/bin/bash
# Production identity (DC-8 harness, unchanged construction): before/after binaries on annulus (natural limits) and
# m6_le / tube_root_x / two_tube_y with a work-limited V (no time limit reached). Usage: run_identity.sh <binary> <tag>
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc9
BIN=$1;T=$2
$E/run_case.sh $BIN DC8 annulus $E/identity/$T
$E/run_case.sh $BIN DC8 m6_le $E/identity/$T SU2_DC8_V_WORK=1500 SU2_DC8_V_SECONDS=1000000
$E/run_case.sh $BIN DC8 tube_root_x $E/identity/$T SU2_DC8_V_WORK=500 SU2_DC8_V_SECONDS=1000000
$E/run_case.sh $BIN DC8 two_tube_y $E/identity/$T SU2_DC8_V_WORK=500 SU2_DC8_V_SECONDS=1000000
echo IDENTITY-DONE
