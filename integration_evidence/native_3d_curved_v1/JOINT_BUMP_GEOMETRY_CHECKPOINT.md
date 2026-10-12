# Compatible bump charts and joint curved volume edits

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-curved-local, based on published geometry checkpoint7c1dcd5fe8.
Goal2 remains ACTIVE and all four integrated completion gates remain pending.

## Change

Declared bump facets with identical marker, origin, extents, amplitude and frozen
tolerance now reuse the existing planar connected-convex component certification
in XY parameter space. Their original physical facets and analytic records remain
immutable. Certified convex components acquire a canonical chart identity; distinct
markers, geometry definitions and disconnected components remain separate. Existing
nonconvex/uncertified components retain their individual finite constraints. This is
an analytic geometry capability based on supplied definitions, not inference from
marker names or recovery of an unknown physical wall.

Complete finite chart membership uses exact XY containment first and the existing
bounded finite-boundary distance for roundoff. Whole-face orientation remains in
chart coordinates. The physical broad phase encloses each complete component;
construction/resident byte models include retained chart references and scratch.
Only one shared chart is charged per identical-definition group, including its
multiple disconnected component roots. No new dependency, config or sidecar
version was introduced. Version2 stores original definitions and reconstructs the
same canonical chart union after reload; version1/default faceted rules remain.

Private edge-cavity insertion projects an analytic surface proposal, rebuilds a
positive chart-oriented wall fan and cones the retained interface only when the
new point sees every retained face. New point placement can be off the old edge.
Complete vertex-star movement projects the candidate and returns projected
replacement coordinates; subsequent field/shape checks must evaluate those actual
coordinates. It rejects projection back to the
original position. Curved movement/collapse use finite chart orientation rather
than an arbitrary physical Cartesian projected-area sign. All cell determinants
and prescribed interface checks remain, with atomic output rollback on rejection.

These operators require the caller's full dependency/identity admission and final
metric and external-clearance checks. The production engine's analytic-runtime
refusal remains unchanged pending donor coverage and final geometric completion.
Sphere finite-chart behavior is retained; this study exercises the extended joint
freedom on bump charts. Cylinder and mixed curved/planar patch expansion remain
unimplemented. General nonconvex chart unions and nonlocal collision certificates
are not established by this checkpoint.

## Evidence

joint_bump_geometry_v1/validation.json PASS: complete68-object driver,
90cases/rank MPI1/2/4, including existing3D mesh/reference/bridge/default/runtime
and2Dfield regressions. Actual collective bump sidecar write/load reconstructs
canonical chart identities and permits rebinding a changed surface diagonal.
Curved proposal arithmetic is executed locally on each rank, not a distributed
curved remeshing transaction. Existing planar runtime M=N/M<N/history/output
checks remain regressions; no curved MPI/state/restart completion claim follows.

New accepted witness: two roof triangles form a single finite square bump chart.
Insert a point(.6,.3,.0504) away from the old diagonal, move it to(.8,.7,.0336)
across the old triangle seam, then collapse it to a retained corner. The final two
tetrahedra use the other coarse diagonal. Three further insert/collapse cycles
alternate diagonals while retaining the declared geometry and quality gates.

Original volume1/3, inserted0.35013333333333335, moved0.3445333333333334,
collapsed1/3. Old vertices remain bit-identical. The code does not enforce identical
old/new volumes during surface movement. Cell removal restores volume here because
all retained coarse roof corners lie on the same plane. This is not solution
conservation; current/BDF transfer on a curved accepted runtime event remains open.

Independent controller checks verify oriented skin, positive volume, unchanged
original coordinates, different final connectivity, geometry residual<=1e-12 and
matching artifacts acrossMPI1/2/4. audit_private_shapes.py/private_shape_validation.json
also verify identity mean ratio>=0.20/scaled Jacobian>=0.05, exact rational
separating-axis no interior tetrahedron overlap and exact signed-skin/cell-volume
closure. These identity shape checks do not certify a CFD-derived sensor edge
length or final curved Hausdorff tolerance.

Inspect actual private grids directly in joint_bump_geometry_v1/mpi1 (andmpi2/mpi4):
joint_bump_original.su2, joint_bump_inserted.su2, joint_bump_moved.su2,
joint_bump_collapsed.su2, with matching JSON. Earlier sphere/bump controls also remain.
These are operator meshes, not solved CFD cases. Incidental planar regression
mesh/state/history/input data are retained in planar_regression_payloads.tar.gz,
with payload_archive_validation.json verifying every byte against the original
immutable controller receipt. Extract into this stage directory to restore paths.

Negative controls retain both outputs for outside-chart insertion, missing physical
interfaces, projection back to the old vertex, nonfinite movement, collapse into a
nonincident geometry, and marker junction crossing. Different primitive definitions
never merge; disconnected identical-definition patches reject queries in the gap
and across components. Insufficient chart construction budget rejects before
construction. Previously saved mixed curved/planar and acute-corner controls pass.

## Continuation

Connect bounded frozen sensor coverage and consistent original/exterior edge
integration; retain exact original physical P1 wherever covered and actual-query
composition. Connect geometric-error/clearance selection and completion plus
supported cylinder/feature authority before removing runtime refusal. Then prove
actual curved MPI1/2/4 andM<N publication, current/BDF/positivity/conservation,
SU2/CGNS/reference/restart, and genuine small3DEuler CFD-derived-metric cycles.
No cluster, performance/scaling, production3DBL, generalCAD, full physical unsteady3D
or CFD-force-convergence claim. Full Goal2 scope and gates are unchanged.
