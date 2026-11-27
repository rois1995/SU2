# DEF1: the metric defaults become norm 2 and gradation 1.6; ADAP_HAUSD per marker for MMG; two capability runs

Author: reviewer, 2026-11-27 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/DEF1/inputs/`, together with the measure script
`reviewer_handoff/scripts/s6/shock_refinement_measure.py` (SHA256 in section 7). Start only after CL1 is closed
(it is: `efbab5351e`). The reviewer's MMG probes (`reviewer_handoff/runs/mmg*`, units `reviewer-mmg*`) are all
terminal; if a `reviewer-*` unit is active, do not touch it and start no build, test binary or solver while it is
active (R1: at most 6 heavy processes). Short names R, N, "mode S", "standing gates" as in the order FP3-A (your copy
in `N/FP3_A/inputs/`).

User orders (2026-11-27): "make gradation 1.6 and norm 2.0 the default"; for MMG, a Hausdorff distance for each
boundary with the form that SU2 already has ("there should be already an option of the SU2 config for this, so we
should use that"); the native remesher keeps one value for now.

## 1. Why

| Fact | State |
|---|---|
| Default `ADAP_NORM` = 1.0 (`CConfig.cpp:3280`); default `ADAP_HGRAD` = 1.3 (`CConfig.cpp:3293`) | read in the source |
| On CL1 (Mach sensor, 50k, p = 1, hgrad 1.3) the metric asks 1.83 times the realized shock-normal size at the shock; the gradation holds 87 % of the complexity | measured (offline replay, `reviewer_handoff/scripts/s7/study_metric_replay.md`; it reproduces the stored metric to 1e-11) |
| p = 2 with hgrad 1.6 at 50k asks 0.81 (mesh 4) / 1.11 (mesh 0) times the realized size; p = 2 + 100k + hgrad 1.6 asks 0.60 / 0.86 | measured (same replay; one flow solution each) |
| p = 2 (with `ADAP_HMIN= 3E-3`) passed a full two-event native run (R-SM, 2026-11-23, `15ea00574c`); without the HMIN floor p = 2 asks sub-millimetre cells at the trailing-edge and tip corners | measured |
| hgrad 1.6 failed event 2 of R-SG (2026-11-23, `bcaf43dddc`): two flat far-field "cap" cells below the q gate; the engine and the core have changed since (fast path, FP3-*) | measured then; NOT KNOWN now |
| The goal-oriented sensor (`ADAP_SENSOR= GOAL`) is the L1-optimal metric (`CConfig.cpp:6304-6307`) | read in the source |
| Serial MMG with the one global `ADAP_HAUSD` refines the far-field sphere (radius 100): with 1E-3 one remesh gives 379,085 points and 428,114 far-field faces (input 2,060), 1,325 s; with 1E-1 it gives 106,612 points, 6,833 far-field faces, 9,216 wing faces, 121 s (`reviewer_handoff/runs/mmg2probe`, `mmg3probe`, build680) | measured |
| The SU2 Python adaptation accepts `ADAP_HAUSD= (MARKER, value, ...)`: one MMG local parameter per listed marker (global hmin / hmax, its hausd); unlisted markers 0.01; an odd count is an error (`SU2_PY/SU2/adap/tools.py:65-75`, `interface.py:466-500` of the upstream scripts, for example in `/media/rausa/4TB/SU2_Versions/SU2_NewJST`) | read in the source |
| Our in-memory MMG path reads one double (`CConfig.cpp:3295`) and already passes MMG local parameters per marker reference (`CMMGInterface.hpp:91-96`; `WallLocalParameters`, `CMMGInterface.cpp:221-248`; override `SetLocalParameters`, `CMMGInterface.hpp:206`; the calls at `CMMGInterface.cpp:738-760`) | read in the source |

## 2. The production change (R2: allowed by this text, nothing else)

1. `CConfig.cpp` option `ADAP_NORM`: default 2.0. Exception for GOAL: if `ADAP_SENSOR` contains `GOAL` and the
   scalar key `ADAP_NORM` was not supplied, set the scalar `Adap_Norm` to 1.0 BEFORE the present GOAL warning
   (`CConfig.cpp:6303`) and before the per-level values are built (`CConfig.cpp:6627-6643`). The scalar is then the
   fallback of each level; a nonempty `ADAP_NORMS` list still overrides it per level through the existing level
   builder (only in loop mode), and `ADAP_NORMS= NONE` (empty list) leaves the scalar fallback. Keep the warning for
   an explicitly supplied scalar other than 1. "Supplied" = given in the local configuration or inherited from its
   base configuration; check in R4 what `OptionIsSet` (`CConfig.hpp:10321`) returns and write the test that uses it
   with that meaning.
2. `CConfig.cpp` option `ADAP_HGRAD`: default 1.6.
3. The option texts: `CConfig.cpp` comments, `config_template.cfg` (`ADAP_NORM= 2.0`, `ADAP_HGRAD= 1.6`, one
   comment line: "GOAL uses 1.0 when ADAP_NORM is not set"). The per-level list `ADAP_NORMS` in the template stays
   as it is.
4. Fallback constants that only mirror the old default (for example `CMMGInterface.hpp:63` `hgrad = 1.3`, which
   the constructor overwrites from the configuration; `BLLayer.inl:29`, fixture-only): list each in the R4 report.
   Change one only if a production path reads it instead of the configuration (then set it to the configuration
   value, not to a new constant). Do not change other code.
5. Serial MMG (user ruling for this order): the two new defaults ALSO apply to the serial MMG path (its metric norm
   and MMG's `hgrad`) when the keys are omitted. For these two options this supersedes the exclusion "a change of the
   default of the serial MMG path" in `AGENTS.md` section 6. MMG stays the default remesher (`CConfig.cpp:3308`); no
   other MMG default changes.
6. `ADAP_HAUSD` per marker (MMG only), the input form of the SU2 Python adaptation:
   - Scalar form: ONE numeric token, with or without parentheses (`ADAP_HAUSD= 0.01`, `ADAP_HAUSD= (0.01)`), takes the
     existing scalar path, unchanged (default 0.01). (SU2 strips parentheses and commas before the option parser,
     `option_structure.inl:86`, `CConfig.cpp:9155-9215`; today `(0.01)` reaches the scalar parser as `0.01`.)
   - List form: pairs `NAME, value` (`ADAP_HAUSD= (WING3D, 1E-3, FARFIELD, 1.0)`). Keep the parenthesized comma
     fields through validation. Errors in `CConfig` (`SU2_MPI::Error`): an empty field, an odd count, a value whose
     text is not fully consumed as a number, a value not finite or not > 0, a name twice, a list together with
     `ADAP_REMESHER= NATIVE_CAVITY` (text: "ADAP_HAUSD per marker is available only with ADAP_REMESHER= MMG; the
     native remesher uses one value."). Store the parsed form through the normal option and inheritance path, so a
     local scalar replaces an inherited list and a local list replaces an inherited scalar.
   - In list form `GetAdap_Hausd()` returns 0.01. It is the MMG global `hausd` and the value of every other reader
     (the transfers `CConservativeTransfer.cpp:867`, `CBarycentricTransfer.cpp:490`, `CAdjointTransfer.cpp:172`; the
     native readers, which a list cannot reach). Their formulas and checks do not change. An unlisted marker gets
     0.01 (as the Python scripts). The marker values go ONLY to MMG local parameters.
   - Mesh membership: before the choice between an override and the automatic rows (`CMMGInterface.cpp:604-614`),
     check every listed name against the physical markers of the gathered mesh (`CMeshGather.cpp:176`); an absent
     name is an error. This holds also with a nonempty `SetLocalParameters` override (TWO_PASS,
     `CBoundaryLayerRemesher.cpp:1139-1163`).
   - Rows: automatic path: one row per reference; where the wall rule (`WallLocalParameters`,
     `CMMGInterface.cpp:221-248`, enabled by `ADAP_BL_LOCAL_HMAX`) has the marker, keep its hmin / hmax and replace
     only its hausd by the listed value; a listed marker without a wall row gets global hmin / hmax and its value. A
     nonempty override keeps its present priority and its rows unchanged; an empty override keeps its present
     fallback. With the scalar form the rows, their order and their values are exactly today's. Put the row build in
     one small function. Pass full-precision values and the exact row count to MMG (2D edges, 3D triangles).
   - Print on the MMG status row the whole remesh time `endTime - startTime` at full precision (print-only; see
  section 4b). Print once per remesh (print-only) the MMG global hmin / hmax / hgrad / hausd and each local row (marker name,
     reference, hmin, hmax, hausd) as given to the MMG setters (`CMMGInterface.cpp:725-760`).
   - Option text in `CConfig.cpp` and `config_template.cfg`: both forms; for MMG the value is MMG's Hausdorff
     tolerance of the boundary approximation; for the native remesher the one absolute geometry tolerance (a check,
     gate R1).

No other default, gate, budget or tolerance changes (R1).

## 3. Keep the standing gates exact

The standing gates and the references in force must not move. Rule: every configuration that a standing gate, a
reference check or a regression test reads, and that does not set the key, gets the OLD value written explicitly
(`ADAP_NORM= 1.0` and / or `ADAP_HGRAD= 1.3`). A configuration that sets the key already does not change.
- Evidence controllers (G2, GQ, G3, G5 S with its isolated restart, `saved_current`, the shared-engine gates of
  `R/shared_engine_regression_gates.json`, the 2D BL / no-BL checks at 1 / 2 / 4 ranks with G4): write DEF1-local
  adapters (test code, R3). Each adapter (a) authenticates the sealed reference configuration and receipt UNCHANGED
  with the existing resolver (for example `FP2_P3/run_prefix_p3.py:141` compares the configuration SHA256 to the
  reference; `FP3_E/run_fp3_full.py:236` authenticates the historical run); (b) derives the launch configuration by
  inserting ONLY the missing legacy keys (`ADAP_NORM= 1.0` and / or `ADAP_HGRAD= 1.3`) and changes no other byte;
  (c) requires the launched bytes and the recorded hash to equal that derived configuration. Do not edit sealed
  evidence or the output comparators. The receipt records the sealed hash, the inserted lines and the derived
  hash. A configuration that sets a key already keeps its value.
- `TestCases/` and `UnitTests/` configurations, generated configurations and test code that build a `CConfig`
  without the key (the G1 set, the G4 set and the regression scripts): add the old value. In fixture sources, put
  the pin where it does NOT shift any line that carries an assertion (FP3-E build679 failed G1 from a one-line shift,
  `FP3_E/reports/G1_build679_diagnosis.md`); for example on an existing line of the configuration string. Keep the
  comparators and the references unchanged. List all touched files in the report.
- In R4 also list every code path that reads `GetAdap_Norm()` / `GetAdap_Hgrad()` (native metric, MMG, BL metric,
  BL outer transition, native core options) so that the pinning covers each caller.

Gates (shared code change; the results must be identical):
- G1 (identity table), G2 and GQ (exact prefixes, as FP3-E), G3, G4 (the Catch summaries of the three 2D runs at
  1 / 2 / 4 ranks against the sealed results, from the same runs as the shared 2D checks), the shared-engine gates,
  and ONE G5 run S with the pinned old values, with all its standing checks and the isolated restart: all six payloads
  equal to the S reference in force (`N/FP3_E/accepted_reference.json`). No OFF and no CORE full run (the shared
  gates and G2 / GQ cover the old paths); write them "not evaluated". Keep the native storage ceiling 12288 MiB and
  the job limit 19073 MiB distinct.
- Checks of the new defaults (test code, R3), part of section 3:
  (a1) put the GOAL decision in one small pure helper (inputs: sensor list contains GOAL, scalar supplied, scalar
       value; output: the effective scalar) called from `CConfig` postprocessing, and test it in the normal
       `test_driver` for all cases below;
  (a2) a `CConfig` fixture in the normal `test_driver` asserts the effective scalar norm and hgrad and the per-level
       norms for: both keys omitted (2.0 / 1.6); `ADAP_NORMS= NONE` (scalar fallback); an explicit `ADAP_NORMS`
       list (the list wins per level);
  (a3) the GOAL cases need the discrete adjoint, which the standing build rejects (`CConfig.cpp:5927`; reverse AD
       is off in `build_fp3.py:64`): build `test_driver_AD` once from the pinned DEF1 source (for example in
       `integration_evidence/build-integrated-ad-v1`, nice 19, 2-4 workers, R1) and assert: GOAL with `ADAP_NORM`
       omitted (1.0), GOAL with the scalar inherited from a base configuration (the inherited value), GOAL with an
       explicit `ADAP_NORM= 2` (2.0 and the warning), GOAL with an explicit `ADAP_NORMS` list (the list wins per
       level). Record its build and test receipts. Use the normal driver for G1 and the normal solver for CL2.
  (a4) `ADAP_HAUSD`: `CConfig` fixtures for the scalar form (`0.01`, `(0.01)`, a nondefault scalar: the value
       unchanged), the list form (per-marker values; `GetAdap_Hausd()` = 0.01), inheritance both ways, and each error
       (empty field, odd count, `1e-3junk`, a value <= 0 or not finite, a duplicate, a list with NATIVE_CAVITY);
       fixtures of the row function: list only; wall rule only; both on one marker; `ADAP_BL_LOCAL_HMAX= NO`; a
       nonempty override; an empty override; the scalar form (rows identical to today); a configured name absent from
       the gathered mesh, also with a nonempty override. Verify the effective global and local rows at the MMG setter
       boundary for 2D edges and 3D triangles (a small MMG fixture with two markers). Run each fatal case in its own
       subprocess and assert its diagnostic (`SU2_MPI::Error` aborts, `mpi_structure.cpp:111`). The scalar form
       must leave every standing gate unchanged (no pin needed).
  Compare floating values numerically (the logger prints 17 digits, `CNativeEngine3D.cpp:71`).
- A failed gate: R5 (one diagnosis, one correction, then revert and FAILED).

## 4. The capability run CL2 (no gate, no reference)

After section 3 passed. Use the CL1 controller and its gates and report (your `N/CL1/`, copy it to `N/CL2/`, test
code R3). Configuration: `N/CL1/run/run.cfg` with these changes ONLY:
```
(remove the line ADAP_HGRAD= 1.3)     -> the new default 1.6 applies
(no ADAP_NORM line)                   -> the new default 2.0 applies
ADAP_HMIN= 3E-3
```
`ADAP_HMIN= 3E-3` is the floor of R-SM (p = 2 asks sub-millimetre corner cells without it); it is not a default.
So the CL1 comparison measures the combined settings (norm, gradation, floor). Use a fresh case folder `N/CL2/run/`.
Bind the copied controller to the accepted DEF1 build receipt (CL1's `plan()` hard-codes build 680 and the FP3-D
template, `CL1/run_cl1.py:86`). Authenticate the sealed CL1 configuration and inputs, derive the CL2 configuration
(remove `ADAP_HGRAD`, replace `ADAP_HMIN`), and assert that `ADAP_NORM` and `ADAP_NORMS` are absent.
Runtime check of the defaults (in CL2, not in section 3): the direct driver's cycle-0 summary prints "norm 2"
(`CSinglezoneDriver.cpp:595-597`) and the first event's `NATIVE3D_REQUIREMENTS` rows report hgrad 1.6 (numerically;
performance logging on, as in CL1). The native metric uses it (`CSolver.cpp:3156`); the local core takes metric
tensors and has no hgrad of its own. If CL2 stops before these lines, write the check "not evaluated". A proven
wrong effective default (for example "norm 1" printed) is a DEF1 failure (R5 on section 2); any other CL2 failure
keeps the capability disposition below. Same binary as section 3 (the new build), same
launch environment rules, 6 ranks, watchdog 21600 s, the same failure rules and labels as CL1 with "CL2" in place of
"CL1". The report has the CL1 tables and, beside each CL2 number, the CL1 number. For each event also give: the
points and cells of the mesh, the sensor complexity before and after the gradation (from the log), and, on a
rejection, the rejected cells (q, J, the number of surface points, the markers) as in the R-SG diagnosis, where the
saved evidence has them; write "unavailable" for a field it lacks (for example the marker ids of the M29 dump).

A failed CL2 gate is a capability RESULT: it does not revert section 2 (the user decided the default). Write it,
preserve the evidence, and continue with section 4b.

## 4b. The capability run MMG2 (serial MMG with the new defaults; no gate, no reference)

Start only after CL2 is terminal AND section 3 stays passed (no R5 revert from the CL2 default check). Folder
`N/MMG2/run/`. Configuration: the CL2 configuration with these changes ONLY: remove every `ADAP_NATIVE_*` key and set
```
ADAP_REMESHER= MMG
ADAP_HAUSD= (WING3D, 1E-3, SYMMETRY, 1E-3, FARFIELD, 1.0)
```
(norm 2 and gradation 1.6 by default, `ADAP_HMIN= 3E-3`, 4 cycles, 2000 iterations, the same flow settings). Same
binary, 6 ranks, nice 19, the launch environment of CL1 (the `SU2_NATIVE_*` variables are not needed), watchdog
21600 s for the whole run with the reap of CL1.

An MMG2-local validator (test code, R3), per cycle k = 1..4 (capability results, not gates):
- the remesh: "MMG3D status SUCCESS" in the log section of event k, before the transfer lines; the WHOLE remesh
  time of event k (`endTime - startTime` of the remesh, `CMMGInterface.cpp:998-1025`, extraction, MMG and the
  distribution of the result included; print it at full precision on the existing status row, print-only) against
  the standing 5400 s limit; report the component times beside it;
- the applied parameters (section 2 item 6 print): global hmin 3E-3, hgrad 1.6, hausd 0.01; the three local rows
  with the three listed values on the right references; and "norm 2" in the cycle-0 summary;
- the physical criteria of CL1 gate 2 on mesh k and restart k (positive volumes, restart coordinates = mesh to 1e-12,
  finite positive density and internal energy), without the `.native_ref` (MMG writes none);
- the transfer of event k: conservative residuals <= 1e-10, zero inadmissible states, the ordered markers remesh
  status -> transfer -> replacement (no `NATIVE3D rank=` marker exists on this path);
- the continued CFD rows of cycle k as CL1 gate 4 and the solver exit message of cycle k;
- memory: the sum of the six terminal per-rank high-water marks against 19073 MiB.
Hash the files MMG2 actually writes (mesh, restart, `.vtu`). The native gates (timers, proof faces, native packets,
`.native_ref`) do not apply: write them "not applicable".

Failures: a proven wiring defect (a wrong applied value, a marker on the wrong reference, "norm" not 2) follows R5 on
section 2 (see the R5 exception in section 5). Missing runtime evidence is "not evaluated". A remesh, transfer, CFD or time-limit failure is a capability
RESULT. Report: the CL1 report tables (cycles, the shock measure with the unchanged script, the output files with
hashes) beside CL2 and CL1, plus per cycle the faces of each marker and the MMG seconds. Labels as CL1 with "MMG2"
("at cycle <k>, check <name>" in place of event and gate). Then stop.

## 5. Report and labels

R5 exception for the runtime checks of CL2 and MMG2: if a run proves a wiring defect of section 2 (CL2 default
check; MMG2 applied values), use the ONE diagnosis of R5; if the cause is a transcription or wiring error, correct it
once, rebuild, repeat the section-3 gates that the correction touches, and repeat the affected run ONCE (CL2 or
MMG2, from its start) to repeat the failed check. This is the only allowed second run. If the check fails again or
the cause is in this text: revert section 2 in one commit, label "DEF1: FAILED at <run> <check>", and do not update
`AGENTS.md` section 2a. After a proven wiring defect, DEF1 is accepted only when the same runtime check PASSES on the
corrected binary. If the one repeat ends before that check (watchdog, cut evidence, an earlier capability failure),
label "DEF1: OPEN at <run> <check>", preserve both attempts, and do not update `AGENTS.md` section 2a. A capability failure (remesh, transfer, CFD, time limit) never gives a second run.

`N/DEF1/reports/DEF1_<build>.md` and `.json`: the R4 list, the touched files, the gates of section 3, the default
checks, the CL2 report (or its link). Labels:
- "DEF1: defaults norm 2 and gradation 1.6; standing checks passed" after section 3 passed (list the "not
  evaluated" items: full OFF, full CORE);
- then "CL2: loop run DONE ..." / "CL2: ... FAILED at event <k>, gate <n>" / "CL2: ... OPEN" as in CL1;
- then the same labels with "MMG2" (section 4b; "at cycle <k>, check <name>" in place of event and gate);
- or "DEF1: FAILED at <gate>" (section 3 failed; reverted).
After the final acceptance only (section 3 passed, no R5 revert, and no open repeat of section 5), add one bullet to `AGENTS.md` section 2a: "Since DEF1 the defaults are `ADAP_NORM= 2` (GOAL: 1) and
`ADAP_HGRAD= 1.6`, also for serial MMG; `ADAP_HAUSD` takes a marker list for MMG; every NEWLY EXECUTED standing-gate and reference configuration that omits
a key gets `ADAP_NORM= 1.0` / `ADAP_HGRAD= 1.3` inserted (existing explicit values and sealed inputs unchanged); the
references do not change." Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and the timings
handbook. Then STOP.
Rules R1-R13 hold. Expected time: a build and the AD test build, the gates (about 2-3 h), CL2 (about 1.5 h) and
MMG2 (about 2-3 h).

## 6. Not in this order

A change of the metric order of operations (the user's decision of 2026-11-23 stays: gradation, then scale). A
change of `ADAP_HMIN` default, of the complexity, of the sensor. A repair of cap cells. A second CL2 or MMG2 run (except the R5 exception of section 5). A
per-marker geometry tolerance in the native remesher (a later item; it changes the geometry gate of R1).

## 7. Inputs

- Measure script `reviewer_handoff/scripts/s6/shock_refinement_measure.py`, SHA256
  `b74990859163f7936d897f886928eb79200d63ce27322fa59864bf96b80dc1ee`.
- Study `reviewer_handoff/scripts/s7/study_metric_replay.md` (context, not an order).
