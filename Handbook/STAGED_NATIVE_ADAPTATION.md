# Valid construction, movable boundaries, then quality improvement

Current direction, reaffirmed by the user on2026-10-26. This implements an existing
ledger decision; it is not a proposal to weaken final mesh acceptance. Fixed physical
boundary-node coordinates are not required. The complete M6 Euler then RANS workflow
remains the acceptance target, followed by the other fields/geometries in the ledger.

## Missing end-to-end capability

Construct a geometrically valid provisional mesh under the target geometry and wall
constraints, improve its metric quality through movement and topology changes, and
publish only after complete strict validation and conservative transfer/restart.
The prior production path forced Absolute admission before Refine and Coarsen;
the active, unvalidated change now uses ValidityOnly with the strict final census.
A serial RefineOptimize wrapper exists, but falls back to ordinary Refine on MPI.
Its RegionFieldService executes the private constructor on rank0. These are not a
completed distributed staged workflow.

Reuse the distributed Adapt/Repair engine: it already tracks strict deficits,
acquires complete vertex/edge/face stars, supports interior movement under validity
admission, and preserves the accepted input. Its final ValidateMetricPatch remains
absolute in either admission mode. The missing work is not a new remesher or a
second partitioner: it is geometry/BL-aware quality operators, their staged lifecycle
and concurrent complete-star scheduling. Do not merely toggle ShapeAdmission.

## Evidence fixing the implementation direction

Diagnostic56 replays all26 M6 construction calls and11 shape refusals exactly.
Fixed-wall flips fail; a single interior fan and its optimizer also fail. Exhaustive
six-vertex enumeration finds one physical child triangle with no admissible existing
apex. The existing surface placement returns the initial failing split unchanged.

The same captured old cavity DOES admit a replacement with an adjusted authoritative
edge parameter. Offline7 tests15 positions and passes local transaction, final-patch
metric, curvature-progress, prescribed interface and full-current-mesh contact:
q=.2441217804,J=.0557041571,max edge1.598362316. Original source queries are retained.
No extra interior vertex is needed. This demonstrates a needed boundary-placement
freedom; it does not establish an accepted full M6 mesh or justify another fallback
ladder. Use this saved state to test the quality stage after a valid midpoint split.

## Connected implementation scope

1. **Geometry mobility.** See the [cross-chart diagnosis and certificate design](CURVED_BOUNDARY_MOBILITY.md). Reuse MoveVertex and forward boundary bindings. Distinguish
   actual physical corners/creases from original mesh-chart boundaries. New points
   can already move within their finite support; original mapped vertices currently
   have dimension0 bindings and are pinned. Curved facet-local authority also treats
   original edges as protected supports. Releasing smooth original nodes therefore
   needs cross-chart face coverage and geometry-error/source certification, not an
   unconditional coordinate move. Preserve body/marker identity, true sharp features,
   symmetry constraints and contact. Keep immutable authority and frozen sensor data.
2. **Quality improvement.** Optimize complete admitted stars/regions in the actual
   composed metric. Include surface and interior movement plus existing swaps,
   insertion and removal. Couple boundary movement with required first-row apices
   and track first-height deficits alongside metric quality; final-only height gates
   must not silently become a universal intermediate veto. Preserve any actual
   geometric/structural constraints and restore requested heights before acceptance.
   Use deficit improvement for private steps, validity/topology/geometry for every
   published provisional edit, and strict q/J/edge/height only for final acceptance.
3. **Staged lifecycle.** Connect construction and quality improvement under one
   resource/work deadline, with current reference parameters propagated across
   stages. Strict census remains authoritative. A provisional mesh, exhausted budget
   or failed optimization never replaces the CFD mesh or authorizes transfer.
   Preserve all coarsening, complexity and requested wall-height obligations.
4. **MPI coordination.** Reuse versioned Directory dependencies, donor imports,
   reservations and atomic publication. Batch independent quality regions and do
   multiple private improvement steps per admitted region. Current repair proposals
   are excluded from concurrent refinement selection; extend feature-keyed duplicate
   election/conflict ordering for vertex/edge/face repairs. Partition interfaces
   must be revisited through complete cross-owner stars; never freeze them forever.
   The rank0 regional query service is not a parallel optimizer.

## Current implementation, 2026-10-26

