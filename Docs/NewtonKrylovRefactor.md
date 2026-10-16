# Common globalized Newton–Krylov refactor

## Common Newton periodic equations and code documentation, 2026-10-16

The shared equation interface now retains periodic copies as explicit unknowns.
For native stored state `w`, its physical residual is
`F(w) = P*R(w) + B(w)*(I-P)*w`. `P` averages equivalent copies and rotates flow
momentum; `B` is live density for conservative scalars and identity otherwise.
The consistency term removes the missing periodic modes without averaging trial
primary values. Full finite differences retain arbitrary directions and live
cross-equation responses. The native `P*A*P+(I-P)` matrix remains an approximate
block preconditioner, rather than the definition of that full derivative.

Each attempt fixes group coordinates `q=M_base*w` and positive state scales `S`.
The pseudo-time action is `S^-1*P*D*P*S`, with local partial-cell masses in `D`,
including dense incompressible/low-Mach flow actions. The consistency modes get
no extra pseudo-time mass. Base time steps are explicitly synchronized using the
native periodic minimum before implicit assembly. Residual evaluations freeze
this iteration history and restore the solver's original periodic mode, including
when flow uses explicit time integration. No new numerical controls or relaxed
acceptance tolerances are introduced.

The common selection and legacy implicit `NEWTON_KRYLOV` dispatch admit core
periodic mean-flow, turbulence and existing transition groups. Periodic species
and streamwise combinations remain explicitly excluded from the new selection
pending their separate applicability audits; existing legacy routing is retained.
NEMO, further species/flamelet development and multigrid are outside this work.

Four native cases independently check the residual consistency term, native mass
mapping and nullspace, arbitrary scaled products against centered differences,
and production-driver acceptance plus forced linear-failure rollback. They cover
2D translations/intersections and 3D rotation, explicit/implicit flow, SA, SST and
LM, reversed block packing and selected subgroups. Initial detecting failures,
the four implicit/explicit roundoff comparisons, and the passing same-mode exact
restoration checks are retained separately. The independent references do not
call the production projector. All 53 native cases pass on 1x1, 1x2, 2x2 and
2x4 layouts (104170 assertions on either single-rank layout; 61774/55562 per MPI
rank). All 88 ordinary cases pass 105424 assertions. The four periodic cases
pass 41122 assertions under Valgrind with zero errors (leaks unchecked), and
all three existing CLI checks pass. This includes the legacy periodic flow
factory dispatch. Focused test success alone does not establish converged
endpoints. All six changed production translation units pass forward-mode syntax
checks; this does not establish AD runtime. Converged sector Newton qualification
remains open. Commands, hashes and failures are pinned in
`Docs/NewtonKrylovPeriodicCommonResults.json`.

The first sector pilot retained fixed Newton CFL controls and the native LU-SGS
preconditioner. Native flow converged in 1049 updates, but Newton terminated at
minimum CFL after derivative verification rejected a direction; its last history
entry is iteration 2329. This is a failed endpoint qualification, not convergence
or a wall-time timeout. All recorded rollbacks restore state exactly. Writing
the failure restart then received SIGBUS in OpenMPI's shared-file component while
`/tmp` had only 125 MiB free. The solver failure and subsequent I/O failure are
recorded separately. The complete source, binaries, logs and results are retained
in `periodic_common_initial_ef407ae13a9f`. Follow-up comparisons use workspace
MPI temporary storage and the existing adaptive-CFL policy, without changing
physical equations, derivative acceptance tolerances or minimum-CFL recovery.

The shared equation, linearization, pseudo-time, preconditioning and integration
interfaces now document coordinates, lifetimes, side effects and failure results
using SU2 Doxygen conventions. SU2 section comments explain dependency refresh,
feasible derivative probes, periodic constraints, endpoint readback and bounded
recovery. The implementation and tests retain descriptive local names and small
native blocks instead of exposing a large packed group to fixed solver buffers.

## Periodic baseline corrections, 2026-10-16

The fork's current periodic branches were fetched and audited before changing
the common Newton periodic path. Fourteen existing correction/review commits
from `fix_periodic_axis`, `fix_periodic_counts`, `fix_periodic_implicit`,
`fix_periodic_transforms`, `fix_periodic_wall_distance` and
`fix_periodic_support` are integrated. Exact source commits and all eight
audited branch tips are pinned in `TestCases/newton_krylov/baseline_fixes.json`.
The rotation/limiter corrections already in the develop-derived base are
retained. Streamwise dependencies require a separate applicability audit.

The native implicit operator keeps partial periodic rows and uses
`K = P*A*P + (I-P)`, where P averages equivalent periodic copies and rotates
momentum. This includes matching off-diagonal dependencies and its transpose.
Additional corrections cover axis copies, intersecting stencils, slip normals,
volume refresh, inverse affine transforms and translated wall-distance queries.
Unsupported periodic continuous-adjoint, radiation and elasticity configurations
fail explicitly. Existing Newton row metadata, scalar updates, limiter scaling
and native pseudo-time interfaces are preserved.

Conflict resolutions combine axis edge classification with the new intersecting
weights, retain `HasPeriodicRotation`, combine independent tests/AD registrations
and keep the existing default unit-fixture coloring with the imported partitioned
mode. The final support branch's removal of temporary test helpers is honored.
An unrelated backscatter reference edit from an older branch base is excluded.

All 13 periodic/limiter cases pass on 1x1, 1x2, 2x2 and 2x4 layouts (3259
assertions serial, 2418/2322 per MPI rank). The existing 49 native Newton cases
also pass all four layouts. The combined 88-case unit selection passes 105424
assertions; periodic Valgrind passes with zero errors (leaks unchecked). Three
existing CLI checks and 13 affected production forward-syntax units pass.
Forward syntax does not establish AD runtime. An initial ordinary-unit launch
used an empty cwd and aborted on fixture lookup; the identical command passes
from the established fixture directory. Both outcomes are retained.

The native DG affine regression and three support guards pass. Three registered
finite-iteration histories do not match the combined baseline: the 2D sector,
3D axis sector and weakly coupled periodic heat case. Their references and
tolerances have **not** been changed. Identical short archived/current controls
show that the 2D reference matches the archived executable and changes with the
complete operator; axis/heat references already differ on the pre-import base.
These are open reference/applicability checks, not converged comparisons.

A separate 2D sector endpoint check uses the same controls in the archived and
corrected native executables: velocity-based nondimensionalization, CFL100,
FGMRES50, tolerance1e-4 and all four flow RMS residuals below -10. Independent
restart reports must have raw and normalized RMS below 1e-9 and state agreement
within 1e-6. All four runs pass. Archived/current update counts are 1349/1049
serial and 1349/2229 on 2x2; maximum state discrepancy is 9.40e-8, maximum
normalized RMS is 1.207e-11. The corrected runs cost more wall time here;
this is correctness evidence, with no general performance claim. Full commands,
inputs, hashes, failures and reports are retained under the parent artifact
directory's `periodic_*` records; the committed summary and exact native
configuration are in `Docs/NewtonKrylovPeriodicBaselineResults.json`.

