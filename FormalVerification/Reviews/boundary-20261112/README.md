# BoundaryPath provisional review and memory-gap investigation

This records the user-supplied Agent B review of frozen candidate
`73a322b640f45505e8d8fb70fbce46e6ccffd206`. It is **not acceptance of that Lean
candidate** and introduces no new proof candidate, theorem or production patch.
The user authorized recording the provisional review and investigating the memory gap.

- [External review](external/SU2_Lean_boundary_loop_independent_review_20261112.md)
- [Original evaluator record](external/SU2_Lean_boundary_external_evaluator_record_20261112.json)
- [Evaluator native fragment results](external/evaluator_result.json)
- [Original attachment](external/Agent_B_SU2_Lean_20261112.zip)
- [Worker attribution and exact identities](review.json)
- [Source investigation and proposed proof boundary](FINDINGS.md)
- [Follow-up tasks](TODO.md)
- [Pinned source hashes and line selections](source-evidence.json)
- [Supplementary rational diagnostic](diagnostic.json)
- [Record-integrity checks](validation.json)

The evaluator reports successful archive/source identity checks and recompilation
of nine fragment controls: original and refactor pass, seven numerical mutations
compile and violate their unchanged contracts. The evaluator explicitly did not
complete the cold Lean/theorem/axiom checks or full SU2 native/integration checks.
The earlier EulerPath candidate also remains pending independent formal acceptance.

The report identifies a real proof-composition obligation: `InputsReady` currently
assumes the update memory already contains the boundary result. This investigation
traces the selected source writes and specifies the work needed to derive that
relation from pre-boundary memory. It does not claim the gap is now proved closed.

All four supplied report members are retained byte-for-byte. The three members
listed by the evaluator's checksum record match. The fourth referenced file,
`CFD_Lean_new_agent_instructions_20261112.md`, is absent from the supplied ZIP; this
omission is recorded, not repaired by inventing content. No CFD_Lean files were
created or edited. The proposed mathematical-library assignment is not treated as
an approval of its specifications.

The frozen BoundaryPath, EulerPath, accepted foundations/source gate, compiler
pins, specifications, controls and handoff archives remain unchanged. This review
adds only `FormalVerification/Reviews/boundary-20261112/` on a separate branch.

## Reproduction and interpretation

Check `review.json` against the original attachment and the delivered ZIP checksum
`a7b5115deca59e7c938632157a5eb0255f21736e107aa05a4bdb5d1d72b24879`.
The frozen candidate lock is
`7c6a795c2dc8ef748bd4cdd9cdba3dc31330e55decb4b504b947d592654ef37e`.
For each `source-evidence.json` selection, obtain `git show REVISION:PATH`, compare
its full SHA-256, then hash the indicated inclusive, one-based line range with
original line endings. The observed statements are BY READING, not newly proved
C++ semantics. The source baseline remains `d162016382f0c122433000c114e88d808b48f7a7`.
No fresh upstream baseline gate was performed; the later CI-only revision is
attributed solely to the evaluator's report.

`diagnostic.json` is reproducible with Python's exact `fractions.Fraction`:
project each vector x as x - dot(x,n)*n, then compute
old_projected - (dt/volume)*(residual_projected + truncation_projected).
Compare with running the update on the original, unprojected memory.
This is an explanatory arithmetic diagnostic, not a Lean or production-C++ test.

No Lean or native solver build was rerun for this record-only change. Existing
proof/native evidence retains its previous worker/evaluator provenance. Cold
acceptance must be completed by the external evaluator against the frozen identity.
