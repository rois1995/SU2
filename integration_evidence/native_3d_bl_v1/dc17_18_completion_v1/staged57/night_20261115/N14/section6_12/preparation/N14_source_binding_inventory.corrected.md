# N14 source-binding inventory (corrected; read-only)

Preparation only. This inventory records the current frozen source and the N14 condition. It is not the connected source check or acceptance. No production/test/policy/pinned helper was changed; no build/test/run was started and the live B2 G1 job was not touched.

## Correction record

The prior inventory mixed the contents of commit `5488d1f1b5` with the current source snapshot and incorrectly described the commit’s law-weighting/source-counter additions as current. Cause: I read commit contents while mapping restoration paths, then copied those symbols into the “current snapshot” paragraph without an independent current-source check. Correction: ran literal `rg` against the current files and verified current SHA/line locations. The prior file is preserved as `N14_source_binding_inventory.md.before`; it should not be used as evidence for current-state symbols. This corrected file separates current facts from commit facts.

## Current source snapshot

| File | SHA256 |
|---|---|
| `Common/src/adaptation/CNativeEngine3D.cpp` | `ffcb292b4639128c26495f7df44a4b63b9c61b0df62222547f4b4ad253ba9020` |
| `Common/src/adaptation/CNativeBoundary3D.cpp` | `359c502bd84de723f11ccf7d873fe89d4ec21016d3e9c6967c7250af49972dac` |
| `Common/include/adaptation/CNativeBoundary3D.hpp` | `6ff3a90de05efd059ace65b8e775e9792f0d31d0cd670a612a8469cb3b6638ae` |
| `Common/src/adaptation/native_engine3d/EngineTrial.cpp` | `717634a514b169f383cf3e88ecc6cbc10783dab3babb76c69ea7a3b249341fe5` |
| `Common/include/adaptation/CNativeWallDiagnostics3D.hpp` | `9cc513ea95f34fee5d6ad44c345e339bfdb5fecd5d86a49ed84d76185bd04fda` |
| `Common/include/adaptation/CNativeWallGeometryLaw3D.hpp` | `79eb840a398e91a45586e78750544b767c5f1eb682f829289acf228b907cf72d` |
| `Common/src/adaptation/CNativeWallGeometryLaw3D.cpp` | `6671e88472352d35cd8f95366fdeba3b22c87a2fe861955ddb0f9e0b986636e6` |

Literal current-state check:

- `CNativeEngine3D.cpp` contains no `WallFaceEdgeWeights`, `law_weighted_faces`, `ObserveSource`, or `NATIVE3D_WALL_SOURCE` symbol. `EstimateWallEdge` is used by `PrintWallLawCensus` at line 119; its declaration/definition are in the wall-law header/source (header line 31, source line 80).
- The current source still emits `PrintWallLawCensus(..., "entry", ...)` at line 1545. This differs from the no-entry-census behavior in 5488 and must be treated as a real current-vs-commit difference for any by-symbol restoration.
- The current `Selection::Add` geometry score is the unweighted `excess*excess*(x*x+y*y+z*z)` path (around lines 1070–1100); it has no `WallFaceWeights` call or law counter.
- The current diagnostics header has no `PatchSource`, `ObserveSource`, or `NATIVE3D_WALL_SOURCE` names. Do not report these as already present.

## Read orders

| Order input | SHA256 |
|---|---|
| Archived `NIGHT_WORK_ORDER.md` (6.10 and 6.12) | `0c446a618cda84b707a66d27551bdd8344db1166206811ee3fa84c0ec9432464` |
| Archived `section6_12_user_order.md` | `4a34e41ef943d9bfa4b5d75790b04a7263c61f046434538e032db44ef87720e6` |
| Archived `IMPLEMENTATION_8_3_8_4.md` (I; B1/B2 gates) | `40a2cf607454dc86d33824bc867ef65d64c739fe360e78bc485cc08af5d9a557` |

The archived 6.12 text says N14 was prepared, not released. The later user instruction explicitly releases N14; that later instruction governs. 6.12 replaces 6.10(b/c/d): use strict split-point lift, computed once before primary validation; at least 689 accepted splits at exit in both G3 runs remains the purpose gate. N15 belongs in the same build under 6.12’s release terms.

## Primary split point and fresh-node binding

Current absolute source locations, verified against the SHA above:

