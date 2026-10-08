#!/bin/bash
#$ -S /bin/bash
#$ -N NativeAdapt
#$ -q aero-ags.q
#$ -pe mpi 4
#$ -cwd
#$ -v PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/bin
#$ -v LD_LIBRARY_PATH_GCC=/home/aero/share/bin/compiler/gcc-10.2.0/lib64
set -euo pipefail
cd "${SGE_O_WORKDIR:?Submit from the repository root}"
export PATH="${PATH_GCC:?}:$PATH"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH_GCC:?}:${LD_LIBRARY_PATH:-}"
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1
kind="${BENCH_KIND:-frozen_euler_to_bl}"
if [[ "$kind" == frozen_* ]]; then
  binary="${SU2_TEST_BIN:-$PWD/build-native/UnitTests/test_driver}"
else
  binary="${SU2_CFD_BIN:-$PWD/build-native/SU2_CFD/src/SU2_CFD}"
fi
machinefile="${MACHINEFILE_PATH:-$PWD/machinefile.${JOB_ID:?}}"
[[ -f "$machinefile" ]] || { echo "Missing scheduler machinefile: $machinefile" >&2; exit 2; }
python3 integration_evidence/run_native_cluster_campaign.py \
  --kind "$kind" --binary "$binary" --machinefile "$machinefile" \
  --ranks "${NSLOTS:?}" --workers "${ADAP_WORKERS:-}" \
  --repeat "${REPEATS:-3}" --events "${ADAPT_EVENTS:-10}"
