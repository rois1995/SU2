# Independent external review — SU2 Lean BoundaryPath candidate

**Date:** 2026-11-12  
**Candidate commit:** `73a322b640f45505e8d8fb70fbce46e6ccffd206` (`rois1995/SU2`)  
**Candidate lock SHA-256:** `7c6a795c2dc8ef748bd4cdd9cdba3dc31330e55decb4b504b947d592654ef37e`  
**Pinned production baseline:** upstream `su2code/SU2` `d162016382f0c122433000c114e88d808b48f7a7`  
**Archive SHA-256:** `a7b5115deca59e7c938632157a5eb0255f21736e107aa05a4bdb5d1d72b24879`  
**Decision:** **Provisionally acceptable as an exact-real, restricted-model boundary/update-loop extension. Full external acceptance is pending independent cold Lean/theorem/axiom and full native execution. No complete C++/Euler-path verification claim is accepted.**

## 1. Independently completed checks

| Check | Result | Scope / limitations |
|---|---|---|
| ZIP CRC test | **PASS** | `unzip -tq`: no archive errors. |
| Archive SHA-256 | **RECORDED** | `a7b5115deca59e7c938632157a5eb0255f21736e107aa05a4bdb5d1d72b24879`; no user-supplied independent sidecar checksum. |
| Internal manifest SHA-256 | **PASS** | Independently recomputed all 527 manifest file hashes against the ZIP members. |
| Candidate lock | **PASS** | Independently computed SHA-256 of `BoundaryPath/candidate.json`, matching `7c6a795…`. |
| Git commit identity | **PASS** | Commit available in user fork and restored in independent local Git checkout; parent is the earlier Euler candidate `15ee28ad…`. |
| Production baseline | **PASS** | Compared local source to pinned upstream tree. `git diff d1620163..73a322b` shows 522 files, all under `FormalVerification/`; no production patches. |
| Source-span identity | **PASS** | Independently checked the hashes of all 11 selected byte spans, drawn from 5 real pinned production files. |
| New Lean-source theorem inventory | **PASS as source count** | 81 explicit `theorem` declarations: Boundary 22, Controls 17, Generated 20, Loop 20, Spec 2. Counts do **not** constitute proof validation. |
| Obvious forbidden proof tokens | **No occurrences observed** | `sorry`, `admit`, explicit `axiom`, `native_decide` and `unsafe` absent from submitted `SU2Boundary/*.lean`; not a substitute for a transitive kernel axiom audit. |
| Native C++ fragment controls | **PASS independently** | Rebuilt nine fixture binaries with `g++ -std=c++17 -O2`: original and refactor passed; seven semantically mutated variants compiled and failed their unchanged contracts as intended. These are small mock-object fixture tests, not whole-SU2 execution. |
| Cold Lean build, theorem-type check and transitive axiom audit | **NOT YET INDEPENDENTLY PASSED** | Source-only inputs were successfully restored into a fresh workspace. A clean mathlib/Lake rebuild was launched, but no completed external Lean result is asserted in this report. |
| Actual full SU2 boundary-method test (7,747 assertions) | **AGENT-REPORTED ONLY** | Supplied logs describe success; not independently compiled/executed in this evaluation. |
| Full selected Euler integration (two 20-iteration runs) | **AGENT-REPORTED ONLY** | Repeated end states in the supplied logs, not a convergence result or correctness proof. |

### New upstream commit since the pinned baseline

At the time of this review, `su2code/SU2/develop` had advanced to `103442f30ca2fba1ff48ef12aa1ae93480256a30` (12 November 2026). Repository comparison from `d1620163` reports **one intervening commit affecting only `.github/workflows/regression.yml` and `.github/workflows/release-management.yml`**. The current candidate can remain pinned to `d1620163` without invalidating its production mathematical source identity; the next baseline gate should record and assess the new head.

## 2. Substantive review of the theorem statements and code

The candidate is materially stronger than the previous EulerPath draft:

- `SU2Boundary.Spec` contains a closed far-field Riemann-invariant reconstruction, characteristic primitive cache, a manually represented **scalar boundary** Roe flux, a stationary-wall projection on **old/residual/truncation**, and an explicit update contract.
- `SU2Boundary.Boundary` proves refinement between a manual implementation representation and the stated boundary mathematical specification under explicit density/enthalpy assumptions. It models far-field processing before wall processing, preserves the unaffected current buffer, and states unit-normal constraints.
- `SU2Boundary.Loop` proves that its **formal** `Fin N × Fin 4` traversal visits each owned point/component once, respects its modeled memory layout and writes the stated update to every owned current cell while framing other abstract addresses. A specified positive volume and appropriate initialization remain practical caller requirements.
- `SU2Boundary.Parser` rejects unrecognized syntax for the selected statement and loop-header grammar; selected source statements are extracted as exact byte spans with hashes. It does **not** parse the full C++ method or prove full source-to-semantics equivalence.
- `SU2Boundary.Controls` includes semantic rejections for additive sign, three wall-buffer correction signs, skipped point, skipped energy component and timestep-field substitution. Native fragment controls independently reproduce their expected behaviors.

