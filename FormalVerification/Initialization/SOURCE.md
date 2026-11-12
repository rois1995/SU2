# Source connection and trace

Numerical source remains upstream d162016382f0c122433000c114e88d808b48f7a7.
Develop was rechecked at 103442f30ca2fba1ff48ef12aa1ae93480256a30; numerical trees
are unchanged. No production patch was made. source-manifest.json pins entire
files and exact selected method/span bytes; scripts/source.py regenerates them.
Selection is an unverified Python step and its surrounding source must be reviewed.

| Operation | Production implementation | Evidence level |
|---|---|---|
| Conservative initialization | CEulerVariable constructor, val_solution and Solution_Old assignment | Exact-real storage model; constructor span native test; manual lowering |
| Truncation initialization | CFlowVariable constructor Res_TruncError.resize assignment | Pinned source/manual model; C++ allocation not proved |
| Primitive/fallback | CEulerVariable::SetPrimVar, SetVelocity, density/pressure/sound/temperature/enthalpy getters/setters; CIdealGas::SetTDState_rhoe | Point-model proof and native unchanged method bodies with mock storage; manual lowering |
| Old snapshot | CSolver::Set_OldSolution forwards to CVariable::Set_OldSolution and parallelCopy | Closed Lean grammar for complete wrapper and copy function, explicit operational semantics, full traversal/frame proofs |
| Matrix flat extent | C2DContainer.hpp size returns M*N | Source identity/manual concrete-size correspondence; four columns remains a selected configuration premise |
| Residual clearing | CEulerSolver::CommonPreprocessing -> LinSysRes.SetValZero | Zero-storage proof and native body with mock scalar assignment; full CSysVector assignment not translated |
| Interior | CEulerSolver::Upwind_Residual -> CFVMFlowSolverBase::EdgeFluxResidual -> SIMD ComputeFlux | Source trace only; not the scalar boundary Roe implementation |
| Timestep | CEulerSolver::SetTime_Step -> CFVMFlowSolverBase.hpp::SetTime_Step_impl | Source trace only, eigenvalue/volume/CFL/max-dt branches open |

Only snapshot/copy syntax has a new restricted source grammar. It consumes every
token and rejects extra statements and unsupported spelling. Correct, wrong-buffer,
incomplete and reversed-loop variants all parse and have explicit semantics.
`source_snapshot_execution` connects the correct parsed plan to executeSnapshot;
`source_incomplete_semantic` and `source_wrong_buffer_semantic` reject numerical
behavior, not parser failure. `source_refactor` proves equal whole-memory results.
The existing boundary, source-gate and update grammars are preserved unchanged.

Selected actual order (serial, MGLEVEL=0, explicit Euler):

1. CFlowVariable base initialization, then CEulerVariable conservative initialization;
   additional solver setup and geometry/freestream initialization remain open.
2. CMultiGridIntegration::MultiGrid_Cycle -> PreSmoothing: Preprocessing.
   CommonPreprocessing calls SetPrimitive_Variables over nPoint, including halos,
   and clears LinSysRes for the selected non-reducer, non-output branch.
3. At iRKStep=0: Set_OldSolution snapshots the corrected current solution.
4. SetTime_Step computes/stores dt.
5. Space_Integration dispatches first-order SIMD Roe interior computation; source,
   communication and residual strategy branches require separate caller qualification.
6. Weak far-field contributions precede strong stationary Euler-wall processing
   in the selected integration ordering; repeated visits remain preserved.
7. Explicit Euler updates all owned conservative components.

The current composition executes step 3, exposes steps 4–5 as OPEN relations and
then applies proved steps 6–7. It never moves the snapshot past them by an unproved
commutation. The point conversion model addresses step 2 independently; linking
its actual memory writes is the next useful connection. The complete caller trace
itself is manual/source-identity evidence, not a Lean proof of caller dispatch.

Native tests compile selected unchanged method bodies with mock containers and
fluid scratch in a C++17 harness. They do not execute full production classes or
prove C++ semantics. The new source-body fixtures use gamma=2, R=1 for exact reference witnesses,
not the configured freestream nondimensionalization. Finite test cases do not lift exact-real proofs to machine
arithmetic. Initial harness compilation failures (argument-size warning; unused
parameter in a mutant) were repaired and are not counted as defect controls.
