#!/usr/bin/env python3
"""Bind the completed restart log before the inherited resource proof reads it."""
import importlib.util
import os
from pathlib import Path
HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("frozen_restart_validator545", HERE / "validate_disk_restart545.py")
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)
source_case = HERE.parent / "full_workflow"
case = HERE / "disk_restart_event2"
validator.source_receipts(source_case)
module = validator.load_base()
os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
source = module.retargeted_source()
exec(compile(source, str(validator.BASE), "exec"),
     {"__name__": "__main__", "__file__": str(HERE / "validate_disk_restart545.py"),
      "source_receipts_pass": validator.source_receipts_pass,
      "conservative_time_bound": module.conservative_time_bound,
      "log": (case / "run.log").read_text()})
