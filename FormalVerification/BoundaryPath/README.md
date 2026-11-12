# SU2 Lean: explicit boundaries and complete numerical update loop

New candidate on upstream `d162016382f0c122433000c114e88d808b48f7a7` with no
production patches. Independent evaluation is pending. The earlier EulerPath
candidate is preserved unchanged, including its pending evaluation status.

- [SPECIFICATION.md](SPECIFICATION.md): independent mathematical contracts and assumptions.
- [CORRESPONDENCE.md](CORRESPONDENCE.md): exact source routines and evidence boundaries.
- [PATH.md](PATH.md): composition, coverage and outstanding full-path obligations.
- [REPRODUCE.md](REPRODUCE.md): pinned offline cold reproduction and evaluator instructions.
- [RESULTS.md](RESULTS.md): worker proof/native evidence and exact frozen identity.

`Spec.lean` contains closed far-field and stationary-wall definitions, including
all affected buffers and the characteristic cache. `Boundary.lean` refines separate
implementation models and proves their geometric and admissibility contracts.
`Loop.lean` covers every owned point/component, getter reads, memory frames and
composition with the preserved assembly result. `Controls.lean` formally rejects
parseable numerical mutations under unchanged contracts.

The public mathematical theorem is `SU2Boundary.assembled_boundary_complete_update`.
It has explicit source-input/readiness premises. It proves an exact-real model,
not complete C++ execution-path or floating-point correctness. Interior SIMD Roe,
mesh/vertex traversal, buffer initialization, full C++ semantic correspondence and
floating-point behavior remain open.
