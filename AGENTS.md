# Work order 3 (clean copy of the order in force)

Written 2026-11-19, 20:15, after N22 and FP0. Author: reviewer, for the main agent. The user sends it; it is a user order.

This file replaces "Work order 2" and the "User order" lines above it in `AGENTS.md` as the order in force. Put it in
`AGENTS.md` in their place, from the top of the file to the line before "# Standing user instructions (unchanged)". The
standing user instructions below that line do not change. Work order 2 and its files (H, D, C, I, I8R, I8A, I8R2) stay in
your evidence folders as history. Where this file and an old text differ, this file wins. If you find a rule of the old
order that is still necessary and is not here: use the old rule and write it in the report (R4, "reading used").

Sections 1 and 2 are the text of work order 2, not changed. Section 2a has the references that changed since then.

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## 1. Rules (the same rules as before, in one place)

The user and the reviewer are not available during the work. You get no answer during
the work. A question never stops the work; R13 says where a question goes.
Where an older text says "stop and wait for the reviewer", read it as "stop THAT ITEM,
record it, and go to the next item that does not depend on it".

R1. No gate, budget, tolerance or limit changes. 5400 s for each event, 19073 MiB for the
    job, geometry 1e-4, q / J / size / height gates, requested heights and progression.
    One owned heavy job, nice 19, at most 6 heavy processes on the machine. Check the
    host load and the free memory before each long run.
    A heavy process is a solver rank, a build job, a test binary, or a script of ours
    that uses more than half of a core for more than 60 seconds. A browser or another
    program of the user is not a heavy process of the work. It does not fail a run and
    is not a reason to repeat a run. Record it in the receipt as an observation only.
    Your own helper scripts count (phase B: a renderer that was not awaited made 7).
R2. No redesign and no own fix of production behaviour. The reviewer supplies the ideas
    and the design. You implement what H, D, C, I and this file say. A production
    change that is not written there is not allowed. D is read-only for you.
R3. Your own test code is yours to make work. Fixtures, scripts and report helpers that
    you wrote: find the cause, correct, run again, until they test what the item asks.
    No limit on the count; limit of 90 minutes for one fixture. Never change production
    code, a gate or a tolerance to make a test pass. Write each cause and correction in
    the report.
    Do not change pinned fixture values; report changed counts or ranges.
R4. One source check for a whole item, before its first production edit. Write all
    contradictions in one list in the report.
    - A contradiction on a print-only counter or a report row: write it, continue.
    - A contradiction on a result where the written text has one clear reading that the
      source supports: use that reading, write it in the list as "reading used".
    - A contradiction on a result with no clear reading: do not implement that part.
      If the rest of the item is independent of it, do the rest. If not, skip the item.
R5. A failed result gate (outputs not identical where they must be, accept / reject,
    validity, geometry, quality, memory, time):
    - Preserve the evidence. Write the first number that differs.
    - Use ONE diagnosis pass of at most 60 minutes, read-only plus the existing counters,
      to name the cause. If the cause is a transcription or wiring error against the
      written proposal (your code does not do what the text says), correct it one time
      and repeat the failed check one time.
    - If it fails again, or the cause is in the proposal itself: revert the item in one
      commit (keep the evidence and the reverted commit hash in the report), mark it
      FAILED, and go to the next item that does not depend on it.
R6. Time limits: 90 minutes for one fixture (R3); 60 minutes for one diagnosis (R5).
    At a limit: write the state, go on.
R7. Keep the progress file `R/night_20261115/progress.md`. Update it after each sub-step:
    item, sub-step, commit, gate numbers, next action. After each compaction of your
    context, and at the start of each new session, read this work order and the
    progress file again before you continue.
R8. Commit after each passed sub-step. Push the checkpoints. Never commit a state that
    fails its gates as "accepted"; a preserved failure is labelled as a failure.
R9. Readable code (D 7.1a) for all new code. Written exceptions are listed in C.
R10. An item with "identical results" adds no timer operation and no field to `Session`,
    `CellState` or a row that travels. These sizes go into the modeled bytes. Each
    `NATIVE3D_TIMER_SUMMARY` row prints `fixedstorage=4120`.
