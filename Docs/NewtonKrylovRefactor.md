# Common globalized Newton–Krylov refactor

The governing specification is `HANDOFF_NEWTONKRYLOV.md` in the parent workspace
(confirmed by the user on 2026-10-15). The work is on `feature_LineSearch`, starting
at `5615a8f5f6e9a7bfa88f0f279e1eadb7f89fe607`. The goal is still in progress.
Production steady primal CPU flow and selected flow/SA/SST/LM/species groups now
use the common controller. Equation selection, segregated versus coupled solves,
CFL ownership, final physical residuals and group failure output are wired into
the native driver. A compatibility class remains for legacy execution modes
outside this migration. Dense native pseudo-time mappings and explicit-flow
preconditioning are supported. The full goal remains in progress: converged
matched family comparisons and the conditional-feature audits below are pending.

## Production selection and dispatch

For an available flow/SST/LM configuration:

```ini
NEWTON_EQUATIONS= (FLOW,TURB,TRANS)
NEWTON_COUPLED= YES
NEWTON_BLOCK_LOWER= YES
```

`NEWTON_COUPLED= NO` advances the same selected equations in separate Newton
solves. FLOW, TURB, TRANS and SPECIES can be selected independently; unselected
equations retain their native updates. The list order determines packing and
lower block preconditioning. The earliest native dispatch owns a coupled group,
using the native FLOW, TRANS, TURB, SPECIES iteration order. Group members are
skipped in later native hooks, so each equation advances once.

The generic `NEWTON_*` controls in `config_template.cfg` configure globalization,
forcing, finite differences, Krylov work and diagnostics. `NEWTON_KRYLOV= YES`
without an explicit selection selects common FLOW in eligible steady primal CPU
cases and retains its existing integer/double direction controls. To use those
controls with an explicit group, include FLOW in the selection. `SST_NEWTON`
retains its separate legacy settings and delegates to the same common algorithm;
combining it with explicit `NEWTON_EQUATIONS` is rejected.

Explicit selections currently require steady primal single-zone CPU execution
without periodic markers, with `MGLEVEL=0`. New auxiliary selections do not yet
restrict updated scalars for multigrid flow; the SST legacy wrapper retains its
existing restriction. Pressure-based incompressible
selection is guarded because its separate correction iteration is not integrated
here. Native conventional pressure-based behavior is retained. Selected scalars
require implicit discretization; FLOW can use the explicit diagonal fallback.
Unavailable, duplicate and unknown equations, conflicting legacy selections,
unsupported execution modes and invalid numerical controls fail configuration.
NEMO integration is excluded. AD/reverse/mixed-precision/GPU execution is not
established by the syntax checks or primal runs.

Each common controller owns the selected equations' CFL. Native flow adaptation
continues for unselected flow and does not overwrite selected scalar CFLs.
Unselected auxiliaries follow the accepted flow controller's CFL. After all
updates and native outlier-flag updates, one common owner refreshes every active
named equation's physical residual/statistics and flow forces at the final state.
This prevents earlier segregated residuals from describing an intermediate state.

A failed group restores its captured state and stops remaining auxiliary updates.
The driver reports non-convergence, exits nonzero and writes all active restart
fields with `_newton_failure`; the SST alias keeps `_sst_newton_failure`.
Diagnostics use `newton_coupled.csv` for a group, or `newton_flow.csv`,
`newton_trans.csv`, `newton_turb.csv`, `newton_species.csv` for separate owners,
with matching `_trials.csv` files. Rows report physical defects, merit, accepted
steps, recovery decisions, residual work and native/outer linear iterations.

The production dispatch tests compare complete states and CFLs against direct
controller calls for 24 configurations over three updates, including explicit
flow, reverse packing, segregated/coupled solves, LM and mixture/turbulence/
transition groups. Independent final residual sums check every active equation.
Auxiliary-only tests also verify native flow CFL adaptation continues while
selected CFLs remain owned; failure tests check exact rollback and skipped
remaining auxiliaries. These short checks do not establish final convergence or
performance. Historical checkpoints below retain their original evidence scope.

Production-checkpoint verification (2026-10-15): 22 native cases / 41,700
assertions pass serially; 26,671 / 21,528 assertions pass on two ranks x two
threads. Three production cases pass 8,072 / 8,696 assertions on two ranks x
four threads. All 80 supported unit cases / 102,273 assertions pass. Fourteen
affected translation units pass forward syntax checks; the final configuration
guard has a separate forward recheck. Focused production Valgrind reports zero
memory errors, with leaks unchecked.

