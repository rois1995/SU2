# Worker evidence and acceptance status

- Pinned Lean 4.19.0 build succeeded, including approved private CFDLean M1.
- 63 explicit new theorem declarations compile with warnings treated as errors.
- Transitive audit: 299 initialization/CFDLean logical declarations, including 159
  theorem declarations; only propext, Classical.choice and Quot.sound were reached.
  Internal unsafe compiler-runtime declarations are inventoried separately and
  no proof dependency is exempted from the axiom traversal.
- Nine native variants compiled: original and reverse-copy refactor pass 500 checks
  each; seven numerical mutants fail the unchanged contract. Tests use unchanged
  source bodies in mock storage. Compiler failures in preliminary harness attempts
  are excluded from these semantic results.
- Full frozen checker additionally regenerates source, checks prior file identities,
  prints theorem types/axioms, rejects a transitive external bad axiom, and runs
  seven deliberately false postcondition proof attempts. Exact output belongs in
  the separately delivered worker-validation directory.
- Prior BoundaryMemory controls/specifications are preserved byte-for-byte. The
  full preserved check chain is requested in this candidate's worker validation.

Worker dependencies may be warm; this is not independent cold acceptance. No new
full native SU2 build/integration acceptance is claimed by these fragment tests.
The external recipe includes preserved full native and integration work. Source-only
restoration is tested separately and proves packaging, not theorem acceptance.

The user's external source-level BoundaryMemory PASS is recorded in
review-decision.json. Its independent kernel/native acceptance is still pending.
New initialization, point conversion and composition results require their own
independent review. Mathematical M1 acceptance does not approve Roe/far-field.
