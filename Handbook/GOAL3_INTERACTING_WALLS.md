# Goal 3 — joined fronts for interacting opposite walls

Full Goal3 remains ACTIVE. The published curved-cylinder milestone remains in
[coarsened metric fronts](GOAL3_COARSENED_METRIC_FRONTS.md). This step addresses
the inability of the admitted constructor to join two wall layers without a
BL-free core; it does not replace the remaining general/interacting geometry,
MPI/current-BDF/CFD/viscous accuracy or affordability requirements.

Evidence: `integration_evidence/native_3d_bl_v1/interacting_walls_v1`.
Target: six-tet unit box, opposing planar walls h.02/growth1.2/T.6 each, immutable
original-P1 sensor4*(1+3*x) I. The thickness bands overlap across the central.2.
q>=.20, J>=.05, complete actual composed edges<=1.8, exact finite planar reference,
normal foot and prescribed height, work4096/time300s remain unchanged.

## Evidence before changing construction

The reused CoupledBL test rejects17.83s total at shared4096 refinement work after
first-height initialization leaves30 private cells. No subsequent metric repair
publishes. The last local rejected candidate has q.182599/J.038854 at cell3469,
centroid(.25,.75,.5). Full rejected SU2, coordinates and exact source/build/binary
receipt are retained in native_opposing_v1. Rejection is not physical infeasibility.

A complete independently generated prismatic proposal passes1920 tets/525 points,
20 normal intervals, qmin.581909/Jmin.281381/full Lmax1.6680623. Exact rational
positive volumes sum1, oriented manifold/every vertex link, finite original facet
coverage, no interior overlap, both first-height rows and full numerical edge
integrals pass. The independent analytic field reproduces all30 native rejected
cell q/J/max-length measurements to1.28e-15/5.78e-14/2.06e-13 respectively.
The two flat walls have inverse tangent covariance eigenvalues<=1.5, below the
sensor>=4. Nearest-wall distance<=.5 is inside its full.54 band. Consequently the
full exact field is diag(s,s,1/hn(min(z,1-z))^2), s=4+12*x. Its normal span is
19.66094218; shared metric-spaced fronts avoid a direct unresolved jump.
Inspect prismatic_design_v1/candidate.su2 and its complete result/source receipts.
This is a feasibility witness, not native construction or work/time/MPI/CFD proof.

## Connected production change being qualified

Geometry-only admission recognizes a finite right prism with requested walls
confined to opposite planar caps. No Euler/RANS labels, case identifiers or saved
coordinates enter production. The constructor joins both exact first-height
planes through shared full-metric-spaced fronts. It refines cap triangles against
the actual field along the planned fronts, closes new column demands, and builds
coherently ordered three-tet prism subdivisions. Physical cap/side triangles bind
to the finite immutable authority. Existing whole-region shape/height/links/embedding/
full-edge validation and distributed immutable source service/atomic publication
remain in charge; failed proposals preserve accepted input and expose diagnostics.

Admission bounds source cap storage, normal front arrays, generated cells and
charged planning before allocation; elapsed time/memory use existing event limits.
Fresh identities exceed accepted and original donor identities. Unsupported side
wall junctions, slanted non-right prisms and ordered-front height conflicts have
focused controls. Current serial native and independent complete-mesh qualification pass; MPI and actual backend/CFD workflow remain pending.

This is one bounded regional construction on one worker, not general wall-front
collision/merging, arbitrary nonprismatic features, exterior geometry or scalable
parallel generation. Arbitrary cap triangulations can still block shape; the final
validator must reject them rather than weaken contracts. Wall-height conflicts
reported by this layout are not a global proof that every tetrahedral topology is
infeasible. The common test gives each refinement/removal phase its existing4096
work ceiling; actual backend shared-event accounting remains a separate workflow
gate. Preserve the full goal and record actual bounds without promoting pilots.


## Current complete serial acceptance

native_opposing_v3 passes native1920 BL cells/work2309, then250 original-sensor
cells/work1269. Independent exact geometry/no-overlap/manifold/links/finite
reference/volume1 and all actual field edges pass. BL qmin.467316/Jmin.193160/
Lmax1.668062861; removed qmin.254034/Jmin.121529/Lmax1.779940683. Original
six-cell input bytes are identical to the failure. All64 wall-face heights and
feet pass. All2674 full BL edges use the independent analytic composed field.

The separate exact coverage certificate proves all20 complete unit-area height
slabs, not just apex altitude: each cell lies in consecutive front planes and
slab tetrahedral volumes sum exactly to its width. Every one of46 wall-node
normal edges is present; no interior node lies below.02. Maximum centroid tilt
is13.1762 and is not claimed orthogonal. Actual viscous accuracy remains unproved.
The native source admission v2 error and independent coplanar-alias parser error
remain explicitly failed. Only the corrected saved-mesh audit was rerun; no
successful native case was repeated for that collector correction.

