#!/bin/bash
#$ -S /bin/bash
#$ -N NativePredictCheck
#$ -q aero-ags.q
#$ -pe mpi 4
#$ -cwd
#$ -v PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/bin
#$ -v LD_LIBRARY_PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/lib64
set -euo pipefail
cd "${SGE_O_WORKDIR:?Submit from the repository root}"
predict_launcher="ClusterResults/jobs/${JOB_ID:?}_predict_launcher"
mkdir -p ClusterResults/jobs
mkdir "$predict_launcher"
exec > >(tee "$predict_launcher/launcher.out") 2> >(tee "$predict_launcher/launcher.err" >&2)
trap 'printf "%s\n" "$?" > "$predict_launcher/exit_code.txt"' EXIT
cp integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh "$predict_launcher/RunPredictCorrectnessSGE.sh"
export PATH="${PATH_GCC:?}:$PATH"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH_GCC:?}:${LD_LIBRARY_PATH:-}"
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1
machinefile="${MACHINEFILE_PATH:-$PWD/machinefile.${JOB_ID}}"
[[ -f "$machinefile" ]] || { echo "Missing scheduler machinefile: $machinefile" >&2; exit 2; }
python3 integration_evidence/native_post_rebase_metric_v1/complete_predict_history_validation.py \
 --binary "${SU2_CFD_BIN:-$PWD/build-native/SU2_CFD/src/SU2_CFD}" \
 --test-binary "${SU2_TEST_BIN:-$PWD/build-native/UnitTests/test_driver}" \
 --machinefile "$machinefile"
