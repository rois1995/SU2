# F3 readability audit — 2026-11-15

Refreshed against frozen build503 after the old-cavity metric measurement scope was added. Every reported source hash matches `build503_source_pins.json`; numerical checks remain outside this report.

Instrumentation audit only; this report does not establish numerical correctness or timing coverage. Source hashes and exact tool output are in `readability_report.json`. No production source or runner was changed by this audit.

Counts are physical lines from opening through closing body brace, inclusive, with comments, blank lines and nested scopes included. A balanced lexical scan masks comments and strings; lambdas/control blocks are excluded. The ten longest named-function signatures and closing braces were manually checked. No C++ AST parser was installed.

## Ten longest functions in changed production files

This is a file-level inventory: an unchanged function can appear because another function in the same file received instrumentation. Existing large functions are not presented as new F3 functions.

| Function | File and body lines | Body lines | F3 scope |
|---|---|---:|---|
|`Adapt`|`Common/src/adaptation/CNativeEngine3D.cpp`:1178–6037|4860|Legacy body receives narrow instrumentation|
|`CNativeRemesher3D::Remesh`|`Common/src/adaptation/CNativeRemesher3D.cpp`:202–1076|875|Legacy body receives narrow instrumentation|
|`ImportGeometry`|`Common/src/adaptation/CNativeImport3D.cpp`:1015–1364|350|Legacy body receives narrow instrumentation|
|`JointAffineCoverage`|`Common/src/adaptation/CNativeImport3D.cpp`:355–676|322|Legacy body untouched|
|`MappedFacetReference::ClosestCurved`|`Common/src/adaptation/CNativeAffineReference.cpp`:1594–1902|309|Legacy body untouched|
|`BoundaryParameterState::Prepare`|`Common/src/adaptation/CNativeBoundaryParameters3D.cpp`:623–913|291|Legacy body untouched|
|`CSinglezoneDriver::RunTimeFixedPointLoop`|`SU2_CFD/src/drivers/CSinglezoneDriver.cpp`:1484–1767|284|Legacy body untouched|
|`MappedFacetReference::QuerySurfaceUnionAtResolution`|`Common/src/adaptation/CNativeAffineReference.cpp`:2152–2414|263|Legacy body untouched|
|`CSinglezoneDriver::RunTimeAdaptationLoop`|`SU2_CFD/src/drivers/CSinglezoneDriver.cpp`:1055–1288|234|Legacy body untouched|
|`CSinglezoneDriver::PredictWindowMetric`|`SU2_CFD/src/drivers/CSinglezoneDriver.cpp`:744–950|207|Legacy body untouched|

Exception reason: the handoff explicitly allows a small instrumentation repair to leave old surrounding code unchanged. F3 adds timer scopes and lifecycle accounting; it does not move or refactor the existing algorithm. Cutting the legacy `Adapt` and its large lambdas belongs to the reviewer’s design 7.2/implementation step 8.2 after user acceptance. Reformatting or extracting these bodies during F3 would broaden the numerical change and violate that sequence. The existing function-size and lambda debt is recorded, not waived or claimed fixed.

## Ten longest new timer-header functions

| Function | Body lines | Count |
|---|---|---:|
|`OperationName`|115–159|45|
|`Session::Snapshot`|215–238|24|
|`PhaseName`|90–112|23|
|`Session::Report`|282–302|21|
|`Session::AddOpenFrames`|245–257|13|
|`Scope::Scope`|344–355|12|
|`Session::Session`|170–178|9|
|`Scope::~Scope`|322–329|8|
|`Session::Finalize`|273–279|7|
|`Session::Add`|265–270|6|

All 18 new timer-header function bodies are at most 80 lines; the largest is 45. The header has no lambdas. Every new header file/struct/class/function has a preceding purpose or lifetime comment; deleted special-member declarations also explain the stable-frame invariant. The test fixture has file/test-purpose comments. This is a source readability check, not a proof that every pre/postcondition is exhaustively documented.

## clang-format dry-run

Tool: `Ubuntu clang-format version 18.1.3 (1ubuntu1)`. New files use `clang-format --dry-run --Werror <file>`. Full changed legacy files use the same strict check plus `--ferror-limit=5` to bound diagnostic output; that limits emitted diagnostics, not formatting scope. The HEAD version of each legacy file was checked through stdin with `--assume-filename=<file>` to distinguish pre-existing failures. No file was reformatted.

| File | Working-tree exit | HEAD baseline exit | Result |
|---|---:|---:|---|
|`Common/include/adaptation/CNativeDistributed2D.hpp`|0|0|PASS|
|`Common/src/adaptation/CNativeAffineReference.cpp`|1|1|FAIL; HEAD also fails|
|`Common/src/adaptation/CNativeBoundaryParameters3D.cpp`|1|1|FAIL; HEAD also fails|
|`Common/src/adaptation/CNativeDonor3D.cpp`|1|1|FAIL; HEAD also fails|
|`Common/src/adaptation/CNativeEngine3D.cpp`|1|1|FAIL; HEAD also fails|
|`Common/src/adaptation/CNativeImport3D.cpp`|1|1|FAIL; HEAD also fails|
|`Common/src/adaptation/CNativeRemesher3D.cpp`|1|1|FAIL; HEAD also fails|
|`SU2_CFD/include/adaptation/CNativeBLTarget3D.hpp`|1|1|FAIL; HEAD also fails|
|`SU2_CFD/include/adaptation/CNativeTargetCheckpoint3D.hpp`|1|1|FAIL; HEAD also fails|
|`SU2_CFD/src/drivers/CSinglezoneDriver.cpp`|1|1|FAIL; HEAD also fails|
|`UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp`|1|1|FAIL; HEAD also fails|
|`Common/include/adaptation/CNativeTimerTree3D.hpp`|0|new|PASS|
|`UnitTests/Common/adaptation/fixtures/native_timer_tree3d_tests.inc`|0|new|PASS|
|`Common/src/adaptation/CNativeMetric3D.cpp`|1|1|FAIL; HEAD also fails|

Both new files pass. Full legacy files do not all pass; their existing format debt remains explicit. Exact stdout/stderr, bounded to five format errors per legacy invocation, is retained in the JSON. No blanket legacy format compliance is claimed.

## Added old-cavity closure

`CNativeMetric3D.cpp` is included in this audit. Its immediately invoked closure in `ValidateMetricChange` spans body lines 642–647: 6 physical lines including braces, below the 15-line lambda limit. It captures the existing old cells, target, stats and measured edges by reference, opens `OldCavityMeasurement`, and returns the existing conditional measurement. The old expression matches HEAD after whitespace removal; the surrounding validity/error handling is unchanged.

The in-body comment reads: “Include both old shape and edge work; nested service timers retain their own exclusive attribution.” It explains why the scope covers both branches and why nested timers are retained. This is a small measurement closure, not a new numerical operator or an extracted large legacy function. Header functions remain unchanged and at most 45 lines. The new metric file does not displace any entry in the ten-longest production-file table.

Only this report and its supporting JSON were created. Builds, execution checks, resource gates and the 95% coverage acceptance remain with the primary agent.
