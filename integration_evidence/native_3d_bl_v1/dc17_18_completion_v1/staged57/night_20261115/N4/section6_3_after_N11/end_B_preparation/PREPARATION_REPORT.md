# End-of-phase-B validation preparation

Status: **UNAPPLIED PREPARATION**. This directory contains validation adapters only. No build, G3/G4/G2/GQ run, or numerical work was launched. Existing B6 and B8 helpers were left unchanged.

## Gate binding

- **G3 reference:** first sealed N14 G3 case `N14/section6_12/validation/cases/n14_g3_first_20261117T104157_3163858`. Its first gate passed with 1,113 splits; the N14 pair passed with 1,099 common edits. The end-cut adapter uses the 6.12 physical checks, requires at least 336 splits, and compares the ordered `(edge, point)` edits up to the smaller accepted count plus observer-adjusted entry work. It does not enforce a P6c first-336 prefix. Other G3 integer rows remain report-only.
- **G2/GQ references:** direct N14 sealed cases `N14_G2_552` and `N14_GQ_552`. Their saved statuses report the expected `*_prefix_passed` stop, with all owned handles terminal. Candidate G2/GQ validation remains with the existing B8 runner and its primary receipt/direct comparison; this directory does not add a second validator or gate.
- **G4:** the isolated runner retains the N4 `run_cut_checks.py` / `run_unit_gates.py` path and verifies the `2d1`, `2d2`, and `2d4` gate set. It records the existing process samples with `/proc` PID start ticks and verifies observed handles are terminal before emitting its receipt. Launch requires explicit `--run`.

## Files and use

- `validate_end_b.py`: setup/reference self-check and post-run G3 result check. G3 needs the candidate case and the primary's terminal receipt containing PID/start-tick identities.
- `run_g3_558.py`: single G3 candidate launch adapter, reusing the N14 fixture/config/runtime pins and saved `SU2_NATIVE_*` environment, with the 900-second bounded serial run, resource sampling and PID/start-tick recording. `--prelaunch-stub` stops at the saved-command/pins boundary before `test_driver` Popen. It verifies all source pins and terminal build handles before either path.
- `prelaunch_g4_558.py`: G4 setup-only stub. It runs the existing `checked_step` provenance path and stops each of 2D1/2D2/2D4 before `test_driver` Popen. It has a source-only `--static-check`; its build-dependent `--self-check` does not launch a test process.
- `materialize_cut_build_plan.py`: after build558 is sealed, `--write` can create only the two-field `build558/cut_build_plan.json` required by `run_cut_checks.checked_step`; it refuses to overwrite an existing file.
- `run_g4_558.py`: G4 launch and receipt adapter. Before use, the primary must seal `build558` with `build_status.json`, `source_pins.json`, `terminal_handles.json`, and `cut_build_plan.json` (the latter must name `build558` and hash the source-pins file). The current directory has no sealed build558 status, so this runner has not been executed.
- `source_provenance.json`: SHA-256 and byte-size record for the drafts, controlling orders, P/source check, exact N14 references, and reused helper sources.

The setup check run was:

```text
python3 validate_end_b.py self-check
```

It passed, reporting `launched=false`, `built=false`, the expected N14 references, 336 minimum G3 splits, no P6c prefix gate, and G4 ranks `[1,2,4]`. The G3 adapter self-check and G4 static check passed; all five Python drafts passed `py_compile`. The build-dependent no-launch stubs and cut-plan materializer were not run; the primary is sealing build558 and coordinating its earlier B8 checks. No numerical result is claimed.

The G3 `g3_terminal_receipt.json` records the inner `run_cases` exit and its process-tree PID/start-tick handles. It does not claim that the outer controller has exited. For a real launch, the primary wrapper's `launch_g3.py` and `g3_primary_launch_identity.json` must exist and are pinned into the case; the primary controller must add its own terminal receipt after the outer exec returns before final G3 acceptance. The prelaunch stub does not require these real-launch files.

The latest applicable 6.12 correction at `N14/section6_12/correction_1010/NIGHT_WORK_ORDER.md` and the supplied user order control the G3 reference/gate reading here. `source_check.md`'s older P6c-prefix direction is superseded by the 6.12 10:10 correction for this validation.
