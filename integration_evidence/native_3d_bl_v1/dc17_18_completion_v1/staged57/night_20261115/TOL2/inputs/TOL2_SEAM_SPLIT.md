# TOL2: a frozen marker seam may be split on its reference edge; one capability run

Author: reviewer, 2026-11-28 (reviewed by Codex). For the main agent. The user sends it; it is a user order.
Copy this file with its SHA256 into `R/night_20261115/TOL2/inputs/`. Start only after TOL1 and CL3 are closed (they are:
`c1e1c8fac5`, `50f19e926e`). Short names R, N, "mode S", "standing gates" as in the order FP3-A. TOL1 (your copy in
`N/TOL1/inputs/`) stays in force; this text changes only its section 2.3 rules 3 and 4 and the items named here. If a
`reviewer-*` unit is active, do not touch it and start no build, test binary or solver while it is active (R1).

User decision (2026-11-28) unchanged: "Yes, I think a looser tolerance on the farfield would be better." No gate,
budget or limit changes.

## 1. Why

| Fact | State |
|---|---|
| CL3 (TOL1, FARFIELD 1.0) rejected events 1-4 on SIZE only: every rank has q_bad = j_bad = geometry_bad = 0; rank 3 has `size_bad=3`; the census has exactly 3 edges above 1.8 (`edge_histogram=465995,3,0`) | measured (`reviewer_handoff/scripts/s12/diag_cl3_event1.md` section 1) |
| The 3 edges are input seam edges of the circle y = 0, r = 100 (input ids 49-50, 48-49, 45-46; 9.81 m; metric lengths 2.136, 2.020, 1.872 against 1.8); their squared overshoots sum exactly to rank 3's size debt | measured (s12) |
| CL2 (scalar 1e-4) split exactly these 3 edges at their chord midpoints (mesh 1 points 12517, 12522, 12438; error 1.4e-14) and kept all 64 input circle vertices; its event 1 was accepted | measured (s12 section 4) |
| TOL1 2.3 rule 3 refuses every split of a seam edge; with both ends fixed a long seam edge can never get shorter; the size gate has no exemption (`EngineSelectionCensus.cpp:480`, `:217-220`) | read in the source (s12 sections 6-7) |
| The TOL1 text said "the metric asks 16-18 m there, so no split is lost on M6": false for 3 of the 64 seam edges. The cause is in the reviewer's text, not in the implementation (the TOL1 wiring matched the order: 64 / 64 seam, classes, values) | measured (s12 sections 7-9) |

## 2. The production change (R2: allowed by this text, nothing else)

The seam model after TOL2: a seam between two marker classes can be REFINED but never changed otherwise. Every seam
point is fixed for its life (no move, no removal). A seam edge may be split at a point ON its reference seam edge;
the new point is a seam point from then on. So the mesh seam is always the input seam plus points on the reference
seam edges, in order.

1. Split of a seam edge (replaces TOL1 2.3 rule 3 for splits only). A split of a mesh seam edge is allowed when its new
   point is the existing feature-edge split placement of that edge, with a binding on the SAME reference seam edge
   (dimension 1 on that reference edge; for a mesh seam edge that spans a reference seam vertex, see item 2), in the
   engine and in the local core. Remove only the split refusals of TOL1 for this case (R4: name each site; at least
   `CNativeLocalSurfaceGeometry3D.cpp:710-712`, `:745-746`, `CNativeLocalCore3DSurface.cpp:1022-1025` and the engine
   surface split path that TOL1 guarded). The shared placement routine `ProposeFeaturePoint`
   (`CNativeCurvedAtlas3D.cpp:603`) requires mobility 1 and would still refuse: give it ONE split-only exception for a
   parent that is a verified seam edge: the new binding must lie STRICTLY between the parameters of the two current
   endpoints on the same reference edge; its mobility stays 0 (`CNativeAffineReference.cpp:2178`); every other
   caller and every other check of that routine are unchanged. Flips and retirements of a seam edge stay refused, except the retirement of
   the parent edge by its own split. Moves and removals of any seam point stay refused (TOL1 2.3 rules 2 and 5,
   unchanged; the new point is dimension 1 on a seam edge, so `MobilityDimension` already gives 0).
