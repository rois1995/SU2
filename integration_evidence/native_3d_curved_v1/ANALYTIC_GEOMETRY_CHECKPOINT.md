# Analytic geometry and coupled split checkpoint

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local, based on published Goal1.
Status: private geometry checkpoint qualified locally; Goal2 INCOMPLETE.

## Implemented

The existing PlanarReference accepts explicit immutable analytic definitions per
original finite facet ID. Sphere uses radial projection; bump uses the declared
polynomial graph z=z0+A*u(1-u)*v(1-v). Definitions are supplied, never inferred
from marker names or a faceted mesh. Unannotated facets keep exact planar rules.
Finite radial/XY footprints and oriented full-face checks preserve facet and
marker identity. Curved broad-phase boxes cover the represented surface; sphere
boxes conservatively cover the whole sphere, so tighter indexing remains possible.
This is no geometry-search performance qualification.

SplitEdge requires its input chord point to be exactly representable on the
original edge. It projects against declared incident analytic facets, checks all
incident physical constraints, then replaces the complete edge star. Both child
determinants must be positive in inherited connectivity; negative children are
rejected rather than repaired by reorientation. Interface coordinates stay exact.
UndoEdgeSplit admits the same finite projected edge and restores paired parents.
Geometric proposals remain private and preserve both outputs on rejection.

Physical skin validation now checks complete oriented faces. Curved fan insertion
still rejects pending complete chart-union reconstruction. Existing general move,
collapse and reconstruction are not qualified for curved runtime by this study.

Version2 native reference sidecars persist analytic definitions and frozen
projection/chart tolerance. Version1 encoding/decoding stays available and default
encoding stays version1. Count/file/construction-memory checks include analytic
records. Encoding verifies saved facets/markers match immutable authority. The
existing collective loader elects failures before state publication.

Runtime adaptation explicitly refuses analytic authority until sensor coverage
and final geometry completion gates are integrated. This prevents passing the
sensor-only planar completion check and publishing an unqualified curved mesh.
No sensor interpolation, BL/fade, Hessian/noise, solution-transfer or MMG policy
was changed. No configuration option was added.

## Evidence and experiments

Final controller: analytic_geometry_v3/run.py and validation.json, PASS.
Incidental planar regression mesh/state/history/input payloads in v2/v3 are retained
in planar_regression_payloads.tar.gz. payload_archive_validation.json verifies all
archived bytes against the original immutable controller artifact hashes. Logs,
analytic sidecars and private curved JSON/SU2 grids stay directly inspectable.
Extract an archive into its stage directory to restore the original payload paths.
Full configured 68-object test driver, source/archive/member/binary pins, one CPU,
nice19, threads1, build-j1, MPI1/2/4. There are 86 cases per rank, including new
geometry/reference checks and connected original3D mesh/reference/bridge/runtime/
default rejection and2Dfield regressions. Existing planar runtime cases exercise
M=N/M<N, output and BDF history handling; this does not establish curved MPI
publication or curved transfer/restart. Curved private arithmetic is repeated on
each rank; only reference I/O/refusal is an actual collective curved integration.

Private accepted volume proposals are saved under mpi1, mpi2 and mpi4 as
sphere_original/split/coarsened and bump_original/split/coarsened JSON and SU2.
They are operator grids, not a solved CFD case or full metric acceptance.

Independent controller checks: oriented skin equals cell boundary, positive
volume, unchanged old coordinates, exact restored connectivity/physical labels,
projected point on declared geometry and matching MPI artifacts. Independent
additional audit_private_shapes.py uses exact rational separating-axis tests
for no interior tetrahedron overlap and exact signed-skin/cell-volume equality,
and identity-metric mean-ratio >=0.20/scaled-Jacobian >=0.05 on all saved grids.
Identity-metric shape checks do not certify a CFD-derived target edge length.

Initial sphere volume 0.5132002392796676 increases to 0.7010445640842783 after
projection; bump volume 0.3333333333333333 increases to 0.3541666666666667.
Both reverse exactly in the saved audits. Requiring old/new faceted volumes to
match would incorrectly reject these surface refinements. This is geometric
closure evidence, not solution conservation; transfer/new-lost-domain accounting
is still to be exercised on actual curved accepted runtime events.

Split grids have minimum identity-metric mean ratios about 0.819 (sphere) and
0.808 (bump), scaled Jacobians about 0.628 and 0.471. Projection residual is zero
in these fixtures, checked against a frozen absolute tolerance of1e-12.

analytic_geometry_v1 retains the first failed test and exact source.patch:
annotating an octant's sphere face alone cannot expand its adjoining finite
planar side triangles. The failure is retained; no finite boundary gate was
loosened. Final tests explicitly require atomic rejection for that missing mixed
reference capability. Accepted controls cover a closed spherical tetrahedron and
a two-cell bump wall star where the edited edge has both required curved faces.
This does not resolve the mixed curved/planar geometry gap.

analytic_geometry_v2 retains its source.patch and passing preliminary evidence.
Source review found that allowing each infinite supporting plane a tolerance
could amplify that allowance at acute chart corners. Finalv3 uses actual distance
to finite boundary arcs/edges after exact footprint tests. Acute sphere and bump
corner controls reject points1e-6 away and admit1e-13 roundoff within the same
frozen1e-12 bound. This corrects the admission model; it is not target relaxation.

## Required continuation

1. Complete geometry authority for supported cylinder/feature intersections,
   finite component chart unions and coupled curved movement/collapse/reconnection;
   supply conservative final surface-error and nonlocal clearance checks and
   geometry-driven candidate/completion logic. Preserve faceted mode and reject
   unsupported intersections or exhausted dependencies atomically.
2. Integrate the bounded associated-triangle sensor trace and consistent exterior
   edge integrals before freezing. Keep original physical P1 unchanged wherever
   covered, compose geometry at actual queries, and prevent wrong-component or
   coarse-cell spreading. Initial coarse curved facets can have projection gaps
   larger than final Hausdorff tolerance: explicitly derive/admit source coverage
   from declared geometry, rather than silently using final Hausdorff as the only
   envelope. No production ghost-tet or query-only fallback has been selected.
3. Remove runtime refusal only when geometry/source/completion contracts are
   connected, then qualify actual local MPI1/2/4, M=N/M<N, interface edits,
   transfer/current+BDF/positivity/conservation/SU2CGNS/reference/restart.
4. Run actual small3DEuler solution-derived-metric adaptation/continue/restart,
   manufactured controls and locally feasible resolved nonuniform/shock cases.

All four full Goal2 gates remain pending. No cluster, timing/scaling or efficiency,
production3DBL, generalCAD, full physical unsteady3D or force-convergence claim.
