# FP2-P3: the witness closure by default, the default proof, and the new references

Author: reviewer, 2026-11-25 (user decision of 2026-11-25: "make the repair the default"; reviewed by Codex,
source facts by Codex). For the main agent. The user sends it; it is a user order. Copy this file with its
SHA256 into `R/night_20261115/FP2_P3/inputs/`.
Code base: `8f1325ef35` ("FP2-P2: S passed; default path not yet proven"), build672.
Short names: R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; N = `R/night_20261115`;
AR = `Common/src/adaptation/CNativeAffineReference.cpp`; P1, P2 = the orders FP2-P1 and FP2-P2 (your copies in
`N/FP2_P1/inputs/`, `N/FP2_P2/inputs/`); C1c1, C1c2 = `FP2_C1_CONTINUE_1.md` and `FP2_C1_CONTINUE_2.md` (your
copies in `N/FP2_C1/inputs/`); "closure" = the witness closure of P2.
The three full workflows, by the two production options `ADAP_NATIVE_LOCAL_CORE` / `ADAP_NATIVE_LOCAL_SURFACE`
(both NO by default): "OFF" = NO / NO, with the pinned config of `N/FX2/G5/build645/OFF/full645_run1`;
"CORE" = YES / NO, with `N/FP1/run.cfg`; "S" = YES / YES, with the config and the launcher of the run S of
FP2-P2. Keep the pinned configs byte for byte. The lab option T and the observers of FP2-M2 / M3 belong to the
lab test driver; only item B1 uses the M2 observer.

Scope. FP2-P2 showed with the closure on: N1 `classification_work` 156,105,696 against 463,538,756; first
collapse 76,652,506 work units (the historical comparison value is 90,809,869; the hard limit is
108,459,426); the run S 656.0 / 350.4 s against 951.3 / 689.0 s (build664); six lab trials with three exact
pairs; all hard checks passed. The six files of the run S differ from those of build664 and the saved-current
check has another trajectory hash: the closure changes results. This order makes the closure the default,
proves the default path, and makes the new references with the rule of the user for an item that changes the
mesh: all standing gates pass, and two runs with the same binary give the same files.
The geometry query serves the old engine and the fast path (OFF, CORE, S, the BL checks G3 and C492; the
native 2D checks use another remesher). So this order evaluates OFF, CORE and S.
Part A: one small production change. Part B (control, the closure OFF by the switch): the present code is the
accepted default. Part C (the closure ON, the new default): all standing gates and the two-run rule.

## 1. Assumptions

| Assumption | State |
|---|---|
| The exact switch value `0` bypasses the closure | from the form of the change. B1 checks the N1 projection of build664 and the named work sum; B2 checks the wider accepted control path |
| With the closure off, the present code gives the accepted default results (G2, GQ, G3, G4, safeguards, the six OFF files of N21 W, the six CORE files of FP1) | NOT KNOWN since FP2-C part 1 (build647). Tested by part B |
| The closure changes the results of OFF and of CORE | NOT KNOWN (it changes S and saved-current; the N1 lab result was exact). Measured by part C |
| Two runs with the closure on give the same files | MEASURED for the lab (three exact pairs). NOT KNOWN for the full runs, which have elapsed-time budgets. Tested by part C |
| With the closure on, rows of G2 and GQ differ from their references | expected (geometry work, scope bytes; and all that follows from another decision). Measured by part C |
| The full runs are faster with the closure on | MEASURED for S. Reported for OFF and CORE; no time gate |

## 2. Part A: the production change

In `ConformingWitness()` (AR:236-244) change the reading of `SU2_NATIVE_CONFORMING_WITNESS`: the closure is
OFF only when the value is exactly `0`; with no value and with each other value it is ON. Print ONE line to
stderr at the first read only when it is OFF: `NATIVE_SURFACE_CONFORMING_WITNESS enabled=0`. With the closure
on no line is printed (the old line `enabled=1` goes away). All the rest of P1 2.1 stays: one read in the
process, the two call sites, no config option, no member, no key bit. No other production change.
Build `SU2_CFD` and `UnitTests/test_driver` (a new build number under `N/FP2_P3/`), no new warning; freeze
and pin this ONE binary pair for all of parts B and C. One owned heavy job; the host preflight of R1 before
each launch. Commit: "FP2-P3: closure default on; not yet proven".

