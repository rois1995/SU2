# B3 unapplied proposal status

No production source was changed. The proposal generator reran its source binding, reversal, capture, duplicate/unused-alias, and lexical token checks. No build, test, or solver was run.

Primary review found that `Phase` was an Adapt-local type alias and therefore
was not in scope in the extracted member definitions. The generator now adds
`using Phase = EnginePhase;` inside exactly `MeasureCell`, `MeasureBatchCells`,
and `SeedNext`, the three proposed methods that use `Phase`. Token normalization
strips that explicit method-local alias in addition to borrowed reference
aliases. The manifest separately audits local type aliases. A scan of the
original Adapt scope found only `using Phase = EnginePhase;` and
`constexpr size_t MeasurementBatch = 16;`; the latter remains in the methods
where it is used and is normalized only in the established body proof.

The frozen build552 input hashes remain:

- `CNativeEngine3D.cpp`: `7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848`
- `EngineCensus.hpp`: `bfe3181885c22e13bc1104c405e3adbf383bced1fff5861baed92a04ce02d739`
- `EngineCensus.cpp`: `314fbe6483953704d96d0ad5582e9bbcd507d740c65295b8a2f49d676979a326`

The draft keeps the six extracted Adapt workers and uses `Measure` as orchestrator, `ImportMeasurementBatch` for the existing retry/import portion, `MeasureBatchCells` for admission, field construction/planning, catch and final vote, and `MeasureCell` for only the existing per-cell loop. All nine methods are at most 66 project-formatted lines. The previous duplicate `surface_target` alias in `MeasureBatchCells` has been removed. The alias audit reports no duplicate or unused borrowed aliases; binding expressions/types and explicit parameters are recorded in the manifest. `started` remains `const auto&` in `Measure` and `SeedNext`.

All six proposed bodies compare token-for-token with the actual removed lambdas from the pinned `CNativeEngine3D.cpp`, after documented alias normalization and helper inlining. Main-file call/lambda reversal is byte-exact. This is a token proof, not a compile check.

Latest proposed artifact SHA256:

- `CNativeEngine3D.cpp.proposed`: `806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5`
- `EngineCensus.hpp.proposed`: `30f8ecd276e13710fa55177427257b0e0e7c65634ec76dd5d846d19ddd956183`
- `EngineCensus.cpp.proposed`: `e208dcae1331dffd25256577dccb3eef14025319d1b99269c95ccd14bf0df599`
- `binding_manifest.json`: `7ad18e014cc16d037be73944a2ab4a2d60d4e1c0aee6700120fc7a09dfecfda7`
- `generate_b3_proposal.py`: `f1ac3f4f75562d4494f8ea9e8b598e1ab79704638f08ad800e063008856a4a68`

This is an unapplied draft bound to the build552 snapshot only. Regenerate and review it against the post-N14-disposition base before any B3 edit.
