# Exact current round and slot declarations

Read-only declaration inventory. Base is the current A3 Exchange candidate, validation pending; Commit draft remains unapplied. This is not an implementation proposal. It enumerates PART2 §2.1 Round and §2.2 Slot data locals in current lexical order, with source spelling and initializer. Lambdas are excluded from these tables. Evaluation-level (§2.3) locals remain evaluation locals and are not Round/Slot fields.

## Source fingerprint

- `Common/src/adaptation/CNativeEngine3D.cpp`: `07fd72461e9f4ac2ce44f27d0597e784058d712cfb0782820a0cfb9fcc2e35b9`
- `Common/include/adaptation/CNativeEngine3D.hpp`: `c661acd0f8866b7e2bcf0884bf944286409f5c2d92ff548c510da3aefe3e75d7`
- `Common/include/adaptation/CNativeLaneCensus3D.hpp`: `d26379e1aee755659c4bedfe12445d70dc23ceae51e130b7e05c09f2e1530b08`
- `inputs/CUT_INVENTORY_PART2.md`: `5b0b7d498e31f0637d0c23c950499c76c06cc66d19f33297e3ceb0bdea0d966a`

`CUT_INVENTORY_PART2.md` line anchors are historical; below are current source lines. A semicolon-separated declaration is copied as written. The `auto` spelling is retained where the source does not spell out the type.

## Round data, by lexical scope

| Scope / current lines | Source-exact declaration | Binding / initialization note |
|---|---|---|
| Loop initializer, 3624 | `for (size_t round=0;;) {` | `round` is a mutable loop value; initialized to zero. Round-body `continue` sites may bypass tail increment. |
| Round body, 3641–3646 | `const unsigned timed_lane=bidirectional&&phase==Phase::Construction&&!repairing&&!coarsening?stats.split_lane:0;` | Const value, conditional initialization. |
| Round body, 3642 | `double untimed_lane=0;` | Mutable scalar value, zero initialized. |
| Round body, 3645–3646 | `Choice choice;` | Class object declared without initializer (default-initialization). It is overwritten by `pick`/slot acquisition. |
| Round body, 3646 | `bool complete=false,any=false,bad_field=false,refill=false;` | Four mutable bool values, each explicitly initialized false. |
| Round body after round decisions, 3715 | `const bool clean_quality=!metric_cleanup && phase==Phase::Improvement &&`<br>`        !world.maximum(selection.quality_invalid!=0 || selection.unmeasured_shape!=0 || selection.cells.size()!=working.size());` | Const bool value initialized by a collective expression; remains in the round lexical body. |
| Round batch block, 3966–3970 | `const bool concurrent=(repairing || ((composition || curved || selection.split_lanes) && !coarsening) || (bidirectional && coarsening)) && options.candidate_batch>1 && world.size>1;` | Const bool value. |
| Same block, 3969–3970 | `const bool batched=repairing || (options.candidate_batch>1 &&`<br>`        (!(selection.split_lanes&&!coarsening) && !curved && !composition && (!coarsening || !bad_field)));` | Const bool value. |
| Same block, 3971 | `std::vector<Choice> choices;` | Class object declared without initializer (default-initialization); initially empty per vector semantics. |
| Same block, 3972 | `DependencyBatch dependency_batch;` | Class object without initializer (default-initialization); do not infer member initialization beyond its declaration/type. |
| Same block, 3973 | `ImportedBatch imported_batch;` | Class object without initializer (default-initialization); do not infer member initialization beyond its declaration/type. |
| Same block, 3974 | `ReservationBatch reserved;` | Class object without initializer (default-initialization); do not infer member initialization beyond its declaration/type. |
| Same block, 3975 | `size_t slots=1;` | Mutable scalar value, explicitly initialized to one; later assigned collective maximum. |
| Same block, 3976 | `const bool planned_coarsening=coarsening && !repairing && !curved && !composition && world.size>1;` | Const bool value. |
| Same block, 3979 | `size_t refused_candidates=0,refused_slots=0,pass_deferred_candidates=0;` | Three mutable scalar values, each explicitly zero initialized. |
| Same block, 3980 | `std::array<RepairReceipt,MaximumCandidateBatch> refused_receipts{};` | Fixed-size array, value-initialized with empty braces. `MaximumCandidateBatch` is 8 (`CNativeEngine3D.hpp:14`). |
| Same block, 3980 | `size_t refused_receipt_count=0;` | Mutable scalar value, explicitly zero initialized. |
| Round repair setup, 4233 | `std::vector<RepairProposal> repair_proposals;` | Class object without initializer; empty vector. `RepairProposal` is namespace-scope at `CNativeEngine3D.cpp:1418–1434`, not function-local. |
| Same scope, 4234 | `CLocalFailure private_failure;` | Class object without initializer (default-initialization). |
| Same scope, 4234 | `uint64_t private_work_used=0,private_work_reserved=0;` | Two mutable scalar values, explicitly zero initialized. |
| Same scope, 4235 | `bool private_deadline=false;` | Mutable bool value, explicitly false. |
| Per-round slot census setup, 4253 | `std::array<lane_census::Slot, MaximumCandidateBatch> lane_slots{};` | Eight Slot objects, value-initialized. This is accepted fixed-eight print-only metadata and lives through private batch commit. |
| Same scope, 4254 | `const auto private_started=repairing?Clock::now():Clock::time_point{};` | Const value with source-spelled `auto`; conditional value is a steady-clock time point. |
| Same scope, 4255 | `const auto private_collectives=world.collective_calls+world.election_collective_calls;` | Const value with source-spelled `auto`; snapshot of two counters, retained for later comparison. |

