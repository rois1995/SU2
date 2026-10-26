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

Build9/session76705 is active, linking both UnitTests/test_driver and SU2_CFD.
All933 compiled/source/build inputs are frozen. The Coarsen fix includes the
round-cap completion path. Legacy diagnostic (inactive production path) full-shape
minima now initialize after measurement; unknown records remain NaN. New run_m6.py
requires a successful build receipt that explicitly includes the CFD target, verifies
input/runtime provenance and cannot treat solver exit0 alone as native acceptance.
Its acceptance parser self-check covers saved rejection, success, missing ranks,
explicit rejection and solver failure. No full M6 job is running yet.
