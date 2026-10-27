# Transfer operators — findings and validation ledger

Last updated: 2026-10-27. Maintained alongside implementation work.

This file covers solution-transfer operators only: geometric intersections,
conservative projection, coverage and boundary accounting, transferred state and
history, distributed consistency, and restart validation. Mesh-construction and
quality-operator findings belong in the adaptation handbooks.

## Current status

The clipping correction and regressions are committed and pushed in
`6aee162837` on `rois1995/SU2`, branch `codex/native-3d-bl`.
The saved failing geometry passes a focused overlap replay after the arithmetic
correction. Integrated build15/session90655 is still running; the corrected
transfer/restart workflow and broader serial/MPI tests are **not yet validated**.
Full M6 transfer remains unqualified because no complete accepted M6 mesh exists.

## TR-001 — Subnormal clipping distances lose valid intersections

**Status:** confirmed defect; shared correction implemented; integrated validation pending.

- Trigger: the curved RANS runtime fixture produces finite boundary coordinates
  containing `-denorm_min`. Build14 completed remeshing, but its BDF2 transfer
  check failed with relative error `0.0013328061` against `2e-9`.
- Cause: polyhedron clipping used `(wk*xk + wr*xr)/(wk+wr)`.
  Subnormal distances can underflow weighted coordinates or overflow an optimized
  reciprocal. Nonfinite intersection volume then failed the positive-volume
  predicate and was silently treated as an empty intersection.
- Impact: genuine donor coverage disappeared and created artificial fill/loss
  regions. This is demonstrated independently of donor search and marker
  classification by replaying all 6 donor × 128 target cell pairs.
- Correction: shared `CutFraction` normalizes signed distances by an exact power
  of two before division. Polygon and polyhedron clipping both use it;
  polyhedra interpolate coordinates with normalized weights. Conservative kernels
  explicitly reject nonfinite raw and dual-piece volumes.
- Scope: `SU2_CFD/include/adaptation/ConvexClipping.hpp`,
  `SU2_CFD/src/adaptation/ConservativeKernels.cpp`, and
  `UnitTests/SU2_CFD/adaptation/CConservativeTransfer_tests.cpp`.
  No conservation tolerance, marker rule, mesh input or acceptance gate changed.

Saved all-pair replay:

| Quantity | Before correction | After arithmetic correction |
| --- | ---: | ---: |
| Raw overlap volume | NaN | 0.99999999999999933 |
| Summed dual-piece overlap | 0.97888950038997369 | 0.99999999999999956 |
| Target fill | 0.022975264580252234 | 0.0018647649702263503 |
| Donor loss | 0.021110499610026284 | 3.6082248300317588e-16 |

The target volume is approximately `1.0018647649702259`: its curved boundary
expands outward. Nonzero target fill is therefore expected; vanished donor
coverage is not. After correction, each of the six donors has coverage `1/6`
to roundoff. The replay establishes the arithmetic correction, not the integrated
nonfinite-error path or the complete transfer workflow.

Three added regressions cover subnormal endpoint cuts, finite halfspace
coefficient scaling in 2D/3D, and the exact first failing curved cell pair with
finite moments, reciprocal intersection symmetry and dual-piece partition checks.
Their test tag is `[ConservativeClipRobustness]`.

Detailed diagnosis and reproducer provenance:
[clipping underflow](TRANSFER_CLIPPING_UNDERFLOW.md).
Evidence root:
`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/`:

- `cases/connected_fix_20261027T082636_1472866`: failing integrated run,
  saved mesh/state/projection outputs and source pins.
- `transfer_coverage/`: all-pair reproducer, literal target cells and before/after
  logs. Its original `command.json` describes the baseline build; do not treat it
  as a complete manifest for the corrected replay binary.
- `build15_*`: integrated rebuild receipts, currently incomplete.

## TR-002 — Constant preservation alone misses coverage defects

**Status:** demonstrated validation limitation; stronger regression added,
integrated qualification pending.

The Euler constant field remained constant despite TR-001's lost overlap.
Open-boundary fill/loss compensation can conceal the coverage defect in that
check. Constant preservation remains necessary but cannot establish conservation
or correct geometric coverage by itself.

Keep independent checks of finite overlap, per-donor coverage, raw versus
dual-piece volume/moments, and physical fill/loss accounting. Retain the original
curved RANS BDF2 conservation expectation; do not rescale it to the erroneous
boundary ledger. Distributed projection and restart checks remain separate
obligations.

## Pending validation and improvements

- Finish build15 and run the connected curved Euler/RANS transfer and reload
  checks with unchanged expectations, including all three clipping regressions.
- Run appropriate broader conservative clipping/projection/transfer regression
  in serial and MPI6 at the integration milestone; save source, binary, input,
  configuration and helper provenance with terminal outcomes.
- Verify complete accepted M6 Euler and RANS transfer, history and restart once
  mesh construction supplies admissible meshes. Fixture success cannot close this.

## Maintenance rule

Update the current status in place when evidence changes. Give each new confirmed
bug or improvement a stable `TR-` identifier, its trigger, affected operator and
callers, correction, evidence and remaining validation. Separate demonstrated
defects from hypotheses. Record failures as well as passes; a build or focused
reproducer alone does not close an integrated transfer finding.
