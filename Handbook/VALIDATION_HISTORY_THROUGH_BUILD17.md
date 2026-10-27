# Archived validation narrative through build17

Archived on 2026-10-27 while build18 was running. All job/status/next-step wording
below is historical; use [current progress](../ADAPTATION_PROGRESS.md) and
[handoff](../HANDOFF_Codex.md) for live state. Evidence and failed attempts are
preserved here rather than mixed into the current result. Relative links in this
archived narrative were written from the repository root.

**DC-17 and DC-18 remain open. No full M6 adaptation has passed final acceptance.**

Validity-first construction, explicit shape-unavailable records, FIRST_ROW handoff,
measured provisional height admission and useful quality passes are implemented.
Earlier build8 passed with all933 source/build pins unchanged. Its saved M6
offline3 repair passed strict q/J/edge, prescribed interface and external contact: q=.2446273,
J=.0532005,edge=1.5983623 after1 move/12 trials. It still owes surface approximation;
this is a LOCAL repair witness, never full-mesh acceptance.

Build13 passed1069.09s with933 unchanged pins. Its connected campaign is incomplete:
exit134 after246.91s,44/46 reached cases pass,3978/3981 assertions. Both curved
runtime variants now finish Refine in~1.89s with173 cells and zero cached invalid
cells, using157work. Coarsen then spends~118s alternating removals and quality
reconstruction:292 removal publications versus279–280 quality patches, ending
118/128cells. Phase resets lose blocked-collapse state, and combined-pass progress
needs a proper criterion. No construction quality floor will be reinstated.
A separate new fixture abort is a missing cyclic index at the last ring segment.

Full M6 Euler `staged57/m6_runs/euler_build13`, session70428, is terminal REJECT:
1863.87s wall,1802.45s native, solver exit0/native false, all pins unchanged.
The six-rank run reached repair but never coarsening:774,314 private cells,
275,115 cached invalid cells and1,240,202 remaining repair demands after112,024
private publications. Time exhausted at1,218,977/19,414,976 work; original CFD
state retained. Sampled process-tree RSS maximum4.616GB. This does not establish
boundary pins as the cause. It does show the coarsening fix alone cannot close M6.
Build14/session8407 passed594.86s with933 unchanged pins. Connected_fix/session39461
finished25.65s:77/80 cases pass,18,339/18,502 assertions. Both curved remeshing stages
now complete; RANS transfer still fails. Two new fixture setups were invalid and
are corrected. An all-pair replay isolates non-finite clipping arithmetic: lost
volume.02111049961 falls to3.6e-16 with normalized signed-distance interpolation.
Shared correction and three regressions pass the build15 connected transfer/reload
checks. Build15 passed573.64s/933 unchanged pins; connected49438 is terminal82/83
cases (16,062/16,063 assertions), with only the large-star initial-quality premise
failing. Build16/session41647 corrects that fixture by approaching a face from
inside; no production code changed. Build16 passed105.17s/933 unchanged pins;
connected4342 PASS83 cases/19,836 assertions. Broad serial_full/session2882 is
terminal178/179 cases pass,60.69s,unchanged pins. All transfer/kernel/history/
curved lifecycle checks pass. Collision clearance exposed a paging regression:
ordinary proposals with no removed cells were incorrectly rejected. Build17/
session53915 restores this API while keeping nonempty coordinate-star admission;
build17 passed135.20s/933 unchanged pins. Broad serial77363 PASS179 cases/
1,121,211 assertions,61.68s,unchanged pins. MPI6/session97981 terminated on serial-only fixture selection; selector corrected
to use distributed counterparts (serial evidence retained). MPI6/session4734 PASS
43.79s:156 cases on each rank, unchanged pins. The integration milestone is
validated; full M6 acceptance remains open. Next: incremental quality-stage defect
trends and complete private-state capture under the [reviewed design](M6_QUALITY_STAGE_PROGRESS.md).
That diagnostics implementation is now integrated; build18/session62205 active,
933 source pins frozen,one worker. Focused invariant checks precede the bounded
four-quality-window capture on the exact build13 frozen M6 field. No convergence
change or new final acceptance claim. Transfer findings: [ledger](TRANSFER_OPERATORS_FINDINGS.md).
[Diagnosis and replay](TRANSFER_CLIPPING_UNDERFLOW.md). Full M6 quality closure is still a separate open capability.
Previous build12 rejected during construction on the4096 notification scan limit.

Latest pushed: `b33ec2ccb9` records build13 and connected-failure evidence. Code
checkpoint `536025f6e4` connects fair global repair scheduling, construction frontier
generations and atomic complete coordinate-star movement; it remains WIP.
No milestone is closed. See [staged implementation](STAGED_NATIVE_ADAPTATION.md).
Curved cross-chart mobility, distributed qualification and accepted full M6 remain
open. External-SSD references are copied locally and hash-verified:
[archive record](ADAPTATION_REFERENCE_INPUTS.md).