R11. Print-only code runs under `options.timings`. No decision reads its values.
R12. A new order never stops a running job. Let the check that runs end first.
R13. Questions (user order, 2026-11-17). When you have a question about an
    implementation detail or a choice (examples: a text has two readings; new code has
    two possible places; R4 lets you choose a reading), write it in the file
    `R/night_20261115/QUESTIONS_FOR_REVIEWER.md`. One entry: date and time, item and
    sub-step, the question, the options that you saw, the option that you used and why,
    the commit. Append only. Then continue the work with the option that R2 and R4
    permit. Do not wait: no answer comes during the work. If the reviewer answers, the
    answer comes as a user order at a phase boundary. Only such questions go in this
    file. Status, failures and contradictions stay in the progress file and the reports.

## 2. Gates (C has the details; the lines below are the corrections that are in force)

- G1 is an identity gate with its own reference (C 2.1): the table of cases, pass /
  fail and failed assertions is identical. The totals of successful assertions are a
  report. Reference: the N14 set, `R/night_20261115/N14/section6_12/validation/adopted_reference/`
  (375 / 114 / 228 records for serial / 2 ranks / 4 ranks).
- G2 and GQ: all old integer rows, modeled bytes and timer calls of each row are equal
  to the reference. One exact rule for the operation `census`: for each rank and each
  timer boundary, the sum of `calls` over its rows is (reference value) + (count of
  `NATIVE3D_WALL_LAW` lines that this rank printed before that boundary) - (the same
  count in the reference run). For an item with identical results this difference is 0.
  References: `R/night_20261115/N14/section6_12/validation/cases/N14_G2_552` and
  `.../cases/N14_GQ_552`.
- G3 (the BL wall fixture; it has clock inputs, so its counts are not repeatable):
  (a) Physical part: all private cells positive with sensors, zero upper refinements,
      the modeled storage and the job peak inside their limits, at least 336 splits.
      (The old row "the first 336 edits identical to P6c" ended with N14.)
  (b) Identity part, for an item with "identical results": the accepted edits (edge and
      point of each edit) are identical to those of the G3 reference run, up to the
      smaller accepted count of the two runs; and `entry atlas_work` minus the observer
      work is equal in the two runs.
  (c) All other integer rows of G3 are a report beside the reference. They are not a gate.
  Reference run: the first G3 run of N14 (1,113 splits),
  `R/night_20261115/N14/section6_12/validation/cases/n14_g3_first_20261117T104157_3163858`.
- G4: as C says.
- G5 (the full two-event no-BL workflow): all standing gates of both events (5400 s;
  19073 MiB with the reported job peak equal to the sum of the process peaks; physical
  validator; transfer; continued CFD; isolated restart where C asks it; 95 % timer
  coverage; join agreement; owned proof faces SUM equal to the surface size). Event 1:
  three files identical to P6c. Event 2: three files identical to the P7 reference
  (your `R/night_20261115/section6_1/accepted_reference_payloads.json`). Where C says
  "six files byte-identical to P6c" (C section 2 row G5 and C section 6 item 1), read
  this rule. The event-2 hashes:
  - `adapted_mesh_adap_00002.su2`            63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36
  - `adapted_mesh_adap_00002.su2.native_ref` 36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a
  - `restart_flow_adap_00002.dat`            44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94
- Safeguards for a change of shared code: the executed gates of
  `R/shared_engine_regression_gates.json` (exact first collapse, C492, 481, current
  target, 2D at 1 / 2 / 4 ranks).
- An item that changes the mesh is accepted only when its own text says so. The rule of
  the user for such an item: all standing gates pass, and two runs with the same binary
  give the same files. These files are then the new reference.

## 2a. References in force (2026-11-19)

- Accepted code: N22 part C, commit `e40d7e3795` (report `03323fa621`); FP0 test code `689ae4746e`.
- G5 files: the six files of N21 part W. Record: `R/night_20261115/N21/accepted_reference.json` (it names the payload list
  `N4/section6_3_after_N11/round_8_5t_W/G5/qualified_reference_payloads.json`). N22 parts M and C gave the same six files.
  Where section 2 names the P6c and P7 files for G5, read these six files.
