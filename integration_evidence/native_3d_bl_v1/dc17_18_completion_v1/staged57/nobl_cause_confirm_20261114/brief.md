# Brief: confirm the cause of the slow no-BL path on build 494 (one test build, one short run)

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
BP = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`.

## 1. Task

Test ONE hypothesis. Report. Stop.

- This is a diagnostic test. It is not a fix. Do not commit the test change to the branch.
- Do not change a gate, a budget, a limit or a config value.
- Do not overwrite the pinned build 494 binary or its build directory.
- Do not design. Do not recommend a fix.

## 2. Why

The unprofiled check (`Handbook/ADAPTATION_M6_NOBL_CLEAN_CHECK_20261114.md`, evidence
`R/nobl_clean_check_20261114`) shows this for the first collapse pass of the no-BL M6 case:

| Quantity (sum of the 6 ranks) | Build 293 | Build 494 |
|---|---|---|
| Upper-bound work | 54,229,713 | 1,034,769,513 |
| Upper-bound calls | 19905 | 26642 |
| Accepted edits | 2495 | 2496 |
| Collapse exit, rank 0 | 194.72 s | 1410.69 s |

Hypothesis: commit `76f10c8310` causes this. In `BoundaryParameterState::EntityFaceError`
(BP:368-378) the `tighten` lambda now asks for `SurfaceBoundPurpose::ErrorMagnitude` for each
face above 1 x tolerance. Before, it asked for a classification with ceiling 2 x tolerance,
only for faces above that ceiling. The source comparison of the check report ranks this commit
first. Nobody tested it.

## 3. The test

1. Make a test build that differs from the build-494 source pins in the `tighten` lambda only.
   Put back the code of the parent of `76f10c8310`:

   ```cpp
   const auto ceiling=2*state.GeometryTolerance();
   if(std::isfinite(ceiling)&&upper>ceiling)
     upper=std::min(upper,g.BoundPhysicalSurface(support,{p},ceiling,*budget));
   ```

   Use a separate worktree or a separate build directory on the 4TB disk. Same compiler,
   same options as build 494. Record the diff and the SHA256 of the new binary.
2. Run the same case and command as `R/nobl_clean_check_20261114/run/provenance.json` with the
   test binary. New case directory. No profiler. nice 19. One owned heavy job. At most 6 heavy
   processes on the machine. Record the host load every 10 s.
3. Stop your own run after rank 0 writes the `length_collapse_exit` record of the first
   adaptation and all ranks have written their collapse counters. Verify that the handle is
   terminal. If the record does not come in 1500 s, stop the run and report that.

## 4. What to report

**T1.** The table of section 2 with a third column for the test build.

**T2.** Rank 3: proof seconds, proof work, accepted collapses (build 293: 28.65 s, 4.56 M, 263;
build 494: 934.26 s, 830.25 M, 263).

**T3. Verdict, one of three:**
1. Confirmed: the upper-bound work of the test build is at most 2 times the build-293 value.
2. Partly: the work is lower than build 494 but more than 2 times build 293. Give the ratio.
   Name the next candidate from the source comparison.
3. Not confirmed: the work stays near the build-494 value.

**T4.** Does the test build make the same accepted edits as build 293 in this pass (count and,
if the logs permit, the same collapse ids on rank 0)?

## 5. Rules

- Numbers come from this run and from the saved logs.
- Stop only if the build fails or the run does not start. Then report.
- The test change undoes a correction that build 481 made for the BL surface stage. So it must
  not go into the branch. Say this in the report.

## 6. Report back

- One file in `Handbook/`, 40 lines at most, with T1 to T4.
- Small evidence under R with provenance (diff, binary hash, command, host receipts).
- One line in `HANDOFF_Codex.md` and in `HowToProceed.md` that points to the report.
- Then stop.
