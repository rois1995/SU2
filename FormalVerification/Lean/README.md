# SU2 Lean foundations — Phase 1

A real Lean 4/Lake project with exact-real specifications, separate implementation
models, and 52 explicitly written, machine-checked theorems. Local validation passed, including a fresh project rebuild and dependency audit. **Final specification approval and
independent acceptance remain pending the user's evaluator.**

The work does not establish whole-solver, floating-point, or production-C++ correctness.
No SU2 production file was modified. Source correspondence is explicitly qualified.

- Lean: `leanprover/lean4:v4.19.0`, compiler commit `6caaee842e94`.
- mathlib: `c44e0c8ee63ca166450922a373c7409c5d26b00b` (v4.19.0).
- SU2: `e0d3ee8d72598767227e62a9be5834a4554532b6`, reusing its moving-wall repair.
- Every transitive Lake dependency is locked in `lake-manifest.json`.

## Build

Install the toolchain named in `lean-toolchain` with elan. From this directory:

```sh
lake build
python3 scripts/verify.py
```

`verify.py` uses the task-local elan installation automatically if present, limits itself
to two CPUs and two Lean threads, checks source/dependency identities, rebuilds every project module,
audits proof dependencies, verifies rejection of a temporary bad-axiom fixture, and runs
the supplementary C++ source-slice experiment. Python 3 and g++ are needed for that last
step. It obeys the workspace load threshold. `lake build` alone checks the Lean library
and its dependency audit. `lake exe cache get` is an optional way to obtain mathlib build
artifacts before a normal build; a clean independent validation can build from source.
Do not run `lake update` to change dependencies in this frozen release.

## Read the results

- [Specification and assumptions](docs/SPECIFICATION.md)
- [Coverage report and open obligations](docs/REPORT.md)
- [Baseline and defect repair](docs/BASELINE.md)
- [C++ correspondence and branch exclusions](docs/CORRESPONDENCE.md)
- [Proof dependencies and full Euler roadmap](docs/DEPENDENCIES.md)
- [Independent evaluation procedure](docs/EVALUATION.md)
- [Local validation evidence](evidence/local-validation.json)

The seven modules in `SU2/` contain the actual definitions and proofs. `Integrity.lean`
checks their transitive axiom dependencies. `source/` contains exact, licensed source
snapshots and the pre-existing repair patch. Tests never replace Lean proofs.

The demonstrated defect is the historical loss of moving-wall impermeability when density
changes but the old residual correction removes all normal momentum. Lean proves a
finite, admissible counterexample and proves the repaired operation's general invariant.
A helper-extraction C++ refactor is tested and its formal model proved equivalent without
changing the mathematical specification.
