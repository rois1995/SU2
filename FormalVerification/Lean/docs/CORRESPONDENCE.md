# Source correspondence and its limits

All line references below refer to the source snapshot pinned by `source/manifest.json`,
not the user's currently edited branches. The array interpretation is rho=U[0],
m_i=U[1+i], E=U[d+1]. The Lean channel type expresses that interpretation without
machine indices. A proof of that layout/heap interpretation for C++ remains required.

| Production routine / selected slice | Lean representation | Evidence level |
|---|---|---|
| CEulerVariable.hpp:284 SetVelocity (density/energy getters at 237/243); CEulerVariable.cpp:112 SetPrimVar; CIdealGas.cpp:43 SetTDState_rhoe | toPrimitive, pressure_specific_energy, dotLoop | Kernel-checked exact-real identities; manual source mapping |
| CVariable.hpp:343 scalar AddSolution, called from CFVMFlowSolverBase.inl:588 CompleteImplicitIteration | conservativeUpdate | Exact-real update theorem; manual mapping of scalar overload; pointer overload has different semantics and is not conflated |
| CEulerSolver.cpp:2028 Upwind_Residual, ReducerStrategy=false | scatter, assembleLoop | Fold/scatter refinement proved in Lean; manual mapping to AddBlock/SubtractBlock |
| CSysVector.hpp:619 SetBlock, :631 AddBlock, :639 SubtractBlock | pointwise addition/subtraction | Source inspected; C++ bounds, aliasing, parallelism and floating-point arithmetic not proved |
| CFVMFlowSolverBase.inl:1243–1268 BC_Sym_Plane old-state correction | Implementation.stateProjection | Exact-real hand-written loop model proved equal to wallProject |
| Same file :1270–1283 residual correction and :1320–1335 truncation correction | Implementation.residualProjection | Exact-real loop model proved equal to wallProject; extracted residual statements tested separately |
| Same file :2993 onward MultigridProjectEulerWall compressible correction | Implementation.stateProjection | Same algebraic operation with sol[0], including density increments; selection/ordering of multigrid fields unproved |

The pinned `CFVMFlowSolverBase.hpp:1154` routes BC_Euler_Wall to BC_Sym_Plane.
The dot-product helper is included as `source/Common/include/toolboxes/geometry_toolbox.hpp`.

`Implementation.sumLoop_correct` proves finite serial scalar accumulation equals a list
sum. `dotLoop_correct` connects ascending finite-coordinate accumulation to dot product.
These are formal implementation models, distinct from the mathematical wall relation
and assembly sum. They are manually written. **There is no proof in this project that
production C++ executes those models.** The exact source snapshot and the extracted
slice tests reduce ambiguity but do not close that gap.

## Branch accounting

The selected wall model covers the classical compressible layout: regime COMPRESSIBLE,
iVel=1, nVar=nDim+2; dynamic_grid=true uses supplied w, false means w=0. It models only
one already-selected normal and one owned node. It includes density, all momentum
components, and total energy of that projection. It covers arbitrary real residual and
truncation vectors, including zero or negative density increments.

The enclosing production routine also includes halo skipping, normal lookup or
normalization, nonclassical conservative layouts, reflected-state/upwind energy work
for other layouts, implicit matrix row transformations, neighbor traversal, threading,
and source-state selection. Those branches are **not formalized**. Neither are the
Jacobian block multiply, full implicit solve, real-gas EOS, MG cycling, communication,
MPI/OpenMP, geometry updates, AD types, SIMD/reducer strategy or invalid-state fallback.
The explicit finite-volume update theorem is an independent mathematical foundation;
no complete SU2 explicit integration routine is claimed translated. The scalar
AddSolution correspondence does not prove the linear solver produces a wall-compatible
increment: the theorem states that prerequisite explicitly.

## Repair and refactor experiments

The parent's residual correction uses m_res·n, instead of m_res·n−rho_res(w·n). The Lean
counterexample starts with rho=1, m=(2,0), E=5, n=(1,0), w=(2,0) and density increment 1/2.
Initial and incorrectly updated states are admissible. The old correction yields wall
defect −1; the repaired correction yields zero. This isolates a previously repaired
behavior without asserting equivalence of the entire old/new solver paths.

`scripts/source_experiment.py` extracts the actual residual statements from both source
snapshots, keeps the compressibleProjection conditional with its selected true value,
and compiles them with a small bounds-checked array wrapper. It tests the same witness
and finite 2D/3D cases. Extraction, wrapper and test oracle are unverified. The separate
full-SU2 historical unit tests are recorded in BASELINE.md, not rerun or claimed as proofs.

The refactor experiment extracts the existing normal-momentum accumulation loop into a
`normalMomentum` helper and calls it before the unchanged correction statements. It
preserves accumulation order, reads and arithmetic. The Lean helper has its own proved
sum contract (`normalMomentum_correct`), and `factored_satisfies_spec` proves that the
composed refactor satisfies the unchanged wallProject definition. The C++ experiment
checks bitwise equality against the original for the finite test set. A separate local-
variable rename also has a definitional-equality proof and bitwise test. These demonstrate
specification reuse across helper extraction and renaming; they do not establish general
C++ refactor validation or justify reassociating floating-point expressions. No production
refactor was applied.

## Routes to rigorous C++ correspondence

1. **Certificate-checked translation of a restricted kernel into Lean.** Preserve
   statement order, indices, memory updates, all branches and scalar semantics. Define
   a small source/IR operational semantics, prove the translator or validate a per-kernel
   translation certificate, then prove refinement to these specifications. A regex
   extractor, Clang AST dump, source hash or manually reviewed transcription is not such
   a certificate. Frontend correctness remains explicit until proved/validated.
2. **C++ semantics in BRiCk/Rocq.** The primary documentation describes a C++ program logic
   and cpp2v frontend, including limitations for templates and other language features.
   This is a plausible path to memory/statement correctness of small instantiated SU2
   kernels, but is not a Lean backend and requires an explicit bridge between the two
   proof environments. We have not run it on SU2 or discharged its frontend trust.
   Sources: [BRiCk](https://skylabsai.github.io/BRiCk),
   [cpp2v](https://github.com/SkyLabsAI/BRiCk/blob/main/rocq-skylabs-cpp2v/README.md).
3. **LLVM transformation validation with Alive2.** Useful for some compiled refactor/
   optimization comparisons, subject to its supported IR and validation limits. It
   does not by itself prove a CFD mathematical specification or C++ frontend correctness,
   nor export the required Lean proof here. Not executed in this phase.
   Source: [official Alive2 repository](https://github.com/AliveToolkit/alive2).

Next recommendation is route 1 for the small residual and scalar-update kernels, with
float semantics introduced explicitly rather than replacing machine operations by reals.
That is an engineering proposal, not a claim of available verified C++→Lean translation.

## Trust boundary

Real-number proofs do not handle rounding, NaNs, infinities, signed zero, overflow,
underflow, FMA contraction, normal normalization error, or platform/compiler/AD behavior.
Exact-real order invariance of sums is not bitwise order invariance of machine sums.
All current proof obligations are checked relative to Lean's standard logical axioms
and the pinned imported library environment. Review meaning, source correspondence and
axiom dependencies separately; see [Lean's validation guidance](https://lean-lang.org/doc/reference/4.33.0/ValidatingProofs)
and [Lean FAQ on external translations](https://lean-lang.org/faq).
