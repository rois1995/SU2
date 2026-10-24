#!/bin/bash
# DC-13 campaign, serially into <root>, resumable per case. Usage: run_campaign.sh <binary> <root> <set> [VAR=value ...]
#   set fast  = annulus tube_root_x two_tube_y m6_root_te volonly:tube_root_x (implementation iterations)
#   set final = all six cut cases and both volume-only runs (final binary)
set -u
E=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc13
BIN=$1;ROOT=$2;SET=$3;shift 3
case $SET in
  fast) L="tube_root_x annulus two_tube_y m6_root_te volonly:tube_root_x";;
  final) L="tube_root_x annulus two_tube_y m6_root_te m6_tip_te m6_le volonly:tube_root_x volonly:m6_root_te";;
  *) L="$SET";;
esac
for c in $L; do $E/run_case.sh $BIN $c $ROOT "$@"; done
echo CAMPAIGN-DONE