Switch receipts (test code, R3). In the environment of the actual child, after each inherited cleanup: set
the exact value `0` for part B and for the hidden public-off process; REMOVE the variable for part C and for
the public-on process. Export `0` explicitly to each MPI rank of an off run. Record the effective setting of
each solver, test and restart command. With `0`: require ONE line `enabled=0` on each rank whose query entry
is established by other evidence, and none on a rank that is known to have no entry. With the variable
absent: require that no such line is there. Keep the raw logs. Where stderr is merged into an output that is
compared, use the byte-rejoin procedure of P2 for the exact new line before the parse.

Runners (test code, R3). Use adapters and fresh attempt roots under `N/FP2_P3/`, apart by closure setting,
mode, trial and attempt. They bind this order, the build / source / runtime pins and the launch settings of
P3. The inherited runners ask for receipts that this order does not make: the build647 / build649 restore
hashes and the same-build fixture receipt with the 18 FP1 cohorts (`FP2_C1/run_c1_default_scoped.py`,
`FX2/run_scoped_fx2.py`); a same-build `lab_acceptance.json` (`FP2_C1/run_default_g5.py`);
`c3_acceptance.json` and a C3 lab receipt (`FP2_C3/run_fp2_b_g5.py`); the D1 / D2 promotions, the P2 checkpoint
and the P2 campaign (`FP2_P2/run_shared.py`, `run_s.py`); `default_proven.json` of build647
(`FP2_C1/run_c1_s2.py`). Replace these prerequisites by those of this order: the B2 scoped adapter takes the
closure-off fixture controls of B0 and the actual P3 receipts of G2, GQ, G3, G4 and the safeguards; OFF and
CORE of part B take the qualified receipts of B1 and B2a; each adapter of part C takes the passed checkpoint
of part B and the prerequisites that section 4 names (the full pairs: C1 to C4 passed); both S runs pin the
passed part B, the short items C1 to C4 and the historical S receipts that the report uses. Keep the inherited execution
validators and the historical evidence pins where they apply. Do not make a pass receipt for a check that
this order leaves out.

## 3. Part B: control, the closure OFF (`SU2_NATIVE_CONFORMING_WITNESS=0`)

B0. Fixture controls: the focused fixtures of FP2-C3 continuation 1 (9 serial, 3 two-rank; trace `0`) and the
    curved fixture of FP2-M1 (trace `1`) with both checkers of FP2-M3, with their inherited Catch counts (the
    build668 fixture summaries, through `FP2_P2/run_p2.py::expected_fixture`); the hidden public-off process
    of P2 section 4(c).
B1. N1, one lab trial, after B0 passed (trace `0`; `ADAP_HAUSD= 1E-4`, `SU2_NATIVE_FP2_M2=1`,
    `SU2_NATIVE_FP2_M1=0`, `SU2_NATIVE_FP2_M2_TABLE=0`, `SU2_NATIVE_FP2_M3_FACETS=0`). In the P3 copy of the
    `default-N1` adapter of P2, the prerequisites are the passed B0 receipts (not a same-build G1; do not
    make a G1 receipt for it). Gates: a qualified serial completion by the inherited lab evaluator;
    `seal_d1.py::stage_totals` of P2 on the selected measurement (the ten distinct stage keys and one
    complete observer receipt); EMPTY `repeat_differences` against the sealed trial 1 of build664
    (`N/FP2_C3/cases/build664/N1/measurement.json`), the compared `modeled_peak` fields included; and
    `classification_work == 463538756`.
B2. The default proof (all with the closure off):
    a. G2 and GQ with the qualified prefix comparator of C1c1 / FX2, the sealed scheduling-call reference
       included, against `N/N14/section6_12/validation/cases/N14_G2_552` and the N22 GQ capture
       (`N/N22/gq_reference.json`); G3 with BOTH its physical and its identity part against the G3 reference
       of N14; G4 with its three filters at 1, 2 and 4 ranks, its pins and its Catch-summary comparison (the
       summaries that `N/N4/section6_3_after_N11/N22C/validation/g4_case_paths.json` names); the
       safeguards `merit481`, `saved481`, `saved_current` against their reference cases
       (`N/N4/section6_3_after_N11/phase_N6/safeguards/acceptance_summary.json`);
    b. one run OFF with its inherited validators: all standing gates of both events; the six files identical
       to the N21 W reference (`N/N21/accepted_reference.json`); `compare_full.py --mode off` against
       `N/FX2/G5/build645/OFF/full645_run1` gives `exact_difference_count == 0`. Its findings on replay
       seconds are a report, not a gate;
    c. one run CORE with its inherited validators: all standing gates; the six files identical to
       `N/FP1/on_reference.json`.
