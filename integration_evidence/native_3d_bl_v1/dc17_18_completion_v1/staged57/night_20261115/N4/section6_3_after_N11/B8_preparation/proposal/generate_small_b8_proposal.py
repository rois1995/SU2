#!/usr/bin/env python3
"""Emit a tiny unapplied B8 proposal and prove exact source reversal."""
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
CPP_PATH = "Common/src/adaptation/CNativeEngine3D.cpp"
REV = "ce252"
OLD_BEGIN = "  if(bidirectional)directory->BeginConstructionPass();\n"
OLD_SEED = """  {
    CLocalFailure failure;
    try{selection.SeedFrontier(working);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!context.exchange.Vote(context, failure))return finish();
  }
"""
NEW_BEGIN = "  EngineSetup::BeginConstructionPass(context);\n"
NEW_SEED = """  {
    CLocalFailure failure;
    if (!EngineSetup::SeedFrontier(context, failure)) return finish();
  }
"""


def sha(data: bytes) -> str:
    """Return a stable SHA256 for source evidence."""
    return hashlib.sha256(data).hexdigest()


def read_source() -> bytes:
    """Read only the declared immutable git snapshot."""
    return subprocess.check_output(["git", "show", f"{REV}:{CPP_PATH}"])


def replace_once(source: bytes, old: str, new: str) -> bytes:
    """Apply one exact unique replacement or fail closed."""
    old_bytes = old.encode()
    if source.count(old_bytes) != 1:
        raise ValueError(f"expected one source anchor, got {source.count(old_bytes)}")
    return source.replace(old_bytes, new.encode(), 1)


def main() -> None:
    """Write the small draft and require byte-exact reversal."""
    original = read_source()
    proposed = replace_once(original, OLD_BEGIN, NEW_BEGIN)
    proposed = replace_once(proposed, OLD_SEED, NEW_SEED)
    reversed_source = replace_once(proposed, NEW_SEED, OLD_SEED)
    reversed_source = replace_once(reversed_source, NEW_BEGIN, OLD_BEGIN)
    if reversed_source != original:
        raise AssertionError("caller transformation does not reverse exactly")
    (HERE / "CNativeEngine3D.cpp.proposed").write_bytes(proposed)
    methods = '''// UNAPPLIED B8 methods; include after EngineContext dependencies.\nvoid EngineSetup::BeginConstructionPass(EngineContext& context) {\n  if (context.bidirectional)\n    (*context.exchange.directory)->BeginConstructionPass();\n}\n\nbool EngineSetup::SeedFrontier(EngineContext& context,\n                               CLocalFailure& failure) {\n  try {\n    context.census.selection->SeedFrontier(\n        *context.ownership.working);\n  } catch (const std::exception& e) {\n    failure.Set(2, 0, e.what());\n  }\n  return context.exchange.Vote(context, failure);\n}\n'''
    (HERE / "EngineSetup.cpp.proposed").write_text(methods)
    header = '''// UNAPPLIED declarations; append within EngineSetup.\n  static void BeginConstructionPass(EngineContext&);\n  static bool SeedFrontier(EngineContext&, CLocalFailure&);\n'''
    (HERE / "EngineSetup.hpp.proposed").write_text(header)
    proof = {
        "status": "unapplied proposal; no build or run",
        "source_revision": REV,
        "source_path": CPP_PATH,
        "source_sha256": sha(original),
        "proposed_main_sha256": sha(proposed),
        "reversal_sha256": sha(reversed_source),
        "byte_exact_reversal": reversed_source == original,
        "replacements": [
            {"old": OLD_BEGIN, "new": NEW_BEGIN, "old_count": 1},
            {"old": OLD_SEED, "new": NEW_SEED, "old_count": 1},
        ],
        "scope": "S17 BeginConstructionPass and S18 SeedFrontier only",
        "limitations": [
            "S10 through S15 remain unapplied planning; S13/S15 stay inline pending scope-safe partition.",
            "No proof is claimed for unmodified S10-S15 method extraction.",
        ],
    }
    (HERE / "b8_small_reversal_proof.json").write_text(
        json.dumps(proof, indent=2) + "\n"
    )


if __name__ == "__main__":
    main()