Thirteen executable profiles cover legacy flow selection, individual auxiliary
systems, partial/full coupled groups, segregated groups and explicit flow.
Thirteen config guards pass. A forced group failure restores exactly, exits
nonzero without numerical MPI abort and writes all eleven restart fields for
flow/SST/LM/species plus coordinates. Eight conventional profiles keep identical
six-iteration histories. The SST alias accepts 10/10 updates in baseline/current
controls; its forced failure accepts seven and fails on attempt eight with exact
restoration and its legacy restart suffix.

Evidence uses `production_dispatch_` in the parent workspace's
`newton_krylov_refactor_20261015/`. Final native logs use `monitored_` (nonzero wall
drag); earlier native fixtures and executables are retained as historical
controls. `run_production_dispatch_checks.py` records profile commands and
binary/configuration hashes. The initial restart-filename assertion failed
because the default was `restart`; the verified runner pins `restart_flow`.
This was a harness failure: the initial failure restart was written correctly.
Native reporting failures are handled by the bounded path below. Conditional
boundary/model bounds, near-bound derivatives and larger production-layout
repeatability still need their stated audits before completing the full goal.

## Bounded native residual reporting

Equation-set evaluations now reject nonfinite residual entries collectively after
native spatial assembly, including dependent flow residuals and final all-active
evaluations. Finite-difference probes receive a failed evaluation rather than a
numerical MPI abort or a projected derivative.

`CSolver::RefreshResidualStatistics` supports a non-aborting call that returns a
collective validity result. It retains the native NaN/excessive-RMS checks and
restores previously published RMS, maxima, point indices and coordinates on
failure. The failure decision is global even with `COMM_LEVEL=MINIMAL`. Ordinary
native reporting keeps its default abort policy and existing valid arithmetic.

The common controller reports its physical base before native preparation, then
prepares each native matrix with provisional statistics deferred. Its physical
right hand side remains independent of native sign changes and multigrid
truncation data. Reporting validation precedes acceptance bookkeeping: an invalid
trial restores the whole captured group and published base statistics before
bounded CFL recovery. An invalid base stops immediately. Such attempts are
recorded as `RESIDUAL_FAILURE`; forcing, accepted CFL handoff and reconstruction
history are committed only after successful reporting.

Final reporting validates every active named residual. A failure stops further
updates, retains valid statistics and writes a failure restart. The driver skips
convergence monitoring once Newton has declared failure. Other native setup and
configuration errors retain their existing handling; this does not establish
all conditional thermodynamic/boundary failure paths as audited.

`[NewtonResidualFailure]` tests inject NaN, infinity, excessive finite values and
sum-of-squares overflow into all four native equation families on one rank. They
check FULL/MINIMAL communication, deferred preparation and restoration of the
reporting policy. A nonlinear source test accepts the scaled Armijo merit while
exceeding the native energy RMS limit, then verifies retry, exact rollback and
restored statistics. Other tests poison provisional truncation data and compare
against clean physical updates, or invalidate an unselected final residual and
check finite restart fields through the actual driver.

The self-contained CLI check needs no external mesh:

```sh
python3 TestCases/newton_krylov/check_residual_failures.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD --output /path/to/new-evidence
```

Optional `--before` records an older executable alongside the current one;
`--ranks` and `--threads` select the layout. The cases include compressible SST,
incompressible constant-density flow, explicit flow/SST, stiff mixture/species,
and the ordinary native abort control. All Newton failure restarts are checked
for finite solution data and exact state/merit restoration. Variable-density
body force subtracts the operating density, so its freestream mixture case tests
stiff linearization rather than an excessive physical base residual.

Residual-reporting checkpoint evidence: **27 native cases / 43,029 assertions**
pass serially; **27,717 / 22,571** pass on two ranks x two threads. The five
focused failure cases pass **1,043 / 1,046** on two ranks x four threads.
All **80 / 102,273** supported unit checks pass. Eighteen affected units pass
forward syntax checks. Focused Valgrind reports zero errors; leaks are unchecked.
Eight conventional six-iteration histories remain identical, all thirteen
production profiles/thirteen configuration guards pass, and the existing SST
10/10 acceptance and 7/8 failure controls retain exact restoration.

