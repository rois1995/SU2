# Metric-length satisfaction: proposed staging and M6 cost evidence

Design record, 2026-10-27. The connected scheduling candidate is applied; build69 focused checks pass, but curved runtime and full construction remain unqualified.
The geometry-closure candidate is a separate connected change. DC-17/DC-18 remain open.

Build65 was owner-cancelled (51.1455s, exit 130, unchanged pins; all 22 processes gone). Source66 fixes secondary pass-deferral handling and mode agreement; direct footprint and mismatched-entry MPI tests and a three-component joint-coarsening fixture await validation. Next are focused serial/MPI6, production CFD and early M6 length diagnostics.

Build69 PASS1198.313s, 1293 pins unchanged; all owned build processes exited.
Audit passes 23 selections (atlas 63, serial 252, MPI 229). Atlas checks pass
63 cases/12,847 assertions in 132.018s, including actual smooth/curve/unpublished
endpoint collapses. Mapped runtime FAIL10.806s: curved flows reached 14 splits,
zero collapses, 136 rejections/274 geometry trials and the unchanged 67,207,168
arithmetic cap (global work 744); 153 early constraint filters and 19 orientation
vetoes in 420 checks. Flat workflow passes with 31 edits (27 splits, 4 collapses;
work 282). The connected construction blocker and accepted M6 remain open.
M6 source69 `euler_build69_incident_collapse` terminated rejected after 1876.115s (solver exit 0, pins unchanged; all nine owned processes gone). Each rank reports zero completed length split/collapse passes yet one quality stage: a stagnation-window branch entered quality while pass-deferral scans changed neither attempts nor mesh progress. There were 289,853 directory elections per rank and 66,365 deferred candidates summed across ranks. Sampled process-tree maximum RSS was 3,757,772,800 bytes, not an allocation peak. The initial working mesh had 303,359 cells; 146,862 is source-envelope metadata. The next connected MIS bulk-retirement and lifecycle-gate design addresses this without weakening quality gates or caps. No M6 acceptance or transfer follows.


Committed pass claims currently retire selected deferred edges lazily, one at a time:
`CNativeEngine3D.cpp` ServiceEdge:605, service_frontier:870 and deferral paths
:2151/:2234/:2333. Directory NotifyChanges:1104 sends actual changed vertices to
prior/new row owners; Engine Touch:3830 uses the notifications, and Selection
vertex_cells and per-cell edges provide local incidence. Pending M6 final deferral
counters, assess standard MIS active-frontier retirement: for splits retire all
edges of local cells containing a claimed vertex; for collapses retire directions
whose removed-vertex star contains that vertex (each neighboring removed R, all
retained D). Do not retire the opposite direction merely because it is adjacent.
Keep Directory final claims and expanded-footprint checks. The source70 change implements this approach and awaits validation; the M6 evidence does not establish it as the sole cost cause.

Build71 PASS99.169s, pins unchanged and all build processes gone. Bidirectional serial passes 3 cases/558 assertions in 5.804s; MPI6 passes 3 cases per rank in 5.785s (466/118/118/70/70/70 assertions). Both plain and composed tight-window variants cover input immutability and final gates. Matched plain fixture: 60 accepted cells, 19 splits, 2 collapses; admitted work fell from 1556 (build67) to 21 and directory elections from 38603 to 610. Deferral counter semantics changed, so raw counts are not comparable. Mapped runtime71 `staged57/cases/mapped_runtime_20261028T014634_194809` FAIL10.944s: flat completes at work 31; curved reaches work 150, 14 accepted and 136 rejected before the same arithmetic cap, without quality. Opt-in rejection-reason instrumentation is added for build72; validation and M6 remain pending.
ServiceClaimedVertex retires measured local frontier directions using existing
ChangedVertices notifications, without new cache, state or communication; Directory
still guards unmeasured, remote and expanded support. Deferred-frontier progress
prevents stagnation fallthrough; both legacy generic/stagnation quality handoffs
are disabled for bidirectional mode, leaving pair quiescence as the construction-to-
quality handoff. A tight-window (1) plain/composed fixture requires both pass counts
before quality. Review found no blocker. `length_pass_deferred` now counts selected
deferrals plus bulk-retired potential directions, including balanced edges; raw
counts cannot be compared as equivalent work to earlier runs.

