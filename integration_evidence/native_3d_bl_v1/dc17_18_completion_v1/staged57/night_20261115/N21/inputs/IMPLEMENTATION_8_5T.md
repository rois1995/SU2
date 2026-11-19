# Step 8.5t: the ranks share the final surface proof (item N21), then the wide rounds with a fair time rule

Author: reviewer, 2026-11-19. For the main agent. Base: `f968325a69` (active build 586, part A of N20 is in).
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

The item has two parts.
- Part P: the ranks share the model-to-mesh proof of the final surface. Meshes and files stay identical. Only time and work counters change.
- Part W: the wide repair rounds again (the three paths of `6b56b42f16`), with a time rule that compares against the run of part P.

## 1. Why (facts from N20, all from your logs)

1. Part W of N20 passed all standing gates and gave the same files in two runs. It failed only gate 8 (time):
   event 1 1999.2 s (T0 2236.9 s), event 2 1521.5 s (T0 1416.5 s).
2. The quality phase became faster in both events (mean exclusive seconds for one rank: event 1 1419.6 -> 1228.2, event 2 832.1 -> 647.1).
3. Event 2 lost the gain in another place: `phase=final_proof operation=certification` went from about 90 s to about 297 s on each rank
   (run 2: 299 s). In the three other cases (T0 event 1, T0 event 2, W event 1) it is 81 to 91 s.
4. This time is `BoundaryParameterState::GeometryCertificates` (`CNativeBoundaryParameters3D.cpp:444-488`), called from
   `ProveOwnedSurface` (`CNativeEngine3D.cpp:330`). For each entity it calls `PatchErrorBounds` with direction `ModelToMesh`
   -> `BoundModelSurface` -> `QuerySurfaceUnion(include_model=true, include_mesh=false)`.
5. Each rank does this proof for the complete surface. The work is the same on all ranks: `full_work` at `stage=exit` is
   77,690,982 on each rank (T0 event 2) and 139,683,427 on each rank (W event 2). So 6 ranks do the same work 6 times.
6. The proof is a subdivision of each reference facet (a root) until its upper bound is not more than the tolerance
   (`CNativeAffineReference.cpp:2441-2518`, `from_model`, `magnitude=false`). In the passing case:
   - each root is independent of the other roots (the only stop before the end is `lower>tolerance`, which is the failing case);
   - the upper bound of a node does not depend on the order of the calls (`TriangleUnion::Upper`, lines 296-313);
   - the result `model_to_mesh` is the maximum of the leaf bounds. A maximum over parts is exact.
7. The traversal is at least 96 % of the proof time (T0 event 2, rank 0: `traversal_seconds` +122.6 s and `upper_seconds` +47.1 s
   between `size_repair_entry` and `exit`, `full_seconds` 88.8 s). The part that each rank must keep (the triangle union of the
   mesh faces) is about 10 to 13 s.
8. The value `model_to_mesh` of the stored certificate is read later (`CNativeImport3D.cpp:479`, `SourcePatch::Tube`).
   So the stored certificates must stay bit-identical.

Cause of the higher cost in W event 2: its surface needs 1.8 times the subdivision work for 7 % fewer faces. This is permitted
work, not an error. The cost of this proof changes much with the mesh. The fix is to stop doing it 6 times.

## 2. Idea, and what does not change

Rank r proves the roots number r, r+P, r+2P, ... of each entity (P = number of ranks). One election and one MAX reduction join
the parts. Then each rank stores the same certificates as today.

Does not change: the tolerance, the proof of each root, the triangle union, the reverse (mesh-to-model) proof of the owned faces,
the budget and storage admission, the acceptance decision, the error texts, all meshes and files. In the passing case the stored
certificate values are bit-identical. In the failing case the flags (`outside_tolerance`, `within_tolerance`) are the same and
the event fails as today; only the reported value `model_to_mesh` can differ (it is above the tolerance in both cases).
A run with one rank executes the same statements as today. No field is added to `Session`, `CellState`, `EngineStats` or a row
that travels. No timer operation is added. `fixedstorage=4120`.

