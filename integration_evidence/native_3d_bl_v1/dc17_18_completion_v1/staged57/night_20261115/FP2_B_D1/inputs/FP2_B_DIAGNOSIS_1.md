# FP2-B diagnosis run D1: where the near-zero edges come from, and which error ends event 2

Author: reviewer, 2026-11-22 (checked against the source and your evidence). For the main agent. The user sends it; it
is a user order. Copy this file with its SHA256 into `R/night_20261115/FP2_B_D1/inputs/`.
This item is a DIAGNOSIS. Nothing is accepted by it. It adds print-only rows to production code for ONE run and takes
all of it out again at the end. No gate G1 to G5. One source check (R4) before the first edit; questions R13.
Line numbers are those of commit `ccd9d60593`. `Run.inc` = `Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`;
`Surface.cpp`, `Pass.cpp`, `Core.hpp` = the files of the local core as before.

## 1. Why

Your run S1 of build633 and your revert (`240ca96db8`) were correct. The reviewer's reading of the logs:
- Event 1 passed (1,667.955 s). Event 2 did not end by a limit. Rank 0 threw "Incomplete quality refresh edge
  measurements ... metric_or_height_cells=625" (`EngineSelectionCensus.cpp:264`): `MeasureCell` got a patch metric
  that is not complete for one cell, because `ValidateMetricPatch` (`CNativeMetric3D.cpp:437-455`) caught an exception
  of the metric measurement and wrote its text into the local `reason` of `MeasureCell` (`EngineCensus.cpp:173`),
  which the improvement path never uses. That text is in no log.
- The first number that differs from run CORE: the smallest metric edge of the first `NATIVE3D_LENGTH` row of rank 0
  in event 2 is 3.5e-9 (CORE: 0.0199). Event 1 has 5.45e-10 on rank 2, the same value as S1 of build627, although the
  surface rules changed between the two builds. The lab (N1, N6) has no edge below 1e-2, but the lab uses another
  metric. So the lab gate has a blind spot, and the origin of these edges is not known: the input of the event,
  step L (it cuts by the metric length and did not change), a placement, the volume work, another metric in the
  engine, or the first collapse pass of the engine.
This run prints what is missing.

## 2. The code state for the run

1. Bring the FP2-B code that ran as build633 back, production and test code, with nothing else:
   `git checkout ccd9d60593 -- Common SU2_CFD UnitTests`. Do NOT `git revert` the commit `240ca96db8`: it also added
   the S1 evidence. Check: `git diff --stat ccd9d60593 -- Common SU2_CFD UnitTests` is empty, and 9 paths differ from
   `240ca96db8` (4 under `Common/`, 5 under `UnitTests/`). Do not add untracked files. Label each commit of this item
   "FP2-B D1 diagnosis; default path not proven; not accepted".
2. Add the print-only rows of section 3 (R11: only when the report switch of the place is on; no decision reads
   them; no new field in a row that travels, in `Session`, in `CellState`, in a type of `Mesh::BytesPerCell` or
   `SurfaceArrayBytes`; local variables only). Exception for P2 only: one print-only sink field
   `std::function<void(char step, const std::string& row)> surface_near_sink;` in `PassStats` (beside `surface_sink`),
   and one defaulted last parameter `std::string* near_row = nullptr` of `Mesh::SplitSurface`. No other new field or
   parameter. Each new print catches every exception and never throws into the work (`Length` throws for a zero or
   non-finite edge; a throw of a print would change the result).
3. Build `SU2_CFD` and `UnitTests/test_driver` without a new warning. Run fixtures (a) to (d) of FP2-B correction 1
   section 2 ((d) on the cylinder only) and the A1 fixtures one time; they must pass as on build633. (They do not
   use the new sinks; the real check that the prints change nothing is the equality of section 4.)

## 3. The print rows

