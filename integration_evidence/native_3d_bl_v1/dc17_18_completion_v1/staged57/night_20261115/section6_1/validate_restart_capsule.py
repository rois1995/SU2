#!/usr/bin/env python3
"""Run the pinned validator with its complete module namespace; never launches CFD."""
import hashlib
import importlib.util
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
VALIDATOR = HERE / "validate_disk_restart522.py"


def main():
    """Correct only the validator loader while preserving every pinned input and assertion."""
    assert len(sys.argv) == 3
    source_case, case = (Path(value).resolve(strict=True) for value in sys.argv[1:])
    assert HERE in source_case.parents and HERE in case.parents
    spec = importlib.util.spec_from_file_location("pinned_restart_validator", VALIDATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.self_check_records()
    module.source_receipts(source_case)
    os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
    os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
    namespace = vars(module)
    assert namespace["os"] is os and callable(namespace["source_receipts_pass"])
    namespace.update(__name__="__main__", __file__=str(VALIDATOR))
    namespace["log"] = (case / "run.log").read_text(errors="replace")
    source = module.retargeted_source()
    exec(compile(source, str(module.BASE_VALIDATOR), "exec"), namespace)
    receipt = {"loader_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "pinned_validator_sha256": hashlib.sha256(VALIDATOR.read_bytes()).hexdigest(),
               "correction": "supply the complete module namespace and run log before the inserted resource proof",
               "numerical_run_repeated": False}
    (case / "validator_loader_receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