## Evidence and source identity

Case: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build61_working_surface`.
The solver exited zero but the native event was rejected on its existing elapsed deadline;
`native_assessment.json` records native_event_accepted=false. No accepted adapted mesh or
transfer/restart success follows from these private counters.

Working input is 303,359 volume cells (run.log114 and native initial=303359 on275–280).
146,862 is source-envelope metadata, not the input working-cell count. Final private map
contains 1,376,207 owned cells: 4.5366 times input, +1,072,848 cells. There were 205,967
accepted and 1,815 rejected local proposals: 99.1265% accepted, +5.20883 cells/acceptance.
No moves, reconnects, collapses or quality entries occurred. Of measured cells, 877,469
remain invalid for metric length; the Euler configuration has no requested BL height.
The 91,444 queued edges belong only to current finite frontiers; this is not a census of
all oversized edges on the evolving mesh. No remaining geometric-error demands were
reported at the exit.

Initial metric complexity was 499999.99999998609. The normalization/gradation completed;
there was one frozen normalization stage and no correction. Minimum/maximum source
density were0.22929 and6.92985e9; sensor gradation reached its fixed point with maximum
transported-metric ratio1 (run.log230–240). Complexity integrates volume*sqrt(det M),
not cells (SU2_CFD/src/adaptation/CNativeComplexity3D.cpp358–371). These counters do not
establish overrefinement or convergence. The density field is fixed on original donors;
interpolation is CNativeField3D.cpp48–54 and composed edge integration363–377.

The Engine file at review start matched build61 pins. The current candidate adds
working-surface preflight lines, so Engine line references below refer to BUILD61,
not the shifting working file.
Other referenced files matched pins when inspected.

Pinned source SHA256 values:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `743a72d4ab3966cd06a50e3ac1edac4034e00478142282ea008f018854eff5b3`
- `Common/src/adaptation/CNativeField3D.cpp`: `19548b1da72caadc7f87d110a63cce2eff6c3c4f020bc48561f123030b702d8a`
- `Common/src/adaptation/CNativeMetric3D.cpp`: `724fbbfb0bcaa553f34cc405e6dc36df469fb34753345d76074ab431585b3f96`
- `Common/src/adaptation/CNativeRemesher3D.cpp`: `619b3545f6d58de4e38e55d154597d9efcde47ff0d9ddb7ec4755b93a034253d`

## Contact cost receipts

Each row is one rank's accumulated engine timer/counter, from the final refine summary.
Do not sum rank wall times or add nested election time to exclusive contact time.
Contact candidate/check totals count visits/tests, not unique geometric intersections.

| rank | log line | engine s | contact s | rebuilds | candidates | exact/contact checks | Directory elections | nested election s | reconstruction s | private reconstruction CPU s |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 289 | 1729.846662318 | 755.68513790799841 | 824 | 131135339 | 9560683 | 1629519 | 584.78134196014014 | 160.24926009900213 | 25.638675000000429 |
| 1 | 298 | 1729.8525418239999 | 755.70282956899894 | 935 | 130585314 | 11069919 | 1629519 | 556.36633940017055 | 159.25846706400736 | 24.211493999999995 |
| 2 | 307 | 1729.866319836 | 755.72738908100087 | 899 | 145083594 | 9491667 | 1629519 | 541.5326968591479 | 160.26444575500344 | 35.598377000000085 |
| 3 | 316 | 1729.8499116779999 | 755.70723752899926 | 900 | 133410017 | 10819987 | 1629519 | 560.03495284017811 | 159.36663289700314 | 24.826629000000057 |
| 4 | 325 | 1729.85271233 | 755.70319700999835 | 888 | 135708694 | 8456759 | 1629519 | 551.12994803312324 | 159.11659204500177 | 81.536988000001273 |
| 5 | 334 | 1729.858084832 | 755.72349041699761 | 952 | 139194882 | 9888954 | 1629519 | 526.19411629912986 | 159.01756236800264 | 72.919862999999793 |

All-rank contact candidate visits total815,117,840; checks total59,287,969.
Contact took755.685–755.727s/rank, approximately43.69% of the1729.85s engine stage.
Directory elections were1,629,519/rank; nested election time526.194–584.781s/rank
is already included in the surrounding phase timers. Tracked engine transport calls
were4,405,071/rank. These totals support a contact/coordination bottleneck, not a claim
that rational arithmetic or source metric integration dominates wall time.

Directory::Clearance (CNativeDistributed3D.cpp325+) reuses an immutable collision tree
plus current additions, rebuilds after the existing addition/staleness threshold,
broadcasts candidate pages across workers, and tests each received candidate against
locally owned current cells. A surface edit on any rank triggers the collective path.
Rebuild count900/rank does not measure its time separately. Preserve all contact gates;
consider batching independent selected cavities and safe broad-phase/current-version
reuse only after demonstrating which work is repeated. Do not infer a new rebuild
threshold or remove contact checks from this receipt.

The existing perf_phase_13min report has only137 samples and DWARF decoding errors.
Its MPI/contact stacks corroborate the timers qualitatively; its percentages are not
used here as a robust decomposition of the whole1800s event.

## Smallest connected scheduling path

Add one production entry (for example AdaptMetric) into the existing Adapt lifetime.
Keep legacy Refine/Coarsen/Repair wrappers' externally visible behavior, particularly
strict standalone coarsening rollback. This is an API distinction, not a user config
switch or a scenario label. Keep EnginePhase Construction/Improvement and per-edit
MetricChange Refine/Coarsen/GeometryRefine/Repair roles separate from pass direction.

One production lifetime retains working, original donor import, FrozenMetricIdentity,
Directory, private parameters/geometry, ID allocation, deadline, work limit, atlas budget,
metric edge certificates and transfer authority. Do not alternate public Refine and
Coarsen calls: each currently completes strict quality and creates a fresh lifetime.

In Construction, alternate finite split and collapse generations at collective safe
boundaries after all proposal/batch scopes have ended. Each generation enumerates
only edges present at its entry, services each directed candidate finitely, measures
all published changed cells using the same source, and defers newly created edges to
later generations. A productive split generation must give collapse a turn before
reseeding another split generation, even with unresolved long edges. Full current
census is required before judging a pair quiescent or entering quality.

Reuse SplitEdge/InsertEdgeCavity and CollapseVertex, existing dependency import,
reservation, geometry/coverage/body/feature checks, contact clearance, parameter
Prepare/Commit, ownership routing and changed-star invalidation. Use the existing
edge-only MeasureMetricEdges and per-edit Refine/Coarsen validation during construction.
Do not use Repair intents or MeshOptimizer as construction surrogates: those measure
q/J. The proposal kind must be the actual edit kind on every rank, including empty
query ranks, and all ranks switch pass direction through the same global decision.

No strict q checkpoint is required before a construction collapse. The entire mesh is
already private inside the outer CFD transaction; inability to reach final gates rejects
the event and preserves accepted mesh/solution/reference. Retain standalone Coarsen's
old checkpoint machinery for its legacy API; do not accidentally enable it whenever
the new shared scheduler happens to be in a collapse pass.

After a quiescent/full-census bidirectional pass pair, enter the existing strict quality
census and dependency-tracked repair stage. Remaining blocked length/geometry demands
remain explicit failures/repair obligations, never acceptance. Preserve the existing
repair-to-construction return when repair changes size/geometry; such a return must
resume both length directions. Final success still requires complete strict q/J/size,
geometry/contact, requested first-height and source coverage. Exhausted time/work/RAM,
round limits and stagnation remain incomplete/refusal outcomes, with feasibility
undetermined. Neither a generation boundary nor small step progress is success.

## Hysteresis and boundedness

Candidate splits have L>1.8; candidate collapses have L<0.9. A balanced straight split
of a selected edge gives two lengths>0.9, preventing direct reversal. A collapse may
remove cells only if it introduces no oversized CHANGED edge (Metric3D coarsening
validation); recreating the split's old edge of length>1.8 is refused. Retained pre-existing
long edges are allowed locally, so collapse can coexist with unresolved demands elsewhere.
Use unchanged target/geometry/BL constraints and both directed endpoint-collapse choices.
Surface projection changes child paths, and neighboring edits can create other demands:
this hysteresis is not a theorem excluding all multi-edit cycles. Finite generation service,
retained refusal state and monotone shared work/deadline bound execution; report no-progress
or resource exhaustion honestly rather than asserting general convergence. Do not add an
aggregate-excess acceptance gate: selected split insertion currently permits aggregate
debt growth while shortening the longest changed edge (Metric3D449–459).

## Concrete invariant hazards and affected code

- Engine3D.cpp coarsening currently conflates event policy, operation direction, queue
  priority, collapse filtering, donor-query selection, checkpoint/rollback, parent row
  routing, result.splits/collapses, completion and counters. Classify each branch; a blind
  mutable toggle is incorrect. construction_kind must follow actual pass direction.
- Selection::ChangePhase/SeedFrontier/Queue/Best/Block/Touch and collapse filter state
  need a direction-aware reset preserving current measured cells and immutable edge
  certificates. Collapse direction needs up to12*owned frontier entries vs6 for splits;
  admit this before allocating. Forbidden aggregates seeded while splitting are zero;
  either rebuild them or conservatively use the existing selected-candidate filter and
  full operator proof. Never treat absent/stale cheap filter data as acceptance.
- Rejection records must distinguish operation semantics where one short geometry edge
  can be both a collapse candidate and geometry split demand. A failed collapse must not
  suppress a later geometry split merely because they share endpoint IDs. Preserve
  rejected-direction state across unchanged sweeps; Touch owns reactivation on real edits.
- Old coarsening dirty/complete and strict checkpoint branches (Engine build61~1875+)
  must not force quality before a validity-only construction collapse. Final completion
  cannot depend only on the active direction's queue. Blocked, constrained short edges
  are not an added final lower-length acceptance gate.
- Parent owner assignment (Engine routing near3560+ in build61) assumes two children per
  split old tetra and omission of collapsed rows; mixed histories must branch by actual
  current edit kind. Reserve/import the full removed-vertex star for each collapse.
- Surface coverage, traces, point refcounts, row versions and Directory cache invalidation
  must travel with every accepted publication across directions. Source geometry/donors
  are frozen before working-association rebases. Never re-freeze sensors on newly split
  working cells or rebuild authority from the current triangulation.
- Preserve moved/new node identity rules, monotone reserved cell/node IDs across passes,
  and the existing final ReaderSlices rebind. Collapse retains the selected endpoint;
  movement continues to use current explicit movement/identity contracts. Histories are
  diagnostics, not transfer donors or accepted-point correspondence.
- Engine header: new production entry plus direction/pass diagnostic fields, without a
  new production configuration option. Source stats/reporting callers need consistent
  accounting when one invocation contains both edit directions.
- CNativeRemesher3D.cpp514–547: replace production Refine-then-Coarsen orchestration
  with one shared-lifetime entry; avoid doing a second independent coarsening campaign.
  Keep the existing shared budget across FIRST_ROW, normalization stages and this engine.
  Complete source migration, worker/subset synchronization, complexity reassessment,
  correction bound, return packing, beforeAcceptance deadline and onAccepted reference
  swap remain unchanged (Remesher584–708).
- RefineOptimize serial diagnostic wrapper and existing direct Refine/Coarsen/Repair
  unit callers should retain their contracts unless deliberately migrated. Update real
  production call sites only; do not turn validation scenario labels into dispatch.

## Diagnostics and acceptance evidence

At generation entry/exit record actual global cells/nodes, accepted split/collapse counts,
full census coverage, geometry/size bad-cell counts, total size hinge (six incidences,
not unique edges), unique owned oversized/short-edge counts or an explicit ownership
convention, metric length min/max and bins, blocked directions, source identity, work/time
and memory receipts. Existing current-frontier queue length alone hides descendant debt.
These are diagnostics, not new gates. Cell count is not compared with complexity as a cap.

Connected checks should include a distorted valid input with both L>1.8 and L<0.9,
construction still unmeasured for q/J, both directions before quality; near-threshold
hysteresis and rejected direct undo; curved feature/body/BL constraints with coordinated
coverage publication; asymmetric MPI/empty ranks; failure/budget rollback preserving
accepted reference/solution; source identity/edge-certificate reuse across directions;
and final q/J/size/height plus transfer/restart on the difficult M6 target. Primitive
passes alone do not qualify the capability.

Established comparator: Omega_h satisfy_lengths alternates refine_by_size and
coarsen_by_size before quality (Omega_h_adapt.cpp160–175):
https://raw.githubusercontent.com/sandialabs/omega_h/main/src/Omega_h_adapt.cpp
Its quality-ranked criteria are not permission to add q/J to this construction stage.


## Connected source66 candidate (2026-10-28; unvalidated)

Production now calls AdaptMetric through the existing Adapt lifetime. Standalone
Refine/Coarsen/Repair retain their construction/checkpoint policies. Direction is
separate from strict standalone coarsening rollback; only the final strict census
can accept the outer transaction. The legacy `refine` diagnostic slot stores the
joint lifecycle, with explicit split/collapse pass and acceptance counters; there
is no second independent coarsen budget.

A full entry-edge sweep alone can delay the opposite operation for many thousands
of commits. The candidate therefore reuses Directory's distributed vertex-footprint
reservation as an independent construction pass. Successful Publish stages all
changed working-vertex keys on their existing rendezvous owners; allocation-free
Commit installs these transient claims. Failed/vetoed publication installs none.
Later intersecting cavities receive a distinct pass deferral in single, batch and
planned reservation. Service removes that candidate only from the current frontier,
not from later passes. Resetting the direction discards every claim; quality runs
without construction claims. Interior reservations use their existing cell namespace.

Split/collapse rejection caches remain separate. A globally productive pass dirties
both directional caches; the next entry conservatively drops stale refusals. Quality
movement similarly invalidates them before returning to construction. Queues are
rebuilt through existing ChangePhase and retain checked version/coordinate/field
certificates. No q/J measurement or ranking is added to construction.

New pass receipts report local current edges (shared MPI edges may be counted on
multiple ranks), short/long populations, min/max length, cell-incidence size debt,
geometry/size bad-cell counts, prior q/J measurement counts, accepted directions,
arithmetic work, admitted storage and elapsed time. These are diagnostics, not gates.

Publication accounting also retains the staged directory/parameter records through
later parameter proof and changed-owner notification. This includes staged pass
claims and their admitted nodes; memory/time/work limits are unchanged. Claims are
process-local scheduling data and never enter native reference archives or restart.

Focused protocol tests cover committed claims, pass deferral/reset, veto/low-memory
atomicity and the separate interior reservation namespace. The mixed-length fixture
requires actual splits and collapses before quality, zero initial construction
shape measurements, final gates and immutable input/source. These are unbuilt tests,
not qualification. Complete CFD/M6, repeated adaptation, transfer and restart remain
required, along with the existing geometry qualification and all unresolved failures.


Build65 was owner-cancelled after51.1455s with unchanged pins when read-only
review found pass vetoes misclassified by four secondary reservation callers.
All22 recorded processes exited before source edits. The callers now use the
typed pass flag, stop the local deferred growth and retain collective participation;
mixed AdaptMetric/Refine calls are elected as mode disagreement before divergence.
Build66 must qualify these fixes. An expanded-footprint Directory regression is
not a full engine-growth continuation regression; diagnostics count actual expanded
pass deferrals for difficult-target assessment.

Joint coarsening also uses the existing concurrent complete-vertex-star and final
reservation path. The mixed-length fixture has one long-edge component and two
short-edge fans; on at least three ranks it requires parallel commits before
quality, with the first collapse pass occurring while oversized cells remain.
Standalone coarsening semantics are retained. Curved/composed multi-slot batches
remain separate: expansion can invalidate an unprocessed slot's imported rows;
committed-pass claims alone do not protect future batch leases. No blind batch
flag change is part of this candidate.
