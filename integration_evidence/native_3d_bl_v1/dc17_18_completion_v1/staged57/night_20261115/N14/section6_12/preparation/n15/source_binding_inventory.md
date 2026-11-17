# N15 / M26 source-binding inventory

**Status and scope.** Read-only inventory for the M26 print-only measurement in copied work order 6.11, with the M25 result / N14 condition from 6.12. Current phase-A build551 source is frozen; G1 serial is live. This note proposes no implementation and does not release or implement 8.5ab. No source, test, policy, pin, or active-job file was changed; no build or run was started.

Copied inputs read:

- `N14/section6_12/inputs/NIGHT_WORK_ORDER.md` SHA256 `0c446a618cda84b707a66d27551bdd8344db1166206811ee3fa84c0ec9432464` (includes current 6.11 and 6.12).
- `N14/section6_12/inputs/ADAPTATION_ONE_ENGINE_DESIGN_V1.md` SHA256 `0ebcb6e50a2f8091ec1e660f3ad44d4ee6a4dbe225b041bb2dd24b886565f018`, revision 2.3, section 5.12.

## N12 population and actual commit point

`CNativeLaneCensus3D.hpp` (`Slot`, `Record`, `Counters::Classify/Begin/Commit/Print`) SHA256 `d26379e1aee755659c4bedfe12445d70dc23ceae51e130b7e05c09f2e1530b08` defines the population being extended:

- `Classify` returns class 1 (`local_wall`) when every `Dependencies::cells` owner equals `rank` and at least one `Imported::cells` physical facet is present. `Record` stores `uint64_t` trial/approved/commit/work/skin/parameter/coverage counts and `double seconds`.
- `Trial` is constructed at `CNativeEngine3D.cpp:4319` at entry to `evaluate_candidate`; its destructor adds the wall-clock interval and `repair_work` delta. `repairing=true` selects the quality-stage row. These seconds belong to trials reaching `evaluate_candidate`, not specifically the subset later committed.
- Approved quality proposals become `RepairProposal` entries in the private repair loop. Per-proposal `BoundaryParameterState::Changes(old,fresh,revision)` runs at `CNativeEngine3D.cpp:6248`; the only retained per-slot derivative is `lane_slots[...].no_parameter_edits`. Each proposal also retains `old`, `fresh`, `choice`, `approved`, and `ladder`.
- The actual quality commit receipt is at `CNativeEngine3D.cpp:6357–6359`, after `Directory::Publish` succeeds and after `final_reserved` may reject a proposal. It calls `lane_counters.Commit(true, lane_slot, SameSkin(old,fresh), no_parameter_edits, fresh.coverage.empty())`. Only approved proposals reach this call, so it is the binding point to identify M26 commits and to compare against N12 `quality/local_wall/commits`.
- The `lane_slot.category` and `reached` bits were set at trial entry, with category from dependencies/imported payload. `RepairProposal::ladder` is the existing `unsigned` accepted ladder: kind 0 is a validated `move_trial`; kind 1 is `optimize_repair`; kind 2 is a validated non-move repair/reconnect or geometry repair (`note_ladder` at 4250, 5685, 5738, 5820). The proposal defaults to 3 before a successful ladder. `w1_kind0/1/2` and `w2m_kind1` therefore have source-backed kind values, rather than new meanings.
- `lane_counters.Print(world.rank)` is called from the common Adapt `finish()` path at `CNativeEngine3D.cpp:2249`. It prints the 2x4 N12 census rows at adaptation exit. There is no separate existing quality-stage print hook. The current N12 wall time is the accumulated `Trial` duration for every class trial, not a per-slot duration retained for committed proposals.

`SameSkin` is `CNativeEngine3D.cpp:341`: compares the boundary face set of `Incidence(old.cells)` and `Incidence(fresh.cells)`, then exact stored coordinates for every boundary vertex. N12 `no_coverage` is specifically the empty check on `fresh.coverage.faces` and `.traces` at the commit sites (`6086`, `6359`); this is the proposal coverage delta, not a subtraction of old coverage. `fresh.coverage` is also the delta passed to `Directory::Publish`.

## Boundary changes, wall faces, and proof inputs

