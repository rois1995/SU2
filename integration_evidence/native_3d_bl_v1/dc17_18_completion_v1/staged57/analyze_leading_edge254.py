"""Reuse the checked leading-edge diagnosis on the complete corrected-target capture."""
from pathlib import Path
import hashlib
import json
import time

ROOT = Path(__file__).resolve().parent
source = ROOT / "target_delivery253_v1/analyze_leading_edge.py"
output = ROOT / "target_delivery254_v1/leading_edge_diagnosis.json"
assert not output.exists()
code = source.read_text()
changes = [
    ("HERE=Path(__file__).resolve().parent;R=HERE.parent", "HERE=Path(__file__).resolve().parent.parent/'target_delivery254_v1';R=HERE.parent"),
    ("m6_runs/euler_build247_canonical_mach_only_real_cfd50k", "m6_runs/euler_build254_conditioned_mach_only_real_cfd50k"),
    ("metric_target244_mach_only/L1", "metric_target254_conditioned_v1/L1"),
    ("{'q_bad':1148,'j_bad':484,'geometry_bad':1044}", "{'q_bad':251,'j_bad':205,'geometry_bad':95}"),
    ("complete247 cached production", "complete254 cached production"),
]
for old, new in changes:
    assert old in code, old
    code = code.replace(old, new)
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
inputs = [source, Path(__file__).resolve()]
before = {str(p): sha(p) for p in inputs}
started = time.monotonic()
namespace = {"__file__": str(source.resolve()), "__name__": "actual254_le_diagnosis"}
exec(compile(code, str(source), "exec"), namespace)
report = json.loads(output.read_text())
assert before == {str(p): sha(p) for p in inputs}
assert all(sha(Path(p)) == digest for p, digest in report["sha256"].items())
report["reused_analysis"] = {"input_hashes_before": before, "input_hashes_after": before,
                             "transformed_sha256": hashlib.sha256(code.encode()).hexdigest(),
                             "changes": changes, "seconds": time.monotonic() - started,
                             "scope": "Existing captured production lengths only; no new certification, target change or remeshing run."}
output.write_text(json.dumps(report, indent=2) + "\n")
