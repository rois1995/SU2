#!/usr/bin/env python3
"""Install only candidate-local links to the already accepted N14 references."""
from pathlib import Path
import hashlib
import json

P = Path(__file__).resolve().parents[2]
HERE = P / "B8/validation"
NIGHT = P.parent.parent
N14 = NIGHT / "N14/section6_12/validation"
B3 = P / "B3/validation"
N10 = NIGHT / "N10"


def link_exact(target, source):
    """Create an absent symlink or verify that an existing link has the same target."""
    assert source.exists(), source
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.is_symlink():
        assert target.resolve(strict=True) == source.resolve(strict=True), target
    else:
        assert not target.exists(), target
        target.symlink_to(source.resolve(strict=True), target_is_directory=source.is_dir())


def main():
    """Create expected reference and helper aliases without modifying their sources."""
    (HERE / "cases").mkdir(parents=True, exist_ok=True)
    draft = Path(__file__).resolve().parent
    helper_names = ["run_g1_558.py", "run_g2_gq_prefix_558.py", "validate_g2_gq.py",
                    "prelaunch_housepin_stub.py", "prelaunch_prefix_stub.py"]
    for name in helper_names:
        source = draft / name
        target = HERE / name
        if target.exists():
            assert hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256(source.read_bytes()).digest()
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.read_bytes())
    reference = HERE / "references"
    sources = {
        "G1_shared_reference_manifest.json": B3 / "references/G1_shared_reference_manifest.json",
        "G1_actual_selection.json": N10 / "G1_actual_selection.json",
        "G2_reference": N14 / "cases/N14_G2_552",
        "GQ_reference": N14 / "cases/N14_GQ_552",
    }
    sources.update({f"G1_candidate_{cohort}.json": B3 / f"references/G1_candidate_{cohort}.json"
                    for cohort in ("serial", "mpi2", "mpi4")})
    for name, source in sources.items():
        link_exact(reference / name, source)
    link_exact(HERE / "run_g2_gq_prefix.py", HERE / "run_g2_gq_prefix_558.py")
    alias_source = draft / "build558_aliases.py"
    alias_target = P / "build558_aliases.py"
    if alias_target.exists():
        assert hashlib.sha256(alias_target.read_bytes()).digest() == hashlib.sha256(alias_source.read_bytes()).digest()
    else:
        alias_target.write_bytes(alias_source.read_bytes())
    report = {"installed": True, "references": {name: str(source.resolve()) for name, source in sources.items()},
              "candidate_prefix_alias": str((HERE / "run_g2_gq_prefix.py").resolve()),
              "build_alias": str(alias_target.resolve()), "source_files_unchanged": True}
    (HERE / "setup_aliases_receipt.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