2. One reference edge per mesh seam edge. A mesh seam edge always lies on one reference seam edge, because the input
   seam edges are the reference edges, every seam point stays fixed, and a split puts its point on that same reference
   edge. R4: confirm it on the M6 input (the 64 input circle edges are reference edges) and refuse a split whose
   placement binding is on another reference edge or off the seam (count it as a seam refusal).
3. Seam check (replaces TOL1 2.3 rule 4's "equal to the event input").
   Which edges are seam edges: decide it from the two ACTUAL incident faces of each surface edge in the complete
   post-change incidences (an edge whose two faces have markers of different classes), not from the endpoint
   bindings. Every such mixed-class edge must lie on ONE reference seam interval of the baseline; otherwise reject
   (this also catches a relabeled face that creates a new mixed-class chord).
   Baseline and sites:
   - Engine publication: baseline = the accepted store. In the shared `BoundaryParameterState::Prepare`
     (`CNativeBoundaryParameters3D.cpp:907`): keep the veto on any edit of an EXISTING seam record; replace the
     insertion half of the guard (`:1022-1025`): a NEW seam record is admitted only when the subdivision check below
     consumes it; replace the side count of `:1056-1062` by the subdivision check. `CheckFrozenSeam` (`:875-899`) uses
     the same rules.
   - Local core: at each local split, validate the split against the LIVE parent edge and its two oriented sides in
     `PlaceOnFeature` (`CNativeLocalSurfaceGeometry3D.cpp:559`), which has the two live parent faces and builds the
     child faces BEFORE the volume commit (`Mesh::SplitSurface`, `CNativeLocalCore3DSurface.cpp:514`), so a mismatch
     is a clean split refusal; `Accept` (`:742-746`) keeps recording the validated placement. At installation (`:957`, the replacement state), compare the full
     subdivision with the INSTALLED graph (baseline = installed state), and separately require every seam point that
     the pass accepted into the overlay to keep its exact id, coordinates and binding.
   Subdivision rules (against the baseline):
   - every baseline seam point is present, with the same id (before renumbering), exact coordinates and binding (so a
     point accepted by an earlier split can never be replaced);
   - every baseline seam edge is either present, or replaced by ONE chain of seam edges between its two endpoints
     whose inner points are new seam points; the parameters of the chain points on the parent's reference edge are
     strictly monotone from one endpoint to the other and the chain exhausts the parent's interval;
   - every new seam point has a binding on that reference edge and the exact coordinates of its placement, and is used
     by exactly one chain; no other mixed-class edge exists;
   - for each retained edge and each child edge, its two oriented sides (direction taken along the reference-edge
     parameter, not by ids) carry the same marker and entity as the baseline edge's (or the parent's) sides, compared
     edge by edge.
   Anything else is the TOL1 error "Native 3D adaptation changed a frozen marker seam."
   New point coordinates (engine). Validate each new seam binding and its exact realization in the publication's added
   records (`Publication::added`); then, before the collective precommit vote (after `Prepare` fills the publication, `CNativeDistributed3D.cpp:1611`, and
   before the vote at `:1616`), join
   every staged occurrence of a new seam id (a staged `Node` has id and coordinates, `CNativeMesh3D.hpp:29`) with that
   record by id and compare the coordinates bitwise. Keep the early staged-coordinate check
   (`CNativeDistributed3D.cpp:1491-1493`) for the empty-delta case.
4. Counters (print-only, R11): `NATIVE3D_TOLERANCE` gets `seam_splits=<accepted seam splits so far>` beside
   `refused_seam_edits`, and `seam_points=<current seam points>`.
5. Option text (`CConfig.cpp`, `config_template.cfg`): "where two boundaries with different values meet, their seam
   keeps its points; a seam edge can only be split, at a point on the seam."

Scalar mode and equal lists: no seam exists (`classes` empty), so none of these paths runs; every byte stays as today.
No other change.

## 3. Gates

