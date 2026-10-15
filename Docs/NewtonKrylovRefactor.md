# Common globalized Newton–Krylov refactor

The governing specification is `HANDOFF_NEWTONKRYLOV.md` in the parent workspace
(confirmed by the user on 2026-10-15). The work is on `feature_LineSearch`, starting
at `5615a8f5f6e9a7bfa88f0f279e1eadb7f89fe607`. The goal is still in progress.
SST now uses the common selected-equation controller. Production flow still uses
a temporary legacy compatibility class until its direction policies are migrated.

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

## Shared equation state and residual interface

`CNewtonEquationSet` is now used by the SST integration for trial installation and
residual evaluation. It also evaluates native flow/SA, flow/SST, flow/SST/LM and
incompressible flow/mixture-species groups in the integration tests. Equation
selection is validated for empty, duplicate, unavailable and unsupported systems;
NEMO is explicitly excluded. This is the residual/state foundation, **not yet a
fully coupled Newton solve or a replacement of the two Newton algorithms**.

For a Newton update the coordinates are `q = M_base*w`, where `w` is the stored
solver state and `M_base` is the accepted density for conservative scalars, one
otherwise. This is a fixed coordinate transformation during that update, not a
claim that `q` equals the physical transported `rho_trial*phi_trial`. The native
residual uses live trial density, mixture/transport properties, pressure and eddy
viscosity; their cross derivatives remain in the residual. The common controller
must use this same coordinate transformation for increments, scaling, mass terms
and block preconditioner input/output. Native scalar solvers expose their own
conservative flag and bounds, including signed SA bounds.

All trial unknowns are installed before refreshing properties. Each evaluation
communicates solutions, refreshes flow/turbulence/transition properties, installs
strong values through the existing boundary routines, refreshes gradients again,
and evaluates flow before auxiliaries so their characteristic boundary states
are current. Runtime equation configuration is restored on return. Explicit
residual mode survives native `SetGlobalParam` calls and preserves the assembled
preconditioner matrices. Invalid/nonfinite ideal-gas flow probes and native flow
restoration during primitive refresh return failure rather than silently forming
a derivative in a projected direction. Model-specific admissibility and boundary
constraint elimination for the final common operator remain to be completed.

Snapshots own every active dependent solution and `Solution_Old`; physical-time
history is never scratch. Each residual evaluation separately preserves the
caller's current `Solution_Old`, including when a conventional outer update has
replaced the preceding Newton history. Under-relaxation is restored on explicit
rollback. Runtime equation settings have defined defaults before their first
selection, so saving/restoring them does not read uninitialized configuration.
Residual-work counters in SST diagnostics count the physical equation residuals
performed by this interface, cumulatively in flow/turbulence/transition/species
order. They do not yet count all conventional outer-iteration work.

Validation/evidence in the parent `newton_krylov_refactor_20261015/` directory:

- `equation_set_units_verified.log`: existing 78 supported unit cases, 102250
  assertions, all pass. `equation_set_valgrind_verified.log`: zero memory errors
  for that suite (leak analysis was disabled).
- `equation_set_integration_verified_mpi.log`: three native-driver integration
  cases pass on 2 MPI ranks x 2 OpenMP threads (7577 / 7102 assertions on the
  ranks). `equation_set_integration_verified_serial.log`: 12475 assertions pass.
  `equation_set_integration_valgrind_verified.log`: zero memory errors for the
  serial integration suite (leak analysis was disabled). These check state mapping, exact
  repeated trial/residual values, physical-time history, preserved matrix
  diagonals, equation-config restoration, nonzero cross responses, centered
  step-size sensitivity, an independent SST pressure derivative and invalid
  flow-probe rejection. The integration executable is `UnitTests/newton_equation_tests`.
- `equation_set_smoke_history_final/`: matched baseline/current 10-update SST smoke
  controls, plus deliberately limited-linear-solve terminal recovery with exact
  measured nodal rollback and a failure checkpoint. Ten updates are not a
  convergence demonstration. The failure fixture now explicitly uses one linear
  iteration and a maximum defect of 0.001; the old one-trial setting alone did
  not force failure in the refreshed operator's first ten updates.
- `equation_set_regressions_history_final/`: six conventional regression runs and six
  configuration rejection checks. `equation_set_manifest.json` binds source,
  binaries, logs and configuration/input hashes.