Round RAII objects and lambda objects are intentionally outside this data table. Their exact declarations and scopes are listed below.

## Slot data, by original lexical scope

### Slot-loop head and candidate setup

| Scope / current lines | Source-exact declaration | Binding / initialization note |
|---|---|---|
| Loop header, 4256 | `for (size_t slot=0;slot<(repairing?choices.size():slots);++slot) {` | Mutable index value initialized zero; bound is local choice count for repair, common `slots` otherwise. |
| Slot head, 4257 | `Dependencies dependencies;` | Class object without initializer (default-initialization); filled from acquisition path. |
| Slot head, 4258 | `Imported imported;` | Class object without initializer; filled from acquisition path. |
| Slot head, 4259 | `bool chosen=false,pass_deferred=false,collapse_corrected=false;Id collapse_apex=0;` | Three explicitly false bool values and one `Id` value initialized zero. |
| Slot head, 4260 | `std::string collapse_correction_refusal;` | Class object without initializer; default empty string. |
| Slot head, 4369 | `CLocalFailure failure;` | Class object without initializer; shared by evaluation and later vote paths. |
| Slot head, 4370 | `const bool coordinate_only=repairing && imported.coordinate_star.count==1 && imported.cells.size()>MaximumCavityCells;` | Const bool value. |
| Slot head, 4371–4376 | `const size_t private_scratch=transfer_memory::Add(transfer_memory::Bytes(imported.cells),`<br>`        transfer_memory::Mul(std::max(MaximumReplacementCells,imported.cells.size()),8192),`<br>`        surface_target?SurfacePlanningScratchBytes(std::max(MaximumReplacementCells,imported.cells.size())):0,`<br>`        transfer_memory::Mul(selection.geometry_repairs?2:1,MaximumFrozenFieldBytes),MetricPatchSolveScratchBytes,`<br>`        transfer_memory::Mul(3*MaximumCampaignEdges,sizeof(std::pair<const Edge,EdgeMeasure>)+6*sizeof(void*)),`<br>`        transfer_memory::Mul(8*MaximumReplacementCells,64));` | Const size value initialized by the admission calculation. |
| Candidate payload, 4377 | `Patch old,fresh;` | Two class objects without initializer (default-initialization). |
| Same scope, 4378 | `Node point;` | Class object without initializer (default-initialization). |
| Same scope, 4379 | `bool approved=false, moved=false, repair_approved=false, cavity_inserted=false;` | Four explicitly false bool values. |
| Same scope, 4380 | `bool optimization_approved=false;OptimizationOperations repair_optimization;` | Bool explicitly false; class object without initializer. |
| Same scope, 4381 | `MetricDiagnosis certified;` | Class object without initializer. |
| Same scope, 4382 | `bool geometric_progress=false;const char* last_geometry_operation=nullptr;` | Bool false; pointer value null. |
| Same scope, 4383 | `std::unique_ptr<FrozenField> field;` | Class object without initializer; unique owner default constructs empty. |
| ID range, 4384 | `const auto epoch=transfer_memory::Add(transfer_memory::Mul(round,world.size),world.rank);` | Const value, source-spelled `auto`; computed from round/rank. |
| ID range, 4385 | `const auto token=transfer_memory::Add(transfer_memory::Mul(epoch,options.candidate_batch),slot);` | Const value, source-spelled `auto`. |
| ID range, 4386 | `const auto offset=transfer_memory::Mul(token,MaximumReplacementCells);` | Const value, source-spelled `auto`. |
| ID range, 4387 | `const size_t construction_nodes=!options.wall_heights.empty() ? MaximumReplacementCells+3 : (composition || curved) ? 9 : 1;` | Const size value, conditional. |
| ID range, 4388 | `const size_t node_stride=construction_nodes+160;` | Const size value. |
| ID range, 4389 | `const auto node_offset=transfer_memory::Mul(token,node_stride);` | Const value, source-spelled `auto`. |
| Candidate observer state, 4390 | `bool capture=false, capture_selected=false;` | Two mutable bool values explicitly false. |
| Same scope, 4391 | `unsigned accepted_ladder=3;` | Mutable unsigned value explicitly initialized to three. |
| Same scope, 4392 | `std::vector<CellQualityFlag> old_flags,fresh_flags,measured_flags;` | Three class objects without initializers; empty vectors. |
| Transaction key, 4408 | `const Feature transaction_feature=repairing ? choice.feature : Feature{{choice.edge[0],choice.edge[1],0},2};` | Const value initialized from repair feature or constructed edge feature. |
| Repair work, 4414 | `size_t repair_budget=0,repair_work=0;` | Two mutable size values explicitly zero initialized. |
| Boundary predicate, 4420 | `const bool physical_edge=chosen && std::any_of(imported.cells.begin(),imported.cells.end(),[&](const OwnedCell& c) {`<br>`      const auto faces=Faces(c.cell);`<br>`      for(unsigned i=0;i<4;++i)if(c.physical[i].present &&`<br>`          std::find(faces[i].begin(),faces[i].end(),choice.edge[0])!=faces[i].end() &&`<br>`          std::find(faces[i].begin(),faces[i].end(),choice.edge[1])!=faces[i].end())return true;`<br>`      return false;`<br>`    });` | Const bool value. Nested `const auto faces` is the source-spelled type-deduced value inside the predicate, not an outer Slot field. |

