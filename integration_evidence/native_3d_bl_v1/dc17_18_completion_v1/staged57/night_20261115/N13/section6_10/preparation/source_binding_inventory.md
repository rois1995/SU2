# N13 / M25 read-only source and setup inventory

**Scope/status.** Read-only preparation while the G5 run is live. N10 is withdrawn and N14 is prepared but not released per 6.10. This is not a test edit, source change, implementation, or release decision. The user order requires N13 at the next cut boundary; the pending AGENTS prefix is to be written after current validation and restart are terminal. No build or test was run here.

## Frozen input and saved492 D9 case

The copied 6.10 order is `inputs/NIGHT_WORK_ORDER.md`, SHA256 `3ba8d840f63bd555100f4271aa12ee03881f609b215b5eaaf45e40aae2f3493f`. The exact copied M25 list is `inputs/m25_edges.csv`, SHA256 `5d3a73801727bb3443b7557d6afe56434a6ddc3dc89577aa6825b27892d7accf`; it has 2,408 rows and the specified nine columns: `edge_a,edge_b,law_pick,n11_first_verdict,failing_faces,limiting_faces,limiting_failing_faces,need,estimate`.

The existing hidden case that produced the saved492 geometry CSVs is:

- Test: `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp:2817`, hidden Catch2 case `D9 geometry need and exact wall proof measurements`, filter `[.][NativeD9Geometry]`.
- Historical descriptor: `N8/A2_A3_N12/section6_8/descriptors/n5_d9_a1_geometry_saved492_534.json`, SHA256 `eb156489cf682818b6667998109a401cc66f09a6cde666d337e1910229a87bf3`.
- Historical receipt: `N8/A2_A3_N12/section6_8/unit_measurement_receipts/n5_d9_a1_geometry_saved492_534.json`, SHA256 `f25ea5eec768135fc302fe606e35af3363ed72f9967cee7ff91e88095c703fe1`.
- The produced saved492 attempt is `N8/A2_A3_N12/section6_8/cases/n5_d9_a1_geometry_saved492_534_20261116T170031_554683`; it contains `d9_geometry_edges.csv`, `d9_geometry_faces.csv`, and `d9_geometry_projections.csv`. Their hashes are pinned in the receipt.
- Runner: `N8/A2_A3_N12/section6_8/run_section6_8_focused.py`, SHA256 `bd73cd33dd28250363a4011c8d5289f396c1f81c4c3a6fc3eea73a2ce0592f37`.

Descriptor inputs and hashes:

| Input | SHA256 | Role in the existing fixture |
|---|---|---|
| `staged57/prepared_reference441.native_ref` | `9cdbc63cb6853f2c8b052bb0549f7b561299bfe82cd21fd6b792d2600dadf7e9` | Decoded reference authority (`SU2_D9_REFERENCE`). |
| `staged57/cases/curved492_saved_production_surface_serial_20261113T163756_3724702/stage_resumed_surface_rank0_parameters.bin` | `52ddcac68784232c81f9556149ed2b31efc48844839ea89755ef85ec47c94dbc` | Saved492 parameter-state payload (`SU2_D9_PARAMETERS`). |
| `staged57/saved_surface492_checkpoint.json` | `a7a0acfea2e71d88f3253a553b877788afe9476bbec4f6ab70ccb362aacdc990` | Checkpoint provenance pinned by the descriptor. |
| `staged57/cases/curved492_saved_production_surface_serial_20261113T163756_3724702/stage_resumed_surface_rank0_owned.bin` | `89886f8bf3dc736b9f04df73b7c588becf489519ebd909a3dbb0dd8faee92539` | Saved owned-state provenance pinned by the descriptor. |
| `staged57/cases/curved492_ramp5_metric_stage_20261113T161313_3653005/target_rank0.bin` | `ef809cdfc51aeaebc0411f5c30fc84548b94975789c96373385d32a1574b787f` | Target provenance pinned by the descriptor. |