Compatibility verification exposed pre-existing undefined inputs: the FVM base
loop chunk had no default when hybrid initialization was not called, and the MSW
TKE test omitted its sensor values. A chunk size of one is now the safe base
fallback; normal flow constructors still assign their calculated chunk size.
The test sensor is initialized explicitly. Several unit includes also resolved
through the parent checkout; they now resolve inside this feature source.
These changes affect shared initialization/test correctness, not NEMO Newton
integration. The original stalled/Valgrind diagnostic logs are retained.

The corrected-baseline binaries were archived as `baseline_7846e12020/` before
reusing the build directory. Its manifest and the older prototype `control/`
remain historical evidence; neither should be relabeled with the new source.

Reproduce the native integration checks with:

```sh
OMP_NUM_THREADS=1 /path/to/build/UnitTests/newton_equation_tests
OMP_NUM_THREADS=2 mpirun --bind-to none -n 2 /path/to/build/UnitTests/newton_equation_tests
```

## Shared grouped linearization and native block preconditioning (prior milestone)

`CNewtonLinearization` now forms `S^-1 J_R S + D` for an equation group, using
the fixed coordinates described above. All trial unknowns are installed together;
the physical product therefore includes the off-diagonal blocks. `D` is a supplied
diagonal pseudo-time operator. The SST caller supplies its existing volume/time
diagonal. The native coupled integration tests deliberately choose `D = 100 I`;
they do not validate the native incompressible or low-Mach preconditioned mass
matrices. Those mappings remain a requirement for the common controller.

The reference residual and probes use the same explicit path. This avoids an
implicit/explicit roundoff offset amplified by the finite-difference quotient,
while preserving the assembled native matrices. Newton scaling and probe
references are separate: the default relative probe reference uses the physical
scale, including at valid zero gauge pressure or zero species. SST supplies its
existing field-relative reference. Trial installation and the selected constraint
mask are shared; masked columns do not perturb the state and masked rows use an
identity equation. This tests the algebra of a supplied mask, not the completeness
of native boundary-constraint elimination.

`CNewtonBlockPreconditioner` uses the existing native preconditioner for each
diagonal equation block, with scaling applied at its input and output. Its lower
triangular mode subtracts the full matrix-free coupling action of previously
solved blocks before the next diagonal solve. Each additional block therefore
costs a grouped residual probe per preconditioner application. This is a measured
cost to include in comparisons; no performance benefit is presumed. It leaves
all cross blocks in the outer operator. SST now delegates its product and
preconditioning to these shared classes; its nonlinear controller and the legacy
flow Newton algorithm have not yet been consolidated.

The native tests solve manufactured linearized systems using FGMRES, with right
hand sides and final defects checked by a separate centered residual stencil.
Fixtures cover SA, SA_NEG at negative working-variable values, SST, SST/LM,
variable-density mixture species, and all four equation blocks together (flow,
SST, LM and eight transported species, 16 variables per point). Smooth reference
states have nonzero strain and interior intermittency; model kinks are not a
claim of a unique centered derivative. The tests also cover zero products without
residual work, constrained rows/columns, and rejected invalid products.

The buffer audit found that `CSysMatrix::MAXNVAR=20` applies to each native matrix
and is already enforced in `CSysMatrix::Initialize`. The grouped product and
preconditioner do not create a group-sized matrix: their state/Krylov vectors,
masks and `CSysMatrixComms` packets use the actual total group width. Existing
model limits still apply: dynamic scalar edge fluxes currently allow eight
variables (`numerics/util.hpp::MaxScalarVar`), and `CFluidScalar` allows sixteen
mixture components. Neither limit was enlarged to construct a wider test. The
largest native group tested here has 16 variables; groups wider than 20 were not
tested. Periodic transformations remain a separate open gate.

The new primal classes are excluded from differentiated builds, as SST Newton
already is. The SST header also excludes passive workspaces in that branch, so a
forward build does not acquire references to unavailable passive-vector symbols.
Four affected translation units passed forward syntax checks using the configured
compile commands plus the CoDiPack/MeDiPack include paths. This is not a complete
forward build/link/run or reverse-mode validation.

Evidence for that prior milestone is recorded in `linearization_manifest.json` under
the parent evidence directory. Four integration cases pass 14019 assertions
serially, and 8491 / 7965 assertions on two MPI ranks with two threads. All twelve
manufactured solves pass with both preconditioners; independent centered defects
are below 1.2e-6 in these fixtures. Valgrind reports zero memory errors (leak
analysis disabled). The existing 78 cases / 102250 assertions, six conventional
runs and six configuration guards also pass.

