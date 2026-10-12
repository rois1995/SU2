# Original-first declared exterior sensor coverage

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-curved-local, based on33b70f3273.
Goal2 ACTIVE; all four integrated completion gates remain pending.

## Implemented field contract

DonorCell now transports an immutable extension component ID. UINT64_MAX marks
original CFD-source cells. Other IDs mark explicitly declared finite exterior
coverage cells whose coordinates and sensor-only tensors are supplied before
freezing. FrozenField admits an exterior component only when the caller explicitly
associates it in its constructor; default callers keep original-only behavior.
All supplied records undergo finite positive-tetrahedron/shared-node/SPD/duplicate
validation, even when their component is not associated. Source records and
component association are copied, so caller mutation cannot change the target.

Original-source cells sort before exterior cells while retaining canonical
original-node keys within each group. Query uses exact containment and preserves
original physical-space P1 at every point covered by the complete admitted
original patch, including overlapping exterior coverage and original boundary
points. Exterior cells fill only uncovered points and remain bounded by their
explicit tetrahedra. Unassociated/out-of-envelope queries reject. FieldSample
and statistics identify exterior use. Sensors remain in global Cartesian axes;
no BL tensor is placed in an exterior node. Actual-query composition still runs
on each Query, including sensor-cache hits. TargetEdgeLength still rejects a
composed field lacking its geometry-resolved integrator.

SensorEdgeLength uses exact rational clipping and interval ordering. Original
cells take priority on covered intervals, and declared exterior cells fill gaps.
It never picks a field from a rounded midpoint. Exact subinterval widths and P1
endpoint weights retain nonzero contributions even when displayed cuts coincide.
Each affine sensor quadratic form is integrated analytically with the existing
stable sqrt-affine formula. Ambiguous overlaps at the selected interval priority,
coverage gaps and invalid priority prefixes reject. Each priority group's source
must be embedded and conforming; point containment and MPI identity validation
alone do not certify global embedding. Canonical conforming-face/edge ties
remain deterministic. The original-only path retains its previous exact trace.
The new path is bounded by256donors and at most513intervals; no timing/scaling
qualification follows. Native engine/query-buffer byte models now conservatively
include bounded layered trace, imported records/index and sensor cache; a compile
assert checks the per-donor exact-cut storage allowance. Models still exclude
CFD allocations, allocator/RSS and MPI-internal storage.

MPI DonorImport transports the component tag through the existing source routing
and immutable source validation; all tensor/coordinate and cell/simplex authority
checks remain. The field still depends on complete original-source discovery for
its actual query footprint. A missing original donor cannot be repaired silently
by an exterior record; expanded private/reader query admission remains part of
runtime integration.

## Evidence

layered_sensor_v2/validation.json PASS: full68-object driver,95cases/rank MPI1/2/4.
Exact source/archive/member/binary pins and unchanged-source checks retained.
Selected controls include original3D/2D fields, planar native bridge/default/
runtime, analytic geometry/reference/chart tests, and new layered field/MPI tests.

Conflicting overlapping exterior tensors intentionally prove original P1 priority
independent of delivery or node IDs. A scalar example has original xx=1+3x on
x in[0,1], exterior xx=9 on[1,2]: the exact edge integral is41/9, with original-only
integral14/9. Forward/reverse, immutable association/source, boundary preference,
wrong-component/default-disabled/beyond-envelope and actual-query/cache-hit
composition controls pass. Coarse exterior tensors do not suppress thin original
sensor peaks, including source intervals whose numerical endpoints coincide.
The composed-target negative control still rejects sensor-only substitution.

A real MPI source import places a tagged exterior donor on the last rank and
original cube donors across ranks. Every requester imports the same tag, evaluates
original/exterior queries and edge integrals consistently with the complete frozen
oracle, and rejects an unassociated component. This proves source-tag routing and
field consistency, not curved MPI mesh publication or current/BDF transport.

The first layered_sensor_v1 experiment is retained FAIL with exact source.patch.
Its exterior tetrahedron covered the queried outside point but left a segment gap
between that point and the cube. The integrator correctly rejected it. The corrected
fixture covers the adjoining boundary triangle and full tested segment; the old
gapped tetrahedron remains an explicit negative control: point queries pass while
the uncovered edge rejects. No metric/geometry/roundoff tolerance was relaxed.

A coupled private sphere split uses a declared finite radial shell, supplied and
associated before freezing. This witness is geometry-aware but manually prescribed,
not the production shell generator. Original physical P1 is retained inside; the
new projected wall point outside the original tetrahedron is covered by the shell.
Actual replacement vertices/centroids and all edges pass sensor evaluation and
identity-shape/sliver/length checks (mean ratio>=0.20, scaled Jacobian>=0.05,
maximum metric edge<=1.6). Sensor tensors are0.5I; the frozen geometry projection
bound remains1e-12. The radius factor4coverage is declared upfront. This does not
establish final curved-facet Hausdorff compliance or solution conservation.

Independent private_shape_validation.json PASS checks all saved sphere/bump/joint
and new layered sphere grids: exact rational no interior tetrahedron overlap and
signed-skin/cell-volume closure, plus identity shape/sliver gates. Current
layered_sphere_split.su2/JSON is directly inspectable under layered_sensor_v2/mpi1,
mpi2 andmpi4. These remain private operator grids, not solved CFD cases. Other
current private grids and analytic sidecars are retained. Incidental planar
regression input/mesh/state/history payloads are byte-verified in each stage's
planar_regression_payloads.tar.gz with payload_archive_validation.json. Extract
into that stage directory to restore original artifact paths. Archive verification
PASS does not change the first experiment's FAIL status.

## Required continuation

Generate and admit bounded geometry-aware exterior source coverage automatically
from actual CFD boundary ownership and sensor data before freezing; choose and
validate its declared target definition, component provenance, bounds and MPI
identities. The manual shell does not select a production padding policy.
Connect actual private/remote query-footprint discovery and associated components
through engine, weights and reader paths; preserve complete original P1 priority.
Connect geometric error/clearance candidate and completion checks, supported
cylinder/feature and mixed-reference authority, then remove runtime refusal only
when all contracts are connected. Exercise real curved MPI1/2/4 andM<N replacement,
transfer/current+BDF/positivity/conservation, SU2/CGNS/reference/restart and actual
small3DEuler CFD-derived-metric cycles. Those gates remain pending.

No new cfg/dependency, pressure/Hessian/noise/fade/BL/MMG/solution-transfer policy
change, cluster work or performance/efficiency/scaling claim. No production3DBL,
generalCAD, full physical unsteady3D or CFD-force-convergence qualification.