This is a baseline integration checkpoint. The common Newton periodic guards
remain in place until scaled matrix-free products, residual/state closure,
pseudo-time mappings, strong constraints and recovery are integrated and tested
against this corrected native operator. Larger production configurations,
remaining reference applicability and streamwise policy are still open.

## Current priority, 2026-10-16

The user has narrowed the current delivery to mean flow, turbulence and existing
transition models. Further species and flamelet development is deferred; neither
adapter is required by the shared controller or by flow/turbulence/transition
groups. Existing implementations and their evidence are retained. The remaining
work should first validate the relevant production configurations, audit their
baseline dependencies and complete periodic integration. NEMO is optional later
work: first integrate its current refactor and applicable convergence fixes from
the relevant branches, then establish a corrected native baseline. No NEMO
integration or claim of complete branch synchronization has been made.

The shared production controller, equation selection, segregated and coupled
groups, verified matrix-free products, Armijo acceptance, exact rollback and
bounded recovery are implemented. Matched converged comparisons cover mean
flow, SA, SA-BC, SST and active LM on serial and MPI/OpenMP layouts. The new
modes require steady primal single-zone CPU cases without multigrid. Core periodic
groups are covered by the current qualification above; earlier sections retain
their original evidence and restrictions. Larger production cases and general performance gains
remain unproved. Historical sections below describe earlier milestones and
their remaining work at that time.

The final physical-property admission check now rejects nonfinite or nonpositive
Cp where the flow stores it, and nonfinite or negative viscosity, conductivity
and scalar diffusivity after refreshing properties. It includes the implicit
last ordinary species and unselected scalar dependencies. Zero transport remains
valid. The two new native cases verify collective rejection and exact rollback
of primary values, old values, row masks, properties and residuals. Their
detecting run fails 18 of 3921 assertions before the production fix; the final
property cases pass 3945 assertions. Compressible Euler's unused stub Cp getter
is excluded from the guard.

All 49 native cases pass on 1x1, 1x2, 2x2 and 2x4 rank/thread layouts; all 80
ordinary cases and 22 focused Valgrind cases pass, with zero reported memory
errors (leaks unchecked). Three CLI checks, the affected forward-syntax unit
and all 42 existing matched comparisons pass. Of these comparisons, 30 cover
the five current priority families; the others retain existing species and
flamelet evidence. Iteration counts are unchanged. Complete current commands,
source and executable hashes and per-run evidence are recorded in the parent
artifact directory's `physical_properties_results.json`. Forward syntax is not
AD runtime validation. Flamelet's weak-source coupled MPI timeout remains
unresolved and is deferred with the other flamelet work.

The governing specification is `HANDOFF_NEWTONKRYLOV.md` in the parent workspace
(confirmed by the user on 2026-10-15). The work is on `feature_LineSearch`, starting
at `5615a8f5f6e9a7bfa88f0f279e1eadb7f89fe607`. The goal is still in progress.
Production steady primal CPU flow and selected flow/SA/SST/LM/species groups now
use the common controller. Equation selection, segregated versus coupled solves,
CFL ownership, final physical residuals and group failure output are wired into
the native driver. A compatibility class remains for legacy execution modes
outside this migration. Dense native pseudo-time mappings and explicit-flow
preconditioning are supported. Converged matched comparisons now cover generated
mean-flow, ordinary SA, SA-BC, SST, active LM and variable-density species
fixtures. Model bounds and feasible near-bound probes now have focused native
coverage. Flamelet source, coefficient, halo and output checks now have a
matched reacting-front comparison. The full goal remains in progress: the
conditional-feature and production-layout audits below are open.

## Flamelet physical coefficients, halos and source coordinates

Flamelet trial evaluations now derive thickening from the current progress,
temperature and velocity gradients. The native outer-iteration relaxation of
flame thickness is numerical history: residual, finite-difference and implicit
matrix refreshes preserve that history while recomputing the physical target.
This distinction retains the coefficient's cross derivatives and makes restart
residual reports independent of unrecorded numerical relaxation history. Native
output refreshes also preserve history. Scalar gradients are refreshed before
coefficients and sources, and thread extrema/gradients are combined before MPI
reductions. A uniform hot field has no resolvable flame front and uses the
default thickness, avoiding division by zero. The normalized-gradient coefficient
can still switch nonsmoothly as a front appears or ignition changes; a unique
smooth derivative is not claimed at those switches. The convergence fixture
uses the same nonuniform native flame-front initialization in every mode.

The thickening factor also uses the owning point's complete cell volume on
MPI halos. Local halo geometry contains partial cells; computing diffusion
coefficients from those partial volumes made halo coefficients half their owner
values in the regression. An identical serial restart then had species scaled
RMS 1.51e-6 on two ranks despite ~1e-14 on one rank, while the owned coefficients
and flow residuals agreed. A single scalar volume work vector now uses the
existing halo exchange before computing thickening. It is allocated only when
thickening is active and refreshed each preprocessing call, preserving geometry
and allowing volumes to change. Halo coefficients are checked against values
collected independently from the owning ranks.

The stored auxiliary consumption coefficient includes the same inverse
thickening factor as the source residual. Its native implicit diagonal divides
by live density because the native correction uses `rho*phi` coordinates.
Step refinement is also required for thickened-flamelet products with first-order
reconstruction. Near a uniform hot field, unrefined products had scalar/coupled
centered defects of 0.326/0.320. Existing Richardson refinement passes the same
2e-4 reference bound; centered-reference refinement is checked independently.
The full physical operator and true linear defect use verified products, while
approximate lower preconditioning can retain its cheaper coupling action.
Transported flamelet enthalpy uses `rho*Cp*T` for its reference scale, including
when signed enthalpy crosses zero. This scaling is distinct from physical
admissibility.

A synthetic affine LUT checks properties, sources and lookups independently.
Its temperature is `300+100*p+5*h`, with `Cp=0.2` so `Cp*dT/dh=1`.
An earlier draft supplied Cp=1000, inconsistent with this affine fixture and
its native inverse-enthalpy iteration; all twelve rows failed the fixture
identity check. Correcting Cp and conductivity together
preserves Prandtl number and thermal diffusivity. Runs with that inconsistent
draft remain archived and are excluded from the final comparison evidence.
Focused tests exercise ordinary native and Newton preprocessing, repeatability,
implicit assembly, selected FLOW/SPECIES/coupled ownership, live coefficients,
centered products, rollback, halo ownership and nonmaster gradient contributions.
The latest focused suite passes five cases and 5134 assertions serial, with a
matching MPI/OpenMP run. The independent product case covers domain lengths
0.05 and 0.005 and checks flow and species blocks separately against a refined
centered reference. Its first stretched draft had nearly tied gradient maxima
and failed its own reference refinement check; the apparent 16.9% error from
that invalid reference is excluded. The corrected profile separates the
extrema across y and passes the original 2e-4 product and 1e-5 reference bounds.