- GQ: the capture of N22 (`R/night_20261115/N22/gq_reference.json`), with the rows that N22 part C lists as "report only"
  (calls and seconds of `communication`, the collective and election counters, all seconds).
- G1 (375 / 114 / 228), G2 and G3: the references of section 2, not changed. G2 and GQ use the comparator of N22 part C
  (`R/night_20261115/N22/compare_prefix.py` with part C). The calls and seconds of each `communication` timer row
  (in each phase) and the collective, election and tracked-call counters of the integer rows (the fields `C_FIELDS`
  of that comparator) are a report. All other rows are exact: integer rows, modeled bytes, decisions, the timer rows
  of each other operation, `fixedstorage=4120`. Reference for the exact rows: N14 for G2, the N22 capture for GQ.
- Times (native maxima of the two events): N21 part W 2048.286 / 1278.700 s; N22 part M 1981.679 / 1348.621 s; N22 part C
  2018.805 / 1083.551 s. The machine has time noise: a run during the day can be 3 to 6 % slower than at night, and more
  when another program runs. A time rule compares two versions that ran one after the other; where that is not possible,
  a miss of a time rule alone is "OPEN for the user", not a revert (as in the order for N22).
- Timer coverage gate (95 % on each rank): the smallest value is now 95.23 % (N22 part C, event 2, rank 5). The gate does
  not change. Do not start a step that removes more named time before the reviewer's text for it says how the unattributed
  time gets names.
- References in force since FP2-P3 (the witness closure is the default):
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
  change.

- Since FP3-A the S reference is the `workflows.S` entry of
  `R/night_20261115/FP3_A/accepted_reference.json` (in mode S the engine makes no construction collapse
  after step C completed; the optional improvement cleanup stays). Ordinary S runs use it. The FP2-P3 record
  stays in force for OFF, CORE, G2, GQ, G3 and `saved_current`, and its S entry stays as the reference of the old
  behaviour (`SU2_NATIVE_FP3_PROBE=2`, a lab control).

- Since FP3-B the S reference is the `workflows.S` entry of
  `R/night_20261115/FP3_B/accepted_reference.json` (no ownership exchange at a construction-collapse exit whose
  pass was closed). It replaces the S entry of FP3-A for ordinary S runs. The FP2-P3 record stays for OFF, CORE,
  G2, GQ, G3 and `saved_current`; its S entry stays the reference of the lab control value `2`.

- Since FP3-D the S reference is the `workflows.S` entry of
  `R/night_20261115/FP3_D/accepted_reference.json` (with the closed collapse queue the ownership exchange is
  made when the owned-cell ratio is above 2, at each evaluation site). It replaces the S entry of FP3-B for
  ordinary S runs. The FP2-P3 record stays for OFF, CORE, G2, GQ, G3 and `saved_current`; its S entry stays the
  reference of the lab control value `2`; the S entry of FP3-B is the reference of the lab control value `3`.

- Since FP3-E the S reference is the `workflows.S` entry of
  `R/night_20261115/FP3_E/accepted_reference.json` (with the closed collapse queue, a quality step whose best global
  debt falls less than 5 % over four observed windows arms at most one hand-off request per engine invocation,
  subject to the existing construction guards). It replaces the S entry of FP3-D for ordinary S runs. The FP2-P3
  record stays for OFF, CORE, G2, GQ, G3 and `saved_current`; its S entry stays the reference of the lab value `2`;
  the S entry of FP3-B is the reference of the lab value `3`; the S entry of FP3-D is the reference of the lab value `4`.

- Since DEF1 the defaults are `ADAP_NORM= 2` (GOAL: 1) and `ADAP_HGRAD= 1.6`, also for serial MMG;
  `ADAP_HAUSD` takes a marker list for MMG; every NEWLY EXECUTED standing-gate and reference configuration that omits
  a key gets `ADAP_NORM= 1.0` / `ADAP_HGRAD= 1.3` inserted (existing explicit values and sealed inputs unchanged);
  the references do not change.

- Since SC1, ordinary mode S can reuse exact geometry certificates in the construction census when
  step C has closed the construction-collapse queue. Two build682 runs and their isolated restarts passed
  with the six S payloads identical to FP3-E, whose `workflows.S` entry remains the S reference in force.
  Use the DEF1 legacy settings for standing checks. Lab value `5` disables the certificates and value
  `6` measures them with all legacy distance queries active.

