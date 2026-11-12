# Follow-up after the provisional BoundaryPath review

Completed by this record:

- [x] Verify attachment members, candidate/ZIP/lock identities and record missing referenced file.
- [x] Preserve the external review verbatim and distinguish provisional review from acceptance.
- [x] Trace selected boundary/store/caller routines at the existing production baseline.
- [x] Identify incoming-memory, cache-layout, combined-frame and initialization obligations.
- [x] Check supplementary exact-rational witnesses for discarded writes and changed reduction order.
- [x] Preserve all existing production files, protected proofs, controls and handoff artifacts.

External gates remain open:

- [ ] Evaluator completes cold Lean build, theorem-type/axiom audit and full native/integration checks for the existing frozen candidate.
- [ ] User/evaluator records the exact accepted scope or findings; a worker rebuild does not substitute.
- [ ] Separate CFD_Lean agent obtains literature/specification approval; import only an explicitly approved pinned revision when available.

Proposed next implementation, without modifying the pending candidate:

1. Agree the boundary-entry memory relation and write footprint; keep input and post-state relations separate.
2. Extend source-connected storage views for primitive and per-marker characteristic rows, preserving 11-column stride and untouched fields.
3. Execute/prove each projection's complete reduction before stores; compose old/residual/truncation writes with frames.
4. Execute/prove far-field cache stores and residual accumulation using the disclosed closed source model, with explicit borrowed-pointer and scratch-state limits.
5. Fold the actual ordered visit list over evolving memory; establish post-boundary `InputsReady` rather than assume it.
6. Reuse the complete update theorem with the produced memory and prove the combined write-set frame from the original memory.
7. Preserve controls; add stale-memory, alias/layout, cache-frame, repeated-visit and reduction-order semantic controls under an unchanged approved contract.
8. Source-gate new supported syntax explicitly; reject unsupported syntax. Record manual caller connections that the parser does not establish.
9. Complete a new pinned proof/audit/native handoff only after the required review decisions; do not call the full Euler path verified.

Later caller work: allocation and initialization, primitive fallback/enthalpy consistency,
actual geometry/normal/visit construction, memory-level assembly, first-order interior
SIMD Roe, timestep computation, C++ heap/compiler/FP semantics and auxiliary method frames.
A future baseline gate may assess new upstream changes; this investigation does not
migrate away from d162016382f0c122433000c114e88d808b48f7a7.
