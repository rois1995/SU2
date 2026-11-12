# Worker results and frozen identity

The new principal result is
`SU2BoundaryMemory.source_boundary_to_complete_update`. It starts with pre-boundary
memory, executes the selected boundary model, derives post-boundary `InputsReady`,
uses the preserved complete explicit update theorem, and proves the combined frame
relative to the original memory. No correct post-boundary-memory premise remains.

The exact commit, tree, candidate-lock hash and ZIP checksum are supplied in the
external delivery manifest. This avoids a self-referential commit hash inside its
own protected files. All 534 historical formal/review files from investigation
commit `5a6d3a578a4a348b91ef6ce8dcd259e142f60d37` remain byte-identical, including
BoundaryPath at `73a322b640f45505e8d8fb70fbce46e6ccffd206`.

| Evidence | Worker result |
|---|---|
| New Lean project and source connection | Build succeeds with warnings treated as errors; 115 explicit theorem declarations |
| Transitive new-namespace dependency audit | 584 logical declarations, including 306 theorem declarations; only propext, Classical.choice, Quot.sound |
| Meaningful negative controls | Eight numerical postcondition violations proved: discard, wrong buffer, partial projection, stride, marker, vertex, untouched cache and repeated visit |
| Refactor | General exact-real memory equality, including failures, for reversed independent correction stores; positive fixture values match bit-for-bit |
| New compiled native controls | Original and refactor pass; all eight mutants compile and then fail the numerical contract |
| Actual pinned SU2 method checks | 24,968 checks; 20 points, 110 far visits, 69 wall visits, seven flow regimes, two far passes, eleven-column frames |
| Preserved native controls | Nine boundary/loop variants and eleven scalar/composed variants retain expected outcomes |
| Private upstream build/unit tests | 545 assertions in six selected upstream test cases pass |
| Full configured solver execution | Two 20-iteration runs, identical final conservative-state files; observed positive density/internal energy |
| Production changes | None |

`evidence/` contains worker logs, native results, release build options, theorem
types and audit/negative-control evidence. The full frozen checker also reproduces
all preserved checks, verifies protected files, reselects source bytes, checks pins,
rejects an injected external bad axiom, and attempts all eight false postconditions.
A numerical negative is counted only when the unchanged contract reduces to False
or the compiled fixture reports its numerical mismatch. Parser/hash/compiler
failures are not counted.

Development used pinned local mathematical dependency caches. The source-only
restoration recipe supplies no project/dependency compiled Lean artifacts, enabling
a genuinely cold evaluation by the user's evaluator. Worker execution is not
independent acceptance, even if performed in a fresh local directory.

The prior EulerPath and BoundaryPath cold/dependency/full-native independent
acceptance remains pending. This candidate also remains pending external approval.
The far-field/Roe literature requirements are provisional; the Agent A request is
prepared locally for user forwarding and no approved CFDLean dependency has been
imported. The accepted foundations and earlier accepted source gate are preserved.

No new production defect is confirmed. This is connected exact-real model progress,
not complete Euler execution-path, C++ machine-memory, floating-point or compiler
verification. See `assumptions.json`, `CORRESPONDENCE.md` and `PATH.md` for the
remaining interfaces and unsupported configurations.
