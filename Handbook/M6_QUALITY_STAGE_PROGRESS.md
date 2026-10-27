# M6 quality-stage progress

Reviewed design and receipt, 2026-10-27. Measurement/capture implementation is
integrated in build18/session62205 (PASS726.79s/933 pins); focused serial
stage_checks/session45262 PASS10.5101s/6 cases/4419 assertions and MPI6
stage_checks6/session51405 PASS11.5294s. M6 capture/session11197 is TERMINAL
PASS623.327s, pins unchanged, with six complete rank shards, four quality windows
and unchanged original donors. No job remains live. Complete accepted M6 adaptation
remains the milestone.
See [staged adaptation](STAGED_NATIVE_ADAPTATION.md) and
[curved boundary mobility](CURVED_BOUNDARY_MOBILITY.md).

**Evidence and its limits.**

The completed diagnostic compares quality entry at round19016 with the deliberate
four-window observation stop at round19264. These are owned-cell sums; size debt is
cell-incidence weighted. Only248 rounds were observed, so this is an early trend,
not full convergence. The stop retained private state; it is neither exact
continuation nor final mesh acceptance.

| Measure | Quality entry | Observation stop | Change |
|---|---:|---:|---:|
| Owned cells | 664,744 | 680,869 | +16,125 |
| Invalid cells | 261,089 | 271,217 | +10,128 |
| q squared-hinge debt | 6,432.094 | 6,481.093 | +48.999 (+0.76%) |
| J squared-hinge debt | 11,227.554 | 11,193.364 | −34.189 (−0.30%) |
| Size squared-hinge debt (incidence weighted) | 903,864.555 | 892,325.825 | −11,538.730 (−1.28%) |
| Top-level moved / reconnected counters | 0 / 0 | 0 / 947 | Nested operations hidden; see below |

Thus size debt fell, q debt rose slightly, J debt fell slightly, while invalid-cell
count increased. Construction-invalid cells rose 204,496→214,273 and
quality-invalid cells 62,950→63,238. The diagnostic reports no height request for
this Euler run; it does not assess requested first-height behavior.

All 947 commits equal `optimization_validated`, rank by rank. They came from
`OptimizedRepairPatch`, whose internal insertions/moves were discarded from the
engine counters and classified as reconnected. The top-level zero insertion/move
counts therefore cannot establish which internal operators ran.
The complete shard comparison establishes 3,215 added interior vertices, no removed
entry vertices and no changed coordinate bits on retained entry vertices. All
33,310 physical vertices and physical parameter payloads remain unchanged, apart
from publication revision. Inserted points could have moved inside a private
optimizer call; the snapshots do not expose those transient operations.
Evidence: [topology comparison](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/m6_stage_20261027T094843_1705337/topology_comparison.json),
produced by the checked `staged57/analyze_stage_topology.py` decoder.

**Connected change in progress, not yet validated:** separate existing-point
relaxation, topology improvement and enrichment across the optimizer's current
bad-cell worklist. The old first-success order puts insertions before collapses
and smoothing, so repeatedly successful insertion can bypass those operators.
Reuse Smooth/SmoothBoundary and existing exact admission; reserve bounded work
for later enabled stages within the unchanged total budget. Newly inserted points
get relaxation opportunity on the next sweep. Publish nested operation counts
only with accepted outer transactions. This addresses an evidenced ordering
limitation; it does not establish M6 convergence or cross-chart mobility. Keep
minimum-score admission for now: the small combined q/J increase alone does not
justify a new aggregate gate. Validate the connected change on the captured M6
field early, then require the full accepted workflow under unchanged constraints.

The pinned build13 six-worker Euler run reached quality improvement and rejected
at its unchanged 1800-second native deadline. Solver exit 0 did not mean native
acceptance. Its receipt is
[diagnosis.json](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build13/diagnosis.json).
Across workers it retained 774,314 private cells, 275,115 cached-invalid cells and
1,240,202 repair demands after 112,024 private publications. Three final cells lacked
a completed census. Work was 1,218,977/19,414,976; coarsening was not entered.
These totals do not show whether whole-stage debt decreased. Final samples had
unavailable q/J, and do not establish that boundary pins caused rejection.
The Euler configuration requests no first height; height remains essential to the
general Euler/RANS workflow, but is not an evidenced cause of this run's failure.

