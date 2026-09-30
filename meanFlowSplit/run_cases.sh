#!/bin/bash
# Runs the test cases of one PR with the build before the PR and the build of the PR.
#
#   BEFORE=/path/SU2_CFD AFTER=/path/SU2_CFD ./run_cases.sh pr1|pr2|pr4
#
# pr1: BEFORE = develop,            AFTER = the under-relaxation PR
# pr2: BEFORE = the under-relaxation PR, AFTER = the HLLC PR
# pr4: BEFORE = the Roe PR,         AFTER = the small-fixes PR
# Results go to <pr>/<case>/<run>/ (history.csv, log.txt); plot_split.py reads them. NP = MPI ranks (default 2).
set -u
pr=$1; : "${BEFORE:?}"; : "${AFTER:?}"; NP=${NP:-2}
ROOT=$(cd "$(dirname "$0")" && pwd)
case $pr in pr1) b=develop; a=pr1;; pr2) b=pr1; a=pr2;; pr4) b=pr3; a=pr4;; *) echo "pr1, pr2 or pr4"; exit 1;; esac
run() {  # run <case> <cfg> <run name> <binary>
  local dir="$ROOT/$pr/$1/$3"; mkdir -p "$dir" && cp "$ROOT/$pr/$1/$2" "$dir/run.cfg"
  for f in "$ROOT/$pr/$1"/*.su2 "$ROOT/$pr/$1"/*.dat; do [ -e "$f" ] && ln -sf "$f" "$dir/"; done
  echo "$pr/$1/$3"; (cd "$dir" && mpirun -n "$NP" "$4" run.cfg > log.txt 2>&1); echo "  exit code $?"
}
case $pr in
  pr1) for c in nozzle_restart nozzle_from_rest; do run $c case.cfg $b "$BEFORE"; run $c case.cfg $a "$AFTER"; done;;
  pr2) run hllc_translating moving.cfg $b "$BEFORE"; run hllc_translating moving.cfg $a "$AFTER"
       run hllc_translating fixed.cfg fixed "$AFTER";;
  pr4) run inc_pitching case.cfg $b "$BEFORE"; run inc_pitching case.cfg $a "$AFTER"
       for c in body50_gravity body100 body59.81; do run sources $c.cfg ${b}_$c "$BEFORE"; done
       run sources body50_gravity.cfg ${a}_body50_gravity "$AFTER"
       for c in turbo_riemann mglevel; do run $c case.cfg $b "$BEFORE"; run $c case.cfg $a "$AFTER"; done;;
esac