Validation54 is historical evidence predating the current code. Validation54 passes the full six-worker flat and cubic Euler/RANS workflows,
including conservative BDF2 transfer and SU2/CGNS reload. All 11 selected cases
pass on every rank. The curved workflow uses 1,975 of 4,096 work units: 1,750 for
refinement and 225 for coarsening. The flat workflow uses 105 total. Full demand
priority now precedes ownership in shared selection, dependency reservations and
peer contact selection; independent proposals still commit concurrently.

The focused priority tests also pass (4 cases, 31 assertions). Connected serial qualification passes 127 cases and 1,082,802 assertions.
Connected MPI qualification also passes: 118 cases on each of six ranks.
Full M6 Euler6 rejected at the native time limit: 24,972 private edits, 427,852
private cells, 235,717 cached invalid cells and 235,227 queued edges. It reached
six simultaneous commits, but no mesh or transfer was accepted. Original CFD
state was retained. Driver exit zero does not change that result.

Required gates remain q ≥ 0.20, J ≥ 0.05, metric edges ≤ 1.8, first height within
10%, geometry preservation, the configured 500k metric-complexity target,
conservative transfer and restart. The target is not a literal tetrahedron count.
Curvature uses `ADAP_HAUSD` in mesh units, independently of h0.

Work budgets count attempted proposals and repair trials, not generated cells.
The 4,096 limit quoted below belongs to the small regression workflows. Automatic
production work is bounded by `max(4096, 64 × initial cells)`; time, storage,
local-cavity and sampling limits separately bound resource use. These limits do
not replace the mesh-quality gates. Exhaustion means the search is unfinished,
not that the requested quality is impossible.

The supplied MPI audit request and Loseille/Alauzet/Menier paper were assessed
against the current code and validation54:
[assessment and next decisions](MPI_AUDIT_AND_LOSEILLE_ASSESSMENT.md).
Native already uses a separate weighted tetrahedron partition; M6 enabled it.
The paper's current/target metric mismatch model and hierarchical interface
recovery are candidates to evaluate, not implemented fixes. Its published quality
outliers do not establish our strict acceptance gates. No new benchmark or C++
change was made for this assessment; DC-17/DC-18 remain open.

Follow-up diagnostic55 is terminal (2026-10-26): see the linked
[decision package](M6_DIAGNOSTIC_DECISION.md), with construction/MPI/weight
reports and raw receipts. Original coordinates/global IDs and all 61,330 metric
tensors round-trip exactly. Weighted partition reduces proxy max/mean 4.699 to 1.08449;
offline current-formula sum matches production in aggregate only. The replica has
no 1024 clipping (maximum 591); production per-cell weights were not exported.
Matched work-2048 six-rank replays have 476 private edits/879 shape refusals and no
accepted mesh. Optional PMPI records 18.74–26.00 s/rank inside MPI over 62.72 s, with no
trace overflow; useful compute/wait decomposition and true instrumentation overhead
remain unresolved. Independent original-P1 replay reproduces 16 positive-volume shape
refusals among 28 captured patches. No repaired accepted witness exists. Production
code/gates unchanged. Next: the specific complete-context transaction experiment in
the decision package, not another full-M6 timeout or partition API replacement.

The supplied independent review of `f5e9456f06` is now
[reconciled and preserved](M6_REVIEW_RECONCILIATION.md). Corrected the
exemplar table to show failed-cell and full-patch q/J separately, and relabeled
the 1.428 timing quotient as hypothetical accounting rather than a speedup ceiling.
The next replay starts with edge23754–24333 only and requires exact pre-transaction
context before repair. Local transaction acceptance and final-patch edge acceptance
are distinct. No production fix, new numerical validation or capability closure is
claimed by this documentation checkpoint.

Diagnostic56 now establishes an accepted LOCAL replacement for its first M6 failure.
The capture retains303,428 current cells,70 events and722 verified dependencies;
all26 construction calls and11 shape refusals replay exactly. Fixed-wall repairs
fail, but changing the authoritative split parameter passes q=.24412,J=.05570,
maximum edge1.59836, curvature progress, prescribed interface and external contact.
Independent original-P1 measurement agrees; only the new wall point moved. This
proves local possibility, not a complete M6 mesh. The diagnostic rejection-message
fix passes5 tests on each of2 ranks. [Evidence](M6_TRANSACTION_REPLAY.md).

The user reaffirmed validity-first construction followed by quality improvement,
and allowed physical boundary-node movement. This was already a ledger decision;
production's forced Absolute admission remains an integration gap. Next work follows
[the connected staged implementation scope](STAGED_NATIVE_ADAPTATION.md):
geometry/BL-aware movement and topology optimization, complete distributed stars,
strict final acceptance and transfer/restart. No more isolated strict-construction
fallbacks. Curved chart restrictions currently pin some smooth original mesh nodes
and must not be mistaken for genuine feature constraints.
