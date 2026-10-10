# Native 3D original sensor field checkpoint — 2026-10-10

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core after cavity checkpointe7f87ecf4a.
Goal 1 remains ACTIVE; runtime native support still rejects3D. This is original
sensor/query infrastructure, not completed metric-gated remeshing or MPI support.

## Strict geometry and tensors

CNativePredicates3D.cpp now provides magnitude-controlled P1 weights with exact
containment signs. A sign-safe determinant filter alone does not bound a thin
cell's barycentric-ratio error. The new weight path admits the filtered magnitude
only with relative bound1e-13; otherwise the existing bounded exact integer
fallback supplies its rounded magnitude. Weights are nonnegative and normalized;
outside queries leave the caller's weights unchanged. No clipping/extrapolation
or change to the original tensor target is used. Ordinary orientation filtering
retains its previous sign criterion. ValidateTensor shares finite-component and
exact Sylvester admission with Measure; no SPD repair or condition-number cutoff.

## Immutable field and actual-point composition

CNativeField3D.hpp/.cpp copies original tetrahedra and SENSOR-ONLY nodal tensors,
checks positive donors and shared coordinate/sensor consistency, and sorts by
canonical original-node keys. Those keys resolve containing-donor ties independently
of cell IDs and arrival order. Donors have no mutation API during a remesh event.
The source snapshot is bounded to256 donors per imported patch; larger discovery
requires the later bounded distributed import/query service, not target substitution.

For multiple donors, reuse SU2's existing CADTElemClass with the two-corner LINE
box representation already used by 2D imports. The ADT is a conservative box broad
phase; exact donor containment remains authoritative. Zero/single donor patches
allocate no tree or traversal buffers. This is a local imported-patch index;
MPI routing/discovery and full dependency admission are still pending.

A bounded2048-entry FIFO cache stores original sensor samples, donor index and
weights at exact binary64 coordinates. It never caches composed wall tensors.
The composition callback is evaluated at EVERY actual query, including cache hits,
and its resulting tensor must pass SPD admission. Query returns original sensor,
composed target, canonical donor key and weights for later residual audits.
Rejection retains original data; a failed composition may reuse a valid cached
sensor on retry, then recomposes and rejects again. No evolving-cell interpolation.

Current donor admission is EXACT containment. Physical-reference-associated
roundoff admission/extension is not yet implemented: do not silently project or
extrapolate through an unassociated boundary. This is an explicit remaining step
before general surface queries/remeshing. The manufactured composition test uses
a thin normal floor only to test query semantics and finer-demand preservation;
it is not a production 3D geometric BL/fade/combined-gradation provider.
Native2D/MMG source, noise defaults, fade and actual BL policy remain unchanged.

## Profiling and bounds

FieldStats counts requests, hits/misses, invalid queries/failures, FIFO evictions,
box candidates, exact containment attempts, interpolation/composition attempts
(including rejected validation), peak cache entries/candidates and actual retained
ADT bytes. Kernel counters distinguish filtered/exact orientation evaluations.
Detail timers are optional and default OFF. Query wall time is the parent;
cache lookup/insertion, search+weights, scalar sensor interpolation/admission and
composition/admission are mutually exclusive child phases. Parent-minus-children
covers other local work; do not add parent and children. Build time covers canonical
sorting, node validation and index construction after ownership of the input
vector is moved; caller-side snapshot construction/copies belong to the import
parent phase. No MPI waits/remote transfers or full-event closure are measured yet.

The supplied donor count, cache count and original exact fallback are bounded.
Constructor/index/query heap admission still needs integration with the existing
transfer-memory ceiling. A256-donor separated-box control returns one box candidate
and one containment attempt. This demonstrates pruning, not a timing speedup.
Avoiding an unnecessary singleton tree also preserves all210 oracle result lines
byte-for-byte. No repeated benchmark, instrumentation-overhead measurement,
practical whole-mesh envelope or affordability claim is made here.

## Evidence

Final focused folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/field_controls_v4`.
PASS22 Catch cases/2294 assertions, plus290 exact Fraction orientation signs.
Tests include prior geometry/incidence/cavity controls, thin rotated weight
fallback, affine sensors, canonical ties, immutable snapshots, actual-point thin
composition without coarse-cell spreading, finer sensor demands, ADT pruning,
2048-cache eviction, timer closure, invalid input/extrapolation and failed composition
work counts. Actual SU2 ADT/base and SERIAL MPI-wrapper objects are compiled/linked;
no stub replacement. One low-priority process at a time under foreign Python/MPI
activity, library threads1. No full Meson executable, solver, MPI or AD-build test.

Final independent folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/field_oracle_v3`.
PASS210 actual indexed queries:150 exact-inside,60 exact-outside. Independent
Python Fraction geometry uses the actual binary64 donor/query inputs, not nominal
coordinates before rotation/rounding. Cases span aspect1..1e10, coordinate scales
1e-50/1/1e50 and translations0/1000 scales, including vertices and outside points.
Sensor weights and five directional quadratic forms of the six-component tensor
are compared to exact P1 references; nodal zz is deliberately1e10..4e10.
Max absolute weight error8.1262e-17, max directional tensor defect1.5378e-16;
tolerance1e-12.294 exact predicate calls recorded. This establishes the sampled
P1 queries, not extreme rotated-metric factorization or transported CFD solutions.

The initial field_controls_v1 compile failure is retained: strict Werror exposed
unused parameters in existing no-op AD/option headers. The runner now matches
SU2's own Wno-unused-parameter setting for that compilation. The original runner
is retained. v2 passes before singleton-tree removal; v3 passes afterward. Each
keeps its original field source/test snapshot for exact receipt replay. v4 adds
failed-work counters. Oracle v1/v2/v3 responses are byte-identical. Commands,
source/artifact hashes and stage observations are retained in validation.json.
Compiled objects/binaries remain local ignored data; raw ClusterResults untouched.

## Replay and next work

Check contention, announce NEW folders and run sequentially:
```bash
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_geometry.py /absolute/new/controls
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
nice -n 19 python3 integration_evidence/native_3d_core_v1/run_field_oracle.py /absolute/new/controls /absolute/new/oracle
```
Oracle reuses objects only after checking source AND object hashes against a PASS
receipt. Receipt pins include compiled house source units, not all transitive
headers; full build/configuration provenance is still needed for cluster packages.

Next integrate actual-query metric/shape/edge acceptance with cavity proposals.
Edge integration must resolve original sensor-cell breakpoints and thin geometric
constraints instead of accepting a coarse quadrature that can miss a fine region.
Add physical-reference-associated roundoff/extension, coupled planar surface edits
and stronger reconnection; then MPI ownership/dependencies/atomic conflicts,
weighted M<=N workers, migration/return/transfer and full phase/memory controls.
Full SU2/CGNS output lifecycle, refreshed2D regression and the complete original
correctness/performance envelope remain pending. This checkpoint does not shrink
or complete Goal1.
