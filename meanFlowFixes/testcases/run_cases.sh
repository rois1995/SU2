#!/bin/bash
# Runs the test cases of PR #2941 with two SU2_CFD builds.
#
#   DEVELOP=/path/to/develop/SU2_CFD BRANCH=/path/to/meanFlowFixes/SU2_CFD ./run_cases.sh [case_folder ...]
#
# Each config <case>/<name>.cfg is run in <case>/develop/<name>/ and <case>/meanFlowFixes/<name>/, and the
# fixed-grid reference of the HLLC case (2_hllc_translating/fixed.cfg) only with develop, as in the PR plots.
# NP sets the number of MPI ranks (default 2). Without arguments all the cases are run.
# The long cases (1b, 1c, 2) run up to 20 000 iterations.

set -u
: "${DEVELOP:?set DEVELOP to the SU2_CFD of develop}"
: "${BRANCH:?set BRANCH to the SU2_CFD of meanFlowFixes}"
NP=${NP:-2}
ROOT=$(cd "$(dirname "$0")" && pwd)

cases=("$@")
[ ${#cases[@]} -eq 0 ] && cases=(1a_nozzle_restart 1b_nozzle_from_rest 1c_rae2822 2_hllc_translating 3_inc_pitching \
                                 4a_sources 4b_turbo_riemann 4c_mglevel)

run() {  # run <case> <cfg name> <variant> <binary>
  local dir="$ROOT/$1/$3/$2"
  mkdir -p "$dir"
  cp "$ROOT/$1/$2.cfg" "$dir/"
  # meshes and restart files are linked, not copied
  for f in "$ROOT/$1"/*.su2 "$ROOT/$1"/*.dat; do [ -e "$f" ] && ln -sf "$f" "$dir/"; done
  echo "$1/$3/$2"
  (cd "$dir" && mpirun -n "$NP" "$4" "$2.cfg" > log.txt 2>&1)
  echo "  exit code $? (log: $1/$3/$2/log.txt)"
}

for c in "${cases[@]}"; do
  c=${c%/}
  for cfg in "$ROOT/$c"/*.cfg; do
    name=$(basename "$cfg" .cfg)
    run "$c" "$name" develop "$DEVELOP"
    [ "$c/$name" = 2_hllc_translating/fixed ] && continue
    run "$c" "$name" meanFlowFixes "$BRANCH"
  done
done
