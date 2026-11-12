# SU2 Lean — next-agent instructions following BoundaryPath handoff

**Status:** This is a proposed *next assignment*, not approval of the just-submitted candidate. The independent evaluator's `SU2_Lean_boundary_loop_independent_review_20261112.md` must be considered first. The new BoundaryPath candidate has passed source-identity and external fixture checks but **has not yet obtained full external cold Lean/native acceptance**.

## A. Protect and complete review of the frozen candidate first

- Frozen candidate: `73a322b640f45505e8d8fb70fbce46e6ccffd206`, `BoundaryPath/candidate.json` SHA-256 `7c6a795c2dc8ef748bd4cdd9cdba3dc31330e55decb4b504b947d592654ef37e`.
- Production baseline: upstream `d162016382f0c122433000c114e88d808b48f7a7`; no production patches.
- Preserved EulerPath candidate: `15ee28adf032a866e292926502a5f022e38199c8` and lock `0a5781dad650c8de5bf24ef57804c6a422dbef3aa2be3f373b7ebc21371ccac6` (previously source-reviewed, still pending formal independent acceptance).
- Do not rewrite, amend, rebase, force push or silently replace the frozen submission while independent acceptance is outstanding. Resolve any evaluator findings in a *new clearly identified* candidate only.
- Supply a fresh, reproducible source-only archive if required. Differentiate worker logs from an independent evaluator's successful execution. Completion of a local build does not confer external acceptance.

## B. New two-agent architecture

A second agent will develop a solver-independent Lean 4 mathematical library in the user's existing **`CFD_Lean` directory**, based on references in:

`/media/rausa/4TB/SU2_Versions/Prove/Papers`

The second agent must inventory the references, identify exact editions/errata, propose bibliography and statements, and **ask the user for approval before starting implementation**. That separate effort will eventually have its own Git repository and pinned Lake package.

**Responsibility split:**

- **CFD_Lean agent:** literature-derived governing equations, Roe variants, flux consistency and linearization, node-centered FV geometry/assembly, exact physical and discrete boundary methods, explicit Euler and their proven mathematical properties. It must not treat SU2 C++ as its normative source.
- **You, SU2 agent:** production-source selection and dispatch, faithful implementation models, source semantics/correspondence, memory/index/geometry/caller contracts, mutation controls, and composition into a whole configured solver execution path.
- **User / external evaluator:** scientific specification approval, discrepancy classification and proof acceptance.

Do not create or edit the CFD_Lean repository on behalf of its agent. Do not inspect SU2-derived definitions as a substitute for approved independent mathematical specifications. The CFD library must not depend on SU2. When ready, SU2 imports only an explicitly approved pinned CFD_Lean revision; do not make an unpinned dependency on its live development branch.

## C. Immediate high-value technical work after candidate acceptance

1. **Close the post-boundary memory readiness gap.** The current `assembled_boundary_complete_update` has the premise `InputsReady m (boundaryImpl ... ) ...`, which assumes the actual update memory already contains precisely the post-boundary old/residual/truncation buffers. Formalize the *memory-level boundary execution* (or an accurately qualified source-derived model of its writes) and prove that it establishes this relation from explicit pre-boundary initialization, without assuming the required post-boundary state.
2. **Close caller and storage connections.** Account for primitive cache and ghost state, point/vertex ordering, ownership and repetitions at corners, old-solution copy, truncation initialization, residual zeroing, and when those operations occur. Surface all assumptions that remain unverified; do not call these complete until discharged.
3. **Interior SIMD Roe and geometry/edge traversal.** Continue source-extraction research on the actual selected ideal-gas, first-order, serial fixed-grid Roe path. Preserve vector masks, lane bounds, orientation, face normals, and unrolled/templated branches. Do not prove a convenient scalar routine instead of the executed SIMD path.
4. **Prove composition of actual selected operations.** Build from accepted mathematical contracts and source-connected models. Make the composed theorem identify which transitions are *executed in its formal semantics* and which are only prepared as premises.
5. **Improve C++ semantics fidelity gradually.** Record memory/object lifetimes, aliasing, layout, header/template instantiation, AD traits, `su2double` types, compiler options, and floating-point assumptions as distinct obligations. Do not equate exact-real proof with IEEE-754 correctness.