`compare_flamelet.py` uses an 81-point stretched rectangle and affine progress,
transported enthalpy and auxiliary-trace data. The reacting fixture's progress
source is `10*(1-p)+0.02*h`; it must produce at least 0.005 progress variation.
All methods start from the same native nonuniform flame-front initialization.
Every physical residual must reach log10 RMS -13, followed by an independent
restart check below raw RMS 1e-12 and normalized RMS 1e-9. State and quantity
agreement limits remain 1e-6. The independent report additionally requires no
table misses, positive diffusion, active thickening, and thickness consistent
with the history output. The output getter reports the thickness actually used;
a separate derived field keeps it distinct from native relaxation history.
A later native update is checked against the original history after all trials.

| Ranks x threads | Native updates | Segregated updates | Coupled updates |
| --- | ---: | ---: | ---: |
| 1x1 | 3025 | 16 | 15 |
| 2x2 | 3025 | 18 | 17 |

The progress range is approximately 0.41664 to 0.42502; maximum diffusivity is
0.02925865 for the base value 0.02. All six physical residuals pass, and the
largest state discrepancy from the conventional solution is 9.53e-10. The
largest cross-layout discrepancy is 1.64e-10. These are synthetic data and a
coarse generated mesh, not physical flame validation or a production speedup.

The original weak-source fixture, `0.1*(1-p)+0.02*h`, remains an unresolved
stress case. Its serial three-method comparison converges, but coupled MPI
hits the 180-second and then 600-second harness wall limits. At the latter
limit it has made 4038 updates; this is not an iteration cap or a successful
comparison. Recent retry records show reported Krylov convergence followed by
large true defects, which the controller correctly rejects. None reached the
linear iteration limit. The reacting fixture is a separate convergence case;
its success does not fix or relabel that weak-front failure. The failed source,
executables, data, configs and logs remain archived. Replaying the weak restart
uses its archived LUT, since the final fixture now has the stronger source.
The harness records failed timed-out modes explicitly; an intentional timeout
self-check verifies that their command, configuration hash and logs survive.

Final release checks pass all 47 native cases on 1x1 and 1x2 (59103 assertions)
and on 2x2 and 2x4 (36819/31029 per rank). The 80 ordinary cases pass 102273
assertions. Focused Valgrind passes 20 cases/13338 assertions with zero errors;
leaks are unchecked. Restart-report, species-boundary and residual-failure CLI
checks pass. Thirteen affected translation units pass forward syntax, including
CConfig and the active volume work vector; this does not establish AD runtime.

All 42 matched runs across seven families and both layouts pass on the final
executable. The preceding six families keep their model-bounds checkpoint's
iteration counts. Maximum normalized RMS is 1.188262e-10; maximum state/quantity
discrepancies from their conventional controls are 8.492298e-7/3.349589e-7.
Complete commands, per-system residual counts, outer/inner Krylov work, rank RSS,
wall times, physical states and hashes are in
`Docs/NewtonKrylovFlameletResults.json`.

| Ranks x threads | Method | Updates | Residual assemblies | Outer Krylov iterations | Seconds | Peak rank RSS, KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1x1 | native | 3025 | 6050 | 12100 | 1.572 | 25284 |
| 1x1 | segregated | 16 | 1128 | 330 | 0.776 | 25432 |
| 1x1 | coupled | 15 | 2072 | 210 | 0.869 | 25600 |
| 2x2 | native | 3025 | 6050 | 66663 | 9.247 | 25512 |
| 2x2 | segregated | 18 | 3347 | 896 | 2.973 | 25320 |
| 2x2 | coupled | 17 | 6440 | 732 | 4.480 | 25488 |

Native Krylov counts describe conventional implicit linear solves; Newton counts
describe outer matrix-free solves. Inner Krylov work is zero in these runs.
Residual counts exclude the independent restart reports.

Timings are single launch-inclusive samples on a tiny synthetic problem. They
do not establish a production speedup. Source, binaries, configs, tests and
failed intermediate evidence are preserved in the parent artifact directory's
`flamelet_63d439765714/` archive and `flamelet_manifest.json`.

LUT-domain admission, preferential diffusion, moving/CHT and periodic integration,
broader production layouts and AD runtime remain outside this milestone's
established support. The weak-front MPI timeout remains unresolved. NEMO remains
excluded; the full refactor goal is still active.

## Model bounds, exact strong values and feasible derivatives

The common equation interface now checks each scalar solver's
`GetSolutionBounds`, including dependent scalar systems outside the selected
group. This covers standard versus signed SA, configured SST floors, LM's
native intermittency upper bound of 5 and configured passive/species clipping.
Mixture fractions additionally require nonnegative transported and implicit
last fractions. Signed passive scalars and SA_NEG remain supported.

Existing physical guards run before property evaluation. Model clipping bounds
are checked after refreshing strong boundary data and their row metadata.
Prescribed rows can therefore retain exact native boundary values below a
numerical clipping floor, such as wall SA=0 or SST k=0. This does not exempt
them from physical admissibility. The ordinary native scalar update now tracks
these rows too and installs their prescribed `Solution_Old` exactly before
clipping free unknowns. This matters because the existing clipping operation
adds the correction to `Solution_Old`, not to the current solution. Merely
skipping the update would leave SA's previous wall value in place.

The conventional baseline changes accordingly. Sixteen matched two-update
native runs cover implicit and explicit SA, signed SA, SST and clipped mixture
species. Old SA walls were EPS, SST k walls were 1e-10 and prescribed species
zero became the configured 0.1 floor. Corrected runs retain zero exactly;
signed SA's already exact zero is a control. These runs demonstrate the
boundary correction, not convergence. The original and corrected executables,
configurations, restarts and hashes remain in
`model_native_wall_before_after/` under the parent artifact directory.

A single signed finite-difference step need not be feasible when different
unknowns touch opposite bounds, or at a mixture simplex vertex. The derivative
now constructs feasible endpoint directions `a,b` such that `a-b=u` on free
coordinates and samples
`(R(q+h*S*a)-R(q+h*S*b))/(h*S)`. Ordinary feasible forward probes still use
the captured base residual. For active box bounds, each component uses the
feasible side; mixture endpoints receive the same inward shift to leave room
for the implicit last fraction. Coordinates use the captured scalar density
factor, so varying mixture density does not change the requested difference.
The requested direction is never clipped or projected. If both endpoints
cannot fit, the existing bounded search reduces the step and can fail cleanly.
Physical checks still validate each complete endpoint. Richardson refinement,
Armijo and true linear-defect tolerances are unchanged. One extra vector is
allocated lazily only when a shifted pair is needed.