`BoundaryParameterEdit` (`CNativeBoundaryParameters3D.hpp:35–39`) is `{BoundaryPoint point; uint64_t revision; bool insert;}`. `BoundaryParameterState::Changes` (`CNativeBoundaryParameters3D.cpp:642`, header declaration at 147) validates old/fresh patch parameter-to-coordinate identity, then emits erasures for IDs in old but not fresh and insertions for IDs in fresh but not old. Its order is all sorted old-ID erasures followed by sorted fresh-ID insertions. Same-ID coordinate movement is rejected; moved boundary points receive a new identity. For M26, wall-only `e`/`i` counts can be derived by intersecting these edits with vertex IDs from `old.surface` / `fresh.surface`; the current lane slot retains only whether the full edit vector was empty, not its wall split counts.

`Patch::surface` is `vector<SurfaceFace>`; each face carries ordered `v` IDs and a facet ID. `old.surface`/`fresh.surface` at the commit site provide the old and fresh wall-face sets and their face counts. `proposal.fresh.coverage` is the forward coverage delta associated with the approved proposal; the repair-round union passed to `Directory::Publish` is assembled from these per-proposal face/trace entries at `CNativeEngine3D.cpp:6252–6256`.

M6b’s first local sign is source-visible in `UnitTests/SU2_CFD/adaptation/fixtures/native_m6b_projection_tests.inc:56–68`: for each `old.surface` face it resolves the old ordered face vertices and calls `Orientation(p0,p1,p2,projected_point)`. `Orientation` is implemented in `Common/src/adaptation/CNativePredicates3D.cpp:306` and is the certified filtered/exact tetrahedral orientation predicate; exact fallback uses integer determinant arithmetic. For M26 `first_proof`, the specified inputs are the one inserted wall point from the insert edit and each old wall face not present as a face of fresh; test nonnegative sign with the original ordered `old` face vertices. This is distinct from the M6b fixture’s per-marker census of every old wall face. Relevant predicate internal header: `Common/src/adaptation/CNativePredicates3D_internal.hpp`.

## `link_local` data path and cost boundary

Public `BoundaryParameterState::FacesOnVertex(Id)` (`CNativeBoundaryParameters3D.hpp:126`, implementation `CNativeBoundaryParameters3D.cpp:578`) returns a range in the replicated `vertex_faces` set indexed by `(vertex ID, FaceKey)`. `FaceKey` sorts the three IDs. This lets the check enumerate physical faces incident to old wall vertices without scanning all global parameter faces.

There is also a local ownership star index already maintained by `Selection`: `vertex_cells` is `map<Id,set<Id>>` (`CNativeEngine3D.cpp` around 518); `Selection::Add`, `Drop`, and `Restore` update it from each rank’s owned cells (around 1077, 860, 1199). It is public within the engine-local `Selection` class. For each `FacesOnVertex` face, its vertex’s `selection.vertex_cells` set supplies only local candidate cell IDs; `working.find(cell_id)` then exposes each `OwnedCell::cell` and its four `physical` bindings. The candidate tetra faces can be compared by sorted `FaceKey` and physical facet ID. Thus the available indices support a local-star membership check without rescanning the whole `working` mesh per commit. `Selection::CellState::physical` stores edge-adjacent booleans, not facet IDs; authoritative face/facet verification must use the current `working` `OwnedCell::physical` rows.

M26 `link_local` requires every `FacesOnVertex` wall face for every wall vertex of `old` to be matched by such a rank-owned physical tetra face. The existing `vertex_cells` index contains only cells currently cached in `Selection`; it is populated and maintained from owned cells during adaptation. It is not a directory-wide remote ownership query and must not be confused with `Directory::Lookup`/`Dependency` owner rows. `FacesOnVertex` iterators are borrowed views invalidated by boundary-state commit/replacement; a future check must not retain them across publication.

For `first_proof`, compare face identity using canonical vertex set plus facet identity, but use the original `old.surface` ordering for `Orientation`. Per 6.11, W1 `i=1` requires testing every old wall face absent from fresh; `w1_link_local_first_proof` is the intersection of W1, link-local, and all those signs nonnegative. The M26 implementation is report-only; no gate or wall-lane behavior is inferred here.

## `Directory::PrepareParameters` measurement binding and gaps

`Directory::Publish` calls the private `PrepareParameters` only when the directory has a parameter store (`CNativeDistributed3D.cpp:1619`). The quality repair-batch publication is `CNativeEngine3D.cpp:6323` and passes `repair_support_changes=true`; the call collects all approved proposals’ parameter edits and coverage before that single publication. Other construction/layer publications are at Engine lines 2434, 2588, and 2835; those are not the quality lane population.

