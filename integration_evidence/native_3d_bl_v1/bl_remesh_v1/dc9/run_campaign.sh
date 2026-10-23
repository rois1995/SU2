#!/bin/bash
# DC-9 campaign: the six cases serially, first the reference limits (V 8192 work / 120 s) into runs/, then the same
# binary with V 8192 work / 600 s into runs_v600/. Resumable per case (harness provenance check). Usage: run_campaign.sh <binary> [ref|v600|both]
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc9
BIN=$1;WHAT=${2:-both}
CASES="annulus tube_root_x two_tube_y m6_le m6_root_te m6_tip_te"
if [ "$WHAT" != v600 ]; then for c in $CASES; do $E/run_case.sh $BIN DC9 $c $E/runs; done; fi
if [ "$WHAT" != ref ]; then for c in $CASES; do $E/run_case.sh $BIN DC9 $c $E/runs_v600 SU2_DC9_V_SECONDS=600; done; fi
echo CAMPAIGN-DONE
