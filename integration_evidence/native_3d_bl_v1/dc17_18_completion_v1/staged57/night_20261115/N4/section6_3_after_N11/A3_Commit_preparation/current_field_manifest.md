# A3 Commit current-field manifest (Exchange candidate; validation pending)

Read-only mapping of PART1 §2.6 Commit against the current Exchange candidate. This Exchange candidate is present in source and validation pending; no Commit implementation is applied. PART1 line numbers below are its original snapshot, not a quota. Source hashes at inspection:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `07fd72461e9f4ac2ce44f27d0597e784058d712cfb0782820a0cfb9fcc2e35b9`
- `Common/src/adaptation/native_engine3d/EngineContext.hpp`: `3f70c65a3f911970b7d022e1c2eeb4c76c4373b6b5254870bb37ba1ad67fbd1f`
- `Common/src/adaptation/native_engine3d/EngineSetup.hpp`: `46bbb95747d1f39dd5dc4faa0553577b3b686fd0cebd198431c6b00b934d2d1b`
- `Common/src/adaptation/native_engine3d/EngineQueues.hpp`: `d4fac4045aef775c2bec4df2fb9ed74d8b97bd26503670b4649a7e8f2de2df37`
- `Common/src/adaptation/native_engine3d/EngineQueues.cpp`: `05e8495ab0edffde28bf5b18838deac614f67087508d41df61e01a36afe441fa`
- `Common/src/adaptation/native_engine3d/EngineExchange.hpp`: `5218f814e469ce682b03549983198aedf1586f7e4b23449531088a6bcdf464c8`
- `Common/src/adaptation/native_engine3d/EngineExchange.cpp`: `764dd7425ccbcc9378b9f8cf467bb0190d4e0c92801f2ffb8cf894aa3d547121`
- `Common/include/adaptation/CNativeLaneCensus3D.hpp`: `d26379e1aee755659c4bedfe12445d70dc23ceae51e130b7e05c09f2e1530b08`
- `Common/include/adaptation/CNativeMetricTransport3D.hpp`: `cb9a45c0b8a990e1e58514fe7335ba970aecc4a6240401bf0120a308c62b3494`
- `Common/include/adaptation/CNativeDistributed3D.hpp`: `2510df14023dc3c7ab26e77516b46a084823ade348e8b43a345243a7f31e1e3a`
- `inputs/CUT_INVENTORY_PART1.md`: `034275df32e2f4b7f4ef9bb2aef0bc16453f6d0249971b90fc42ba6a33b663cd`

## Current fields and ownership

Rows are listed in current declaration order for these cross-cutting Commit fields; unrelated Census, Queues, Exchange, and Control declarations interleave them in Adapt.

| Name | Exact current type / initializer | Current source | Role / canonical relation |
|---|---|---:|---|
| `private_parameters` | `auto& private_parameters=setup.private_parameters.emplace();` (`std::shared_ptr<BoundaryParameterState>`) | 1479 | Canonical boundary parameter state owned by `EngineSetup`; Directory publication edits flow through this object and checkpoint captures/swaps its contents. Not a Commit-owned copy. |
| `result` | `ContextValue result_lifetime(control.result); auto& result=...Get();` (`EngineResult`, default-emplaced) | 1568–1569 | Canonical Control result. Commit writes its split/collapse records in place; checkpoint may resize those vectors on rollback. |
| `stats` | `auto& stats=result.stats;` (`EngineStats&`) | 1570 | Alias into the canonical result, shared by all phases. Commit writes report/count fields through it; no separate Commit stats object. |
| `working` | `Owned working;` | 1571 | The one mutable working mesh. Initialized from `accepted`, mutated by publish paths, swapped during ownership/checkpoint changes, and swapped into `result.cells` at finish. Current Exchange `source` is constructed with this object by reference; it is not duplicate Commit storage. |
| `batch_bytes` | `ContextValue batch_bytes_lifetime(control.batch_bytes, 0); auto& batch_bytes=...Get();` (`size_t`) | 1683–1684 | Control-owned live accounting for retained batch storage; included in `bytes()`, grows in candidate/reservation paths and resets at round completion. |
| `batch_changed` | `std::vector<Id> batch_changed;` | 1697 | Adapt-scope changed-cell ID buffer; appended from batched commit results, capacity counted by `bytes()`, cleared after round processing. |
| `batch_touched` | `std::set<Id> batch_touched;` | 1698 | Adapt-scope set populated from directory-reported changed vertices during batched repair commits; current consumers are `bytes()` and additional admission sizing. |
| `batch_supports` | `std::set<std::array<Id,4>> batch_supports;` | 1699 | Adapt-scope set populated from directory-reported changed repair supports during batched repair commits; current consumers are `bytes()` and additional admission sizing. |
| `maximum_node`, `maximum_cell` | `Id maximum_node=0,maximum_cell=0;` | 2496 | Paired Adapt-scope ID high-water marks; seeded from working/donor IDs, globally reduced, then used to reserve new point/cell IDs in commit and construction paths. |

`result`/`stats` and `batch_bytes` remain Control aliases/storage. `private_parameters`, `lane_counters`, and `wall_counters` remain Setup-owned. `source` and `directory` remain Exchange state: source borrows the canonical `working` alias; neither is duplicated in Commit. The proposed Commit holder is not applied.

## Accepted observation state

`EngineSetup` owns optional `lane_counters`; Adapt activates it with `options.timings` at current line 1457. The private-loop `lane_census::Trial` observes trial slots; `lane_counters.Approved` and `lane_counters.Commit` are called at 5988/6015 and 6201/6472. `wall_counters.Accepted()` is also called on accepted publication (6199, 6468). These are print-only instrumentation, not part of `batch_changed`, the mesh, identity allocation, or the accept/reject decision. They print at stage exit; keep them with Setup/observability rather than Commit payloads.

`bytes()` counts `batch_bytes`, `working`, `batch_changed`, `batch_touched`, `batch_supports`, `result.splits/collapses`, original/initial/eligible data, selection, queues, and checkpoint. Admission adds `source.ResidentBytes()`, `directory->ResidentBytes()` and caller scratch. Preserve that accounting boundary and the existing high-water/commit outputs.