Evidence is stored with the `residual_failure_` prefix in the parent workspace.
Final native logs use `residual_failure_recovery_`; earlier uncommitted-stage
fixtures are historical checks. The CLI runner records commands, binary/config
hashes and finite restart fields. The archived compressible and constant-density
executables abort MPI; the current Newton paths exit with bounded failure and
finite checkpoints. The ordinary native control still aborts in both versions.
The original mixture fixture expectation is retained as a harness diagnostic;
its density-subtracted source has a stiff derivative and a finite base residual.
Larger production-layout repeatability, conditional boundary/model bounds,
periodic support and converged matched family comparisons remain open.

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

## Corrected-baseline reproduction and historical verification

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

## Shared equation state and residual interface (prior milestone)

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

## Shared nonlinear controller (prior milestone)

`CNewtonIntegration` now owns one attempt, matrix-free/Krylov solve, feasible
endpoint, Armijo search, forcing/CFL update, bounded recovery and diagnostic path
for a selected group. `CSSTNewtonIntegration` is removed; `SST_NEWTON` constructs
the common controller for turbulence. The public C++ `Iterate` entry point also
supports individual flow, SA/SA_NEG, SST, LM and species, or one coupled group.
Runtime equation selection/group dispatch is not yet exposed in configuration.

At this milestone the original flow implementation moved verbatim, apart from its class name,
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

## Ordered reductions and larger-layout audit (prior milestone)

The common controller now sums thread partials by thread index, following the
existing vector-dot pattern. The temporary legacy flow finite-difference norm
uses the same ordering. Native residual statistics accumulate on their owning
threads in a fixed order, preserving active arithmetic ownership. Tied MPI
maxima now report coordinates from the selected point. This changes shared
residual reporting; it does not introduce NEMO integration into the refactor.
The legacy source-equivalence claim above applies to the preceding controller
milestone, before this norm-ordering correction.

A hidden `[NewtonRepeatability]` native test uses a nonuniform 193x193 flow/SA
fixture, with at least three 1024-point chunks per participating thread on every
rank. It checks repeated physical residuals and Jacobian products, one accepted
coupled update, forced linear/late-search failures, exact restoration and intact
physical-time history. A synthetic reduction with three small contributions and
one large contribution detects reordered addition; tied maximum points and their
coordinates are checked too. A separately linked control restoring only the old
unordered statistics merge fails 12 RMS assertions and the maximum-point checks.
The production executable/source are not replaced for that detecting control.

Three runs each of 1x1, 1x4, 2x1, 2x4 and 4x2 complete their native assertions.
Actual thread counts, owned-point counts, binary/configuration/state/partition
hashes and attempt/trial records are saved. Preconditioner partitions are fixed
to one, with ILU level scheduling enabled; MPI partitioning still changes across
layouts. At identical installed states, residuals and products match exactly.
Matrices, time steps and native preconditioner actions reconstructed after
restoring the initial state also match exactly in these samples.

**Full solve repeatability is not established.** The 2x4 and 4x2 repeats have
accepted-state differences: maximum relative L2 **6.45e-9**, maximum absolute
**0.06037** in stored energy coordinates. A forced search-failure
solve uses two versus three Krylov iterations in the 2x4 repeats, changing its
residual-evaluation count by one. Accept/reject outcomes, step lengths, CFL and
exact rollback remain consistent. The audit runner intentionally returns nonzero
for the bitwise mismatch. Initial-state operator/preconditioner snapshots do not
locate the first divergence inside the solve. Earlier diagnostic runs, including
a warm-up variant that matched in three 1x4 samples, are retained; they do not
resolve this finding. Do not describe tolerance agreement as bitwise determinism.

Authoritative artifacts are `ordered_repeatability_native_actions/results.json`
and `comparison.json` in the parent evidence directory. The runner and analysis
scripts preserve exact comparisons and numerical differences separately. The
standard eight native cases still pass 15717 assertions serially and 9840 / 9240
on two ranks with two threads. Valgrind reports zero memory errors (leaks not
checked), 78 supported unit cases / 102250 assertions pass, all four legacy flow
controls remain identical, and the SST controls plus six conventional/six guard
regressions pass. Eight affected translation units pass forward syntax checks.
The new `ordered_manifest.json` binds this source, archived binaries and evidence.
There is no MPI/OpenMP race-detector or converged-layout validation yet.

## Block-preconditioner synchronization correction

