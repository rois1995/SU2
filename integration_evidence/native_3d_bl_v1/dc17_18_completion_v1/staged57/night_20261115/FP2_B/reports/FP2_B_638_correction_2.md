# FP2-B correction 2 · build638

The order was archived and verified against SHA256 `933df9ee2224b908ca952cc80a7a3f7951f71ec2622818d12c841755721ed1df`. The whole-item check preceded the D1 restoration at `6415b6dab9`. [Order](../inputs/FP2_B_CORRECTION_2.md) · [SHA256](../inputs/FP2_B_CORRECTION_2.sha256) · [R4 source check](../source_check_correction_2.md) · [R13 questions](../../QUESTIONS_FOR_REVIEWER.md)

## D1 production-partition baseline

The test-only checkpoint `7f6e262cfc` reproduced the named N6P short-edge fault. N7P ran with the pinned event-2 input and differed at the native partition, so it is report-only. The v9 sidecar does not store the prepared facet data retained by D1's live geometry reference. The baseline report contains the six cell/face pairs for each event, the exact shifted edge pairs, and the first discrepancy. [Build637 baseline](FP2_B_637_correction_2_baseline.md)

The first baseline comparator included the unpinned 0.1 bins and rank-3 above-floor minimum in its combined reproduction flag. Those measures remain in the evidence. The corrected evaluator checks the enumerated minima, IDs and short-edge counts against the sealed receipt.

## Production correction and build

Step L uses the longest metric edge of each physical parent, with increasing point IDs for the tie. A locked longer neighbor blocks the candidate for the pass. An unblocked longer neighbor defers it for the sweep. Each accepted surface split keeps all four physical child altitudes finite and at or above the geometry resolution. The existing positivity and entity checks and Step D's spatial length floor still apply.

The refusal, border and neighbor-lock sets are disjoint. `SurfaceArrayBytes` already allows two set entries per edge, so the model is unchanged.

The P1 `c1` and `shifted` rows, P2, P3 and P4 retain D1's report switches. P2 now identifies the nearest old physical-parent vertex. The donor exception includes the failing donor identity and its four vertex coordinates. The direct `MetricAffineWeights` message remains exact for the FP1 move fixture.

Gate E is reused from build630. `CNativeLocalCore3DChanges.cpp` and `CNativeLocalCore3DPass.cpp` match build633 exactly. Changes to Core.hpp and Surface.cpp cover Step L, the shared altitude guard, and the requested print fields. [Gate E source audit](../correction2_gate_e_sources.json)

Both binaries built in 329.406 s with zero new warnings. [Build receipt](../build638/build_status.json)

| Binary | SHA256 |
|---|---|
| SU2_CFD | `165ff93c42ca453e6483bc0481af29db005f91b1789189b0ea35a2f5fbb5d58b` |
| UnitTests/test_driver | `88e284396115fa7fd73b0a244f4bde2191750f2e776b67e02fee9802974e79d1` |

## Serial and MPI fixtures

- The seven corrected path and altitude cases passed 351 assertions. Fixture e kept the two locked-face long edges and accepted the two ordered splits in the unlocked case. Fixture f exercised the wait branch and accepted Z, M and E in that order over later sweeps. Fixture g measured a `Near` refusal with no point allocation in both steps and acceptance at the larger height. Fixture g2 reproduced the thin child with all new edges above the resolution and accepted the larger-height case. [Path and altitude receipt](../scoped/build638/locked/fixture_status.json)
- The box/cylinder cohort passed 797 assertions in four cases. The existing cylinder observer counted 27 Step D splits with zero violations. A1 passed 105 assertions in three cases. [Box and cylinder](../scoped/build638/box/fixture_status.json) · [A1](../scoped/build638/volume/fixture_status.json)
- All 18 inherited FP1 cohorts and the requested FP2-A fixtures passed, including frozen IN1 and the two-rank and four-rank exchange. [Fixture cohort receipt](../scoped/build638/fixtures.json)
- G1 identity passed at 375, 114 and 228 case records in 752.544, 9.769 and 10.812 s. [Serial](../scoped/build638/G1/serial/comparison.json) · [MPI2](../scoped/build638/G1/mpi2/comparison.json) · [MPI4](../scoped/build638/G1/mpi4/comparison.json)

## Frozen hand-over gate

| Case | Seconds | Exactly positive cells after pass or shift | Bad cells by class (a/b/c/d) | Short edges below 0.01 | Donor rejects | Faces by pass |
|---|---:|---:|---|---:|---:|---|
| N1 | 280.055 | 546,565 | 2,706 (1,142/1,449/0/115) | 0 | 0 | 76,518 |
| N6 | 311.162 | 541,410 | 2,511 (1,168/1,324/6/13) | 0 | 0 | 76,394 / 76,554 |
| H6off | 66.116 | 419,732 | Report-only scan | — | 0 | — |

