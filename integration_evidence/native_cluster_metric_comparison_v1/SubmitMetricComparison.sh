#!/bin/bash
# Run on the SGE login host, from a complete candidate checkout.
set -euo pipefail
candidate_root=$(cd "$(dirname "$0")/../.." && pwd -P)
baseline_root=$(cd "${1:?Usage: SubmitMetricComparison.sh BASELINE_CHECKOUT [RANKS=4] [REPEATS=3] [EVENTS=2]}" && pwd -P)
ranks=${2:-4}
repeats=${3:-3}
events=${4:-2}
[[ "$baseline_root" != "$candidate_root" ]] || { echo "Keep baseline and candidate in separate complete checkouts." >&2; exit 2; }
for value in "$ranks" "$repeats" "$events"; do
  [[ "$value" =~ ^[1-9][0-9]*$ ]] || { echo "Ranks, repeats and events must be positive integers." >&2; exit 2; }
done
(( ranks <= 192 )) || { echo "At most 192 ranks are supported by this campaign." >&2; exit 2; }
python3 - "$candidate_root" "$baseline_root" <<'PY'
import hashlib, json, sys
from pathlib import Path
candidate, baseline = map(Path, sys.argv[1:])
pack = 'integration_evidence/native_cluster_campaign_v1'
expected = json.loads((candidate/'integration_evidence/native_cluster_metric_comparison_v1/comparison.json').read_text())
for label, root in [('baseline', baseline), ('candidate', candidate)]:
    pinsfile = root/pack/'source_pins.json'
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    assert sha(pinsfile) == expected[label+'_source_pins_sha256'], label+' source manifest differs'
    pinsfile.resolve().relative_to(root)
    pins = json.loads(pinsfile.read_text())
    for name, digest in pins.items():
        path = root/name
        path.resolve().relative_to(root)
        assert sha(path) == digest, label+' sources differ: '+name
    for name in ['build-native/SU2_CFD/src/SU2_CFD', 'build-native/UnitTests/test_driver']:
        binary = (root/name).resolve()
        binary.relative_to(root)
        assert binary.is_file(), binary
    assert sha(root/pack/'input_manifest.json') == expected['input_manifest_sha256']
    for name, row in json.loads((root/pack/'input_manifest.json').read_text())['files'].items():
        path = root/pack/name
        path.resolve().relative_to(root)
        assert not path.is_symlink() and sha(path) == row['sha256'], path
PY
mkdir -p "$candidate_root/ClusterResults/submissions"
receipt="$candidate_root/ClusterResults/submissions/metric_comparison_$(date +%Y%m%d_%H%M%S)_$$.tsv"
printf 'repeat\tversion\tkind\tranks\tevents\tjob_id\thold_job_id\tcheckout\n' > "$receipt"
previous=${HOLD_JID:-}
for (( rep=1; rep<=repeats; rep++ )); do
  versions=(baseline candidate)
  if (( rep % 2 == 0 )); then versions=(candidate baseline); fi
  for kind in frozen_euler_to_bl frozen_bl_to_euler actual_euler_to_bl actual_bl_to_euler; do
    for version in "${versions[@]}"; do
      checkout=$candidate_root
      if [[ "$version" == baseline ]]; then checkout=$baseline_root; fi
      # Older Bash treats an empty array expansion as unset under nounset.
      qsub_args=(-terse -pe mpi "$ranks" -N "Metric_${version}")
      if [[ -n "$previous" ]]; then qsub_args+=(-hold_jid "$previous"); fi
      variables="BENCH_KIND=$kind,REPEATS=1,ADAPT_EVENTS=$events"
      if [[ -n "${ADAP_WORKERS:-}" ]]; then variables+=",ADAP_WORKERS=$ADAP_WORKERS"; fi
      job=$(cd "$checkout" && qsub "${qsub_args[@]}" \
        -v "$variables" \
        integration_evidence/native_cluster_campaign_v1/RunNativeSGE.sh)
      [[ "$job" =~ ^[0-9]+$ ]] || { echo "Unexpected qsub job ID: $job; preceding jobs remain submitted." >&2; exit 2; }
      printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$rep" "$version" "$kind" "$ranks" "$events" "$job" "$previous" "$checkout" >> "$receipt"
      previous=$job
      printf '%s %s repetition %s: job %s\n' "$version" "$kind" "$rep" "$job"
    done
  done
done
printf 'Submission receipt: %s\n' "$receipt"