The test fixes `tau = 1e-4` at line 2820 and marker filter `2`; the loaded parameter state supplies `GeometryTolerance()` and the summary records whether it equals M1 tau. The hidden case creates `loaded.base = reference.original->WithParameterState(loaded.parameters)` from the above two direct environment inputs and checks its held-memory model against a 12 GiB ceiling. It also pins the owned/checkpoint/target inputs as fixture provenance; those are not direct environment variables in this test.

D9 measurement constants at test lines 2307–2312 are: 540-second D9 diagnostic deadline, `D9WorkLimit = 1<<24`, `D9WorkspaceLimit = 8<<30`, `D9HeldMemoryLimit = 12<<30`, and `D9IntegerLimit = 4096`. `GeometryAtlasBudget()` at line 840 has aggregate defaults `(maximum_work=1<<24, maximum_bytes=1<<28, maximum_integer_bits=4096)`; D9 face/projection measurements explicitly override `maximum_bytes` to 8 GiB and use the 12 GiB held-memory cap. D9 projection uses a fresh budget for each selected edge, with `caller_bytes = context.held + D9OverlayBytes(overlay)` and its time callback. These values distinguish the helper's default budget from the D9 query budget. The 6.10 M25 run limit is 60 minutes; it is separate from that historical 540-second D9 deadline.

## Production prediction path and exact evaluation order

Current definitions and source hashes:

- `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`, SHA256 `4f5607bb8dea58a0a5ceb271e629e64def8207723b4bbb94829bee7da7a51261`; `PlanarReference::SurfaceDeviation` begins at 1358 and `PredictGeometrySplit` at 1366.
- `Common/include/adaptation/CNativeBoundary3D.hpp`, SHA256 `6ff3a90de05efd059ace65b8e775e9792f0d31d0cd670a612a8469cb3b6638ae`; prediction result and public declaration at 193–202.
- Production caller: `Common/src/adaptation/CNativeEngine3D.cpp:451`, `MeshOptimizer::PrioritizeGeometrySplits`; it passes the measured edge endpoints, current `geometry`, and `tolerance`. `PredictGeometrySplit` has no other production caller in the current source.

The result contains `long double before`, `long double after`, `size_t evaluated_faces`, and `bool complete`; elapsed seconds/work are not result fields. A test can record a steady-clock interval and the associated `AtlasBudget.work` delta around the call.

The call's decision/evaluation sequence, which the independent self-check must reproduce literally:

1. Reject invalid ordered edge/tolerance; return incomplete when state/entity geometry, matching endpoint identities/positions/mobility, trace support, two incident faces, common entity, or admitted atlas reservation is absent. The same edge is canonicalized before querying.
2. Check sticky budget state, open `AtlasStorage` using `caller_bytes + ParameterOverlayBytes + sizeof(Patch) + 6*sizeof(array<Point,3>) + 8*512 + 4*128`, then create `geometry.PrivateParameters(Patch{}, budget)`.
3. Build the two-facet incident set; form the arithmetic chord midpoint coordinate by coordinate as `double(((long double)a + b)/2)`; bind it using `BindNewPoint(Node{UINT64_MAX,point}, facets, &edge)`. A bound midpoint identical to either endpoint returns the checked incomplete result.
4. For each `state->IncidentFaces(edge)` pointer in returned order, collect the parent triangle in `face->v[0..2]` order, finding the endpoint indices `a` and `b`. Missing points or endpoint indices return incomplete. Score the parent first into `before`.
5. Copy parent to `left` and `right`; replace `left[a]` and `right[b]` by the bound midpoint. For each child, replace the matching face ID with `midpoint.id` and rotate the coordinate array so the minimum face ID is first. Score left, then right, into `after`; the `||` early exit skips right if left fails.
6. The score performs `ContainsFace(facet,triangle)` first. If true, calls `SurfaceDeviation(facet,triangle,SurfaceQueryNeed::Magnitude(),true)`. Nonfinite bounds fail. It computes `long double excess = std::max(0.L, (long double)bound - tolerance)`, accumulates `sum += excess*excess`, increments `evaluated_faces`, then checks `std::isfinite(sum)`. The bound is double; sums and excess products are long double. `complete=true` is assigned only after both parents and all four children score successfully.
7. `bad_alloc` and `length_error` propagate. `invalid_argument` propagates only for sticky work/geometry failures; otherwise the function returns the checked incomplete result. The final return passes through `checked()`.