Trial snapshots now include prescribed-row metadata. Every trial restores it
along with current and old solutions, and the rollback error includes mask
differences. This prevents the final derivative probe's conditional boundary
mask from changing the native correction at the accepted base state.

Five focused native tests cover ordinary implicit/explicit clipping, selected
and unselected model bounds, box-bound products against an independent centered
linear-scalar reference, coupled variable-density mixture vertices, and row
metadata rollback. Detecting runs separately retain 34/244 model-bound
failures, 6/38 box-probe failures and 400/409 metadata-rollback failures.
The corrected focused suite passes all 1558 assertions. The initial native
test draft set only the current free value; that was an invalid clipping
control because clipping reads `Solution_Old`. Its failures are retained but
are not counted as independent detecting evidence.

The first mixture additivity fixture put a velocity jump at exactly zero
normal upwind velocity. Its flow additivity error was 6.54%, while its species
error was 6.53e-9. That switch has no unique smooth linear derivative. The
retained replacement uses nonzero crossflow, keeps the same 2e-4 test bound,
and obtains flow/species additivity errors 2.56e-8/1.73e-8. Direct checks of
the actual packed endpoint difference also verify the requested direction.
The failed fixture, source and executable remain archived separately.

All 42 native cases pass on 1x1 and 1x2 (51119 assertions), and on 2x2 and
2x4 (32455/26819 assertions per rank). The 80 supported ordinary cases pass
102273 assertions. Focused Valgrind passes 15 cases/8130 assertions with zero
memory errors; leaks are unchecked. Restart-report, species-boundary and
residual-failure CLI checks pass. Twelve affected production translation units,
including all native scalar instantiations and CSysMatrix, pass forward syntax;
this does not establish AD runtime support.

All 36 matched converged comparisons pass on this final executable on 1x1/2x2.
Outer counts remain MMS 67/69/69, SA 3327/2927/2927, SA-BC 3334/2933/2933,
SST 3488/3058/3119, species 81/82/82 and LM 1416/1414/5615, in
native/segregated/coupled order. Original physical and agreement thresholds are
unchanged. Maximum independent state-scaled RMS is 1.189e-10; maximum
within-family component/quantity difference is 8.493e-7/3.350e-7 (SA-BC).
Direct layout comparisons give maxima 3.172e-10/2.915e-11. The corrected native
baseline differs from the preceding native checkpoint by at most
3.135e-8/7.606e-10; exact old-native parity is not claimed. Full measurements,
criteria, work, per-rank resources and hashes are in
`Docs/NewtonKrylovModelBoundsResults.json`. Timings increased even for native
solves compared with the preceding session; these single samples do not isolate
code-change cost. Coupled LM still takes more updates and work than native or
segregated solving.

The preserved Re=1000 LM native configuration also closes its independent
residual gap. Identical configuration bytes and the same 2496 iterations now
give Re-theta RMS 3.678e-13 instead of 1.084e-10. Every raw component is below
1e-10 and every state-scaled component below 1.028e-11. Evaluating the old
restart with the current reporter gives exactly its old physical report, so
the change comes from the native prescribed-row correction, not the reporter.
This isolates the shared correction as a whole; it does not separate wall-k
clipping from other prescribed-value roundoff.

A 125-point 3D flow/SST/LM/eight-scalar stress case exercises 17 combined
variables and three genuine accepted updates on 1x1 under Valgrind and 2x2.
Maximum true linear defects are 4.024e-7 and 6.990e-5, respectively, with
finite restart fields and zero Valgrind errors. This is a buffer/execution
check, not convergence evidence. The attempted 28-variable configuration is
retained: after fixing its wall-option syntax, the existing native scalar
edge-flux guard rejects 20 scalars because `MaxScalarVar=8`. No capacity was
increased. CSysMatrix's separate 20-variable scratch limit applies per native
matrix; the common operator packs its combined group dynamically. The native
cap prevents a production group larger than 20 with these four equation types.

## Resolved groups during segregated convergence

A group can reach the requested physical tolerance while another group still
needs updates. Previously, only exactly zero merit permitted a stationary
iteration. Further tiny corrections eventually failed STEP_TOO_SMALL or
NON_DESCENT and exhausted recovery, even though that group's residual was
already resolved. This stopped the dimensional species pilot and active LM
pilot described below.

`CFluidIteration` now forwards the physical RMS target only when output has at
least one validated, registered RMS convergence field. Coefficient-only
monitoring and ignored invalid residual selectors do not enable this path.
Before preparing a linear solve, the common controller requires every native
component RMS in the group to meet the target. It also requires
`sqrt(2 * merit / global_points)` to meet that target, using the existing
state-scaled merit. This norm bounds every component's scaled RMS and prevents
small scalar units from hiding a significant residual. These checks are
stricter than checking the configured subset of raw history fields alone.

A resolved group keeps its state and reports an accepted zero step. Native
timesteps are still refreshed for dependent equations. Startup counters,
automatic reconstruction, CFL adaptation and forcing remain unchanged during
that stationary iteration. Fresh physical residuals are checked again after
dependent updates, so the group resumes solving when needed. Global stopping
still belongs to native output monitoring; this does not bypass its other
fields or CONV_STARTITER.

CSV rows identify stationary groups with status `RESOLVED`, decision `ACCEPTED`,
step zero, unchanged merit and zero Krylov/trial work. Their linear defect is
NaN because no linear solve was attempted. The existing exact-zero path uses
the same diagnostics. Nonzero accepted directions retain the unchanged Armijo,
derivative and true linear-defect requirements. The comparison runner reports
stationary iterations and nonzero updates separately for every group, so an
outer iteration is not mistaken for a nonlinear state update.

The detecting test drives the actual production hooks for FLOW and coupled
FLOW/SA. It balances native sources to leave a controlled small nonzero
residual, checks unchanged state and no solver work, then raises a non-density
residual and verifies resumed solving with the original startup window intact.
For coupled SA, that raised residual is below the raw target but above the
scaled target. Valid RMS, coefficient-only and ignored-invalid-field policies
are tested separately. The corrected final fixture fails 12/56 assertions on
a fresh original-controller build; the final test adds the unavailable-defect
assertion and passes 58/58 on 1x1 and both 2x2 ranks. The initial fixture omitted
the legacy startup enable flag; its startup expectation was invalid, and its
failed logs remain separate from the corrected detecting evidence.

Both production failures now pass without changing their physical thresholds.
The dimensional species comparison takes 127 native or 130 Newton outer
iterations. Its segregated species group is stationary for 71 iterations and
actually updates 59 times; flow updates 130 times. The previously failing LM
case now passes all three methods on 1x1 and 2x2 using
`compare_lm_flatplate.py`: compressible SST/LM, Re=500, common fixed CFL20,
turbulence/transition factor 0.5, native ILU and first-order Roe. All eight
history residuals must reach -11; independent raw RMS <=1e-10, state-scaled RMS
<=1e-9, pointwise-state/quantity agreement <=1e-6 and the original explicit
boundary-projection limits are unchanged.

