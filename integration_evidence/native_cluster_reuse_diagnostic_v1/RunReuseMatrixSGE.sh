#!/bin/bash
#$ -S /bin/bash
#$ -N NativeReuseMat
#$ -q aero-ags.q
#$ -pe mpi 4
#$ -cwd
#$ -v PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/bin
#$ -v LD_LIBRARY_PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/lib64
set -euo pipefail
cd "${SGE_O_WORKDIR:?Submit from the NativeIntegrated repository root}"
launcher="ClusterResults/jobs/${JOB_ID:?}_reuse_matrix_launcher"
mkdir -p ClusterResults/jobs
mkdir "$launcher"
exec > >(tee "$launcher/launcher.out") 2> >(tee "$launcher/launcher.err" >&2)
trap 'printf "%s\n" "$?" > "$launcher/exit_code.txt"' EXIT
cp integration_evidence/native_cluster_reuse_diagnostic_v1/RunReuseMatrixSGE.sh "$launcher/"
export PATH="${PATH_GCC:?}:$PATH"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH_GCC:?}:${LD_LIBRARY_PATH:-}"
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 PYTHONDONTWRITEBYTECODE=1
machinefile="${MACHINEFILE_PATH:-$PWD/machinefile.$JOB_ID}"
python3 integration_evidence/native_cluster_reuse_diagnostic_v1/matrix_run.py \
  --machinefile "$machinefile" --repeat "${REPEATS:-1}" --timeout "${CASE_TIMEOUT:-7200}"
