# N6 Selection draft

The draft starts from the accepted post-N16 rollback source at `3276cbb862`. `selection.patch` applies the census and queue split. `extract_selection.py` reconstructs the patch and `mechanical_checks.json` from that Git snapshot using the repository clang-format configuration.

## Production scope

- `CNativeEngine3D.cpp` includes the new declaration and fragments at the former `Selection` definition. `WallLinkLocal` reads the census vertex index through the same const handle.
- `EngineSelection.hpp` owns the census and queue state under the existing `Selection` lifetime. `EngineSelectionCensus.cpp` carries the versioned indexes, measurements, and aggregate changes. `EngineSelectionQueues.cpp` carries the construction, geometry, and repair queues.
- Eight existing caller files qualify the same members through their new owner. The files are `EngineAdapt.cpp`, `EngineCensus.cpp`, `EngineCommit.cpp`, `EngineControl.cpp`, `EngineExchange.cpp`, `EngineOwnership.cpp`, `EngineQueues.cpp`, and `EngineReports.cpp`. Their code tokens match the base after those owner qualifiers are removed.
- The supplied independent MeshOptimizer draft and this patch were combined in an isolated directory. `git apply --check --whitespace=error` and application both passed. Bytes from the MeshOptimizer include to the end of the translation unit matched the optimizer draft. That is the only common tracked file. Reading the existing production files and checking patch applicability left them untouched.

## State and ordering

The work keeps the existing owner references and the same two live object sites in Adapt and ownership migration. The census owns cell and edge caches, vertex incidence, aggregate debt and completeness. The queue part owns frontier directions, geometry scores and priorities, rejection history, repair features, severity and service age. State changes that must touch both parts keep the existing `Selection` call site and order.

`SwapState` still swaps the explicit member list at the same point in migration. `Reset` still preserves blocked directions and links, repair epoch, and policies outside its original reset. `Add` keeps the outer D9 scope and the order of validation, debt measurement, vertex indexing, edge and queue updates, aggregate accounting, and repair seeding. It delegates contiguous debt and edge blocks to named functions. Its geometry face helper returns to the same caller loop for the two original skip cases. `RepairBatch` delegates the existing complete-star walk to a named method and keeps fair and severe selection order. `Bytes` keeps the same operands, container types, node accounting, and capacity changes. Six nested types match the original tokens, including the fields and serializer order of `CellState` and `CachedRow`.

## Readability and review

Clang-format dry-run with warnings as errors passed on all three new fragments. The lexical audit found 58 out-of-line functions and 11 lambdas. The longest function body is 60 lines and the longest lambda body is seven lines. Fifty of 52 original out-of-line function bodies match the base tokens after removing owner qualifiers and restoring one local counter name. The other two are the bounded `Add` and `RepairBatch` extractions described above. No readability exception was needed.

| Function | Body lines |
|---|---:|
| `RefreshQuality` | 60 |
| `ChangePhase` | 51 |
| `MeasureGeometryFace` | 50 |
| `FilterCollapse` | 48 |
| `RepairBatch` | 46 |
| `RestoreRepairState` | 44 |
| `Batch` | 43 |
| `CollectCellDebt` | 42 |
| `Diagnose` | 37 |
| `Restore` | 37 |

The draft generator first resolved the repository root one directory too high. It now finds the repository clang-format file in its ancestors and regenerated successfully. The integrated syntax check and numerical gates belong to the ordered N6 campaign. This draft makes no runtime qualification claim.
