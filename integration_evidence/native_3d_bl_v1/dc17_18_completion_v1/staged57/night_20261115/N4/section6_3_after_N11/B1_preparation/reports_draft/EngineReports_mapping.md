# EngineReports unapplied caller and body map

Scope: proposal-only mapping for the current build545 `CNativeEngine3D.cpp`. This file and the adjacent `.proposed` files do not change or claim acceptance of the live production source. The source remains frozen while G5 runs.

## Bindings and include point

- `EngineReports` is stateless and has no instance or payload. The private header and implementation are included immediately after `native_engine3d/EngineContext.cpp` in the existing `CNativeEngine3D.cpp` anonymous namespace, before `Adapt` and after the context types are complete.
- All current state is borrowed directly through the existing active optionals: Control (`result`, rejection vector/sequence, fixed points/cells/flags), Census (`selection`), Exchange (`source`, `directory`), and Ownership (`working`). Method bodies bind canonical old-name aliases (`world`, `working`, `selection`, `source`, `directory`, `options`, `stats`, `composition`, and fixed-metric fields) directly through those optionals. The only callable passed is the existing local `bytes` closure by reference to `EmitFixedMetricDrift`; no new callback, storage, counter, allocation, decision, or API is added.
- The current source defines `FixedMetricSamples=16` at Adapt scope before fixed-point state activation. Move that one existing declaration into the private EngineReports header before its exact array signatures, removing its old local declaration when applying the proposal. This keeps one value and one identifier; it is not a new configurable limit. The existing `diagnostic_rejection_limit=64` remains local to Adapt because it also controls the original reserve guard; each RecordRejection caller passes `size_t(diagnostic_rejection_limit)`.
- Every proposed function is bounded below 80 formatted lines. `InitFixedMetricDrift`, `ReadFixedMetricDrift`, and `CompareFixedMetricDrift` are private templated members used only by `EmitFixedMetricDrift` to keep its extracted bodies within that limit. No additional lambda is introduced.

## Original bodies and direct caller replacement

| Existing local lambda/body | Existing definition / body | Existing uses | Proposed destination / direct replacement |
|---|---|---|---|
| `record_rejection` | `CNativeEngine3D.cpp` 1851–1863 | 4043; 6222; 6554 | `EngineReports::RecordRejection(context, size_t(diagnostic_rejection_limit), feature, old, metric, reason)` at each call. Body retains the capacity early return, ring index/sequence, row reset, cavity/metric copies, UTF-8 truncation, and truncation flag in source order. |
| `consider_fixed_metric_row` | 1979–1994 | Called by `update_fixed_metric_cells` at 1998 and `reset_fixed_metric_cells` at 2002 | `EngineReports::ConsiderFixedMetricRow(context,row)` from the two Reports methods. Retains the available check, point/cell loop order, bounds test, `SampleContainedCurrentMetric` call, and swallowed `std::exception` boundary. |
| `update_fixed_metric_cells` | 1995–1999 | 6301; 6593 | `EngineReports::UpdateFixedMetricCells(context, removed, added)` at both sites. Keeps removed-cell invalidation before added-cell consideration. |
| `reset_fixed_metric_cells` | 2000–2003 | 3450 | `EngineReports::ResetFixedMetricCells(context)`. Keeps the availability return, fill with `UINT64_MAX`, then traversal of the same `working` map. |
| `emit_fixed_metric_drift` initialization body | 2004–2034 | Part of 2214 | `InitializeFixedMetricDrift(context, available, bytes)` inside the original condition. Keeps initialization flag write, failure/local/outgoing construction and lifetimes, sample categorization, exact quota, exchange, sample sort, and fixed-point/cell assignment. |
| `emit_fixed_metric_drift` read body | 2035–2061 | Part of 2214 | `ReadFixedMetricDrift(context, selected, have, available, bytes)` only under the original fixed-available condition. Keeps selected/have declarations after initialization and before the old read condition; keeps local/readings lifetimes, election/exchange, admission, and tie ordering. |
| `emit_fixed_metric_drift` comparison body | 2062–2074 | Part of 2214 | Initialize the four scalars at their original point, then call `CompareFixedMetricDrift(...)` only under the original `available` condition. Keeps all long-double weights, `hypot`, finite checks, and rank ownership logic. |
| `emit_fixed_metric_drift` timer and output | 2004, 2075–2078 | 2214 | `EngineReports::EmitFixedMetricDrift(context, stage, bytes)` at 2214, preserving timer start before initialization, diagnostic-seconds update after comparison, and the original output expression/field order at the end. |

No forwarding aliases/lambdas remain at the old sites. Direct call-site inventory after application should show: RecordRejection at 4043/6222/6554; UpdateFixedMetricCells at 6301/6593; ResetFixedMetricCells at 3450; EmitFixedMetricDrift at 2214. The helper-internal calls to ConsiderFixedMetricRow are the only remaining callers for it.

## Original lexical boundary plan for Emit

1. `began` and `available` are created first. The `!fixed_metric_initialized && std::string(stage)=="quality_entry"` expression stays in the top member unchanged and calls the initialization member.
2. `selected{}` and `have{}` remain declared after initialization as `std::array<FixedMetricReading, FixedMetricSamples>` and `std::array<bool, FixedMetricSamples>`; helpers accept those exact array types and retain `fixed_metric_cells[i]` indexing without `.at()` checks. The existing `if(fixed_metric_available)` calls the read member with both arrays and `available` by reference.
3. `supported`, `owned`, `maximum`, and `local_maximum` are declared after the read condition in the same order. Only the original `if(available)` loop body moves to the comparison member.
4. The existing elapsed calculation updates `result.stats.quality.diagnostic_seconds` after comparison. The `options.timings` condition and full `NATIVE3D_QUALITY_DRIFT` output remain last, with precision, fields, values, and order unchanged.

The original exception scopes remain inside the same moved blocks. The initialization and read `CLocalFailure`, local vectors, outgoing vectors, exchange results, and catches are not hoisted or combined. The original combined `before[6], after[6]` declaration remains one declaration statement. Existing world election/exchange, `bytes()` invocation order, memory ceiling, and source/directory resident-byte accounting are unchanged.

## Source bodies preserved by the draft

- RecordRejection copies from the original lambda at 1851–1863.
- Consider/Update/Reset copy from 1979–2003.
- Fixed-metric initialization/read/compare/output bodies copy from 2004–2078, split only at the caller-local block boundaries above.
- The only binding translation is local capture to `EngineContext` optional dereference. The member names resolve to the same canonical source objects; no duplicate owner or independent state is introduced.

## Unapplied status and checks

The proposal is intentionally not compiled, run, applied, or treated as a result gate. Before any later application, perform a token/identifier reverse-map against the complete pre-B1 `Adapt` body, confirm every old call has exactly one new direct call, and verify include order after EngineContext type completion. The phase-A G5 run is live and remains untouched.
