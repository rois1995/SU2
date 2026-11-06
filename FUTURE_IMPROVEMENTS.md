# Improving the reliability of the SA/SST stress-field approach

The central idea is worth keeping: apply a saved stress correction, let SA or
SST respond, and measure what changes. The infrastructure is now ready for small,
controlled research within its supported scope. Its answers are ranges of tested
scenarios, not trustworthy error bars without further evidence.

## What is implemented

- Exact donor capture and replay at actual flux and source queries, with saved
  state, mesh, units and parent-setting checks. Separate residual tables allow
  comparison without advancing the solution. Tiny 2D and 3D QCR/EPM checks pass.
- A bounded local runner checks baseline recovery, nonzero fields, disk restarts,
  changed-input rejection and actual scalar/vectorized assembly. It saves hashes,
  timings and failures. The final suite passes 37 checks and 20 comparisons.
- Owned-volume summaries show stress work, production limits, floor activity and
  physically impossible SST covariance. Query counts, failures, storage and time
  are reported. These summaries do not claim an exact discrete energy balance.
- The offline generator accepts localized fields, opposite signs and combinations.
  The saved physical field remains fixed during a solve.
- Study tools freeze parameter and reference files, separate calibration from
  independent validation, and report missing cases, misses, widths and costs.
  Their tests use synthetic data; no physical study has been performed.

The [verification report](Docs/CAMPAIGN_REPORT.md) records evidence and limits.
The [runbook](Docs/STRESS_DISCREPANCY.md) gives commands. Production runs and
cluster access remain excluded. The separate adjoint estimator is outside scope.

## What would improve credibility next

### Choose field strength using independent evidence

Amplitude, smoothing length and wall damping largely determine the answer.
Choose a narrow family of flows, estimate a few parameters using its calibration
cases, then freeze them before examining separate validation cases. Split whole
related cases together, rather than scattering neighboring samples across sets.
Measure both misses and range width: containing everything with an enormous range
is not useful. Keep inflow, measurement and numerical uncertainty separate.

Calibration across several flows has precedent, but success within one family
does not establish general reliability. See
[Edeling, Cinnella and Dwight](https://sam.ensam.eu/bitstream/handle/10985/10035/Dynfluid-JCP-Cinella-2014.pdf?isAllowed=y&sequence=1).

### Make additional field flexibility earn its complexity

Localized fields and combinations are available. First test whether they capture
reference discrepancies. If they do not, consider different smoothing lengths
along and across shear layers, and different strengths in attached flow,
separation and wakes. Derive those choices from geometry or a saved baseline,
then freeze them. No flow-dependent correction is silently updated during a solve.

Test the restrictions too. Wall damping may exclude an important stress error;
zero correction at an outlet may make results depend on outlet location. General
curved-wall support needs implementation and verification before use.

### Reject misleading physical interpretations

For SST, examine the reconstructed total covariance, not positivity of the
correction alone. The new summary flags negative directional fluctuation energy.
It does not clip the field and thereby change the experiment. SA has no transported
turbulence energy for the same test; describe its field as a momentum-stress
correction unless extra information is justified.

Track the gap between stress work and limited SST production. Physical stress
constraints alone do not ensure plausible dynamics; see
[Matha and Morsbach](https://arxiv.org/abs/2311.01355).
The deliberately strong tiny SST fixture already shows substantial violations.
Its successful execution is a wiring check, not physical acceptance.

### Separate insertion, representation and convergence errors

Exact same-state replay now verifies insertion. Next fit the donor into the
mesh-independent field, measure the additional representation error, and then
check whether a solve reaches the donor solution. A nodal fit alone cannot
establish exact face/source recovery. Recovering QCR or EPM validates machinery,
not predictions against reality.

Retain failed cases. A nearly uniform 1C EPM scalar/vectorized comparison differed
by 2.17e-10, above the fixed 1e-10 threshold. A nonuniform 1C case and a uniform
3C case pass. Nearly repeated eigenvalues remain a qualification limit; the
threshold was not relaxed and a general fix is not claimed.

### Establish numerical robustness before enlarging studies

Compare more than one starting state and gradual introduction of the correction,
always ending at the same declared field. Measure mesh error, incomplete
convergence and field interpolation error separately. Improve the incomplete
implicit derivative only if these tests show it is needed. Stress prescriptions
can be sensitive to small errors in some formulations; see
[Wu and colleagues](https://arxiv.org/abs/1803.05581).

Local MPI checks remain blocked by this environment's socket restrictions.
Parallel execution and repartitioned restarts need verification on an authorized
local setup. Exact replay deliberately supports one rank and one thread only.

## How I would proceed in practice

1. Choose a small initial flow family and a few outputs, collect reference
   intervals and record uncertainty in the measurements and boundary conditions.
2. Use the study tools to separate calibration and validation cases. Choose field
   parameters using calibration cases, then freeze the parameter file and case list.
3. On an authorized local setup, finish parallel checks and donor representation
   studies. Keep the current nearly uniform 1C EPM limitation visible.
4. Run a separately authorized validation study with explicit convergence and
   mesh-error criteria. Save every requested scenario, including failures.
5. Add more field flexibility or improve the implicit derivative only when the
   evidence identifies a shortcoming. Revalidate after changing the model family.

The agreed infrastructure work is implemented. Remaining physical calibration,
converged donor recovery and mesh studies are research work. Broader geometry,
parallel replay and a complete derivative would be additional implementation work
if those capabilities become necessary. None is silently claimed ready here.
