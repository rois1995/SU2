# Local repair scripts: references for the native 3D goal

Inspected 2026-10-10 at the user's suggestion:
`/media/rausa/4TB/MeshAdaptation/Scripts`.
This is a source review, not a validation of those workflows on new inputs.
Source hashes are recorded in scripts_reference.json; original files are untouched.

| Component | Useful for SU2 native 3D | Adaptation required |
|---|---|---|
| MetricOptimize3D_Fun.py / optimizeMetricPatch3D | Coupled movement in metric coordinates, analytic shape/size derivatives, bounded outer refresh, actual-field acceptance and retained-neighbor constraints | Bounded native solve, actual frozen P1 sensor plus geometric queries; configurable work admission and per-phase/query cost |
| MetricOptimize3D_Fun.py / flipMetricPatch3D | 2-to-3 and interior edge-ring reconstruction (valence 3..7), including direct 4-to-4 and larger reconnections | Full distributed edge/vertex stars, version/conflict checks, feature constraints and atomic commits |
| MetricOptimize3D_Fun.py / splitMetricPatchEdges3D | Complete edge-star insertion followed by new-point movement and joined checks | Metric-balanced candidates, adaptable physical faces, native IDs and admitted private scratch |
| MetricPatchRepair3D_Fun.py / selectMetricPatch3D | Size cushion, manifold vertex-link closure and fixed artificial-face feasibility diagnostics | Avoid global graph/Dijkstra or whole-mesh scans in private MPI repair; import obstructing dependencies selectively |
| MetricPatchRepair3D_Fun.py / tetMetricMeasures3D | Separate metric mean-ratio shape and RMS size; flat slivers fail shape despite reasonable edges | Supply the actual-query tensor and test individual query edge lengths; do not average composed wall tensors across coarse cells |
| MeshEmbedding3D_Fun.py | Filtered/exact orientation, surface-contact checks, oriented manifold skin and component nesting checks | Native bounded broad phase and exact predicates; MPI ownership/dependency closure and mesh/cavity audit |
| MixedMeshRepair_Fun.py | Coupled coordinate untangling, analytic Jacobians and trial validation | Recovery of damaged input is separate from normal adaptation of accepted valid CFD meshes |

The first native kernel uses the same regular-tetrahedron mean-ratio definition
as tetMetricMeasures3D, plus minimum metric-scaled vertex Jacobian as an explicit
sliver diagnostic. Initial frozen native 3D shape gates are q >= 0.20 and
minimum scaled Jacobian >= 0.05. These are proposed engineering gates, not
validated robustness or CFD-accuracy limits. The existing edge/query contract
is additional; the scripts' RMS [.5,2] criterion is not a replacement for it.
The regular-tetrahedron vertex Jacobian is normalized to one by sqrt(2).
All geometric acceptance and global embedding checks remain additional.

Important differences in the current script workflow:
- repairLocalMesh3D rejects non-fixed boundary policies and requires MMG3D.
  Its standalone local operators are more directly useful than its full controller.
- The resolution field is deformation-transported from independent samples and
  includes visible log-metric interpolation. SU2's original sensor-only P1
  interpolation, geometric BL composition, fade and finer demands remain authoritative.
- Retained source cells may carry quality/size exceptions; the native strict
  adaptation gates cannot inherit these exceptions silently.
- Sparse SciPy solves and VTK/Numba locators are serial reference implementations;
  there is no demonstrated MPI or native performance equivalence.
- The 3D Euler repair state explicitly rejects physical viscous-layer targets.
  Availability of a 3D wall metric is not demonstrated BL mesh-generation support.

Use these routines to guide and benchmark bounded native proposals, not to
change the prescribed problem or introduce a production Python/MMG dependency.
No imported script is run automatically. An immutable artificial cavity
interface can stay fixed while physical surface connectivity adapts in the
coupled native proposal. New operators still need independent MPI, geometry,
metric, failure and performance validation in Goal 1.
