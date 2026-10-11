#!/bin/bash
#$ -S /bin/bash
#$ -N Native3D
#$ -q aero-ags.q
#$ -pe mpi 4
#$ -cwd
#$ -v PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/bin
#$ -v LD_LIBRARY_PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/lib64
set -euo pipefail
cd "${SGE_O_WORKDIR:?Submit from repository root}"
bundle=${1:?Frozen package required}
mode=${2:?correctness, lifecycle or timing required}
shift 2
launcher="ClusterResults/launcher/native3d_${JOB_ID:?}"
mkdir -p ClusterResults/launcher
mkdir "$launcher"
exec > >(tee "$launcher/launcher.out") 2> >(tee "$launcher/launcher.err" >&2)
trap 'printf "%s\n" "$?" > "$launcher/exit_code.txt"' EXIT
export PATH="${PATH_GCC:?}:$PATH"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH_GCC:?}:${LD_LIBRARY_PATH:-}"
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 PYTHONDONTWRITEBYTECODE=1
machinefile="${MACHINEFILE_PATH:-$PWD/machinefile.${JOB_ID}}"
[[ -f "$machinefile" ]] || { echo "Missing scheduler machinefile: $machinefile" >&2; exit 2; }
python3 "$bundle/integration_evidence/native_cluster_3d_core_v1/run.py" \
 --package "$bundle" --mode "$mode" --ranks "${NSLOTS:?}" --machinefile "$machinefile" \
 --results "$PWD/ClusterResults/native3d_${JOB_ID}_${mode}_n${NSLOTS}" "$@"
