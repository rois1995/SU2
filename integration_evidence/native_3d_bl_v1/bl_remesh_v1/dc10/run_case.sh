#!/bin/bash
# DC-10 runner: one serial case at a time (nice 19, single thread). Usage:
#   run_case.sh <binary> <case> <root> [VAR=value ...]   (extra env, e.g. SU2_DC10_LIFT=single SU2_DC10_V_SECONDS=600)
# Resumable: the harness skips a case whose JSON in <root> carries the same binary sha256, limits/lift and a total time.
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc10
BIN=$1;C=$2;ROOT=$3;shift 3
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
SHA=$(sha256sum $BIN | cut -d' ' -f1)
# runtime inputs of the case (fixture/cut, configuration, reference): their hashes enter the harness resume key
WP=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b;M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts
case $C in
  m6_*) IN="$M6/$C.txt $(awk '$1=="cfg"||$1=="ref"{print $2}' $M6/$C.txt)";;
  annulus) A=$WP/../../mapped_component_geometry_v1/annulus_n1/affine_annulus_bl_budget_20_m1;IN="$WP/annulus_case.txt ${A}_rejected_rank0.native_ref $A.cfg";;
  *) IN="$WP/$C.txt";;
esac
INSHA=$(sha256sum $IN | sha256sum | cut -d' ' -f1)
mkdir -p $ROOT; cd $ROOT
echo "case=$C binary=$SHA inputs=$INSHA ($IN) root=$ROOT env=$* start=$(date -Is)" >> $E/campaign.log
env "$@" SU2_DC10_INPUT_SHA=$INSHA SU2_DC10_BINARY_SHA=$SHA SU2_DC10_ROOT=$ROOT SU2_DC10_WP0B=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b \
  SU2_DC10_M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts SU2_DC10_CASES=$C \
  /usr/bin/time -v nice -n 19 timeout --kill-after=5s 7200s $BIN "[NativeFirstRowDC10]" --use-colour no > $ROOT/$C.log 2>&1
code=$?
echo "case=$C exit=$code end=$(date -Is)" >> $E/campaign.log
exit $code
