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
