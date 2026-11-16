# Section 6.1 harness retargeting

The unit and safeguard runners use the existing P7/P8 case definitions and checks,
but read the new `build522` receipts and write attempts under this folder. The
shared `staged57/run_cases.py` root is rebound to this folder at runtime. The
`build522_aliases.py` helper exposes the verified build receipts in the legacy
`build522_status.json`, `build522_source_pins.json`, and
`build522_terminal_handles.json` names expected by `run_cases.py` and the
481/current-target checkers. The aliases derive from the actual terminal receipts;
they do not synthesize process completion.

Run after build522 is terminal and pinned, in this order:

```sh
python3 run_units.py serial
python3 run_units.py mpi2
python3 run_units.py stats
python3 run_first_collapse_section6_1.py
python3 run_remaining_checks.py bl
python3 run_final_checks.py
```

`run_first_collapse_section6_1.py` reuses the reviewed full-controller loader and
enforces exact G2 work `90809869` plus the six expected rank counts. It runs only
the collapse prefix. `run_remaining_checks.py` is a checked adapter to the original
P7/P8 helper: `--self-check` validates all source substitutions without reading the
build receipt or launching a case. The BL mode loads the original `check_cheap_bl.py`
from its P7/P8 directory so its `__file__`-relative inputs keep their original
meaning, while enforcing the strict `19999490048` byte ceiling. Merit and
saved-target helpers keep their shared staged input root while
their attempt scans and outputs point to `section6_1/cases`.

`run_final_checks.py` uses the existing stop-on-first-failure order:
`merit481`, `saved_current`, `2d1`, `2d2`, `2d4`. It writes a per-mode launch and
terminal receipt, samples host CPU/load/free RAM before each mode, and stops when
another SU2/build/helper process is active. The CPU preflight records every visible
PID/start identity and its three-second per-process CPU percentage, and treats
non-controller processes above 50% of one core as competing work. Each mode runs in
an isolated subprocess. The RSS bound reads the case's `run.log`, not the controller
log. Its cap is the exact 19073 MiB value, `19999490048` bytes. The `bl` mode remains
the saved C492 safeguard. No physical or resource gate was relaxed.

The first-collapse wrapper imports the primary-reviewed full-controller loader,
which rebinds the purpose, staged root, repository root, build, and receipt paths.
Its output is `first_collapse/` under this folder. The event/timer helpers and
unchanged-build pin manifest are symlinks to the immutable P7/P8 copies.

The full controller is available as `run_full_section6_1.py`; it still requires
primary's separate `reference_identity.py` qualification after the physical, P7/P8,
timer, transfer, and restart gates. The legacy disk-restart helper has not been
retargeted in this harness. No build or numerical run was launched for this harness.