Active source is unvalidated after the first integrated diagnostic. Build2 passed
(session3212,447.5s, pins unchanged). Its saved-M6 quality test failed:
24 trials,0 moves,0.033s, no caps. The unmodified-binary debugger trace identifies
the extra monotonic surface-error veto: the midpoint seed has weighted squared
surface-error excess1.0514311742511105e-14; all six quality-improving proposals
increase it (minimum1.4848961336968936e-14). This is distinct from leaving the
prescribed geometry. Evidence: `staged57/offline1`, `debug1`–`debug3`; debugger2
stopped on an optimized-out variable, debugger3 completed. Debugger exit0 is not
a passing numerical test.

Build3/session10140 passed (1426.6s), with unchanged pins. Subsequent implementation
is unvalidated: explicit construction/improvement phases replace a policy-only
change. Construction imports edge samples without centroid shape queries, records
q/J as unavailable, and omits shape-ranking/fallback optimization. Exact positivity,
topology, authority membership and contact remain hard constraints. FIRST_ROW must
hand its provisional result to the same distributed lifecycle. Fully seeded stalled
construction may enter improvement with size/geometry debt; an admitted improvement
can return to construction. Exhausted improvement without publication rejects rather
than bouncing phases. One shared resource/work budget and strict final census remain.
The current M6 optimizer test uses the preserved valid midpoint seed separately from
historical byte-identical replay of the superseded26 constructor calls.

The connected revision now:

- Carries physical bindings and private moved-point parameters through the shared
  repair optimizer; artificial interfaces require complete stars before movement.
- Elects duplicate vertex/edge/face features and reserves complete final footprints
  for concurrent quality repair, using the existing directory and contact protocol.
- Runs production with validity-only shape admission and the unchanged strict
  final census. Quality repairs may leave surface-error deficits for refinement;
  exact geometry membership, orientation and publication contact checks remain.
- Distinguishes fully measured first-height deficits from invalid/partial geometry
  measurements. Only measured altitude/footprint deficits become provisional;
  positive curved-reference clearance remains mandatory.
- Scores requested height and normal footprint alongside metric quality, caches
  immutable current-cell height scores, and proposes requested-altitude projection
  and normal-row positions. A separately measured height improvement may authorize
  a valid repair even if metric quality alone is not increasing. Exact changed-edge
  guards and every final gate remain active.

The M6 quality-stage test reports remaining geometric excess explicitly; passing
it would demonstrate a provisional repair, not an accepted full mesh. The shared
MPI interior-star test now also starts from wrong first heights and requires
strict final height, metric quality, unchanged original inputs and mapped state,
in uniform/sheared fields with faceted/mapped walls. Independent-star tests require
simultaneous commits. Neither test has run on this latest revision yet.

Still open: curved cross-chart movement of smooth original vertices; coupled
surface/apex freedoms beyond individual complete-star movement; complete difficult
M6 acceptance and the wider ledger. Split cleanup deliberately retains coordinates
until its relocation accounting is connected. Final-only size handling still has
monotone changed-edge guards; do not claim all intermediate restrictions are gone.

## Acceptance and validation cadence

First validate the connected quality stage on the saved valid-but-poor M6 cavity,
including actual boundary movement and all surrounding contacts. Use a contrasting
captured difficult cavity and a small set covering smooth/feature geometry, rotating
anisotropy and requested first height. Then qualify the staged whole-mesh workflow,
including the difficult M6 target and bounded MPI2/6. Reuse existing tests and pinned
receipts where unchanged. Do not run another1800s M6 timeout until the connected
construction/quality path is implemented and its necessary freedoms are verified.

Full acceptance still requires q>=.20,J>=.05,all integrated metric edges<=1.8,
first height within10%, geometry/feature/contact conformity,500k metric complexity,
conservative transfer and restart. A local accepted witness is diagnostic evidence,
not DC-17/DC-18 closure. Fewer MPI synchronization points are an architectural
opportunity; no speedup is established yet.

Build4/session95149 stopped on a long-double/double `std::min` type mismatch in
regional geometric junction placement (72.1s, pins unchanged). Corrected the explicit
comparison type. Build5/session20546 then compiled the shared adaptation sources
but stopped on FIRST_ROW helper API errors (288.4s, pins unchanged). Restored the
legacy diagnostic admission implementation and corrected the stale helper call;
build6/session5459 is running with933 pinned inputs. No numerical validation yet.
Build receipts stay under `staged57`.

