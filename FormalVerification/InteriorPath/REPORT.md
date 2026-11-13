# Worker report and acceptance limits

The principal new result derives pre-boundary memory from pre-preprocessing memory by executing CFL initialization, preprocessing/snapshot, timestep and the full selected interior packet loop. It then composes preserved explicit boundary transformations and the complete explicit update. It removes the former assumed timestep and interior residual representations within this exact-real model. This advances the connected configured path without claiming full source semantics.

This candidate contains 126 explicit theorem declarations and 239 public elaborated theorem declarations, including generated helpers/equations. The final audit checked 3,055 connected logical declarations and 19,983 transitive constants, with 1,508 connected theorem declarations. These counts include preserved imported results; they are not all new theorems.

The theorem inventory and axiom audit are generated from elaborated Lean declarations, including types and per-theorem transitive axioms; the union of all connected logical declaration dependencies is also checked. Only `propext`, `Classical.choice` and `Quot.sound` are permitted. Counts distinguish explicit theorem statements from public elaborated helpers/equations. No sorry/admit/new axiom is accepted. Worker checks are supporting evidence. Independent source review, cold kernel/dependency/native checks and final approval are outside this worker's authority.

The parent preprocessing candidate remains unchanged. Its user-reported source-level review and 96 explicit declarations are recorded in `reviews/preprocessing-status.json`; cold/kernel/full native acceptance remains pending. Failures in either frozen candidate must be reported and corrected in a separately identified candidate, without altering approved definitions or criteria.

## Evidence classes

- **Mathematics:** independent conservative/primitive, orientation, conservation and assembly foundations; new proposed timestep/geometry contracts; limited compatibility with accepted CFDLean mathematics.
- **Implementation model:** sequential Option load/compute/store execution, packet traversal, scalar/tensor Roe arithmetic, memory readiness and frame proofs. This arithmetic is exact real and the heap is an abstract address map.
- **Source identity/correspondence:** pinned full files/spans; two restricted leaf grammars checked in Lean; manual mappings for the remaining selected bodies and configured dispatch. The selector, accessor/ABI/heap interpretation and general C++ semantics are unverified.
- **Native observations:** baseline SU2 build/unit tests, actual methods on the tiny selected mesh, compiled mutations and unchanged refactor contracts, all preserved controls and selected full integration. These are finite tests, not proofs of production C++ semantics.
- **External literature status:** accepted CFDLean `c92c0f66360f85c0a3339ab9b30fc00c39ee5f93` (M2b J/A/R/L), acceptance record `b4fc959ba820a7a4752fff5d824f57f199b73d05`; 28 supplied review-payload files hash-verified. Full Roe numerical flux and far-field specifications are pending. Request forwarding/receipt/approval is never assumed.

## Robustness investigation

With admissible old rows, the preserved preprocessing selects an admissible current even when a trial current is inadmissible; snapshot copies it. Interior/timestep operations preserve old. Far field preserves old; stationary wall projection preserves admissibility; the explicit update writes current and frames old. The new invariant therefore excludes an inadmissible old row along these **exact-real** modeled steps, conditional on the explicit initial/geometry/source assumptions. This does not prove admissibility of the explicit Euler result.

The actual-method harness starts from actual admissible SU2 initialization, executes preprocessing/snapshot, timestep, **actual CIntegration::Space_Integration** and explicit update for 100 iterations. At selected CFL 0.1 it observed no inadmissible old row and no rejected trial. A separately labeled CFL 10 stress run observed 1,823 rejected trial rows and no inadmissible old row. CFL 100 terminated at SU2's residual-divergence guard (>10^20); it is an unstable-method observation, not evidence of the reported fallback defect. Binary-bit finite checks avoid relying on isfinite under the retained release flags. These finite cases do not settle floating-point reachability. The prior injected-invalid-backup counterexample remains hypothetical input, not a reachable production bug.

## Discrepancy register

| Item | Classification / verdict | Evidence / disposition |
|---|---|---|
| SetPrimVar fallback with invalid old backup | Potential issue / production reachability **not reproduced** | Existing injected counterexample; new conditional invariant and native trajectories above. No production patch. |
| Inadmissible trial after large explicit step | Numerical-method limitation | CFL 10 rejected trials; CFL 100 divergence outside selected CFL. No update-positivity claim. |
| SIMD vs scalar-boundary ratio, sound-square guard and energy evaluation | Disclosed implementation variants / unverified scientific and FP discrepancy | Exact selected source and separate models; no authoritative Roe specification copied from SU2. |
| Inactive lane diagnostic repeats | Source behavior / not a confirmed defect | Active residuals exclude padding; diagnostic source counts padding. Selected zero counters make it zero. |
| Incorrect timestep cap, reset, endpoint sign, index, active padding, skipped energy/last packet, normal sign | Hypothetical semantic mutations | Each modified source compiles and violates the same numerical contract; no claim these defects exist in baseline. |
| Geometry/ownership/heap assumptions | Unresolved verification obligations | Typed input contracts, source trace and fixture checks; full implementation refinement remains open. |

No new confirmed production defect was established, and no production SU2 code was changed. Floating-point, libm, C++ memory safety/lifetimes, compiler and MPI/OpenMP semantics remain unproved. The public reproduction kit excludes private CFDLean source and all project/dependency proof caches; its separate authorized private input remains local.