## 3. Part P: the changes

### 3.1 P1: a root share in `PatchErrorBounds`

Files: `Common/include/adaptation/CNativeGeometryBackend3D.hpp`, `Common/src/adaptation/CNativeGeometryBackend3D.cpp:249-267`.

- New small type `struct ProofShare { unsigned part = 0, parts = 1; };` ("this caller proves the roots whose position modulo
  `parts` is `part`"). Define it where `CNativeBoundaryParameters3D.hpp` can see it (that header includes only
  `CNativeAffineReference.hpp`): in `CNativeAffineReference.hpp`, or include the backend header.
- `PatchErrorBounds` gets a last parameter `ProofShare share = {}`. One interface (hpp:61) and one `final` override (hpp:77):
  write the same default in both declarations.
- First statements, for all directions, before any work: `share.parts == 0` or `share.part >= share.parts` throws
  `std::invalid_argument`; direction `Both` with `share.parts > 1` throws `std::invalid_argument` (not supported, not used).
- The function builds the complete sorted list `facets` as today, with the same `Use(budget, ...)` calls and the same
  `budget.caller_bytes` charge of line 262 for the COMPLETE list. Then, for direction `ModelToMesh` and `share.parts > 1`, it
  keeps the facets at the positions `i` with `i % share.parts == share.part` (position in the complete list). Filter in place
  (the capacity and the charge stay). The kept list stays sorted.
- IMPORTANT: add no new `budget.caller_bytes` charge and remove none, here and in all of the shared path. The triangle union
  of the mesh faces refuses plane groups by `caller_bytes` (`CNativeAffineReference.cpp:528`). A different `caller_bytes` at
  that point can change the upper bounds and then the certificate. The values of today must be there.
- If the kept list is empty: `result` is a default `SurfaceUnionBounds` (all zero, all flags false), `reason.clear()`, return
  `true`, do not call `BoundModelSurface` (it throws for an empty list). This return comes after the list is built and after
  the body check.
- No change in `CNativeAffineReference.cpp` other than the possible definition of `ProofShare` in its header.

### 3.2 P2: `BoundaryParameterState` gives its proof in two steps

Files: `Common/include/adaptation/CNativeBoundaryParameters3D.hpp`, `Common/src/adaptation/CNativeBoundaryParameters3D.cpp:444-488`.

`GeometryCertificates` keeps its signature and its behaviour for all its callers. Split its body into small private helpers so
that the new functions use the same statements (R9; no second copy of the loop of lines 467-474 or of lines 476-486; one
shared helper gives the entities and their faces). Keep the order of the statements, also of the `budget.CheckTime()` calls, so
that the work counts of a serial run and of a share with `parts = 1` are equal to today.

New public functions (names are a proposal):

- `bool ReverseProofComplete(const std::vector<double>& reverse_bounds) const`:
  true when entity geometry is active, `geometry->ReferenceComponents()` is false, the size is equal to the number of entities,
  and each bound is finite, `bound >= 0` (as line 464) and not more than `geometry_tolerance`. This is the case in which line 478 gives
  `reverse_known` for each entity, so each entity uses direction `ModelToMesh`.
- `std::vector<ModelShareBound> ModelProofShare(AtlasBudget&, const std::vector<double>& reverse_bounds, ProofShare) const`,
  with `struct ModelShareBound { double model_to_mesh = 0; bool outside = false; size_t roots = 0; };`, one element for each
  entity in the order of the entity map.
  It throws `std::invalid_argument` when `ReverseProofComplete` is false. It opens the same `Certification` timer scope, calls
  `budget.CheckTime()`, makes the same `AtlasStorage` admission and builds the same entity face lists as `GeometryCertificates`
  with `complete=true` and reverse bounds (all faces of the complete surface; no `EntityFaceError` call, as today in this case).
  For each entity it calls `PatchErrorBounds(..., GeometryErrorDirection::ModelToMesh, share)` and copies `model_to_mesh`,
  `outside_tolerance` and the count of kept roots. A `false` return throws `std::invalid_argument(reason)` as today.
  It does not read or write `geometry_certificates` or `certificate_revision`.