## 3. State (history; do not repeat a passed check)

Items N1 to N18 and the cut (N4 phases A to D, N6): see the table of work order 2 in your evidence and the handbooks.

| Item | Result | Commit |
|---|---|---|
| N19 (M29, dump of the stuck cell) | PASSED | `da20c9e9fe` |
| N20 A (step 8.5s, fine move for stuck cells) | PASSED | `01d2bbd24f` |
| N20 W (wide rounds, first retry) | FAILED its time rule, reverted | `1174e4c241` |
| N21 P (step 8.5t, the ranks share the final surface proof) | PASSED | `08a8d7c7ff` |
| N21 W (wide rounds with the time factor 0.95) | PASSED; its six files are the reference | `7ac99f096d`, `4fe083d460` |
| N22 M (step 8.5u, ownership decision without the clock) | PASSED; the time flag "OPEN" is ACCEPTED by the user (2026-11-19) | `484f29315f` |
| N22 C (step 8.5u, census without a vote for each chunk) | PASSED; the time flag "OPEN" is ACCEPTED by the user (2026-11-19) | `e40d7e3795` |
| FP0 (fast path stage 0, measurement) | DONE | `689ae4746e` |

Both OPEN flags of N22 are closed. No action is necessary for them.

## 4. The work from now

The user decided a new direction on 2026-11-19: a fast path for the adaptation (a new lean local remeshing core; the
present engine stays as the checker and repairer). The design of the reviewer is `FAST_PATH_DESIGN_V1.md` (your copy:
`R/night_20261115/FP0/inputs/`). It is context, not an order.

1. Now: no item is open. Do not start work of your own. Wait for the next user order. It will name the text of fast path
   stage 1 with its SHA256.
2. When an order comes: copy its text with its SHA256 into your evidence folder, read it completely, make one whole-item
   source check before the first production edit (R4), and do it.
3. The report of each item, the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and `Handbook/ADAPTATION_TIMINGS.md`:
   as in the items of 2026-11-19.

The folder `reviewer_handoff/` in the repository root belongs to the reviewer: do not edit it and do not commit it.

## 5. Open items that have their own text

None.

## 6. Not in this work order

Design steps 8.5ab, 8.5c, 8.5d, 8.6 and 8.7 of the old design D (the fast path replaces their purpose; do not implement
them). A quota from the job memory limit. Any work on A5 / B5. Any BL or RANS capability claim. A change of the default of
the serial MMG path.

# Standing user instructions (unchanged)

User order, 2026-11-14 (M6 fix handoff): protect shared-engine changes with the
executed gates in `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/shared_engine_regression_gates.json`.
For each connected shared-code change, run the affected no-BL first-collapse
work/count gate, exact saved-state BL repeat and native 2D BL/no-BL checks at
1/2/4 ranks. Run the full two-adaptation no-BL workflow at integration milestones.
Keep the stated time/work/resource and physical gates unchanged, and report failed
or unevaluated gates explicitly. Artifact hashes alone prove files unchanged;
they do not establish execution or performance preservation.

User clarification, 2026-11-11 (LATEST CADENCE; supersedes earlier immediate-full-run retries):
Use incremental, stage-based integration. Establish the execution sequence/timing
from source and existing evidence. Work in order: saved CFD/geometry; composed
metric/normalization (finish tangential-profile investigation); coupled surface
preparation; constrained directions/first row; successive growth; outer transition
and strict closure; transfer/continued CFD/second adaptation/isolated restart.
Define each stage's input/output/acceptance before changes. Save credible reusable
stage outputs with complete provenance and enough state to reconstruct the SAME
composed target and geometry; sensor tensors alone are insufficient. Preserve
validity-first provisional outputs with diagnosed quality debt; final gates still
apply before mesh acceptance. Use focused checks and meaningful MPI integration
milestones. Reserve full M6 retries for integration milestones or failures that
cannot be reproduced at a stage boundary. Keep one cycle, one owned heavy job,
budgets, source freezing and the complete final objective unchanged.
Maintain Handbook/ADAPTATION_TIMINGS.md with current per-stage timings/work and
evidence-backed major computational efficiency improvements; keep nested timers,
contention and attribution gaps explicit.

