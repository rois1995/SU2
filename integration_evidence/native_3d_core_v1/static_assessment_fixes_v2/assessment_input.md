# Native 2D and 3D remeshing: static assessment

- Branch: `codex/native-3d-transport-accounting`, HEAD `7746fd53fa`
- Worktree read: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated` (tracked `Common` and `SU2_CFD` sources clean at HEAD)
- Date: 2026-10-12
- Method: reading of the source only. Nothing was built or run. No timing, memory or frequency figure in this file is measured; each is a count or an estimate from the code and is marked as such.
- Scope: `Common/{include,src}/adaptation/CNative*` (about 11 000 lines), and the driver hooks in `SU2_CFD/src/drivers/CSinglezoneDriver.cpp`, `CDriver.cpp`, `SU2_CFD/src/solvers/CSolver.cpp`, `Common/src/CConfig.cpp`.
- Not read: `CReaderSlices`, the `CSolutionTransfer` family, `CBoundaryLayerMetric`, the unit tests beyond a skim, `integration_evidence/`. Conservation of the solution transfer and the consistency of the nodal metric across ranks are therefore not assessed.

Parts II and III are the full per-dimension reports, each with file:line evidence. Part I is the merged verdict. Finding labels restart in each part, so they are written `2D-C1`, `3D-C1` and so on in Part I.

---

# Part I — Verdict

## I.1 Summary

| Axis | 2D | 3D |
| --- | --- | --- |
| Correctness of what is accepted | Good. Exact orientation predicate, transactional cavities, validation before publication. No memory-safety, overflow or deadlock defect found. | Good, same design. Exact integer predicates, private validation, atomic publication. |
| Correctness as a remesher (does it produce the requested mesh) | Limited by fixed work budgets and a fixed quality contract that ends the run when missed. | Not usable on real geometries: the boundary must be one closed surface of at most 512 triangles, flat-faced, and the pass budget is 512 rounds. |
| Performance | Does not scale to 1e6 cells. One cavity per rank per collective round; several full-mesh scans per round. | Does not scale. Same round structure, plus repeated full-mesh rebuilds per event and exact rational tracing of metric edges. |
| MPI readiness | Truly distributed (the volume mesh is never gathered; interfaces are not frozen). But the protocol is dense in P and synchronous, and it saturates at roughly 50 to 200 ranks. | Same architecture and same limits, with an extra false-refusal mechanism that grows with rank count. Tested to 4 ranks only. |

Counts: 2D has 1 Critical, 5 High, 13 Medium, 12 Low. 3D has 3 Critical, 10 High, 10 Medium, 10 Low.

The code is a careful, conservative prototype. Its failure mode is refusal, not a corrupt mesh. The blocking problems are the hard-coded limits and the round protocol, not bugs in the operators.

## I.2 Blocking findings

Each item below was re-read against the source for this merged report, in addition to the per-dimension review.

1. **Any incomplete remesh ends the CFD run (2D-C2, 3D-C2).** `CSinglezoneDriver.cpp:1676-1681` turns every status other than `COMPLETE` into `CollectiveFailure`, and `CDistributedSearch.cpp:460-463` turns that into `SU2_MPI::Error`. The message says "the accepted mesh and solution are retained", but the job stops. There is no relaxation, retry or fallback to the previous mesh inside the loop. Every budget below therefore becomes a fatal error.
2. **3D boundary limited to 512 triangles and one closed sphere (3D-C1).** `CNativeImport3D.cpp:361-363` validates the whole original skin with `ValidateSurfaceTopology(skin, true)`. `CNativeCavity3D.cpp:130-133` rejects more than `4 * MaximumReplacementCells` = 512 triangles (`CNativeCavity3D.hpp:13`), and `Surface` (`CNativeCavity3D.cpp:55-83`) demands one connected surface with Euler characteristic 2. A body inside a far-field box has two components and fails. So does any surface above 512 triangles.
3. **3D pass budget of 512 rounds (3D-C3, 3D-C4).** `CNativeEngine3D.hpp:13` fixes `maximum_rounds = 512`; the driver constructs the remesher without options (`CSinglezoneDriver.cpp:299`). With one attempt per rank per round, a pass can make at most 512 × P edits. `CNativeEngine3D.cpp:898-903` also marks a mesh that satisfies every gate as invalid when collapse candidates remain at the limit.
4. **2D work budget (2D-C1).** `CNativeRemesher.cpp:421-422` caps `phase_rounds` at 20 000 and `CNativeEngine2D.hpp:60` fixes 6 sweeps, so one rank can make about 120 000 bulk insertions per call. Reaching 1e6 cells needs many well-balanced ranks; 1e7 needs more than 40.
5. **No curved boundaries in 3D (3D-C6).** New boundary points must lie exactly on the edge or exactly in the plane. There is no reprojection. In 2D the reference is the original polyline (2D-C6), so refinement does not recover curvature either.

## I.3 Performance

- **Round structure (2D-P1, 3D-P1).** Each global round commits at most one cavity per rank and costs about 150 to 250 blocking collectives in 2D and about 165 to 180 in 3D (counted from the code). Throughput is bounded by collective latency, not by local work. This is the main reason the design does not scale, and it is architectural.
- **Full-mesh work inside the loop.** 2D: surface selection rescans all owned cells every round (2D-P3), `CollisionVeto` scans all owned cells per proposal (2D-P4), and the donor search tree is disabled above about 29 000 cells per rank by a hard-wired 2 MiB budget (`CNativeField2D.hpp:341-354`, 2D-P2), after which each query scans the whole donor map. 3D: two full-mesh seeds, four `DonorImport::Build` and two to three `Directory::Build` per event (3D-P2).
- **Containers.** Cells live in `std::map`; the kernels rebuild `std::map`/`std::set` and copy cavities per trial (2D-P7, 3D-P5). Estimated footprint in 2D is about 0.9 kB per triangle (2D-P8). 3D has a fixed 2 GiB ceiling (3D-P6).
- **Exact arithmetic in 3D.** Every metric edge is traced with exact rationals and the same edges are integrated two to three times per accepted edit (3D-P3, 3D-P4). The cost is an estimate.
- No O(N²) stage was found in 3D. In 2D the full scans per round make the total cost O(rounds × N).

## I.4 MPI readiness

What is good:

- The volume mesh is never gathered on one rank. Interfaces are not frozen: cross-rank cavities are imported, validated privately, and committed atomically.
- Failures are elected collectively. No path was found where one rank leaves a collective sequence while others wait, so no deadlock. Message sizes and global identifiers are 64-bit.
- Results are deterministic for a fixed rank count as far as the source shows (ordered containers, no pointer ordering).

What is not ready:

- **Dense in P.** Regions, decisions, certificates and physical faces go all-to-all (`MPI_Alltoall` plus P-sized buckets; 2D-M2, 3D-M2). The hash-distributed directory makes almost every lookup remote (3D-M5).
- **Budgets that shrink with rank count.** 2D surface certificates are sent to all ranks under the 2 MiB budget, so at roughly 50 to 200 ranks every surface transaction is refused (2D-M1, threshold estimated). 3D `Publish` applies the single-cavity limits (64 removed, 128 added) to the per-rank sum over all concurrent cavities (`CNativeDistributed3D.cpp:504-505`, 3D-M1), so false refusals grow with rank count.
- **Fairness.** Reservation conflicts always go to the lowest rank (`CNativeDistributed3D.cpp:428-434`, 2D-M3, 3D-M3). The rotating-reservation scheme named in the recent commits exists only under `integration_evidence/`, not in the production sources.
- **One refusal discards the round.** In 2D a single refused transaction discards and blacklists every rank's transaction of that round (2D-C7).
- **No load balancing during a pass** (2D-M4, 3D-M4). `ADAP_NATIVE_REPARTITION` is off by default.
- **Master-rank residue.** Boundary faces of the returned mesh and the reference sidecar are gathered on the master (2D item 12, 3D-M10). This is O(boundary), acceptable for now.
- **Rank-count dependence.** Results differ with the rank count by design (2D-M5, 3D-M7).
- **Test coverage stops at 4 ranks** on small meshes.

## I.5 Driver integration (read for this report)

- `ReplaceMesh` (`CSinglezoneDriver.cpp:1669-1852`) rebuilds the full CFD geometry, repartitions (ParMETIS through `SetColorGrid_Parallel`), recomputes wall distances and rebuilds all solvers every cycle. This is the same path as MMG and is not specific to the native backend; it is a fixed cost per cycle that the native code cannot reduce.
- If `transfer.Transfer` or a later initialisation step fails after the new geometry and solvers are allocated (lines 1715-1784), they are not released. The failure is fatal anyway, so this is Low.
- `ComputeMetric` (lines 251-254) calls `MakeRemesher()` only for its side effect on the retained reference and discards the object. It works, but the dependency is hidden.
- `ADAP_NATIVE_RANKS` and `ADAP_NATIVE_REPARTITION` are validated in `CConfig.cpp:6412-6416`. No option exposes the round, memory or dependency budgets of items I.2.3, I.2.4 and I.3.
- "Transport accounting" in the branch name is bookkeeping of MPI bytes and calls for profiling. It is not solution transfer. It leaves out the failure elections, which are most of the collectives (3D-M8).

## I.6 Recommended order of work

1. Make an incomplete remesh non-fatal: keep the previous mesh, or accept the best valid mesh with a warning. This removes the fatal consequence of every budget.
2. Remove the 3D boundary limits: validate the original skin with a routine that has no cavity-size cap and allows several components, then add projection to a curved reference.
3. Expose the round and memory budgets as options and scale them with the mesh size.
4. Commit many independent cavities per rank per round (colouring or an independent set), and replace the all-to-all exchanges by neighbour exchanges. Without this step neither dimension scales, whatever else is tuned.
5. Replace per-round full scans by incremental queues, and `std::map` cell storage by flat arrays.
6. Move rotating reservations into production and fix the `Publish` budget to be per cavity.
7. Test at 16, 64 and 256 ranks on a mesh of at least 1e6 cells before any further qualification work at 1, 2 and 4 ranks.

## I.7 What only a run can settle

- Real cost of one round, and the rank count at which the protocol saturates.
- How often the 64-cell star limit (3D-C5), the shape gate (3D-C7), the donor caps (2D-C4, 3D-C8) and the collective refusals trigger on realistic cases.
- Whether the fixed 2D quality contract (q ≥ 0.18, edge ≤ 1.8) is reachable on realistic anisotropic metrics.
- All memory figures.

---

# Part II — 2D full report


Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch `codex/native-3d-transport-accounting`, HEAD `7746fd53fa`.
`git diff HEAD --stat --ignore-submodules -- Common SU2_CFD` is empty: tracked sources are clean.
Method: static reading only. Nothing was built or run. No repository file was modified.

Files read in full: `Common/include/adaptation/` CNativeMesh2D, CNativeEngine2D, CNativeBoundary2D, CNativeField2D,
CNativeDistributed2D, CNativeImport2D, CNativeReference2D, CNativeGeometryValidation2D, CNativeBalanceProfile2D,
CNativePartition2D, CNativeReferenceIO, CNativeRemesher, CRemesher, CSimplexMesh (.hpp); `Common/src/adaptation/`
CNativeRemesher.cpp, CNativePartition2D.cpp, CNativePredicates2D.cpp (no header; declared in CNativeMesh2D.hpp:59-61).
There is no other `CNative*2D` source (no separate 2D scaling file; only the hidden unit test `CNativeScaling2D_tests.cpp`).
Read in part, to check callers: CTransferMemory.{hpp,cpp}, CPassiveComm.cpp (AlltoallvRounds), CDistributedSearch.cpp
(ElectFailure/CollectiveFailure), CReaderSlices.cpp (BuildDistributedSlices), CSinglezoneDriver.cpp (MakeRemesher,
ReplaceMesh, AdaptMesh), CBoundaryLayerMetric.cpp (ApplyPoint/Evaluate), meson files, CConfig option defaults.

Path shorthand below: `Mesh2D` = Common/include/adaptation/CNativeMesh2D.hpp, `Engine2D`, `Boundary2D`, `Field2D`,
`Distributed2D`, `Import2D`, `Reference2D`, `GeomVal2D`, `Partition2D.cpp`, `Remesher.cpp`, `Predicates2D.cpp` likewise.

---

### 1. Architecture summary (data flow)

1. **Import** (`Remesher.cpp:85-153`, `Import2D:59-167`). Each rank turns its owned CFD triangles into value-type
   `Cell` records (`Boundary2D:36-47`): three `Node`s stored by value (global id, coordinates, fixed flag), three edge
   markers, a version, cached quality and edge lengths, three nodal target tensors. Node ids are CFD global indices,
   cell ids come from an exclusive prefix sum. Halo coordinates and metrics are fetched through a `CPointDirectory`.
   Volume edges and boundary labels are hashed to an owner rank to find physical faces and check manifoldness.
2. **Reference** (`Reference2D`, `GeomVal2D`, `CNativeReferenceIO.hpp`). All physical faces are replicated on every
   rank and chained into directed polyline components (`PolylineReference`). This original piecewise-linear geometry
   is the only boundary geometry; it is persisted in a `.native_ref` sidecar. Boundary points are placed on it by
   arclength; deviation is measured against it.
3. **Optional working partition** (`Partition2D.cpp`). Off by default (`ADAP_NATIVE_REPARTITION= NO`). A dual graph
   is built through hash owners and partitioned with ParMETIS on predicted work weights; whole cells migrate.
   `ADAP_NATIVE_RANKS < size` moves all cells to the first ranks, which then work on a split communicator.
4. **Engine** (`Engine2D`). There is no mesh data structure with adjacency. State per rank: `std::map<Id,Cell> owned`,
   an immutable copy of the input cells as metric donor (`DonorField`, with a local ADT), and a distributed hash table
   node id -> incident (cell, version, owner) (`Directory`, `Distributed2D:250-486`, rank = hash(id) mod P).
   Work is a sequence of collective **rounds**. In one round every rank proposes at most one cavity operation
   (`Engine2D:84-86`). The round: look up the stars of the seed nodes in the directory, fetch the cells from their
   owners, reserve all cavity nodes (lowest rank wins a conflict), import the donor cells that cover the cavity,
   allocate fresh ids, rebuild the cavity privately (`Boundary2D::reconstruct` -> `Mesh2D`), exchange perimeter
   certificates, let every participant validate, vote, then publish erase/add on all owners and update the directory.
   Operators: surface HEIGHT / SPLIT / REMOVE / REDISTRIBUTE (re-triangulate the cavity polygon, carve first-layer
   wall triangles at the prescribed height) and bulk REMOVE / SPLIT / FLIP / MOVE, plus a "joint" two-point repair.
   `adapt()` runs 6 sweeps of the 8 phases; a phase is at most `phase_rounds` rounds (`Engine2D:618-681`).
5. **Metric**. The nodal sensor tensor is frozen on the input mesh. Every query interpolates it P1 (linear in M) on
   the imported donor cells, then applies an optional point-wise composition (BL wall constraints,
   `CSinglezoneDriver.cpp:304-330`). Cell quality uses one centroid sample, edge length a 3-point Simpson rule.
6. **Acceptance** (`Remesher.cpp:499-526`). Fixed contract: quality >= 0.18, metric edge length <= 1.8, reference
   deviation <= ADAP_HAUSD, wall height within 1e-8. Otherwise the status is INCOMPLETE.
7. **Output** (`Import2D:185-239`, `CReaderSlices.cpp:303-647`). Each rank builds reader slices from its cells with a
   distributed dense renumbering. The volume mesh is never gathered. Solution transfer is the existing driver code
   (`CSolutionTransfer`), not part of the native kernel.

---

### 2. Correctness findings

Each entry: location, scenario, severity, confidence.

**C1. The work allowance caps one remesh call at about 120 000 bulk insertions per rank.** Critical. Confirmed.
`Remesher.cpp:421-422`: `phase_rounds = min(20000, max(300, ceil(max(globalCells, estimatedCells)/(4*ranks))))`.
`Engine2D:60`: `sweeps = 6` (never set by the remesher). `Engine2D:618-635`: a phase runs at most `phase_rounds`
rounds, and a rank commits at most one cavity per round, successful or not. So a rank can do at most 6 x 20000 bulk
splits per call (+2 cells each). Example: 50k cells to a 1e6-cell target on 4 ranks needs about 475k splits; the
ceiling is 480k only if every round commits on every rank with no conflict, rejection or imbalance. A 1e7-cell target
needs more than 40 perfectly balanced ranks. Below the 20000 cap the formula still gives only a factor 3 margin and
assumes operations are spread evenly over ranks: a rank that needs more than about 3/P of all insertions (a shock in
one partition, repartition off by default) runs out of rounds. The result is INCOMPLETE_QUALITY, which is fatal (C2).

**C2. An incomplete result terminates the CFD run.** High. Confirmed.
`SU2_CFD/src/drivers/CSinglezoneDriver.cpp:1676-1680`: any status other than COMPLETE goes to `CollectiveFailure`,
which calls `SU2_MPI::Error` (`CDistributedSearch.cpp:460-463`). All callers (`:444`, `:1005`, `:1435-1442`, `:1861`)
go through this. One residual cell with q < 0.18 or one edge with L > 1.8 after the bounded work ends the simulation.
The thresholds are hard-coded (`Engine2D:646-647`, `Remesher.cpp:506`) and there is no relaxation, retry with more
rounds, or "keep the old mesh and continue". NATIVE_ADAPTATION.md says the candidate is "rejected before replacing
the accepted CFD mesh"; the code then stops the run.

**C3. With ADAP_SURFACE= NO a metric finer than the boundary can never be satisfied.** Medium. Confirmed.
`Engine2D:193`: `fixed_boundary` skips all surface SPLIT/REMOVE/REDISTRIBUTE candidates. `Engine2D:177` still
proposes every boundary edge longer than 1.6 as BULK_SPLIT; `Mesh2D:482-485` rejects it ("not a complete interior
edge star") because a boundary edge has one cell. Each such edge costs one full collective round per sweep, and
because candidates are ordered by score the longest boundary edges are tried first. `Engine2D:646-647` and
`Remesher.cpp:504-508` count the edge as a residual, so the call ends INCOMPLETE and the run aborts (C2).

**C4. A cavity may overlap at most 256 input cells.** Medium. Mechanism confirmed, impact plausible.
`Field2D:21` (`FIELD_LIMIT = 256`), `Field2D:375-379` and `:389-392`: if the cavity overlaps more donors the
transaction is refused and counted under "memory rejects". Once current cells cover more than roughly 25 input cells
each (about 5x coarser in h) every operation that touches them is refused, including flips, moves and splits needed
to repair shape there. Coarsening therefore saturates per call without a message that says so, and a poor cell in such
a region cannot be repaired, which leads to C2.

**C5. Surface operations next to a first-height wall have no shape floor and delete interior nodes.** Medium.
Mechanism confirmed, consequence plausible. `Boundary2D:284-288` takes only the cavity polygon; `:414-473` carves the
wall triangles and ear-clips the rest, so every interior node of the enlarged cavity (`Engine2D:701-724`) disappears.
`Boundary2D:507-513`: `shapeFloor = 0` when a layer is built and the action is not a coarsening, and the length check
(`:516`) applies to coarsening only. The published cells only need positive area. Repair is left to the bulk phases,
but bulk REMOVE/FLIP refuse any cavity that contains a protected wall cell (`Mesh2D:666-670`), so slivers attached to
the first layer can stay and trigger C2.

**C6. The boundary geometry is the original polyline; refinement cannot recover curvature.** Medium. Confirmed.
`Reference2D:124-137` interpolates linearly between the original boundary nodes; new boundary points
(`Boundary2D:311`, `:356-361`) lie on the original chords. Refining a curved wall adds collinear points. ADAP_HAUSD
only limits how far coarsening may leave the polyline. The sidecar keeps this polyline for all later cycles and
restarts, so the discretisation error of the first mesh is permanent.

**C7. One refused transaction can discard all transactions of the round and blacklist them.** Medium. Confirmed.
The admission flags come from collective maxima (`Distributed2D:152`), so `Engine2D:526-545` sets `approved = false`
on every rank when any rank exceeds its byte budget. The same holds for `failed.any` (`Engine2D:572-576`) and for a
refused directory staging (`Engine2D:581-588`; star cap of 128 at `Distributed2D:295-297`). `Engine2D:633-634` then
inserts each rank's own, valid candidate into `attempted`, so it is not retried until the next sweep. No mesh
corruption, but lost work that counts against the round budget of C1.

**C8. Metric interpolation and quadrature are coarse relative to the composed field.** Low. Plausible.
`Field2D:111-122` blends tensors linearly in M (arithmetic mean). This stays SPD but is biased toward the smaller size
in graded regions; log-Euclidean interpolation is the usual choice. `Mesh2D:115-116` evaluates quality with a single
centroid sample and `Mesh2D:133-145` evaluates length with three samples, while the BL composition is not P1 and can
change sharply inside one edge. A thin band between samples is not seen. The acceptance gates and the final contract
use the same samples, so COMPLETE does not certify conformity to the continuous composed metric.

**C9. Only the predicate unit is compiled with strict arithmetic; the whole 2D kernel is fast-math.** Low. Confirmed.
`Common/src/meson.build:46-56` puts `CNativePredicates2D.cpp` and all 3D kernel sources in the strict library.
`Common/src/adaptation/meson.build` puts `CNativeRemesher.cpp` and `CNativePartition2D.cpp` (which instantiate every
header-inline 2D routine) in the normal library, built with `-ffast-math -fno-finite-math-only` (`meson.build:101`).
I found no bitwise comparison that depends on a recomputed value (shared nodal targets are copied, not recomputed), so
the effect is limited to results that can change with compiler, flags or inlining, and to different code for the same
inline function in the two translation units (`WorkWeights` versus the engine).

**C10. The kernel depends on `long double`.** Low. Confirmed.
41 uses in the 2D headers; areas, qualities, lengths and the orientation filter (`Predicates2D.cpp:124-140`) run in
long double. The filter is written generically and stays valid, but results differ between x87 80-bit, binary128 and
64-bit long double, and on AArch64 Linux long double is software binary128, which makes every predicate and quality
evaluation slow.

**C11. Fault-injection hooks are part of the production kernel and wire format.** Low. Confirmed.
`Boundary2D:57` (`Operation::fault`, serialized at `:61`), `:312`, `:520`, `:572`; `Engine2D:483-489`. No production
caller sets the field.

**C12. `ValidateStaging` calls `area()` before the finiteness test.** Low. Confirmed (not reachable today).
`Engine2D:809-810`: `area()` throws `std::invalid_argument` on non-finite coordinates (`Predicates2D.cpp:125-127`)
and this line is outside the `try` at `:823`. A non-finite remote cell would end in `SU2_MPI::Error`
(`Remesher.cpp:690-693`) instead of a veto. `reconstruct` rejects non-finite cells earlier, so this is a
defence-in-depth ordering error.

**C13. `remeshAttempt` restarts at 1 when the remesher is recreated.** Low. Confirmed for the code path.
`CSinglezoneDriver.cpp:340-343` creates a new `CNativeRemesher` for every `RemeshFromMetric()` (used by `AdaptMesh`,
`:1859`). `Remesher.cpp:649-651` then finds the balance-profile files of the previous call and fails
(`workers.Fail`), and `Remesher.cpp:184-185` refuses to overwrite the rejected-candidate files. Diagnostic modes only.

**C14. `Repartition` takes a `World&` but uses the default communicator.** Low. Confirmed (latent).
`Partition2D.cpp:173` (`MPI_Comm_split(SU2_MPI::GetComm(), ...)`), `:206`, `:255` and every `CollectiveFailure`. The
only production caller passes the default world (`Remesher.cpp:452`), so this is wrong only for a sub-communicator.

Checked and found sound (no finding): orientation and cavity invariants are enforced before every publication
(`Mesh2D:185-235`, `:787-833`); ids are never reused (`Engine2D:689-700`), stale cells are detected by version
(`Distributed2D:440-443`, `Engine2D:805-807`); `ear_clip` cannot index a polygon with fewer than 3 nodes through any
caller; `relax_wall_apices` apex ordering holds because the carved apex is always left of the base; all loops are
bounded; integer widths are checked where they narrow (`Distributed2D:67-76`, `Partition2D.cpp:114-117`);
no uninitialized read found (`Tensor` has no default initializers but every use is value-initialized).

---

### 3. Performance findings

**P1. Every cavity operation costs a full collective round.** High. Confirmed (call count); timings not measured.
Counting calls in `Engine2D:307-616`, `Distributed2D:107-181` and `CPassiveComm.cpp:97-167`: one `exchange` is 4
failure elections (Allreduce), 2 Alltoall, 2 Allreduce and up to one Alltoallv. A bulk round has 16 exchanges plus
about 16 further collectives: about 150 blocking collective calls. A surface round (three dependency-growth passes,
certificate broadcast, collision veto) has 26 exchanges: about 250. Each round commits at most one cavity per rank,
and each exchange allocates P buckets. Throughput is therefore bounded by rounds per second, not by cores; adding
ranks adds commits per round but makes every collective slower, and the same protocol also runs in serial.

**P2. The donor index is switched off above about 29 000 input cells per rank.** High. Confirmed.
`Field2D:341-343` uses the ADT only if `IntersectionQueryBound(owned.size())` fits the dependency budget.
`CTransferMemory.cpp:44-47` with `CTransferMemory.hpp:80` gives 72 bytes per donor cell; the budget is hard-wired to
2 MiB (`Engine2D:58`, not set in `Remesher.cpp`). Above 29 127 cells the code falls back to a linear scan of the
whole donor `std::map` for every active caller (`Field2D:351-354`), in every round. Cost per rank and round:
(active callers) x (owned donor cells), which is the global cell count when all ranks are active. With 1e6 cells on 8
ranks that is about 1e6 map-node visits per rank per round, for up to 20000 rounds per phase. The log line "donor
search candidates/full-scan equivalent" (`Remesher.cpp:615`) shows it at run time.

**P3. Surface selection rescans all owned cells every round and evaluates O(K) reference deviations.** High. Confirmed.
`Engine2D:625-626` uses the uncached `choose` (`:226-235`) for the four surface actions. For SPLIT, `Engine2D:203`
calls `reference.deviation` for every boundary edge; `Reference2D:139-184` makes that two nearest-point searches over
all K knots of the component plus a loop over all knots. Cost per SPLIT round: O(N_owned) + O(N_boundary x K).
`satisfied()` (`Engine2D:640-661`) and the final check (`Remesher.cpp:513`) pay the same once per sweep.

**P4. `CollisionVeto` scans all owned cells once per proposal.** Medium. Confirmed.
`Engine2D:759-788`: for every proposal that edits a physical face, each rank loops over its whole `owned` map
(`:779`). Per surface round and rank this is (editing proposals) x N_owned.

**P5. Bulk selection is a 64-entry cache refilled by a full scan.** Medium. Confirmed.
`Engine2D:241-305`: a refill (`:274-281`) visits every owned cell and is needed whenever the cached top falls below
the best discarded candidate, about once per 64 consumed candidates. Total selection work is O(ops x N_owned / 64):
quadratic for a fixed rank count, and dominant in serial at 1e6 cells. A priority queue keyed by edge would remove it.

**P6. REDISTRIBUTE visits every non-feature boundary node in every sweep.** Medium. Confirmed.
`Engine2D:218-221` gives every such node the constant score 0.1, and `:631-634` marks it attempted only after its
round. One surface round (P1, P3) per boundary node per sweep, needed or not, up to `phase_rounds`.

**P7. The private kernel rebuilds ordered containers and copies cavities for every trial.** Medium. Confirmed.
`nodes()` and `boundary()` build `std::map`s on each call (`Mesh2D:157-184`); `validate_replacement` calls them three
times (`:210-211`). `MoveStar` copies the star and validates for each of up to about 150 trial positions
(`Mesh2D:347-401`). `strict_cells` is O(E^2) with map lookups in the inner loop (`Mesh2D:817-831`).
`relax_wall_apices` copies the whole cavity and runs `strict_cells` per trial (`Boundary2D:183-198`).
`SplitSeed` evaluates about 1480 sample points (`Mesh2D:950-954`) and `JointPatch` nests 3 x 4 x 2 starts, 13 step
sizes, 8 sweeps, all movable nodes and 9 directions, rebuilding `nodes(state)` per node (`Mesh2D:988-1107`). Every
metric evaluation goes through `std::function` and a hash lookup. This is a large constant per operation, not a
complexity problem, except for the joint repair.

**P8. Memory is about 0.9 kB per triangle in steady state, more during setup.** Medium. Confirmed by struct sizes.
`Cell` is 248 bytes because node coordinates are stored by value in every incident cell (`Boundary2D:36-47`); in a
`std::map` about 300 bytes. The donor snapshot adds about 250 bytes (`Field2D:467-472`) and the directory about 320
bytes per cell (`Distributed2D:252`, one map per node, one node per incidence). Setup holds extra full copies
(`Engine2D:113-129`), and migration holds the input, the send buckets, the received records and the replacement map
at once (`Partition2D.cpp:213-254`). 1e7 cells need about 9 GB plus transients, roughly ten times a compact array mesh.

**P9. Physical-graph validation is O(B^2) in the worst case.** Low. Confirmed.
`GeomVal2D:46-66`: the x-sorted sweep degenerates when many faces share an x-range (a long vertical wall). It runs on
every rank at first import and on sidecar load. The source comment states the bound.

---

### 4. MPI readiness findings

What is distributed: the volume mesh, the node-to-cell directory, the donor field, cavity import across ranks,
ownership of new cells, the output slices. Partition interfaces are not frozen: a cavity may span owners and is
committed atomically on all of them. There are no halos; dependencies are fetched on demand each round.
What is replicated or serial: all physical faces and the reference on every rank (O(B)); output boundary rows and the
sidecar on the master (O(B)); serial METIS only when one rank holds all cells (`Partition2D.cpp:184-190`).

**M1. Per-rank replication of certificates meets a fixed 2 MiB budget.** High. Mechanism confirmed, threshold estimated.
`Engine2D:492-499` sends each surface certificate to all P ranks under the dependency ceiling. The admitted peak
(`Distributed2D:144-150`) holds P copies of the certificate in memory, P packed copies, and everything received from
all active callers: about 232 bytes x rows x (P + active callers). With 40 perimeter rows the 2 MiB budget
(`Engine2D:58`) is reached near P + active = 226, with 100 rows near 90. Because admission is a collective maximum,
all surface transactions are then refused on all ranks (C7), boundary adaptation stops, and the call ends INCOMPLETE.

**M2. Several exchanges are all-to-all by construction.** Medium. Confirmed.
Cavity regions go to every rank (`Field2D:321-323`, source comment: "rank routing/ADT remains deferred"), decisions and
proposals use `metadata` (`Distributed2D:182-186`), physical faces are replicated in `Import2D:121` and `:175`. Traffic
per round grows as P^2 x cavity size, and every rank tests every caller's regions against its donors.

**M3. Reservation conflicts always go to the lowest rank.** Medium. Confirmed.
`Distributed2D:467-479`. A higher rank whose best candidate touches nodes claimed by a lower rank is deferred
(`Engine2D:388-390`), retries the same candidate next round (`:633`), and each deferred round counts against
`phase_rounds`. Ranks with higher numbers make systematically less progress along interfaces (HANDOFF_Codex.md notes
the same bias).

**M4. No load balancing during adaptation; the static one is off by default.** Medium. Confirmed.
`CConfig.cpp:3307` (`ADAP_NATIVE_REPARTITION` default NO). New cells inherit the owner of the parent cell
(`Engine2D:725-742`); nothing migrates while refining. A phase lasts until the busiest rank is done
(`Engine2D:628`), with all other ranks taking part in every collective. The predicted weights are capped at 1024 and
are not fitted to measured cost (`Partition2D.cpp:55-57`).

**M5. Results depend on the rank count and the partition.** Low. Confirmed (by design).
Each rank selects its local best (`Engine2D:226-305`), conflicts are decided by rank number, ids are allocated in
rank order (`Engine2D:689-700`), owners follow the partition. For a fixed rank count and partition I found no source
of run-to-run variation: ordered containers, sorted replies (`Distributed2D:282`, `:456`; `Field2D:327`, `:347`),
unordered maps used for lookup only, no time-dependent decisions. ParMETIS runs with seed 0.

**M6. Rank 0 does a pairwise test of all edited physical faces.** Low. Confirmed.
`Engine2D:792-798`: O((P x edited faces)^2) exact segment tests on rank 0 per surface round. Negligible below about
100 ranks.

**M7. A rank-local exception aborts the job; it does not hang it.** Low. Confirmed.
Collective errors are elected and unwound together (`Distributed2D:91-98`, `Remesher.cpp:686-689`). Any other
`std::exception` on one rank (for example from `choose`, `Owner`, or a reference callback) reaches
`Remesher.cpp:690-693` and calls `SU2_MPI::Error`. I found no path where one rank leaves a collective sequence while
the others continue, so no deadlock, but also no recovery.

**M8. Message sizes and counts are 64-bit safe.** Low (information). Confirmed.
`Distributed2D:117-143` checks byte counts in `size_t`/`uint64_t`; `CPassiveComm.cpp:121-164` splits into rounds with
checked `int` counts. Idle CFD ranks poll an `MPI_Ibarrier` every 1 ms (`Remesher.cpp:699-714`).

Overall: the protocol is functionally distributed and careful about consistency, and the repository evidence is at
1, 2 and 4 ranks on small meshes. For 1e6 to 1e7 elements the limits are C1 (work cap), P1 (one cavity per rank per
round), P2/P3/P5 (linear scans per round) and M1/M2 (per-rank replication), in that order.

---

### 5. Strengths

- Orientation is exact: a long-double filter with a correct error bound and an exact fixed-size integer fallback
  without heap allocation (`Predicates2D.cpp:26-140`), compiled strictly, tested at denormal and maximum magnitudes.
  Segment, containment and simplicity tests are built on it without tolerances.
- Transactional cavities: private reconstruction on copies, then area, perimeter-identity, orientation, manifoldness,
  duplicate, crossing and coincident-node checks before anything is published; participants re-validate certificates.
- Versions and monotone 64-bit ids: no index reuse, no dead-entity states, stale data is detected.
- Publication after the final vote uses node transfers and swaps that cannot allocate (`Engine2D:589-598`,
  `Distributed2D:340-355`).
- Collective failure election on every exchange; a working sub-communicator failure returns to the CFD communicator
  before the fatal protocol.
- Wire format is scalar by scalar, without struct padding, with range checks on decode.
- The metric is frozen for the call; shared nodal targets are kept bitwise identical by copying authoritative samples
  (`Engine2D:407-416`), and the output checks it (`Import2D:196-214`).
- Every loop has an explicit bound; byte budgets are checked before allocation.
- No complete volume mesh is gathered at any stage, including output.
- Extensive counters and timers are reported per phase and per rank.

---

### 6. Not verifiable statically

- Wall-clock cost of a round, and therefore the real throughput and the rank count at which it saturates (P1, M1).
- Whether q >= 0.18 and L <= 1.8 are reachable on realistic cases within the round budget (C1 to C5); the source only
  shows that failure is fatal and that the work is bounded.
- How often the donor cap (C4) and the collective refusals (C7) occur in practice.
- Behaviour of `-ffast-math` code generation on the inline kernel (C9), and of the filter on non-x87 long double (C10).
- Determinism of ParMETIS for a fixed seed and rank count; correctness of `CADTElemClass` for boxes encoded as LINE
  elements (`Field2D:437-466`); `SendRounds`/`RecvRounds` used by the diagnostic writer (`Remesher.cpp:193-202`).
- SPD-ness and continuity of the BL composition (`CBoundaryLayerMetric::ApplyPoint`); only its ADT lookup was checked.
- The CFD solution transfer and geometry rebuild after the remesh (driver code, outside this scope).
- Unit tests were not run. Coverage gaps seen by skimming: no test above toy size (the scaling test is hidden,
  `[.]`), none for the 29k donor-index threshold, the work cap, the INCOMPLETE path in the driver, rank counts above
  4, or reproducibility across rank counts.

---

### 7. Finding count

| Severity | Count | Findings |
| --- | --- | --- |
| Critical | 1 | C1 |
| High | 5 | C2, P1, P2, P3, M1 |
| Medium | 13 | C3, C4, C5, C6, C7, P4, P5, P6, P7, P8, M2, M3, M4 |
| Low | 12 | C8, C9, C10, C11, C12, C13, C14, P9, M5, M6, M7, M8 |

---

# Part III — 3D full report


Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch `codex/native-3d-transport-accounting`, HEAD `7746fd53fa`.
`git diff HEAD --stat --ignore-submodules -- Common SU2_CFD` is empty at the start and at the end of the assessment: tracked sources are clean.
Nothing was built, run or modified. All statements come from reading the code.

**Read in full** (`Common/include/adaptation`, `Common/src/adaptation`): CNativeMesh3D.hpp, CNativePredicates3D.cpp, CNativeTopology3D, CNativeCavity3D, CNativeBoundary3D, CNativeMetric3D, CNativeField3D, CNativeDonor3D, CNativeDistributed3D, CNativePartition3D, CNativeImport3D, CNativeReferenceIO3D, CNativeEngine3D, CNativeRemesher3D, CNativeRuntimeProfile3D.hpp, CRemesher.hpp, CSimplexMesh.hpp, and CNativeDistributed2D.hpp (it defines the `World` transport that 3D reuses).
There is no CNativeMesh3D.cpp and no CNativePredicates3D.hpp: the predicates are declared in CNativeMesh3D.hpp and implemented in CNativePredicates3D.cpp.
**Read as excerpts only**: CPassiveComm (hpp, AlltoallvRounds), CDistributedSearch (ElectFailure, OwnedSimplices), CADTElemClass::DetermineIntersectingElements, mpi_structure.cpp (Error), CSinglezoneDriver.cpp (MakeRemesher, ReplaceMesh, call sites), meson.build flags, the three design documents, test case titles.
**Not read**: CReaderSlices, CSolutionTransfer, CMeshGather, CTransferMemory.cpp, CRankBoxTree/CPointDirectory internals.

Severity scale: Critical = blocks use beyond toy meshes or aborts the run; High = wrong outcome or loss of the event in realistic cases, or a cost that prevents scaling; Medium = significant limit or waste; Low = minor.
Confidence: "Confirmed" = the code path was followed through its callers; "Plausible" = the mechanism is in the code but its frequency or cost is my estimate.

Findings per severity: **Critical 3, High 10, Medium 10, Low 10** (33 in total).

---

### 1. Architecture summary

**Data structure.** There is no classical tetrahedral mesh structure. The working mesh is `Owned = std::map<Id, OwnedCell>` (CNativeDistributed3D.hpp:26). Every cell stores its four vertices by value (id and coordinates, CNativeMesh3D.hpp:29-44), a version and four optional boundary bindings. There is no stored adjacency: local adjacency is rebuilt for each cavity by sorting (`Incidence`, CNativeTopology3D.cpp:44-116), and global vertex stars live in a hash-distributed `Directory` (`std::map<Key, std::map<Id, Entry>>`, placed on rank `hash(vertex) % P`, CNativeDistributed3D.cpp:73-81). Identifiers are 64-bit, never reused, and grow sparsely (`maximum + 1 + round*P + rank`, CNativeEngine3D.cpp:542-543, 558).

**Data flow of one adaptation event** (`CNativeRemesher3D::Remesh`, CNativeRemesher3D.cpp:86-442):

1. `CheckSupport`: tetrahedra only, triangular boundary faces, no periodic/multizone/moving mesh/adjoint, `ADAP_SURFACE= YES` required, any `ADAP_BL` rejected, `ADAP_HESSIAN_NOISE= 0` (lines 29-84).
2. Reference: load `<mesh>.native_ref` or build the immutable `PlanarReference` from the original boundary triangles (replicated on every rank).
3. `ImportGeometry`: owned tetrahedra into `Owned`, a frozen copy `original` (`DonorCell` = cell plus four nodal metric tensors), face rendezvous by hash to bind boundary faces to reference facets.
4. `DonorImport::Build` (ADT and rank boxes over the original cells), `WorkingWeights`, optional ParMETIS `Repartition` to M <= N worker ranks, donor migration, `MPI_Comm_split`.
5. `Refine`, then `Coarsen` (same function `Adapt`, CNativeEngine3D.cpp:289-911) on the worker communicator. Each builds its own `Directory` and `DonorImport`, measures every cell ("seed"), then runs globally synchronous rounds. In one round each rank proposes at most one cavity: Lookup (complete edge/vertex star) -> Fetch (remote cells) -> Reserve (vertex claims, lowest rank wins) -> private operator (SplitEdge / InsertEdgeCavity / CollapseVertex / MoveVertex / Reconnect, CNativeBoundary3D.cpp) -> donor import -> metric gate (CNativeMetric3D.cpp) -> routing -> atomic `Publish` -> re-measure the new cells.
6. `ReaderSlices`: nodal metric by P1 query of the frozen source, renumbering through `CReaderSlices::FromDistributed`. The driver then rebuilds the geometry and transfers the solution with the shared `CSolutionTransfer`.

**Metric.** The target is the *original* nodal tensor field, linearly interpolated inside original tetrahedra (`FrozenField`). Edge length is the exact piecewise integral of sqrt(e^T M e) along the edge through the original cells (CNativeField3D.cpp:127-182). Gates: edge <= 1.8, collapse candidates < 0.9, mean ratio >= 0.20, scaled Jacobian >= 0.05 (CNativeMetric3D.hpp:12, CNativeEngine3D.hpp:11, CNativeMesh3D.hpp:82-83).

**What "donor" and "transport accounting" mean here.** "Donor" is the frozen metric source, not the CFD solution. No solution transfer or conservation logic exists in the 3D native files; that is in `CSolutionTransfer` (not assessed). "Transport accounting" is MPI byte/call/time bookkeeping (`World::bytes_sent`, `collective_calls`, `collective_seconds`).

**Acceptance model.** All-or-nothing: an event is accepted only if every cell passes every gate and, for coarsening, the candidate queue is empty. Any budget refusal rejects the whole event.

---

### 2. Correctness findings

#### C1 — Critical, Confirmed: the original boundary is limited to 512 triangles and to one closed sphere
`CNativeImport3D.cpp:361-367` calls `ValidateSurfaceTopology(skin, true)` on *all* boundary facets of the first mesh. `CNativeCavity3D.cpp:130-133` throws when `triangles.size() > 4 * MaximumReplacementCells` (4 x 128 = 512, CNativeCavity3D.hpp:13). The same check runs when the sidecar is decoded (`CNativeReferenceIO3D.cpp:88`). With `closed = true`, `Surface()` (CNativeCavity3D.cpp:55-87) also requires one connected component with Euler characteristic 2.
Failure scenario: any mesh with more than 512 boundary triangles, or with two boundary components (body inside a far-field box), or with a handle, ends in `CollectiveFailure` (fatal) at the first import (CNativeImport3D.cpp:371).
The largest case in the project notes has exactly 512 boundary triangles (MESH_ADAPTATION_FINDINGS.md:1305), which matches this cap. This alone rules out 1e6-1e7 tetrahedra and external aerodynamics.

#### C2 — Critical, Confirmed: a rejected event aborts the CFD run
`Remesh` returns `INCOMPLETE_QUALITY` with empty slices on any refusal (CNativeRemesher3D.cpp:90, 291-313) and prints "accepted CFD mesh/solution retained". The driver passes the result straight to `ReplaceMesh`, which calls `CollectiveFailure` -> `SU2_MPI::Error` when the status is not `COMPLETE` (CSinglezoneDriver.cpp:1676-1680; call sites 432/444, 997/1005, 1504 with 1435-1442).
So the "atomic rejection" protects in-memory state only for unit tests. In a run, each of the following stops the job: C3, C4, C5, C7, C8, M1, a memory-ceiling refusal (P6), or a metric evaluation failure in a single cell (`Selection::Add` throws on an incomplete measurement, CNativeEngine3D.cpp:226-227, 442-449).

#### C3 — Critical, Confirmed: hard limit of 512 rounds per pass, one attempt per rank per round
`EngineOptions::maximum_rounds = 512` (CNativeEngine3D.hpp:13). The driver constructs `CNativeRemesher3D(nativeReference3D)` with default options (CSinglezoneDriver.cpp:299); there is no config option (only `ADAP_NATIVE_RANKS` and `ADAP_NATIVE_REPARTITION`, CConfig.cpp:3307-3308). Each round holds one `Choice` per rank (CNativeEngine3D.cpp:458-470), and rejected proposals also consume the round. At the end of the loop the result is invalid unless everything is complete (lines 897-909).
Consequence: at most 512 x P attempted edits in `Refine` and again in `Coarsen`; in serial at most 512. A mesh that needs more is rejected, and by C2 the run aborts.
Corroboration from the project notes: the 1536-tetrahedron wedge fails at the default budget (MESH_ADAPTATION_FINDINGS.md:868-873).

#### C4 — High, Confirmed: a valid mesh is rejected when coarsening does not finish
`incomplete == 1` ("valid cached metric cells and pending collapse candidates") still sets `result.valid = false` (CNativeEngine3D.cpp:899-903). Commit c407d8b9a4 changed only the message. In `Remesh` the coarsening result replaces `valid` (CNativeRemesher3D.cpp:247-252), so a completed refinement is discarded as well.
Scenario: refinement succeeds, all cells pass all gates, 91 optional collapse directions remain at round 512 -> event rejected -> abort. The notes describe exactly this case.
Coarsening is optional by the engine's own contract ("need not reach a minimal mesh", CNativeEngine3D.hpp:73-78), so this refusal is not needed for mesh validity.

#### C5 — High, Confirmed mechanism (frequency Plausible): a star with more than 64 cells rejects the whole event
`Directory::Lookup` returns status 3 when the matches exceed `MaximumCavityCells = 64` (CNativeDistributed3D.cpp:274-289); the caller throws (lines 305-310) and the vote fails on all ranks. The engine then returns instead of blocking that candidate (`CNativeEngine3D.cpp:525-527`).
Scenario: the shortest collapse candidate removes a vertex with 65 incident tetrahedra, or the repair queue contains such a vertex. Transition vertices next to bisected regions gain two cells per split of an opposite edge, so valence above 64 is realistic. `FilterCollapse` (lines 140-170) scans the local star but does not test its size.

#### C6 — High, Confirmed mechanism: new boundary points must be exactly on the edge or exactly coplanar; no reprojection, no curved surfaces
The split point is the rounded midpoint (CNativeEngine3D.cpp:558-559). `SplitEdge` needs exact collinearity (`OnEdge`, CNativeBoundary3D.cpp:34-39, 364-367). The fallback `InsertEdgeCavity` needs exact coplanarity with the reference facet (`PlanarReference::Contains`, lines 319-322, used at 449) and refuses an edge between two different facets (lines 439-440).
Consequences:
- Axis-aligned planes and axis-aligned ridges work with any coordinates.
- On an oblique plane a rounded midpoint is not exactly coplanar unless the coordinates are dyadic. The wedge cases in the notes use dyadic coordinates (MESH_ADAPTATION_FINDINGS.md:1305).
- On a faceted curved surface every original boundary edge is a ridge between two facets, so only the exact `SplitEdge` path remains.
A boundary edge that is too long and cannot be split leaves an invalid cell; the event stagnates (lines 509-513) and is rejected. The documents place curved geometry in "Goal 2, not started", so this is a declared scope limit, but the code does not detect it up front.

#### C7 — High, gate Confirmed, thresholds Plausible: the strict shape gate on every intermediate edit blocks multi-level anisotropic refinement
`MeasureMetricCells(fresh, ..., require_shape = true)` throws when any new cell has mean ratio < 0.20 or scaled Jacobian < 0.05 in the target metric (CNativeMetric3D.cpp:64, 101-109, 189-190). Old cells may be worse; new ones may not.
With the formula in `Measure` (CNativePredicates3D.cpp:482-486) a regular tetrahedron under a metric stretched by a factor s has q = 3 s^(2/3) / (2 + s^2): 0.42 at s = 4, 0.21 at s = 7, 0.18 at s = 8, 0.07 at s = 16. One bisection cannot lift q from 0.07 to 0.20.
Scenario: an isotropic mesh with a target anisotropy mismatch above roughly 7-8 (three or more bisection levels in one direction). The first splits are rejected, the repair fallback passes through the same `Shape` check (CNativeMetric3D.cpp:154, 189-190), the edge is blocked, and the event stagnates (CNativeEngine3D.cpp:509-513). The reported tests use aspect ratios 1.5 to 2.

#### C8 — High, Plausible: the 256-donor cap per cell limits the coarsening ratio per event
One cell's metric queries (centroid and six closed edges) must be covered by at most `MaximumDonors = 256` original tetrahedra (CNativeDonor3D.cpp:344, 365; CNativeField3D.cpp:35; CNativePredicates3D.cpp:334). When the one-cell import still exceeds the cap, the event is rejected (CNativeEngine3D.cpp:414-421, 687).
The exact filter keeps every donor that *touches* a segment, including all tetrahedra around its end vertices and around every original vertex it passes through. Estimate for a Kuhn-type source (24 cells per vertex): an edge spanning k original edges touches about 24 + 16k cells; six edges of a cell with k = 2 give about 250, with k = 4 about 350. So coarsening by more than about a factor 2 relative to the source mesh in one event is expected to hit the cap. Not measured.

#### C9 — Medium, Confirmed: boundary coarsening is limited to convex coplanar same-marker patches
Coplanar facets are merged only when the component is a disk with a convex border (CNativeBoundary3D.cpp:153-228). A non-convex planar region, or one with a hole (symmetry plane cut by a body), keeps one facet per original triangle ("retain raw constraints", line 224). The flood fill marks all its facets as seen, so no convex sub-patches are tried.
Then `CollapseVertex` needs the retained vertex inside every facet incident to the removed one (lines 585-594), which fails for original boundary vertices, and `InsertEdgeCavity` refuses edges between two raw facets (439-440). Corners and marker junctions are preserved correctly by the same rule.

#### C10 — Medium, Confirmed: operator set and features are narrower than the task list assumes
- No in-sphere predicate, no Delaunay kernel, no cavity enlargement. Insertion is a cone over the fixed edge star (`Cone`, CNativeCavity3D.cpp:163-190).
- Swaps exist only as "cone to an existing vertex" (`Reconnect`, CNativeBoundary3D.cpp:726-744), and smoothing only as a move toward the neighbour centroid with steps 1, 0.5, 0.25 (CNativeEngine3D.cpp:606-632). Both run only as repair after a rejection, not as an optimisation pass.
- No gradation in these files; no metric repair (a non-SPD nodal tensor is fatal at import, CNativeImport3D.cpp:207-217).
- No boundary-layer or frozen regions: `ADAP_BL` and `ADAP_SURFACE= NO` are rejected (CNativeRemesher3D.cpp:77-80).
- Tensor interpolation is component-wise linear, not log-Euclidean. It is self-consistent with the exact edge integral.

#### C11 — Low, Plausible: the rounded centroid of a boundary sliver can fall outside the source
`Centroid` rounds to binary64 (CNativePredicates3D.cpp:287-295); `FrozenField::Query` uses exact closed containment and throws "Query outside" (CNativeField3D.cpp:230). In the seed or update this becomes an event rejection (CNativeEngine3D.cpp:448).

#### C12 — Low, Confirmed: `UndoEdgeSplit` is dead in production
CNativeBoundary3D.cpp:484-573 has no caller outside the unit tests.

#### C13 — Low, Plausible: the floating-point filter is compiled with `-ffast-math`
meson.build:101 adds `-ffast-math -fno-finite-math-only`. `SegmentMayIntersectBox` protects its rounding points with `volatile` (CNativeDonor3D.cpp:72-76); the main filter in `Determinant` (CNativePredicates3D.cpp:136-156) does not. Any regrouping of six triple products stays far inside the 64-epsilon bound by my analysis, and the exact path is integer-only, so I expect no wrong sign. It is not proven for every compiler transformation.

#### C14 — Low, Confirmed: documentation is partly stale
NATIVE_ADAPTATION.md:34 and :211 say that 3D is rejected / unsupported, while the driver routes 3D to `CNativeRemesher3D` (CSinglezoneDriver.cpp:289, 298-299). HANDOFF_Codex.md is consistent with the code on the points I checked (rotating policy "not promoted", default-512 failure, Goal 2 not started).

#### Checked and found correct (no finding)
- Exact arithmetic: limb budgets (66/132/198/396 limbs) cover the worst case; `Product` carry placement, `Difference` sign handling and the magnitude-only `CompareRoot` (all roots are non-negative) are right.
- Cone, collapse and move: strict positive orientation of every new cell against an unchanged embedded skin implies that the apex is in the kernel, so the new cells tile the cavity. External edge/face collisions are excluded geometrically.
- `SegmentMayIntersectBox` and `Outside` are conservative (they only discard certain misses).
- The edge integral formula (2/3) L (1 + r + r^2)/(1 + r) is the correct integral of the square root of an affine function.
- Query bit masks between sender and owner match (CNativeDonor3D.cpp:277-287, 347-352).
- Direction indices of the collapse filter are consistent between `Queue`, `Forbidden` and `Touch`.
- Loops are bounded everywhere; no index reuse; 64-bit id overflow is guarded (CNativeEngine3D.cpp:549-551).

---

### 3. Performance findings

#### P1 — High, Confirmed structure: one cavity per rank per global round, about 165-180 MPI collectives per round
Counting the code path of one refinement round with one measure batch: 47 votes/reductions plus 14 `World::exchange` calls. Each exchange is four failure elections, one `MPI_Alltoall` of counts, one reduction, and `AlltoallvRounds` with a second `MPI_Alltoall`, one reduction and at most one `MPI_Alltoallv` (CNativeDistributed2D.hpp:108-181, CPassiveComm.cpp:97-165). Total about 131 `MPI_Allreduce`, 28 `MPI_Alltoall`, up to 14 `MPI_Alltoallv`. Coarsening adds the change notification.
The yield is at most one accepted cavity per rank. Idle ranks take part in every collective. The rounds needed equal the edit count of the busiest rank, not total/P.
Project-reported rates for small cases are 26 to 183 edits/s (HANDOFF_Codex.md:1557, 2630). At 75 edits/s, 1e6 edits take about 3.7 hours before any scaling loss.

#### P2 — High, Confirmed: full-mesh passes are repeated several times per event
- Two complete seeds: `Refine` and `Coarsen` each build a new `Selection` and re-measure every cell, including exact integration of every edge (CNativeEngine3D.cpp:301, 454-456; called at CNativeRemesher3D.cpp:238, 249).
- `DonorImport::Build` runs four times (CNativeRemesher3D.cpp:176-177; CNativeEngine3D.cpp:392 twice; CNativeImport3D.cpp:529-531). Each sends six authority records per original cell through a hash exchange and checks them in a `std::map` (CNativeDonor3D.cpp:163-211).
- `Directory::Build` runs two or three times (CNativeEngine3D.cpp:389 twice; CNativePartition3D.cpp:147-149).
- `WorkingWeights` always runs, also when no repartition is requested and the weights are then unused (CNativeRemesher3D.cpp:181-190): five point queries per cell and N/8 collective import batches.
- The mesh map is deep-copied at the start of each pass (`working = accepted`, CNativeEngine3D.cpp:374).

#### P3 — High, mechanism Confirmed, cost Plausible: every metric edge is traced with exact rational arithmetic
`SensorEdgeLength` -> `TraceSegment` (CNativePredicates3D.cpp:333-387). The only floating-point shortcut is the strict separation test (lines 345-353). Every donor that touches the segment goes to `ClipInterval` (lines 228-260), which computes the exact volume and up to eight more exact 3x3 determinants on up to 6300-bit integers, then root comparisons with 396-limb products.
For an edge of the unchanged mesh about 40-50 source cells touch the segment (all cells around both end vertices), most of them only to be discarded as zero-length. My estimate is 100-400 exact determinants and 0.1-1 ms per edge; not measured. A mesh has about 1.2 edges per tetrahedron and they are all traced in each seed.
`ValidateTensor` (two filtered determinants) also runs inside every `MetricNorm` call (line 399), twice per piece.

#### P4 — Medium, Confirmed: the same edges are integrated two to three times per accepted edit
- The measurement from validation is discarded (`PatchMetric metric` local at CNativeEngine3D.cpp:598). The update re-measures the new cells with reuse limited to cached and retired edges (lines 435-442), so every new edge is traced again.
- `MeasureMetricCells(old, target, stats, &measured.edges)` (CNativeMetric3D.cpp:191) reuses only edges of the fresh patch. Edges that exist only in the old cavity (the split edge, all spokes of a collapsed vertex) are traced again although the caller holds them in `frozen_reuse`.

#### P5 — Medium, Confirmed: node-based containers and repeated local rebuilds in the hot path
- `std::map` for the mesh, the cell cache, the edge cache and vertex stars; `std::set` for the queue (CNativeEngine3D.cpp:83-90). Every access is O(log N) with poor locality.
- One proposal builds `Incidence` three to six times (operator, `ValidatePrescribedInterface`, `Validate`), each with an orientation test of all cells, and `Ball` builds a map of graphs per vertex twice (CNativeCavity3D.cpp:88-103, 108-127).
- `Directory::Prepare` copies the whole star map of every changed vertex before each publication (CNativeDistributed3D.cpp:152-155).
- `GeometryPatch`, `Nodes` and `SkinMap` are rebuilt for every repair trial (up to 19 per cavity).
No O(N^2) stage and no per-operation whole-mesh validation was found. The cost is in constants and in the round structure.

#### P6 — Medium, formulas Confirmed, numbers estimated: memory footprint and the fixed 2 GiB ceiling
Resident storage is about 2.5-3 kB per tetrahedron: two copies of the 248-byte map node, selection caches of about 0.5-0.9 kB, the directory share of about 0.7 kB (five map entries per cell), the donor copy of 328 bytes plus boxes and ADT, and a second donor copy after repartition.
Admission charges 2048 to 6144 bytes per cell for the selection alone (`selection_bound`, CNativeEngine3D.cpp:306, 371-372, 425). With `memory_ceiling = 2 GiB` (CNativeEngine3D.hpp:13, not configurable from the driver) I estimate a limit of 0.3-0.5 million tetrahedra per rank. Beyond it the event is refused collectively, and by C2 the run aborts.

#### P7 — Medium, Confirmed: metric queries are served in collective batches of at most 16 cells
The seed loop (CNativeEngine3D.cpp:397-453) and `Samples` (CNativeImport3D.cpp:414-465) perform, per batch, a collective donor import (seven votes, two exchanges) and build a new `FrozenField` with a new `CADTElemClass` (CNativeField3D.cpp:61-111). For N/P = 1e5 cells this is about 6000 batches per seed, twice per event, plus the weight and return passes.

#### P8 — Low, Confirmed: `Repartition` builds a full `Directory` only as a validation
CNativePartition3D.cpp:147-155; the dual graph comes from a separate face rendezvous.

#### P9 — Low, Confirmed: the planar reference is map-based and replicated
Five `std::map` members (CNativeBoundary3D.hpp:55-59), a convexity loop over border edges times vertices per component (CNativeBoundary3D.cpp:219-223), and an admission model of 4096 bytes per facet (line 269). Not relevant under the 512-triangle cap of C1, but it becomes relevant when that cap is lifted.

---

### 4. MPI readiness findings

**What is distributed.** Import, donor search, directory, engine, partition and return all run distributed; no rank gathers the volume mesh. Not distributed: the boundary reference (replicated on every rank, M6), the sidecar I/O (rank 0), and the boundary faces of the returned mesh (gathered on the master, M10).
**Interfaces.** Partition interfaces are not frozen. A cavity may span ranks; its cells are fetched from their owners and replaced atomically. There are no halo cells. Ownership does not migrate during a pass.

#### M1 — High, Confirmed by reading, not reproduced: single-cavity budgets are applied to the per-rank sum over all concurrent cavities
`Directory::Publish` refuses when `removed.size() > 64` or `added.size() > 128` (CNativeDistributed3D.cpp:504-506). In the engine these vectors are everything routed to this rank from *all* proposers of the round (`world.exchange(removals)`, `world.exchange(additions)`, CNativeEngine3D.cpp:849-851). The same holds for `retired_edges.size() > 6 * 64` (lines 870-874), which is checked after the publication has been committed.
Scenario: in a coarsening round rank X collapses a vertex with 24 cells, and four neighbour ranks each collapse an interface vertex whose star holds 12 cells of X: 72 > 64 -> publication refused -> event rejected (C2).
The probability grows with the number of neighbour ranks. The tests stop at four ranks.

#### M2 — High, Confirmed: every exchange is dense in P
`World::exchange` takes `std::vector<std::vector<T>>(P)` buckets and performs two `MPI_Alltoall` calls of P counts and five to six reductions even when nothing is sent (CNativeDistributed2D.hpp:108-181; CPassiveComm.cpp:97-125). Callers also allocate P-sized arrays per call (`selected`, `payload`, `to`, `reply`; e.g. CNativeDonor3D.cpp:250-255, 309, 354).
With about 14 exchanges per round the work is O(P) per rank and O(P^2) in total per round, independent of activity. There is no neighbour-only or sparse (NBX-type) exchange. Acceptable at 4-64 ranks, a hard wall at hundreds to thousands.

#### M3 — Medium, Confirmed: the reservation gives fixed priority to the lowest rank; "rotating reservations" are not in the production code
`Directory::Reserve` takes `std::min(old->second, row.caller)` (CNativeDistributed3D.cpp:428-434). The rotating policy of commits 2c20503dce to 1bfb136dbd exists only as `integration_evidence/native_3d_core_v1/rotating_reservation_*/directory_rotating.cpp`; its difference to production is a `first_rank = (stats.reservations - 1) % size` rotation carried in the claim. No file under `Common/` changed in those commits.
Effects in production:
- A higher rank loses every interface conflict. It keeps its candidate and retries next round, so there is no livelock, but each lost round is taken from the shared 512 budget (C3).
- Vetoes are transitive: a caller that loses a vertex to another caller is refused even if that other caller is itself vetoed (line 434).
- The lowest active rank can never be vetoed, so at least one proposal proceeds per round.
Note on the probe: the rotation is keyed on a statistics counter. It works because every rank calls `Reserve` the same number of times, but it couples the protocol to a diagnostic field.

#### M4 — Medium, Confirmed: no load balancing during a pass
New cells go to the owner of their parent (CNativeEngine3D.cpp:815-836). Repartition is optional, runs once before the passes, uses static predicted weights, and is off by default (CNativeRemesher3D.cpp:181). Local refinement (a shock inside one partition) therefore runs at serial speed with parallel overhead, and that rank can reach the memory ceiling (P6).

#### M5 — Medium, Confirmed: the hash directory makes every operation remote
The star of a vertex lives on `hash(v) % P`, unrelated to ownership (CNativeDistributed3D.cpp:73-81). A cavity that lies fully inside one rank still needs remote lookup, reservation and publication. There is no local fast path, and the directory memory of a rank does not follow its mesh share.

#### M6 — Medium, Confirmed: the boundary reference is replicated with P copies on the sender, and the sidecar is capped at 1 MiB
`World::metadata` builds `std::vector<std::vector<T>>(size, local)` (CNativeDistributed2D.hpp:183-186), used for the facets at CNativeImport3D.cpp:358 and admitted as `P * count * 3 * sizeof(Facet)` (lines 339-343). `ReferenceFileLimit3D = 1 MiB` (CNativeReferenceIO3D.hpp:11, .cpp:48-49) holds about 4000 text facets; writing beyond it throws on rank 0 and is fatal. Hidden today by C1.

#### M7 — Low, Confirmed: results depend on the rank count
Identifiers contain `round * P + rank` (CNativeEngine3D.cpp:542-543), the reservation depends on the rank, and the edit order depends on the partition. For a fixed rank count and binary I found no source of nondeterminism: ordered containers throughout, the `unordered_map` is a pure lookup cache with FIFO eviction, donor ties are broken by canonical keys, ParMETIS has a fixed seed. The documents state that identity across partitions is not required.

#### M8 — Low, Confirmed: the transport accounting counts only part of the MPI traffic
`collective_calls` and `collective_seconds` are incremented for the count `Alltoall`, for `AlltoallvRounds` (as one call) and for `sum`/`maximum` (CNativeDistributed2D.hpp:133-135, 165-167, 189-195). Not counted: every `ElectFailure` (about 100 of the roughly 170 collectives per round), direct `CPassiveComm::Allreduce` calls in the engine, `LoadReference`, `CReaderSlices::FromDistributed`, ParMETIS. `bytes_sent` includes self-traffic. The code comments say so (CNativeEngine3D.hpp:30, CNativeImport3D.hpp:26); the numbers must not be read as total MPI cost. I found no double counting between the parent, import and worker scopes.

#### M9 — Low, Confirmed: unguarded exceptions between collectives abort the job; no deadlock path found
Examples outside the try blocks: `selection.Best` -> `edges.at` (CNativeEngine3D.cpp:462), bucket allocations (804-806), `reserve` and decoding inside `exchange` (CNativeDistributed2D.hpp:159-179), `.at()` in the donor migration (CNativeRemesher3D.cpp:203-204). They reach `SU2_MPI::Error` (CNativeRemesher3D.cpp:262-263) or `std::terminate`, so the job dies instead of hanging.
I followed every early return in Lookup, Fetch, Reserve, Publish, ImportRegions, Samples, Repartition, LoadReference and the engine loop: each depends on an elected (collective) result or is followed by no further collective. Batch sizes are derived from reduced values. The collective refusal is consistent.

#### M10 — Low, Confirmed (outside the listed files): the master rank gathers all boundary faces of the returned mesh
`CReaderSlices::FromDistributed`, called from `ReaderSlices` (CNativeImport3D.cpp:588), collects the boundary connectivity on `MASTER_NODE` with `GathervRounds` (CReaderSlices.cpp:608-634). This follows the SU2 reader convention. It is a serial memory and time term proportional to the global boundary size, small next to the volume mesh.

#### Checked, no finding: integer ranges
MPI counts go through `CheckedInt` and 1 GiB rounds (CPassiveComm.cpp:86-165); graph sizes are checked against `idx_t` (CNativePartition3D.cpp:143-146); identifiers are 64-bit. No `int` overflow was found in the 3D files.

---

### 5. Mechanisms of the last 20 commits

| Mechanism | Where | Result |
|---|---|---|
| Transport accounting | CNativeDistributed2D.hpp:99-102; CNativeRemesher3D.cpp:316-322 | Consistent but partial (M8). It is not solution-transfer accounting. |
| Donor prefilters: packed per-owner queries, exact cell and segment filters, segment/box separation (7b4e0d767f, 7f76fd9a27, 8c3b40c9be) | CNativeDonor3D.cpp:64-108, 266-289, 317-352 | Conservative and correct by reading; bit masks consistent. They do not remove the exact clipping cost (P3) or the 256 cap (C8). |
| Rotating reservations | only under `integration_evidence/` | Not in production (M3). |
| Coarsening work limits (c407d8b9a4) | CNativeEngine3D.cpp:897-909 | Diagnostic text only; the event is still rejected (C4). |
| Collapse prefilter and re-activation | CNativeEngine3D.cpp:113-170, 200-223 | Correct as a prefilter: a local counterexample is sufficient and the operator remains authoritative. |

---

### 6. Strengths

- Predicates: exact signs through bounded integer arithmetic that compiler floating-point flags cannot change, with a static filter in front; closed containment without tolerances; exact SPD test by Sylvester signs.
- Every edit is built privately and validated before commit: ball topology with manifold vertex links, identical oriented skin, strict positive volumes, exact facet containment for boundary faces.
- Publication is staged: directory changes and mesh changes are prepared, voted, then committed without allocation (CNativeDistributed3D.cpp:492-536). The accepted input is `const` and untouched on failure.
- Failure handling is collective and uniform (`ElectFailure`), with memory admission before allocation.
- Markers, corners and ridges cannot drift: boundary motion is admitted only inside every incident reference facet; marker names are compared at each import.
- The metric source is frozen on the original mesh, so repeated edits do not diffuse it; the edge length is integrated exactly for the P1 field.
- Deterministic containers and canonical tie-breaks; bounded loops; explicit id-range overflow guards.
- No master gather in the engine; worker subset M <= N; idle ranks wait with a sleeping `MPI_Test` loop instead of spinning.
- Wide unit-test coverage at component level (about 100 test cases), and project notes that report failures and limits openly.

**Coverage gaps seen from the test titles:** more than 4 ranks; stars above 64 cells inside an engine run; several cavities converging on one owner (M1); boundaries above 512 triangles or with two components (C1); oblique boundaries with non-dyadic coordinates (C6); anisotropy above about 2 (C7); events that need more than 512 rounds as an accepted outcome.

---

### 7. Not verifiable statically

- How often C5, C7, C8 and M1 occur on real meshes, and the true cost per edge of P3. These need runs.
- All timings and the memory per cell including allocator overhead; my figures come from `sizeof` and the admission formulas.
- Whether `-ffast-math` changes any filter decision with the compilers in use (C13).
- Behaviour of `CADTElemClass` box inflation, `CRankBoxTree`, `CPointDirectory` and `BisectionBoxes`; I assumed they are conservative, as their comments say.
- `CReaderSlices::FromDistributed` (consistency of nodal metric on nodes shared between ranks, renumbering) and `CSolutionTransfer` (conservation). Neither was read.
- ParMETIS determinism and its behaviour with very uneven vertex counts.
- The interplay of the non-collective `SU2_MPI::Error` (one-second `MPI_Ibarrier` probe on the same communicator on which idle ranks wait in their own `MPI_Ibarrier`, CNativeRemesher3D.cpp:273-279) across MPI implementations. By reading it ends in an abort, not a hang.
- Whether any production path passes non-default `EngineOptions`. I found only CSinglezoneDriver.cpp:299.
