#!/usr/bin/env python3
"""Reuse the accepted safeguards runner for N9's build527 cheap-BL check."""
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
BASE = STAGED / "night_20261115" / "section6_2" / "run_remaining_checks.py"
spec = importlib.util.spec_from_file_location("section62_safeguards", BASE)
checks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checks)
checks.HERE = HERE
checks.STAGED = STAGED
checks.REPO = STAGED.parents[3]
original_adapted_source = checks.adapted_source


def adapted_source(mode=None):
    """Retarget only the build aliases and per-case names to this N9 folder."""
    source = original_adapted_source(mode)
    source = source.replace("HERE=Path(__file__).resolve().parent;",
                            "HERE=Path(__file__).resolve().parents[1]/'N9';", 1)
    source = source.replace("build525", "build527").replace("section6_2_", "N9_")
    source = source.replace("str(Path(__file__).resolve()):sha(__file__)",
                            "str((HERE/'run_remaining_checks.py').resolve()):sha(HERE/'run_remaining_checks.py')")
    source = source.replace("Path(__file__).resolve()):sha(__file__)",
                            "(HERE/'run_remaining_checks.py').resolve()):sha(HERE/'run_remaining_checks.py')")
    source = source.replace(
        "str((HERE/'build527_aliases.py').resolve()):sha(HERE/'build527_aliases.py')",
        "str((HERE/'build527_aliases.py').resolve()):sha(HERE/'build527_aliases.py'),"
        "str((HERE/'run_g3.py').resolve()):sha(HERE/'run_g3.py')")
    return source


def self_check():
    """Check the runner targets build527 and the original cheap-BL gate."""
    regular = adapted_source()
    cheap_bl = adapted_source("bl")
    compile(regular, str(BASE), "exec")
    compile(cheap_bl, str(BASE), "exec")
    assert "build527_status.json" in regular
    assert "build527_aliases" in regular
    assert "parents[1]/'N9'" in regular
    assert "run_remaining_checks.py" in regular
    assert "run_g3.py" in regular and "run_g3.py" in cheap_bl
    assert "19_999_490_048" in cheap_bl
    assert "check_cheap_bl.py" in cheap_bl
    assert "20_000_000_000" in cheap_bl
    return {"passed": True, "launched": False, "mode": "bl",
            "base_helper_sha256": checks.sha(BASE)}


def main():
    """Run only the selected existing safeguards mode through the retargeted helper."""
    if sys.argv[1:] == ["--self-check"]:
        print(json.dumps(self_check()))
        return
    assert len(sys.argv) == 2 and sys.argv[1] == "bl"
    self_check()
    checks.adapted_source = adapted_source
    checks.main()


if __name__ == "__main__":
    main()
