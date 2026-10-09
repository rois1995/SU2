#!/bin/bash
#$ -S /bin/bash
#$ -N NativeSavedAudit
#$ -q aero-ags.q
#$ -pe mpi 1
#$ -cwd
#$ -v PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/bin
#$ -v LD_LIBRARY_PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/lib64
set -euo pipefail
cd "${SGE_O_WORKDIR:?Submit from the candidate repository root}"
[[ $# -eq 2 ]] || { echo "Usage: qsub $0 BASELINE_ClusterResults ASSESSMENT.json" >&2; exit 2; }
output="ClusterResults/saved_metric_audit_${JOB_ID:?}"
launcher="ClusterResults/jobs/${JOB_ID}_saved_audit_launcher"
mkdir -p ClusterResults/jobs
mkdir "$launcher"
exec > >(tee "$launcher/launcher.out") 2> >(tee "$launcher/launcher.err" >&2)
trap 'printf "%s\n" "$?" > "$launcher/exit_code.txt"' EXIT
cp integration_evidence/native_cluster_metric_comparison_v1/RunSavedMetricAuditSGE.sh "$launcher/"
cp integration_evidence/native_cluster_metric_comparison_v1/audit_metric_comparison.py "$launcher/"
export PATH="${PATH_GCC:?}:$PATH"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH_GCC:?}:${LD_LIBRARY_PATH:-}"
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 PYTHONDONTWRITEBYTECODE=1
python3 integration_evidence/native_cluster_metric_comparison_v1/audit_metric_comparison.py \
 "$1" "$PWD/ClusterResults" "$2" "$output"
