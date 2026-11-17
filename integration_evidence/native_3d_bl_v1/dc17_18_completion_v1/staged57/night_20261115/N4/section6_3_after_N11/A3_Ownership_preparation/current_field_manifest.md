# A3 Ownership current-field manifest (Commit candidate; validation pending)

Read-only mapping of PART1 §2.8 against the current Commit candidate. Commit is the current base and remains validation pending; the Ownership draft below is unapplied. PART1 line references are its inventory snapshot, not current-line quotas. Source hashes at inspection:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `b9f9152ff16c9ce0a9af0599641b814e15c475888c552172cc7b4ba8c7542cf5`
- `Common/src/adaptation/native_engine3d/EngineContext.hpp`: `08cf3a6a2a138f7c24405230f4ce60eed74694150f502bdcce044b2fe7ff6dc0`
- `Common/src/adaptation/native_engine3d/EngineCommit.hpp`: `4ec8ccd59d65825126c941582f1e09f5fe233019256144c060bea26783b6db8a`
- `Common/src/adaptation/native_engine3d/EngineQueues.hpp`: `d4fac4045aef775c2bec4df2fb9ed74d8b97bd26503670b4649a7e8f2de2df37`
- `Common/src/adaptation/native_engine3d/EngineExchange.hpp`: `5218f814e469ce682b03549983198aedf1586f7e4b23449531088a6bcdf464c8`
- `Common/include/adaptation/CNativeEngine3D.hpp`: `c661acd0f8866b7e2bcf0884bf944286409f5c2d92ff548c510da3aefe3e75d7`
- `Common/include/adaptation/CNativePartition3D.hpp`: `d627f810d53b0b36fa31d9acfcc4bb841a319ff64fcf6b84d197dee78a11b756`
- `Common/src/adaptation/CNativePartition3D.cpp`: `84abd9c97aa1a81efbdedc0018a815471bddeb4e563c0671171c0fafdf6b7669`
- `Common/include/adaptation/CNativeDistributed3D.hpp`: `2510df14023dc3c7ab26e77516b46a084823ade348e8b43a345243a7f31e1e3a`
- `Common/include/adaptation/CNativeAffineReference.hpp`: `057640d76fd8623ee964b02646d149245180e35b62b8f5797cbf525a1c1ad36f`
- `Common/src/adaptation/CNativeAffineReference.cpp`: `038d827da44be3c1277c3e53493fdbc2c084a096a9edb6e5072fe9b1d2814f31`
- `inputs/CUT_INVENTORY_PART1.md`: `034275df32e2f4b7f4ef9bb2aef0bc16453f6d0249971b90fc42ba6a33b663cd`

## Canonical inputs and persistent Adapt fields

| Name | Exact current type / initializer | Current source | Ownership / relationships |
|---|---|---:|---|
| `world` | Adapt input `World& world` | 1443 | Canonical collective context. Passed by reference through `EngineContext`, Directory, `VertexStarSizes`, and `Repartition`; no duplicate World object. |
| `working` | `ContextValue working_lifetime(context.commit.working); auto& working=working_lifetime.Get();` | current Commit candidate Adapt setup | Current base stores the canonical mutable partition in `EngineCommit`; the draft places its optional storage in `EngineOwnership` to match PART1 §2.8. At the original Adapt-local activation point, the same `working` alias binds to that stored object, and `source` borrows it. No `Owned` value is copied or moved between slots. Ownership staging still copies to `pending`, then swaps `working` only after staged migration and observer checks pass. |
| `selection` | `ContextValue selection_lifetime(context.census.selection, ...); auto& selection=...Get();` | 1573–1576 | Canonical Census/Queues `Selection`; retains same `geometry` and `stats` references through ownership transition. Its cached cell/edge rows and repair support migrate with cells; state swaps with `pending_selection`. |
| `geometry` / `geometry_authority` | `const auto& geometry=atlas_geometry?*atlas_geometry:private_geometry?*private_geometry:accepted_geometry;`; `geometry_authority` is a Control optional pointer populated from `geometry.Parameters()` | 1549, 1562–1564 | Canonical resolved geometry view and authority/cache. The ownership cost model reads `StructuralEstimate` / geometry statistics from the authority; pending Selection and Directory use the same geometry/parameter authority. |
| `surface_target` | Adapt by-value `BoundaryMetric` | 1445 | Canonical target callback; the ownership weight loop checks it and `selection.RequestedWall` to classify untouched volume cells and physical edge demand. It is not copied into a new ownership object. |
| `ownership_evaluated` | `std::array<bool,2> ownership_evaluated{{false,false}};` | 3258 | Adapt-scope per-boundary evaluation state; boundary index is `quality ? 1 : 0`. Construction marks before early skip checks; quality marks after its progress/checkpoint gates. |
| `quality_ownership_round` | `uint64_t quality_ownership_round=0;` | 3259 | Adapt-scope last quality evaluation round; used with `stats.rounds` and `options.stagnation_window` to defer repeated quality migration work. |