The remaining hybrid variation above came from the block preconditioner's input
handoff. `CSysVector` assignments deliberately use `nowait`; their documentation
requires consumers with another work schedule to synchronize first. FGMRES
normalizes a basis vector with such an assignment. The block preconditioner then
copied it with native 1024-point loops, allowing fast threads to read unfinished
parts of the vector. A single entry barrier in the shared block preconditioner
fulfills that contract for both diagonal and lower-triangular modes.

A focused native regression deliberately delays the last producer before the
vector assignment. Both preconditioner modes fail before the fix, with a maximum
error of approximately 0.001612 against the synchronized reference; both pass
exactly afterward. The old detecting executable, source and failing log are
preserved in `producer_prebarrier/`. An isolated trace executable also made
three 2x4 solves identical because its snapshot barriers synchronized the inputs;
that was diagnostic evidence, not the production fix.

With the uninstrumented production correction, **all three repetitions of all
five layouts (1x1, 1x4, 2x1, 2x4, 4x2) now match exactly** in saved configurations,
partitions, initial/accepted fields, residuals, products, reconstructed native
actions and attempt/trial diagnostics. This includes the previously differing
Krylov counts and evaluation counts. Cross-layout differences are recorded
separately; native MPI preconditioners need not be mathematically identical.
These are fixed-layout samples of one flow/SA fixture, not universal determinism
or converged-layout evidence.

The standard native suite passes **9 cases / 15722 assertions** serial and
**9845 / 9245** on two ranks with two threads. The delayed-producer case additionally
passes with four threads on one and two ranks. Valgrind reports no memory errors
(leak checking disabled). The 78 supported unit cases / 102250 assertions pass.
The legacy flow/SST controls and conventional regressions are recorded separately.
The affected translation unit passes a forward syntax check; complete AD/GPU
and MPI/OpenMP race-detector coverage remain open.

Current evidence is `producer_barrier_repeatability/{results,comparison}.json`,
`producer_regression_{old,fixed,fixed_mpi}.log`, `producer_native_{serial,mpi}.log`,
`producer_valgrind.log` and `producer_supported_units.log` in the parent evidence
directory. `producer_manifest.json` binds the committed fix, archived binaries,
before/after tests and repeatability results. Earlier failed audits remain
historical; do not overwrite their manifests or describe them as passes.

## Legacy policy boundaries before consolidation

The legacy explicit diagonal preconditioner also consumed a `nowait` input with
a different point schedule. A matching entry barrier fixes its delayed-producer
regression (approximately 2.30e-5 error before, exact afterward). The same entry
protects mixed-precision input copies; a complete mixed-precision build has not
been run. The detecting test covers explicit diagonal, implicit weak and implicit
nested preconditioning in the current precision build.

The equation-set residual guard now saves/restores the legacy reconstruction
relaxation setting and evaluates the full physical reconstruction. Previously,
leaving that setting at 0.25 or zero changed the physical residual consumed by
the common controller: the nonuniform MUSCL fixture detects differences of about
8575 and 11433 respectively. A Jacobian model knob must not alter the physical
merit or true grouped derivatives. The regression now checks explicit probes,
implicit assembly and restoration of the caller setting after a rejected invalid
trial. Legacy flow still applies its own direction-policy relaxation outside
this shared physical-residual interface; migrating that policy remains required.

The native suite passes **11 cases / 15749 assertions** serial and **9872 / 9272**
on two ranks with two threads. The two focused cases additionally pass on one
and two ranks with four threads. Supported units remain 78 / 102250 and Valgrind
reports zero memory errors, with leak checking disabled. Exact repeated-layout
and legacy controls are recorded with this source; no converged or full AD/GPU
claim follows from these checks.

Evidence uses `legacy_policy_manifest.json`, with before controls preserved in
`legacy_producer_prebarrier/` and `legacy_relaxation_pre_guard/`. Current checks
are `legacy_policy_native_{serial,mpi}.log`, `legacy_policy_valgrind.log`,
`legacy_policy_supported_units.log`, `legacy_policy_focused_{serial,mpi}.log`,
`legacy_policy_repeatability/`, `legacy_policy_controls/`, `legacy_policy_smoke/`
and `legacy_policy_regressions/` in the parent evidence directory. Both affected
translation units pass forward syntax checks; full differentiated executables
and mixed-precision execution remain unchecked.

## Prior implementation gates (superseded by production selection above)