Read-only review during build6 confirms the final census rejects unknown/nonfinite
shape and finds no reachable production construction q/J or .001 floor. Remaining
height-policy gaps: BLGrowth12 D2 preserves a prior passing requested-height child
using strict HeightT; some shared wall constructors internally reject strict
CheckFirstHeight deficits before the engine can allow provisional height debt.
Assess operator-specific row placement separately from these universal preservation
vetoes. These findings are not closed by a successful compile or local Euler replay.

Build6 passed (738.8s,933 pins unchanged). Offline2/session19070 terminal failure:
optimizer moved the midpoint once in12 trials (0.00775s branch), then the diagnostic
expected skin used uncanonical cyclic vertex rotations. `interface_analysis.json`
independently decodes the old/candidate records: same8 face keys and same oriented
faces after cyclic normalization;3 raw rotations differ. Production already performs
that normalization. Test correction does not alter interface, metric or contact gates.
Those latter gates were not reached; no accepted local repair claim yet.
Focused serial/session67260:20/21 cases pass (2806/2810 assertions). Only the height
variants of the interior-star fixture fail its movement-stat assertion: the first-row
constructor already satisfies the final height and mesh gates without optimizer moves.
All source pins remained unchanged. Next connected revision fixes these test contracts,
tracks quality/height debt separately to amortize full censuses across productive
repair passes, and removes universal intermediate height vetoes in shared constructors.

The connected revision built successfully in build8/session57428 (396.46s, all933
pins unchanged). General construction/repair admissions share a private height helper that
permits fully measured height/footprint debt only under ValidityOnly; all missing
child, partial-measurement and reference-clearance failures remain hard. Strict final
CheckFirstHeight is unchanged. BLGrowth12 D2 uses the same existing provisional
height contract. Separate cached quality/height debt keeps incremental repairs in
one useful pass; return to construction requires actual publication and either no
quality debt or exhausted repair work. Existing event bounds/final gates remain.
New focused tests cover height-debt admission with strict final rejection, direct
optimizer height recovery, and multiple repair rounds without repeatedly querying
an untouched component's centroids. The multi-star test uses the existing per-star
work allowance multiplied by its copies; production limits are unchanged.

Build7 is terminal (198.3s, all933 pins unchanged): the saved-cavity fixture used
a private `Key` helper outside its namespace. The fixture now sorts its local
face key directly. Build8/session57428 rebuilds that fixture; production code
and acceptance gates are unchanged by this compile correction.

## Current qualification and next connected correction

All build8 validation handles are terminal. `staged57/offline3` (session76089)
passes in18.03s with unchanged pins and no caps. Its production boundary optimizer
makes1 move in12 trials; q=.2446272866,J=.0532005356,edge=1.5983623161.
Prescribed interface, transaction/final patch metric and external contact pass.
Surface excess remains1.7747488435e-13, so `final_geometry=0` and
`full_mesh_accepted=false`; it is only an accepted LOCAL provisional repair.
Largest sampled individual process RSS921,788,416 bytes, not whole-job peak.

Focused serial `cases/serial_20261026T181653_3818417` (session77808) is terminal:
23/24 cases and3345/3361 assertions pass;6.38s, pins unchanged. The16 failures are
q/J checks on eight partly repaired stars when that new fixture reaches its112-round
cap. Direct optimizer height recovery, intermediate height admission and all prior
selected cases pass. Its allowance will reuse128 rounds per star plus blocked-region
work; production limits and strict assertions remain unchanged.

Read-only stage review also found a completion obligation: Coarsen suppresses its
short-edge queue during quality improvement, so a quality edit can create a short
edge that never gets revisited. Return to construction after any productive Coarsen
quality pass, even without upper-size/surface debt; the publication flag prevents
zero-work bouncing. A roofed-box regression will require actual quality movement,
resumed collapse and final strict acceptance. Build9 will also link SU2_CFD for the
unchanged six-rank full M6 campaign; no accepted full M6 is claimed.