```sh
python3 TestCases/newton_krylov/compare_lm_flatplate.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests \
  --output /path/to/new-lm-comparison --ranks 2 --threads 2
```

| Ranks x threads | Method | Outer iterations | Total residual assemblies | Outer Krylov iterations | Seconds | Peak rank RSS, KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1x1 | Native | 1416 | 4248 | 17096 | 0.969 | 25824 |
| 1x1 | Segregated Newton | 1414 | 60392 | 6434 | 8.445 | 25928 |
| 1x1 | Coupled Newton | 5615 | 207474 | 6205 | 21.339 | 26028 |
| 2x2 | Native | 1416 | 4248 | 32428 | 1.369 | 25748 |
| 2x2 | Segregated Newton | 1414 | 67232 | 10985 | 10.661 | 25524 |
| 2x2 | Coupled Newton | 5615 | 255093 | 11496 | 28.424 | 25676 |

In both layouts, segregated flow updates 610 times and waits 804; turbulence
updates 962 times and waits 452; transition updates all 1414 times. Coupled LM
uses 5615 nonzero updates. This is a measured convergence/work/time regression
for coupling, not a performance benefit. There are no inner Krylov iterations.
Work and per-rank memory use the same accounting as the earlier comparisons;
independent reporting is outside solve timing/work.

LM's 72 free points include 17 partially active and 55 high-intermittency points:
minimum gamma 0.17334744, maximum 1.00196241, mean 0.88491046. All methods retain
these band counts. Maximum differences in activity statistics are below 6e-12.
Maximum independent state-scaled RMS is 3.097e-11, component-state difference
2.468e-8 and relative physical-quantity difference 1.203e-10. Wall forces are
nonzero. This coarse low-Reynolds-number fixture validates nonlinear convergence
and activity, not a resolved transition front or physical accuracy.

All **36 final matched runs** on this same corrected executable pass:

| Fixture | Layouts | Native outer iterations | Segregated outer iterations | Coupled outer iterations |
| --- | --- | ---: | ---: | ---: |
| Manufactured flow | 1x1, 2x2 | 67 | 69 | 69 |
| Ordinary SA | 1x1, 2x2 | 3327 | 2927 | 2927 |
| SA-BC | 1x1, 2x2 | 3334 | 2933 | 2933 |
| SST | 1x1, 2x2 | 3488 | 3058 | 3119 |
| Variable-density species | 1x1, 2x2 | 81 | 82 | 82 |
| Active SST/LM | 1x1, 2x2 | 1416 | 1414 | 5615 |

Every physical residual, quantity, activity statistic, component-state error,
group update count, work counter, rank resource and provenance record is in
`Docs/NewtonKrylovResolvedGroupResults.json`. Direct 1x1/2x2 comparisons for each
method give maximum component-state difference 3.172e-10 and quantity difference
2.915e-11. Twelve comparisons against preceding native results retain the same
iteration counts, with state differences below 1.424e-14 and quantities below
1.150e-15. The old tables below retain their original executable bindings; they
are historical checkpoints, not relabeled results for this implementation.
Full restart fields are preserved in the referenced evidence directories.

All 37 native tests pass on 1x1/1x2 (49561 assertions) and 2x2/2x4
(31489/25909 per rank). All 80/102273 supported ordinary tests pass. Focused
Valgrind covers convergence, transition and boundaries (10/6572), zero memory
errors, leaks unchecked. Restart-report, species-boundary and residual-failure
CLI checks pass. Four affected production units pass forward syntax. Evidence
uses the `resolved_group_` prefix in the parent workspace. Full AD/reverse/
mixed/GPU execution and the model-bound/conditional audits remain unproved.

## Independent restart reports and converged manufactured flow

`TestCases/newton_krylov/report_state.py` loads a saved binary or ASCII restart in
a separate native test process and evaluates every available FLOW/TURB/TRANS/
SPECIES physical residual without taking a solver step. This avoids changing the
conventional solver's caches or comparing its pre-update history with Newton's
accepted-state history. The hidden `[NewtonStateReport]` native test reports native
RMS/maxima, RMS of `R/S` using `CNewtonEquationSet::BuildScales`, volume integrals
of packed unknowns, total mass and native force/heat quantities. It also reports
the largest state-scaled change from raw restart values to native strong boundary
values; this projection is explicit rather than silently included in residuals.
The script redirects history output, checks config/restart hashes before and after,
and saves the command, executable/input hashes, status and elapsed time.

Both shared SU2 restart readers previously copied the filename into a 100-byte
stack buffer. They now use the input string directly. A binary replay initially
detected the overflow; the regression check covers long binary/ASCII paths,
1x1 versus 2x2 reporting, unchanged solver output files and agreement with the
coupled accepted-state history. Native SST's earlier history differs from its
independent final physical report by up to 0.00781 in log10 RMS; the coupled
history agrees within 4.5e-10. Native strong-value projection is nonzero, while
the coupled restart needs no projection. Neither comparison claims convergence
for this four-update SST fixture.

The mixed flow/SST/LM/species replay detected two further native restart bugs.
Species skipped flow and turbulence columns but omitted the two transition
unknowns, loading LM gamma/Re_theta as species fractions. LM unconditionally
read two derived intermittencies immediately after its solution fields, although
the default restart contains no such fields. This crossed into species/the next
point and read beyond the final record. Species now also skips transition fields,
and LM derives its auxiliary intermittencies through native postprocessing.
Species restart postprocessing refreshes LM too, including coarse grids. Both
loaders reject missing solution fields before indexing them. The regression
covers all nine physical residuals in variable-density flow/SST/LM/species,
binary/ASCII parity, 1x1/2x2 parity and truncated-field rejection. The saved
pre-fix executable gives a Valgrind invalid read in `CTransLMSolver::LoadRestart`
and fails the physical replay; both formats pass with zero memory errors after
the fixes. A conventional MGLEVEL=1 mixed restart also takes one finite update
under Valgrind without memory errors, exercising coarse-grid postprocessing.
SA replay also matches its accepted-state history within 4.6e-10.

```sh
python3 TestCases/newton_krylov/check_state_report.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests --output /path/to/new-checks

python3 TestCases/newton_krylov/compare_manufactured_flow.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests \
  --output /path/to/new-comparison --ranks 2 --threads 2
```

The self-contained comparison uses an 81-point unit square, dimensional
Navier–Stokes MMS, first-order Roe, native CUSTOM Dirichlet values, ILU,
`MGLEVEL=0`, fixed CFL 10 and all four flow residuals in the stopping condition.
Every run uses the same corrected executable. All four native RMS values must
be below 1e-6, followed by an independent final restart check requiring raw RMS
below 1e-6, state-scaled RMS below 1e-9 and zero boundary projection. The
configured Newton step floor is 1e-14: the default 3.67e-11 floor stopped the
initial pilot at energy log10 RMS -5.61, before the requested -6 threshold.
The true linear-defect acceptance bound remains 0.1. A smaller step floor is
a case setting, not evidence that the default converges to arbitrary tolerances.

