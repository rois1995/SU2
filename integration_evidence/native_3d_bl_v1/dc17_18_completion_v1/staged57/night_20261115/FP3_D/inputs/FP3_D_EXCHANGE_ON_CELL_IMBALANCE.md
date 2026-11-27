# FP3-D: in mode S the ownership exchange is decided on the owned-cell ratio, not on the attempt rate

Author: reviewer, 2026-11-27 (reviewed by Codex, source facts by Codex). For the main agent. The user sends
it; it is a user order. Copy this file with its SHA256 into `R/night_20261115/FP3_D/inputs/`.
Code base: `186f9ab790` ("FP3-C: not adopted; the sweep gives 213 collapses"); its production and test
sources are those of `f7447473e5` (FP3-B, build676).
Short names: R, N, "mode S", OFF, CORE, "worker stage", engine file root as in the order FP3-A (your copy in
`N/FP3_A/inputs/`), called "A"; "B" = the order FP3-B and its record `N/FP3_B/accepted_reference.json` (the S
reference in force); "closed queue" = the live flag `selection.collapse_closed` of A is true (mode S after
step C completed; a metric cleanup opens it until Construction is entered again); "exchange" = an ownership
migration of the engine (`EngineOwnership::MigrateOwnership`).
"Owned-cell ratio" = the largest number of owned cells of a rank divided by the mean number of owned cells
over the participating ranks.

Scope. One production change in `EngineOwnership::ShouldMigrate`, active only with the closed queue. With
the open queue (OFF, CORE, wall heights, the lab control value `2`, and mode S after a metric cleanup opened
the queue) expect the rule of build676. One more lab value of the diagnostic variable (section 3). The
change alters the files of mode S: the acceptance is the two-run rule for S.

## 1. Why (measured on your runs; reviewer study `reviewer_handoff/scripts/s3/study_exchange_and_final_proof.md`)

The rule of the engine (`OwnershipExchangePays`, `Common/include/adaptation/CNativeEngine3D.hpp:23-32`):
remaining work = max(0, D - C) x measured / max(1, attempts); gain = remaining x (imbalance - 1); it pays when
imbalance > 1.25 and gain > 2 C. `imbalance` is the largest of three ratios (weighted demand, geometry cost,
quality debt). `measured` counts every census measurement (the seed census and the entry census are two full
passes over the cells); `attempts` counts attempted edits. Since A the engine makes few attempts in mode S,
so the ratio measured / attempts is not a rate of the work any more. Rank-0 rows (printed imbalance / gain /
attempts -> moved cells):
- event 2, quality entry: P3 `1.649 / 309,300 / 30,495` -> no exchange; B `2.644 / 1,245,511 / 112` ->
  165,441 cells. In B the weighted demand ratio there was 1.185 to 1.188 and the owned-cell ratio 1.186; the
  printed 2.644 is the quality-debt ratio (a debt sum of 6.27 units over six ranks). The exchange made the
  cell ratio 1.066 and the debt ratio 1.146; its net effect on the time is not known. After it the
  final-proof union work was 42.4 M against 12.8 M (P3, event 2, no exchange), with 42.0 % memo hits against
  84.5 %. A migrated cell carries its census record; the geometry memo and the prepared facets stay on the old
  rank (`EngineOwnership.cpp:298-310, 343-370`). The logs do not attribute each miss to a moved face.
- event 1: the hand-over has the cells 28,066 / 32,975 / 86,374 / 166,771 / 31,346 / 26,293 for the six ranks
  (owned-cell ratio 2.691). A exchanged at the initial collapse exit (202,212 cells, ratio then about 1.09)
  and again at the quality entry (39,423 cells, by the degenerate rate). B exchanged one time at the quality
  entry (216,475 cells).
- event 2: the owned-cell ratio at the hand-over was 1.24 (P3), 1.34 (A), 1.19 (B).

| Assumption | State |
|---|---|
| The exchange of event 2 at the quality entry is decided by the degenerate ratio and the debt term, with nearly balanced cells | MEASURED (rows above) |
| An owned-cell ratio above 2 separates the two events of this case (2.69 against at most 1.34) | MEASURED on this case only. The value 2 is a design constant of this order (one rank owns more than two times the mean), not a derived threshold. With two ranks the ratio cannot be above 2: the rule then makes no exchange with the closed queue. It ignores a pure quality-debt imbalance |
| How many exchanges the new runs make, and where | NOT KNOWN (an observation). Expected from the rows of A: event 1 one exchange at the initial collapse exit, event 2 none |
| An exchange before the split generations and the entry census is at least as good as one at the quality entry | NOT KNOWN. The logs do not establish a speed win. Reported by steps 2 and 4; no gate |
| The final proof of event 2 gets its work and memo hits back | NOT KNOWN. Reported |
| Two runs S give the same files | NOT KNOWN for this change. Gate of step 3 |

