# Uncertainty-controlled complexity experiment — current work

User specification 2026-10-16: bounded difficult-cell integration, global MPI
uncertainty, stage-frozen normalization, original sensor-only donor P1 plus
actual-query geometric BL/finer/fade, reassessment of THAT original field on
subdivisions or adapted cells, explicit bounded correction if material. All mesh,
geometry, prescribed height, positivity, quality, full field and transfer gates
remain. No testcase/iteration/Euler-to-BL dispatch. Tolerance must follow evidence.

Worktree NativeIntegrated; branch codex/native-3d-bl. Previous Goal3 paused for
user discussion; this experiment is now explicitly authorized. Full Goal3 remains
incomplete. No accepted M6 mesh, normalization or construction claim yet.

## First connected implementation (experimental; not normalization policy yet)

New scalar EstimateComplexity3D reuses native geometric support/chart/band cuts,
actual sample composition and positive quadrature. Bounded density-query work;
fixed 3/5 radial comparison, 7/12 angular comparison, complete parent-cover
retention if a child transaction exceeds the work budget. It returns a value,
HEURISTIC indicator, actual query count, panels, limit/support state. Indicator is
not a rigorous bound; unsampled possible wall support has infinite uncertainty.
If initial band coverage cannot finish, a complete positive Q4 estimate is kept
WITH infinite uncertainty, never a passing omission of BL mass. Strict integration
and mesh acceptance callers retain their existing behavior.

Cells certified outside every geometric support use positive Q18 versus Q4 of
the sensor-only P1. No sampled BL tensor is propagated. No per-cell successful
accuracy condition is needed to return a bounded estimate. Local refinement
preference does not establish global accuracy; MPI allocation and admissible
normalization tolerance are pending the exact-field evidence.

Evidence folder: integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1.
run.py prepares immutable exact hosting-rank tensor shards from the previously
captured NPZ and runs difficult cells88/157/329 at temporary longest-edge bisection
depths0/2/4. Subdivision changes integration cells only; the density callback still
uses the original parent P1 and actual geometry, never a fitted refined field.
These are not meshes delivered by the remesher. Reassessing an actual adapted
mesh will be compared only if construction produces one under unchanged gates.

## Reference evidence preserved

Independent positive tensor-Gauss Duffy results at128/apex0&3 for329/88 and72 for157
are now terminal complete. Values:329 .260534013743/.260534233810;
88 4.692764843616/4.692760245951;157 1.396852842040/1.396852145601.
Final chart gaps below1e-6 relative, largest last-order change1.423e-5 (329).
Numerical references, not rigorous enclosures or independent physical laws.
Do not rerun these expensive stages merely because the new integration controller
changes; the physical Evaluate/ApplySample law and archived original fields remain.

## Required remaining evidence

Compare indicators against those references; complete frozen-M6 full-BL mass and
normalization discrepancy; assess hidden support; global MPI1/2/4 reduction;
bounded explicit normalization/correction with immutable field during operations;
attempt accepted native mesh and independently distinguish construction failure.
No new geometry/meshing fallback is authorized by an integration-only success.

## First strategy outcomes and current replacement

Focused13 assertions passed: complete polynomial moments near/outside a wall,
exact query ceiling/unknown support on seed exhaustion, invalid densities/control.
Projected-scalar pilot depth0/2/4 (0/2/4 longest-edge bisections,1/4/16 subcells):

| Cell | Relative error depth0 | depth2 | depth4 | raw indicator / true error depth0 |
|---|---:|---:|---:|---:|
|88|.0495006|.00545936|.00442680|1.27894|
|157|.000917780|.000347515|.000219853|.522697|
|329|.00107371|.000202242|.000501405|2.87016|

Thus raw indicators underestimate157 by1.91x; subdivision is NOT monotonically
more accurate (329), and total work can increase substantially. No global tolerance
has been justified. No assumption that an actual adaptation will succeed or cure
quadrature follows from these data.

Initial full-field batch failed on cell4 because endpoint placement incorrectly
used the interior-query test. Corrected endpoint construction preserves every
actual density query check. Source and decision preserved in
rejected_endpoint_membership_v1. Corrected projected strategy then saved94 cells
and failed on94 (source-sample representability);2 earlier cells had unknown
support. Cell7 took45s at8192 density evaluations: counting density evaluations
alone misses expensive global-root admission/preparation. Complete source and
failed receipts remain in rejected_projected_scalar_v2 and field_v2. No whole-field
mass, normalization, MPI or construction success is claimed from these prefixes.

## Current result and revised priority

The conditioned single-fan estimator passes the 13 primitive assertions and 12
strict controls (259 assertions). Source-cell94 now uses scale-aware inward
placement in original coordinates; all actual points still pass exact original
source containment. This changes integration query placement only, not a mesh gate.
The tested source snapshot is conditioned_global_root_v1.

