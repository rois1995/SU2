# BL suitability evidence route — 2026-11-10

Read-only audit during frozen full RANS/BL314. No source, runtime, input or pinned
helper changes; no new job. This records remaining completion obligations, not a
competing implementation or evidence that an actual generated BL mesh fails.
Euler293 accepted workflow/checkpoints remain the regression baseline.

| Property | Existing authority/output | Scope and remaining proof |
|---|---|---|
| First height and foot | Common/src/adaptation/CNativeBoundary3D.cpp::CheckFirstHeight; FirstHeightReport | Every requested current wall face needs a unique child; altitude/h0 tolerance, normal-foot barycentrics and positive curved-reference clearance are enforced. Fields include wall_faces, maximum_relative_error, minimum_barycentric, missing_cells, footprint_failures, minimum_reference_clearance_ratio and worst facet/cell/apex. Altitude is measured from the current triangle; positive reference clearance is not an independent curved-normal requested-height proof. |
| Alignment | FirstHeightReport.maximum_tilt | Tangential apex displacement from triangle centroid divided by h0. Diagnostic relative to the triangle normal, not a complete curved-normal alignment census or universal angle gate. |
| Progression, coverage and outer transition | BLGrowth12.inl::NextLayer, CountLayers, JsonGrowth, JsonLayers | Growth/registry fields include layers_per_column, stops/stop_reasons/refusals, joint retries, without_column, target_failed, deviations and A2_pass. Pipeline exports JsonGrowth but no JsonLayers call was found. Construction registry evidence does not prove current-mesh coverage after general adaptation. |
| Tangential/flow conformity | Composed target provenance, production edge integration, final q/J/size census | Preserve combined target and exact metric integration. Metric acceptance does not separately establish tangential surface coverage or correct target formation. |
| Features, junctions and competing walls | ComputeMetric D1 identity/counts; first-row audit, joint-wall/growth witnesses; retained marker/component authority | Necessary identities and refusal witnesses exist. No complete final per-feature/per-junction suitability export was found. Do not demand perfect orthogonality at justified transitions. |
| After general operators/migration | Current-owned-cell height and dependency census in StageProgress | height_measured_cells, height_unmeasured_cells, height_bad_cells, height_unresolved_cells, height_complete, cache_consistent and complete. cache_consistent is an internal status, not an independent cached/recomputed comparison. Progression and curved alignment are not covered by these flags. |

CNativeBLPipeline3D writes _bl_report.json / partial phase receipts with
first_row_audit, provisional validity, registry counts and performance. Its
_final.mesh is provisional construction, not final distributed acceptance.
.native_rows preserves continuation identities, not final layer coverage.
FIRST_ROW currently constructs with ONE native worker; MPI6 parent CFD and
surrounding workflow do not establish distributed FIRST_ROW generation.

After an actual accepted native mesh exists, retain BOTH event meshes, .native_ref,
.native_rows, final engine acceptance/census and construction reports. Existing
TestCases/adaptation/capability/capcheck.py::check_bl can independently corroborate
face heights, junctions and face coverage; it does not certify curved authority.
Compare its measurements with native height reports. Curved-normal placement,
alignment, progression/coverage/transition and post-migration cached/recomputed
agreement remain explicit obligations. Select any necessary correction from the
actual complete-mesh outcome and representative contexts; do not infer failure
or change construction policy from this static audit alone.

All strict existing geometry, positivity, q/J, metric-size, body-separation and
requested-height gates remain mandatory. Additional suitability evidence does not
replace conservative admissible flow/SA transfer, finite continued CFD, TWO
accepted RANS500k events and an actual isolated adapted disk restart.

## Source pins inspected

- `Common/src/adaptation/CNativeBoundary3D.cpp` SHA256 `851fb17254d6b2b6b63d8e5651dd00a7e4353f8de56c65c47e32d40f4065e14c`
- `Common/include/adaptation/CNativeEngine3D.hpp` SHA256 `b86caabb32b2043b30e147e46642e3a23d73710930435d04a05eb2add7c29928`
- `SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp` SHA256 `f36b7cca0248b14e9b854581a408a90979db6ab416f7d3b188c725bd864ba2a1`
- `SU2_CFD/include/adaptation/native_bl3d/BLGrowth12.inl` SHA256 `d7e4abee687c3d2867718d86a9ccceb9666ea6bd6d2452dc6f337e7e6f6fd400`
- `TestCases/adaptation/capability/capcheck.py` SHA256 `38ee0fe9fc60d7b29ab796303d8c5cec2770ad1beb06e2e5b76d5c687d495f92`