Migrate the legacy flow direction policies, then expose validated selection and
group dispatch with exactly-once advancement, final all-equation reporting and
whole-group failure output. Complete the native mass, boundary/periodic,
admissibility and conditional baseline audits. Retain the delayed-producer
regression and the larger-fixture layout checks while migrating the remaining
direction policies. Full converged family and
matched conventional/segregated/coupled comparisons remain required. NEMO
integration remains excluded.


## Common flow direction policies (prior milestone, 2026-10-15)

The common controller now accepts the legacy flow controls explicitly. Startup
uses the assembled native approximate operator in the same scaled coordinates as
the main solve. Subsequent directions use FGMRES or FGCRODR, with optional nested
native FGMRES, BCGSTAB or fixed-count smoothing in each block preconditioner.
Startup counters, automatic reconstruction relaxation and CFL handoff advance
only after acceptance. The existing tolerance-relaxation and physical-state
finite-difference metric are retained. These are direction policies within the
shared attempt/search/recovery algorithm, rather than another nonlinear solve.

A relaxed Jacobian model has its own reference residual and products. Physical
right hand sides, true defects, merit values and directional slopes always use
full reconstruction, restoring the caller's configuration afterward. Diagnostics
record startup/deflation, reconstruction factor, tolerance factor and nested
native iteration counts. The main coupled operator still evaluates physical
cross derivatives; startup deliberately uses the native block approximation.

The native MUSCL fixture exposed a finite-difference truncation problem: a small
pressure perturbation can strongly change a tangential wall limiter. Flow groups
with reconstruction now compare successive one-sided Richardson products,
refining at most sixteen times until relative agreement reaches 5e-4. Failure to
establish agreement rejects the direction. At least one free coordinate must
have a representable perturbation; a sub-machine request cannot establish a zero
derivative. Independent centered stencils at two sizes validate smooth,
asymmetric physical and relaxed-model states, with relative error below 2e-4
for requested nondimensional probe sizes 1e-4 and 1e-6. Native limiter ties are
piecewise smooth: no unique centered derivative at a branch tie is claimed.
Refinement adds residual work, which remains included in the evaluation counters;
performance has not been established.

Reconstruction's nonphysical-edge counters are iteration history. Residual probes
preserve them, anchored trials restore them, and accepting a flow update commits
the candidate counters once. Gradients and slope limiters remain live trial
quantities. Auxiliary-only updates do not advance the external flow's counters.
The seeded-history check also found that padded SIMD lanes updated an edge twice;
passing the active-lane mask to reconstruction fixes this native behavior.
BCGSTAB's exact alpha step previously reported zero iterations and the initial
residual; it now reports the completed step and its actual residual. Both issues
have detecting before/after checks.

Validation includes sixteen four-update flow/flow-SA policy trajectories,
independent physical/model derivatives, repeated/invalid probes and exact history
restoration. All fourteen native cases pass serially and on two ranks x two
threads; the three new cases also pass on two ranks x four threads. The supported
unit selection now includes the new BCGSTAB case. Four legacy production profiles
retain identical six-iteration histories, and SST success/failure restart controls
retain their established outcomes. This is correctness evidence for short
trajectories, not a converged family or performance comparison.

Production flow remains routed to `CNewtonLegacyIntegration` until the
explicit-diagonal fallback, dispatch/CFL ownership and whole-group failure output
are integrated. The generic API still requires native
implicit matrices. Keep this limitation explicit when continuing production
selection; do not treat the common-policy milestone as completion of the goal.


## Native pseudo-time mappings (prior milestone, 2026-10-15)

`CNewtonPseudoTime` freezes the native per-equation pseudo-time action at each
accepted base and applies it in the same scaled coordinates as the grouped
operator. Ordinary compressible flow and native scalar equations retain their
scalar diagonal. Incompressible, low-Mach and Roe-Turkel flow retain their full
native flow block, transformed as `S^-1 M S`. These blocks remain diagonal between
equation systems; density/species/turbulence cross derivatives stay live in the
physical residual. This is steady pseudo-time regularization in the defined
fixed coordinates, rather than a physical-time integration scheme.

Native dense flow preparation and the common mapping now call the same
`CSolver::GetPseudoTimeMatrix` support. Incompressible implicit mass construction
uses an explicit mode argument, so a residual-only probe cannot accidentally
select the explicit inverse. Native conventional implicit, explicit and
pressure-based behavior is checked against the preceding executable.

