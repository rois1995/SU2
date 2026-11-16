#!/usr/bin/env python3
"""Run the existing serial saved-C492 physical guard under build529 aliases."""
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[2]
REPO = STAGED.parents[3]
BASE = STAGED / "night_20261115" / "section6_2" / "run_remaining_checks.py"
spec = importlib.util.spec_from_file_location("section62_saved_c492", BASE)
checks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checks)
checks.HERE = HERE
checks.STAGED = STAGED
checks.REPO = REPO
original_adapted_source = checks.adapted_source


def adapted_source(mode=None):
    """Retarget the existing runner to this deeper root and fresh build529 names."""
    source = original_adapted_source(mode)
    source = source.replace("HERE=Path(__file__).resolve().parent;",
                            "HERE=Path(__file__).resolve().parents[1]/'N9'/'section6_6';", 1)
    source = source.replace("STAGED=HERE.parents[1]", "STAGED=HERE.parents[2]", 1)
    source = source.replace("build525", "build529").replace("section6_2_", "N9_66_")
    source = source.replace("str(Path(__file__).resolve()):sha(__file__)",
                            "str((HERE/'run_remaining_checks.py').resolve()):sha(HERE/'run_remaining_checks.py')")
    source = source.replace("Path(__file__).resolve()):sha(__file__)",
                            "(HERE/'run_remaining_checks.py').resolve()):sha(HERE/'run_remaining_checks.py')")
    pin_prefix = "str((HERE/'run_remaining_checks.py').resolve()):sha(HERE/'run_remaining_checks.py')"
    anchor = "str((HERE/'build529_aliases.py').resolve()):sha(HERE/'build529_aliases.py')}"
    additions = ("str((HERE/'build529_aliases.py').resolve()):sha(HERE/'build529_aliases.py'),"
                 "str((HERE/'run_g3.py').resolve()):sha(HERE/'run_g3.py'),"
                 "str((HERE/'drafts/check_g3_identity_66.py').resolve()):sha(HERE/'drafts/check_g3_identity_66.py'),"
                 "str((HERE/'unchanged_build499_source_pins.json').resolve()):sha(HERE/'unchanged_build499_source_pins.json'),"
                 "str((HERE.parents[1]/'N4/compare_integer_rows.py').resolve()):sha(HERE.parents[1]/'N4/compare_integer_rows.py'),"
                 "**{str((HERE.parents[1]/'section6_1/cases/section6_1_bl_20261116T082025_3350201'/name).resolve()):"
                 "sha(HERE.parents[1]/'section6_1/cases/section6_1_bl_20261116T082025_3350201'/name) for name in "
                 "('run.log','nose_target_observation.jsonl','actual_saved_surface_stage_rank0.json',"
                 "'cheap_bl_receipt.json','status.json')}}")
    assert source.count(anchor) == 1
    return source.replace(anchor, additions, 1)


def self_check():
    """Compile the adapted existing runner and check the unchanged serial cap."""
    regular = adapted_source()
    cheap_bl = adapted_source("bl")
    compile(regular, str(BASE), "exec")
    compile(cheap_bl, str(BASE), "exec")
    assert "parents[1]/'N9'/'section6_6'" in cheap_bl
    assert "STAGED=HERE.parents[2]" in cheap_bl
    assert "build529_status.json" in cheap_bl and "build529_aliases" in cheap_bl
    assert "case='N9_66_'+mode" in cheap_bl and "19_999_490_048" in cheap_bl
    assert "run_g3.py" in cheap_bl and "check_g3_identity_66.py" in cheap_bl
    assert "check_cheap_bl.py" in cheap_bl
    return {"passed": True, "launched": False, "mode": "bl",
            "base_helper_sha256": checks.sha(BASE)}


def main():
    """Run only the existing cheap-BL gate for the requested saved-C492 check."""
    if sys.argv[1:] == ["--self-check"]:
        print(json.dumps(self_check()))
        return
    assert sys.argv[1:] == ["bl"]
    self_check()
    checks.adapted_source = adapted_source
    checks.main()


if __name__ == "__main__":
    main()
