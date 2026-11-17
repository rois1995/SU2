#!/usr/bin/env python3
"""Stable import interface for build552 prefix validation."""
import importlib.util
import json
import sys
from pathlib import Path

_PATH = Path(__file__).with_name("validate_g2_gq_552.py")
_SPEC = importlib.util.spec_from_file_location("n14_validate_g2_gq_552_impl", _PATH)
if _SPEC is None or _SPEC.loader is None:
    raise RuntimeError(f"cannot load validator {_PATH}")
_IMPL = importlib.util.module_from_spec(_SPEC)
sys.modules[_SPEC.name] = _IMPL
_SPEC.loader.exec_module(_IMPL)
BUILD = _IMPL.BUILD
COMPARATOR68 = _IMPL.TIMER_COMPARATOR
N1_COLLAPSE = _IMPL.reference_case("G2")
N1_FULL = _IMPL.reference_case("GQ")

def read_json(path):
    return json.loads(Path(path).read_text())

def build552():
    source=_IMPL.NIGHT / "N8/A2_A3_N12/section6_8/validation/validate_g2_gq.py"
    spec=importlib.util.spec_from_file_location("n14_existing_build_verifier",source)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.BUILD=BUILD
    return module.build534()

def validate(gate,case):
    return _IMPL.validate_case(case,gate)
self_check = _IMPL.self_check
