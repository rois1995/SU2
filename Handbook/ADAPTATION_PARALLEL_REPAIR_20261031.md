# Parallel quality closure and reusable CFD input — 2026-10-31

Latest update: user explicitly directs stopping CFD when CL/CD are reasonable
and proceeding with mesh adaptation. Residual thresholds below are historical
initial plans, superseded as seed acceptance gates. A validated read-only real
Euler field is now saved (CL.269535/CD.0135359); full unchanged-code TWO-event
BEFORE baseline euler_build179_real_cfd50k_before is LIVE. All strict mesh gates
remain unchanged. See ADAPTATION_M6_CFD_SEED_20261031.md and current HANDOFF.

Status: design/source review; saved real-CFD input is ready, BEFORE baseline LIVE. Latest user authorized
stopping179because it uses ITER1 startup CFD. Verified owned launcher received
SIGTERM; all ten handles absent and all1396pins unchanged. Before implementing
repair/ownership, first qualify/save original-grid CFD and run the current179
binary from that seed as the BEFORE baseline. Then one connected implementation,
serial/oneMPI6 milestone and full two-event AFTER retry from the identical seed.
Primary owns integration and acceptance.

## Missing capability and evidence

Build179 improved repair selection, but each of eight slots still synchronizes
ranks. At complete census5,37522q-bad/37426J-bad/2945geometry-bad remain;
q/Jdebt8141. About10s/round, commits3.38/s vs1752.78/s. Improvement is mostly
q-bad removed/commit1.46vs.49, not batching. Ranks0/2/4 hold79%q-bad;
low-debt rank3 spends disproportionately on geometry. Nested timing/sample shares
overlap and must not be added. Last complete census7 has34386q-bad/33995J-bad/
2854geometry-bad; final partial zeros do not establish closure. First event
naturally time-budget rejected. Current next target is speed based on observed
slot synchronization and rank demand spread; requested richer counters must
confirm whether repair power or construction becomes the next limiting factor.

## One connected implementation

1. Reuse existing batch directory import and full-vertex-disjoint reservation.
   Run admitted private candidates locally without per-slot collectives; combine
   approved replacements/parameter edits into one complete round publication.
   Retain dependency versions, fair oldest slot, worst-first order and unique IDs.
   Preserve collective alignment on empty ranks. Work/time/storage exhaustion
   fails closed with no partial publication; aggregate retained results and
   imported contexts must be admitted, not only one candidate's scratch.
2. Base candidate geometry on the agreed round state. Prove private contexts are
   independent under reserved complete vertex stars, including changed geometry
   parameters, associations, coverage and simultaneous clearance; otherwise
   rebuild/revalidate against combined current state before publication. Never
   treat geometric footprint disjointness alone as this proof. Verify local
   metric context contains the same complete query support as the streamed path.
3. Add the user-requested separate quality q/J debt constraint to existing
   ownership weights, EXACTLY zero for clean cells, separate from geometry cost
   and base cell/work weight. Build179 entry debt989–5322/rank was masked inside
   1+debt. Extend existing Repartition optional aligned weights, rank load receipt,
   dimension checks/serialization/METIS constraint array and migration conservation;
   omit this dimension collectively when global debt is zero. Direct repair
   effort toward required q/J debt. Prefer existing ownership
   migration at complete, quiescent census points with measured transition cost;
   preserve cached measurements and census flags, versions, attributes, epochs,
   field identity and cumulative budgets. Do not add another scheduling framework
   or config. Preserve oldest-demand fairness and safe blocked-star state.
4. Correct the confirmed invalidation gap: changing a healthy support cell of a
   rejected feature must reopen that feature even if its bad-user cell is unchanged.
   Reuse complete published changed dependencies; do not reset unchanged rejected
   stars on every round. Regression must exercise both change and no-change cases.
5. Preserve construction paths, existing geometry reuse and all strict gates:
   geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights, topology, positive
   nondegenerate elements, genuine features and separate bodies. CAD optional.