Environment contract of G3 and C492 (parts B and C): their inherited guard `same_frozen_input_contract`
compares all `SU2_NATIVE_*` keys with a historical command that has no closure key. In the P3 adapters check
the closure setting and the trace setting apart, against the P3 launch receipt, and leave ONLY these two
keys out of that comparison (beside the existing exclusion of the frozen-config path). Keep the exact byte
comparison of the frozen config and each other input and target check.
Order and dependencies of part B: B0, B1, B2a, B2b, B2c in this order; each item needs the passed receipts of
the items before it; finish the independent sub-checks inside B2a. Label of part B: FAILED if a required
check failed; else OPEN if a required check is OPEN or not evaluated; else "FP2-P3: default proven with the
closure off".
Part C starts only after the label "default proven with the closure off".

## 4. Part C: the closure ON (the variable is absent), the new default

Each check runs with the same frozen binary pair. "Two runs" means: the same binary pair, the same frozen
inputs, settings and budgets, one after the other. Order of the work: C1, C2, C3, C4, then C7, C5, C6.
An OPEN check or a measurement failure in C1 to C4 does not skip the other short checks. The full pairs C7,
C5, C6 need C1 to C4 passed; they do not depend on each other. A check that a prerequisite blocks is "not
evaluated".
C1. G1 (closure unset, trace `0`): identical to the reference (375 / 114 / 228). The focused and the curved
    selectors of B0 again with the closure unset: Catch and the unchanged physical assertions pass; counts
    that depend on the closure (work, prepared nodes) are reported apart, beside B0. The hidden direct cases
    and the public-off / public-on processes of P2 section 4 with their cross comparison.
    Trace `0` for the focused cases and the hidden direct cases; trace `1` for the curved fixture and the
    public-query processes (as the runner of P2 sets them).
C2. The five shared checks.
    - First collapse: ALL its hard gates, the accepted-by-rank vector included (480 / 473 / 346 / 263 / 469 /
      464), all six exits, the work limit 108,459,426, zero refinements, the resource and stage limits.
    - Exact C492: its executed-state validator, the 388.8 s stage, its work and physical checks; report the
      first edit difference from RunA and the split count.
    - `merit481`: its comparison as it is.
    - Native 2D at 1, 2 and 4 ranks: as it is.
    - `saved_current`: TWO runs. Each completes the fixture and passes its hard checks. Gate: both runs agree
      on `trajectory_working_owned.bin`, `trajectory_working_parameters.bin`, `correction_unchanged_owned.bin`
      and on the ordered planning projection of `N16/run_c.py::planning_rows`. The comparison with the
      historical reference case is a report. Only this historical comparison of `saved_current` is replaced
      by the new pair. A difference in another required comparison of C2 prevents the acceptance.
C3. G2 and GQ, each TWO captures, with the same inputs and stopping anchors as B2a. The controllers have a
    historical work-equality test (the G2 controller, and the G2 check that is inside GQ:
    `N4/run_prefix_controller.py:684-727`): for part C replace it by the first-collapse work limit, the exact
    accepted counts and zero refinements; keep the complete six-rank evidence, the resource limits, the pins
    and the completion checks. Gate: each capture is complete and valid, and the two captures of one check
    are equal under the comparator of N22 part C (candidate against candidate), with the scheduling calls
    exact between the two candidates and the existing census rule. Then capture 1 is the new reference.
    Report, no gate: capture 1 against the old reference: over the projected rows of the comparator, the
    number of rows that differ, are missing or are extra, by row type; ownership / decision differences
    apart; the first difference (the early return of the comparator is not enough for this report). Rule
    of the count: inside each rank stream and projected row type, compare the occurrences in their order; an
    unequal pair is "changed", an occurrence without a partner is "missing" or "extra"; the first difference
    is that of the comparator order without grouping.
