# Phase-A readiness and function-length draft

**State:** readiness draft only. G3 is live (session 41877); this is not a phase-A completion report. No source, pin, or helper was modified, and no build or launch was performed for this report. G3/G4/GQ/G5 remain unevaluated or in progress as applicable. Phase A is not complete until the candidate build has passed its required gates on the same frozen source, including G1, G2, G3, G4, GQ, and G5, and the final function-length accounting is reviewed.

## Function boundaries

Counts use inclusive physical source lines, including the function signature and closing brace (or the `= default;` declaration line for defaulted definitions). Comments and strings are masked before balancing body braces. Calls and deleted declarations are excluded. The N5 `reports/function_lengths_scan_recipe.txt` supplied the comment/string masking and brace-matching approach; its output-writing main was not run. This inventory is a fresh read of current source, not reuse of its old report.

### Current `native_engine3d` private files

There are 23 definitions in `Common/src/adaptation/native_engine3d/*.{hpp,cpp}` under this scope: 19 explicitly defaulted definitions, three `ContextValue` inline definitions, and `EngineContext`'s constructor. The defaulted definitions are shown separately. The deleted copy/move declarations are excluded. The deduction guide is not a function definition.

Longest definitions:

| Lines | Definition | Inclusive source lines |
|---:|---|---:|
| 14 | `EngineContext::EngineContext` | `EngineContext.cpp:4–17` |
| 5 | `ContextValue<T>::ContextValue` | `ContextValue.hpp:9–13` |
| 1 | `ContextValue<T>::~ContextValue` | `ContextValue.hpp:14–14` |
| 1 | `ContextValue<T>::Get` | `ContextValue.hpp:15–15` |
| 1 | `EngineControlPrefix::EngineControlPrefix = default` | `EngineContext.hpp:28–28` |
| 1 | `EngineControl::EngineControl = default` | `EngineContext.hpp:86–86` |
| 1 | `EngineCensus::EngineCensus = default` | `EngineCensus.cpp:4–4` |
| 1 | `EngineCensus::~EngineCensus = default` | `EngineCensus.cpp:5–5` |
| 1 | `EngineCommit::EngineCommit = default` | `EngineCommit.cpp:4–4` |
| 1 | `EngineCommit::~EngineCommit = default` | `EngineCommit.cpp:5–5` |

All 17 out-of-line defaulted definitions, each one inclusive line (the four Census and Commit rows also appear in the top-ten table above):

| File | Definition | Line |
|---|---|---:|
| `EngineCensus.cpp` | `EngineCensus::EngineCensus = default` | 4 |
| `EngineCensus.cpp` | `EngineCensus::~EngineCensus = default` | 5 |
| `EngineCommit.cpp` | `EngineCommit::EngineCommit = default` | 4 |
| `EngineCommit.cpp` | `EngineCommit::~EngineCommit = default` | 5 |
| `EngineContext.cpp` | `EngineContext::~EngineContext = default` | 18 |
| `EngineExchange.cpp` | `EngineExchange::EngineExchange = default` | 4 |
| `EngineExchange.cpp` | `EngineExchange::~EngineExchange = default` | 5 |
| `EngineOwnership.cpp` | `EngineOwnership::EngineOwnership = default` | 4 |
| `EngineOwnership.cpp` | `EngineOwnership::~EngineOwnership = default` | 5 |
| `EngineQueues.cpp` | `EngineQueues::EngineQueues = default` | 4 |
| `EngineQueues.cpp` | `EngineQueues::~EngineQueues = default` | 5 |
| `EngineRound.cpp` | `RoundState::RoundState = default` | 4 |
| `EngineRound.cpp` | `RoundState::~RoundState = default` | 5 |
| `EngineSetup.cpp` | `EngineSetup::EngineSetup = default` | 4 |
| `EngineSetup.cpp` | `EngineSetup::~EngineSetup = default` | 5 |
| `EngineSlot.cpp` | `SlotState::SlotState = default` | 4 |
| `EngineSlot.cpp` | `SlotState::~SlotState = default` | 5 |