The code contains no obvious deliberate vacuous-domain or assumption-of-conclusion shortcut. In particular, the updated proof has removed the old arbitrary `finish` callback from the **mathematical** boundary transformation. This is a useful improvement. It must not be equated with closing the full C++ implementation path.

### Critical composition gap — still a premise, not a proved call-chain

The theorem `SU2Boundary.assembled_boundary_complete_update` requires

`ready : InputsReady m (boundaryImpl g free far walls {s with residual := scatterLoop ...}) vol (fun _ => 0) dt`.

Thus, its *starting abstract memory* is **assumed to already contain** exactly the calculated post-boundary old, residual and truncation buffers. The theorem proves the update from this prepared memory; it does not prove that executing the selected C++ boundary routines establishes `InputsReady` from initialized pre-boundary memory. The boundary model and full-loop model are composed at the specification level, **not through an established C++ memory-state transition**.

This is an acceptable explicit assumption for an intermediate theorem, and the handoff discloses it. It is a priority to discharge before declaring a complete execution-path proof.

### Other material open obligations

1. **Literature authority:** far-field characteristic selection, scalar Roe eigenmatrices, Roe averaging, entropy/sound-speed branches, and wall enforcement are currently independently defined relative to the source model but **not yet approved against primary numerical-method literature**. A separate CFDLean literature agent will now own these specifications.
2. **C++ correspondence:** source checks and native tests do not prove the behavior of source-language aliasing, object representation, compiler lowering, floating-point `sqrt`/`pow`, or nontrivial frontend semantics.
3. **Interior SIMD Roe:** the primary interior face fluxes remain supplied values, not a verified computation from conservative states and geometry. The scalar boundary Roe refinement does not close this gap.
4. **Traversal and initialization:** arbitrary supplied edge/vertex lists and primitive snapshots do not establish source-level graph coverage, normal orientation, allocated storage contents, ghost/cache consistency, or bounds in the actual solver.
5. **Far-field premisses:** stored interior enthalpy is assumed to match the ideal-gas formula and characteristic primitive snapshots are assumed to correspond to actual solution data. Real-number division totalization is not a floating-point-domain guarantee.
6. **Corner and boundary interactions:** a final wall normal projection does not prove simultaneous impermeability for intersecting incompatible normals.
7. **Numerical positivity/convergence:** the exact-real wall projection can preserve old-state admissibility, but the complete explicit update is not proved positivity preserving, and the short integration run is not a convergence test.

## 3. Defect status

**No newly confirmed SU2 production bug** is reported in this candidate or established by my checks. The seven intentionally injected numerical mutations are *validation controls*, not discovered production defects. The earlier moving-wall issue is a known historical defect, outside this stationary selected path.

## 4. Acceptance decision and next agent direction

**Verdict:** Conditional/provisional approval of the *design and source-level identity*, and independent acceptance of the restricted C++ mutation experiment. **Do not mark the new Lean proof milestone externally accepted** until a completed external cold Lean build, statement/axiom audit and reproducible native checks have been recorded. Do not promote any C++ or complete-path correctness claim at this stage.

Upon completion of those checks, proceed without discarding accepted proofs. The immediate technical task is **modeling and proving the transition that populates post-boundary memory**, then working toward the missing production path stages: primitive preprocessing, mesh/edge enumeration, full interior SIMD Roe, initialization and configuration dispatch. The first newly introduced numeric specification should instead be reviewed in the separate, literature-driven `CFD_Lean` library and imported at a pinned approved commit.

The existing abstract boundary model should be labeled **provisional SU2-specific specification** until the new library's independently derived mathematical contracts are accepted. Do not modify the frozen candidate or protected requirements to make them agree silently. If the independent specification exposes an actual discrepancy, classify it and obtain an explicit review decision before changing either side.

## 5. Evidence files

- Uploaded archive: `SU2_Lean_boundary_loop_20261112.zip`.
- Evaluator's independently produced C++ fragment log: `native_fragment_recheck/evaluator_result.json` (packaged separately in the handoff evaluation ZIP).
- Original source manifests, theorem statements, candidate lock, and worker logs are within the uploaded archive.
- Source-only restoration succeeded in a private workspace, without changing the frozen submission. No claim of a completed cold Lean proof audit is made here.
