#!/bin/bash
# DC-12 campaign: the six cut cases and the two production volume-only runs, serially into <root>, resumable per case.
# Usage: run_campaign.sh <binary> <root> [VAR=value ...]
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc12
BIN=$1;ROOT=$2;shift 2
for c in tube_root_x annulus two_tube_y m6_root_te m6_tip_te m6_le volonly:tube_root_x volonly:m6_root_te; do $E/run_case.sh $BIN $c $ROOT "$@"; done
echo CAMPAIGN-DONE
