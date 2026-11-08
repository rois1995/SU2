"""Reuse complete-census counters for the frozen260 attempt and same-target256 comparison."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parent
source = ROOT / "analyze_m6254_frozen.py"
code = source.read_text()
changes = [
    ("a=r/'m6_runs/euler_build254_conditioned_mach_only_real_cfd50k'", "a=r/'m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k'"),
    ("m6_runs/euler_build253_witness_cutoff_mach_only_real_cfd50k/quality_progress.json", "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k/quality_progress.json"),
    ("Replace BOTH literal244 attempt paths with254", "Replace BOTH literal244 attempt paths with260"),
    ("live_quality254_analysis_provenance.json", "live_quality260_analysis_provenance.json"),
    ("round254", "round260"), ("round253", "round256"),
    ("quality_seconds254", "quality_seconds260"), ("quality_seconds253", "quality_seconds256"),
    ("gap254_minus253", "gap260_minus256"),
    ("current254", "current260"), ("previous253", "previous256"),
    ("quality_comparison253.json", "quality_comparison256.json"),
    ("Corrected target changes workload;no same-query or isolated-speedup claim.",
     "Initial CFD fields,target tensors,configuration and host cache quota match256;late geometry-priority decisions change workload. No same-query or isolated-speedup claim under host contention."),
]
for old, new in changes:
    assert code.count(old) == 1, old
    code = code.replace(old, new)
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
before = {str(p): sha(p) for p in (source, Path(__file__).resolve())}
exec(compile(code, str(source), "exec"), {"__file__": str(source.resolve()), "__name__": "frozen260_counter_analysis"})
assert before == {p: sha(Path(p)) for p in before}
attempt = ROOT / "m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
receipt = {"input_hashes_before": before, "input_hashes_after": before,
           "transformed_sha256": hashlib.sha256(code.encode()).hexdigest(), "changes": changes,
           "scope": "Read-only existing first-event counters;whole-field counts require complete flags. Nearest complete quality times are measured,not interpolated;later phase transitions remain separate. No acceptance or timing causality claim."}
(attempt / "frozen260_counter_reuse_provenance.json").write_text(json.dumps(receipt, indent=2) + "\n")
