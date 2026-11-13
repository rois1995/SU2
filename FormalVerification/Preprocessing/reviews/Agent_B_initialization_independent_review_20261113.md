# External evaluation — SU2 Lean initialization milestone

**Evaluation date:** 2026-11-13  
**Delivered archive:** `SU2_Lean_initialization_reviewer_20261113.zip`  
**Archive SHA-256:** `ab57dde9e993640b6b3fccaae5b1f3df09f9ae402309f6a8e736d310c20268df`  
**Delivered Git commit:** `d19af33d2b2ba2845a49038bf10088460ca006a1`  
**Frozen mathematical/source candidate:** `d3aea3de43f6aefbc4dce7a09fc399c367230b9d`  
**Frozen source lock SHA-256:** `1257f65d1c1ce63ef98c9f4c29d89581efb5ff82e9a9cc6a301e092cd1a909bf`  
**Separate corrected-replay lock SHA-256:** `a285fe9b3423fbd96a91539b044f67323865c1ca81cfc2226032d05bda106a40`  
**Selected upstream numerical baseline:** `d162016382f0c122433000c114e88d808b48f7a7`  
**External decision:** **SOURCE-LEVEL PASS; mathematical candidate conditionally acceptable; final independent kernel and full native acceptance PENDING.**

## 1. Independent checks actually performed

| Evidence | Result | Scope |
|---|---|---|
| ZIP CRC test | PASS | All 1,053 ZIP members; no unsafe traversal names |
| Internal archive checksums | PASS | All 1,051 `SHA256SUMS` entries |
| Manifest SHA-256 | PASS | All 1,050 `manifest.json` file entries |
| Git identity | PASS | Local git snapshot HEAD = delivered `d19af33d`; parent = frozen `d3aea3d` |
| Protected new source | PASS | All 81 `candidate.json` protected files |
| Protected historical source | PASS | All 713 previous formalization files |
| BoundaryMemory lock | PASS | `76480f32b04949ce54a14fff97d63da79ac830e6aa21b6fdb9fb20a95d3229cb` |
| Production changes | NONE | No SU2_CFD, Common or externals changes relative to selected upstream baseline |
| Source regeneration | PASS | Independently regenerated 30 source fixtures/manifests; all byte-for-byte identical |
| Lean theorem inventory | PASS static | Exactly 62 explicit theorem declarations; corrected inventory matches |
| Static proof token scan | PASS static | No `sorry`, `admit`, explicit `axiom`, `native_decide`, or `unsafe` in the seven new proof modules |
| Compiled C++ original and refactor | PASS independently | 500 checks each, g++ C++17 mock-storage fixture |
| Compiled C++ negative variants | PASS independently | Seven semantic mutants compile and fail unchanged numerical contracts |
| Independent cold Lean kernel/type/axiom audit | **NOT COMPLETED** | Separate private CFDLean M1 checkout required; worker output not promoted |
| Independent full native SU2 build/integration | **NOT COMPLETED** | Mock-class C++ controls are not a full solver integration test |

The independent native replay produced the expected contracts: `wrongBuffer`, `incomplete`, `omitDensity`, `omitKinetic`, `staleFallback`, `eraseTail`, and `wrongResidual` each compiled and returned the expected numerical-contract failure. These are deliberate mutations, **not production SU2 bugs**. Regenerated fragments agree with the uploaded source at the specified pinned Git baseline; parser/manual-model coverage remains limited.

The worker's cold/warm Lean evidence reports a successful pinned Lean 4.19.0 build, 299 logical declarations (including CFDLean), 159 theorem declarations, transitive dependence only on `propext`, `Quot.sound`, and `Classical.choice`, rejection of an injected external axiom, and seven deliberately unprovable contracts. I inspected those records, but **did not independently replay the kernel or axiom audit**. The private CFDLean M1 source repository was accessible for inspection through the connected GitHub source, but there was no executable local authenticated Git checkout: the container cannot resolve github.com. The archive deliberately excludes private CFDLean source. This is a genuine outstanding verification gate, not evidence of a failed proof.

## 2. Proof and numerical assessment

**A. Conservative constructor.** The proof models the selected 2D non-restart constructor, total-energy convention, truncation initialization and old/current conservative buffers. `initialize_refines`, `seed_compatible` and `seed_admissible` are relevant, reusable results for the stated model. Complete allocation, freestream input production, restart paths and actual C++ heap behavior remain outside scope.