The smallest final metric edge was `0.027364967690064603` in N1 and N6, matching build633. Maximum star size was 162 in both. The most Step D sweeps was 23. Maximum pass time was 69.595 s for N1 and 26.543 s for N6, with `v_limit=0` throughout.

N1 and N6 each accepted 2,307 Step L splits versus 2,416 on build633. N1 and N6 rank 3 pass 1 each recorded 653 longest-edge waits in nine sweeps. Both runs had zero `left_long_neighbor`, `refused_near` and `left_long` on all ranks and passes. The separate N1 distance invariant had zero violations in 280.123 s and reproduced every integer surface field and the bad-cell count. [Invariant receipt](../cases/build638/N1Invariant/measurement.json)

All ranks agreed on the N6 snapshot size of 26,680,752 bytes. N1 qualified all 1,204 above-tolerance faces (100 refused, 1,104 neighbor-locked). N6 qualified all 1,203 (100 refused, five border, 1,098 neighbor-locked). Both recorded zero unexplained faces. [N1](../cases/build638/N1/measurement.json) · [N6](../cases/build638/N6/measurement.json) · [H6off](../cases/build638/H6off/measurement.json) · [Rank measurements beside B0](FP2_B_638_correction_2_handover.md)

## Production-partition hand-over

Corrected N6P matched all six partition cell counts and pass-1 face counts, so its gates applied. It passed in 303.939 s. The rank-2 shifted minimum was `0.038049534891454399` versus `3.9070055295542924e-10` on D1, and every rank had zero edges below 0.01 at the final pass and shift. There were zero donor rejects among 538,657 exactly positive cells. The equal snapshots and owned geometry proof qualified all 1,365 above-tolerance faces with 102 refused, 106 border, 1,157 neighbor-locked and zero unexplained. Largest star was 136, maximum pass time 23.443 s and `v_limit=0` throughout. Step L accepted 2,302 splits and counted 43 locked neighbors and 651 waits. Both steps recorded zero `Near` refusals. Final bad count was 2,501 versus D1 S1's 4,126. Face counts were 72,920 / 75,964 versus 73,962 / 76,940. [N6P receipt](../cases/build638/N6P/measurement.json)

Corrected N7P completed in 452.839 s and remains report-only. The first mismatched partition pair stayed 95,994 / 95,408 on rank 0. That rank's pre-partition metric weight and geometry scale equal D1 at 5,716,288 and 1,626.5383245273117. Its geometry weight is 380,291 versus 395,176. With zero prepared facet entries before weighting, its geometry work estimate is 604,517,893.53717875 versus 628,721,553.31817293. The observed 13,519 boundary faces and 11,079 cross-chart faces are exact. The data confirm the partition's geometry-weight input differed even with matching metric weight and face inventory. Corrected N7P observed 1,739 bad cells, 84,790 / 85,310 face sums, zero final edges below 0.01 and zero donor rejects among 589,718 cells. D1 S1 measured 1,704 bad cells and 85,180 / 85,422 faces on its different partition. [N7P receipt](../cases/build638/N7P/measurement.json) · [Partition pairs](FP2_B_638_correction_2_partition.md) · [Weight comparison](FP2_B_638_partition_weights.json)

The frozen hand-over gate and the valid production-partition gate passed. The report-only N7P evidence does not establish the event-2 production result. The next qualified test is S1 on the same build638.

## S1 with both options enabled

S1 passed both adaptation events and the isolated restart on build638. The main controller completed in 4,845.646 s and the isolated restart controller in 158.658 s. Both events passed physical validation, transfer, continued CFD, ownership and resource checks. [S1 receipt](../G5/build638/S1/run_receipt.json) · [Isolated restart](../G5/build638/S1/disk_restart_full638_run1_event2/disk_restart_validation.json)

| Event | Native max (s) | Points | Cells | Owned proof faces | Minimum named coverage | Largest transfer residual |
|---|---:|---:|---:|---:|---:|---:|
| 1 | 4,108.976 | 109,136 | 537,985 | 73,784 | 99.7120% | 8.603e-14 |
| 2 | 423.776 | 114,484 | 566,001 | 77,958 | 98.8869% | 7.935e-14 |

The reported job peak and sum of process peaks both equal 13,995,974,656 B. Both events had zero coordinate error. Every rank reported zero edges below 0.01 after the core's final shift and at engine seeding. Event 2 rank 0's shifted minimum was `0.020951597058329163` versus D1's `1.616941326830997e-09`. S1 printed no P2 near split, P3 metric rejection or enriched donor exception. [S1 diagnostics beside build633 and D1](FP2_B_638_correction_2_s1.md)