`phase_ownership` is a by-reference closure at 3252. It captures the canonical World, working mesh, Selection, geometry authority, target callback, Control/result/stats, Exchange Directory/source, and Census/Queues state. It does not own copies of these canonical inputs. Its temporaries below live only for one invocation.

## Ownership measurement and migration temporaries

| Name / group | Exact type / initializer | Current source | Source use and lifetime |
|---|---|---:|---|
| `boundary`, `transition_started` | `const unsigned boundary=quality?1:0;`; `const double transition_started=world.seconds();` | 3259, 3272 | Local per invocation; boundary selects the two-element evaluation state, start time covers cost and optional migration. |
| `scratch` | `const auto scratch=transfer_memory::Add(...)` | 3273–3277 | Admission estimate for working-map copy, per-cell weight/cached-row arrays, vertex-star count maps, rank vectors, and optional geometry-facet allowance. Uses exact `sizeof` terms in the expression. |
| `complete_stars` | `std::map<Id,uint64_t> complete_stars;` | 3279 | Filled by collective `VertexStarSizes(world,working,...)`; consumed to construct per-cell demand. |
| cost vectors/scalars | `std::vector<uint32_t> weights,geometric,quality_weights; std::vector<double> costs,geometry_costs,quality_debts; double local_demand=0,local_geometry=0,local_quality_debt=0;` | 3283–3284 | Local per-invocation work estimates. Quality debt/vector are populated only in quality mode. Values are normalized with global MAX/SUM reductions and passed as `Repartition` weights. |
| `pending` / `pending_initial` / `pending_directory` | `Owned pending; std::vector<Id> pending_initial; std::unique_ptr<Directory> pending_directory;` | 3372 | Staged candidate mesh, next initial list, and a new directory. They coexist with active `working`/directory until checks and publication complete. |
| `pending_selection` | `Selection pending_selection(coarsening,repairing,geometry,stats,options.timings,options.surface_tolerance,options.wall_heights,bidirectional,bool(surface_target));` | 3373 | Temporary cache state constructed against the same canonical geometry/stats/heights; receives cached-row certificates and repair state before the ownership swap. |
| `partition` | `PartitionStats partition;` | 3383 | `Repartition` metrics for this call (weights, moved cells, per-rank work/geometry/debt, timings, peak modeled bytes); consumed for stats and print-only report rows. |
| outgoing/cache records | `std::vector<std::vector<Selection::CachedRow>> outgoing;` and `const auto records=world.exchange(...)` | 3391, 3409 | Certificates route cached Selection rows with partition destinations; final records restore the receiving ranks’ cached cell/edge lengths and repair state. This is migration of cache state, not geometry re-query/recomposition. |

## Source-supported cost/cache and observation behavior

The imbalance score is the max of work, geometry, and (in quality mode) quality-debt imbalance. Work demand uses cached edge lengths, insertions, complete vertex-star counts, and quality debt/invalidity. The geometry term in quality mode calls `EstimateGeometryWork`; otherwise each physical face uses `geometry_authority->StructuralEstimate(facet)`, including `prepared_pieces`, and `control_deviation / surface_tolerance`. The same authority’s snapshot contributes `scope_build_seconds / scope_builds * scope_entries` as warming allowance (statistics read from the same geometry authority). Thus existing prepared geometry/cache state is an input to reported cost and the migration proceed test. No proposal is made to alter that model.

After normalization/reductions the code computes elapsed estimate, rebuild/warming allowance, predicted gain and cost, and uses `world.maximum(!locally_eligible)` so all ranks make one collective proceed decision. On migration, cached Selection rows are transferred and restored; source comment explicitly says there is no geometry query, target recomposition, or CFD projection during migration. `World::seconds`/collective reductions, graph cost, pending directory build and observers remain in the existing order.

Print-only rows exist when `options.timings`: `NATIVE3D_PHASE_OWNERSHIP` logs the decision and the committed partition metrics; `ReportOwnershipMemory` emits before/staged/committed observations. `EngineStats` also accumulates ownership evaluations, changes, moved cells, seconds, predicted gain/cost and maximum work bytes. These are existing reports, not newly accepted decision inputs. No new ownership counter state was found in the current candidate.

`bytes()` accounts the working mesh and live Selection/queues; `phase_ownership` separately admits its scratch and pending/outgoing/record estimates. `VertexStarSizes` and `Repartition` receive caller bytes and report their own maximum-native-byte values. Migration baselines include `source.ResidentBytes()` and active Directory resident bytes. The current Adapt body has one raw `decltype` (`decltype(checkpoint.blocked)`); no `typeid` use was found.
