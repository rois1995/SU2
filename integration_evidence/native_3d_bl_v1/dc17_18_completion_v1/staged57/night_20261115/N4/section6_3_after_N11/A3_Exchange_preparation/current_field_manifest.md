# A3 Exchange current-field manifest (Queues candidate, validation pending)

Read-only mapping of PART1 §2.7 Exchange plus its canonical borrowed inputs. This is the current A3 Queues candidate source state, validation pending; it is not an implementation proposal. Source hashes at inspection:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `6f781f70cd2dec94ec89cbbb5d75b4440c9b35725ef9337f401d095436fb5f90`
- `Common/src/adaptation/native_engine3d/EngineContext.hpp`: `4b9312e851643119ac588d96b42359838b773c6f013683c15ef94cfab59e8ddc`
- `Common/src/adaptation/native_engine3d/EngineContext.cpp`: `637c4e0a42147af18635c3398d101892035c5c34cdff5dccd75454b1917dbb58`
- `Common/include/adaptation/CNativeMetricTransport3D.hpp`: `cb9a45c0b8a990e1e58514fe7335ba970aecc4a6240401bf0120a308c62b3494`
- `Common/include/adaptation/CNativeDistributed3D.hpp`: `2510df14023dc3c7ab26e77516b46a084823ade348e8b43a345243a7f31e1e3a`
- `Common/src/adaptation/CNativeDistributed3D.cpp`: `07e62d504e2251865140522404a06ef4eeec2bf5c651e5d93ecacc51d0f9c8a5`
- `Common/src/adaptation/CNativeDonor3D.cpp`: `190770810169bfc3f9e0c6bcfc974bb6500641d07d83ec8e7f5a3922182085eb`
- `inputs/CUT_INVENTORY_PART1.md`: `034275df32e2f4b7f4ef9bb2aef0bc16453f6d0249971b90fc42ba6a33b663cd`

## Adapt inputs and canonical aliases

`Adapt(World& world, const Owned& accepted, const std::vector<DonorCell>& original, const PlanarReference& accepted_geometry, ...)` is the original entry point (PART1 snapshot lines 1237–1239). `World` is `SU2NativeBoundary2D::World`, passed as a non-const reference; the adaptation context also borrows it. It remains the one canonical MPI context: rank/size/communicator, election, reductions, exchange, timing, and communication counters. `accepted` and `original` remain const input references. `working` is the separately owned mutable `Owned` partition. `geometry` is the selected `const PlanarReference&` (private/atlas view when present, otherwise the accepted geometry); it is also passed by reference into `EngineContext` and to `Directory` as its parameter authority. These are references/aliases, not duplicate Exchange state.

The current `EngineContext` constructor stores references to setup, world, accepted, original, geometry, composition, options, intent, bidirectional and surface target. The local lifetime interleaving in `Adapt` is: geometry/context setup; `Owned working`; Selection/Census and Control wrappers; `directory`; `source`; checkpoint wrapper; `retired_directory`; subsequent byte/Control/measurement locals. Exact current declaration points are described below; order is source order, not a proposed regrouping.

## Current Exchange records

| Name | Exact current type / initializer | Current declaration | Source-supported role and references |
|---|---|---:|---|
| `construction_claims` | `bool construction_claims=bidirectional;` | 1577 | Whether the construction pass currently owns directory claims; toggled by phase/generation entry and ownership migration. Relevant `Directory::BeginConstructionPass` / `EndConstructionPass` calls are coordinated with it. |
| `directory` | `auto directory=std::make_unique<Directory>(world,true,geometry.Parameters());` (`std::unique_ptr<Directory>`) | 1603 | Distributed lookup/reserve/publish state. `Directory` holds `World&`, track-changes flag and the borrowed parameter-state pointer; `Build` binds its parameter-geometry pointer from that state. It is swapped during phase ownership migration and checkpoint rollback; replaced instances have their `Statistics()` accumulated before retirement. |
| `source` | `CavityMetricSource source(world,working,options.timings);` | 1604 | Donor/current metric transport. The type stores `World& world`, `Owned& current` (`working`), `DonorImport bootstrap`, stats, a stage snapshot, and an optional borrowed cavity-vector pointer. Its inline constructor binds the two references and constructs `bootstrap(world,timings)`; it declares no custom destructor. `SetCavity` is set/reset around private repair evaluation. Collective source APIs are bypassed by the corresponding local APIs while `private_repair` is true. |
| `checkpoint_lifetime` / `checkpoint` | `ContextValue checkpoint_lifetime(control.checkpoint);` then `auto& checkpoint=...Get();` | 1605–1606 | Control-owned checkpoint slot activated in the original local scope. A checkpoint may carry a dormant Directory whose statistics are retired on discard/finish; it is distinct from the active directory. |
| `retired_directory` | `DirectoryStats retired_directory;` | 1607 | Accumulator for statistics from replaced/retired directories. `retire_directory` adds each statistic field, including maximum work bytes. `finish` temporarily adds active/checkpoint directory statistics, copies the total to EngineStats, then restores the accumulator. |
| `private_repair` | `bool private_repair=false;` | 1650 | Communication-mode flag, not owned directory/source payload. It switches `elect_trial_failure`, `vote`, max/reduction and source import wrappers between world collectives and rank-local work for private repair. It is set immediately before the private repair trial and cleared afterward. |

`retire_directory` is an `[&]` lambda immediately after the accumulator. It mutates only that accumulator from the supplied `DirectoryStats`. `phase_ownership`, `capture_checkpoint`, `rollback_checkpoint`, `discard_checkpoint`, `finish`, `import_source`, metric-measurement helpers and trial wrappers access these objects by reference. These capture relationships are lifetime constraints: the active `Directory` and `CavityMetricSource` must refer to the same `world`, `working`, and resolved `geometry` instances while their calls execute.

## Accounting and type/capture observations

Admission calls include `source.ResidentBytes()` and `directory->ResidentBytes()` alongside local `bytes()` / extra scratch. `bytes()` itself accounts the inactive blocked-edge map and linked estimate; `retired_directory` is a fixed stats value and is not separately added there. `Directory::ResidentBytes()` covers directory resident stores and collision index; `CavityMetricSource::ResidentBytes()` reports its retained transport state. The active `Directory` object’s size is included in one directory-creation/admission expression (`sizeof(Directory)` plus `ResidentBytes()`); no analogous `sizeof` call for the source or retired stats was found in `Adapt`.

`CNativeEngine3D.cpp` has one raw `decltype`: `decltype(checkpoint.blocked)()` in checkpoint rollback. No `typeid` use was found. Lambdas capture local Exchange values by reference; notable ones are `retire_directory`, `vote` / elect wrappers, `phase_ownership`, `finish`, `import_source`, and later helper/trial closures. `private_repair` is consulted by local-vs-collective wrappers; do not classify it as a mere directory statistic.