Read-only ownership survey confirms the reusable compound-publication precedent
in first-layer initialization (Engine~1870–2060), aggregate Directory::Publish
(Distributed~1331) and PrepareParameters (~1197). Current repair loop still has
per-slot chosen/stream flags, work reservations, elections, metric import votes,
clearance and publication (Engine~2959–4515). CavityMetricSource::ImportCells also
votes despite local donor work. The private evaluator must remove all these inner
collectives and report local status once; existing non-repair callers stay intact.
Clearance currently takes one patch/rank: aggregate new-vs-old and new-vs-new
collision checks remain necessary. Full vertex disjointness does not prove
spatial nonintersection. Private ParameterView overlays share an accepted base
revision; combined node/face/trace edits must have noncolliding keys and pass
PrepareParameters once against current complete state.

Exact interfaces and the minimum debt redistribution method remain under source
review; this is not a claim that private publication is already proven safe.

## How geometry closes

Existing engine closes q/J/height first, enables residual size repair, then
EnableGeometryRepairs only after shape/height/size close. Residual geometry gets
its own directed repair pass; exhausted productive repair can return through the
existing construction handoff and then quality remeasurement. Preserve this path
and its strict geometry authority. Geometry changes that introduce shape/size
violations must close those again. All passes share the same cumulative event
budgets; no budget reset, larger native time or unconditional extra trial campaign.
Build179 never established shape closure, so this geometry closure capability
remains unproven. Its slower incidental geometry reduction than175 is not evidence
that the final geometry-directed pass is sufficient. Measure full geometry-bad
counts/types/excess, not q/Jdebt alone; classify any actual remaining failure.

## Passive diagnostics at each complete census

Reuse counters and cached measurements; no extra perf/GDB and no decision changes.
Per rank/window: nonoverlapping private busy seconds, collective-call wall seconds
(protocol plus possible waiting, explicitly labeled), commits, debt and q-bad
removed. Report q/J fixed/new churn with topology replacement and ownership
migration distinguished; avoid counting an ID-only replacement as a repaired cell.
Report stuck bad cells/all features blocked, queue length/blocked-feature counts.
Ladder coordinate/optimizer/collapse-reconnect trials/successes/seconds, removed
debt/bad cells and existing rejection reasons. Classify q/Jbad by surface/interior,
genuine feature proximity and gate-miss bins. Reuse edge measurements for shares
with any edge<.9/<.5. Geometrybad by actual surface type and excess bins. Record
metric drift at bounded fixed spatial samples selected at quality entry; evaluate
the CURRENT transported target, not only original frozen donors, with missing
samples explicit. Keep diagnostics outside decision/work-budget charging and
bound/account their memory; avoid costly additional geometry certification.

## Source-review decisions pending the saved-field baseline

The current Clearance algorithm already pages proposals and validates new-vs-old
and new-vs-new contacts, but groups all pages by owner and permits one proposal
per rank. Extend its existing page/proposal identity to owner+candidate ticket,
retain each candidate's existing topology/coordinate limits and priority, and
return per-ticket veto/defer results in one round exchange. Do not concatenate
multiple candidates under the single-star admission or remove collision checks.
Existing single-candidate callers retain their behavior; construction stays intact.
Combined Publish/PrepareParameters can then stage accepted replacements once.

The entity-geometry local error path is now traced: GeometryGuidanceExcess uses
candidate faces; ParameterError uses their three coordinates/bindings plus the
immutable authority, entity identity and tolerance. Cross-patch candidate discovery
queries immutable NearbySurfaceFacets, not a mutable peer's current physical faces.
Thus two full-vertex-disjoint private candidates do not stale each other's local
entity error query. Legacy coverage still needs disjoint face/trace-key checks and
combined PrepareParameters validation. Global GeometryCertificates is different:
it uses the complete current physical union and is revision-scoped; never carry
its old revision through combined publication as a final certificate. Large
coordinate-only stars preserve old cell IDs and increment versions; ordinary
replacement allocation and their routing must remain distinct in batched commit.

CavityMetricSource::ImportCells is locally computable when the complete current
cavity is supplied, but still performs a world election. Its current nodal donor
path, TransportNodalMetrics and FrozenField construction are the reuse point for
private evaluation, with one error election after local work. Do not substitute
the bootstrap stage_snapshot for the current metric or silently truncate support.

