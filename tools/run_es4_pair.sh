#!/usr/bin/env bash
# Submit generated ES4 manifests from the SGE login node, or run one manifest row in its allocation.
set -euo pipefail
action=${1:?use submit STUDY PHASE or run STUDY PHASE CASE MESH VARIANT LIMIT}
study=${2:?missing study directory}
phase=${3:?use P1, P2, bitwise_r1 or bitwise_r2}

case "$phase" in
  P1|P2) cases=cases_es4 ;;
  bitwise_r1) cases=cases_es4_bitwise_r1 ;;
  bitwise_r2) cases=cases_es4_bitwise_r2 ;;
  *) printf '%s\n' "Unknown phase $phase" >&2; exit 1 ;;
esac

if [[ $action == submit ]]; then
  script=$(realpath "$0")
  mkdir -p "$study/jobs/logs"
  sa_jobs=()
  while read -r case mesh variant slots limit; do
    hold=()
    if [[ $phase == P2 && $case == T4_naca_a10_sst ]]; then
      ((${#sa_jobs[@]} > 0)) || { echo "SA jobs must precede SST in the manifest" >&2; exit 1; }
      hold=(-hold_jid "$(IFS=,; echo "${sa_jobs[*]}")")
    fi
    job=$(qsub -terse -S /bin/bash -cwd -q aero-ag.q -pe smp "$slots" \
      -N "es4_${case%%_*}_${mesh}" -o "$study/jobs/logs/es4_${phase}_${case}_${variant}_${mesh}.log" -j y \
      "${hold[@]}" "$script" run "$study" "$phase" "$case" "$mesh" "$variant" "$limit")
    if [[ $phase == P2 && $case == T4_naca_a10 ]]; then sa_jobs+=("$job"); fi
    printf '%s %s %s %s\n' "$job" "$case" "$mesh" "$variant" | tee -a "$study/jobs/es4_${phase}.jobs"
  done < "$study/jobs/es4_runs_${phase}.txt"
  exit
fi

[[ $action == run ]] || { echo "Unknown action $action" >&2; exit 1; }
case=${4:?} mesh=${5:?} variant=${6:?} limit=${7:?}
set +u
# shellcheck disable=SC1091
source "$study/jobs/env.sh"
set -u
export OMP_NUM_THREADS=1
if [[ -n ${PE_HOSTFILE:-} ]] && [[ $(awk '{print $1}' "$PE_HOSTFILE" | sort -u | wc -l) != 1 ]]; then
  echo "ES4 timing requires a single host" >&2
  exit 1
fi
hostname
base="$study/SimplexBL_Solver/$cases/$case"
names=("$variant" "${variant}_es4")
if [[ $phase == bitwise_* ]]; then names=("b0_$variant" "b1_$variant" "b2_$variant" "b3_$variant"); fi
for name in "${names[@]}"; do
  dir="$base/${name}__${mesh}"
  [[ -f $dir/config.cfg && ! -e $dir/run.log && ! -e $dir/es4_status.csv ]] || {
    echo "Missing config or existing results in $dir" >&2
    exit 1
  }
done

failed=0
for name in "${names[@]}"; do
  cd "$base/${name}__${mesh}"
  tag=$(sed -n '1s/.*binary tag p1v\/\([^ ]*\).*/\1/p' config.cfg)
  [[ $tag == 85-c9 || $tag == 86-es4 ]] || { echo "Unexpected binary tag $tag" >&2; exit 1; }
  bin="$study/bin/$tag/bin/SU2_CFD"
  [[ -x $bin ]] || { echo "Missing binary $bin" >&2; exit 1; }
  sha=$(sha256sum "$bin")
  ranks=${NSLOTS:-1}
  if [[ $phase == bitwise_r1 ]]; then ranks=1; fi
  if [[ $phase == bitwise_r2 ]]; then ranks=2; fi
  start=$(date +%s.%N)
  rc=0
  timeout "$limit" mpirun -n "$ranks" --bind-to none "$bin" config.cfg > run.log 2>&1 || rc=$?
  finish=$(date +%s.%N)
  seconds=$(awk -v a="$start" -v b="$finish" 'BEGIN {printf "%.3f", b-a}')
  printf 'variant,tag,rc,seconds,ranks,job,binary_sha256\n%s,%s,%s,%s,%s,%s,%s\n' \
    "$name" "$tag" "$rc" "$seconds" "$ranks" "${JOB_ID:-manual}" "${sha%% *}" > es4_status.csv
  if ((rc == 0)); then touch DONE; else touch ENDED; failed=1; fi
done
exit "$failed"
