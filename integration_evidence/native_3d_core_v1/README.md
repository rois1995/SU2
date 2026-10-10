# Native 3D Goal 1: first geometry checkpoint

Active branch: codex/native-3d-core, based on the published 2D reuse checkpoint
038fdfa5786fb3f6af392d97257bfbb2bb29f036. Goal1 was activated 2026-10-10.
NATIVE_3D_DEVELOPMENT_GOALS.md remains the authoritative scope and completion
contract. This initial increment does not complete Goal1 or enable 3D remeshing.

## Implemented

Common/include/adaptation/CNativeMesh3D.hpp and the strict
Common/src/adaptation/CNativePredicates3D.cpp provide:
- Binary64 tetrahedral orientation with a long-double filter and bounded,
  allocation-free integer exact fallback. The sign is exact; magnitude is rounded.
  Counters distinguish filtered/exact calls. Arithmetic uses the existing
  strict passive library flags, separate from general SU2 fast math.
- Exact leading-principal-minor signs for tensor admission and scaled
  long-double Cholesky; unresolved factorization is rejected, never repaired
  by modifying the supplied metric.
- Scale-independent regular-tet mean ratio, minimum normalized metric vertex
  Jacobian, RMS and maximum edge length in a supplied constant query tensor.
  Minimum mean ratio .20 and minimum scaled Jacobian .05 are initial frozen
  shape gates for the first campaign. Full query edge/geometry/topology contracts
  are separate; nodal wall tensors are not averaged by this API.
- Focused Catch2 tests registered in UnitTests/meson.build. No changes to 2D
  source/operator guards or CNativeRemesher::CheckSupport; 3D driver use remains
  rejected until its actual implementation and validation are ready.

Local repair source inspection is recorded in SCRIPTS_REFERENCE.md and the
source-hash ledger scripts_reference.json. It supplies concrete candidate
algorithms for the next cavity implementation, not a production Python/MMG
dependency or a relaxed metric/quality contract.

## Local validation

Actual folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/geometry_controls_v2`.

PASS: six isolated Catch2 cases / 40 assertions, covering permutation signs,
coplanarity, cancellation, binary64 extremes, geometric scaling, rotated and
aligned aspect10000, flat slivers and invalid SPD inputs. An additional290
determinant signs match independent Python Fraction references exactly:
118 filtered and172 exact-fallback evaluations. Fractions operate on the actual
binary64 inputs, including random extreme exponents, near-coplanar cases and
exact degeneracy. These counts do not establish general predicate performance.

Sequential low-priority build/test with g++9.4.0 and Python3.8, library threads1;
no full SU2 build, CFD or MPI execution. Predicate compilation0.65s, Catch test
compilation9.86s, tests0.0045s and exact-oracle subprocess0.0054s in this observation.
These are execution observations under host contention, not speedup claims,
adaptation-event timing or a scalability benchmark. At the initial real-host
check a foreign MPI/Python run was active, so heavy work was deferred; the later
check showed that MPI run ended with one busy Python process remaining. Only the
small single-core low-priority kernel check was launched.

The first attempt geometry_controls_v1 passes its C++ tests but fails in the
Python runner: Python3.8 lacks math.ulp, and a relative __file__ broke the final
provenance writer. failure.json, the original runner and logs retain that failure.
The corrected runner uses a hexadecimal smallest-subnormal constant and resolves
its own path. v2 retains source/artifact hashes and commands in validation.json.

Reproduce in a NEW output folder, after checking host contention and announcing
that folder (the runner prints it and refuses to overwrite existing evidence):
```bash
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_geometry.py \
integration_evidence/native_3d_core_v1/geometry_controls_NEW
```

## Incidence checkpoint

Common/include/adaptation/CNativeTopology3D.hpp and its strict source add canonical
face/edge/vertex records and immutable indexed supplied-cell stars. Construction
rejects nonpositive/duplicate cells, duplicate vertex IDs within a cell,
inconsistent coordinates, nonmanifold face incidence and uncancelled internal
face orientations. It reports oriented boundary triangles and face-connected
components. This does not establish globally complete MPI stars, manifold vertex
links, embedding or a valid arbitrary replacement. See DESIGN_REFERENCES.md and
papers_reference.json for the local primary-paper assessment.

Actual check folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/topology_controls_v1`.
PASS: all10 Catch cases/93 assertions and the290 exact Fraction orientation
references. A six-tet cube verifies that a complete six-cell edge/vertex star
exceeds a two-cell face pair, orientation-preserving permutations leave incidence
unchanged and all12 exterior triangles point outward. Malformed record,
same-side/nonmanifold shared-face, disconnected and vertex-pinched controls are
included. Face connectivity is a diagnostic, not a full skin/link audit.
One sequential low-priority check only; no full SU2 build, CFD or MPI run.
Raw logs, commands and source/artifact hashes are retained in validation.json.
The current runner includes geometry and incidence; reproduce in a NEW folder.
Geometry-only v2 remains the immutable earlier checkpoint from191f35fd58.

## Private-cavity reconstruction checkpoint

See CAVITY_CHECKPOINT.md for implementation, preserved rejection and complete
limits. A bounded private coning primitive supports interior insertion/movement/
removal and some reconnections, with exact oriented interface/positive-cell and
manifold-link guards. It leaves output unchanged on rejection. PASS16 focused
cases/163 assertions and290 exact orientation references in cavity_controls_v3.
A stronger test first reproduced cancellation-prone volume-closure rejection;
failed v2 source/logs are retained, and closure now follows exact oriented faces.

Inspectable *_initial.su2 and *_candidate.su2 grids are in cavity_meshes_v2.
Independent Fraction volume/skin and pairwise tetrahedron SAT checks PASS8 probes:
five constant-metric/shape-gated controls plus three extreme geometry-only controls.
The latter intentionally fail isotropic shape limits and are explicitly labelled.
No CFD state, physical surface edit, driver or MPI adaptation is implied.

## Next work and limits

Extend geometric embedding validation and metric-gated coordinated proposals,
including full edge-star split/removal and bounded face/edge-ring reconstruction. Preserve immutable
original sensor queries; add indexed donor discovery and bounded query phase
costs before larger meshes. Adapt planar physical surfaces as coupled volume
proposals, then connect ownership/dependency/memory admission, MPI conflicts,
N=M/M<N partitions and mesh return.

No native 3D runtime remeshing, physical surface adaptation, BL construction,
solution transfer lifecycle or performance envelope has been validated.
Full Meson build/link, MPI and refreshed 2D integration regressions are pending.
The geometry/incidence-only controls produce no grids; cavity_meshes_v2 now
contains initial/private-candidate grids. Actual adapted CFD grids remain pending.
Goal tracker remains ACTIVE.
