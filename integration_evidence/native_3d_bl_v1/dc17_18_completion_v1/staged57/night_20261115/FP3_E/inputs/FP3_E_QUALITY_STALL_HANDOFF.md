# FP3-E: in mode S a quality step whose debt stalls requests the hand-off to construction

Author: reviewer, 2026-11-27 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/FP3_E/inputs/`.
Code base: `c53e7dbce2` ("FP3-D: accepted; exchange on the owned-cell ratio in mode S"), build678.
Short names: R, N, "mode S", OFF, CORE, "worker stage", engine file root (`Common/src/adaptation/native_engine3d/`)
as in the order FP3-A (your copy in `N/FP3_A/inputs/`), called "A"; "B" and "D" = the orders FP3-B and FP3-D
and their records; "closed queue" = the live flag `selection.collapse_closed` (as in D); "the S reference" =
`N/FP3_D/accepted_reference.json` `workflows.S` (in force). "Window" = one observation of
`RoundHead::CheckStagnationWindow` (`EngineControl.cpp:1581-1631`): the block that runs when
`stats.rounds > phase_start_round && round && round % WindowRounds(options, repairing) == 0` (8 rounds in the
quality step of these runs). "Quality step" = the engine phase `Improvement` entered by `EnterPhase`
(`EngineControl.cpp:646-690`) with `repairing` true and `metric_cleanup` false.

Scope. One production change in the round head of the engine, active only with the closed queue. With the open
queue (OFF, CORE, wall heights, the lab value `2`, mode S after a metric cleanup opened the queue) nothing
changes and no new collective is made. One more lab value of `SU2_NATIVE_FP3_PROBE` (section 3). The change
can alter the files of mode S: the acceptance is the two-run rule for S.

## 1. Why (reviewer studies, read-only, on your logs)

Studies: `reviewer_handoff/scripts/s4/study_per_step_balance.md` and
`reviewer_handoff/scripts/s5/study_quality_tail.md` (with `out/`), offline replay
`reviewer_handoff/scripts/s5/replay_stall_rule.py` -> `out/replay_stall_rule.txt`.

- D S1 / S2, event 1: the quality step runs rounds 369-1063. In rounds 469-1063 (595 rounds, 64.6 s wall) only
  rank 3 makes choices (1 choice in 537 rounds). The other five ranks wait in `join_wait` for 54 s. Rank 3
  cycles on the neighbourhood of one sliver at the wing leading edge (cell 3430297, q 0.026, J 0.0047): its
  debt does not change; the 165 commits of the tail remove 277 and create 276 bad cells and remove 0.64 of
  debt (106.8 in the 95 rounds before). Each commit reopens the sliver features (`TouchRepairSupport`,
  `EngineSelectionQueues.cpp:366-383`); they are refused again (6,237 topology trials, 1 certified).
- The step ends only because rank 3's queue drains (`!any`); then `ReturnToConstruction`
  (`EngineControl.cpp:1200-1231`) hands the residual debt to construction. One surface split removes the
  sliver, and 6 quality rounds and a geometry repair close all debt in about 4 s.
- No stop rule of the quality step sees this. The stagnation window stops only when no rank changed its
  size, quality, removal or geometry progress counters AND no rank attempted anything (or deferred a length
  edit) over the window (`EngineControl.cpp:1598-1626`). A repair commit counts as quality progress when its
  patch score rises by more than 1e-7, or under `ValidityOnly` when its debt falls
  (`Common/src/adaptation/CNativeMetric3D.cpp:532-545, 575-588`). One attempt per round on one rank keeps the
  step alive. `maximum_rounds` is 0; the work budget is 1 % used.
- The tail depends on the partition: A S2 entered the quality step with the same aggregates (cells, geometry
  bad cells, debts equal to 10 digits) and handed over after 123 rounds. probe3, B and P3 have no stuck cell.
- Offline replay of the rule of section 2 on the window debts of seven runs (D S1, D S2, D probe3, B S1, B S2,
  A S2, P3; both events): it fires only in D event 1, at window round 496 (27 rounds after the tail began). The
  largest ratio `best(k) / best(k-4)` in all other quality steps is 0.716 (A S2), 0.203 (P3), 0.167
  (probe3, B); the threshold is 0.95.

| Assumption | State |
|---|---|
| The D tail is a cycle on one stuck neighbourhood with no debt fall | MEASURED (study s5, Q2) |
| The rule fires in D event 1 at round 496 and nowhere else in the seven runs | MEASURED by offline replay on printed window debts, carried forward between printed rows; the engine evaluates the exact census values |
| An early hand-off (round 497 instead of 1064) closes the debt as cleanly as the late one | NOT KNOWN. The debt is larger then (rank 3 q_bad 4-5). The standing gates of steps 2 and 3 decide |
| The construction pass after the early hand-off ends in the same number of extra quality rounds | NOT KNOWN. Reported |
| Event 2 is unchanged against D | expected (no event-2 quality step has more than 3 windows); the six files can still change because event 1 changes |
| The gain is about 55-60 s of event-1 native time | NOT KNOWN (an estimate: the 64.6 s tail minus the hand-off that happens anyway). Reported by steps 2 to 4; no gate |
| The thresholds (4 windows, factor 0.95) | design constants of this order, checked by the replay; not derived |

## 2. The production change

A "debt stall" test inside `CheckStagnationWindow` arms at most one hand-off request for each engine
invocation; `ReturnToConstruction` reads the request through its unchanged outer guards.

1. Enabled worker. The test can be enabled only in a worker started with
   `context.options.close_construction_collapse` and with `SU2NativeLocal3D::FP3Probe()` neither `FP3B`
   (value `3`) nor the new value `4` (section 3). These inputs are rank-uniform: the option is a worker option,
   and the launcher verifies and receipts the same probe value on every participating rank (as in A). A worker
   that cannot enable the test adds no state use, no collective, no timer call and no row.
2. State (worker control, one engine invocation): `stall_ring` (four `double`), `stall_count` (observed
   windows of the current phase), `stall_best` (double; `-1` = no observation in this phase),
   `stall_request` (bool), `stall_used` (bool). `stall_used` and `stall_request` are false at the start of the
   invocation; `stall_used` is never reset in it. Every `EnterPhase` call resets the observation history
   (`stall_ring`, `stall_count`, `stall_best`) and clears `stall_request`. Size repair and geometry repair do
   not call `EnterPhase` and do not clear the request. The state is plain values with no allocation. Charge
   its fixed bytes through the existing modeled-storage admissions ONLY for workers enabled by item 1, for the
   lifetime of that invocation; a worker that is not enabled keeps the modeled bytes of build678 (so
   `native_peak_bytes` of G2 / GQ does not change). An enabled worker keeps its state and charge during a metric
   cleanup, but while the live queue is open it prints neither stall row kind and makes no new collective or
   timer call. A metric cleanup that starts while a request is pending preempts it (report it); the later
   `EnterPhase` clears the request and leaves `stall_used` true.
3. Active window. A window is ACTIVE when the worker is enabled, the live queue is closed
   (`selection.collapse_closed`), the phase is `Improvement`, `repairing` is true, `metric_cleanup` is false,
   and `stall_used` is false. All these values are equal on all ranks of the worker at the window (the agent
   confirms each in the R4 check; a value that is rank-local is a contradiction under R4, not a reading to
   choose). The test runs in an active window after the existing `PhaseOwnership` call returned true and before
   the existing stop test (`!mesh_progress && !search_progress`). A round head that ends at the `PhaseOwnership`
   call (it returned false) or earlier makes no test and no row. The ownership cool-down (a `true` return
   without an evaluation) does not end the head.
4. Observation. On each rank take `p = selection.Progress(working.size())`. Local validity:
   `p.cache_consistent && p.census_complete && p.shape_complete`. Make ONE collective: the array form
   `CPassiveComm::Allreduce(send, recv, 3, CPassiveComm::Op::SUM, world.comm)` of three doubles on the stack
   (`double(p.q_hinge_squared + p.j_hinge_squared)`, `double(p.q_bad_cells + p.j_bad_cells)`, `valid ? 0 : 1`),
   inside a `SU2NativeTiming3D::Scope` of the operation `Communication`, as `TimedAllreduce` does
   (`Common/src/adaptation/CNativeEngine3D.cpp:281-285`). No allocation and no admission. The global debt `G`
   is the summed first value (the engine already takes decisions from a double SUM, e.g. `EngineCensus.cpp:94`;
   the SUM gives every rank the same value in one call; the two-run gate checks workflow-file repeatability); the global bad count `N` is the second value (exact below
   2^53); the window is OBSERVED when the third value is 0. An unobserved window changes no state.
5. Predicate, for an observed window: `stall_best = G` if `stall_best < 0`, else `min(stall_best, G)`;
   increment `stall_count`; let `B4` be the oldest ring entry when `stall_count > 4` (the `stall_best` of the
   observed window four observed windows before), else none. When `B4` exists and `N > 0` and
   `stall_best > 0.95 * B4`: set `stall_request` and `stall_used` to true (a fire). Then put `stall_best` in
   the ring, replacing the oldest entry. Write the predicate as one small pure function with a comment that
   names this order (readable code, D 7.1a).
6. Hand-off. In `ReturnToConstruction` (`EngineControl.cpp:1200-1231`) replace the last clause
   `((... ordinary clause ...) || !any)` by `((... ordinary clause ...) || !any || stall_request)`. The outer
   guards stay unchanged and in the same order (the metric-cleanup guard, the phase, the improvement change, the
   construction-invalid or standalone-coarsening demand), so the collectives of the rule keep their order and
   count. Each later round head that reaches `ReturnToConstruction` tests the pending request through these
   guards. An existing stop or another phase entry earlier in the head order may preempt it; then
   `EnterPhase` clears the request and, because `stall_used` is true, no new request can arm in this
   invocation. The `refine_geometry` choice and the `quality_exit` observation of the rule stay as they are.
7. No other production change except section 3.

Print (under `options.timings` only; no decision reads it):
- In each active window that reaches the test, on each rank, one row `NATIVE3D_QUALITY_STALL round=<the
  completed-round number stats.rounds> observed=<0|1> debt=<G> bad=<N> best=<stall_best after the update>
  best_before=<B4 or -1> count=<stall_count> stalled=<0|1>`, doubles at round-trip precision. On an unobserved
  row: `stalled=0`, `best_before=-1`, `count` and `best` unchanged (`best=-1` when there was no observation in
  this phase), and `debt` / `bad` print the summed values as they came. All fields are collective (equal on all
  ranks).
- In each round head that reaches `ReturnToConstruction` while `stall_request` is true and the live queue is
  closed, on each rank, one row
  `NATIVE3D_QUALITY_STALL_HANDOFF round=<stats.rounds> taken=<0|1> by=<ordinary|no_candidate|stall|none>
  failed=<metric_cleanup|phase|improvement|demand|none>`: `failed` names the first outer guard that was false
  in the existing short-circuit order; `by` names the first true member of the last clause. Use only the
  outcomes that the rule already evaluated; make no new reduction (restructure the expression into named
  values that are evaluated in the same order with the same short-circuit). `taken=0` prints `by=none` and a
  named `failed`; `taken=1` prints `failed=none` and a named `by`. The row is printed before the existing
  `ObserveProgress(..., "quality_exit")` call.

Hidden cases (G1 keeps its selection):
(a) the pure predicate on fixed series: fires at the first observed window where `stall_best > 0.95 * B4`
with `N > 0`; no fire with `N = 0`, with fewer than five observed windows, or with a ratio of exactly 0.95; an
unobserved window between observed ones changes nothing;
(b) MPI, at least two ranks, timings on: one rank with complete cell coverage but incomplete shape coverage
makes the window unobserved on all ranks; and an all-valid input with known positive debts and bad counts gives
`observed=1` with the known sums of `debt` and `bad` on all ranks;
(c) the enabled and active guards: with value `3`, value `4`, the open queue, or `stall_used` true there is no
row and no collective (equal collective and timer counts against a run without the test);
(d) the hand-off takes the branch with `any` true, the ordinary clause false, every outer guard true and
`stall_request` true (`by=stall`);
(e) a false outer guard blocks the pending request (`taken=0`, `failed=` that guard), and the request stays
pending until `EnterPhase`.

## 3. The diagnostic variable (lab only)

Add the exact value `4` to `SU2_NATIVE_FP3_PROBE` (enum value after `FP3B`). Value `4` keeps the ordinary
queue state and ownership policy of mode S (as value `0`), turns off ONLY the debt stall test of section 2, and
prints no probe row: it is the behaviour of D. Value `3` also turns off the test (it stays the behaviour of B).
Values `1` and `2` keep their meaning. Lab-only: it gives no acceptance and publishes no reference. The value `0` is mandatory for
all acceptance launches and restarts, verified and receipted on every rank as in A.

## 4. The work, the gates and the labels

A new build number and receipt folders under `N/FP3_E/`. One frozen binary pair. One owned heavy job; the host
preflight of R1 before each launch. Adapters: copies of the adapters of D (test code, R3) bound to this order
and the base `c53e7dbce2`; the runner rules of A, B and D hold (the one-key exclusion for G3 / C492; each S
trial launched and sealed by itself; seals of THIS order as prerequisites). In the copies the value `4` is a
control with ZERO probe rows and the expected ownership policy `cells` (as value `0`); its launcher needs the sealed S run 2 and its terminal restart, sets, exports
and receipts the value `4` on every rank, authenticates both accepted S run receipts of D and uses their common
six hashes for the comparison.

Step 0. One R4 source check of this text.
Step 1, the paths with the open queue (closure default, probe `0`), as in D step 1:
1. G1 identical to the reference; the hidden cases.
2. G2 and GQ against the accepted P3 captures with the unchanged comparator: equal.
3. The five shared checks as in A step 1 item 3; G3 and G4 as in B step 1 item 4.
Label: "FP3-E: default paths unchanged", or "... FAILED", or "... OPEN". Not evaluated (write it): the CORE and
OFF full workflows.
Step 2, S run 1 with its isolated restart (after step 1 passed). Gates: all standing gates of both events; the
suppression gate of A, unchanged; the exchange gate of D, unchanged (a new `quality_exit` -> construction entry
is an ordinary entry into Construction for its expected-policy rule); and the STALL gate. Extend D's scanner and seed-packet builder to retain `NATIVE3D_QUALITY_STALL` and
`NATIVE3D_QUALITY_STALL_HANDOFF` rows; pair the rows of the ranks in packet order by `round`.
- Row presence: in a successful packet, one `NATIVE3D_QUALITY_STALL` row on each rank for each window that
  reaches the test of section 2 item 3 (the post-ownership point) while the live queue is closed, the quality
  phase is active (`Improvement`, repairing, no metric cleanup) and the packet has not fired. The expected
  queue state is found independently of the rows: a packet of an acceptance run starts closed; a
  `metric_cleanup_entry` opens it; the next `construction_entry` closes it again. Zero rows after the first
  fire of the packet and zero rows on any inactive path. For a cut terminal packet, apply A's evidence rules to
  a window cut off before the post-ownership point.
- Across the ranks: the same ordered rows, equal in all fields.
- Inputs: for each expected stall row derive `observed` and `bad` independently from the rank-local
  `NATIVE3D_STAGE` rows of the same completed round: `stage=window` on a window without a quality migration,
  `stage=ownership_committed` at the same round after a successful quality migration (retain that row in the
  collector). `observed` must equal the conjunction of `cache_consistent`, `census_complete` and
  `shape_complete` over all ranks; `bad` must equal the integer sum of `q_bad + j_bad` over all ranks. For an
  observed row, `debt` must equal the sum over ranks of `q_debt + j_debt` within a relative 1e-12 (report the
  largest difference).
- Arithmetic: the checker derives, from `observed`, `debt` and `bad` alone, in each phase (a phase entry
  resets the history only), the observation count, the running minimum, the ring and `stalled`, and compares
  each derived value with the printed `count`, `best`, `best_before` and `stalled` (doubles compared as
  printed, round-trip).
- At most one `stalled=1` row in each packet (one engine invocation).
- Hand-off rows: they appear only between a fire and the next phase entry of the packet, with the queue
  closed. In a successful packet, while a request is pending with the queue closed, each `quality_exit` stage
  row has exactly one preceding `taken=1` row of the same completed round on each rank, and each `taken=1` row
  matches that exit. `taken=0` rows have `by=none` and a named `failed`; `taken=1` rows have `failed=none` and a
  named `by`. In a partial or failed packet, an unmatched terminal `taken=1` follows the standing result and
  evidence rules of A. When there is neither a hand-off row nor a matching exit after a fire, report that no
  hand-off call was recorded (the existing rows cannot prove that every blocked call was logged).
A watchdog, or missing, cut or malformed evidence, follows the measurement and watchdog rules of A; complete
contrary evidence is a result failure.
Report (no gate), beside the S pair of D: for each event the stall rows (first and last window of each quality
step, the fired row); the `NATIVE3D_QUALITY_STALL_HANDOFF` rows (round, `taken`, `by`,
`failed`), or that no hand-off call was recorded before the next phase entry; the quality steps of each event
(entry and exit rounds, rounds with choices per rank, the longest run of rounds where one rank is the only
active rank and its choices and busy seconds per round; the `join_wait` of the other ranks over the
covered interval of their census reports, named, and the exact span total only when those boundaries align); the construction pass after the
hand-off (stages, rounds, splits, seconds) and the debt at each later entry; native maxima and phase maxima
with the host load. Expected from the replay (report only): event 1 fires at round 496 (best 2.643946, four
windows before 2.776760, ratio 0.9522) and, if the existing guards hold, the next head that reaches the call hands over; event 2 does not
fire. A different firing or exit round is a report, not a failure.
Step 3, S run 2 (same pair, inputs, settings) with its restart: the gates of step 2, and the six files equal
in the two runs. Then they are the new S reference.
Step 4, a time observation (after step 3; no gate, no acceptance): one run S with `SU2_NATIVE_FP3_PROBE=4`
after the normal host preflight. Its six files against the common six hashes of D (equal expected). Report
its time stamps, host load, native maxima and phase maxima beside the two S trials of this order. Name it "the
quality step of FP3-D". The times are observations under a recorded load that was not controlled; call them
"same day" only when the time stamps say so. A mismatch, a failure or a timeout of this observation is
reported apart and does not prevent the publication after steps 1 to 3 passed.

Acceptance (when steps 1 to 3 passed): publish `N/FP3_E/accepted_reference.json` (`workflows.S` in the form of
D) and append to `AGENTS.md` section 2a: "- Since FP3-E the S reference is the `workflows.S` entry of
`R/night_20261115/FP3_E/accepted_reference.json` (with the closed collapse queue, a quality step whose best global
debt falls less than 5 % over four observed windows arms at most one hand-off request per engine invocation,
subject to the existing construction guards). It replaces the S
entry of FP3-D for ordinary S runs. The FP2-P3 record stays for OFF, CORE, G2, GQ, G3 and `saved_current`; its S
entry stays the reference of the lab value `2`; the S entry of FP3-B is the reference of the lab value `3`; the
S entry of FP3-D is the reference of the lab value `4`." Update the progress file, `HANDOFF_Codex.md`,
`HowToProceed.md`, `Handbook/ADAPTATION_TIMINGS.md`. Label: "FP3-E: accepted; a stalled quality step requests
the hand-off to construction in mode S". Report `N/FP3_E/reports/FP3_E_<build>.md` and `.json`. Then STOP.

Failures: the paragraph "Failures" of A holds for each required result gate of steps 1 to 3, with "the
production change of FP3-E" for "the connected production change of FP3-A": a result failure that stays
reverts ONLY the change of this order with its dependent test code in the same commit (the tree must build);
label "FP3-E: FAILED; the quality step of FP3-D stays". The rules R1, R3, R5, R6, R7, R8, R9, R10, R11, R12, R13
hold. Expected machine time: about 2.5 hours.

## 5. Not in this order

A change of the stagnation window, of the 1e-7 progress rule, of `TouchRepairSupport`, of the repair batch or
of the round join. A per-rank stop or a round without a global join. A change with the open queue. A second
stall hand-off in one engine run. A time gate.