### Slot post-evaluation and commit scopes

| Scope / current lines | Source-exact declaration | Binding / initialization note |
|---|---|---|
| After candidate evaluation, 6026 | `const std::string pending_rejection=chosen && !approved ? result.reason : std::string{};` | Const value initialized by conditional copy or empty temporary; temporary string lifetime ends at full expression. |
| Same scope, 6027 | `const bool surface_edit=approved && !SameSkin(old,fresh);` | Const bool value; this is a result of the existing SameSkin check. |
| Clearance block, 6031 | `const bool prior=approved;bool deferred=false;` | Const snapshot bool and mutable bool initialized false; both block locals. |
| Final reservation block, 6038–6039 | `Timer timer(stats.reservation_seconds,options.timings, SU2NativeTiming3D::Operation::Communication);CLocalFailure final_failure;` | RAII timer plus default-initialized failure accumulator; both block-local, not Slot payload fields. |
| Same block, 6054 | `const bool admitted=reserve_dependencies(imported,approved,result.reason,options.memory_ceiling,`<br>`          transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch),choice.reservation_priority);` | Const bool value from reservation. |
| Same block, 6062 | `const auto commits=TimedAllreduce(uint64_t(approved),CPassiveComm::Op::SUM,world.comm);` | Const collective result, source-spelled `auto`. |
| Commit preparation, 6067 | `std::vector<BoundaryParameterEdit> parameter_edits;` | Empty vector, default construction; conditionally filled. |
| Per-slot routing, 6085 | `std::vector<std::vector<Dependency>> removals(world.size);` | Vector value-initialized to `world.size` elements; inner vectors default constructed empty. |
| Same scope, 6086 | `std::vector<std::vector<OwnedCell>> additions(world.size);` | Same outer-size/empty-inner behavior. |
| Same scope, 6087 | `std::vector<std::vector<BlockedEdge>> blocked(world.size);` | Same outer-size/empty-inner behavior. |
| Same scope, 6088 | `std::vector<Dependency> removed;` | Empty vector. |
| Same scope, 6089 | `std::vector<OwnedCell> added;` | Empty vector. |
| Same scope, 6090 | `std::vector<BlockedEdge> failures;` | Empty vector. |
| Routing nested block, 6092 | `Timer timer(stats.routing_seconds,options.timings, SU2NativeTiming3D::Operation::Communication);` | RAII timer remains nested in the routing block. |
| Parent list nested in approved branch, 6096 | `std::vector<Id> parents;` | Nested local, not a cross-block Slot field. |
| Metric rows nested in approved branch, 6113 | `std::vector<OwnedCell> metric_rows;` | Nested local, not a cross-block Slot field. |
| Update tail, 6168 | `std::vector<Id> changed;` | Empty vector created after publication/frontier service; its lifetime spans the update block. |

