# What is and is not connected to source

All source identities are at `e0d3ee8d72598767227e62a9be5834a4554532b6`, recorded in
`source-manifest.json`. No new production defect or patch is proposed. The previous
moving-wall repair and its independent fixed/parent experiment remain in the accepted
Phase 1 record; the new scalar mutants are deliberate experiments, not discovered bugs.

| Layer | Evidence | Limit |
|---|---|---|
| Independent update/frame specification | Lean definition, generic storage proof, real bridge | Restricted memory model; no automatic machine-real equality |
| Restricted source implementation | Complete method parsed by kernel computation; generic correctness theorem | Operational semantics authored here, not proved equivalent to full C++ |
| Changed source behavior | Three source mutations parse into distinct programs; formal counterexamples and failed unchanged-contract proofs | Deliberate mutations only |
| Refactor | Parsed renamed/local-variable source; universal restricted-model equivalence and unchanged contract | Does not include floating-point flags, compiler behavior or full C++ observational equivalence |
| Production C++ source | Git pin, complete method selection, original-token comparison after preprocessing, tests using actual class/container | Extraction, C++ preprocessing/context interpretation and compiler are unverified |
| Native floating-point behavior | Sampled bitwise destination/frame checks, including zeros, subnormals, rounding, overflow, infinities and NaN | Tests only; no IEEE-754 formal proof |

## Exact production mapping

- `SU2_CFD/include/variables/CVariable.hpp:343`: scalar `AddSolution`; the full method is
  selected from `git show`, never a dirty working-tree file. The scalar argument is copied
  in before the body. The adjacent pointer overload at line 352 is excluded.
- The same header's `MatrixType` selects `unsigned long`, `su2double`, row-major storage,
  64-byte allocation alignment and dynamic rows/columns for `Solution` and `Solution_Old`.
- `Common/include/containers/C2DContainer.hpp`: `MATRIX_ACCESSORS` supplies `rows`, `cols`,
  `operator()` and its `m_data[i * N + j]` row-major address calculation. The dynamic
  specialization supplies `m_rows` and `m_cols`. This accessor is manually inspected and
  actually compiled in the test, but is not yet source-derived/proved by the Lean parser.
- `Common/include/code_config.hpp`: passive configuration selects `su2double = double`.
  Test static assertions reject other scalar types, non-LP64 or non-IEC559 hosts.
- `CFVMFlowSolverBase::CompleteImplicitIteration` in
  `SU2_CFD/include/solvers/CFVMFlowSolverBase.inl` calls this overload with
  `GetUnderRelaxation(iPoint) * LinSysSol(iPoint, iVar)`. Its continuous-adjoint condition,
  owned-node/component loops, relaxation calculation, linear solve and communication are
  excluded. The supplied increment in the theorem already exists at the call boundary.

`evidence/maintained-inspection.json` records that the same scalar method was also found
unchanged at maintained commit `c5129639310c1442f0ac7df466dbad426f00ec6c`. This is a
limited inspection record, not revalidation of that commit's caller or entire configuration.
Only the e0d source is required by the reproducible gate.

## Trust boundaries

The Python selector records a whole-header hash and byte span and embeds raw text; it
never emits an AST. It masks comments only while locating the unique method; selected
text remains unchanged and must pass the strict Lean parser. This selector is unverified.
The native script checks the compiler-preprocessed method's tokens against that same text.
That is useful correspondence evidence, not a verified preprocessor or C++ frontend.

All five programs are obtained by `parsedProgram source (by decide)`. Their parser-success
and AST theorems are checked by Lean's kernel. The parser does not skip unsupported branches
or provide a fallback AST. The mathematical contract and interpreter do not depend on
fixture names or source hashes. Mutations therefore fail for semantic reasons after
successful parsing. The three accepted counterexample theorems make the failure independent
of whether the chosen positive-proof tactic happens to find a proof.

The native fixture derives from the real `CVariable`, uses its protected default constructor,
allocates the actual two matrix members, and calls the inherited actual scalar overload.
It links the pinned `CVariable.cpp` with function-section garbage collection; it does not
provide mock accessors or a rewritten update. It does not exercise the full solver's
construction/configuration route or prove those routes establish the model's preconditions.
Only a disposable archived source tree receives the refactor and mutant edits.

C++17 sequencing and arithmetic interpretation were checked against the standard draft:
[assignment](https://timsong-cpp.github.io/cppwp/n4659/expr.ass) and
[additive expressions](https://timsong-cpp.github.io/cppwp/n4659/expr.add).
Those readings support the chosen sequencing and left association; they are not formal
proofs of correspondence. Standard heap/lifetime, aliasing and scalar arithmetic rules
remain outside the implemented language.

## Remaining proof obligations

1. Prove or independently validate the complete method/context selection, scalar overload,
   preprocessing and typedef/member resolution; turn the accessor's instantiated source
   into a checked memory-address representation.
2. Establish a C++ heap refinement: allocated and initialized matrix cells, valid indices,
   allocation lifetime, ownership, and preservation across the call.
3. Give addition/subtraction explicit binary64 semantics, including exceptional values,
   rounding and relevant environment effects, and prove a qualified bridge to the real spec.
4. Connect those source semantics to the actual compiled routine with an adequate verified
   frontend/compiler or explicitly validated translation. Hashes and native tests do not
   discharge this step.
5. Extend to the caller's multiplication and complete component loop, then compose state
   layout and admissibility obligations with existing Phase 1 update/wall theorems.

The previous [correspondence investigation](../Lean/docs/CORRESPONDENCE.md) discusses
certificate-checked restricted translation, BRiCk/cpp2v and LLVM validation as possible
routes. This candidate implements a small source-consuming Lean interpreter; it has not
run BRiCk/Alive2, proved a general frontend, or closed the full-C++ correspondence gap.
No Euler residual path, MPI/OpenMP/AD/SIMD, Navier–Stokes/RANS, real-gas EOS, implicit solve
or mesh adaptation execution path is claimed verified.