All 23 are at or below 80 lines. The existing read-only formatting receipt also records `clang-format-18 --dry-run --Werror` exit 0 for `EngineRound.hpp/.cpp` and `EngineSlot.hpp/.cpp`, with `no_source_modified=true` (`A4/format_dry_run.json`).

### `Adapt` and current `CNativeEngine3D.cpp` function rankings

`Adapt` is currently 5,230 inclusive lines: `Common/src/adaptation/CNativeEngine3D.cpp:1447–6676`.

The first scan ranked namespace-level candidates but omitted qualified `MeshOptimizer::` member definitions. That omission is corrected below. Both scans mask comments/strings and balance braces from the recognized definition signatures; calls are excluded. The qualified scan found 15 member definitions. The namespace-only scan excludes qualified member names. Because this is a source scanner rather than compiler AST, the ranked list is limited to these identified forms; scope limitations are recorded below.

**Namespace-only top ten** (inclusive lines):

| Lines | Definition | Source lines |
|---:|---|---:|
| 5,230 | `Adapt` | `1447–6676` |
| 119 | `Add` | `994–1112` |
| 49 | `OptimizedRepairPatch` | `1333–1381` |
| 46 | `RefineOptimize` | `7962–8007` |
| 44 | `RepairBatch` | `673–716` |
| 39 | `FilterCollapse` | `812–850` |
| 36 | `PrintWallLawCensus` | `100–135` |
| 35 | `AccountQualityChurn` | `365–399` |
| 34 | `CollapseConstraintViolation` | `6678–6711` |
| 34 | `RestoreRepairState` | `1237–1270` |

**Qualified `MeshOptimizer` method top ten** (inclusive lines):

| Lines | Definition | Source lines |
|---:|---|---:|
| 223 | `MeshOptimizer::SmoothBoundary` | `7173–7395` |
| 139 | `MeshOptimizer::Smooth` | `7396–7534` |
| 100 | `MeshOptimizer::CoupledBoundary` | `7770–7869` |
| 54 | `MeshOptimizer::EdgeRemoval` | `7083–7136` |
| 51 | `MeshOptimizer::Run` | `7870–7920` |
| 49 | `MeshOptimizer::BoundaryFlip` | `7538–7586` |
| 49 | `MeshOptimizer::ShellBoundary` | `7670–7718` |
| 31 | `MeshOptimizer::ConeReplace` | `7617–7647` |
| 26 | `MeshOptimizer::AccountNested` | `7744–7769` |
| 25 | `MeshOptimizer::TryCell` | `7719–7743` |

**Combined ten longest definitions**, including all three definitions tied at49 lines:

| Lines | Definition | Source lines |
|---:|---|---:|
| 5,230 | `Adapt` | `1447–6676` |
| 223 | `MeshOptimizer::SmoothBoundary` | `7173–7395` |
| 139 | `MeshOptimizer::Smooth` | `7396–7534` |
| 119 | `Add` | `994–1112` |
| 100 | `MeshOptimizer::CoupledBoundary` | `7770–7869` |
| 54 | `MeshOptimizer::EdgeRemoval` | `7083–7136` |
| 51 | `MeshOptimizer::Run` | `7870–7920` |
| 49 | `MeshOptimizer::BoundaryFlip` | `7538–7586` |
| 49 | `MeshOptimizer::ShellBoundary` | `7670–7718` |
| 49 | `OptimizedRepairPatch` | `1333–1381` |

The combined ranking contains ten definitions, including the three-way49-line tie. The qualified-method self-check was anchored against the source definitions of `SmoothBoundary` (7173–7395), `Smooth` (7396–7534), and `CoupledBoundary` (7770–7869), and `Adapt` (1447–6676). The scan is mechanical, not compiler-AST complete; unusual declarators and nested local constructs may not match the candidate patterns. No source formatting was applied.

## Preserved source-check findings

