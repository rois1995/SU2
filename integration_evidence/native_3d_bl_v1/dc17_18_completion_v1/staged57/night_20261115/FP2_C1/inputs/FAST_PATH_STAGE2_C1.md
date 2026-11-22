# Fast path stage 2, item FP2-C part 1: the length collapse of the core also at the wall (pure volume)

Author: reviewer, 2026-11-22 (checked against the source; the design was reviewed by a second model). For the main
agent. The user sends it; it is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_C1/inputs/`.
One whole-item source check (R4) before the first production edit; questions R13.
Code state: if FX2 is not finished when this order arrives, finish FX2 by its own text first (R12). Then S0 = HEAD
after FX2's last commit. If its label is "FX2: accepted", S0's build is FX2's accepted build. If the label is "FX2:
FAILED", check that `git diff --stat c843610249 -- Common SU2_CFD UnitTests` is empty. If FX2 stopped without one of
these two labels: do not start, write it in the progress file, and wait. Write S0 and its build in the progress
file. Line numbers are those of `c843610249`; FX2 changes only `CNativeAffineReference.{hpp,cpp}`,
`CNativeBoundaryParameters3D.cpp` and test files, so the lines named here are the same on S0.
Short names: `Core.hpp` = `CNativeLocalCore3D.hpp`, `Changes`, `Pass`, `Mesh` = `CNativeLocalCore3D<name>.cpp`,
`Run` = `NativeRemeshRun.inc`, `VH` = `UnitTests/SU2_CFD/adaptation/fixtures/native_volume_handover.inc`, `Box` and
`RHO` = your test files that hold `Box()` and `BoundMesh` / `LayeredBipyramid`.

## 1. Why

FP2-B is accepted (event 1: 650 s; run CORE: 1,509 s). The next block is the construction of the engine, about 210 s
in each event; about 120 s of it is its collapse pass. No gate needs these collapses (the size gate counts long
edges); they keep the mesh small: about 5,000 points and 17,000 cells less in each event. The engine makes them where
the core does not work: at the wall and on the surface. User decision of 2026-11-22: the core gets these collapses,
and then the engine makes no construction collapse when the surface core ran.
This is part 1: the collapse of free points beside the wall. The collapse of surface points (part 2) and the change
of the engine (part 3) come as later orders; the reviewer needs the numbers of this part for their design.
The rule that this part removes is from stage 1 (section 6.2): the core did not collapse a free point beside the
surface, because the surface was not yet at its target, and that text says "stage 2 removes this rule". Since FP2-A
the surface steps run before the volume work, and the core runs only without wall heights (`applicable`).

## 2. The change (production; pure volume; under the surface option only)

1. `CoreOptions::wall_collapses` (default false); `Run` sets it together with `surface_splits` (`Run:1010-1011`).
   With `ADAP_NATIVE_LOCAL_SURFACE= NO` nothing changes: the run with both options NO and the run CORE keep their
   six files (this follows from `Run:698` and `:1014`; no run of OFF or CORE in this part).
2. `CanCollapse(p) = Free() && (options_.wall_collapses || !wall_near)` (`Changes:183`). Nothing else changes: its
   three readers (`Pass:163`, `Pass:177`, `Changes:190`) are the length collapse sweeps; the tests stay as they are
   and in their order (eligibility; `Prepare` with `Blocked`; the size test: no new edge above `LongEdge`; the shape
   test; exact positivity). `CollapseNear` of the V rounds reads `wall_near` itself and does not change.
   Comment at `CanCollapse`: the core runs without wall heights, and the surface steps ran before the volume work,
   so a wall-near free point may be removed. A free point is on no physical face; a physical face in its star keeps
   its oriented vertex triple and its binding in a new cell, or `Prepare` refuses the change.
3. Print only (R11): new fields of `PassStats` (counted with `Increment`, so only under `report`), printed at the end
   of the row `NATIVE3D_LOCAL_CORE` (`PrintLocalPass`, after `bytes=`):
   - `collapses_wall_near`: the commits of `Collapse` whose removed point is wall-near (not the V rounds);
   - `refused_size_wall`, `refused_shape_wall`, `refused_topology_wall`, `refused_valid_wall`, `blocked_wall`:
     counted in `Collapse` (`Changes:223-245`) for each direction separately: a direction whose removed point has
     `wall_near` set (the mark of the sweep start) and whose plan is refused adds 1 to the counter of its reason
     (`Size`, `Shape`, `Topology`, `Validity` = the float test of `Prepare`, or `Blocked`). If the chosen direction
     removes a wall-near point and the exact test refuses it (`Changes:235-237`), `refused_valid_wall` gets 1 more;
   - `four_surface_cells`: live cells whose four points have `PointRow::surface` set, counted in the cell loop of
     `ReportResult`;
   - from `ReportResult`, the short edges by class (each short edge counts once; the four sum to `short_out`):
     `short_free` (an end that is free and not wall-near), else `short_wall_near` (an end that is free and
     wall-near), else `short_border` (a border end and no free end), else `short_surface` (both ends surface
     points).
   No field in a type of `Mesh::BytesPerCell`, in a row that travels, in `Session` or `CellState`; no timer operation.

## 3. Fixtures (serial; hidden test cases, tags `[.][NativeFP2C1]`, so that the case table of G1 stays identical)

(a), (c), (d) go in `Box`; (b), (e) go in `RHO`. No new file. Each fixture that calls `Collapse` sets the option
where it says so and calls `MarkWallNear()` first.
(a) On `Box()`, the first-layer pair p = (3/8, 1/8, 3/8), k = (3/8, 1/8, 1/2) (both free and wall-near; the metric
    length is about 0.6). Without the option: `Ineligible`. With it: accepted, `collapses_wall_near == 1`. The star of
    the removed point holds the two physical faces of the wall square on y = 0 under it. After `WriteBack`,
    `Surface(rows) == Surface(initial)`: the same oriented triples and facet ids. `Check` passes. Report the pair
    x = 1/4 and 1/4 + 1/128 (z = 3/8) as one line: accepted or refused, and the reason.
(b) A 4-cell mesh (`BoundMesh`, identity metric): T = (k, a, b, c) with k = (0,0,0), a = (1,0,0), b = (0,1,0),
    c = (0,0,1), and p = (1/8, 1/8, 1/8); the cells are p joined to the four faces of T; every face of T is physical
    with its own facet. `Collapse({p, k})`: without the option `Ineligible`; with it accepted. The one cell left is
    T; its four faces have the oriented triples and facet ids of the four physical faces before. `Check` passes.
(c) On `Box()`, with the option: p = (3/8, 1/8, 3/8) and k = (3/8, 0, 1/4) (the corner of p's wall square on y = 0
    that is a vertex of only one of its two triangles). `Collapse`: refused `Validity` (the cell of the other
    triangle becomes flat); `refused_valid_wall == 1`; the snapshot of the mesh is equal before and after.
(d) On `Box()`, the pair p = (1/2, 3/8, 3/8), k = (1/2, 3/8, 1/2) (both free and wall-near through the wall x = 1;
    the metric length is about 0.6). Without the option: `Ineligible`. With it: refused `Size` (each direction makes
    a new edge of about 2.4 in the metric to a point of x = 1); `refused_size_wall == 2`.
(e) The inner six cells of `LayeredBipyramid(.25)` (only the cells `{5, a, b, tip}`), so that points 0 to 4 are
    surface points and point 5 is free and wall-near; the options of the existing shape fixture plus the option.
    `Collapse({5, 0})`: refused `Shape`; `refused_shape_wall == 1`; the snapshot is equal. Without the option:
    `Ineligible`.
(f) The option off: each fixture of FP1, FP2-A and FP2-B gives the same results as on S0 (the cohort of
    `R/night_20261115/FP2_B/scoped/build640/fixtures.json`, and the FX2 fixtures if FX2 is accepted).
The expected reasons of (a) to (e) come from a hand computation of the reviewer. If one cannot be built or gives
another reason inside R3: build the nearest case that shows the same rule and write the difference.

## 4. The lab (after the fixtures; BEFORE a full run)

Runs N1, N6 and N6P (valid, as in FP2-B) with the production `RunPass`. The lab option line (`VH:281`) serves every
case of `VH`: set `options.wall_collapses` there together with `surface_splits`. Each case TWO times.
Two new rows (test code):
- after the row `NATIVE3D_LOCAL_SURFACE` of each pass, on each rank: `FP2_C core rank=<r> pass=<p>` with these
  `PassStats` fields: `cells_in cells_out points_in points_out surface_points border_points wall_near_points
  collapses splits edge_removals face_swaps moves deferred skipped_memory evaluated_collapse refused_valid
  refused_size refused_topology refused_shape refused_exact blocked short_out edges_out bad_in bad_out
  sweeps_collapse sweeps_split sweeps_quality rounds limit round_changes`, then the new fields of section 2 item 3,
  then `seconds_collapse seconds_split seconds_quality`;
- at the end of each case, on each rank: `FP2_C cells rank=<r> cells=<n> digest=<d>`: d is the 64-bit FNV-1a hash
  over the owned cells sorted by their four sorted point ids; each cell adds its four sorted ids and, for each of
  its points, the id and the bit patterns of x, y, z (the 8 little-endian bytes of each value, in this order).
Conditions (the hand-over gate of FP2-B correction 3, as it was on build640): each cell exactly positive; the check
of 8.4; the proof condition with `classify_unexplained=0`; N6 and N6P: the bytes of `Snapshot()` equal on all ranks;
no core metric edge below 1e-2 at the end of the last pass; `star_max` not above 1,000; `sweeps_distance` below 40;
`v_limit=0`; the donor scan 0; no pass above 600 s; each run inside its 45-minute wrapper; N6P valid.
And the two runs of each case are equal: in the rows `NATIVE3D_LOCAL_SURFACE`, `FP2_B pass`, `FP2_B sweep`,
`FP2_B bad_before_v`, `FP2_B bad`, `FP2_B state`, `FP2_B donor_scan`, `FP2_B end`, `FP2_B partition` (N6P), the
proof rows (in N1 `FP2_A LAB_S_PROOF`; in N6 and N6P `FP2_B proof` and `FP2_B proof_entity`), `FP2_C core` and
`FP2_C cells`, each field with an integer value is equal, except fields whose name starts with `seconds` or contains
`rss`. Compared by a test-code script `R/night_20261115/FP2_C1/compare_lab.py` that takes the list of row names as
an argument.
The surface must not change in pass 1 by this part: every field of the pass-1 row `NATIVE3D_LOCAL_SURFACE` except
`v_rounds v_limit v_collapses v_splits v_changes bad_before_v seconds_v` and the fields that start with `seconds_`
(these come after the volume work; all others are set in `SurfaceWork`, `Pass:478`, before `LengthWork`), and the
pass-1 rows `FP2_B sweep` of the kinds `surface_length` and `surface_distance`, equal those of the same lab on S0
(the runs of section 5 item 1). If they differ: a wiring error.
Report beside the same lab on S0, for each run, rank and pass: the row `FP2_C core` with the new counters (on S0 the
wall counters are 0 by construction; the short classes and `four_surface_cells` are reported for the new build
only), the bad cells by class before and after the V rounds, the V counters, the edges below 0.1 and the smallest
edge, `star_max`, the surface faces, the modeled peak. A rise of the bad cells is a finding, not a failure.

## 5. The order of the work

1. On S0, test code only (no production edit): the rows `FP2_C core` (without the new fields) and `FP2_C cells` in
   `VH`. Build `UnitTests/test_driver`, then run N1, N6 and N6P one time each. Commit ("FP2-C1: S0 lab reference;
   default path not yet proven"). These rows are the S0 reference of section 4.
2. The change of section 2 and the fixtures. Build `SU2_CFD` and `UnitTests/test_driver` without a new warning.
   Fixtures of section 3; all fixtures of FP1; G1 identical (375 / 114 / 228). Commit and push ("FP2-C1: fixtures
   passed; default path not yet proven").
3. The lab of section 4. Commit and push ("FP2-C1: lab passed; default path not yet proven").
4. One run S (both options YES; the config `R/night_20261115/FP2_A/run.cfg`, SHA256
   `07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382`, copied into `FP2_C1/`; the runner is a copy
   of `FP2_B/run_fp2_b_g5.py`; the isolated restart config derived as in FP2-A 11.3), all standing gates of both
   events. The six files are not a gate (the mesh changes by design); write their SHA256. The reference of the
   report is FX2's run S if FX2 is accepted, else `R/night_20261115/FP2_B/G5/build640/S1/full640_run1`
   (649.954 / 446.547 s). Report beside it, for each event: the native maximum; the cells, points and proof faces of
   the final mesh; the rows `NATIVE3D_LOCAL_CORE` with the new counters; the engine's collapse pass (seconds,
   rounds, `collapse_accepted`); the engine seconds of construction, of the quality phase and of each geometry
   repair; q / J at `quality_entry`.
5. After the run: commit and push the evidence ("FP2-C1: S passed; default path not yet proven"), update the progress
   file, `HANDOFF_Codex.md` and `HowToProceed.md`, verify that no owned job runs, STOP and wait for the user. Do NOT
   start a default proof or a second run S: the reviewer decides from the numbers how parts 2 and 3 follow; nothing
   is accepted before the default proof and two equal runs.
In this part no step reverts (this user order replaces the revert of R5 for all of FP2-C1). At a failed gate: one
diagnosis pass (R5, read-only). Only for a wiring error against this text (item 2, and the pass-1 surface rule of
section 4): one correction and one repeat. Then commit the evidence with "FP2-C1: <step> FAILED; default path not yet
proven", update the progress file, verify that no owned job runs, STOP and wait.

## 6. Not in this part

The collapse of a surface point. A change of the engine. A change of `CollapseNear`, of the V rounds, of the surface
steps, of `Limit()`, of the tolerance, of the target. A default proof.
