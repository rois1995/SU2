# A3 Queues field manifest (current Census candidate, validation pending)

Read-only mapping of the current A3 Census candidate against PART1 `2.3 Queues` and the current `Selection` implementation. Source hashes at inspection:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `d5aaa4e384ac24b20bbe2bedd564e2bec92b059d7d057ac26a0cabfb60065edd`
- `Common/src/adaptation/native_engine3d/EngineContext.hpp`: `fbc8c052decc41f01a6cf9110b8a6de3a81b2fdce3214f062fc3e60451867842`
- `Common/src/adaptation/native_engine3d/EngineCensus.hpp`: `bfe3181885c22e13bc1104c405e3adbf383bced1fff5861baed92a04ce02d739`

PART1 line numbers refer to its old 7,444-line snapshot. Current source anchors below are live candidate locations, descriptive rather than a quota. `EngineContext` has `EngineControl control` then `EngineCensus census`; there is no `EngineQueues` field/substruct yet. Census payload `census.selection` is an optional holding the existing shared `Selection` object. The queue state inside that object is listed in original member order below; it is not a second object.

## PART1 queue fields outside `Selection`

| Name | Exact current type/initializer | Current source | Uses / shared relation |
|---|---|---:|---|
| `inactive_blocked_edges` | `std::map<Id,std::set<Id>> inactive_blocked_edges;` | 1590 | Inactive-direction blocked edges; exchanged with `selection.blocked_edges` by `length_direction` at 3230 and cleared after ownership migration at 3426. Included separately in `bytes()` at 1699. |
| `inactive_blocked_links` | `size_t inactive_blocked_links=0;` | 1590 | Counter paired with `inactive_blocked_edges`; swapped with `selection.blocked_links` at 3230, reset with its map, included in `bytes()` at 1700 using `sizeof(Id)+4*sizeof(void*)`. |
| `length_blocks_dirty` | `std::array<bool,2> length_blocks_dirty{{false,false}};` | 1591 | Per-direction blocked-state invalidation; read/reset by `length_direction`, set at phase/loop transitions (e.g. 3484, 3803), reset after ownership migration (3426). |

## Queue-bearing members inside shared `Selection`

`Selection` is declared at `CNativeEngine3D.cpp:415`; constructor line 420–421 takes mode/timing values by value and borrows the canonical geometry, `EngineStats`, and wall-height policy by const reference. Its queue-bearing data members are declared in this order:

