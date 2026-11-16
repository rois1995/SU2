#!/usr/bin/env python3
"""N11 build535 binding for the accepted N8 G2/GQ case validator."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parent
STAGED = N11.parents[1]
REPO = STAGED.parents[3]
NIGHT = STAGED / "night_20261115"
N8 = NIGHT / "N8"
A2 = N8 / "A2_A3_N12/section6_8"
N10 = NIGHT / "N10"
BUILD = N11 / "build535"
BASE = A2 / "validation/validate_g2_gq.py"
TIMER_HELPER = N11 / "section6_8_preparation/drafts/compare_timer_census_delta_69.py"

source = BASE.read_text()
assert "build534" in source
old_roots = ("HERE = Path(__file__).resolve().parent\nA2 = HERE.parent\nSTAGED = A2.parents[3]\n"
             "NIGHT = A2.parent.parent.parent\nREPO = A2.parents[7]\nN1 = NIGHT / \"section6_1\"\n"
             "N1_FULL = N1 / \"full_workflow\"\nN1_COLLAPSE = N1 / \"first_collapse\"\n"
             "BUILD = A2 / \"build534\"\nCOMPARATOR = NIGHT / \"N4/compare_integer_rows.py\"\n"
             "COMPARATOR68 = HERE.parent / \"drafts/compare_integer_rows_68.py\"")
new_roots = ("HERE = Path(__file__).resolve().parent\nN11 = HERE.parent\nSTAGED = N11.parents[1]\n"
             "REPO = STAGED.parents[3]\nNIGHT = STAGED / \"night_20261115\"\nN8 = NIGHT / \"N8\"\n"
             "A2 = N8 / \"A2_A3_N12/section6_8\"\nN1 = NIGHT / \"section6_1\"\n"
             "N1_FULL = N1 / \"full_workflow\"\nN1_COLLAPSE = N1 / \"first_collapse\"\n"
             "BUILD = N11 / \"build535\"\nCOMPARATOR = NIGHT / \"N4/compare_integer_rows.py\"\n"
             "COMPARATOR68 = TIMER_HELPER")
assert source.count(old_roots) == 1
source = source.replace(old_roots, new_roots, 1).replace("build534", "build535")
exec(compile(source, str(BASE), "exec"), globals())
N1 = NIGHT / "section6_1"
N1_FULL = N1 / "full_workflow"
N1_COLLAPSE = N1 / "first_collapse"
BUILD = N11 / "build535"
COMPARATOR68 = TIMER_HELPER
