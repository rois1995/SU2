# Native 3D private-cavity checkpoint — 2026-10-10

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core, following published incidence checkpoint4be53587cc.
Goal 1 remains ACTIVE. The runtime native support check still rejects 3D;
this increment is a private reconstruction foundation, not a completed remesher.

## Implementation and contracts

CNativeCavity3D.hpp/.cpp adds a bounded coning operation shared by interior
insertion, relocation/removal and some face/edge reconnections. The driver supplies
an apex and a reserved cell-ID range. Existing apex faces are omitted; all new
cells must have exact positive orientation. Failure leaves the caller's output
unchanged. Obstruction reasons include the particular face/apex IDs for subsequent
targeted dependency growth. Candidate selection, metric-balanced placement,
reconnection search and automatic growth are not implemented by this primitive.

ValidateFixedInterface checks unchanged oriented face IDs and exact boundary
coordinates, duplicate/coincident identities, internal-face cancellation,
face connectivity, spherical cavity skins, sphere/disk vertex links and manifold
edge/link connectivity. It admits at most64 source and128 replacement cells before
building incidence/scratch. These are explicit initial private budgets, not
universal valence/robustness limits. Non-ball patches are diagnosed/rejected.
An embedded accepted source cavity is a precondition; arbitrary damaged sources,
external entity collisions, complete MPI dependencies and physical geometry
references require separate validation. Metric acceptance is also separate.

The private interface is held fixed in this primitive. Goal 1 still includes
adaptable physical planar surfaces, coupled with their neighboring volume cells.
This increment does not redefine the goal to fixed physical boundaries.

## Reproduced rejection and correction

cavity_controls_v1 passes15 cases/155 assertions. A stronger rotated-thin regression
in cavity_controls_v2 reproduces false volume-closure rejections at1e6,1e8 and1e10
aspect, while1e4 passes. The rejected implementation sums rounded orientation
magnitudes and compares them with a volume-scaled tolerance; exact sign filtering
does not provide the relative magnitude bound that this tolerance assumes.
The failed run and exact original source/test files are retained.

The correction uses the invariant already checked: each internal oriented face
cancels and the remaining oriented faces and coordinates are identical before/after.
Signed tetrahedral volume therefore closes algebraically via boundary determinants;
there is no cancellation-prone sum or enlarged tolerance. Exact positive signs and
all topology/interface guards remain. This is signed-volume closure under the
stated source precondition, not an arbitrary mesh embedding certificate.

PASS cavity_controls_v3:16 cases/163 assertions and290 orientation signs checked
against independent exact binary64 Fraction determinants. Regressions cover
interior insertion/movement/removal,2→3 and reverse reconnection, outside/nonconvex
obstructions, changed interfaces, empty/oversized patches, ID overflow, duplicates,
disconnected cavities and a face-connected48-tet genus-one skin. Small sequential
single-core nice19 checks; no full SU2 build, solver or MPI launch.

## Inspectable candidates and independent audit

Final folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/cavity_meshes_v2`.

Each case has *_initial.su2, *_candidate.su2 and corresponding JSON with persistent
IDs, actual binary64 coordinates, a supplied constant tensor and native measures.
All boundary triangles carry the diagnostic private_interface marker; these files
are local cavities, not flow cases or validated physical marker transfers.

Eight probes PASS independent_audit.json. The standard-library auditor imports no
native/script predicates. It uses exact Fraction geometry for positivity, all
pairwise tetrahedral separating-axis tests for positive-volume intersection,
oriented face cancellation, boundary coordinate/skin equality and exact volume
sums. It separately checks constant-tensor SPD, metric edge lengths, mean ratio
and scaled Jacobians against the native outputs. SAT overlap/separation/contact
negative controls are included. Lower-dimensional geometric contacts and arbitrary
global embeddings still need the later independent full-mesh audit.

Five probes enforce frozen q>=.20, min scaled Jacobian>=.05 and max metric edge<=1.8:
insertion1→4, movement4→4, removal4→1, reconnection2→3 and rotated-aspect10000 insertion.
Across their candidates qmin=.3485660541, Jmin=.1561737619 and Lmax=1.4142135624.
Three rotated1e6/1e8/1e10 probes test geometric closure only: their isotropic
shape gates deliberately FAIL, explicitly flagged metric_gates_required=false.
They are not accepted anisotropic meshes or extreme metric-factorization evidence.

The earlier five-probe folder cavity_meshes_v1 is retained with its original
harness files; all10 SU2 mesh files are byte-identical in v2. Current probe timings
and predicate counts are recorded, with a single observation only. Private
reconstruction time excludes export/measure/audit; counters cover reconstruction
and the exported source/candidate measures. Compile/test/audit stage times are
separate. No speedup, full adaptation affordability or scaling claim.
The quadratic all-pair Fraction auditor is intended only for these tiny controls;
upgrade to indexed broad-phase auditing for actual mesh-scale campaigns.

## Replay and retention

After checking contention and announcing a NEW output folder:
```bash
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_geometry.py /absolute/new/controls_folder
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_cavities.py /absolute/new/meshes_folder
```
Run sequentially. Runners refuse existing folders and retain failure logs/receipts.
Source/artifact SHA256 and exact commands are in each validation.json. Compiled
binaries/objects remain ignored locally; raw user ClusterResults are not staged.
Legacy source/test snapshots are matched to their original receipt hashes:
- cavity_controls_v1/original_CNativeCavity3D_tests.cpp;
- cavity_controls_v2/original_CNativeCavity3D.cpp and original_CNativeCavity3D_tests.cpp;
- cavity_meshes_v1/original_cavity_probe.cpp, original_audit_cavities.py and original_run_cavities.py.
The old cavity source in v2 also matches the earlier v1 control/probe receipt.
Other pinned inputs are unchanged. Preserve these historical failures and probes.

## Next work

Implement indexed immutable original tetrahedral sensor-only P1 donors, actual-query
composition and bounded query caches/cost counters; then apply metric/shape/edge
acceptance to coordinated proposals. Extend full edge-star splitting, removal,
movement and bounded face/edge-ring search, including targeted obstruction import.
Implement physical planar-facet surface changes as coupled volume transactions,
then MPI ownership/dependencies/conflicts/versions/memory admission, weighted
partitions/M<=N workers and return/rebuild/transfer. Full Meson link, MPI1/2/4,
2D refreshed integration, SU2/CGNS lifecycle and practical performance envelope
remain required by the original goal; this checkpoint completes none of those.
