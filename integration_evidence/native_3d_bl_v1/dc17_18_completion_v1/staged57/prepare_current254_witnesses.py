"""Serialize selected complete terminal254 contexts with the unchanged preparer."""
from pathlib import Path
import hashlib
import json
from prepare_current237_witnesses import prepare

ROOT = Path(__file__).resolve().parent
RUN = ROOT / "m6_runs/euler_build254_conditioned_mach_only_real_cfd50k"
selection = RUN / "bounded_residual_selection.json"
probe = json.loads(selection.read_text())
assert probe["schema"] == "build254_bounded_actual_residual_selection_v2"
assert probe["input_hashes_before"] == probe["input_hashes_after"]
prepare(RUN, selection)
receipt_path = RUN / "current_witnesses_v1/witness_preparation.json"
receipt = json.loads(receipt_path.read_text())
for source in (Path(__file__).resolve(), ROOT / "select_build254_bounded_residuals.py"):
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    receipt["input_hashes_before"][str(source)] = digest
    receipt["input_hashes_after"][str(source)] = digest
receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")