- `geometric_change` begins at 4351; `validate_private` begins at 4796; the primary `validate_private` call is 4858 with `choice.geometric ? MetricChange::GeometryRefine : construction_kind`.
- The primary non-coarsening branch constructs `point` at 4505 using a new ID and the endpoint midpoint; composition may replace its position with `splitPosition` at 4507.
- It calls `SplitEdge` at 4508, then `InsertEdgeCavity` at 4510 if the guarded result is false. If these fail, the central fallback sets `point.p=centralPosition` at 4515 and retries `SplitEdge` at 4516 and `InsertEdgeCavity` at 4518. The final successful call’s input can therefore be the initial midpoint, `splitPosition`, or `centralPosition`.
- Header declarations at `CNativeBoundary3D.hpp:271,278` take `Node point` by value. In `CNativeBoundary3D.cpp`, `SplitEdge` locally replaces it with `ProjectSplitPoint` (around 1573); `InsertEdgeCavity` locally projects it for a declared wall facet (around 1634). The caller’s `point.p` stays at the input coordinate; the corresponding output coordinate is in `fresh.cells` under `point.id`.
- Current caller-point replacement is line 5944, after primary validation and later reservation/clearance work. It replaces `point` with the fresh node and is too late for the required pre-validation lift. The needed identity lookup must be from `fresh` before primary validation.
- `guarded_geometry` wraps each raw operator bool with existing validity checks and diagnostics. The stored `approved` is that guarded bool, not a separately retained raw operator result; only the successful guarded primary candidate can be classified for the validation bypass.

## Validation and separation of paths

- Inside `validate_private`, the primary relevant guidance invocation is line 4802: unless `merit_first`, it calls `geometric_change(candidate, selection.geometry_repairs ? GeometryRepair : kind)`. The second call at 4831 is guarded by `valid && merit_first`; `merit_first` requires `!surface_target`, so it is not reached for N14’s surface-target case. There is another `geometric_change` call at 4412 in `validate_metric_vector`, outside this primary `validate_private` path.
- At the primary call on line 4858, geometry queue means `kind=GeometryRefine`. The effective kind at line 4810 substitutes only when `kind==Repair`, so remains `GeometryRefine`; `EngineTrial::MetricTrial` at `EngineTrial.cpp:35–57` applies its surface geometry-progress veto only to `GeometryRepair`. The earlier guidance call can see `GeometryRepair` if `selection.geometry_repairs` is true; this is separate from the effective metric kind.
- Under 6.12, the bypass condition is `surface_target && curved`, not coarsening, not repairing, `choice.geometric`, `fresh` contains `point.id`, and long-double squared Euclidean lift strictly exceeds `surface_tolerance²`. Equality is false. Compute the three differences and their sum of squares once after the successful primary split and before primary validation. No A1 law call, wall-face search, or travel/session/timer field. Required splits retain `geometric_progress=false`. Joint and repair checks remain unchanged.
- Joint paths are distinct: placement validation at 4975; strip validation at 5269; joint operator checks (`RefitWallSplit`, `ReconstructWallSplit`, `ReconstructSplitPatch`) occur downstream of `!approved` (joint selection at 4862; `validate_joint` at 5432–5435). Repair validation calls are at 5681, 5735, 5802, 5811. The secondary `SplitEdge`/`InsertEdgeCavity` calls at 5251–5252 operate on a joint `star`, not the primary patch. No lift condition belongs in these paths.

## 5488d1f1b5 restoration symbols (distinct from current facts)

Commit `5488d1f1b5` changed these paths for B2 and 6.9 print-only behavior:

- `Common/src/adaptation/CNativeEngine3D.cpp`: adds `WallFaceWeights` scoring in `Selection::Add`, law-face/source/split observation wiring and reporting call sites, and removes the entry `PrintWallLawCensus(..., "entry", ...)` call.
- `Common/include/adaptation/CNativeWallGeometryLaw3D.hpp` and `Common/src/adaptation/CNativeWallGeometryLaw3D.cpp`: `WallFaceWeights` and `WallFaceEdgeWeights`.
- `Common/include/adaptation/CNativeWallDiagnostics3D.hpp`: `PatchSource`, source/law counters, and `NATIVE3D_WALL_SOURCE`/law-selection output.
- Tests: `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp` and new `UnitTests/Common/adaptation/fixtures/native_wall_law_b2_tests.inc`.

These are commit contents, not current-source facts. The integration should restore by symbol on the current cut snapshot, not replay old line-number hunks. N4’s existing source check and the designated connected N14/N15 source check govern; this note creates no source-check result or acceptance.
