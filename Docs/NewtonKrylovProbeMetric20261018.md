# Momentum probe sizing in internal flows

The SST nozzle application initializes its freestream at Mach 1e-9 but its supplied
restart contains velocities of hundreds of metres per second. The common finite-difference
operator used the freestream momentum reference to size perturbations near zero momentum.
This could hide physical derivative errors behind strongly unequal residual scales.

The focused detector uses a moving state with Mach 1e-9 and 1e-12 freestream references.
Two independently centered physical differences agree within 1e-4. On the original operator,
the verified product nevertheless differs from that reference by 0.0121121 and 1.47559.
The corrected operator passes the existing 5e-4 derivative requirement in physical rows too.

The correction uses local density times the larger of velocity magnitude and sound speed
as the momentum **probe** reference, including the absolute-step norm. It reuses the output
vector as scratch. State/residual scaling, pseudo-time coordinates, prescribed rows,
derivative refinement limits, true-defect checks, acceptance, and rollback remain unchanged.
Legacy flow controls retain their original probe metric. Incompressible and auxiliary-only
probe sizing is unchanged.

## Checks

Immutable binaries, source hashes/patch, commands and raw logs are under
`newton_krylov_refactor_20261015/application_qualification_20261018/` in the workspace.

- `probe_detector_reference_fixed/`: original operator fails both physical derivative checks;
  the independent centered references pass. Earlier detector-development attempts remain preserved.
- `probe_metric/manifest.json`: corrected candidate source/binary hashes and `source.patch`.
- `probe_metric_check_1x1/` and `probe_metric_checks/`: focused detector passes on
  1x1, 1x4, 2x2, 2x4; all 61 equation tests pass on 1x1 and 2x2; supported 91 unit tests pass.
  Species boundary, residual-failure and turbomachinery guard checks pass on both layouts.
  Equation tests include periodic products, live cross responses, accepted/rejected steps,
  resolved groups, retries and exact rollback. No timing assertions are used.

## Application limits

These matched original-control prefixes use the same supplied restart. Counts below are
completed-controller telemetry, excluding the driver's initial residual evaluation in both
versions. They are work measurements, not time-to-convergence comparisons.

| Application / mode | Version | Updates | Flow assemblies | Turb assemblies | Krylov iterations | CFL retries |
|---|---|---:|---:|---:|---:|---:|
| NACA0012 SA / segregated | Original | 11 | 442 | 428 | 423 | 4 |
| NACA0012 SA / segregated | Corrected probe | 11 | 443 | 428 | 429 | 4 |
| SST nozzle / segregated | Original | 20 | 533 | 380 | 280 | 9 |
| SST nozzle / segregated | Corrected probe | 20 | 895 | 368 | 373 | 8 |

NACA SA native and segregated endpoints pass the frozen all-equation/state/quantity checks.
Coupled SA still misses pointwise agreement, as before. Both nozzle prefixes fail the physical
convergence requirements. The corrected nozzle spends more work before reducing CFL: it
resolves some products that previously failed immediately, but does not cure the difficult
linear solves. This correction is **not an application performance improvement**.

The original nozzle native run stops after 548 updates on its density history criterion;
fresh transverse momentum normalized RMS is 7.65e-9 against the frozen 1e-9 target, so it
is not a qualified reference. All these times are contention-ineligible. Continued application
qualification and separately labelled CFL/forcing/preconditioning studies remain required.
See [the maintained ledger](NewtonKrylovApplications20261018.md).

Reproduce the focused detector with the pinned binary from its existing fixture directory:

```sh
OMP_NUM_THREADS=2 OMP_WAIT_POLICY=PASSIVE mpirun --bind-to none -n 2 \
  /absolute/path/to/probe_metric/newton_equation_tests '[NewtonProbeMetric]'
```

Use the recorded MPI temporary-directory environment and working directory from
`probe_metric_checks/commands.json`. Full application commands and configuration hashes
are retained in each trial's `result.json` and the maintained CSV.