## 2. The production change

In `EngineOwnership::ShouldMigrate` (`EngineOwnership.cpp:242-273`): keep the existing raw model, its
statistics and its reductions exactly as today. The effective policy of an evaluation is one of:
- `raw`: the queue is open: the effective `pays` is the raw result of `OwnershipExchangePays` (build676);
- `fp3b`: the closed queue with the lab value `3` (section 3): the effective `pays` of build676 with the
  closed queue (the raw result with the veto of B for `quality` false);
- `cells`: the closed queue otherwise: reduce the current `working.size()` by ONE additional integer MAX
  over the ranks and take `global_cells` from `PhaseOwnership`; the effective `pays` is
  `world.size * max_owned_cells > 2 * global_cells`, compared without integer overflow, for `quality` false
  AND true.
Take the additional reduction ONLY on the `cells` path, which all ranks select together (so the collective
calls and timer counts with the open queue do not change). Keep `DiagnosticAlways`, the vote and all guards of
`PhaseOwnership` (one construction evaluation for each worker stage; the cool-down of the quality sites).
Print: leave the existing row `NATIVE3D_PHASE_OWNERSHIP ... evaluate=1 ...` unchanged for workers started
without `context.options.close_construction_collapse` (OFF, CORE and the other open-queue workers: no new
field). For workers started with that option, append `policy=<cells|raw|fp3b>`. For `cells`, also append
`max_owned_cells=<integer>` and `owned_ratio=<value at round-trip precision>`. The gate reads the existing
integer field `global_cells` of that row. Print that field exactly one time.
No other production change except section 3.
Hidden MPI cases (G1 keeps its selection): with at least THREE ranks, Automatic ownership, timings on, the
`cells` policy: assert the measured ratio and the raw result first, then the decision: (a) ratio above 2
with the raw result false: `migrate=1`; (b) ratio below 2 with the raw result true: `migrate=0`; (c) ratio
equal to 2: `migrate=0`. With the open queue: the raw outcomes for the same inputs. With the value `3`: the
outcomes of B. Use separate processes for the settings that change `FP3Probe()`. In the inherited two-rank
fixture of B compare the legacy fields as today and test the new policy fields apart.

## 3. The diagnostic variable (lab only)

Add the exact value `3` to `SU2_NATIVE_FP3_PROBE`. Only the value `1` enables the core probe. Only the value
`2` opens the engine queue and enables the engine probe. The value `3` keeps the ordinary queue state of
mode S, uses the ownership policy `fp3b` and prints no probe row. It is an authorized LAB-ONLY control: it
gives no acceptance and no reference. The value `0` is mandatory for all acceptance launches and restarts,
verified and receipted on every rank as in A.

## 4. The work, the gates and the labels

A new build number and receipt folders under `N/FP3_D/`. One frozen binary pair. One owned heavy job; the host
preflight of R1 before each launch. Adapters: copies of the adapters of B (test code, R3) bound to this order
and the base `186f9ab790`; the runner rules of A and B hold (the one-key exclusion for G3 / C492; each S
trial launched and sealed by itself; seals of THIS order as prerequisites). B's launcher knows only `probe2`
and B's collector expects an engine probe row for a nonzero value other than `1`: in the copies, the value
`3` is a control with ZERO probe rows; its launcher needs the sealed S run 2 and its terminal restart, sets,
exports and receipts the value `3` on every rank, authenticates both accepted S run receipts of B and uses
their common six hashes for the comparison.

Step 0. One R4 source check of this text.
Step 1, the paths with the open queue (closure default, probe `0`):
1. G1 identical to the reference; the hidden cases.
2. G2 and GQ against the accepted P3 captures with the unchanged comparator: equal.
3. The five shared checks as in A step 1 item 3; G3 and G4 as in B step 1 item 4.
Label: "FP3-D: default paths unchanged", or "... FAILED", or "... OPEN". Not evaluated (write it): the CORE
and OFF full workflows.
Step 2, S run 1 with its isolated restart (after step 1 passed). Gates: all standing gates of both events;
the suppression gate of A, unchanged; and the EXCHANGE gate, on the worker packets of B's collector:
- as in B: in each completed packet exactly ONE construction evaluation (`quality=0 evaluate=1`), after the
  initial `length_collapse_exit` and before the first split-stage row or the fallback boundary, with
  `attempts=0`. The old requirements `migrate=0` and "no construction commit" of B do NOT apply any more;