## RAII and callable exclusions (kept separate)

These declarations are excluded from the Round/Slot data tables because their dynamic lifetime is itself significant:

- `std::optional<SU2NativeTiming3D::PhaseScope> phase_scope;` (3625), conditionally emplaced (3626–3629), caller's round scope.
- `Timer lane_timer(timed_lane==1?stats.surface_seconds:timed_lane==2?stats.interior_seconds:untimed_lane, options.timings&&timed_lane!=0);` (3643–3644), round scope.
- `wall_diagnostics::Choice wall_choice(wall_counters);` (4261), current slot scope.
- Local `struct CavityScope { CavityMetricSource& source; ~CavityScope() { source.SetCavity(nullptr); } };` and `CavityScope cavity_scope{source};` (4416–4419), current slot scope; the reference member binds canonical source and resets the cavity pointer on scope exit.
- `lane_census::Trial lane_trial(lane_counters, repairing, lane_slots[slot], dependencies, imported, world.rank, repair_work);` (4452–4453), local to `evaluate_candidate`; it references the selected fixed-array entry and `repair_work`, measures only the evaluator call, and is noncopyable with no declared move operations (`CNativeLaneCensus3D.hpp:87–111`).
- `FieldAccounting` (1315–1319) is a separate nested RAII object at field-query sites; its reference members are `EngineStats& stats; const FrozenField& field;` and destructor accounts field counts, including exception unwinding.
- Query and operation `Timer` / `D9QueryScope` objects remain at their existing inner block/call sites. They are not data payload.

No data table row is a reference alias or a `constexpr` local. `auto`-deduced declarations are marked explicitly. The only bound noted here is the header-level `MaximumCandidateBatch` constant (8); PART2 §2.3 evaluation locals are intentionally deferred until any later function-cut inventory. No move of a nontrivial temporary is proposed or performed; source contains ordinary moves at batch-to-slot unpack and proposal storage, but those operations are outside this declaration inventory's design scope.
