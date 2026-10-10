# Native 3D original-sensor edge integration — 2026-10-10

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core, after published immutable-field checkpoint215f5d91f0.
Goal1 remains ACTIVE. Native runtime still rejects3D; this is an integration and
residual-audit primitive, not a completed 3D adaptation event.

## Why source crossings matter

An evolving edge can cross a narrow original sensor feature between ordinary
quadrature points. In the micron slab control, endpoint/midpoint samples all
return sensor length1, while source-resolved integration gives2.33333233337.
The two-binary64-spacing strip gives2.48029736617 with the corrected stronger
fixture. Both exceed the initial1.8 edge limit. A sampled acceptance rule would
miss them. These are manufactured original P1 SENSOR fields, not geometric BL
profiles or CFD solutions.

CNativePredicates3D.cpp now traces a finite nonzero binary64 segment through at
most256 supplied original tetrahedra. Exact rational barycentric halfspaces set
crossing parameters; exact cross products sort and compare them. Coverage must
span[0,1]. Gaps, overlapping interiors and nonconformal coincident interval
supports reject. Conformal face/edge ties use canonical input order and shared
original node IDs. The caller must supply a conforming embedded source; segment
checks do not certify arbitrary global embedding. FrozenField validates shared
coordinate/sensor consistency and canonicalizes donor keys before this call.

Width is rounded only AFTER exact subtraction of rational cuts. Endpoint weights
are evaluated in EACH selected original donor at its exact rational crossings.
The coincident-cut control has two cuts which both round to0.5, yet retains the
positive interval width. It must not sample a neighbouring fine tensor at a
rounded crossing and apply that tensor over a coarse interval.

## Length and field semantics

FrozenField::SensorEdgeLength reuses the house ADT for the segment bounding box.
On each original interval, the chord quadratic form is affine in its parameter.
Its square root has an analytic integral; the implementation uses a rearrangement
that avoids subtracting nearly equal cubes. Endpoint sensor interpolation uses
that donor's original P1 data. No metric gradation, clipping, target replacement
or evolving-cell interpolation is introduced.

When the broad phase returns exactly one donor, magnitude-controlled endpoint
Barycentric containment plus tetrahedron convexity proves there is no internal
source crossing. The implementation skips rational crossing construction there.
Multiple candidates retain the exact path, including shared-face ties and gap/
overlap rejection. The final oracle records60 direct intervals and180 endpoint
containment attempts;60 of71 covered oracle edges use the direct path.

MetricNorm has a magnitude filter with bounded dyadic exact fallback for positive
quadratic cancellation. Cross coefficients are widened before doubling, avoiding
binary64 intermediate overflow. Unlike orientation signs, unresolved length
magnitudes cannot use a sign-preserving underflow sentinel. Such magnitudes reject.
Zero-length SensorEdgeLength requests reject; finite SPD MetricNorm on a zero chord
returns0. Tensor admission remains exact Sylvester, with no tensor repair.

IMPORTANT: SensorEdgeLength deliberately excludes geometric composition, even
when FrozenField has a composition callback. It is a sensor-residual audit, NEVER
sufficient for composed-target acceptance. Point Query still evaluates composition
at every actual point, including cache hits; no BL tensors are interpolated or
cached. The3D geometric BL provider, its geometry-derived integration cuts and
bounded composed quadrature remain pending. Thin sensor tests do not establish
thin geometric BL resolution or combined gradation.2D/MMG/noise/fade unchanged.

## Evidence and inspectable grids

Final kernel folder: integration_evidence/native_3d_core_v1/edge_controls_v4.
PASS28 Catch cases/2373 assertions plus290 independent Fraction orientation signs.
Controls cover exact clipping/coverage, conformal ties, reversal, invalid source,
integer/donor limits, cancellation-safe norm, huge components, underflow handling,
affine integral, finer strips without spreading, coincident cuts, actual-query
composition exclusion, failed-work counts and exclusive timer closure.