The test calls Refine with BL and Coarsen with sensors only. It does not yet prove
the actual backend's Refine-then-Coarsen removal sequence, refreshed source/current-
BDF transfer/restart, or shared-event control for this prismatic geometry. Those
are required next, alongside MPI and wider nonprismatic/perpendicular junctions.
No complete full-Goal3 or affordability claim follows from this accepted class.


## MPI qualification and the remaining removal failure

Production75777343c1 is published. Opposing BL generation accepts1920 cells/
work2309 on MPI1/2/4. Serialized complete mesh/finite reference/original-source
model is byte-identical; explicit equality and unchanged independent callbacks
reuse the complete serial proof for MPI4 construction. MPI2 direct removal
accepts230 cells independently. MPI4 removal rejects at4084 proposal work before
another batch fits4096, leaving496 private cells. Independent exact finite
geometry/embedding/volume1, every link, q.207827741/J.056364718 and all full sensor
edges<=1.781147819 pass for that rejected state. This is a search-budget refusal,
not evidence that the metric or geometry is infeasible. Native receipt remains
FAIL; the audit reports construction-only acceptance and rejected-state inspection.
MPI rejection messages corrupt some rejected MESH stdout lines; the intact regular
SU2 export is used for rejected geometry. Original traces are kept. Positive
serial/MPI2 audits retain their exact executed helper snapshots.

Next inspect the actual coarsening completion/work policy: the optimizer's remaining
short-edge agenda is not automatically an unmet hard mesh constraint. Do not
promote this saved result or weaken any gate; establish whether a general bounded
coarsening completion proof is consistent with the API, mandatory refusal/atomic
state tests, requested complexity and useful coarsening before changing code.
This is preferable to adding another collapse fallback without diagnosing why
valid geometry still receives a native rejection.

Current-source curved RANS N4/M2 v13 independently passes24->621->48, physical
current+BDF/open-ledger, explicit SA-floor replayv27, SU2/CGNS/reference and real
restart. It is a curved integration guard, not the missing actual prismatic
backend workflow. Direct engine Coarsen is different from the backend's Refine-
then-Coarsen sequence. Qualify that workflow plus rotated/localized tangential
sensors next; the present imposed sensor is affine isotropic. Cap triangulation
is inherited/refined and shared across fronts, so arbitrary anisotropic surface
alignment, tangential coarsening and localization efficiency are not established.
Do not substitute another easy geometry pilot for these known limitations.

## Performance evidence and next useful direction

profile_curved_bridge_v1 samples the already required curved RANS rank0 process
until its first native refinement report:1398 samples/no losses. Its original
helper misclassified the expected self-SIGINT return code-2; offline reports parse
the valid data. Both original and corrected receipts remain; no CFD/profile rerun.
Raw data is verified and compressed. About60.7% of inclusive root CPU samples are
under ComputeMetric, mostly Allreduce/failure-election waiting. Other ranks are
not sampled, so their critical numerical work is unresolved. Exact integer contact
projection/ConformingDisjoint is another substantial sampled cost. These are
inclusive overlapping CPU samples, not phase wall-time percentages or speedups.

Saved first curved regional validation is10.26s inside11.34s refinement. A useful
shared-kernel optimization to assess later is an exact-predicate face-halfspace
filter before full multiprecision tetrahedron contact: strict separation, or
separation with only common-ID vertices on the plane, proves intersection is
confined to the permitted shared simplex. Existing robust Orientation may provide
that proof cheaply. Keep the complete old exact contact path otherwise; test
shared-face/edge/vertex, nonconforming coplanar contact and extreme-scale cases
before any complete-mesh/performance comparison. This is an unimplemented design,
not a measured gain or permission to relax contact checks. Do not prioritize point-
interpolation micro-caches from evaluation counts alone.

All planned current cases have saved verdicts, with one native workflow failure;
all owned jobs are terminal. Full Goal3 remains active and incomplete.


Important next-field gate: new geometry admission also covers one requested wall.
The old CoupledBL single-wall test does not populate region_walls, so it continues
to exercise the old local path and cannot qualify the new one-wall regional path.
Test its known h.1/T.3/finer original-P1 field with actual region metadata before
claiming broad prismatic integration, alongside actual backend and rotated/localized
tangential fields. A maximum cumulative column prefix does not mathematically
bound each difference interval when the maximizing column changes; the current
constructor explicitly checks every resulting local interval and can reject.
This is an unqualified layout limit, not proof that such fields are infeasible.
