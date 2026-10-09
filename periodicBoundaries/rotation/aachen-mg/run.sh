#!/usr/bin/env bash
set -euo pipefail
case "${1:-}" in
  develop_mg1|pr_mg1|pr_mg2|pr_mg3) case_name=$1 ;;
  *) printf 'Usage: %s {develop_mg1|pr_mg1|pr_mg2|pr_mg3} /absolute/path/to/SU2_CFD\n' "$0" >&2; exit 2 ;;
esac
solver=${2:-}
[[ "$solver" == /* && -x "$solver" && $# == 2 ]] || { printf 'Supply an absolute path to the appropriate executable.\n' >&2; exit 2; }
base=$(cd -- "$(dirname -- "$0")" && pwd)
if [[ ! -d "$base/inputs" ]]; then
  (cd "$base" && sha256sum -c SHA256SUMS)
  tar -xzf "$base/inputs.tar.gz" -C "$base"
fi
(cd "$base/inputs" && sha256sum -c SHA256SUMS)
mkdir -p "$base/runs"
run="$base/runs/$case_name"
mkdir "$run"
cp "$base/configs/$case_name/"*.cfg "$run/"
for name in Aachen_3D_41_blade_coarse.su2 R_Theta_MachRel.eqn solution_flow_0.dat solution_flow_1.dat solution_flow_2.dat; do
  ln -s "../../inputs/$name" "$run/$name"
done
cd "$run"
OMP_NUM_THREADS=1 mpirun -np 8 "$solver" -t 1 aachen_3D_MP_restart.cfg > solver.log 2>&1