Final independent folder: integration_evidence/native_3d_core_v1/edge_oracle_v2.
PASS104 edges in37 source groups:71 covered,33 rejected,147 intervals.
Exact binary64 Fraction clipping/coverage/endpoint weights and150-digit Decimal
antiderivatives supply independent references. Maxrelative lengtherror4.1162e-17,
maxabsolute weighterror4.8867e-20, maxrelative widtherror7.1193e-20; tolerance1e-12.
Rotation/aspect1..1e10, scales1e-50..1e50 and translations0/1000 are included.
The three24-tet slab sources are independently checked for exact positive volume,
closed oriented skin, no positive-volume pair intersections and full slab volume.
Inspectable grids: micron_source.su2, two_spacings_source.su2,
coincident_cuts_source.su2. Matching *_sensor.json and edge_defects.json retain
source data and residual locations. These are SOURCE grids, not accepted/rejected
adapted proposals. The enormous-domain coincident-cut grid is an arithmetic stress
control, not a physical simulation grid.

Final point-regression folder: integration_evidence/native_3d_core_v1/field_oracle_v5.
All210 queries pass the original Fraction weights/directional tensor references;
150inside60outside. Output is byte-identical to field_oracle_v3. Source/compiled
object receipts are checked before linking reused objects. Current kernel and
edge-oracle runners pin sources before/after and reject changes during execution.
No full Meson, CFD, MPI, AD build or CGNS runtime campaign was performed.

## Performance scope and remaining work

FieldStats separates edge request/failure/candidate/piece/direct/containment counts
from point queries/cache/composition. Optional detail timing defaults OFF. Edge
parent covers search, trace-or-direct containment and sensor integral as exclusive
children. Parent residual is remaining local work. Do not add parents to children.
MetricNorm separately counts filtered/exact norms; exact source determinants
remain visible in KernelStats. Donors/crossings are bounded256, with fixed-size
integer scratch. Full memory-ceiling admission and MPI import limits remain pending.

In the final104-edge observation, edge parent0.012249153s, search0.000019558s,
trace/containment0.012084238s, integral0.000052631s, parent residual0.000092726s.
This is one control observation under foreign Python/FSI contention, not a paired
performance campaign or speedup claim. Exact multi-donor tracing is still costly;
source crossings/containment are the evident local cost centre. Before bulk use,
measure fixed work, amortize immutable source data and safely filter exact work
without missing intervals. Instrumentation overhead, representative source patches,
full event phases, transfer/partitioning, MPI imbalance/scaling and affordability
remain unestablished.

Next: geometry-declared composition cuts, bounded actual-target integration and
metric/shape/progress gates on private replacements; associated surface roundoff,
coupled adaptable planar triangles; then complete distributed dependencies,
atomic MPI commits, cost-weighted M<=N workers/return/transfer and the original
Goal1 validation/performance contract. Follow DESIGN_REFERENCES.md and the pinned
local Loseille/Tsolakis/Galbraith reading scopes; no additional paper required yet.

## Preserved attempts and replay

edge_controls_v1 FAIL is a fixture expectation mistake: two-spacing peak1e30 gives
length1.07401, matching the analytic reference but below the asserted1.1. The final
fixture strengthens peak to4e32 and requires length>1.8; no numerical gate relaxed.
Its original test source matches the receipt hash and is retained.

edge_controls_v2 reports a real27case/2370assertion PASS, but its old runner captured
source hashes only after an in-flight predicate edit. SOURCE_PROVENANCE_NOTE.md
marks these objects unfit for final-source reuse; original logs/receipt and
pre-edit predicate source remain. Final runs use pre/post source pinning.
edge_controls_v3 PASS28cases2371assertions precedes the singleton fast path; its
changed field header/source/tests and mesh tests are retained with matching hashes.
edge_oracle_v1 retains its pre-counter driver. Final controls_v4/oracle_v2 supersede
intermediate observations for current-source claims; failures/evidence are intact.

Replay sequentially after a host-contention check and announcing NEW folders:
```bash
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
 nice -n 19 python3 integration_evidence/native_3d_core_v1/run_geometry.py /absolute/new/controls
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
 nice -n 19 python3 integration_evidence/native_3d_core_v1/run_edge_oracle.py /absolute/new/controls /absolute/new/edge_oracle
PYTHONDONTWRITEBYTECODE=1 OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
 nice -n 19 python3 integration_evidence/native_3d_core_v1/run_field_oracle.py /absolute/new/controls /absolute/new/point_oracle
```
Binaries/objects stay ignored locally; only compact evidence is published.
Raw ClusterResults and the external repair repository are untouched.
