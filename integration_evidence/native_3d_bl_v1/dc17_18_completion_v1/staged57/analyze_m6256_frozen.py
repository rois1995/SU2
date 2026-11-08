"""Reuse complete-census counters for the frozen256 attempt and same-target254 comparison."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
source = ROOT / "analyze_m6254_frozen.py"
code = source.read_text()
changes = [
    ("a=r/'m6_runs/euler_build254_conditioned_mach_only_real_cfd50k'", "a=r/'m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k'"),
    ("m6_runs/euler_build253_witness_cutoff_mach_only_real_cfd50k/quality_progress.json", "m6_runs/euler_build254_conditioned_mach_only_real_cfd50k/quality_progress.json"),
    ("Replace BOTH literal244 attempt paths with254", "Replace BOTH literal244 attempt paths with256"),
    ("live_quality254_analysis_provenance.json", "live_quality256_analysis_provenance.json"),
    ("round254", "round256"), ("round253", "round254"),
    ("quality_seconds254", "quality_seconds256"), ("quality_seconds253", "quality_seconds254"),
    ("gap254_minus253", "gap256_minus254"),
    ("current254", "current256"), ("previous253", "previous254"),
    ("quality_comparison253.json", "quality_comparison254.json"),
    ("Corrected target changes workload;no same-query or isolated-speedup claim.",
     "Initial CFD fields,target tensors,configuration and host cache quota match254;repair decisions change workload. No same-query or isolated-speedup claim under host contention."),
]
for old, new in changes:
    assert code.count(old) == 1, old
    code = code.replace(old, new)
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
before = {str(p): sha(p) for p in (source, Path(__file__).resolve())}
exec(compile(code, str(source), "exec"), {"__file__": str(source.resolve()), "__name__": "frozen256_counter_analysis"})
assert before == {p: sha(Path(p)) for p in before}
attempt = ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k"
receipt = {"input_hashes_before": before, "input_hashes_after": before,
           "transformed_sha256": hashlib.sha256(code.encode()).hexdigest(), "changes": changes,
           "scope": "Read-only existing first-event counters;whole-field counts require complete flags. Nearest complete quality times are measured,not interpolated;later phase transitions remain separate. No acceptance or timing causality claim."}
(attempt / "frozen256_counter_reuse_provenance.json").write_text(json.dumps(receipt, indent=2) + "\n")