Conditioned difficult-cell errors at subdivision depths0/2/4:
88 .768196%/.0623747%/.0639516%;157 .091778%/.0347515%/.0219853%;
329 .107371%/.0202242%/.0501405%. Raw157 indicator still underestimates error1.91x.
Only159/303359 full-field cells completed in the bounded300s prefix: value562.18055,
raw indicator1.04895,472592 density calls,44 limited,zero unknown support.
Cell7 took21.4s and29 took40.0s despite the8192 density-query ceiling.
This is NOT a full-field complexity estimate or a calibrated uncertainty policy.

The four-rank construction-only replay completed: accepted=false, backend104.59s,
348.22s including driver setup. No CFD steps or solution transfer. Rejection:
composed edge integration budget exhausted on edge18209,61331 near
(.61945,1.02516,-.01533) to(.61987,1.02046,-.01581), during first-layer initialization.
This is a separate failure from volume normalization. No accepted adapted mesh
exists for reassessment; the hypothesis that actual refinement cures integration
remains untested. No limits/gates were raised to produce a result.

User steering: prioritize1 uncertainty-aware scale solving,2 distributed absolute
global-error allocation,3 geometry reuse across scale trials. Use cheap early
trials and tighten only near the final target; overlap with an uncertainty interval
is not an exact root. Avoid per-cell relative convergence requirements. Account
for preparation, memory and communication. A BL-correction decomposition is an
exact but deferred measured alternative, with union support and wall overlaps.
Further unbuilt subdivision/seed-screening proposals have been archived in
unbuilt_subdivision_proposal_v1 and withdrawn; completed negative evidence remains.
Production normalization is still unchanged. The experimental scalar API is not
used to certify an accepted mesh or a final normalization.

## Harder 2D validation scope

User supplied /media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes.
Start with 2D_L1_coarse_r1.cgns (7.4MiB); medium/fine levels also available.
This adds multiple walls, narrow gaps and possible overlapping BL support to the
accepted RAE/2D controls. No configuration or saved sensor field was found in that
folder yet; no run has started. 3D_L1_coarse_r1.cgns (470MiB) is available for later
qualification, not a replacement for the current M6 target. Shared uncertainty
and normalization policy should retain dimension-specific geometry preparation;
validate accepted2D behavior before replacing its existing preparation policy.


## Geometry reuse qualification

Geometry-only ComplexityWorkspace3D retains support candidates and original-barycentric
chord cuts for this immutable wall model/original cell. It rejects reuse with another
cell/model. Every density callback is reevaluated with the current original P1 sensor
and actual-position geometric wall sample; no composed tensor/density is cached.
An absolute-error allocation can replace the old local relative refinement preference.
A workspace payload/node accounting ceiling bounds retention, not allocator overhead,
transient preparation or RSS. Final global allocation/scale policy is not wired yet.

Distance-only cutoff/support/chart checks now reuse exactly the full evaluator's
nearest-facet calculation, returning before unused tangent tensors/eigendecompositions.
Full EvaluateImpl branch equals baseline Evaluate after stripping those early returns;
ApplySample/ApplyPoint/Build remain text-identical. See updated physical_law_identity.json.

18 focused3D/cache/edge/volume/curved cases pass435537 assertions, and8 existing2D
composition controls pass95. Exact original/changed-sensor-and-core warm/cold estimates
agree on88/157/329. Independent reference errors are unchanged, not improved by caching.
On88, cold2.20059s/preparation2.08060s vs repeated.11325s,58112bytes;157 .12095s vs
.02311s,19072bytes;329 .17431s vs .04663s,40664bytes. Changed157 needs109 new chords,
showing geometry is extended when the sensor changes the adaptive decisions. These are
local replay observations, not whole-normalization or MPI speedup/scaling results.
Three exact-field owner contributions reduce identically on1/4 ranks: C6.312537784630236,
raw indicator.051376315498162364,20060 queries. This is a3-cell reduction check, not
complete frozen-M6 or global allocation qualification. Receipts: geometry_reuse_v1 and
geometry_reduction_np1_v1/geometry_reduction_np4_v1. No owned job remains in this batch.

The airfoil file named2D is actually one layer of63957 hexahedra. Its z=0 planar cap
has been extracted and triangulated into127914 positive triangles/64540vertices,
with all1170 source boundary edges and markers preserved. Interior edges are paired,
boundaries degree2, Euler characteristic−2 (three holes), area relative difference
2.97e-18. No source snapping; opposite-cap projection differs up to3.52e-7 and is
not used to define the selected cap. Reusable preparation, original CGNS inventory,
source/output hashes and validation: ../three_element_input_v1/preparation.json.
The prepared SU2 input is30P30N_coarse_tri.su2 there. No CFD configuration, sensor
field, adapted mesh or accepted airfoil workflow yet. The3D coarse file contains
8634195 hexes and requires separate input/construction qualification; the native
backend currently rejects non-tetrahedral cells.