Results on 2026-10-16, including MPI launch in elapsed time:

| Ranks x threads | Method | Updates | Flow residual assemblies | Outer Krylov iterations | Seconds | Peak rank RSS, KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1x1 | Native | 67 | 67 | 536 | 0.439 | 24660 |
| 1x1 | Segregated Newton | 69 | 831 | 209 | 0.468 | 24604 |
| 1x1 | Coupled Newton | 69 | 831 | 209 | 0.467 | 24688 |
| 2x2 | Native | 67 | 67 | 993 | 0.467 | 25388 |
| 2x2 | Segregated Newton | 69 | 975 | 353 | 0.527 | 24276 |
| 2x2 | Coupled Newton | 69 | 975 | 353 | 0.569 | 24628 |

No inner Krylov iterations are used. Residual assembly counts exclude the
independent reporting process; native counts follow the single implicit
`MG_PRE_SMOOTH=1` pass, while Newton counters include the final all-active
physical refresh after the last diagnostic row. Each solver rank is measured
with `/usr/bin/time`; RSS is the maximum rank value, not the launcher's memory.
These are single timing samples on a tiny mesh and establish no speedup.

All six independent maximum state-scaled RMS values are below 1.19e-10.
Newton's maximum pointwise component difference from the native final solution,
scaled by that component's maximum magnitude, is below 1.90e-11. Total mass is
1.08219592821 in all runs. Native force/heat quantities are zero for these
unmonitored CUSTOM boundaries; the packed volume integrals and pointwise state
comparison supply the nontrivial final-state checks. Full raw/scaled residuals,
quantities, work, memory and hashes are committed in
`Docs/NewtonKrylovMMSResults.json`. FLOW alone has one block, so its segregated
and coupled flags describe the same equation group. The following SA comparison
has separate flow and turbulence blocks. SA-BC, SST and species comparisons
follow it; nontrivial LM still requires its own converged matched comparison.

Evidence in the parent workspace uses `state_report_` and
`manufactured_converged_final_{1x1,2x2}`. All 33 native cases pass on 1x1 and
2x2, and all 80 supported ordinary unit cases pass. Long-path binary and ASCII
reporting each produce zero Valgrind memory errors, with leaks unchecked.
All three changed production translation units pass forward syntax checks;
this does not establish full AD/reverse/mixed-precision/GPU execution.
The initial 265-character-path test instead exposes an overflow inside this
installation's OpenMPI `mca_sharedfp_lockedfile` backend; the retained stack
identifies the external library. The verified SU2 regression uses paths above
100 characters within that backend's capacity. The initially recorded 1x2
mixture/MUSCL startup failure is resolved by the preconditioning change below.

## Lower-block MUSCL preconditioning

The legacy startup-SMOOTHER mixture case failed at iteration 1 on one MPI rank
with two OpenMP threads. A trace found the first failed product inside lower
block preconditioning, with no pseudo-time term: the correction's physical norm
was 1.13e-49, its derivative norm 5.17e-50, and successive refined products still
differed by 8.48e-52 after sixteen halvings. The fixed-count smoother subsequently
requested corrections down to roughly 1e-141. Failure of this relative derivative
test in an approximate preconditioner propagated NaNs into the direction and
exhausted CFL recovery. Changing the augmented operator norm would not address
this physical-only preconditioner call.

Lower-block coupling now uses a single admissible forward probe. The full
matrix-free operator, directional slope and true linear-defect calculation keep
their existing configured Richardson verification and 5e-4 agreement threshold.
The true defect acceptance bound stays 0.1; the failing case is retained. This
changes preconditioner accuracy and work, and can change Krylov iteration counts.
Failed/nonfinite/inadmissible probes still fail through the existing bounded
path. No physical derivative is projected or accepted by a relaxed defect check.

The independent centered-reference fixture checks both probe paths for requested
steps 1e-4/1e-6 and physical/relaxed reconstruction. Verified products remain
within 2e-4 of the reference, approximate products within 1e-2. Calling the
approximate path between two verified products leaves the latter bitwise equal.
The existing failing policy case now completes all 1290 assertions on 1x2.
All 33 native cases pass on 1x1 and 1x2 (49404 assertions), 2x2 and 2x4
(31368/25796 assertions per rank). All 80 supported ordinary cases pass.
Four affected integration translation units pass forward syntax checks; full
AD/reverse/mixed-precision/GPU execution remains unverified.

Evidence uses `threaded_muscl_` in the parent workspace. Before-fix failures,
temporary diagnostic source/executables and traces are retained alongside the
final checks. The full 2x4 suite was slow but completed: live stack samples
showed it advancing through different solvers and tests, not a deadlock.
Focused `[NewtonFlowPolicy],[NewtonLinearization]` Valgrind coverage passes five
cases / 2750 assertions with two threads and `OMP_WAIT_POLICY=PASSIVE`, with zero
memory errors and leaks unchecked. The preceding default-wait run timed out at
180 seconds and supplies no completed memory result; its partial log is retained.
Generic production CLI controls also accept four updates of a reverse-packed
flow/SST/LM/species MUSCL group on 1x2 and 2x4, with maximum true defects below
2.7e-5, valid Armijo bounds and finite eleven-field restarts. These controls are
not convergence evidence. All six manufactured-flow comparisons pass again
with the shared comparison driver; the original numerical record above remains
bound to its original executable.

## Converged flow and SA comparison

`compare_sa_flatplate.py` uses the same comparison driver as the manufactured
case. It generates an 81-point compressible flat-plate problem at Re=1e5,
Mach 0.2, first-order Roe, native adiabatic no-slip/far-field boundaries, ILU,
MGLEVEL=0 and fixed CFL 20. It compares native updates, segregated FLOW/TURB
Newton and a coupled FLOW/TURB group with lower-block preconditioning. All
five native residuals must reach log10 RMS -10 before the 10000-update cap.
Every final state is independently replayed: raw RMS <=1e-10, state-scaled RMS
<=1e-9, component-scaled pointwise state difference <=1e-6 and relative
mass/drag/lift/moment/heat difference <=1e-6 from the native solution. Native SA
clips its prescribed wall zero to EPS; the reporter records the resulting
3.34e-12 scaled projection explicitly. Newton restarts need no projection.

```sh
python3 TestCases/newton_krylov/compare_sa_flatplate.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests \
  --output /path/to/new-sa-comparison --ranks 2 --threads 2
```

All six runs pass the same criteria on the same corrected executable:

