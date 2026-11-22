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
