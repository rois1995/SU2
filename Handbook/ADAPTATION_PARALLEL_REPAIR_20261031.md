# Parallel quality closure and reusable CFD input — 2026-10-31

Status: design/source review, saved-CFD helper preparation. Latest user authorized
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

## Saved original-grid CFD solution — latest user requirement

After179terminal and before the next full two-event M6, solve the ORIGINAL M6
mesh at unchanged Euler physical conditions. Reuse the current CFD solver and
existing restart/config facilities, ADAP_LOOP=NO; one owned nice19 MPI6 heavy
job, single-threaded libraries and resource receipts. Select a bounded iteration
cap and qualify with residual reduction plus sustained CL/CD stability, finite
fields and positive density/internal energy. Initial bounded qualification plan:
5000 iterations/3600s outer allowance (not an adaptation time-budget change),
retain Mach.8395/AOA3.06, Euler/ROE/MUSCL/current limiter and original geometry.
Use existing CFL adaptation capped20 rather than fixedCFL1; preserve MGLEVEL0
initially. Independently require density log residual<=-6, all five Euler residuals
at least four decades below their initial finite levels, and final200-iteration
CL/CD ranges<=max(0.001*abs(mean),1e-5 for CL/1e-6 for CD). Record history at every
iteration including forces. These are practical initial-solution criteria, not
experimental agreement or grid convergence. Do not publish a seed merely because
the cap or a solver convergence flag was reached. If the solver stops before a
full stability window, continue from its checkpoint. Preserve failed attempts. Record the precise criteria and
actual history; reaching an iteration cap alone is not convergence. Preserve numbered checkpoint files and history so a timeout cannot destroy the
last complete state. Resume only a coordinate/admissibility-validated checkpoint
with known iteration/history provenance, rather than restarting from freestream.

Restart source review: CFVMFlowSolverBase loads SOLUTION_FILENAME; output uses
RESTART_FILENAME. Steady binary input is basename.dat, whereas adaptation output
has numbered cycle names. Set READ_BINARY_RESTART=YES and use separate seed-input
and output basenames. The reader does not certify restart coordinate/order equality;
check every coordinate row against the exact original grid explicitly. Existing
ITER1 history and initial_cfd_receipt(flow_converged=false) cannot qualify this seed.

Save original mesh/native reference, restart, configuration, history, solver/runtime
and hashes in a dedicated immutable input directory. Check restart point count,
ordering/coordinates and state admissibility. Future attempts COPY the checkpoint
into isolated run directories, use RESTART_SOL=YES with verified filename mapping,
and distinct output names. Prove first remeshing consumes this saved field (or a
recorded deterministic restart continuation), not freestream. Subsequent events
continue from conservatively transferred solutions. No source/metric fixture or
case-name reconstruction dispatch. Do not overwrite the saved restart.

The new solution changes the first target metric. Latest user explicitly requests
BEFORE/AFTER runs from this new saved CFD input, superseding the previous restriction
on a new old-code baseline campaign. Run current179 unchanged CFD first, with the
new seed and identical target/gates/native5400/time/work/memory settings. Preserve
terminal evidence before implementing the connected change. Then run the changed
implementation with the same copied/pinned seed and runtime/config (except source/
binary provenance). Existing counters define common comparison; newly added
passive counters must not be pretended to exist in the before run. Future results
use this saved-input baseline. Compare matching quality time, actual q/J/geometry
counts, commits/s,q-bad removed/s, wait share and rank debt spread, with actual
initial/quality-entry workloads explicit. Build179startup data remains historical;
never mix its field with the new baseline for a same-input speedup claim.

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
