# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** All owned jobs are terminal. No cluster job/request.
Foreign ClusterResults, AdapNoExt and dependencies remain untouched.

## Current qualified milestone

The actual short curved RANS/SA lifecycle now passes N/M1/1,2/1,2/2,4/1,4/2,4/4:
24 coarse tetrahedra ->1368 BL cells ->68/70/74/70/72/72 sensor-only cells.
Each case advances actual CFD, computes the pressure metric, remeshes/replaces,
transfers current/BDF/SA, writes SU2/CGNS, performs actual restart and advances again.
Independent exact geometry/embedding/height and physical flow/open-ledger/restart
checks pass every case. Maximum flow conservation residual2.53e-15, SA nodal replay
residual4.22e-15; BL complexity48.71302686078762..48.71302686078766 across ranks.
[Complete source/build/binary/input/helper evidence](integration_evidence/native_3d_bl_v1/region_construction_v1/rans_lifecycle_summary.json),
[workflow details and limits](Handbook/GOAL3_CURVED_RANS_RUNTIME.md).

The original v5 receipt remains FAIL on four SA assertions. Independent reference
projection and separate solver-floor replay prove their source: raw SA conserves,
then existing bounds clip200/56 values. The test now checks exact flow totals and
explicit SA correction, with actual field-scale normalization; production unchanged.
Post-clipping SA integrals are **not** claimed invariant. Exact old executed Python
helpers, failed receipt and raw states are preserved. Current cases end `_v6`.

Native geometry/h.05/growth1.2/thickness.2/fade, q>=.20/J>=.05/full L<=1.8,
4096-work/300s remain unchanged. Native shape/edge gates are enforced; the actual-flow
outputs do not yet have independent full composed-edge quadrature. These pressure
sensors clip to the coarse bound: this is not stronger-flow-feature qualification.
No converged viscous accuracy or lifecycle affordability/scaling claim.

## Next substantial construction step

A source-linked exact certificate proves the current single-kernel core cannot
resolve constant original-P1 sensor diag(1,1,16) over unit axial span: opposite-cap
metric distance>=4 means some kernel edge>=2>1.8. More wall variants or moving the
one kernel cannot fix this strategy. This is **not global target infeasibility**.
`region_construction_v1/single_kernel_metric_span_certificate.json` preserves the proof.

Implement a metric-driven volume core with several interior vertices/regions and
conforming shared interfaces, preserving coordinated wall/transition construction,
actual original-P1/geometric queries, private validation and atomic publication.
Use a complete accepted stronger-field curved mesh plus a distinct varying field as
the next meaningful gate. No local-fallback chain, budget ladder, target reset or
production Euler/BL mode. General/exterior/multiwall/high-ratio fronts, interacting
thin integration/composed-gradation limits, M6/viscous accuracy and performance remain.

## Prior qualified foundations

Original1402-cell target ->1368 MPI1/2/4, plus cold24 ->1368 ->68 with shared work,
are preserved in [region scope](Handbook/GOAL3_REGION_CONSTRUCTION.md) and
[cold evidence](integration_evidence/native_3d_bl_v1/region_construction_v1/cold_summary.json).
Actual source refresh1368 ->3072 donors ->72 cells ->288 donors ->1368 with affine
sensors passes MPI1/2/4, target residual<=1.67e-14. Original donors stay distributed;
actual query/segment footprint and resident cache<=256, no whole-source256 limit.
[Distributed service scope/evidence](Handbook/GOAL3_DISTRIBUTED_REGION_FIELD.md).
Current geometry still constructs one visible radial/caps region on one worker.

[Full scope](Handbook/GOAL3_CURRENT.md),
[requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[permanent rules](AGENTS.md),
[deferred SST/transition/nonideal questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
One owned heavy job, nice19/threads1/build-j1/MPI<=4; announce full case folders.
Group substantial changes; reuse appropriately pinned evidence, preserve failures,
avoid repeated full suites after small edits, and push reviewable checkpoints.