Prescribed coordinates are eliminated from the mass action. Constraints are
read after native implicit preparation, and the preconditioner restores identity
rows after native mass assembly can reintroduce couplings. The same frozen mass
is used in products, true defects, pseudo-time merit and directional slopes. Its
action on the actual feasible endpoint is cached for merit trials. Nonpositive
or nonfinite pseudo-time coefficients cause a collective direction failure.
The arbitrary-diagonal linearization API remains available for independent
linear-system references.

Twenty 2D/3D model/dimension combinations compare the action against native
Jacobian additions, including reordered flow/SA, flow/SST, flow/SST/LM and
incompressible mixture groups. Relative differences are about 2e-16. The tests
verify off-diagonal contributions, residual-only mode independence, prescribed
rows and a bad time step on one MPI rank. Independent centered scalar-merit
references cover both merit choices and both flow-control policies. An isolated
control retaining full products while using a diagonal-only merit fails six
assertions. The flow-policy trajectories now cover flow, SA, SST, LM and mixture
species: forty four-update trajectories, not converged CFD comparisons.

The pressure-based native matrix is covered as a compatibility reference.
Newton flow groups explicitly require the density-based incompressible
formulation because a pressure-based iteration carries additional
pressure-correction state. This restriction does not alter conventional
pressure-based runs. Explicit-diagonal Newton support, production selection and
exactly-once group dispatch remain implementation gates. Moving/conditional
boundary constraints, near-bound derivative behavior and broader model support
still need their stated audits before a full support claim.


Native-mass checkpoint verification: sixteen native cases / 23,830 assertions
pass serially; 16,866 / 12,372 assertions pass on two ranks x two threads. The
three focused mass/policy cases pass 6,824 / 2,934 assertions on two ranks x four
threads. All 79 supported unit cases / 102,253 assertions pass. Valgrind reports
zero memory errors (leaks unchecked), and eight affected translation units pass
forward syntax checks. Eight conventional profiles and four legacy-flow profiles
retain identical six-iteration histories. SST acceptance and bounded-failure
restart controls and all six conventional regressions/six guards pass.

The final 37,249-point flow/SA fixture is repeated three times in each of five
layouts (1x1, 1x4, 2x1, 2x4, 4x2). Fields, products, native actions, decisions and
telemetry match exactly within each sampled layout; cross-layout differences are
recorded separately. This is a sampled repeatability check, not universal race
certification or a fully converged comparison. Concurrent verification jobs
caused substantial scheduling contention; their elapsed times are not solver
performance measurements.


## Explicit flow without a native sparse matrix (prior milestone)

The common controller supports FLOW configured with `EULER_EXPLICIT`, preserving
legacy Newton's raw `(volume + periodic volume)/dt` diagonal preconditioner. It
skips native implicit preparation and startup for that block. Scalar equations
selected for Newton still require implicit discretization. The full physical
matrix-free operator and native mass retain all flow/scalar cross derivatives
and dense incompressible/low-Mach mass; the diagonal is only a preconditioner.

`CSysMatrix` can track prescribed rows without allocating coefficients. The
metadata has its own point/variable dimensions, and native explicit strong
boundary paths mark the same rows as their implicit counterparts. Equation-set
construction enables tracking for dependent equations. An implicit preparation
clears and refreshes the metadata; physical probes retain preparation-only
constraints. Frozen coordinates use identity preconditioning. Conventional
solvers without tracking remain unaffected.

Verification: all nineteen native cases pass (25,516 assertions serial;
17,975 / 13,456 on two ranks x two threads). Three explicit-focused cases pass
1,088 / 1,154 assertions on two ranks x four threads. Tests include metadata
without coefficients, 2D/3D compressible and mixture diagonal actions, delayed
OpenMP producers, and four accepted updates for seven flow/model families in
both segregated and coupled configurations. Every accepted attempt checks the
true physical defect and Armijo inequality; flow matrix storage remains absent.
All 79 supported unit cases / 102,253 assertions pass. The explicit cases have
zero Valgrind errors (leaks unchecked); eight affected units pass forward syntax.
Eight conventional and
four legacy implicit profiles retain their six-iteration histories; a separate
archived/current legacy explicit-flow pair also matches exactly. These are
correctness checks, not converged comparisons or production factory migration.

Evidence uses the `explicit_metadata_` prefix in the parent workspace's
`newton_krylov_refactor_20261015/`; the compatibility script records the source
configurations and binary/history hashes. Conditional boundary semantics and
near-bound derivatives still require the broader audits listed above.