- `const std::map<EntityKey, SurfaceUnionBounds>& AdoptModelProof(const std::vector<double>& reverse_bounds, const std::vector<ModelShareBound>& joined) const`:
  for each entity it builds the certificate with the statements of lines 481-486: `model_to_mesh` and `outside_tolerance` from
  `joined`, `mesh_to_model` = the reverse bound, `physical_pieces` = the number of surface faces of the entity, `within_tolerance`
  by the same expression, `model_pieces = 0` (the value that `BoundModelSurface` returns today in this case). It stores
  `geometry_certificates` and `certificate_revision = revision` and returns the map. It makes no geometry query and opens no
  timer scope (so the `calls` of `certification` stay 2: the scope of `EngineControl.cpp:28` and the scope of `ModelProofShare`).

Required property (fixture T1 proves it): for each number of parts, `AdoptModelProof` with the joined shares (maximum of
`model_to_mesh`, logical OR of `outside`) gives a map that is equal, field by field and bit by bit, to the map of
`GeometryCertificates(budget, true, &reverse_bounds)`.

### 3.3 P3: `ProveOwnedSurface` joins the shares

File: `Common/src/adaptation/CNativeEngine3D.cpp:304-333`. Split the function into small parts (R9): the existing reverse part
(lines 307-329) and the new forward part. The reverse part does not change.

After the MAX reduction of `bounds` (line 329):

```
shared = world.size > 1 && state.ReverseProofComplete(bounds)      // reduced values: the same decision on all ranks
if (!shared)
  the existing lines 330-332, not changed
else
  local step, in try/catch as lines 311-322:
      share = state.ModelProofShare(budget, bounds, {static_cast<unsigned>(world.rank), static_cast<unsigned>(world.size)})
  elected = world.elect(failure);  if (elected.any) throw std::runtime_error(elected.message)      // as line 323-324
  for each entity e:  joined[e].model_to_mesh = world.reduce(share[e].model_to_mesh, MAX)
                      joined[e].outside       = world.reduce(int(share[e].outside), MAX) != 0
  for each row of state.AdoptModelProof(bounds, joined):
    if (!row.second.within_tolerance) throw the same std::runtime_error text as line 332
```

- A rank must reach the election also when its local step throws. Use the `CLocalFailure` pattern of lines 309-324.
- The existing storage scope (`ProofStorage`) stays active to the end, as today.
- With `world.size == 1` the old statements run. So serial results and serial work counters do not change.

### 3.4 P4: prints (under `options.timings` only, R11)

`ProveOwnedSurface` has no options. Give it a parameter `bool timings` (from `options.timings`) and let it return a small local
report value (not stored in any state) that `EngineControl::ProveEntitySurface` prints when `options.timings` is set. Read
`std::chrono::steady_clock` only when `timings` is true. The prints get no timer scope (no `Output` scope: it would add timer calls).
With `shared=0`: `own_roots` = `reference_facets` = the number of reference facets.

- One line for each rank:
  `NATIVE3D_FINAL_PROOF rank= shared=<0|1> entities= reference_facets= own_roots= reverse_seconds= share_seconds= share_work=`
  (`share_work` = the difference of `budget.work` over the local forward step; with `shared=0` the forward step is the old call).
- On rank 0 only, one line for each entity:
  `NATIVE3D_FINAL_CERT entity=<body>/<sheet>/<marker> faces= mesh_to_model= model_to_mesh= tolerance= within=`
  (values with 17 significant digits).

No decision reads these values.

## 4. Fixtures (your test code, R3)

Put T1 to T4 in NEW test cases with a tag that the G1 selection does not run (for example the existing tag
`[NativeOwnedFaceProof3D]`; the G1 serial selection runs `[NativeGeometryBackend3D]`). Do not edit an existing test case. So the
G1 tables stay identical. Run the new test cases as their own step.

