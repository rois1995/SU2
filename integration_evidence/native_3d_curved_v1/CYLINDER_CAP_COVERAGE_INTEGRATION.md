# Cylinder/cap coverage integration

Repository SU2_NativeIntegrated; branch codex/native-3d-curved-local. Goal 2 remains
ACTIVE; the full completion audit remains INCOMPLETE.

The saved tilted-cylinder path had an exact frozen-source gap at projected cap-rim
points. A coordinated strip could satisfy topology and geometry, but actual sensor
edge integration rejected it. Existing cylinder radial frusta did not cover the
cap's representability envelope; disk faces were skipped. Scientific coordinates
show an uncovered tail about1e-18 long; native reprojection differs by ULP, so this
number is not a native-gap measurement.

Coverage now matches declared end disks to a unique declared cylinder and builds
one conforming source collar before freezing. The side and both caps use the same
radial/axial nodal displacement and transient identities. The signed axial padding
is max(4*geometry tolerance,128*double epsilon*coordinate scale); the existing
radial geometry envelope remains. Shared original-node ordering chooses compatible
prism diagonals. The finite envelope, unique association, cap/side orientation,
representability, cell/node identity and collective storage checks reject invalid
construction. Parent-cell routing and component tags remain intact.

The original CFD cells and sensors are unchanged. Original-P1 always has priority;
extended nodes copy the original boundary tensors exactly. The original frozen
72-cell fixture remains a negative regression, while a new actual CFD import
produces24 original+96 extension tetrahedra with36 shared nodes. Constant and
varying isotropic sensor controls exercise preservation of boundary demands and
original centroid weights, complete strip edges and finite-envelope refusal. No
post-rejection extension, containment epsilon, metric reset or acceptance change.

Focused cylinder_cap_coverage_v2 PASS at MPI1/2/4: five selected test cases per
rank. All remeshing-worker counts M=1..N complete the primitive two-event runtime,
actual conservative current/BDF replacement, restart and SU2/CGNS readback. The
previously failing tilted cylinder now finishes at42 cells/32 points, like the
axis-aligned control. Sphere remains26 cells/18 points. Existing runtime operators
complete the tilted control after correcting the source envelope. Automatic
coordinated-strip placement/publication remains an unimplemented optional operator
extension; priority moves to the remaining full-goal CFD/geometry requirements.

Independent cylinder_cap_audit_v1 PASS:

- Fresh collar: exact positive source geometry, no overlap with itself or original
  cells, shared seam coordinates/tensors, copied boundary demands, actual centroid/
  vertex original-priority samples and exact complete metric edge integration.
  Physical source signatures agree at MPI1/2/4 for constant and varying sensors.
- Primitive accepted states:21 complete variants (3 serial,6 onMPI2,12 onMPI4),
  two events each, no missing variants. Exact geometry/positive volumes/manifold/
  no-overlap, source-resolved target lengths, shape/sliver gates, transported
  metrics, actual current/BDF integrals/positivity/new-lost-domain accounting,
  authoritative sphere/cylinder errors, restart states and SU2/CGNS signatures.
  Minimum q0.2018513908; maximum metric edge1.5811388301.

Initial cylinder_cap_coverage_v1 build failed from a private helper used by the
new test; its source diff and failed logs are retained. Broader regression_v1
passed105/107serial cases, failing four diagnostic-string assertions after the
previous projection-helper refactor. Existing node/facet-specific diagnostics are
restored. The correction changes messages only, and source-generation code remains
identical to the independently audited focused run.

Broader cylinder_cap_regression_v2 PASS:107 registered cases/rank at MPI1/2/4,
planar/curved kernels, bridge/default/runtime, 2D field controls, coordinated
operators and old-source rejection, manufactured curved grow/shrink/refine/coarsen
with CGNS restart, and actual solved Euler metric refresh/remesh/SU2 restart/
continuation. Independent lifecycle audit also PASS on these fresh outputs,
checking exact embedding/target edges/transported tensors, actual current/BDF
conservative integrals/positivity and restart/output signatures. Solved pressure
metrics are nonuniform, anisotropic and refreshed between events. This small
subsonic bump remains a control, not a resolved shock or force-converged case.
All owned jobs are terminal; no heavy job remains live.

All source/build/binary/member and required grid/state/reference/ledger pins are
retained. One owned heavy job, one CPU/nice19/threads1, build-j1 and MPI<=4; no
cluster/performance/scaling claim. Remaining full-goal work includes a resolved
nonuniform/shock Euler control where locally feasible and an explicit final audit
of geometry, features/components, partition-interface operations and all declared
requirements. No production3DBL/generalCAD/fullphysical-unsteady/force-convergence
or arbitrary-mesh-feasibility claim is established.
