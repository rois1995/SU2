#!/bin/bash
# Standalone DC-18 checks; each passing case is reusable only with matching provenance and output hash.
# Usage: bash run_tests.sh [output directory]. Original preparation evidence is never overwritten.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
out="${1:-$here/run_$(date +%Y%m%d_%H%M%S)}"
mkdir -p "$out"
out="$(cd "$out" && pwd)"
export TMPDIR="$root/integration_evidence/build-integrated-v2/compile-tmp"
export OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1
cd "$out"
{
  sha256sum "$root/Common/include/adaptation/CNativeCurvedReference.hpp" "$here/test_curved_reference.cpp" "$here/run_tests.sh"
  g++ --version
  echo 'flags -O2 -std=c++17 -Wall; threads=1'
} > sources.current
if ! cmp -s sources.current sources.built || ! sha256sum -c binary.sha256 >/dev/null 2>&1; then
  /usr/bin/time -v -o build.resources nice -n 19 g++ -O2 -std=c++17 -Wall -I "$root/Common/include" \
    "$here/test_curved_reference.cpp" -o test_curved_reference > build.log 2>&1
  cp sources.current sources.built
  sha256sum test_curved_reference > binary.sha256
fi
{
  date -Is; uptime; free -h
  git -C "$root" rev-parse HEAD
  cat sources.built binary.sha256
  ldd ./test_curved_reference | awk '/=> \// {print $3} /^\s*\// {print $1}' | xargs -r sha256sum
} > provenance.txt
# Exclude timestamps and host load from reuse identity, retain sources, binary and shared-library contents.
keybase=$( (cat sources.built binary.sha256; ldd ./test_curved_reference | awk '/=> \// {print $3}' | xargs -r sha256sum) | sha256sum)
status=0
run() {
  local name=$1; shift
  local key code=0
  key=$( { printf '%s\n' "$keybase" "$@"; for arg in "$@"; do [ ! -f "$arg" ] || sha256sum "$arg"; done; } | sha256sum)
  if [ -f "$name.done" ] && [ "$(cat "$name.done")" = "$key" ] && sha256sum -c "$name.sha256" >/dev/null 2>&1; then
    echo "REUSE $name"; return
  fi
  rm -f "$name.done"
  /usr/bin/time -v -o "$name.resources" timeout --kill-after=10s 1800 nice -n 19 ./test_curved_reference "$@" > "result_$name.txt" 2>&1 || code=$?
  if [ "$code" -eq 0 ] && grep -q ': 0 failure(s)' "result_$name.txt"; then
    sha256sum "result_$name.txt" > "$name.sha256"
    printf '%s\n' "$key" > "$name.done"
  else status=1; fi
  echo "$name exit $code" | tee -a provenance.txt
}
run contracts contracts
run m6 m6 "$root/integration_evidence/native_3d_bl_v1/oneram6_frozen_v1/Coarse.su2"
run sphere sphere
run cylinder cylinder
run multibody multibody
run crease crease
run rae2822_original airfoil "$root/integration_evidence/rae2822_transonic_v1/mesh_RAE2822_turb.su2" 1e-6 45 AIRFOIL
run rae2822_frozen_adapted airfoil "$root/integration_evidence/native_frozen_rae_bl_to_euler_hash_v1/airfoil_input.su2" 1e-6 45 AIRFOIL
run 30p30n airfoil "$root/integration_evidence/native_3d_bl_v1/three_element_input_v1/30P30N_coarse_tri.su2" 1e-6 45 Boundary004 Boundary005 Boundary006
run 30p30n_gap gap "$root/integration_evidence/native_3d_bl_v1/three_element_input_v1/30P30N_coarse_tri.su2" 45 Boundary004 Boundary005 Boundary006
# Reject a campaign whose pinned source changed during execution.
sha256sum -c <(head -3 sources.built) > source_check.txt || status=1
printf 'complete exit=%s\n' "$status" > campaign.status
exit "$status"