Historical build9/session76705 linked both UnitTests/test_driver and SU2_CFD; its terminal PASS is recorded below.
All933 compiled/source/build inputs are frozen. The Coarsen fix includes the
round-cap completion path. Legacy diagnostic (inactive production path) full-shape
minima now initialize after measurement; unknown records remain NaN. New run_m6.py
requires a successful build receipt that explicitly includes the CFD target, verifies
input/runtime provenance and cannot treat solver exit0 alone as native acceptance.
Its acceptance parser self-check covers saved rejection, success, missing ranks,
explicit rejection and solver failure. No full M6 job is running yet.

Read-only follow-up while build9 is frozen: `CollapseConstraintViolation` still
uses strict CheckFirstHeight to filter coarsening directions. That can suppress an
otherwise positive/clearance-certified proposal for measured h0/footprint debt before
shared provisional admission gets to see it. Align this filter with the same stage
policy, preserving exact containment, surface error, positive orientation, missing-
child and reference-clearance checks. Its direct strict-default tests must remain;
add explicit ValidityOnly coverage. This does not affect the wall-height-free M6
Euler case, but remains an open consistency defect for the full BL workflow.

Build9 is terminal PASS (552.879s, all933 pins unchanged), both executables linked.
Focused serial `cases/serial_20261026T183126_3857793` (session10231) passes all25
cases/3436 assertions in8.53s. Roof case: one move then one collapse, six rounds,
174 work, two quality censuses/one construction return, strict final mesh accepted.
Multi-star case: all eight stars meet strict q/J,64 accepted repairs, two quality
censuses/one construction return; the independent blocked wheel ultimately rejects
at679 rounds/4695 work. This test certifies useful-pass behavior, not full-mesh
capability. The remaining height-filter consistency edit precedes build10.

Historical build10/session97582 subsequently passed with frozen pins. The height prefilter now
admits measured provisional deficits only under ValidityOnly; Absolute behavior,
containment/surface-error, missing-child, reference clearance and invalid metadata
refusals stay intact. Existing direct test gains both-policy cases and the selected
ShapeAdmission tag. The campaign reuses validation54 connected serial/MPI6 filters
plus new stage/quality/height/coarsening regressions; all-worker workflows remain
separate. Preflight receipt records34+GiB available,40% CPU idle and36% I/O wait;
load average alone includes contention, so one nice19 build worker remains in use.

Build10 terminal PASS:171.243s,933 pins unchanged. Connected serial_full/session42727
is running with all compiled inputs/helpers frozen; each case group saves its own receipt.

## Broad qualification finding: geometric construction versus size debt

`serial_full_20261026T183710_3873135` / session42727 is terminal (85.842s,
pins unchanged):133/138 cases,1,083,966/1,083,996 assertions pass. New coarsening
height-policy checks pass. Failures include four legacy path/accounting cases and
one mapped-runtime case with both curved Euler/RANS variants failing COMPLETE.
Affine variants pass. The cubic variants stop at4096 work after2348 rounds/613
accepted edits (~21s, below120s time limit),608 geometry-progress events and1747
reactivations. Rejected mesh:1268 cells/628 nodes,572 surface-error failures,
1210 q failures/1215 J failures. Quality improvement has not started.

The rejection is `GeometryRefine` adding metric excess: engine1701–1707 independently
requires strict frozen GeometryExcess reduction, then Metric.cpp448 rejects any
added metric excess even in ValidityOnly. The strongest curvature-refinement paths
are unavailable, and alternate subdivisions consume work. Align this intent with
the staged contract: admit temporary size debt only in ValidityOnly, preserve net
geometric freedom/exact-validity/progress and final q/J/edge/geometry constraints.
Do not raise the4096 runtime budget or alter COMPLETE/BDF2 transfer/reload checks.
This is a connected construction-policy fix, not another local fallback.

Other failures: full-shape fixture metadata/error wording and edge-request counts;
FIRST composed-edge failure omitted from Engine field_failures; automatic-repair
fixtures assume retired strict-constructor fallbacks/no splits; centroid-import
fallback counts are obsolete under edge-only construction. Preserve accepted final
mesh/input/geometry assertions when updating path-specific checks. Agent ownership
is split across Metric.cpp/tests and Engine.cpp/Distributed tests; no live job.