The frozen whole-item findings in `N4/section6_3_after_N11/source_check.md` remain the governing list; this report does not open another source-review loop. The preserved behavior findings are: acquisition can throw outside its existing catch region; `import_source` ignores its `requests` argument; receipt sort ties retain input-order dependence; callback presence is not guaranteed symmetric across ranks; retained prepared geometry can change later partition weights; and private collective accounting reads World counters but does not observe direct `Allreduce` calls. These agree with the cut findings in `Handbook/ADAPTATION_ADAPT_CUT_20261115.md` (the handbook lists the first five). They are existing behavior to preserve, not new fixes.

**Written length exception:** `EngineLegacyWall` remains the explicit C exception above 80 lines. No other new exception is authorized; the three growth copies remain separate. New functions still follow the 80-line bound and new lambdas the 15-line bound.

## Source identity for this snapshot

The report is based on these current file hashes:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `dd97c5e2cb75789b1c35934a6fc3851ba297303a25510df5deceaba65efaf66f`
- `N4/section6_3_after_N11/source_check.md`: `3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`
- `Handbook/ADAPTATION_ADAPT_CUT_20261115.md`: `1335243e28043797a524bee6ac982cd28d7923acb54b7d1855e861414ca35657`
- `N4/section6_3_after_N11/A4/format_dry_run.json`: `7d5ed36923c374a3db2b7592c32619ea38593c983f6e60e9cd366b51046e472e`

Current hashes of all files in `Common/src/adaptation/native_engine3d`:

```text
8080f325279156926b04ea05ac907d0e1904bfa92936bde55bddaaf2fff47b55  ContextValue.hpp
314fbe6483953704d96d0ad5582e9bbcd507d740c65295b8a2f49d676979a326  EngineCensus.cpp
bfe3181885c22e13bc1104c405e3adbf383bced1fff5861baed92a04ce02d739  EngineCensus.hpp
e08d08b388deb4e903ca63099f48cc4d23bf7127b5658ae90e30933405981505  EngineCommit.cpp
042453b4539912625e93b310994752cf7cf2c8029eb7f483ac8b5ca8f93801d7  EngineCommit.hpp
637c4e0a42147af18635c3398d101892035c5c34cdff5dccd75454b1917dbb58  EngineContext.cpp
847752c9e043b21e2c1968f21673b1b0abba8ae89af74caef48dc43a55ba2419  EngineContext.hpp
764dd7425ccbcc9378b9f8cf467bb0190d4e0c92801f2ffb8cf894aa3d547121  EngineExchange.cpp
5218f814e469ce682b03549983198aedf1586f7e4b23449531088a6bcdf464c8  EngineExchange.hpp
064e1b42d50255852194f4eb7bfa54e8036d5f4c1f2bc21d09744c8b8ea266d5  EngineOwnership.cpp
474b4ef2d06f079202bd9ece9e01165ed48dab93c4cc23b72ef64a570e8a9e81  EngineOwnership.hpp
05e8495ab0edffde28bf5b18838deac614f67087508d41df61e01a36afe441fa  EngineQueues.cpp
d4fac4045aef775c2bec4df2fb9ed74d8b97bd26503670b4649a7e8f2de2df37  EngineQueues.hpp
39992645f4ae08729b3cbdaa8638b374017b13431798bce1335b3a4ffc41088f  EngineRound.cpp
be6bcc5290edb3543c5a9a8315161e4e1d701820b5885b18b3d31aed2b94d373  EngineRound.hpp
f96ac45c0e0a7108fce51936244290df55da49d87210d431d9a3b0997b99d6b4  EngineSetup.cpp
46bbb95747d1f39dd5dc4faa0553577b3b686fd0cebd198431c6b00b934d2d1b  EngineSetup.hpp
16976122d3cf35c75545881e897ca2721cd55f733d9acf911367942efbb380b4  EngineSlot.cpp
f8665887133dbf2568fb0c1f8564598b75d5530b2adaa01d8859a5b19b6912d4  EngineSlot.hpp
```

These production inputs were not changed here.
