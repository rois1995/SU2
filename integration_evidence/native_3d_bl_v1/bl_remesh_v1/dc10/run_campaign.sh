#!/bin/bash
# DC-10 campaign: the six cases serially into <root>, resumable per case (harness provenance check).
# Usage: run_campaign.sh <binary> <root> [VAR=value ...]   e.g. SU2_DC10_LIFT=single (baseline) or fan
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc10
BIN=$1;ROOT=$2;shift 2
for c in m6_root_te m6_tip_te annulus m6_le tube_root_x two_tube_y; do $E/run_case.sh $BIN $c $ROOT "$@"; done
echo CAMPAIGN-DONE