- T1 share identity (serial). Use the entity fixture of `UnitTests/Common/adaptation/CNativeGeometryBackend3D_tests.cpp` near
  lines 1167-1198 (the test that already calls `GeometryCertificates(budget, true, &bounds)`). For `parts` = 1, 2, 3 and 6:
  call `ModelProofShare` for each part with a fresh budget, join (maximum, OR), call `AdoptModelProof`, and compare each field of
  each entity with the map of `GeometryCertificates(fresh_budget, true, &bounds)` on an equal second state: `CHECK(a == b)` on
  the doubles (bit equality), the counts and the flags. Also check: the sum of `roots` over the parts is the number of reference
  facets of the entity; for `parts` = 1 the `budget.work` of the share is equal to the work of that unshared call. An entity
  with fewer facets than parts has empty shares: they return zero and the joined result is still equal.
- T2 precondition. One reverse bound above the tolerance: `ReverseProofComplete` is false and `ModelProofShare` throws.
  A wrong `ProofShare` (`parts = 0`, `part >= parts`) throws. Direction `Both` with `parts = 2` throws.
- T3 failing surface. Use the `missing` input of `CNativeGeometryBackend3D_tests.cpp:220`: call
  `PatchErrorBounds(scope, missing, 1e-4, b, budget, reason, GeometryErrorDirection::ModelToMesh, {k, 3})` for k = 0, 1, 2. The
  OR of `outside_tolerance` over the three shares is true, as in the same call without a share. Do not compare `model_to_mesh`
  here (section 2). Then call `AdoptModelProof` on a state with a hand-made `joined` row with `outside = true`:
  `within_tolerance` is false.
- T4 the join with real MPI, at 2 and 4 ranks. The gate runs before G5 do not execute the shared path (G3 is serial; the G1
  cases at 2 and 4 ranks use the prepared-surface proof). Add one small MPI test case: each rank builds the same entity state
  as T1, the reverse bounds are the same on all ranks, the new forward part of `ProveOwnedSurface` runs with the real `World`,
  and each rank compares the adopted map with the map of the unshared call (bit equality). If the forward part cannot be
  reached from a test in the limit of R3 (90 minutes): write it in the report and continue; then the G5 run is the first MPI
  execution, and its gates decide.

## 5. Gates of part P

1. Whole-item source check before the first production edit: `R/night_20261115/N21/source_check.md` (R4). If 3.1 to 3.3 disagree
   with the source and no clear reading exists: do not implement part P; then do not do part W; write it and stop.
2. Build without a new warning. T1 to T3 pass; T4 passes or has its written R3 note.
3. G1: the tables are identical to the reference (375 / 114 / 228).
4. G2 and GQ: as in section 2 of `AGENTS.md`. These runs stop before the final proof (the N20 logs have no `final_proof` row).
   If one of them reaches it, rule 6 applies.
5. G3 (a) and (b), G4, and the safeguards of `R/shared_engine_regression_gates.json`: as for an item with identical results.
6. Rows that can differ from the reference in a run with more than one rank, from the final proof on. They are a report, not a
   gate (E = number of entities):
   - `full_calls`, `full_work`, `full_seconds`, `traversal_work`, `traversal_seconds`, `atlas_work`, `atlas_arithmetic_work`
     and the other cumulative work rows of the atlas;
   - the count and the seconds of `communication` in phase `final_proof` (expected count: the old count + 1 + 2 x E; 7 -> 16 for
     E = 4); `collective_calls` (+ 2 x E), `election_calls` (+ 1), `collective_seconds`, `election_seconds`, `tracked_calls`
     and the `communication_calls` rows that come from them;
   - the seconds of `certification`;
   - `atlas_bytes`, `atlas_peak_bytes` and the modeled bytes that come from the atlas peak (`native_bound`,
     `maximum_native_bytes`): expected equal or lower (the queue of a share is shorter). A HIGHER value of one of these byte
     rows is a failed result gate (R5).
   The `calls` of `certification` stay 2 on each rank. All other integer rows and `fixedstorage=4120` are as the rule for
   identical results says.
7. One G5 run (folder `N4/section6_3_after_N11/round_8_5t_P/G5/`): all standing gates of both events, and the six files are
   byte-identical to the references of section 2 of `AGENTS.md`. Both events print `shared=1` on each rank.
