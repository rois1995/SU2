# Source mapping and selected execution trace

`source-manifest.json` pins complete source-file hashes, selected byte spans and fixture hashes to d162016382f0c122433000c114e88d808b48f7a7. `scripts/source.py` is an unverified extractor; source identity is independently replayable with git show. A successful hash check is not a C++ semantic proof.

| Production routine / storage | Formal representation | Correspondence status |
|---|---|---|
| CVariable constructor; CFlowVariable constructor; CEulerVariable seed / old assignment | constructorZeros, executeConstructor; preserved seed/copy contracts | Manual selected-field projection; actual constructors tested; allocation/copy semantics open |
| CIdealGas constructor, default argument in CIdealGas.hpp, STANDARD_AIR factory | gasParameters, freshFluid, FluidReady; entropy true | Manual source mapping; actual default constructor and selected shared model tested |
| CEulerVariable::SetPrimVar | executePoint, point_execution | Ordered exact-real load/store model; manually mapped C++ method; native extracted body and actual method tests |
| SetVelocity, GetDensity/GetEnergy, four setters, SetEnthalpy | executeVelocity/Thermo/Checks/Enthalpy | Ordered model and refinement proofs; manual callee mapping, not certified C++ translation |
| CIdealGas::SetTDState_rhoe | executeFluid, fluid_values/frame/overwrite | Source-derived scalar model including entropy; source-body/native observations; libm and object heap open |
| CEulerSolver::SetPrimitive_Variables | parsePrimitiveLoop, executePrimitivePlan, points_refine/source_loop_refines | Complete restricted Lean grammar and exact-real callee execution; serial macro, GetFluidModel and empty secondary-call interfaces qualified |
| CEulerVariable::SetSecondaryVar | selected nSecondaryVar=0 no-op | Source-inspected dispatch; extracted complete method; no general secondary-variable semantics |
| CommonPreprocessing and Preprocessing | executeCommon | Manual selected caller/control model; other config branches explicitly excluded and actual caller tested |
| LinSysRes.SetValZero / scalar assignment | clearResidual, residual_clear_complete/frame | Preserved modeled bulk clear plus actual caller observations; C++ vector implementation correspondence open |
| CVariable::Set_OldSolution and parallelCopy | executeSnapshot; preserved restricted grammar/copy proofs | Previously frozen source gate reused unchanged; source-body and actual-method tests |
| CMultiGridIntegration::PreSmoothing | trace below | Source identity and inspected dispatch only; no full integration-call interpreter |

The selected source order is:

1. CEulerSolver::Preprocessing → CommonPreprocessing: reset ErrorCounter; SetPrimitive_Variables over `0 <= iPoint < nPoint`; all-point local count; selected mesh-zero COMM_FULL one-rank publication; residual zeroing.
2. In the first RK substep, Set_OldSolution snapshots the corrected current state.
3. SetTime_Step computes and stores timesteps.
4. Space_Integration computes interior SIMD Roe residuals and processes boundary markers.
5. Time_Integration executes the explicit Euler update.

The new snapshot theorem supplies the shape of BoundaryRep **before timestep/interior execution**, with zero residual. It does not assert that this zero residual is the actual pre-boundary residual after interior fluxes. The intervening operations must be connected and their frame/data obligations proved. There is no shortcut invoking boundary processing before interior work.

Selected profile: 2D, serial passive double, fixed mesh, steady EULER, standard ideal gas, mesh zero, COMM_FULL, first-order Roe, explicit Euler, non-output preprocessing, no reducer strategy, MUSCL off, low-dissipation off, no centered/MSW, no data-driven fluid, no extra secondary variables, no engine/actuator/fixed-CL features. Main operational theorems support arbitrary real trials and invalid backups within that profile. MPI/OpenMP, AD, viscous/RANS, moving mesh, implicit integration, restart, multigrid, gradients, thermal/data-driven fluids and other preprocessing branches are unsupported here.

The new finite parser accepts the **complete** primitive caller with original increment, equivalent prefix increment, or a skipped-last condition. Unknown/extra syntax fails. Its plan executes the memory interpreter, not an unconstrained callback. Callee models remain manually constructed and are labeled accordingly. Other newly modeled source operations do not acquire a general C++ grammar by being listed in the manifest. The extractor fails if anchors are missing/ambiguous; this is a source-identity aid, not semantic defect detection.

New native controls preserve seven previous semantic controls and add skipped-last point, early snapshot, bad primitive stride, omitted velocity reset and omitted entropy assignment. Original/refactored source bodies pass; twelve mutants compile and fail numerical requirements. Bad stride, early snapshot and wrong residual mutate the mock caller/storage interface; the others mutate extracted C++ bodies. Such mutants are hypothetical controls, not confirmed production defects. Lean additionally proves skipped-row and early-snapshot inadmissibility, skipped-entropy rejection, the parsed skipped-row rejection and arbitrary-input fluid-refactor equivalence. Parser/hash/compiler failures are not counted as these controls.
