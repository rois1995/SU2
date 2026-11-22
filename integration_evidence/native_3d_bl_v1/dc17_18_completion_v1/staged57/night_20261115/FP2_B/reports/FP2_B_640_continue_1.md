# FP2-B continuation 1 · build640

The archived order matches SHA256 `50c3c7a7fc8a62959a424473deadd36b13ada8ca92f27b4a7398c25d87bc8bb9`. The tracked Common, SU2_CFD and UnitTests paths match `ecaed957db`. The live solver and test driver match the build640 hashes, including the recorded source pins. The entry process audit found 378 FP2-B identities and 19 D1 identities terminal. [Order](../inputs/FP2_B_CONTINUE_1.md) · [SHA256](../inputs/FP2_B_CONTINUE_1.sha256) · [Build receipt](../build640/build_status.json)

| Build640 binary | SHA256 |
|---|---|
| SU2_CFD | `ce93af9ef95c3b5aa5295a850bb4f74f92facdb00e93e5ef8fa27cee57b48b07` |
| UnitTests/test_driver | `9319873b17e14ec9956d666798ee76e110e24ae7909b67ea424f886dff7879a4` |

## Default proof

The original build640 receipt has zero new warnings. The correction 3 fixtures and G1 were verified from their receipts, including 375 serial, 114 two-rank and 228 four-rank records. The N1 invariant includes fixture j and reconciles 253 tolerance placements to 253 accepted splits. N1, the invariant, N6 and valid N6P were reused as the hand-over gate. S1 was reused from its passing workflow and isolated restart. The completed build638 scope remains historical evidence. [Sheet fixture](../scoped/build640/sheet/fixture_status.json) · [Fixture and G1 cohort](../scoped/build640/fixtures.json) · [N1](../cases/build640/N1/measurement.json) · [N1 invariant](../cases/build640/N1Invariant/measurement.json) · [N6](../cases/build640/N6/measurement.json) · [N6P](../cases/build640/N6P/measurement.json) · [Prior S1](../G5/build640/S1/run_receipt.json) · [Build638 scope](../scoped/build638/scoped.json)

G2 and GQ passed the exact-row and scheduling comparisons in 237.768 and 656.293 s. G3 passed in 613.799 s with 1,568 accepted splits, a matching common prefix of 1,113, and equal observer-adjusted entry work. G4 passed its 1/2/4-rank cases in a 36.251 s controller. The shared merit481, saved481 and saved_current controllers passed in 121.473, 144.338 and 143.402 s. [G2](../scoped/build640/prefix/G2_comparison.json) · [GQ](../scoped/build640/prefix/GQ_comparison.json) · [Scoped proof](../scoped/build640/scoped.json) · [Shared safeguards](../scoped/build640/safeguards.json)

The initial entry through the generic correction 2 launcher omitted the correction 3 fixture pins. A temporary launcher then resolved a draft wrapper from its own directory. Both preflight attempts stopped at the immutable provenance comparison before running a check. Using the committed correction 3 provenance and Python's safe path option let the existing controllers run with the original gate values and binaries.

### G5 OFF

Both adaptation events and the isolated restart passed with both options disabled. All six on-disk SHA256 values matched the N21 W reference. Native maxima were 1,757.338 and 1,017.567 s. Minimum named timer coverage was 98.9954% and 99.5266%. Job peaks were 14,014,525,440 and 16,700,268,544 B, each equal to the sum of the process peaks. Owned proof faces were 61,906 and 58,260. [Standing-gate receipt](../G5/build640/OFF/run_receipt.json) · [N21 W payload comparison](../G5/build640/OFF/payload_identity.json)

### G5 CORE

CORE used the pinned FP1 config SHA256 `1ec6c26d49d4ec7c62103d5bd096471b5422290cd06c44b2f1073b7489f2b4c1`. Both adaptation events and the isolated restart passed, and the six on-disk SHA256 values matched the accepted FP1 ON reference. Native maxima were 1,509.433 and 371.336 s. Minimum named timer coverage was 99.0551% and 98.6725%. Job peaks were 11,576,315,904 and 11,652,763,648 B, each equal to the sum of the process peaks. Owned proof faces were 68,006 and 64,758. [Standing-gate receipt](../G5/build640/CORE/run_receipt.json) · [FP1 payload comparison](../G5/build640/CORE/payload_identity.json) · [Default proof](../G5/build640/default_proven.json)