| Ranks x threads | Method | Updates | Total physical residual assemblies | Total outer Krylov iterations | Seconds | Peak rank RSS, KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1x1 | Native | 3327 | 6654 | 36381 | 1.420 | 25252 |
| 1x1 | Segregated Newton | 2927 | 101869 | 20079 | 10.746 | 25076 |
| 1x1 | Coupled Newton | 2927 | 110128 | 14360 | 9.597 | 25368 |
| 2x2 | Native | 3327 | 6654 | 63031 | 2.227 | 25216 |
| 2x2 | Segregated Newton | 2927 | 111400 | 29366 | 13.225 | 25136 |
| 2x2 | Coupled Newton | 2927 | 146064 | 23344 | 13.936 | 25040 |

Residual counts sum flow and SA assemblies, including dependent flow assemblies
during scalar probes and the final physical refresh. Native counts follow one
implicit spatial pass per equation per update. Krylov counts sum both equations
or the coupled solve; no inner Krylov iterations are used. The separate physical
report is outside measured solve time/work. Memory is measured per solver rank.
Newton takes fewer nonlinear updates and more residual work, and is slower on
this coarse problem. Single launch-inclusive timing samples do not establish
production performance or parallel scaling.

Maximum independent state-scaled RMS is below 1.12e-10; the largest relative
component-state difference is below 8.47e-7. Relative drag/lift/moment differences
are below 3.34e-7, with nonzero wall forces; mass differs by less than 2.03e-8
relatively. Full raw/scaled residuals, integrals, quantities, state errors, work,
rank resources and hashes are committed in `Docs/NewtonKrylovSAResults.json`.
The mesh is a convergence fixture and establishes no discretization accuracy.

Evidence uses `threaded_muscl_converged_{mms,sa}_{1x1,2x2}`. Earlier SA pilots
remain under `auxiliary_convergence_pilot_sa*`: CFL2 reached the 10000-update cap
in all methods; CFL100 reached the cap in native mode while both Newton modes
met the preliminary -8 thresholds. The final common-CFL20 comparison tightens
the threshold to -10 and checks every final variable and physical quantity.
This checkpoint covers ordinary SA; the following checkpoint adds SA-BC and SST.

## Converged SA-BC and SST, including native wall/output fixes

The independent final-state comparison exposed two shared native defects. SA-BC
stores its production intermittency through `SetIntermittency`; its volume
export instead read the LM-only `GetIntermittencyEff`, returning zero. The shared
scalar output loader now reads the SA field. A detecting test seeds nonuniform
values and exercises the real loader for compressible and incompressible SA-BC.