Build11/session30951 is running with933 pinned inputs; all sources/helpers frozen.
Implemented GeometryRefine validity-only size debt allowance, with both production
callers audited to require strict GeometryExcess reduction. New positive-cone test
requires zero centroid queries in construction and strict final oversized-edge
rejection; Absolute retains old restriction. Absolute shape failure again stops
before edge tracing (useful diagnostic short circuit; construction remains edge-only).
Engine counts edge failures as well as point failures. Revised fixtures preserve
strict output/geometry/immutability and assert staged behavior rather than disabled
constructor fallback counters. Next `connected_fix` includes all five failed areas,
especially unchanged curved runtime conservative BDF2 transfer/reload.
Header Metric3D.hpp:101 has an inherited overgeneral strict-policy comment; update
it with the next substantive header edit (actual stage contracts documented here
and in the changed implementation). No runtime case budget was raised.

Build11 is terminal compile failure (111.17s,933 pins unchanged): Catch2 requires
parentheses around the newly added disjunction in a support-stat assertion. Root
fixed that assertion only. Build12/session42383 is running, inputs frozen; no
production or acceptance change. Checkpointd0b5f7b70c is pushed to the approved fork.

## Build12: connected result and full-M6 rejection

Build12 PASS141.236s,933 pins unchanged. Affected
`connected_fix_20261026T185235_3916319` / session66158 terminal:68/69 cases,
12928/12930 assertions pass,16.496s,pins unchanged. The curved runtime Euler/RANS
variants remain incomplete under4096 work (~4.08s each), but geometry no longer
starves quality:194 rounds/143 accepted,36 geometry-progress,52 quality-progress,
34 moves/24 reconnections/35 insertions,44 optimizer trials/16 validated. Remaining
6 geometry-demand cells/17 geometry edges and153 quality/height demands; no oversized
metric edges. This is a narrower diagnosis, not accepted mesh capability.

Full M6 Euler `m6_runs/euler_build12`, session43478, is terminal REJECT (solver
exit0,365.99s wall/306.7s native, pins unchanged, original CFD state retained).
Under the unchanged validation54 configuration (6 ranks,500k,native1800s,
outer2400s), construction failed before quality when changed-star notification
exceeded scan budget4096. It made19,658 private edits across372,631 cells and left
165,439 cached construction demands. Evidence is preserved in that run's
`diagnosis.json`, `native_assessment.json`, pins and logs. That M6 job is terminal; current build13 is described below.

Diagnosis for next design review, not a fix: seed max-star88 exceeds current
quality capacity64; changed-star notification work is uneven across ranks and scans
exceed the cap. The connected curved runtime cases also remain incomplete after
4096 work with6 geometry-demand and153 quality-demand cells (no oversized edges).
Reconsider complete quality coordination and scheduling; do not raise limits or add
local fallback variants. No acceptance milestone is closed. Preserve the Build12
and full-M6 receipts unchanged.

### Connected frontier and complete-star correction (in implementation)

The M6 degree growth is shared by ordinary edge bisection and cone insertion:
both replace an interior edge star of m tetrahedra with 2m and increase ring-vertex
valence. The selected edge is removed; the problem is recursive new-spoke debt,
not a no-op or a cone-specific defect. The new SplitInsertion rule permits total
edge excess to increase only while the longest changed edge strictly decreases.
This uses existing edge measurements, never construction q/J samples.

Repair demands retain a service epoch across atomic removal/addition, including
transient zero users. Global selection and reservation use waiting age; serviced
features return to the tail, deferred conflicts retain age, and healthy-support
changes reactivate blocked demands. Construction is being organized into frontier
generations so descendants cannot indefinitely postpone improvement.

Complete coordinate stars have an explicit transaction path beyond the64-cell
arbitrary-topology bound. Metadata and payload admission retain the existing4096
scan and configured byte bounds; movement proves unchanged connectivity, retains
cell identities and advances versions, checks every cell and physical face, and
uses paged global contact checks with one atomic publication. General topology
cavities remain64/128. Larger-star direct movement is being integrated; it does
not yet provide arbitrary large-star retriangulation or cross-chart mobility.
New local tests cover88/176-cell movement and distributed paged publication with
resource-veto rollback. Build13/session1413 is active with933 frozen source pins; these changes are not yet qualified; no acceptance
checkbox is closed.

Build13 includes both the test driver and CFD executable. Runtime qualification will
first exercise the connected curved case and new fairness/sweep/coordinate paths,
then the unchanged full M6 six-rank target. No second heavy job runs concurrently.