P1. `NATIVE3D_LOCAL_SHORT`, printed from `RunLocalCore` (`Run.inc:848-945`) only when `report` (= `control.timings`,
    `Run.inc:870`) is on, by one new print function beside `PrintLocalPass`. It takes `const core::Mesh&`, rank, stage
    (`stats.normalization_stages - 1`), pass, a checkpoint name and the input point base. Its body follows the lab's
    `ReportEdges` (`UnitTests/SU2_CFD/adaptation/fixtures/native_handover.inc:169-212`), with the 5 shortest edges in
    place of 20. Fields: the counts of edges with `Length` below 1e-6, 1e-4, 1e-2 and 0.1; for each of the 5 shortest
    edges, the metric length, the length in space, both ids, both coordinates, `surface`, `border` and `new` of each
    end (new = id above the input point base), and `physical` (the edge is an edge of a face with
    `neighbors[k] == Physical`). The input point base is the value of
    `workers.reduce<uint64_t>(point_base, CPassiveComm::Op::MAX)` at `Run.inc:864`. Keep it in a local and give that
    local to `point_ids`. No second collective. Print it once in each row as `input_point_base=`.
    The function catches every exception and then prints `error=<text>` in place of the edge fields.
    Checkpoints, in each pass:
    - `c0`: after `pass_mesh.MarkBorderSurface(meeting)` (`Run.inc:895`), before `RunPass` (the input of the pass);
    - `c1`: after the surface steps. Set `counts.surface_sink` inside the `try`, after `pass_mesh` is built
      (`Run.inc:893-895`), to a lambda that calls the print with `pass_mesh`. Do this only when `report && surface`.
      `SurfaceWork` already calls this sink at `Surface.cpp:675` (after `SurfaceLength`, `SurfaceDistance` and
      `MarkUnresolved`, before `LengthWork`). `RunPass` does not change. Reset `counts.surface_sink` to empty directly
      after `RunPass` returns;
    - `end`: after `RunPass` returns (`Run.inc:900` / `902`), before `WriteBack` (`:905`);
    - `shifted` (once for each stage): after the pass loop (after `Run.inc:943`), before `InstallLocalSurface`
      (`:944`). Build a temporary `core::Mesh probe(working, workers.size > 1)` inside its own `try`, print, and
      destroy it. This is the mesh that the engine gets (the shift moves whole rows and changes no coordinate or
      tensor, `CNativeLocalCore3DShift.cpp:231-276`).
P2. `NATIVE3D_LOCAL_NEAR`. In `SplitSurface`, after `frozen_.push_back(...)` (`Surface.cpp:338`), only when
    `report && near_row`, a new private const member of `Mesh` writes the row text into `*near_row` when (i) the
    smallest distance in space from m (slot `slot`) to a vertex other than a, b and m of the cells of `Star(slot)` is
    below `geometry.Limit()`, or (ii) the smallest `Length` of an edge at m in `Star(slot)` is below 1e-2. Fields:
    - a, b, m: ids and coordinates;
    - `feature` (the local of `Surface.cpp:332`): 1 = feature, 0 = sheet;
    - the nearest vertex: id, coordinates, `surface` and `border`; that distance; `geometry.Limit()`;
    - |a b| in space; `Length({a, b})` and the smallest `Length` at m;
    - for each of the two faces of `old_faces` (`Surface.cpp:246`), the three point ids and the `facet`.
    After `geometry.Accept(std::move(placement))` (`:337`), do not read `placement` or `point` (`:258`, a reference
    into the moved placement). Use `middle`, `points_[slot]`, `edge`, `old_faces` and `feature`. Call no function of
    `geometry` other than `Limit()` (another call changes the call sequence that fixture (d) watches, and the caches
    of `Samples` and `Chord`). Catch every exception inside the member. The two callers (`Surface.cpp:397` step L,
    `:603` step D) pass a local string only when `stats.report && stats.surface_near_sink`. After an accepted split
    with a non-empty string they call `stats.surface_near_sink('L' or 'D', row)`. The two test calls of `SplitSurface`
    (`CNativeLocalCore3DSurface_tests.cpp:471`, `:703`) keep the default.
    In `RunLocalCore`, when `report`, set `counts.surface_near_sink` to a lambda. It prefixes
    `NATIVE3D_LOCAL_NEAR rank=<r> stage=<s> pass=<p> step=<L|D>`, prints at most 50 rows, and counts the others in a
    local of the pass loop. After `RunPass`, print
    `NATIVE3D_LOCAL_NEAR_LEFT rank= stage= pass= printed= left_out=`.
P3. `NATIVE3D_METRIC_REJECT`, in `EngineCensus::MeasureCell` (`EngineCensus.cpp:159-226`), directly after
    `ValidateMetricPatch` (`:199`) and before `HeightCheck` (`:203` overwrites `reason` when the height fails). Print
    only when all of these hold: `options.timings` (`context.options`, `:169`); `!metric_valid`;
    `metric.edges.size() != 6 || !std::isfinite(metric.maximum_length)` (the throw condition of
    `EngineSelectionCensus.cpp:263`); and `selection.census.cells.count(id)`. Fields:
    - `context.world.rank`, the phase, the cell id;
    - for each vertex `cell.v[k]`: id and coordinates; the centroid;
    - `metric.edges.size()` and `metric.maximum_length`;
    - the six lengths in space of its edges;
    - the six cached engine metric lengths `selection.census.edges.at(key).measure.length` for the keys
      `selection.census.cells.at(id).edges` (`EngineSelection.hpp:72`), with `-1` for a key that is not there;
    - the text of `reason` (quoted).
    Read only; no change of `ValidateMetricPatch`. `RefreshQuality` then throws the same exception as before.