8. Purpose gate of part P: in each event of this G5 run, the largest value over the ranks of the exclusive seconds of
   `phase=final_proof operation=certification` is not more than 53.4 s (0.6 x 89 s, the T0 value). The reviewer expects about 26 s.
9. The native maxima of the two events of this G5 run are the time reference T1 (T1_1, T1_2) for part W. Write them in the report.
10. Commit and push part P ("N21 P").

## 6. Part W: the wide rounds, with the time rule against T1

Only when part P passed.

1. Apply again the three paths of `6b56b42f16` (one commit, the same overlay check as in N20).
2. Text and gates of I8R2 section 5 and section 6 items 5 to 7, as in N20 part W: the scoped gates, the wide fixture at 2 and 4
   ranks, two G5 runs with the same binary, the T0 event-one prefix, the isolated event-two restart, six files byte-identical
   between the two runs. Run folder `N4/section6_3_after_N11/round_8_5t_W/G5/`.
3. The time rule (it replaces gate 8 of I8R2 for this item; the user order gives the factor F):
   - (smaller native maximum of event 1 over the two runs) + (the same for event 2) is not more than F x (T1_1 + T1_2);
   - the smaller native maximum of each event is not more than its T1 value.
4. If all pass: the six files of part W are the new reference (rule of the user for an item that changes the mesh), and its
   times are the new time reference. Update `Handbook/ADAPTATION_TIMINGS.md`.

## 7. Failure rules

- Part P, a result gate (rules 2, 3, 4, 5, 7): R5. If it fails again or the cause is in this text: revert part P in one commit,
  "N21 P FAILED", keep the evidence, do not do part W.
- Part P, only the purpose gate 8 fails: do not revert at once. Write the `NATIVE3D_FINAL_PROOF` rows of each rank and event in
  the report (`own_roots`, `share_seconds`, `share_work`), then revert part P ("N21 P FAILED its purpose rule"), do not do part W.
- Part W, `complete=0` in an event: keep the outputs, no second attempt, no other width, revert only W, keep P, "N21 W FAILED".
  Copy the M28, M29, `NATIVE3D_FINE_*` and `NATIVE3D_FINAL_*` lines to `R/night_20261115/N21/reports/W_dump.txt`.
- Part W, another result gate: R5, then the same revert.
- Part W, only the time rule fails: revert only W, keep P, "N21 W FAILED its purpose rule". Report the numbers of section 8.

## 8. Report

`R/night_20261115/N21/reports/STEP_8_5T_<build>.md`:
- part P: the gates; T1_1 and T1_2; for each event and rank the `NATIVE3D_FINAL_PROOF` row; the `NATIVE3D_FINAL_CERT` rows;
  the exclusive seconds of `final_proof certification` beside the T0 values; the rows of rule 6 beside the reference.
- part W: the table of N20 (native maximum, process-peak sum, timer coverage, proof faces for T0, T1, run 1, run 2); the M28
  table; the fine placement evidence; the `NATIVE3D_FINAL_PROOF` and `NATIVE3D_FINAL_CERT` rows of both events; the mean
  exclusive seconds of each phase (preparation, construction, quality, final_proof) for T1 and for run 1; the time rule.
- Update the progress file, `Handbook/ADAPTATION_TIMINGS.md` (when a part passed), `HANDOFF_Codex.md`, `HowToProceed.md`.
  Commit, push, verify that no owned job runs, stop and wait for the user.

## 9. Not in this item

A change of the proof of one root or of the triangle union; a share for direction `Both`; a share of the reverse proof of the
owned faces (it is already divided by owner); the lock-step census of the preparation phase (event 2 lost about 63 s there in
N20; the reviewer looks at it later with the phase table of section 8); a balance of the shares by cost; another width;
lanes 8.5ab, 8.5c, 8.5d.

## 10. Answers to the questions file

The four N20 entries (raw champion as the placed target; mark all requested vertices first; keep the insertion point and both
stop conditions; `accepted` = the private validation flag) are correct. No action.
