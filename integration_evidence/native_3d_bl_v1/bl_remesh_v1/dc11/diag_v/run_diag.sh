#!/bin/bash
# DC-11 V diagnosis runner (serial, nice 19, single thread). usage: run_diag.sh <tag> <case> VAR=value ...  ("[NativeVDiagDC11]" or "[NativeVolumeOnlyDC11]" via TEST=)
set -u
R=/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated
E=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc11/diag_v
T=$1;C=$2;shift 2
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 MALLOC_ARENA_MAX=2 TMPDIR=$R/integration_evidence/build-integrated-v2/compile-tmp
cd $E
echo "tag=$T case=$C env=$* bin=$(cut -c1-16 $E/test_driver_diag.sha256) start=$(date -Is)" >> $E/runs.log
env "$@" SU2_DC11_ROOT=$E SU2_DC11_WP0B=$R/integration_evidence/native_3d_bl_v1/robust_junction_v1/wp0b \
  SU2_DC11_M6=$R/integration_evidence/native_3d_bl_v1/bl_remesh_v1/dc7/m6_cuts SU2_DC11_CASES=$C \
  /usr/bin/time -v nice -n 19 timeout --kill-after=5s 3600s $E/test_driver_diag "$T" --use-colour no > $E/${C}_$(echo $T|tr -dc A-Za-z0-9).log 2>&1
echo "tag=$T case=$C exit=$? end=$(date -Is)" >> $E/runs.log