## D. Literature gate and discrepancies

The far-field characteristic formulation and scalar boundary Roe/eigenmatrix currently in `SU2Boundary.Spec` are provisional mathematical contracts. They have been proved **relative to their own definitions**, but they have not been independently approved against foundational CFD literature.

When the CFD_Lean agent produces proposed Roe/far-field/slip-wall specifications, compare their precise assumptions and variant choices to the **selected actual SU2 C++ implementation**. Prepare a traceability and discrepancy matrix with statuses:

- Agree within proved assumptions;
- Documented intentional variant (requires independently approved variant specification);
- Potential implementation defect (requires evidence/counterexample);
- Inherent limitation of numerical method;
- Unresolved correspondence or literature gap.

Do not weaken, rewrite, or re-label a scientific specification to match source. The user/evaluator must explicitly approve any mathematical specification amendment. Equally, do not classify every difference from original Roe (1981) as a bug: entropy fixes, scaling conventions, characteristic normal orientation and other variants may be deliberate.

While literature review is pending, you may progress on **pure source fidelity and execution-path semantics**, but do not claim literature-backed algorithm correctness.

## E. Upstream baseline maintenance

The current candidate is validly pinned to `d1620163`. As of the review, upstream `develop` has advanced one commit to `103442f30ca2fba1ff48ef12aa1ae93480256a30`, changing only GitHub Actions workflow files. Document the comparison; no need to rewrite accepted proofs for CI-only changes.

At the next technical baseline gate check `su2code/SU2/develop` anew. If relevant source changes occur, re-establish source identities and re-run affected proofs/tests before claiming coverage of the new revision. Keep historical tests, accepted repairs and their provenance, but don't cherry-pick unvalidated patches blindly. Avoid continuously rebasing during a proof milestone.

## F. Required safeguards and evidence

- Independent specification vs implementation model and immutable reviewer-approved contracts.
- Complete theorem-type and transitive axiom auditing; no `sorry`, `admit`, unapproved axioms or hidden conditional assumptions.
- Positive, negative, mutation and benign-refactor controls: semantic mutants must parse and violate unchanged contracts; syntax failures are not semantic defect detection.
- Reproducible source-only Lean cold build and **actual** C++ compiled-path tests; make numerical test evidence distinct from formal proof evidence.
- Mark numerical-model correctness, source model correctness, C++ source correspondence, compiler/heap/FP semantics, and composed execution coverage separately.
- Preserve a numerical discrepancy register and source coverage map; theorem counts are not scientific acceptance criteria.
- No production SU2 modification without an independently reviewed defect/reason, reproducible test and focused patch.

## G. What to deliver at the subsequent handoff

1. Exact SU2 upstream commit and selected source/dispatch paths.
2. Frozen Lean definitions and theorem types with dependencies and checked assumptions.
3. **Memory-level boundary-to-update connection**: identify whether `InputsReady` was established rather than assumed; show the actual theorem statement.
4. Source correspondence and test controls, including successful native regression and reproducible mutation counterexamples.
5. Comparison against any already **approved** CFD_Lean specifications (if not yet available, report pending rather than inventing approvals).
6. Current path diagram with verified vs assumed transitions for every stage, especially interior Roe, mesh traversal and buffer initialization.
7. New bug/discrepancy register, with confirmed vs possible vs no defect.
8. Source-only independent evaluation kit and explicit statement that worker tests do not equal evaluator acceptance.

**Ultimate objective remains literature-grounded, composable verification of an actual configured SU2 Euler execution path—not a growing count of disconnected Lean theorems.**
