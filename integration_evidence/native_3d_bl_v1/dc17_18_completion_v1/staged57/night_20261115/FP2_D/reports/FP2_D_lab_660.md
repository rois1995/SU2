# FP2-D continuation 2 lab, build660

**Lab passed.** All pairs matched exactly, including the two new metric-floor counters. The final all-edge census found zero edges below 0.01 in all six trials. N1Invariant checked all 8,636 accepted splits with zero violations and matched N1's result counters. [Lab acceptance](../cases/build660/lab_acceptance.json) · [Per-pass, per-rank and historical comparison](FP2_D_lab_660.json)

| Case | Seconds by trial | D splits | Max marked sweeps | Metric seed stops | Metric split refusals | Final faces | Final points | Minimum metric edge |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| N1 | 364.995 / 363.842 | 8,636 | 7 | 0 | 0 | 59,476 | 29,740 | 0.01914525 |
| N6 | 541.888 / 537.239 | 8,664 | 10 | 0 | 12 | 57,118 | 28,561 | 0.01001721 |
| N6P | 361.938 / 359.908 | 8,880 | 13 | 0 | 77 | 57,664 | 28,834 | 0.01004800 |

Both N6P trials recorded 105 above-tolerance faces and 136 residual faces in pass 2. The build655 reference had 324 and 601, giving first failing integers of 487 and 902. Both checks passed. The metrics in the table are observations at hand-over. The engine's contribution is evaluated in S.

The Short-face observer recorded zero rows in the completed trials. The N1 region report has zero captured Short faces in every region. In the three prescribed leading-edge station bands, along-span median edge lengths were 3.433, 3.805 and 3.089 mm, and across-span medians were 1.869, 1.613 and 1.485 mm. [N1Invariant receipt](../cases/build660/N1Invariant/measurement.json)

Build656 N1 had 40 marked sweeps and 1,105 final faces above tolerance. Build657 N1 had seven sweeps and zero above tolerance, but its N6 had five edges below 0.01. The per-pass comparison preserves those observations and records the absent historical columns as unavailable. The reused campaign controller's prose prefix says continuation 1; the build660 receipts pin both continuation orders and identify these executions as continuation 2. [Build656](FP2_D_656.md) · [Build657](FP2_D_657.md) · [Fixture checkpoint](FP2_D_continue2_fixtures_660.md)
