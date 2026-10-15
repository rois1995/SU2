# Common globalized Newton–Krylov refactor

The governing specification is `HANDOFF_NEWTONKRYLOV.md` in the parent workspace
(confirmed by the user on 2026-10-15). The work is on `feature_LineSearch`, starting
at `5615a8f5f6e9a7bfa88f0f279e1eadb7f89fe607`. The goal is still in progress.
The current flow and SST Newton implementations remain separate at this milestone.

## Required final state

- One `CNewtonIntegration` algorithm for mean flow, SA (including signed variants),
  SST, existing transition models and species. Remove `CSSTNewtonIntegration`
  after migrating its controller, diagnostics, recovery and detecting tests.
- Separate equation selection from grouping: individual systems, segregated
  Newton systems, and selected fully coupled groups; keep legacy configuration
  behavior and validate unavailable/unsupported selections.
- The full matrix-free operator must include cross derivatives. Install all
  trial unknowns before refreshing all dependent coefficients and residuals.
  Block-diagonal and block-triangular preconditioners use existing equation
  Jacobians without dropping cross derivatives from the operator.
- Define conservative scalar packing (`rho*phi` versus stored `phi`), density
  coupling, scaling, pseudo-time terms and model-specific admissibility.
  Finite differences must not silently project perturbations.
- Reproducible residuals, exact rollback of solution/history/bookkeeping,
  group-wide Armijo acceptance, adaptive forcing/CFL, true linear-defect checks,
  bounded recovery, failure output and nonzero terminal exit.
- MPI halos/periodic transformations, OpenMP, correct single advancement of each
  group, and audit of the `CSysMatrix::MAXNVAR=20` scratch storage.
- State-mapping, residual-order independence, cross-block/independent derivative
  references, step-size sensitivity, boundaries, rollback, recovery and MPI/thread
  tests, followed by converged representative cases for every supported family.
- Matched conventional, segregated-Newton and coupled-Newton comparisons on the
  same corrected baseline: wall time, residual evaluations, Krylov work, memory,
  every active physical residual, final quantities and stopping criteria. Process
  success or density-only convergence is insufficient.
- Reviewable commits, documentation, reproducible evidence. NEMO integration is
  excluded. Initial comparisons concern steady primal problems; the handoff does
  not establish unsteady, adjoint, multizone or GPU support for the new modes.

## Corrected baseline candidate

`TestCases/newton_krylov/baseline_fixes.json` pins every imported source commit and
records adaptations. These changes are applied individually, without merging
inherited model extensions or unrelated experimental work. Meson registrations
are merged with the existing feature, scalar and NEMO test lists.

| Corrections | Source commits | Detecting checks |
| --- | --- | --- |
| Krylov restart/recycling, tolerance/state, monitoring | `a9b9339828`, `52378aca05`, `97c5d583f5`, `42d241a63b`, `f6683944dc`, `eb5d695c5f` | `[LinearAlgebra]`; invalid FGCRODR configuration |
| Identity halos, owned ILU levels | `8d96b78d70`, `caeebe2669` | `channel_identity` 1/2 ranks; `lam_naca0012_ilu_levels` 2 ranks, 1/2 threads; owned-row unit |
| Quantized upper storage size | `285ea29c2c` | Existing quantization tests; asymmetric-storage coverage remains to audit |
| Exact static-energy under-relaxation | `d1cf7ea33b`, `3062a04822` | `[UnderRelaxationAD]` primal values; forward/reverse derivative variants registered |
| HLLC face frame/Jacobians and reconstructed enthalpy | `a59d827531`, saved enthalpy diagnostic | `[HLLC]`, `[HLLC-face]`, including nonzero SST k |
| Roe wave strengths and nonphysical matrix clearing | `a277f59157`, `ffc5bafc7a` | `[Roe]` contacts/shear and a valid-then-invalid face |
| MUSCL physicality, axisymmetric Jacobian/source, boundary kind, source compatibility | `c333a18ebd`, `536c8d8578`, `df8f9787cd`, `b29368df17` | `[FlowSources]`; source-slot rejection verified; MUSCL physicality and turbo boundary coverage still to audit |
| Incompressible minimum-pressure reduction and moving-grid Jacobians | `ca7aee08cc`, `583c32afd6` | Targeted coverage still to audit before using these benchmark features |
| SST k in fluxes, Jacobians, boundaries and sources | `c0dc0cee39`, `3eb9dc89be`, `8536636377` | `[Roe][SST]`, `[AUSM][SST]`, `[HLLC]`, `[FlowSources]`; nozzle density/velocity inlet |
| SST option combinations/freestream/inlet correctness | `580800a8ec`, `a002e77705`, `37dcfedd0e` | Nozzle regression; `[Config][SST]` option-order tests; freestream/engine rejection checks |
| Flow/scalar van Albada forms and affected references | `75621f9687`, `30c52aa29c` | Scalar MUSCL unit/regression tests; broad V&V references still to validate on this combined base |
| R4 symmetry, wall-distance setup/width validation (including heat follow-up) | `8e2b3aa1c4`, `5657a4728e`, `e16616e237`, `36f4e474a0` | `[Limiters]`, `flatplate_wall_limiter`; invalid-width/no-wall cases |

