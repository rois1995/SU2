# FP2-B correction 3 · build640

The order matches SHA256 `2b305d1c1fcd00aa5f4c59c1ca437068aee33a21095843fc344ac73d09e9adc4`. The last active build638 default-scope check, `saved_current`, finished naturally. The scoped default checks were kept as evidence at `4ba64e9f8b`. The full OFF and CORE workflows and S2 stayed pending. The host audit found 286 recorded owned identities terminal before the source work. [Order](../inputs/FP2_B_CORRECTION_3.md) · [Source check](../source_check_correction_3.md) · [Correction 2 boundary](FP2_B_638_correction_2.md#default-proof-boundary) · [Reviewer questions](../../QUESTIONS_FOR_REVIEWER.md)

An unconverged sheet placement can use a valid binding within the engine projector's accuracy. The new bound adds the binding's realization error to the long-double distance and rounds upward. Finite bounds within tolerance divided by eight continue through the existing mobility, realization, duplicate and split checks. The walk, minimizer, surface samples and engine sources retain their existing behavior. The four counters describe the result of the sheet walk and feed no decision.

Gate E retains the build630 replay. Both volume implementation hashes match build633. The production diff against `da8088eae2` covers only the sheet placement and its bound, placement outcome and counters. [Gate E source audit](../correction3_gate_e_sources.json)

Build640 compiled both binaries in 47.174 s without new warnings. [Build receipt](../build640/build_status.json)

| Binary | SHA256 |
|---|---|
| SU2_CFD | `ce93af9ef95c3b5aa5295a850bb4f74f92facdb00e93e5ef8fa27cee57b48b07` |
| UnitTests/test_driver | `9319873b17e14ec9956d666798ee76e110e24ae7909b67ea424f886dff7879a4` |

## IN1 placement fixtures

The pre-edit test-only run on build639 tested 72,757 eligible edges and found 3,321 Walk refusals. The pin checkpoint was `ef9b508f6a`. The corrected test passed 50 assertions in 37.455 s. It verified 69,436 converged placements and 3,321 tolerance placements, all accepted by Place. The Far, NoStart and Exception counts were zero. The first three rescued edges were 157–3649, 157–3650 and 158–3676. Each appeared in the baseline Walk refusal list and passed the realized binding, upper-bound, mobility and four child entity checks. The three converged pins matched bit for bit. [Baseline](FP2_B_639_correction_3_baseline.md) · [Frozen pin set](../inputs/FP2_B_CORRECTION_3_sheet_pins.json) · [Corrected fixture and exact rows](../scoped/build640/sheet/fixture_status.json)

| First converged edge | Realized position | Binding dimension / feature / version | u | v |
|---|---|---|---:|---:|
| 0–834 | -0.60873054144515593, 8.7342166758857093e-15, -96.531260014668149 | 1 / 2 / 18 | 0.499999999999999 | 0 |
| 0–1095 | 2.9345956594212539, 9.5664341947888499e-15, -97.315357990997413 | 1 / 3 / 18 | 0.50000000000000056 | 0 |
| 0–13978 | 6.1232339957367645e-15, 4.9008570128544697, -99.759236333966754 | 1 / 4 / 18 | 0.5 | 0 |

The pre-edit fixture wrapper encountered a missing output-root field after the test exited successfully and sealed its passing receipt. The receipt and raw hashes were verified directly. The corrected runner supplies the output root and checks the frozen refusal list. The pre-edit fixture was executed once. [Separate baseline log](../scoped/build639/sheet/test_outer.log.gz)

## Fixtures and G1

The correction 1 and 2 fixtures and A1 passed on build640. The requested FP2-A fixtures and all 18 FP1 cohorts passed as well. G1 matched all 375 serial, 114 two-rank and 228 four-rank records in 754.356, 9.849 and 10.555 s. [Fixture cohort receipt](../scoped/build640/fixtures.json) · [Serial G1](../scoped/build640/G1/serial/comparison.json) · [MPI2 G1](../scoped/build640/G1/mpi2/comparison.json) · [MPI4 G1](../scoped/build640/G1/mpi4/comparison.json)
