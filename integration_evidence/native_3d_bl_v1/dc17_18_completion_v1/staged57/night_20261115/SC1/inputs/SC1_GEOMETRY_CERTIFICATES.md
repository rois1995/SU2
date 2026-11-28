# SC1: geometry certificates from the core and the final proof replace the entity-distance query of the seed census (mode S)

Author: reviewer, 2026-11-27 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/SC1/inputs/`. Start only after DEF1 has its final label and its
owned handles are terminal (R12). Short names R, N, "mode S", "standing gates" as in the order FP3-A (your copy in
`N/FP3_A/inputs/`), whose distinction between an R5 result failure, defective evidence and a helper-only watchdog
holds here. The S reference in force is the one of `AGENTS.md` section 2a after DEF1 (FP3-E `workflows.S`, run with
the pinned legacy keys of DEF1). Design basis (read-only studies; context, not orders):
`reviewer_handoff/scripts/s8/study_old_engine_time.md`, `reviewer_handoff/scripts/s9/study_seed_census.md`.

User direction (2026-11-26, 2026-11-27): each old-engine function in the fast path is removed or improved and
embedded in the new core, largest first; "continue working in our removal of the old engine".

## 1. Why

| Fact | State |
|---|---|
| Largest old-engine items over CL1 (4 events) and FP3-E S1 (2 events): quality step 358 s (83 s after event 1), seed census 301 s (269 s after event 1), final proof 219 s | measured (s8) |
| The seed census grows each cycle: CL1 16 / 27 / 38 / 123 s, FP3-E 15 / 81 s; 88-98 % of it on the critical rank is the geometry part (`preparation/geometry_bound`; e4 117.8 of 120.3 s) | measured (s8, s9 section 6) |
| The census geometry: `MeasureGeometryFace` (`EngineSelectionCensus.cpp:519-575`) calls `SurfaceDeviation(facet, triangle, Magnitude)` with `validated=false` (`CNativeBoundaryParameters3D.cpp:1584-1590`): `ParameterContainsFace` (throws if the face leaves its authority), then `ParameterError` (`:1786-1795`): the reservation time check, the affine-chart maximum over the three points, then `EntityFaceError(facet, bindings, points, reservation, need, maximum)` | read in the source |
| For a finite error <= tol the census adds `+0` debt and returns (`EngineSelectionCensus.cpp:536-543`); a non-finite error sets `geometry_measured=false` | read in the source |
| Step C bounds each accepted child face with `WithinTolerance` (`CNativeLocalSurfaceGeometry3D.cpp:758-774`, called `CNativeLocalCore3DCollapse.cpp:487-494`); the final proof bounds the owned faces that it actually queries (`OwnedFaceReverseBounds`, `CNativeBoundaryParameters3D.cpp:541-571`; it skips component-backed faces and stops an entity after a failed bound, `:550-566`) and keeps only the maximum per entity | read in the source |
| These bounds are rigorous upper bounds of the directed distance of the face to its model entity, with another vertex order (`FaceOrder`, `CNativeLocalSurfaceGeometry3D.cpp:299-303,456`) than the census (cell-face order); the first corner can change the numerical bound value, not the truth of "distance <= tol" | read in the source |
| Census queries fill and evict the prepared-facet pool; `StructuralEstimate` reads live piece counts and `Repartition` uses geometry weights derived from them (`CNativeAffineReference.cpp:2854-2861`, `EngineOwnership.cpp:264-278,343-347`, `CNativePartition3D.cpp:295-300`, `CNativeImport3D.cpp:1760-1788`, `EngineExchange.cpp:55-81`); skipped queries also lower charged atlas work | read in the source (Codex SC1 pass 1) |
| So fewer census queries can change where cells go, hence the six S files, even when every certificate agrees with the census | estimated; NOT KNOWN in size |
| Saving with step-C + proof certificates: about -12 / -21 / -32 / -117 s in the CL1 events, -10 / -75 s in FP3-E | estimated (s9) |
| Removing the census queries does not move their facet-pool thrash into the final proof (the proof thrashes on its own) | measured (s9 section 4) |

## 2. The model (one rule)

The surface geometry belongs to the core. A geometry certificate of a physical face says: a rigorous upper bound of
the directed distance of this face to its model entity is <= tol. It is valid for the face only while every input of
that bound is unchanged. In mode S the census computes the entity distance of a face only when the face has no valid
certificate; every other check of `SurfaceDeviation` stays. The metric part of the census (lengths, size debt, queues)
stays exactly as today. The final proof stays the geometry gate; its decisions do not change.

Scope: the entity-geometry path (`EntityGeometry()` true in `ParameterError`). Other paths never read a certificate.

## 3. The production change (R2: allowed by this text, nothing else)

1. Certificate key (compare bit for bit; no hash-only equality), ID-free so that it survives the renumbering at the
   return to CFD (`CNativeImport3D.cpp:2052-2076`, `NativeRemeshRun.inc:1552-1589`):
   - the canonical facet anchor of the face and the identity of its model entity as `Entity(facet)` gives it
     (`CNativeBoundaryParameters3D.cpp:430-432`: body, sheet or canonical facet, marker), and the exact identity of
     the mobility / entity policy and of the accepted (mapped) authority that the bound uses;
   - for each of the three vertices: its 3 coordinate bit patterns and its full binding bits, the binding obtained
     through the same `Parameter(point)` path as the census (`:1788-1789`); the three vertices sorted by their
     coordinate bits (the face identity), independent of the writer's query order;
   - the effective tolerance bits.
   List every input of `EntityFaceError` for this path in the R4 report with file:line; if one is missing here, add
   it (R4 "reading used"). Write a record only from a completed, finite, nonnegative upper bound <= tol.
2. Writers (mode S, eligible events only):
   - Step C: stage the inputs of each child face that `WithinTolerance` accepted, independently of the timings and of
     the reporting sink; keep the staged records after a successful commit and hand-off; drop them if the event does
     not end with `close_construction_collapse` true (`NativeRemeshRun.inc:700-718`).
   - The final proof: each owned face it actually bounded with a finite bound <= tol.
   - Engine surface commits that already compute such a bound for a new face (list the sites in R4; a site without a
     bound writes nothing).
   - Origin tags (step C / proof / commit) are diagnostic only; presence of the key is the lookup result; if two
     writers certify the same key, keep the first origin.
3. Lifetime and replication: an S-only sidecar owned by the remesher object that `RunAdaptationLoop` reuses across the
   events of one solver process (`CSinglezoneDriver.cpp:529-539`), empty at process start, never serialized (not into
   the `.native_ref` sidecar, a G5 payload). Stage the proof records locally per worker stage. An event can run a
   second worker stage after a normalization correction (`NativeRemeshRun.inc:415,567-597,669`): once `RunStages`
   has selected the mesh for return, keep only that stage's proof records and gather them ONCE on that stage's worker
   communicator before the return packing. On acceptance of the event (`onAccepted`) swap the table with this
   prepared snapshot (the next event's step-C records are added to it). Use `MobilityPolicy::identity` (a 64-bit
   hash) only as a lookup aid; equality of authority and policy is exact. A rejected event drops the table. A rank that was not
   a worker starts the next event with misses (correct, slower).
4. Storage and collectives: charge the table, the staging and every gather buffer (send, receive, scratch, the old
   and the pending table at once) through the existing S admissions BEFORE allocation. Agree admission and local
   failure collectively (rank-uniform mode) before the payload gather, so that no rank waits in MPI alone. Use a
   separate passive record type. No new member in `CellState`, `CachedRow`, `Session`, `BoundaryParameterState` (its
   `ResidentBytes` starts with `sizeof(*this)`, `CNativeBoundaryParameters3D.cpp:814`), `PlanarReference` or any row
   that travels; no new timer operation; OFF and CORE keep their calls, scopes, modeled bytes and
   `fixedstorage=4120` exactly.
5. Reader: `MeasureGeometryFace` in mode S, entity-geometry path. On a valid certificate keep `ParameterContainsFace`
   (the throw), the reservation time check and the affine-chart maximum exactly as today; skip ONLY the
   `EntityFaceError` call, and only when the affine maximum is finite and <= tol: the census then handles the face as
   "finite and <= tol" (`+0` debt, nothing else). If the affine maximum is not finite or > tol, the face is queried as
   today (no shortcut). Reader scope: the seed census (`SeedNext` -> `Measure` -> `Selection::Add`) and the
   construction-phase update paths that call `MeasureGeometryFace` (list them in R4). NOT the improvement-entry census
   (`CompletePhaseEntry`, `EngineControl.cpp:585-644`, which reaches `Selection::Add` through `Measure`,
   `EngineCensus.cpp:248-253`): there the lookup and the counts are off and the face is queried as today. The count
   denominator of value `6` uses the same scope.
6. Lab values of `SU2_NATIVE_FP3_PROBE` (`CNativeLocalCore3DMeasures.cpp:205-216`, enum `CNativeLocalCore3D.hpp:54`):
   `1`-`4` stay as they are, and certificates are OFF with them; `5` = certificates OFF (the FP3-E behaviour; its
   reference is the S entry of FP3-E); `6` = COUNT mode: writers write, the reader does NOT use a record, and at each
   reader call it counts `cert_hit` (valid record), `cert_miss`, `cert_disagree` (a hit whose legacy result is NOT
   "finite and <= tol", NaN included) and hits by origin. Certificates are ON only with `0` (default) on eligible S
   events. One print-only row per rank and event (R11) `NATIVE3D_GEOMETRY_CERTIFICATES`: hits, misses, disagreements
   (only in `6`), origins, records, bytes, gather seconds, and in `6` the skip timing of step 2.
7. Timer scopes (the attribution text that `AGENTS.md` section 2a asks for): lookup and count bookkeeping under
   `Census`; proof-record collection and the gather under `Certification`; step-C staging under `Topology`; the
   return-side table work under `Phase::Transfer` with the existing operation `Import`; the diagnostic row under
   `Output`. No new operation. The ledger gives totals per phase and operation, not single spans: the attribution
   evidence comes from step 2 (see there). The removed census seconds are named time that no longer runs.
   The 95 % coverage gate does not change (hard gate).

## 4. Steps and gates

Use SC1-local adapters (test code, R3) on the DEF1-pinned configurations; authenticate the sealed FP3-E S reference
separately. Export and receipt the probe value on every rank. Apply the FP3-E suppression, ownership and stall checks
to every S run; run and seal the isolated restart that the standing gates ask; the required hash comparisons are hard
checks. Keep the P3 `saved_current` and the G3 comparators.

Step 1 (shared code; OFF paths exact): build; G1, G2 and GQ (exact prefixes as FP3-E), G3, G4, `saved_current`, the
shared-engine gates: identical to their references. Full OFF and CORE runs: "not evaluated".

Step 2 (measurement first): one S run with value `6` (count mode). Gates: all standing S gates, and all six payloads
identical to the S reference in force (count mode makes the same queries as today and changes no decision). Require
one valid count row per rank and event, with hits + misses equal to the reader calls and the origins summing to the
hits. Report the counts; report finite over-tolerance disagreements apart from non-finite ones.
Attribution evidence (the text that `AGENTS.md` section 2a asks for). The ledger has only totals per phase and
operation, so value `6` adds two print-only measurements (R11; a steady clock, no timer operation, no decision reads
them), printed in the `NATIVE3D_GEOMETRY_CERTIFICATES` row and one `NATIVE3D_UNATTRIBUTED_SPANS` row per rank and event:
(i) `skip_seconds` and `skip_calls`: the wall time of the legacy `EntityFaceError` calls on the hits that the default
would skip (valid record AND affine maximum finite and <= tol), measured around exactly that call; (ii) the
unattributed gaps in the SAME accounting as the coverage gate: a gap is an interval owned by the remesh root frame
alone or by an innermost `Unattributed` (phase-only, `PhaseScope`) frame, also when a named ancestor is open
(`CNativeTimerTree3D.hpp:220-242,328-371`); root-only gaps are listed under `Preparation` (the root's initial phase,
`CNativeRemesher3D.cpp:213`). Per phase, the three longest single gaps with start, end and the source boundaries that
open and close them (scope push / pop and root start / finish give the timestamps; record them in separate storage;
do not change the tree's rows, calls or `fixedstorage=4120`). Give per rank and event the root seconds, the
unattributed seconds (attributed = root - unattributed) per phase, and the PREDICTED coverage after the removal:
(attributed - skip_seconds) / (root - skip_seconds), with root = the final `root=remesh` summary of that rank and event,
the same totals as the 95 % gate (`collect_fp3_full.py:179,688,842`). If a predicted value is below 96 %: before step 3, put the longest listed
unattributed interval of that phase under an existing named operation of that phase (no new operation), repeat the
count run once, and give the new values. Coverage stays a hard gate (95 %) in step 3.

Step 3 (default `0`, mesh-changing authorized by this text): two S runs S1, S2 on the same binary.
- All standing gates in both runs (5400 s, 19073 MiB with the job peak equal to the summed peaks, physical
  validator, transfer, continued CFD, isolated restart, 95 % timer coverage, join agreement, owned proof faces SUM
  equal to the surface size), and the final proof two-sided as today.
- If the six files of S1 equal the S reference in force: "identical results"; S2 must also equal them.
- Else: the six files of S1 must equal those of S2 (user rule); they become the new S reference
  (`N/SC1/accepted_reference.json`), and one lab run with value `5` must reproduce the S reference in force (all six).
- Report beside the S reference: the seed-census time per rank and event, the facet misses and builds (s9 table
  format), the ownership weights and partition digests, the atlas work beside its unchanged limit, the table size and
  the gather seconds, the timer coverage (attributed and unattributed seconds per rank and event).
- The time saving is a report, not a gate (except 5400 s). `cert_disagree > 0` in step 2 is a finding, not a failure
  (the certificate is a rigorous bound; the census bound was looser); report the faces.
- A failed gate: R5.

## 5. Report and labels

`N/SC1/reports/SC1_<build>.md` and `.json`: the R4 list (key inputs, writer sites, reader callers, the table owner,
the unattributed spans), the gates and reports of steps 1-3. Labels: "SC1: default paths unchanged" (step 1), "SC1:
count mode measured" (step 2), then "SC1: accepted; geometry certificates in the mode-S census (identical results)"
or "SC1: accepted; geometry certificates in the mode-S census (new S reference)", or "SC1: FAILED at <step> <gate>"
(R5, reverted). After acceptance add one bullet to `AGENTS.md` section 2a in the form of the FP3-E bullet (for a new
S reference: the S entry of FP3-E becomes the reference of the lab value `5`). Update the progress file,
`HANDOFF_Codex.md`, `HowToProceed.md` and the timings handbook. Then STOP. Rules R1-R13 hold.

## 6. Not in this order

The metric part of the census (the core and the engine use different length formulas: a later model decision). The
quality-entry census. The facet-pool size (a limit, R1: user decision). A locality order of the final proof loop. Any
change outside mode S. A warm-versus-cold continuation equivalence check (the isolated restart uses `ADAP_LOOP=NO`;
write it "not evaluated").
