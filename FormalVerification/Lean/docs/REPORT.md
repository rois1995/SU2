# Coverage report

This release contains 52 explicitly written Lean theorems in seven modules. Generated equation lemmas, constructors and structure
lemmas are additionally covered by the dependency audit but are not counted as new
numerical results. The final evidence JSON records the successful rebuild and audit of 231 logical declarations
(including 113 theorem declarations after generated lemmas), with only the three standard
logical axioms. The negative audit fixture was correctly rejected.

## Machine-checked properties

| Area | Main theorem names | Explicit assumptions / exact conclusion |
|---|---|---|
| Conservative/primitive consistency | primitive_roundtrip, conservative_roundtrip | Nonzero density; IdealGas has gamma>1 and R>0; two-sided inverse over reals |
| Thermodynamics | primitive_admissible, temperature_positive, pressure_specific_energy | Positive density/internal energy or primitive pressure as appropriate; matches staged ideal-gas pressure algebra |
| Geometry/orientation | edge_conservation, orientation_consistency | Distinct endpoint IDs; integrated +F/−F oriented contributions cancel; reversed orientation and reversed F have identical nodal effect |
| Assembly | assembly_conservation, scatter_correct, accumulation_correct, accumulation_permutation | Arbitrary finite event lists and initial residual; imperative-style scatter/fold equals independent sum specification; exact-real order independence |
| Conservative update balance | volume_update_balance, assembled_update_conserves | Positive volumes; one common dt; interior events only for zero global change |
| Wall relational contract | wall_project_contract, wall_contract_unique | Prescribed n with n·n=1, arbitrary w and conservative block; normal-only correction and wall work uniquely satisfy impermeability |
| Wall update invariant | impermeable_velocity, wall_update_preserves, Implementation.corrected_update_impermeable | Same n,w; a shared scalar relaxation; velocity equivalence needs rho≠0; model theorem establishes correction conditions rather than assuming them |
| Wall admissibility | wall_internal_energy, wall_preserves_admissibility | Unit normal; rho≠0 for identity and rho>0,q>0 for admissibility; exact gain q'=q+a²/(2rho) |
| Model correspondence | Implementation.sumLoop_correct, dotLoop_correct, stateProjection_correct, residualProjection_correct | Real arithmetic and the selected array interpretation; hand-written models refine the specification |
| Defect witness | Examples.repaired_defect_detected, legacy_update_not_impermeable, repaired_update_impermeable, legacy_result_admissible | Explicit finite 2D witness; old rule gives −1 defect, new gives zero; old output remains admissible |
| Assumption witness | Examples.nonunit_normal_counterexample | Dropping normalization can break impermeability; unit-normal premise is substantive |
| Refactor | Implementation.normalMomentum_correct, factored_satisfies_spec, rename_preserves_model, renamed_satisfies_spec | Accumulation extracted into a proved helper; separate local rename; composed model meets unchanged specification |

Additional helper theorems cover finite dot algebra, state extensionality, fixed points,
and idempotence. None assert entropy stability, convergence, consistency of a numerical
flux with Euler PDEs, error bounds, or correctness of a complete residual/update execution.

## Four evidence levels

1. **Mathematical definitions:** proved results listed above are checked by Lean.
2. **Formal implementation models:** scalar loop, scatter/fold and selected wall models
   are proved to implement the mathematical definitions under the stated assumptions.
3. **Actual C++:** exact commit/blob/hash identity is recorded. The extracted residual
   statements are compiled and tested. No formal C++ semantics theorem is delivered.
4. **Unverified correspondence:** array mapping, manual translation, extraction/wrappers,
   historical tests and finite numerical comparisons remain evidence, not proofs of C++.

The C++ experiment tests 1,000 deterministic finite cases in 2D/3D plus the exact defect
witness. Its helper-extraction and rename candidates are bitwise identical in those tests. Its generated
source and compiler/options are recorded. This does not prove correctness for all machine
inputs, compiler settings, or the surrounding BC_Sym_Plane routine.

## Assumptions and unsupported configurations

Physical inputs must satisfy the explicitly listed density, energy, gas and geometry
conditions. No proof establishes that SU2's preprocessing supplies them. Arbitrary
conservative increments are allowed in algebraic/update theorems, but their positivity
or generation by a linear solve is not assumed proved. Mathematical exact-real results
are conditional on Lean's standard classical foundations and a correctly checked pinned
library environment. The dependency audit rejects other axioms.

Excluded configurations/paths: real gas, turbulence/species energy, Navier–Stokes viscous
terms, RANS, DG/NEMO/incompressible layouts, multi-normal/sharp-corner walls, Jacobian
formation and solves, full implicit/explicit stage sequencing, geometry normalization and
motion generation, invalid-state fallback, mesh adaptation, MPI, periodic synchronization,
OpenMP, SIMD/reducer assembly, AD and machine floating-point exceptional values/roundoff.
Not all these exclusions apply to SU2 itself; they describe this formalization's coverage.

## Outstanding proof obligations

- Independently approve the specification meaning and revalidate the frozen artifact.
- Establish source-to-model correspondence with semantics/translation certificates,
  including ownership, aliases, bounds, branch selection and complete memory effects.
- Connect normalization, volume positivity and incidence orientations to mesh construction.
- Show every assembly event is visited exactly as specified, including partition boundaries.
- Formalize the selected numerical flux and all boundary/source terms.
- Establish wall-compatible linear corrections and time/stage ordering where updates are used.
- Add floating-point models and explicit error/exception contracts; real cancellation and
  normalization do not imply exact machine cancellation and normalization.
- Compose a complete serial Euler execution path before expanding configurations.

Local validation was performed by the working agent. Final independent validation has
not been performed. The user requested a frozen handoff for their own evaluator.
The release is a locally checked Phase 1 foundation, pending that independent acceptance.