**B. Primitive conversion and fallback.** `staged_pressure_correct`, `staged_temperature_correct`, `rejection_iff`, `set_point_refines`, and tail-frame results model the ideal-gas stages and both validity branches. Correctness of the selected primitive result requires an **admissible backup state**. The Lean point model is not yet an interpreter of actual per-point primitive/velocity-square/fluid-model scratch writes, nor of the caller traversal. Real division at zero and `Real.sqrt` do not establish IEEE-754 behavior.

**C. Snapshot.** `copy_executes`, `flat_copy_coverage`, `snapshot_refines`, `snapshot_refactor`, and `source_snapshot_execution` meaningfully prove a full logical current-to-old copy over all allocated rows/components, including harmless reversed traversal. The restricted source parser is fail-closed and accepts a limited set of complete method bodies; it does not implement general C++ semantics, aliasing or full memory layout.

**D. Residual clearing.** The abstract clearing/frame theorem captures the selected buffer; the source identity and native control are valuable. The entire `CSysVector::SetValZero` behavior and exact caller path are not formally translated.

**E. Principal composition.** `SU2Initialization.snapshot_interior_boundary_update` **actually executes the snapshot** and establishes the old-buffer representation used later. However, the theorem universally quantifies over an intermediate `mi` that is assumed to satisfy old-buffer preservation, a frame relation, residual/truncation/primitive/cache representations, the geometry/timestep relation, and `FarReady`. It does not execute the interior Roe computation, timestep, actual preprocessing memory effects or their callers. This is a valid intermediate theorem with a substantial **OPEN interface**, not a complete Euler execution-path proof.

**F. Independent mathematics.** A compatibility layer imports exactly the separate CFDLean M1 commit `0a88e36b3810cd5adc3881db526ca8a35f81afbb`. The connected GitHub repository contains an independent acceptance record at `e1b6747e8683f6b6e81e1ccc510271e539611848`. Its scope is **exact-real ideal-gas states and oriented physical flux only**. Roe and far-field formulations are not thereby approved.

## 3. Audit-tool correction

The original candidate file `Initialization/candidate.json` lists 63 purported explicit theorem names. The extra `SU2Initialization.does` was generated by an unanchored regular expression matching the text `theorem does` in a comment. The supplied initial `Types.lean` failed because the name did not exist. The agent preserved that failed attempt, then introduced a separate `InitializationReplay/` checker using anchored theorem declarations. This source-replay-only change does not modify mathematical files; the corrected inventory is **62** and matches the independently enumerated declaration lines. The corrected checker itself still requires independent execution under the pinned Lean environment.

This is a legitimate auditor bug that has been disclosed and corrected, not an SU2 numerical bug. It illustrates why an independent checker should own the final proof inventory and theorem-meaning audit.

## 4. Potential robustness issue, not a confirmed bug

`CEulerVariable::SetPrimVar` copies `Solution_Old` when the first candidate fails validity checks, then recomputes primitives without using the second set of flags to establish that the backup state is valid. This behavior is confirmed by pinned C++ source. The formal theorem explicitly requires an admissible backup. We do **not** know whether invalid backups are reachable under the selected initialized Euler caller path, or whether the calling code has an appropriate failure policy. Treat this as a caller/invariant investigation, not a confirmed upstream production defect.

## 5. Baseline freshness

Connected GitHub reports upstream `develop` at `103442f30ca2fba1ff48ef12aa1ae93480256a30` on 2026-11-12. Relative to the pinned `d1620163`, its only changes are two GitHub Actions workflow files. Pinning `d1620163` for this frozen numerical milestone was therefore defensible. The next milestone should perform a fresh numerical-source baseline gate, not change the revision mid-proof.

## 6. Decision and obligations

**Decision:** accept archive/source integrity, source-fragment regeneration, static theorem inventory, Git preservation and the native mock-storage controls. The candidate is a valuable, honestly bounded Lean formalization; **do not certify it as independently kernel-accepted** until the pinned private CFDLean checkout and reusable source-only evaluator kit are available for a cold external build and a separate theorem-type/dependency audit. The previously reviewed BoundaryMemory candidate also remains pending cold Lean/full-native acceptance.

Prioritize the **primitive-point-to-memory execution and the entire preprocessing traversal**, including fallback, velocity-square/fluid-model scratch, residual clearing, old/current ordering, and preservation across the snapshot. The next proof must derive the memory predicates required by the boundary-entry relation from executed operations rather than assuming them. Continue to keep Roe, geometry traversal, timestep and machine C++/IEEE semantics as separate explicit obligations; integrate Agent A's Roe/far-field specifications only after their independent scientific approval.