SST's blending expression gave F1=0 at the exact k=0, wall-distance=0 boundary.
The conventional solver's small positive k clipping masked this, giving F1=1,
while Newton imposed the native zero-k wall value exactly. The shared blending
function now explicitly sets F1=1 at zero wall distance. This retains the inner
k-omega branch at the wall, consistent with the [SST boundary conditions and
blending definition](https://tmbwg.github.io/turbmodels/sst.html) and Menter's
description of F1=1 near the wall ([NASA TM103975, printed page 4](https://ntrs.nasa.gov/api/citations/19930013620/downloads/19930013620.pdf#page=8)).
Both methods use this corrected native function. No residual threshold was
relaxed. A saved conventional restart previously gave independent omega RMS
4.19997e-4 despite a history near 4e-11; the same restart now gives 4.39543e-11.
The wall test checks exact k=0 and F1=F2=1 in both flow formulations.

`[NewtonStateReport]` also reports `transition_activity` for SA-BC and LM:
minimum, maximum, arithmetic mean and counts below 0.1, in [0.1,0.9], and above
0.9 over owned non-solid-wall points. Halos and solid-wall values are excluded.
SA-BC uses its computed production intermittency; LM uses stored transported
gamma, not the derived effective intermittency. Nonfinite values fail collectively.
The matched transition runner requires low/high bands for SA-BC and partial/high
bands for LM, plus agreement of min/max/mean within 1e-6. Binary/ASCII and layout report checks include this
object; configurations without transition retain their previous report schema.

The converged SA-BC case has 72 sampled free points: 7 below 0.1, 65 above 0.9,
minimum 8.34124e-5, maximum 1 and mean 0.90267673. It exercises transition
suppression but has no resolved intermediate-band cell. A separate smooth
coupled derivative fixture sets native gamma=0.5 and checks the production
product against two centered step sizes per equation block. Relative errors are
7.4e-8 for flow and 2.32e-6 for SA, with the existing 2e-4 tolerance. The first
zero-crossflow fixture crossed the scalar upwind switch at zero normal flux and
gave a 3.015% centered/forward mismatch. Its source, executable and failed log
are preserved. Nonzero smooth crossflow removes that nondifferentiable tie;
the operator and derivative tolerances are unchanged.

Both convergence fixtures reuse the 81-point first-order compressible flat plate,
Re=1e5, Mach=0.2, cold starts, native ILU, MGLEVEL=0 and common fixed CFL20.
SA-BC uses `SA_OPTIONS=BCM`, with all five history residuals below -10. SST
requires all six below -11: native wall-k clipping can slightly shift the
independent physical residual when stopping at -10. Independent final raw RMS
must be <=1e-10 and state-scaled RMS <=1e-9 for every active variable in both
cases. Relative pointwise component and physical-quantity differences must be
<=1e-6. Explicit native boundary projection is <=1e-8 for SA-BC and <=1e-7 for
SST (observed SST k: 2.67e-8); Newton projection is zero. Every accepted Newton
row must meet Armijo, true linear defect <=0.1 and exact rollback.

```sh
python3 TestCases/newton_krylov/compare_sa_bc_flatplate.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests \
  --output /path/to/new-bc-comparison --ranks 2 --threads 2

python3 TestCases/newton_krylov/compare_sst_flatplate.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests \
  --output /path/to/new-sst-comparison --ranks 2 --threads 2
```

All twelve final runs pass:

| Model | Ranks x threads | Method | Updates | Total residual assemblies | Outer Krylov iterations | Seconds | Peak rank RSS, KiB |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| SA-BC | 1x1 | Native | 3334 | 6668 | 34802 | 1.621 | 25228 |
| SA-BC | 1x1 | Segregated Newton | 2933 | 102053 | 20106 | 11.217 | 25420 |
| SA-BC | 1x1 | Coupled Newton | 2933 | 110356 | 14390 | 10.048 | 25356 |
| SA-BC | 2x2 | Native | 3334 | 6668 | 60985 | 2.222 | 25676 |
| SA-BC | 2x2 | Segregated Newton | 2933 | 111614 | 29420 | 13.472 | 25296 |
| SA-BC | 2x2 | Coupled Newton | 2933 | 146368 | 23393 | 13.368 | 25172 |
| SST | 1x1 | Native | 3488 | 6976 | 38141 | 1.669 | 25372 |
| SST | 1x1 | Segregated Newton | 3119 | 108957 | 21621 | 14.863 | 25412 |
| SST | 1x1 | Coupled Newton | 3119 | 117612 | 15367 | 14.080 | 25224 |
| SST | 2x2 | Native | 3488 | 6976 | 66054 | 2.427 | 26064 |
| SST | 2x2 | Segregated Newton | 3119 | 124909 | 34458 | 18.078 | 25316 |
| SST | 2x2 | Coupled Newton | 3119 | 156432 | 25072 | 17.437 | 25704 |

The work/memory accounting matches the ordinary SA comparison above; there is
no inner Krylov work. Newton uses fewer updates, more residual work and more
time on these fixtures. Single samples on a coarse mesh establish no production
speedup, scaling or discretization accuracy. Maximum independent state-scaled
RMS is 1.123e-10 for SA-BC and 1.794e-11 for SST. Maximum component-state
differences are 8.493e-7 and 4.535e-8; maximum physical-quantity differences are
3.350e-7 and 1.689e-8. Wall forces are nonzero. Full residuals, quantities,
activity, state errors, work, resources and hashes are committed in
`Docs/NewtonKrylovTransitionResults.json`.

Evidence uses `transition_converged_{bc,sst}_{1x1,2x2}` in the parent workspace.
Both native fixes first fail their detecting tests on the preceding executable
(60 failed assertions), then pass (2 cases/84 assertions). All 36 native cases
pass on 1x1/1x2 (49503 assertions) and 2x2/2x4 (31431/25851 per rank).
All 80/102273 supported ordinary tests and restart-report checks pass. Focused
Valgrind covers transition and boundary cases (9/6514) with zero memory errors,
leaks unchecked. Both changed production translation units pass forward syntax.
Full AD/reverse/mixed/GPU execution, nontrivial LM/species convergence, model
admissibility/near-bound probes and the other conditional audits remain open.

## Converged variable-density species mixture

`compare_species_mixture.py` reuses the generated 81-point mesh and common
comparison driver for incompressible variable-density flow with the native
two-component `FLUID_MIXTURE`, energy and species transport. First-order FDS
flow and first-order scalar upwinding use native ILU, MGLEVEL=0, cold starts and
fixed CFL20. Inlet speed is 100, temperature 300 and transported fraction 0.25;
the adiabatic no-slip wall prescribes fraction 0.75. Molecular weights are
28.96/16.043 and heat capacities 1009.39/2225. Artificial viscosities 1/1 and
diffusivity 2 give a nontrivial diffusion fixture. `INC_NONDIM=INITIAL_VALUES`
keeps the enthalpy residual on a useful numerical scale. No physical validation
or discretization-accuracy claim follows from these coefficients or mesh.

All five native history residuals must be below -10. Independent final raw RMS
must be <=1e-10 and state-scaled RMS <=1e-9 for pressure, both velocities,
enthalpy and species. Full component states and physical quantities must agree
within 1e-6 relatively. Native boundary projection is explicitly limited to
1e-8 (observed below 6e-16); Newton projection is zero. The runner also checks
all 49 strictly interior points: each fraction lies in [0,1], their sum is <=1,
and each transported field spans at least 0.1. The measured span is
0.2499730574 to 0.5577746095, so boundary values alone cannot satisfy this check.

```sh
python3 TestCases/newton_krylov/compare_species_mixture.py \
  --binary /path/to/build/SU2_CFD/src/SU2_CFD \
  --reporter /path/to/build/UnitTests/newton_equation_tests \
  --output /path/to/new-species-comparison --ranks 2 --threads 2
```

| Ranks x threads | Method | Updates | Total residual assemblies | Outer Krylov iterations | Seconds | Peak rank RSS, KiB |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1x1 | Native | 81 | 162 | 1118 | 0.418 | 24792 |
| 1x1 | Segregated Newton | 82 | 3156 | 772 | 0.969 | 24744 |
| 1x1 | Coupled Newton | 82 | 3486 | 502 | 0.968 | 24612 |
| 2x2 | Native | 81 | 162 | 1977 | 0.417 | 25312 |
| 2x2 | Segregated Newton | 82 | 3686 | 1162 | 1.018 | 24996 |
| 2x2 | Coupled Newton | 82 | 4470 | 748 | 1.373 | 24772 |

All six runs pass on the preceding SST/SA-BC-corrected executable. Maximum
independent state-scaled RMS is 1.157e-11, component-state difference 2.872e-8
and physical-quantity difference 2.367e-9. Forces and mass are nonzero. Work and
memory accounting follow the comparisons above; there is no inner Krylov work.
Newton takes one more update and more work/time; single samples establish no
production performance or scaling. Full results and provenance are committed
in `Docs/NewtonKrylovSpeciesResults.json`; evidence uses
`species_converged_final_{1x1,2x2}` in the parent workspace. This checkpoint
changes only the Python benchmark/checks and documentation, so the preceding
36 native/80 ordinary tests, Valgrind and forward syntax remain bound to their
unchanged C++ source and binaries.

Preserved pilots identify a remaining controller limitation. The initial
dimensional species case uses speed 1, viscosity 0.01 and diffusivity 0.02:
native converges in 127 updates, but segregated species reaches roundoff before
enthalpy meets -10 and fails at iteration 119. This is a different case from the
final nondimensional benchmark, not a before/after solver fix.

The nontrivial LM pilot at Re=500, turbulence/transition CFL factor 0.5 and
all-eight history stop -11 converges conventionally in 1416 updates. Its 72 free
points include 17 with gamma in [0.1,0.9] and 55 above 0.9, minimum 0.17335;
every independent residual passes the same raw/scaled criteria. Segregated
Newton fails at iteration 1007: flow residuals are already below 4e-14, while
transition gamma is still about 1.6e-9. Flow recovery ends STEP_TOO_SMALL at
minimum CFL. This was not a matched converged LM result. The resolved-group
implementation and final converged comparisons at the top of this document now
address this failure; this paragraph records the preserved preceding
executable's behavior.

Earlier LM Re=1e5 reaches the 10000-update cap with clipped wall gamma and nearly
fully turbulent free nodes; Re=1e3 with turbulence factor 0.05 also reaches the
cap. Raising that factor to 0.5 reaches the -11 history stop, but independent
Re-theta RMS is 1.08351e-10, so it still fails the 1e-10 physical criterion.
Native wall-k clipping is a candidate contributor to this small replay shift;
its causal effect has not been isolated. The invalid `CFL_REDUCTION_TRANS`
pilot is a parser failure; the actual shared option is `CFL_REDUCTION_TURB`.
All failed configurations/logs/restarts remain under `lm_family_*` and
`species_family_*`. Neither stopping thresholds nor the controller were changed
to turn these failures into successes. Model admissibility/near-bound probes,
flamelet runtime and the other conditional audits remain open.

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
pressure perturbation can strongly change a tangential wall limiter. Operator
products with reconstruction compare successive one-sided Richardson products,
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