| Declaration order | Exact type and initializer | Current declaration | Queue role / relevant behavior |
|---:|---|---:|---|
| 1 | `bool split_lanes=false;` | 418 | Enables surface/interior split lanes; current N12 lane census separately observes trial slots. |
| 2 | `Lane lane=Lane::Surface;` | 419 | Current active lane. |
| 3 | `bool coarsening, track_repairs, repair_event, prune_collapses=false, track_splits=false;` | 422 | Queue candidate modes and filters. First three are initialized from constructor arguments; flags remain mutable. |
| 4 | `const bool quality_only; const bool surface_planning;` | 423–424 | Constructor mode flags; `surface_planning` affects orientation/queueing. |
| 5 | `bool size_repairs=false,geometry_repairs=false;` | 425 | Repair queue selection gates. |
| 6 | `const PlanarReference& geometry; EngineStats& stats; bool timings; double tolerance; const WallHeights& heights;` | 426–430 | Canonical geometry/stats/heights aliases plus scalar timing/tolerance configuration. |
| 7 | `std::set<std::tuple<long double,Id,Edge>> geometry_queue;` | 431 | Geometry-priority queue. `Next` and geometry selection read its first tuple; `PrioritizeGeometrySplits` rebuilds it. |
| 8 | `std::map<Edge,long double> geometry_split_priorities;` | 432 | Geometry-priority memo used by `GeometryPriority`; not the same as edge measurements. |
| 9 | `bool geometry_priority_mode=false;` | 433 | Switch for memo-backed priority vs raw score. |
| 10 | `std::set<std::pair<long double,Edge>> surface_queue;` | 463 | Surface lane queue. `Queue` chooses this vs ordinary queue when split lanes and a physical edge apply. |
| 11 | `std::set<Edge> construction_frontier;` | 466 | Generation-frozen frontier; publication does not extend it. It filters insertion and is refreshed/serviced at generation transitions. |
| 12 | `struct History {uint64_t age=0,round=0;unsigned disposition=0;}; std::map<Edge,History> edge_history;` | 467–468 | Retry/fairness history for overlong edges; consumed by census/history refresh and queue ordering/reporting. |
| 13 | `std::set<std::pair<long double,Edge>> queue;` | 535 | Main length queue (split or collapse ordering according to `Priority`). |
| 14 | `std::map<Id,std::set<Id>> vertex_cells, blocked_edges;` | 536 | Vertex incidence plus directional blocked-edge state; `blocked_edges` is swapped with the external inactive-direction map. |
| 15 | `size_t vertex_links=0, blocked_links=0;` | 537 | Link-count companions for the maps. |
| 16 | `std::map<Id,std::vector<RepairFeature>> cell_repairs;` | 538 | Per-cell feature support for repair queue. |
| 17 | `struct RepairDemand { size_t users=0;uint64_t epoch=0;bool blocked=false; }; uint64_t repair_epoch=0;` | 539–540 | Shared feature demand and generation epoch. |
| 18 | `std::map<RepairFeature,RepairDemand> repair_users;` | 541 | Aggregated feature users and blocked state. |
| 19 | `std::set<std::pair<uint64_t,RepairFeature>> repair_queue;` | 542 | Age/epoch ordered repair candidates. |
| 20 | `std::map<Id,long double> repair_cell_severity; std::map<RepairFeature,long double> repair_severity;` | 543–544 | Per-cell and aggregate repair weights. |
| 21 | `std::set<std::pair<long double,RepairFeature>> repair_severity_queue;` | 545 | Severity ordered repair view, maintained with demand/severity changes. |
| 22 | `std::set<RepairFeature> repair_zero;` | 546 | Zero-progress repair support and phase readiness state. |

`Selection` also contains Census rows (`cells`, `edges`, debt and progress) interleaved with these queue fields. The object is intentionally shared by Census, Queues, Control and Ownership in PART1; no split is reflected in current code. Queue internal structures that are used for snapshots are included in its `SwapState`, `Reset`, `Bytes`, restore/serialization, and generation/phase methods (e.g. `SwapState`/`Reset` around 1271–1293 in the current source). Do not interpret table ordering as a proposed reordering.

## Current candidate binding and accepted observation state

- `ContextValue selection_lifetime(context.census.selection, coarsening, repairing, geometry, stats, options.timings, options.surface_tolerance, options.wall_heights, bidirectional, bool(surface_target))` is at 1571–1574. This activates the same old constructor and exposes the same local name `selection`; following lines still set `split_lanes` and `track_splits` at the old points.
- Canonical bindings: `geometry` is the resolved geometry reference; `stats` aliases `result.stats`; `options.wall_heights` is the existing by-value options member. The `Selection` constructor stores the first three by reference, not by copying them. `context.census.selection` is only payload storage; it does not replace local `selection` aliases used by lambdas.
- `EngineCensus` currently stores optional Census payloads in order. Queue-only external locals above remain ordinary Adapt locals; no queue substate has been implemented in the current header.
- Accepted print-only observation state remains separate: EngineSetup owns `d9_profile`, `wall_counters`, and `lane_counters`; wall/lane counters print at current Adapt stage exit (2469–2472). Lane slot metadata is a local fixed array in the trial loop (line 4240 range); wall/lane counters and slot metadata must not be mistaken for queue payloads. The geometry-priority queue/memo is real selection state; the wall/lane counters are diagnostics.