See `linearization_native_signed_serial.log`,
`linearization_native_signed_mpi.log`, `linearization_valgrind_signed.log`,
`linearization_supported_validated.log`, `linearization_smoke_validated/` and
`linearization_regressions_validated/`. Earlier failed fixtures and the smoke
control with seven extra retries are retained. Restoring SST's model-specific
probe reference restores the matched ten-update, ten-attempt control. These
remain correctness checks, not converged nonlinear or performance comparisons.

## Shared nonlinear controller milestone

`CNewtonIntegration` now owns one attempt, matrix-free/Krylov solve, feasible
endpoint, Armijo search, forcing/CFL update, bounded recovery and diagnostic path
for a selected group. `CSSTNewtonIntegration` is removed; `SST_NEWTON` constructs
the common controller for turbulence. The public C++ `Iterate` entry point also
supports individual flow, SA/SA_NEG, SST, LM and species, or one coupled group.
Runtime equation selection/group dispatch is not yet exposed in configuration.

The original flow implementation has moved verbatim, apart from its class name,
to `CNewtonLegacyIntegration`. The factory still uses it for `NEWTON_KRYLOV`.
This temporary compatibility path preserves startup iterations, nested solves,
initial tolerance relaxation, deflation and Jacobian relaxation until those
policies move into the common implementation. It is not the final one-controller
architecture. Four matched six-iteration flow controls have identical history
fields, including linear solver fields, against the archived preceding executable
on two ranks with two threads. A source-equivalence audit checks the class move.

Native implicit matrices now optionally track prescribed rows. This metadata,
not a guessed wall-variable mask or numerical matrix entries, defines constrained
rows and columns. Native strong routines that prescribe `Solution_Old` must also
install their current boundary values before gradients and source evaluation.
The residual interface saves and restores iteration history around that work.
Static no-slip/heat-flux walls are checked; moving-wall, wall-function and strong
composition/thermodynamic dependency ordering remain open audits.

Every selected block's native under-relaxation/clipping endpoint is computed
from the same accepted density before combining the endpoints. Scales distinguish
incompressible reference-relative enthalpy, which may be negative at positive
temperature, from physical temperature. Admissibility checks cover all dependent
states, retain signed SA_NEG, and reject invalid SST/LM states and mixture mass
fractions, including a sum above one. Full configured bounds and feasible
finite-difference directions near model/domain boundaries remain open.

Accepted residual statistics use the physical residual at the accepted state.
A production segregated dispatcher must additionally refresh all active systems
and observables after its last group. Pseudo-time currently uses a consistently
defined volume/time-step diagonal; auditing native incompressible and low-Mach
mass mappings remains necessary before comparing these modes.

Final controller checks pass eight native cases: **15717 assertions** serial,
and **9840 / 9240** on two MPI ranks with two OpenMP threads. They include the
prior independent coupled linearized solves, fourteen short nonlinear trajectories
(seven physics fixtures, segregated/coupled), and forced linear and late line-search
failures with exact solution/relaxation restoration and untouched physical-time
history. These short trajectories do not establish convergence. Valgrind reports
zero memory errors with leak analysis disabled. The supported 78 unit cases /
102250 assertions, six conventional regressions and six rejection guards pass.
Corrected-baseline/current SST controls both accept ten updates in ten attempts;
forced failure accepts seven, exits on attempt eight, restores exactly and writes
a restart without a numerical-failure MPI abort.

Evidence is bound in the parent directory's `controller_manifest.json` with
archived binaries. Final native logs are `controller_native_complete_serial.log`,
`controller_native_complete_mpi.log` and `controller_valgrind_complete.log`;
other checks are in `controller_supported_units.log`,
`controller_legacy_controls/`, `controller_smoke_validated/` and
`controller_regressions_validated/`. Eight translation units pass forward syntax
checks with commands saved in `controller_forward_syntax_commands.json`; this
is not full AD linking/running, reverse-mode or GPU validation. Earlier logs and
manifests remain historical.

## Next implementation gate

Migrate the legacy flow direction policies, then expose validated selection and
group dispatch with exactly-once advancement, final all-equation reporting and
whole-group failure output. Complete the native mass, boundary/periodic,
admissibility and conditional baseline audits. Address the handoff's ordered
thread-reduction and larger-fixture repeatability checks: current 5x5 fixtures do
not stress several 1024-point loop chunks per thread. Full converged family and
matched conventional/segregated/coupled comparisons remain required. NEMO
integration remains excluded.