Within `PrepareParameters` (`CNativeDistributed3D.cpp:1441`), local old/added physical faces are netted into `BoundaryFaceChange`; all ranks exchange/broadcast face and parameter-edit payloads, and then `parameters->Prepare(global_edits, faces, ..., global_coverage, reservation)` is called once at line 1565 per rank for that publication. `global_edits` is `vector<BoundaryParameterEdit>` and `faces` is `vector<BoundaryFaceChange>`; they are the union payloads after routing. `BoundaryParameterState::Prepare` is local on that rank and mutates no accepted state; the directory commits the staged result only after publication succeeds.

Current `DirectoryStats` (`CNativeDistributed3D.hpp:75–97`) has `double parameter_publication_seconds`, `uint64_t parameter_edits`, and `parameter_faces`, but no parameter-Prepare call counter or inner-`parameters->Prepare` seconds. Its `Timer timer(stats.parameter_publication_seconds)` at function entry surrounds the entire `PrepareParameters` operation: local admission, collectives, atlas-page routing, union construction, and the final local `parameters->Prepare`; it is not the requested local clock of that final call. The edits/faces counters are incremented after that call (`CNativeDistributed3D.cpp:1571`) from the global union vectors, and are cumulative per Directory, not stage-scoped. The engine copies those totals into `EngineStats` in its common `finish()` path (`CNativeEngine3D.cpp:2211–12`), after collecting retired Directory stats. `Directory` has no `options.timings` input; the current method’s timer is unconditional. These existing fields alone cannot report the exact requested N15 Prepare-call count/time or isolate the 6.11 quality-stage `Prepare` work. No new `Session`, `CellState`, traveling row, or Timer operation is present in the current source.

## Source hashes read

| File | SHA256 |
|---|---|
| `Common/src/adaptation/CNativeEngine3D.cpp` | `ffcb292b4639128c26495f7df44a4b63b9c61b0df62222547f4b4ad253ba9020` |
| `Common/include/adaptation/CNativeLaneCensus3D.hpp` | `d26379e1aee755659c4bedfe12445d70dc23ceae51e130b7e05c09f2e1530b08` |
| `Common/src/adaptation/native_engine3d/EngineSlot.hpp` | `f8665887133dbf2568fb0c1f8564598b75d5530b2adaa01d8859a5b19b6912d4` |
| `Common/src/adaptation/native_engine3d/EngineRound.hpp` | `be6bcc5290edb3543c5a9a8315161e4e1d701820b5885b18b3d31aed2b94d373` |
| `Common/src/adaptation/native_engine3d/EngineContext.hpp` | `74d6202b1462cd3ce3670bf89009b08d8e55dfd05f46cd4010e2fd5ae9e5ee9d` |
| `Common/src/adaptation/native_engine3d/EngineSetup.hpp` | `46bbb95747d1f39dd5dc4faa0553577b3b686fd0cebd198431c6b00b934d2d1b` |
| `Common/include/adaptation/CNativeDistributed3D.hpp` | `2510df14023dc3c7ab26e77516b46a084823ade348e8b43a345243a7f31e1e3a` |
| `Common/src/adaptation/CNativeDistributed3D.cpp` | `07e62d504e2251865140522404a06ef4eeec2bf5c651e5d93ecacc51d0f9c8a5` |
| `Common/include/adaptation/CNativeBoundaryParameters3D.hpp` | `13e6c5d2f3d08b26d0a000e140457b3c9e1bc74eb6bc60f1fa69cb1c7de24421` |
| `Common/src/adaptation/CNativeBoundaryParameters3D.cpp` | `4f5607bb8dea58a0a5ceb271e629e64def8207723b4bbb94829bee7da7a51261` |
| `Common/src/adaptation/CNativePredicates3D_internal.hpp` | `8a0a788705c07d91aa99786c9a39b412da76db2ec2c8d53cdc1995bc72dc5bfe` |
| `Common/src/adaptation/CNativePredicates3D.cpp` | `9adaa29e0b90112bd30e82208f3b4063249065056a8af06692de60d9d794a012` |
| `UnitTests/SU2_CFD/adaptation/fixtures/native_m6b_projection_tests.inc` | `2df90ddc862d5140b50ba0c4e825bc4c6cef516463f3538935cc9c75a2ceeb70` |
| `N14/section6_12/inputs/ADAPTATION_ONE_ENGINE_DESIGN_V1.md` | `0ebcb6e50a2f8091ec1e660f3ad44d4ee6a4dbe225b041bb2dd24b886565f018` |
| `N14/section6_12/inputs/NIGHT_WORK_ORDER.md` | `0c446a618cda84b707a66d27551bdd8344db1166206811ee3fa84c0ec9432464` |