User clarification, 2026-11-11: RANS500k is a NOMINAL metric-complexity target,
not an acceptance gate or exact node count. Explain overshoot and uncertainty;
preserve requested normal heights/progression and all strict acceptance/resource gates.

User clarification, 2026-10-30: the 500k adaptation target was intended for RANS,
not Euler. Use a much smaller Euler target; the current functional-loop attempt
uses 50k (an implementation choice), with 500k reserved for RANS. Keep all geometry,
quality, transfer and requested-height gates strict. Report actual mesh point/cell
counts separately from the metric-complexity target.

User instruction, 2026-10-29: let the complete M6 attempt drive development.
Freeze a coherent implementation, allow generous but bounded certification work,
and attempt the requested CFD → mesh → CFD → mesh workflow. Address construction,
certification or transfer/CFD according to that run's failure; focused checker
changes and tests are diagnostics, not the immediate capability objective.

Permanent user instruction, 2026-10-29 (overnight continuation): if the M6 run
reveals a clear computational bottleneck whose fix will improve efficiency, or a
construction/certification/transfer/CFD error, preserve its evidence, stop only the
verified owned run, implement a connected fix, validate the affected behavior and
rerun ONERA M6. Continue autonomously until a valid accepted Euler
CFD → mesh → CFD → mesh loop succeeds; passing diagnostics are not completion.
Then verify that BL generation still works before proceeding with the RANS grid
and the same target-driven workflow. Keep strict acceptance gates, one owned heavy
workload, resumable evidence and regularly pushed reviewable checkpoints. The user
is unavailable overnight; resolve routine choices within the authorized scope
without waiting for answers. This instruction persists across sessions/compaction.

User instruction, 2026-10-12: prioritize substantial, connected implementation
steps over repeated small checkpoints.

- Group changes that solve the same end-to-end capability before running a broad
  regression. Do not require the full suite after each small edit or checkpoint.
- During implementation, run focused tests when they resolve a concrete failure,
  exercise new risky behavior, or validate a changed interface. Documentation and
  routine low-impact edits do not need a new test campaign.
- Run appropriate broader regression at meaningful integration milestones and
  before declaring the integrated result validated. Never weaken acceptance gates
  or omit necessary correctness checks to satisfy this cadence.
- Reserve MPI campaigns for substantial connected implementation milestones; do
  not repeat them for each small local edit. Use focused serial checks during
  development where they resolve a concrete risk.
- Push reviewable checkpoints regularly. Clearly label incomplete or unvalidated
  work; a checkpoint does not itself require repeating a previously passing suite.
- Updates should state progress toward the actual end-to-end goal and its remaining
  blockers, rather than treating each passing intermediate test as a milestone.

## Capability-first implementation

Permanent user instruction, 2026-10-14; applies across context compaction and
future sessions.

- Organize work around a complete end-to-end capability and its known difficult
  failure, rather than repeatedly extending isolated operators until toy cases pass.
- Before a substantial implementation, state the missing capability, the evidence
  for its underlying limitation, and the acceptance conditions. Choose a connected
  design that addresses that limitation across the affected callers and constraints.
- Use the known difficult target and a small set of meaningfully different fields
  or geometries to assess the complete change. Do not defer the difficult case
  until a succession of easier checkpoints has passed.
- Simple tests establish primitive correctness or resolve specific risks; they do
  not establish robust mesh generation and must not be reported as capability
  milestones. Keep private intermediate progress distinct from an admissible final
  mesh and a successful adaptation/solution-transfer workflow.
- The next meaningful curved-construction milestone is a complete accepted mesh
  under unchanged constraints. If the current recovery cannot deliver it, reconsider
  the construction strategy before adding another local fallback.
- When a strategy stalls on the difficult target, preserve the diagnosis and
  reconsider the construction or coordination it lacks. Do not keep adding local
  variants or raising limits without evidence that they address the blockage.
