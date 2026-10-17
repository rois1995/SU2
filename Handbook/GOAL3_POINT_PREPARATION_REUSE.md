# Point preparation reuse — rejected strategies and retained audit

Full Goal3 ACTIVE/incomplete. Base83fb0678232d191a90e52c3271a7f4cead0775bc,
branch codex/native-3d-bl. Both production cache variants are WITHDRAWN. Current
query schedule is restored, with only a field_builds counter and reusable test-only
frozen-M6 query audit. Restored serial/MPI4 bridge, same512-cell MPI4 query/serial-
reference comparison and independently audited N4/M2 RANS/BDF lifecycle PASS.

Evidence roots under integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1:
point_preparation_reuse_v1 (AABB), point_preparation_exact_v1 (exact-point pilot),
point_preparation_baseline_v1 (restored implementation and aggregate comparison).

## What was tried

The shared WorkingWeights/ReaderSlices path retained bounded original-donor
records, FrozenField index/sensor cache, and certificates for complete imported
point footprints. Missing footprints were imported; collective donor-cap resets
replaced support and discarded certificates. Headroom allowed batch recovery up
to the original8/16 item sizes. No composed tensor was cached, target changed,
constraint weakened, or cap raised. Inactive ranks and collective failures were
covered. The source/policy remained immutable within each call.

AABB support retention passed serial/MPI4 dense200-donor-star/reset/recovery/empty-
rank/failure controls and existing bridge transfers. The SAME512 canonical original
M6 tetrahedra on1/2/4 ranks gave exact integer weights and exact per-query coordinate,
sensor and composed tensors versus fresh imports. Cross-rank weights matched.
The affected curved-extension runtime and initialized RANS/SA BL construction/removal
N4/M2 also passed, including independent geometry/full-target/height/flow+BDF/
transported sensor/SA/restart/SU2+CGNS audits. These are scoped correctness results,
not performance or general wing-construction qualification.

| Variant/ranks | Retained preparation | Fresh reference | Imports | Builds | Index reuses |
|---|---:|---:|---:|---:|---:|
| AABB /1 | .1561s | .0855s | 189 vs125 | 122 vs124 | 0 |
| AABB /2 | .1892s | .1123s | 189 vs125 | 122 vs124 | 0 |
| AABB /4 | .1829s | .0803s | 146 vs95 | 123 vs123 | 0 |
| Exact point /1 | .5566s | .1538s | 189 vs125 | 122 vs124 | 0 |

Times are maximum-rank contended local preparation times for this selected512-cell
workload, not complete M6 adaptation/scaling. Four-rank imports are max per rank;
builds are sums. The reference reproduces the historical fresh-import8/halve query
schedule but omits production admission bookkeeping. CPU/load/cache state varied;
do not interpret ratios as stable full-event speedups or slowdowns. The added
imports/cap churn and lack of useful index reuse are direct work-count evidence.

The first variant needs51–65 support resets. Exact closed point footprints use
the existing ImportSegments(point,point), preserving all touching ties/extensions.
They still need64 resets and add predicate work without improving index reuse.
Thus AABB false positives alone do NOT explain the failed cache in this workload.
Retaining whole incident donor stars is the wrong reuse unit here. Both variants
are withdrawn; no broader MPI/lifecycle campaign was spent on the second after
its serial difficult-case pilot failed the performance test.

## What remains in production and tests

Original preparation/import/query/acceptance schedule restored. A field_builds
counter records actual FrozenField constructions in bridge profiles; no speculative
cache or additional pressure reduction remains. The NativeFrozenVolume3D test has
an optional SU2_NATIVE_PREPARATION_LIMIT path that imports the archived owner-UPPER6
backend source, chooses a bounded canonical-cell prefix independent of rank count,
and compares all actual query coordinates/sensor/target tensors and integer weights
against fresh imports. It never advances CFD or constructs a mesh in that mode.
Archived hosting-rank RAW9 integration fields are untouched; the backend wire
representation is explicitly a different saved authority.

All original geometry/height/positivity/q/J/full composed-edge/transfer gates remain.
The actual initial-coarsest M6 mass790200 +/-27958 against100000 and its earlier
25.25x initializer gain remain the separately qualified83fb checkpoint; this work
adds NO accepted preparation-cache speedup and NO accepted M6 grid.

## Provenance, failures and resources

Both executed variants have gzip source patches against83fb plus per-file digests
verified against their native execution pins. Replay inputs, tensors-match flags,
weights CSVs, independent accepted small meshes/states, logs and resource receipts
remain. There is no need to retain large historical ELF copies. The failed
World.size() test-helper compilation and invalid one-rank/four-worker fixture are
preserved; production validation/configuration rules were not relaxed. Passing
cases were resumed rather than rerun after the fixture-only correction.

One owned heavy job/build-j1/nice19/thread1/MPI<=4; host38–40GiB available. Foreign
CFD and ClusterResults untouched. GNUtime process peaks are separate from native
payload models and from aggregate job RAM. Restored current-binary evidence is in point_preparation_baseline_v1; build6m20s/
peak process2.39GiB, connected wrapper52.77s/529.59MiB, GNUtime0 swaps. These are
process peaks, not aggregate job RAM. All owned handles terminal. Do not reuse
experimental executables as the published implementation.

## Next design implication

The saved512 cells use454 distinct nodal coordinates for2048 nodal requests:
at least1594 of2560 total point queries repeat, and at most966 distinct queries
fit the2048 sample capacity. Offline audit/hash receipts reproduce this without
another solve; no speedup is implied.

Retain canonical original interpolation samples/stencils or scalar integrals with
an explicit immutable-source/field lifetime, rather than retaining every incident
donor star solely for point lookup. The existing regional sensor-only point cache
and retained complexity stencils are house implementations to inspect and extend.
Any source/geometry change invalidates the relevant reuse; geometry-specific BL
composition must still run at every actual query. Do not add an overlapping cache,
a larger donor cap, or a third local reset heuristic to rescue this design.

Separately, first-row initialization in the saved M6 construction uses many
collective transactions and repeated full edge integrals. Scoped composed-edge
reuse and genuinely independent transaction batching remain the requested broader
package, NOT implemented here. Original-field reassessment/correction epochs,
global error calibration/allocation, accepted difficult curved/M6 construction and
complete lifecycle affordability all remain full Goal3 requirements.