P4. `NATIVE3D_SEED_SHORT`, in `SeedAndRun` (`EngineAdapt.cpp`) after the seeding loop (`:258-262`), before
    `return RunRounds(...)` (`:263`), only when `context.options.timings`, once for each run of the engine and rank
    (report item 3 uses the first row of each stage). Fields: `cells=` (`working.size()`), `measured=`
    (`selection.census.cells.size()`), `edges=` (`selection.census.edges.size()`), the counts of
    `edge.second.measure.length` below 1e-6, 1e-4,
    1e-2 and 0.1, and the 5 shortest (ids from the key, `length`, `measure.a`, `measure.b`). Read only (the same loop
    as `EmitLengthPass`, `EngineReports.cpp:907-917`). This is the engine's own metric on the mesh that it gets,
    before its first collapse pass.
If one of these needs a change that is not print-only: do not make it; print what is possible and write what is
left out.

## 4. The run

One run S1 with the config of FP2-B (`R/night_20261115/FP2_A/run.cfg`, SHA256
`07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382`), as the next owned job. Use the S1 controller of
build633 with only its paths and pins changed (R3). Its stop after a failed gate is the end of the run. Do not stop
the job by hand. Save the run under `R/night_20261115/FP2_B_D1/G5/build<N>/S1/`. The run is expected to fail in
event 2 as build633 did; if it passes, write that.
Sanity (it shows that the prints changed nothing): the integer fields of the rows `NATIVE3D_LOCAL_SURFACE` of event 1
are equal to those of S1 of build633; the SHA256 of the three event-1 files (`adapted_mesh_adap_00001.su2`,
`.native_ref`, `restart_flow_adap_00001.dat`) equals that of the same files in
`R/night_20261115/FP2_B/G5/build633/S1/full633_run1/`; and the first `NATIVE3D_LENGTH` row of rank 0 in event 2 has
the `minimum_length` of build633 (3.5e-9). If not, write the first difference; event 2 is then another case, and its
rows are still reported.

## 5. Report

`R/night_20261115/FP2_B_D1/reports/FP2_B_D1_<build>.md`:
1. Event 2: each row P3; for each vertex of the rejected cell: is it an input point of event 2 (id not above
   `input_point_base`), a point of a surface split of the core, or another new point (the rows P1 / P2 that hold it,
   if any); the shortest edge of the cell; the hidden reason.
2. Event 1 and event 2, for each stage: the rows P1 of each rank and pass (`c0`, `c1`, `end`) and `shifted`. For each
   edge below 1e-2: where it first appears (`c0` = it is in the input of the pass; `c1` = the surface steps made it:
   give the row P2 that made it, or "no P2 row"; `end` = the volume work made it).
3. For each rank, stage and event: the smallest edge of P1 `shifted` (core metric), of P4 (engine metric, the same
   mesh) and of the first `NATIVE3D_LENGTH` row (after the first collapse pass of the engine). If P4 is much smaller
   than P1 `shifted`, the metric of the engine differs; if the LENGTH row is much smaller than P4, the first collapse
   pass made the edge: write which.
4. The counts of P2 rows by step (L / D) and by kind (sheet / feature), with `left_out`.

## 6. Close

Revert in ONE commit ("FP2-B D1 reverted; production sources = build627"):
`git restore --source=240ca96db8 --staged --worktree -- Common SU2_CFD UnitTests` (unlike
`git checkout <rev> -- <paths>`, this also deletes the two test files that FP2-B added:
`native_local_surface_probe.hpp`, `fixtures/native_volume_handover.inc`). Verify:
- `git diff --stat 240ca96db8 -- Common SU2_CFD UnitTests` is empty;
- the SHA256 of each path of `R/night_20261115/FP2_A/build627/source_pins.json` outside `UnitTests/`: 0 differences;
- `git diff --stat 852e566d17 -- Common SU2_CFD` is empty;
- rebuild `SU2_CFD` and `UnitTests/test_driver`: both build, and their SHA256 equal `binary_sha256` and
  `test_binary_sha256` of `R/night_20261115/FP2_B/build634/build_status.json` (your build after the revert
  `240ca96db8`).
Update the progress file and one line each in `HANDOFF_Codex.md` and `HowToProceed.md` (production = build627; the
report path). Push, verify that no owned job runs, stop and wait for the user.

## 7. Not in this item

A fix of any kind. A change of a rule of the core or of the engine. A gate. A second run.