- Deliver substantial connected improvements, then validate at the cadence above.
  Preserve the full agreed goal and all acceptance gates; neither a prototype nor
  a passing fixture justifies narrowing the goal or declaring it complete.

## Reuse and resumable validation

Permanent user instruction, 2026-10-13; applies across context compaction and
future sessions.

- Reuse existing implementation and validation facilities before adding new ones.
  Keep implementation and checks tied to the full agreed goal and its exclusions.
- Reuse passing evidence when its scope covers the requirement and the relevant
  source, build/binary, configuration, inputs and validation helpers remain valid.
  Verify the needed provenance; source hashes alone do not prove unchanged runtime
  behavior. Rerun checks affected by changes, failures or unresolved concerns.
- Make long validation campaigns resumable: save complete independently validated
  cases or stages with their provenance as they finish. A timeout must not discard
  completed evidence or force successful work to repeat. Partial results cannot
  establish whole-campaign success.
- Resume a saved result only after verifying its relevant hashes and acceptance
  conditions. Preserve failures and diagnostics, keep resource/work limits explicit,
  and require every planned case or stage before declaring completion.
- Before restarting a job, verify its existing process/session/job handle. Elapsed
  time, an observation timeout or stale logs alone do not establish termination.
- Keep compiled/executing/pinned C++ sources unchanged throughout a build or validation
  campaign. Verify the owned handle is terminal before editing those sources. Git
  snapshots and documentation updates are allowed without changing the pinned code.

## Adaptation architecture

User instruction, 2026-10-14: Euler-to-BL and BL-to-Euler describe validation
scenarios, never production transition modes. Mesh construction and removal must
follow the target metric, geometry authority and currently requested wall-height
constraints through general remeshing operators. Do not dispatch reconstruction
from Euler/RANS labels, an inferred input-mesh class, case names, saved coordinates
or fixture identities. Solver compatibility and solution-transfer checks remain
separate from mesh reconstruction. Wall-aware operators are allowed to enforce
actual geometric constraints; limited fixture success does not establish general
robustness. Qualify wider fields/geometries rather than adding scenario switches.

## Handbook maintenance

User instruction, 2026-10-14: keep HANDOFF_Codex.md current and concise. Put detailed
current topics and accumulated history into smaller linked handbooks when the entry
point grows. Preserve findings, failures and evidence; historical job/status notes
must not be mistaken for current state. Update the current state in place rather
than prepending repeated copies of prior updates.

## Resource discipline

User instruction, 2026-10-17: account for machine contention and RAM usage.

- Check actual host load and available RAM before sustained computation. Run one
  owned heavy job at a time; use low priority,2–4 build workers when resources permit
  (user update2026-10-29) and single-threaded
  libraries. Keep local MPI correctness checks bounded to available ranks/resources.
- Check live job handles before starting another job. Observe process memory during
  longer runs, preserve resource receipts, and leave unrelated processes untouched.
- Use the repository build's compile-tmp on the4TB disk while the root/tmp filesystem
  is nearly full. Do not infer whole-job peak memory from one process's RSS.

## Optional local second opinion

User authorization, 2026-10-18: consult the local Claude Code installation when
useful, using Opus 5.5 at medium or high effort. Keep source-checking and validation
with the implementation; a model review does not establish correctness.

## Staged adaptation and boundary mobility

User reaffirmation,2026-10-26: implement the already agreed validity-first construction
followed by metric/BL-aware quality improvement. q/J/size and requested first-height gates apply to the
final accepted mesh; intermediate meshes must remain valid. Fixed physical boundary
node coordinates are not required. Allow redistribution on the prescribed geometry
while retaining genuine features, body separation and requested BL constraints.
Complete the connected distributed workflow rather than accumulating strict local
construction fallbacks; preserve all final acceptance/transfer/restart obligations.


User clarification,2026-10-26: construction must omit q/J measurements, shape
ranking and quality-progress vetoes that are unnecessary for valid elements.
Exact positive, nondegenerate cells, topology, geometry and body separation remain
mandatory; J>=.001 is a quality threshold, not geometric validity. Measure and
improve shape in the subsequent stage and enforce all final acceptance gates.
Delegate bounded routine tasks to more token-efficient models; retain integrated
design, source review and acceptance assessment with the primary agent.