1. Fixtures (test code, R3). Keep the pinned TOL1 seam fixture for its negative checks (moves, removals, flips,
   off-seam changes). Add ONE new two-marker fixture with a non-straight shared feature curve and a pre-subdivided seam
   edge, built so that a split at the feature placement clears the local outward check
   (`CNativeLocalCore3DSurface.cpp:387`) and the complete surface passes both directed final-proof bounds at its
   declared marker values (record the measured heights and bounds in the report). On it:
   - a split of a seam edge is accepted in the engine AND in the local core (through `ProposeFeaturePoint`); its point
     lies strictly between the endpoints on the same reference seam edge; the new point is then fixed (a move is
     refused; a contraction removing it is refused);
   - a second split of a child edge is accepted; replacing an accepted split point by another point on the same
     reference edge is rejected (baseline = current state, also inside one local pass);
   - the seam check rejects: a child point off the reference edge; a child point on another reference edge; a
     non-monotone chain; an extra new seam edge; an unused new seam point; a removed or moved baseline seam point; a
     side marker / entity change of one child edge and of one retained edge; a flip of a seam edge; a relabeled face
     in a coplanar two-marker patch that creates a new mixed-class chord while every original seam edge stays;
   - a local pass with two splits of one parent edge installs; an engine publication with a new seam record passes
     the shared `Prepare`;
   - a staged occurrence of a new seam id with coordinates different from its insertion record, injected in the split
     publication itself, is caught before the commit; an empty-delta forced move of a seam point is caught;
   - the final two-sided proof of the split mesh passes with the entity values.
2. Scalar standing set: as TOL1 section 3 check 1 (G1, G2, GQ, G3, G4, the shared-engine gates, ONE G5 run S with its
   isolated restart, six payloads equal to `N/FP3_E/accepted_reference.json`). Equal list: as TOL1 section 3 check 2
   (one G5 run S, six payloads equal to the same reference, seam 0). Full OFF / CORE: "not evaluated".
3. A failed gate: R5.

## 4. The capability run CL4 (no gate, no reference)

After section 3 passed. Copy the CL3 controller and report (your `N/CL3/`) to `N/CL4/` and bind it to the TOL2 build.
Configuration: the sealed `N/CL3/run/run.cfg` unchanged (FARFIELD 1.0, SYMMETRY and WING3D 1E-4). Same rules,
watchdog and labels as CL3 with "CL4". The report has the CL3 tables with the CL2 and CL3 numbers beside each CL4
number, and per event also: `seam_splits`, `seam_points`, the refused seam edits; the size census (the edges above
1.8 with their marker and whether they are seam edges); per entity the two final-proof distances and the tolerance
(write "not evaluated" when the event stops before the proof vote); the faces of each marker and the input farfield
points kept; the flat-cell census and the fate of the CL2 cap (TOL1 section 4); the native seconds per phase; the
shock measure with the unchanged script.
A failed CL4 check is a capability RESULT. A proven wiring defect (a seam split off its reference edge, a wrong value
in `NATIVE3D_TOLERANCE`) follows R5 on section 2, with the DEF1 rule for one repeat run.

## 5. Report and labels

`N/TOL2/reports/TOL2_<build>.md` and `.json`: the R4 list (every seam site with its new rule), the touched files, the
gates of section 3, the CL4 report (or its link). Labels: "TOL2: seam splits on the reference seam; standing and
equal-list checks passed" after section 3; then "CL4: loop run DONE ..." / "CL4: ... FAILED at event <k>, gate <n>" /
"CL4: ... OPEN" as in CL3; or "TOL2: FAILED at <gate>" (reverted). After the final acceptance only, replace in the
TOL1 bullet of `AGENTS.md` section 2a the words "their seam keeps its input points and edges" by "their seam keeps its
points and can only be split at points on the seam". Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`
and the timings handbook. Then STOP.
Rules R1-R13 hold. Expected time: a build, the gates (about 2 h), CL4 (about 1.5-2 h).

## 6. Not in this order

A repair of flat cells (CAP1). A change of the size gate. A loose value for SYMMETRY or WING3D. A change of the
optional metric cleanup or of the stop rules. A second CL4 run (except the R5 repeat).

## 7. Inputs

- Diagnosis (context, not an order): `reviewer_handoff/scripts/s12/diag_cl3_event1.md`.
