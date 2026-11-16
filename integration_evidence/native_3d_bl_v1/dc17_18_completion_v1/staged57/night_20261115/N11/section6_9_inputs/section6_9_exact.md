### 6.9 Additions for N11 and N10 (2026-11-16, 20:45; read before the first edit of N11; do not stop a running job)

Facts, from the G3 run of the group of 6.8
(`R/night_20261115/N8/A2_A3_N12/section6_8/validation/cases/section6_2_bl_20261116T181040_727116/run.log`):

- `NATIVE3D_WALL_LAW boundary=entry`: 73.6 s. It runs inside the clock of the wall
  stage (388.8 s). So the stage accepted 373 splits, not 457 as in the N9 run. The
  census costs about 0.5 ms for each edge incidence (the same in the full run: 10 to 16
  s for each rank). The reviewer did not estimate this cost. It was an error.
- `NATIVE3D_WALL_FRAME`: `RefitWallSplit` 408 calls, 0 valid, 9.3 s;
  `ReconstructWallSplit` 396 calls, 0 valid, 18.5 s; `ReconstructSplitPatch` 398 calls,
  380 valid, 26.7 s. The 380 valid results go to the guidance check a second time.
  How many of them are approved is not known.
- Error in the gate text of 6.6 items 4 and 5: "validity as in rule 1 (a)" has the two
  rows "at least 336 splits" and "the first 336 edits identical to P6c". N11 and N10
  change the choice of the edge and the approval. So these two rows cannot hold for
  them.

1. G3 validity for N11 and for N10 (replaces "validity as in rule 1 (a)" in 6.6 items 4
   and 5): all private cells positive with sensors; zero upper refinements; the modeled
   storage and the job peak inside their limits; no validity failure. The rows "at
   least 336 splits" and "the first 336 edits identical to P6c" are NOT gates of N11 and
   N10. The other rows of 6.6 items 4 and 5 stay: two runs, the same edits up to the
   smaller accepted count, the purpose rule of each item.
   The split count of N11 is a report. Reference for the report: the two numbers 457
   (N9 run, no census) and 373 (group run, with the entry census).

2. Print-only changes, in the SAME build as B2 (no extra build, no extra run):
   a. Remove the `entry` call of `PrintWallLawCensus`. The `exit` call stays. The rule
      of 6.8 item 2 for the `census` calls stays; the count of `NATIVE3D_WALL_LAW`
      lines is smaller now.
   b. For the edge-split trials of an `Adapt` call with a surface target: count the
      results of `validate_private` by the source of the patch. Four sources: `primary`
      (the result of `SplitEdge` or `InsertEdgeCavity`), `RefitWallSplit`,
      `ReconstructWallSplit`, `ReconstructSplitPatch`. Line at the stage exit:
      `NATIVE3D_WALL_SOURCE rank= source= validated= guidance_rejected= approved=`.
      No decision reads it. Report the sums beside `guidance_trials`,
      `guidance_rejected` and `accepted`. A difference of the sums is a print-only
      contradiction (R4): write it, continue.

3. N11 purpose rule: no change (geometry queue, guidance rejections / guidance trials
   below 0.70 in both runs). References: 0.82 (N9: 1,170 / 1,425) and 0.83 (group run:
   974 / 1,169). These ratios count a choice two times when `ReconstructSplitPatch`
   gives a valid result. So report also the ratio of the source `primary` alone.

4. No change of a decision for the three joint operators in N11 or N10. The reviewer
   writes a text for them after the numbers of item 2 b.