For diagnostic gate-miss histograms, the existing CellDebt.q/j already store
squared normalized deficits. sqrt(q/j) provides the requested normalized miss
without extra shape evaluation or new raw-score storage. Short-edge flags use
cached six lengths. Stuck cells require a nonempty feature set with every feature
blocked; report missing feature coverage separately. Parameter binding dimension
is not physical feature dimension: use Geometry().MobilityDimension(binding,
Mobility()) so artificial chart seams are not labeled genuine features. State
explicitly whether near means the topological one-ring; no unrequested geometric
distance/certification campaign. Cache geometry excess/type during existing
SurfaceDeviation calls; RefreshQuality deliberately does not repeat geometry work.

Ladder counters distinguish private certified proposals from actual published
commits; inner optimizer accepted moves can still be rejected by the outer gate.
Use existing MetricDiagnosis old/new shape debt and add q/J bad counts to the
existing measurement loop if needed, never remeasure just for diagnostics.
Churn must distinguish retired/introduced cells from matched-cell fixes; coordinate
moves replace node identities, and reused cell IDs alone are not proof of a
persistent tetrahedron. Version differences are invalidation metadata, not by
themselves evidence of topology creation. Account migration in/out separately.

Fixed-point drift must sample the CURRENT nodal field. EvaluateCavitySample uses
clamped best-parent extrapolation outside its donor cells; do not report that as
a genuine sample of the current spatial field. Verify containment first and mark
lost/unsupported samples explicitly. A bounded candidate is to retain 16 fixed
quality-entry cell-centroid points, update containing-cell references from actual
published removed/added rows, and gather tensors only at complete censuses. This
avoids a rebuilt spatial index/full mesh scan for each sample/window and avoids
original-frozen-field false-zero drift. Track diagnostic seconds/memory separately;
apply the existing target composition when applicable. This tracking design still
needs implementation and focused migration/topology tests after the baseline.

## Saved original-grid CFD input and matched comparison

Latest user accepts reasonable stable CL/CD and explicitly directs stopping CFD
and proceeding with adaptation. The assistant's former residual thresholds are
reported diagnostics, not prerequisites. Force-qualified real Euler seed is saved
read-only at staged57/m6_inputs/euler_original_force_seed_v1; CL.26953535374,
CD.01353585654,stable200steps,all finite/exact ordered mesh coordinates/rho+IE>0.
No claim of a residual-converged aerodynamic reference. Full policy, original and
continued histories, source/runtime pins, user-owned-stop receipts and original
checkpoint audits: ADAPTATION_M6_CFD_SEED_20261031.md. No new freestream solve.

BEFORE baseline euler_build179_real_cfd50k_before is LIVE with unchanged179CFD,
50k/native5400/event/outer14400,TWO events,all strict gates. It reuses that exact
saved solution via copy and the same deterministic ITER1 continuation as AFTER.
Do not implement the connected C++ change until this baseline's terminal evidence
is preserved. Then serial/ONE MPI6 milestone and full TWO-event AFTER immediately.
All future results use this saved input; historical startup179is a different field.
Compare matching quality time, whole-field q/J/geometry bad counts, commits/s,
q-bad removed/s, wait share, rank debt spread, geometry work and actual mesh sizes.
Existing before counters define the common comparison; new passive counters must
not be pretended to exist in the unchanged before run. Cache quotas/host contention
must be reported when assessing timing differences. Completion remains unchanged.

## Validation and actual acceptance

Focused serial risks, then ONE MPI6 integration milestone: conflicts/empty ranks,
curved associations/parameters, requested heights, migration preserving census
flags and cached-versus-recomputed census, rejected healthy-support invalidation,
resource exhaustion. Reuse unaffected evidence with verified provenance. Then
run the FULL two-event M6 at once using the saved qualified CFD checkpoint.

Completion remains TWO strictly accepted meshes, actual conservative admissible
transfer, finite continued CFD and isolated adapted disk restart; then dedicated
BL/MPI qualification and corresponding RANS500k. Passing checks, lower memory,
balanced weights and debt percentages alone do not complete it. At each terminal
outcome select speed, repair power or construction as the next target using
whole-field q/J/geometry counts, churn, blocked set and ladder evidence.