`BoundaryParameterState::IncidentFaces` (current source `CNativeBoundaryParameters3D.cpp:562`) sorts the edge and traverses `edge_faces.lower_bound({edge,Face{}})` order; the index uses canonical `FaceOrder`, so retain that returned order. Sum bit equality requires retaining the same parent/child order, casts, expression grouping, midpoint construction, early exits, and strict floating-point evaluation.

## Test-side API reachability

All requested geometry operations are publicly reachable from test code; no production visibility change is needed:

- `PlanarReference::WithAtlasReservation(AtlasBudget&)` and `PrivateParameters(const Patch&, AtlasBudget*)` are public in `CNativeBoundary3D.hpp`. `WithAtlasReservation` creates a scoped view carrying the budget; `PredictGeometrySplit` reads it through `AtlasReservation()`.
- `BoundaryParameterState::IncidentFaces`, `Find(Id)`, `Coverage()`, `Entity()`, `GeometryTolerance()`, and `EntityGeometry()` are public in `CNativeBoundaryParameters3D.hpp`.
- `PlanarReference::BindNewPoint`, `ContainsFace`, and `SurfaceDeviation` are public in `CNativeBoundary3D.hpp`. `SurfaceQueryNeed::Magnitude()` is public in `CNativeAffineReference.hpp`.
- `EstimateWallEdge` and `EstimateSaysNeed` are public free functions in `CNativeWallGeometryLaw3D.hpp` (header SHA256 `79eb840a398e91a45586e78750544b767c5f1eb682f829289acf228b907cf72d`). `EstimateWallEdge` takes the `BoundaryParameterState`, endpoint `Node`s, and anchor facet; its source computes the directed chord as endpoint differences, gathers endpoint support facets, and requires supported/non-unsupported estimates at both endpoints.
- Bound point is the returned `Node` from `BindNewPoint`. Chord-midpoint-to-bound-point distance and edge length are not returned by an API; the test can compute them from coordinates. For the five-edge-per-face-anchor measurements, endpoint nodes come from `state->Find(id)->point.node`; call `EstimateWallEdge` on the edge and the four other unique edges in the union of the two parent faces, once for each parent facet as anchor (ten anchor-edge estimates per input edge).

No b-column is inaccessible from test code. `PredictGeometrySplit` already exposes its aggregate sums/count/completeness; the per-face detail is recreated with those public calls. The test-only sum must use strict arithmetic matching `CNativeBoundaryParameters3D.cpp`: the source is part of `Common/src/meson.build`'s `SU2AccurateSum` library with `-fno-fast-math`, `-fno-unsafe-math-optimizations`, and `-ffp-contract=off`. The normal unit-test target passes global `su2_cpp_args` (`UnitTests/meson.build:112–119`), and root `meson.build:101` adds `-ffast-math` when supported. Thus an inline test reconstruction compiled under the normal test flags can differ in the last bits even when the operations look identical. Any strict arithmetic accommodation must be test-only; do not change production or the Common/Meson source configuration. No such accommodation was implemented here.

## Relevant source hashes

- `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp`: `51488109e3f3f91d3ed2bdda6b6ba174676578bf06969c51a0ba4830c49e0eba`
- `UnitTests/meson.build`: `2f769eddb6d826cef7619a9d21d419883d87b97884e596e14494307ac30c3268`
- root `meson.build`: `5123109a117a61d89ae5d64be5a396361a6e9a1aaddfd00b57eaf12494345ae9`
- `Common/src/adaptation/CNativeWallGeometryLaw3D.cpp`: `6671e88472352d35cd8f95366fdeba3b22c87a2fe861955ddb0f9e0b986636e6`
- copied `inputs/sha256_manifest.json`: `761162c8068e6ca1349c2df31dc2b7b87e5a3071ca72c7b851876f72c7d6e2c6`
