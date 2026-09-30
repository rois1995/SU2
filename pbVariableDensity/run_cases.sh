#!/bin/bash
# Runs the cases of su2code/SU2#2939 with develop and with the fix branch.
#
#   DEVELOP=/path/to/develop/SU2_CFD BRANCH=/path/to/fix_pb_variable_density/SU2_CFD ./run_cases.sh
#
# Each run goes to <case>/<run>/ (history.csv, log.txt); plot_issue.py reads them. NP sets the MPI ranks (default 2).
set -u
: "${DEVELOP:?set DEVELOP}"; : "${BRANCH:?set BRANCH}"
NP=${NP:-2}
ROOT=$(cd "$(dirname "$0")" && pwd)

run() {  # run <case> <cfg> <run name> <binary>
  local dir="$ROOT/$1/$3"
  mkdir -p "$dir" && cp "$ROOT/$1/$2" "$dir/run.cfg"
  for f in "$ROOT/$1"/*.su2 "$ROOT/$1"/*.cgns; do [ -e "$f" ] && ln -sf "$f" "$dir/"; done
  echo "$1/$3"
  (cd "$dir" && mpirun -n "$NP" "$4" run.cfg > log.txt 2>&1)
  echo "  exit code $?"
}

run poly_cylinder pb.cfg pb_dev "$DEVELOP"         # pb_poly_cylinder, 2000 it.
run poly_cylinder pb.cfg pb_all "$BRANCH"
run poly_cylinder db.cfg db_dev "$DEVELOP"         # density-based reference (poly_cylinder.cfg)
run poly_cylinder pb_const.cfg pbc_all "$BRANCH"   # same case with constant density
run poly_cylinder db_const.cfg dbc_dev "$DEVELOP"
run bend_inlet pb.cfg pb_dev "$DEVELOP"            # variable density, inlet at 400 K
run bend_inlet pb.cfg pb_all "$BRANCH"
run bend_inlet pb_pout.cfg pout_all "$BRANCH"      # constant density, MARKER_OUTLET= ( OUTLET, 100.0 )