C4. G3 with a test adapter that gives the unchanged physical acceptance (the 336-split minimum included)
    without the historical identity fields; it computes and seals the common edit count, the first
    mismatch and both observer-adjusted entry-work values, with no assertion on them. The qualified G3 run of
    part C is the G3 reference for later items with "identical results". G4: the three filters, all ranks,
    the pins and the historical Catch-summary comparison; the native 2D execution of C2 may serve G4 when the
    G4 evaluator verifies that same execution.
C5. OFF, TWO runs. Qualify each run and its isolated restart first: all standing gates of both events (5400 s
    for each event, 19073 MiB with the job peak equal to the sum of the process peaks, the physical
    validator, the transfer, the continued CFD, the isolated restart, 95 % timer coverage, join agreement,
    owned proof faces SUM equal to the surface size). In the full controller replace the historical stop
    `work_equals_P6c` (`N4/run_prefix_controller.py:780-788`) by the first-collapse work limit, the exact
    accepted counts and zero refinements; keep the other validators and limits. Do NOT require the historical
    six files or a zero historical row difference to seal a run of part C. Gate: the six files, hashed from
    disk, are equal in the two runs. Report: equal to the N21 W files or not; `compare_full.py --mode off`
    of run 1 against the OFF run of part B (exact-row findings and replay findings apart); the native maxima
    of both events beside part B.
C6. CORE, TWO runs with the pinned CORE config. Each run and its isolated restart pass the inherited CORE
    full-workflow validators, with the inherited CORE treatment of the first-collapse and one-sided
    refinement observations (the controller replacement of C5 is for OFF only). Gate: the six files, hashed
    from disk, are equal in the two runs. Report: equal to `N/FP1/on_reference.json` or not; the native
    maxima of both events beside B2c. CORE has no `compare_full.py` gate.
C7. S, TWO runs (trace `0`) with its isolated restart: each passes all standing gates; the two runs give the
    same six files. Report beside the run S of build664 and of FP2-P2 (build672, closure `1`): the table of
    P1 step 3, and whether the six files equal those of the FP2-P2 run S.
The second run of C5, C6 and C7 is made also when run 1 equals an old reference: the pair is the proof that
the default is repeatable.

## 5. Acceptance and the references in force

Publish the record only when part B has the label "default proven with the closure off" AND each required
check of part C passed (an OPEN or a not evaluated required check prevents it).
- `N/FP2_P3/accepted_reference.json`: the accepted commit; the SHA256 of this order; both binary hashes; the
  sealed gate receipts of parts B and C. For each of OFF, CORE and S: the effective config; for both runs the
  case path, and the full-run receipt path and the restart receipt path with their SHA256, the six verified payload hashes and the native
  maxima of both events. For each G2 and GQ capture: a manifest of its compared rank logs, its validator
  receipts and its provenance, with SHA256. The G3 case with its physical receipt and its edit / work
  evidence. Both `saved_current` cases with the three payloads and the planning projection; run 1 is the
  reference case.
- Append to `AGENTS.md`, at the end of section 2a, this block (the user orders it here):
  "- References in force since FP2-P3 (the witness closure is the default):
  `R/night_20261115/FP2_P3/accepted_reference.json`. A run with the default (closure on) uses its entries:
  OFF for G5, CORE and S for these modes, G2, GQ, G3 and `saved_current`. The older references (N21 W, FP1,
  N14, the N22 capture, the old `saved_current` case) stay for a control run with
  `SU2_NATIVE_CONFORMING_WITNESS=0`; that value is a control, not a supported setting. The references of G1
  and G4 do not change. For G2 and GQ with the default, bind the prefix identity check, the scheduling calls
  included, to the accepted P3 capture with the comparator of N22 part C and its census rule; the
  first-collapse admission in both prefix controllers is the unchanged work limit, the exact counts and zero
  refinements. The historical work value 90,809,869 and the FX1 scheduling table belong to the control with
  the closure off. G5 OFF with the default uses the first-collapse admission of FP2-P3 item C5 and the
  accepted P3 OFF files. The limits and the count reference of the shared first-collapse check do not
  change."
- Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and `Handbook/ADAPTATION_TIMINGS.md` (the
  native maxima of OFF, CORE and S with the closure off and on; where the time is, as the report of S).
- Label: "FP2-P3: accepted; the closure is the default".
The report `N/FP2_P3/reports/FP2_P3_<build>.md` and `.json`: the status of each check (passed / FAILED / OPEN /
not evaluated); for each pair the equality and the first difference if there is one; the tables of part C
beside part B; the list "not evaluated"; no recommendation. Then STOP and wait for the next user order.
Not evaluated in this order (write it in the report): the 18 FP1 cohorts (a separate fixture campaign); a
time rule (the times are a report; the machine has time noise).

## 6. Failures and labels

- Each required check is passed, FAILED or OPEN. A check whose prerequisite is not available is "not
  evaluated". Finish the checks that do not depend on an unresolved check (R12 holds; commit and push the
  checkpoints, R8).
- A confirmed failure of a result gate, a required pair that is not equal included: R5. Keep the first
  difference and the evidence; one diagnosis of at most 60 minutes; a confirmed wiring fault against this
  text: ONE correction and one repeat. If a production correction changes one of the two binary hashes: keep
  the old evidence and run ALL required checks of parts B and C again on the new pair before the
  acceptance. A test-only correction may use again the receipts that it does not affect and whose pins
  still verify. If the failure stays, or its cause is in this text: revert part A in ONE commit (the closure
  is then off by default again, as `8f1325ef35`), write the hash, label "FP2-P3: FAILED; the closure stays
  off by default", report, STOP.
- Exception to R5 for part B, for this order only. Keep the commit of part A (it is labelled "not yet
  proven") only when the read-only diagnosis ESTABLISHES the same gate failure on build672 with the closure
  off, or proves that an older sealed receipt is equivalent to that control for the failing check (verify
  the source, the inputs, the settings and the validator; the same symptom on an older build is not enough).
  Then: label part B FAILED, part C is "not evaluated", no reference is published, report, STOP and wait for
  the user. If the evidence is not available or the cause is not resolved at the R5 limit: the ordinary
  failure rule above.
- Label of the whole item: FAILED if a required check failed; else OPEN ("FP2-P3: OPEN for the user") if a
  required check is OPEN or not evaluated; else accepted.
- A helper watchdog or wall limit is not a gate. Retry ONE time with three times that watchdog, only when the
  owned handle is terminal, the pins and receipts verify, and every result gate that was evaluated passed.
  Keep both attempts; retry only the stage that was interrupted (for an isolated restart, from the sealed
  events). For C492 this is the outer wrapper only. These are GATES and never get a longer limit: the 388.8 s
  stage of C492, the 1,500-second deadline of the first collapse, the 600 s outer acceptance limit of the 2D
  checks, 5400 s for each event. A breach that is shown is a failure, also when a helper ended too. A second
  helper-only end: that check is OPEN. An inherited retry counts as the one retry. For a retry, bind the
  validator of the wrapper command and the timeout classifier to the watchdog that the attempt recorded
  (C492: 600 s on attempt 1 and 1,800 s on the retry; G3: 900 s and 2,700 s; the inherited validators check
  the literal first values). Verify the actual command and the serial rank count. Never write the duration
  of attempt 1 into a retry receipt. The native BL stage, work, split, refinement, memory and input checks
  do not change.
- A defect of a launcher, an adapter, a collector or a report helper (test code): R3 / R6. If a required
  measurement is still wrong or missing after that work, the check is FAILED as a measurement; that needs no
  production revert and prevents the acceptance.
- Never mix receipts of two binary pairs. Never label a preserved failure as accepted.
The rules R1, R3, R5, R6, R7, R8, R9, R12 and R13 hold as in each item.
Expected machine time: about 9 hours (build 5 min; part B about 3 h; part C about 6 h).

## 7. Not in this order

A change of the closure rule, of a check of the closure, of a gate, of a limit or of the tolerance. A config
option for the closure. The removal of the switch (a later cleanup). A change of the production options or of
their defaults. The 18 FP1 cohorts. Work on the open items of the fast path (the failing-root work without a
contact, the scope build, communication and join wait).
