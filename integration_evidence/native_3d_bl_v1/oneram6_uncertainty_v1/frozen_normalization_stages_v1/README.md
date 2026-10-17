# Bounded frozen normalization/adaptation stages

Qualified scoped checkpoint on codex/native-3d-bl, base d9daf0e78f.
Full Goal3 ACTIVE/incomplete. No accepted complete M6 or general accuracy claim.
[Implementation, complete result and limits](../../../../Handbook/GOAL3_FROZEN_NORMALIZATION_STAGES.md).

## Evidence entry points

- `summary.json`: all8 required native groups and independent results.
- `same_field_stage_comparison.json`: identical corrected original P1/BL field
  before/after actual adaptation; includes the coarsening negative result.
- `attempt2_pilot_original_np1`, `attempt2_original_np2`, `attempt2_original_np4`:
  original unclipped family, clipping/gradation/reversion, no Hessian/flow mutation,
  aliases with unused CFD IDs, empty publishers and collective fault controls.
- `attempt2_pilot_runtime_np1`, `attempt2_runtime_np2_m2`,
  `attempt2_runtime_np4_m2`, `attempt2_runtime_np4_m4`:2 deliberately controlled
  initial normalization errors each; accepted RANS/SA+BDF/transfer/restart.
- `attempt2_affected_np4`: existing curved/reader/original-field reassessment cases.
- `validated_pilot_manifest.json`: seals the serial runtime/independent audit;
  `mpi_campaign.py` reuses its exact pins/artifacts without repeating that audit.
- Each case has source capsule, config/input mesh, provenance/result/log and
  inspectable outputs. Production input filenames retain their original6-cell mesh.

## Grids to inspect

Actual accepted meshes with original markers are `normalization_m*_a*_written_SU2.su2`
and `normalization_m*_a*_written_CGNS.cgns` inside each runtime folder. References
and flow restarts are retained. `*_accepted_mesh.su2` is the independent auditor's
additional export with diagnostic side-marker names. All dependencies are local
regular files. Cube geometry and imposed affine histories are correctness controls,
not a solved external aerodynamic RANS testcase or viscous-accuracy result.

## Results

All8 workflows accepted151--240 tetrahedra with unchanged q>=.2/J>=.05/L<=1.8,
height.1, exact geometry/unit volume and original P1/actual BL/finer/fade behavior.
Shared work433--1374<=8192, root calls6--7<=24, at most2 private stages,1 transfer.
Independent complexities19.93549--19.96127 against20:0.194--0.323% discrepancy.
The uncertainty is heuristic and roots remain integration_limited, not1e-6 accurate.
Final integral errors.00107--.06307 fall within indicators.08999--.22692.
Original nonhomogeneous field21.76446 ->20.00000: independent19.99506, indicator.04471.

Same original corrected field: refinement improves indicator3.07--4.07x; coarsening
ratios.915--1.206, sometimes worse. No monotonic accuracy or general thin/curved
claim. A finer candidate does not replace the original donor field.

## Bounded work and preserved failures

8192 points per assessment; geometry/publisher bindings are prepared once for root
trials; actual current-node tensors/BL/core are recomposed at those points. Serial
root counters49152/57344 density evaluations and8192 geometry preparations; spatial
queries during trials0. Preparation is still counted. Rank0 printed query counters
are local; phase/work totals span2 stages but engine detail mostly describes the
last. normalization.changed is a proposal flag; only stage count2 shows application.

Initial missing include/sparse-neighbor syntax failures are retained. Initial
`pilot_runtime_np1` reached a native-complete candidate, then its fresh work-cap
CConfig lacked actual marker order and failed import. The corrected fixture copies
tag/kind/send-receive bindings; no mesh gate changed. The77-operation shared-cap
rejection preserves CFD/reference. `campaign_attempt1.py` and executed capsules
preserve the initial source. The Path-type wrapper failure was before any MPI case;
`mpi_wrapper_path_failure.py` and its resource log are preserved. No failed case is
silently promoted. `design_wip_before_qualification.md` is historical draft scope.

Build24m25s plus driver freshness41s and test-fixture rebuild81s, all one worker.
Successful serial pilot69.35s, MPI+independent audits219.61s. Build process peak2.40GiB,
pilot~50MiB/MPI wrapper~48MiB process maxima, never aggregate job peaks. Actual host
~38--41GiB available; existing swap816MiB unchanged/GNU time swaps0. One owned heavy
job, nice19/thread1/MPI<=4;4TB compile temporaries. No CFD advancement, unchanged
full-M6 retry, full regression or cluster run. No performance/scalability claim.

Historical M6 minimum790200.50 +/-27958.45 against100000 remains provisional and
separate from construction. New code/controls do not requalify that old binary run;
no new M6 mesh/reference is asserted. Global error/default calibration, difficult
accepted M6/curved construction, predicted-family/2D correction, viscous accuracy
and complete event affordability remain open. Do not add another sampler or local
fallback to evade the known construction limitation.
