# Agent B — Next assignment after initialization review

**External review status:** Initialization source-level review and independent C++ fragment controls passed. The frozen Lean candidate at `d3aea3de43f6aefbc4dce7a09fc399c367230b9d` and replay correction at `d19af33d2b2ba2845a49038bf10088460ca006a1` remain pending *independent* Lean kernel, theorem-type/axiom and full SU2 native acceptance. Do not promote agent-local logs into external acceptance.

## 1. Preserve the work and close the evidence gate

Keep all prior accepted mathematical definitions and the frozen EulerPath/BoundaryPath/BoundaryMemory/Initialization proof identities unchanged. The separate `InitializationReplay` checker is the corrected declaration inventory; its proper count is 62 explicit theorems. Do not count comment words as declarations. Independently reproducible proof acceptance must include:

- A clean source-only Lean 4.19.0 build using the exact accepted **private** CFDLean M1 revision `0a88e36b3810cd5adc3881db526ca8a35f81afbb`, provided privately to the evaluator rather than copied into public SU2.
- Independently controlled theorem statements/inventory, a transitive axiom check and a deliberately bad-axiom rejection.
- Semantic negative controls that compile or evaluate successfully before violating the unchanged requirement (not parser/hash/compiler failures counted as defects).
- Full appropriate native SU2 tests and selected integration, reported separately from mock-storage fragment tests.

If the private dependency is inaccessible in the evaluator's execution environment, report that explicitly. Do not create a replacement mathematical specification for convenience.

## 2. Next mathematical/source milestone — full primitive preprocessing memory refinement

**Primary objective:** turn the current `setPoint` point-value model into a source-connected, composable logical-memory execution stage that derives the current/primitive/scratch representations needed before the actual snapshot, rather than assuming them.

Trace and model, in the selected serial fixed-mesh ideal-gas first-order Euler configuration:

1. Full `SetPrimitive_Variables` traversal over selected owned and halo points, including its actual order and ownership/use of primitive data.
2. `CEulerVariable::SetPrimVar` current-state loads, `SetVelocity`, squared-velocity scratch, fluid-model `SetTDState_rhoe`, density/pressure/sound/temperature checks, and enthalpy storage, all with actual selected 11-column primitive stride.
3. Both valid and invalid-state branches, the old-to-current fallback copy, recomputation and its observable success/failure flag. Preserve the second-invalid-backup possibility as an explicit caller/safety obligation, not a silently excluded state.
4. Selected residual zeroing, truncation and other scratch initialization, and the exact order relative to `Set_OldSolution` and `SetTime_Step`.
5. Alias, layout, bounds, read/write order, scratch lifetime, and frame properties, with acknowledged assumptions wherever a full C++-heap proof is unavailable.

The milestone theorem should start with a pre-preprocessing logical memory state and **execute** the supported preprocessing and snapshot operations, establishing the `BeforeSnapshot`, `BoundaryRep` and old-buffer preconditions from those executions where appropriate. Do not pass an assumed post-preprocessing memory as an argument and label this composition.

Do not overwrite the previously accepted boundary-memory model. Introduce a narrow refinement/compatibility layer. Source extraction must fail on unsupported syntax. Model the actual selected C++ dispatch and its fallback branches, not a convenient unused function.

## 3. Make the safety obligation testable

The old-solution fallback is modeled as requiring an admissible backup state. Establish whether that invariant is supplied by the selected constructor and iteration path, or document exactly where it can fail. A source-level retry without revalidating the backup is **not** by itself a confirmed SU2 bug.

If a reachable production counterexample is found, reproduce it against the actual code, identify the expected scientific/algorithmic requirement and propose a minimal fix plus regression test on a separate branch. Do not alter production code just to make a proof succeed.

Maintain meaningful controls for wrong buffer, skipped last point, stale fallback primitives, omitted density/kinetic terms, wrong residual clearing, stride/tail overwrites, wrong snapshot order, and a harmless traversal refactor.

## 4. Continue to target the entire Euler execution path

The following remain explicit downstream obligations: actual dual geometry and normal construction, owned/halo and edge enumeration, first-order SIMD Roe computation and scatter, explicit timestep and volume calculation, remaining source/communication/auxiliary branches, true C++ heap/ownership semantics, and IEEE/compiler behavior.

When preprocessing+snapshot is source-connected, introduce the next stage contracts and **prove their caller preconditions** rather than assuming that the intervening interior/timestep state appears correctly.

Do not claim complete-path verification until all essential stages are composed, with clearly scoped language/arithmetic assumptions. A theorem count alone is not the milestone.

## 5. Coordination with Agent A — independent CFDLean

Reuse the accepted M1 state/physical-flux contract through a pinned Lake dependency. Its approval does **not** cover Roe or far-field characteristics.

Maintain/forward `FormalVerification/Initialization/coordination/SPEC_REQUEST_A.md`, preserving receipt status as unknown until acknowledged. Request independently derived Roe and boundary variants, including reference sections, assumptions, branch conventions and admissibility limitations. Do not change Agent A's mathematical requirements to match SU2. Once accepted, prove compatibility or produce an explicit discrepancy record.

## 6. Deliverables and hard acceptance gates

Submit a separate frozen candidate, exact upstream/working commits, protected theorem/specification checksums, source-span and caller mapping, independent-suitable offline package, native semantic controls, explicit evaluated theorem types, and a coverage map distinguishing mathematical, restricted-model, C++ and floating-point claims. Provide the external evaluator a *separate authorized private CFDLean source input* for the kernel replay; do not publish its contents in public SU2.

**Accept only if:** the new theorem derives preprocessing/snapshot memory representations from genuine modeled executions, including fallback and all-point traversal, with the specified frames and meaningful source mutation controls. An unchanged post-preprocessing-memory assumption or new unconstrained callback is not sufficient. Preserve the user-approved independent evaluation boundary.