Current admission summarizes shape by a cavity minimum:
`Common/src/adaptation/CNativeMetric3D.cpp`, `ValidateMetricChange`, compares
`min(minimum_q/.20, minimum_J/.05)`, or independently accepts size-excess progress.
Validity-only quality progress may add size debt. `GeometryRepair` relies on the
caller's certified geometry/height progress without a combined debt comparison.
In `CNativeEngine3D.cpp`, `repair_kind` uses worst height merit;
`MeshOptimizer::RowScore`, `MinScore` and `FreshScore` also use minima.
`MeshOptimizer::Smooth` already proposes a sum-of-shape-hinges Gauss–Newton step,
but candidate admission still requires improved minimum score.

Two algebraic counterexamples describe this certificate, not demonstrated M6 moves:

- Scores [.2,.4]→[.2,.8] improve total shape debt but leave the minimum unchanged.
- Raising one minimum from .2 to .200001 can pass while many other scores fall
  from .9 to .21, increasing total debt substantially.

This is a concrete objective limitation; its actual contribution on M6 is unmeasured.
Increasing limits or adding another local proposal variant does not diagnose it.

**Measure before changing admission.**

Reuse the existing one-cell quality census/update in `Adapt::measure` and
`Selection::Add/Drop`. Cache per-cell q/J bad flags and squared normalized hinges
`max(0,1-q/.20)^2`, `max(0,1-J/.05)^2`, alongside existing size/geometry validity.
Keep counts and raw sums; averages can improve merely by inserting good cells.
For size, a per-cell sum of six edge hinges is cheap and ownership-independent;
label it incidence-weighted. Summing rank-local unique-edge maps would duplicate
shared edges and must not be called a unique global edge total.

Return the already computed `FirstHeightReport` optionally from `height_check`;
reuse its measured error/footprint values. Reuse `Selection::Add`'s existing
`SurfaceError` evaluations for geometry diagnostics. Do not run a second audit.
Construction adds only known edge/geometry data. Unknown shape/height stays
explicit; no construction centroid imports or q/J evaluation are introduced.

Add new rows and subtract removed rows during normal cache updates. Reduce a fixed
scalar record only at quality entry, existing progress windows, handoffs and exit.
Report separate channel sums/counts, measured/unmeasured counts, mesh size, and
move/reconnect/insertion deltas. No per-proposal collective is needed for trends.
This first change must leave admission, election, limits and final gates unchanged.

**Capture and replay.**

Retain the complete owner-local private mesh at first quality entry and rejection,
with each cell's ID/version/physical binding and matching active parameter payload.
Reuse `EngineOptions::rejected_state`/`SetRejectedStateObserver` for rejection.
A borrowed quality-entry observer at the completed `enter_phase` census is the
small missing hook; it must run at a collective-safe point, perform no collectives,
and retain no borrowed objects. Use rank-local shards, not a whole-mesh gather.

Reuse `fixtures/native_transaction_capture.inc`, `NativeFrozenVolume3D`,
`NativeTransactionReplay3D`, and the pinned `staged57/replay.py` runner. Their
bounded transaction capture supplies old/fresh patches, dependencies and donor
samples. Its donor union is only sufficient for that transaction: full-stage
replay needs the complete immutable original donor shards, not captured query
samples or a field rebuilt from the rejected mesh.

Save original sensor/source hashes once, immutable authority/reference version,
active parameter payload, configuration and composition/normalization scale,
wall-height policy, breakpoints, worker membership/ownership, phase/round/work
and ID-generation offsets, binary/source/helper pins, and complete-shard hashes.
Existing callbacks can be installed by the remesher test harness; no solver-label
dispatch or production configuration switch is needed. Captures remain explicitly
private progress, with final acceptance false.

A cold replay of `Repair` currently starts construction; it is not exact quality
continuation. Replaying the first quality-entry transaction can reuse existing
facilities. Exact mid-pass continuation additionally needs repair tickets, blocked
support/frontiers and remaining controls; do not imply those are serialized today.
Capture bytes/I/O must be admitted and reported; truncation invalidates completeness.

**Acceptance and subsequent decisions.**