The under-relaxation import already declares owned `const su2double` densities;
`3062a04822` therefore contributes its tests without replacing those declarations.
The experimental linearized energy update is deliberately excluded, as required
by the handoff's comparative evidence. SST model extensions/new transition models
and turbomachinery experiments are also excluded. Conditional wall-function,
moving-wall, characteristic-outlet and transition dependencies still require an
applicability audit for their eventual comparison cases. Existing scalar MUSCL,
scalar epsilon, BCGSTAB and rotational-periodic develop changes are retained.

The original executable and unit driver are preserved under the parent workspace's
`newton_krylov_refactor_20261015/control/`, with hashes and source commit. The old
`scalar_limiter_sst_globalization_20261015` results/manifests are historical and
must not be relabeled as validation of changed source. A separate build/evidence
directory is used for all new runs.

## Reproduction and validation status

The candidate is not yet an accepted convergence-comparison baseline. Record the
actual outcomes below; missing checks in the table above remain open requirements.

From the feature source directory, using a configured MPI/OpenMP release build:

```sh
ninja -C /path/to/build SU2_CFD/src/SU2_CFD UnitTests/test_driver
/path/to/build/UnitTests/test_driver '[LinearAlgebra],[HLLC],[HLLC-face],[Roe],[AUSM],[FlowSources],[Limiters],[UnderRelaxationAD],[Config]'
python3 TestCases/newton_krylov/run_corrected_baseline.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --data /path/to/su2-TestCases --output /path/to/new-evidence
```

The baseline runner reuses registered regression values, refuses existing output
directories, stores logs/configurations and records binary/input/suite hashes.
It checks short correctness regressions, not final convergence or acceleration.
Full supported unit runs require the established unit-fixture directory layout;
the MLP fluid test requires MLPCPP, which is disabled in the local build.

Validation on 2026-10-15 (GCC 9.4, release, MPI/OpenMP, skylake,
MLPCPP/TecIO/CGNS/differentiation disabled):

- Build passed. Focused numerical/controller suite: 2,378 assertions / 29 cases.
- Final supported visible unit suite: **102,250 assertions / 78 cases passed**.
  The MLP fluid case is excluded because its optional library is disabled.
  Intentional-abort hidden cases are selected separately, never by a broad
  negative Catch filter. Commas in exact test-name filters need escaping or
  unique prefix patterns; the saved explicit list covers all 78 cases.
- Six short regressions passed: serial/parallel identity channel, wall-distance
  limiter flat plate, SST density/velocity inlet, and ILU on 2 ranks with 1/2
  threads. Six configuration checks passed: excessive FGCRODR deflation,
  zero limiter width, wall limiter without walls, conflicting source slots,
  SST engine markers and nonpositive SST freestream.
- HLLC reconstructed-face test: **56/656 failed before** the enthalpy correction;
  the final focused/full suites pass it, for 2D/3D, fixed/moving faces and
  k=0/0.15. This is a preconditioner-Jacobian correction, not a change in the
  matrix-free nonlinear operator.
- Original channel control: log10 RMS density -2.044866 (1 rank), -1.563665
  (2 ranks). Corrected: -2.045800 / -2.045798. The imported reference also
  predates the combined under-relaxation correction. Serial and parallel
  references now record their actual values separately; the final linear
  residual differs by about 1.9e-5 in log10 units across ranks. No tolerance
  was loosened to conceal that difference.
- No NEMO integration changes. Existing NEMO compatibility tests in the visible
  suite still run as part of the repository's unit coverage.

Evidence is under `newton_krylov_refactor_20261015/` in the parent workspace:
`units_supported_verified.log`, `supported_units.txt`, `units_focused.log`,
`hllc_before_mpi.log`, `regressions_with_guards/results.json`,
`regressions_with_guards.log`, `channel_control/`, and `source_manifest.json`.
The regression runner records configuration, fixture, suite and binary hashes;
`source_manifest.json` binds the complete changed-source set and executables.
The earlier failed filter/reference runs are retained as diagnostic evidence.

Forward/reverse builds, the full integration/V&V matrix and the conditional
features marked open above have not been validated on this combined base.
No fully converged family matrix or Newton speedup is established here.

## Next implementation gate

Complete and pin baseline validation, then replace the two integration algorithms
with the common selected-equation residual/state contract. Implement coupled
cross derivatives and block preconditioning before interpreting convergence
benefits. Controller-only tests do not satisfy the final goal.