- the expected policy, found independently of the printed one: each packet of an acceptance run (value `0`)
  starts with `cells`; a `metric_cleanup_entry` switches the expected policy to `raw`; the next entry into
  Construction restores `cells`. Each evaluation row names the expected policy, and its `migrate` equals the
  predicate of that policy computed from the recorded inputs (`cells`: `ranks * max_owned_cells > 2 *
  global_cells` with the printed integers; `raw`: the raw result). The other sites that return from a guard
  before the evaluation owe no evaluation row and no commit;
- commits: match each `committed=1` row, in order, to the unpaired evaluation directly before it with the
  same `quality` flag in the same packet. In a worker stage that passed its workflow result checks, a
  positive evaluation has exactly one matching commit and a negative evaluation has none. An `exit` row alone
  does not establish success: when the migration or the workflow fails, preserve it and classify it under the
  standing result gates;
- across the ranks: the same ordered sequence of evaluations and commits, and agreement on the collective
  fields (policy, decision, the cell-count inputs, `moved_cells`). The rank-local fields (seconds, work
  weights, bytes, the cells after the migration) are a report. Count `moved_cells` one time for each
  exchange.
A watchdog, or missing, cut or malformed evidence, follows the measurement and watchdog rules of A; complete
contrary evidence is a result failure.
Report (no gate), beside the S pair of B and the P3 pair: the ownership rows of each event (site, policy,
owned ratio, printed imbalance, raw gain, `migrate`, moved cells, seconds, bytes); cells for each rank at the
seed census and at the quality entry; split generations and their seconds; for each rank the delta of the
construction `census` timer between the construction-entry snapshot and the first later snapshot (name the
two boundaries), and apart the wall interval from the construction entry to the evaluation; quality seconds;
final-proof reverse and share seconds, union work and memo hits; native maxima and phase maxima with the host
load.
Step 3, S run 2 (same pair, inputs, settings) with its restart: the gates of step 2, and the six files equal
in the two runs. Then they are the new S reference.
Step 4, a time observation (after step 3; no gate, no acceptance): one run S with `SU2_NATIVE_FP3_PROBE=3`
after the normal host preflight. Its six files against the common six hashes of B (equal expected). Report
its time stamps, host load, native maxima and phase maxima beside the two S trials of this order. Name it
"the exchange rule of FP3-B". The times are observations under a recorded load that was not controlled. A
mismatch, a failure or a timeout of this observation is reported apart and does not prevent the publication
after steps 1 to 3 passed.

Acceptance (when steps 1 to 3 passed): publish `N/FP3_D/accepted_reference.json` (`workflows.S` in the form of
B) and append to `AGENTS.md` section 2a: "- Since FP3-D the S reference is the `workflows.S` entry of
`R/night_20261115/FP3_D/accepted_reference.json` (with the closed collapse queue the ownership exchange is
made when the owned-cell ratio is above 2, at each evaluation site). It replaces the S entry of FP3-B for
ordinary S runs. The FP2-P3 record stays for OFF, CORE, G2, GQ, G3 and `saved_current`; its S entry stays the
reference of the lab control value `2`; the S entry of FP3-B is the reference of the lab control value `3`."
Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`, `Handbook/ADAPTATION_TIMINGS.md`. Label:
"FP3-D: accepted; exchange on the owned-cell ratio in mode S". Report `N/FP3_D/reports/FP3_D_<build>.md` and
`.json`. Then STOP.

Failures: the paragraph "Failures" of A holds for each required result gate of steps 1 to 3, with "the
production change of FP3-D" for "the connected production change of FP3-A": a result failure that stays
reverts ONLY the change of this order with its dependent test code in the same commit (the tree must build);
label "FP3-D: FAILED; the exchange rule of FP3-B stays". The rules R1, R3, R5, R6, R7, R8, R9, R10, R11, R12,
R13 hold. Expected machine time: about 2.5 hours.

## 5. Not in this order

A change of `OwnershipExchangePays` or of the rule with the open queue. An exchange outside the engine (at
the hand-over, before the seed census). A change of the event-start partition. Proof records or memo entries
that travel with a cell. The prepared-facet pool. A time gate.