Diagnostic acceptance requires unchanged construction centroid/q/J query counts,
no new per-proposal collectives, and no duplicate metric/height/geometry audit.
Cache sums must match existing measured fixture records and changed-row deltas;
ownership permutations must give the same owned-cell totals. Partial census and
truncated captures must remain explicit. Saved transaction replay must reproduce
its measured candidate data against the same original frozen field.

Aggregate gating could itself reject necessary progress: topology may temporarily
increase bad-cell count, and quality/height movement may need size or geometry debt.
Volume weighting can hide a vanishing bad sliver; unweighted sums penalize necessary
subdivision; fixed channel priorities can starve another channel. A lower scalar
merit does not certify feasibility or final acceptance. Use captured before/after
candidates to establish the actual tradeoff before choosing a shared optimizer and
admission objective or a phase-level progress rule.

Ultimate acceptance still requires complete M6 Euler then RANS adaptation, strict
q>=.20/J>=.05, edges<=1.8, requested first-height tolerance (±10%), prescribed geometry/features,
body separation/contact, complexity 500,000 and conservative transfer/BDF/restart
checks under unchanged resource limits. A diagnostic or local repair is no substitute.

Implemented diagnostic: stage cache/borrowed observer, streamed owner and immutable
donor shards, bounded parameter payload, and explicit private manifest. The frozen
M6 run captured quality entry and stopped after four measured improvement windows,
saving `observation_stop` on every worker before the collectively elected
diagnostic exception. That deliberate stop is not infeasibility or final mesh
acceptance. Ordinary failure retains `rejected_exit` when the observer is healthy.
Function-valued composition/breakpoints and exact queue continuation are not
serialized. The observed mixed trend is too short to establish the cause of the
original timeout or justify an admission/coordination change by itself.

The build13 `restart_flow_adap_00000.dat` has SHA256
`c91d06b38e840aaada5a8ac417d1f937cdda4b5f5de7d587f7d17bbfc08e236b`,
identical to diagnostic55's pinned extraction input. All 61,330 coordinates, IDs
and sensor tensors match the existing `sensor_original.bin` bit-for-bit. Reuse
that source with build13's configuration/authority; no repeated sensor extraction
or field substitution is needed. `staged57/run_cases.py m6_stage` (with buildN as
its first argument) pins these inputs and verifies complete rank shards, record
lengths, matching immutable donor hashes and four trace windows. Its standalone
`test_stage_receipts.py` rejects missing/truncated shards and changed donor data.

**Conditional batching assessment (read-only, not implemented).**

Current improvement admits one concurrent candidate per worker; the flat
construction batching branch explicitly excludes repair. A steadily falling debt
trend with an occupied independent frontier would justify batching. Six parallel
commits and large donor time alone do not prove that this is the M6 bottleneck.
Flat/rising debt first requires operator/coordination diagnosis.

The smallest safe batching step is age-ordered repair selection, coalesced
priorities retained through Directory planning/reservation, and the existing
collectively ordered slot pipeline with parameter contexts created just-in-time.
Complete old-star vertex footprints establish independence; local visible support
is insufficient. Conflicts keep their age; rejected unchanged support stays
blocked; accepted features with remaining demand return to the queue tail.
Stable slot/ID mappings and actual support-sized memory admission remain required.

FetchBatch/ReserveBatch currently cap64 and omit coordinate-star mode even though
single-star lookup admits larger moves. Extend those contracts together, or serve
aged large stars as singletons without starvation. This first step amortizes only
frontier/metadata work, not private-trial donor and measurement collectives.

Amortizing donor work needs bounded coordinated trial waves, original-source union
queries retaining candidate mappings, contact keys including both rank and slot,
and common-revision parameter preparation plus merged publication. Rank-only
CollisionProposal grouping cannot distinguish simultaneous same-rank candidates.
Disjoint old vertices do not certify disjoint fresh geometry. Independently
prepared contexts become stale after a preceding boundary publication.

Qualify one-rank multiple stars, MPI ownership/fairness, mixed ordinary/80/320-cell
stars, peer success with one rejection, boundary revision/contact, empty ranks and
resource refusal using existing concurrent-repair/fairness/star tests. Compare
batch1/8 on captured M6 state under the same constraints and limits; retain source,
query/collective counts, debt trajectory and throughput. No batch implementation
or performance benefit is established by this review.
