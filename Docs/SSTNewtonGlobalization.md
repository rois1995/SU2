# Globalized SST Newton-Krylov

`SST_NEWTON= YES` enables coefficient-aware matrix-free Newton-Krylov integration of the segregated SST equations. It includes Armijo backtracking, adaptive pseudo-time, bounded recovery, adaptive linear forcing, diagnostics and a graceful failure restart. The default is `NO`, which retains the existing turbulence integration.

The current supported problem is steady, single-zone, compressible RANS with SST, implicit turbulence integration and no transition model, full-multigrid startup, CUDA or differentiation. MPI is supported. SST residual evaluations run outside an OpenMP team; the mean-flow solver retains its normal thread configuration. This implementation does not form a coupled mean-flow/SST Jacobian.

## Nonlinear problem and acceptance

Hold the stored mean-flow conservative state fixed during one SST update. Write the turbulence conservative unknowns as `q = rho * (k, omega)`, and the spatial SST residual as `R(q)`. The pseudo-time Newton system is

`(J_R + D) delta_q = -R(q_old)`,

where `D` is the nodal volume divided by the local pseudo-time step. `CNewtonEquationSet` now installs each trial from a saved base and refreshes flow primitives, gradients, SST blending functions, cross diffusion and eddy viscosity. Pressure responds to SST k at fixed total energy; the primitive state is refreshed during probes. Strong wall values are installed before the differentiated gradients and sources. Invalid flow probes are rejected. The two Newton algorithms remain separate; see `NewtonKrylovRefactor.md` for the common-interface milestone and its remaining requirements.

The default `SST_NEWTON_PSEUDO_TIME_MERIT= YES` globalizes the implicit pseudo-time equation

`G(q) = R(q) + D * (q - q_old)`.

This makes its Jacobian consistent with the matrix used to compute the direction. A stable nonnormal system can have transient growth in its physical residual while its implicit pseudo-time equation is solved accurately. The physical residual is therefore recorded separately and remains the basis of SU2's configured convergence tests.

`SST_NEWTON_PSEUDO_TIME_MERIT= NO` selects the steady residual `G(q) = R(q)`, reproducing the earlier strict steady-residual line-search experiment. In this mode, remove the pseudo-time diagonal from the matrix-free product used to compute the merit slope.

Use `0.5 * norm(G/S)^2` as the merit, with the conservative scale `S = rho * max(abs(phi_old), abs(phi_freestream))`. The scale stays fixed across all trials and recovery attempts for the update. A variable with both magnitudes exactly zero receives unit scale. MPI reductions count owned points only.

The existing SST under-relaxation and clipping define a feasible endpoint. Search along the segment from the base solution to that endpoint. Differentiate this actual direction instead of assuming the ideal Newton slope. For each trial, require strict merit decrease and

`merit(alpha) <= merit(0) + armijo * alpha * slope`.

Try a full step first. Rejected steps use safeguarded quadratic/cubic interpolation. Reject nonfinite evaluations and non-descent directions. The minimum step is the configured relative step tolerance divided by the largest relative endpoint displacement. An accepted trial is retained directly, and its step length is included in the under-relaxation reported to CFL adaptation.

## Recovery and model agreement

For a failed search or an unacceptable true linear defect:

1. Restore the base SST solution and under-relaxation, and recompute the residual and all model coefficients.
2. Check exact nodal restoration and the configured relative merit restoration tolerance. Invalid base states or inconsistent restoration terminate recovery.
3. If recovery is enabled and both its retry budget and CFL floor permit another attempt, contract SST CFL.
4. Recompute the time step, implicit residual/Jacobian, pseudo-time diagonal, preconditioner and Newton direction. Tighten the requested linear tolerance, bounded by `SST_NEWTON_FORCING_MIN`.
5. Repeat acceptance at the same base solution and with the same frozen scale and stored mean-flow conservative state.

Failure never silently skips an SST update, freezes a limiter or accepts the least bad rejected trial. Recovery is bounded, including when CFL reaches its minimum.

For accepted steps, compare actual reduction with the model prediction

`predicted = -alpha * slope - 0.5 * alpha^2 * norm(J_G * p)^2`.

Poor or invalid agreement, or a heavily backtracked step, contracts the next SST CFL. A full step with good agreement permits growth, within the configured bounds. A zero-residual update leaves CFL unchanged. With `SST_NEWTON_ADAPT_CFL= NO`, an accepted recovery step retains its reduced CFL.

FGMRES uses the configured initial target and an optional safeguarded Eisenstat-Walker choice-2 formula on the merit reduction of an undamped accepted subproblem. Damping resets the target; recovery tightens it. This is a forcing policy for changing segregated pseudo-time subproblems, not a claim of the fixed-system Newton convergence theorem. A fresh matrix-free product verifies the true relative linear defect before acceptance; it must not exceed `SST_NEWTON_FORCING_MAX`, independently of the Krylov solver's internal estimate.

## Configuration

All options are listed in `config_template.cfg`. Defaults:

| Option | Default | Purpose |
| --- | --- | --- |
| `SST_NEWTON` | `NO` | Enable the integration |
| `SST_NEWTON_LINE_SEARCH` | `YES` | Enforce nonlinear acceptance |
| `SST_NEWTON_PSEUDO_TIME_MERIT` | `YES` | Include the implicit pseudo-time term |
| `SST_NEWTON_RECOVERY` | `YES` | Retry failed updates at reduced CFL |
| `SST_NEWTON_ADAPT_CFL` | `YES` | Adapt CFL after accepted steps |
| `SST_NEWTON_ADAPTIVE_FORCING` | `YES` | Adapt the requested Krylov tolerance |
| `SST_NEWTON_DIAGNOSTICS` | `YES` | Write attempt and trial CSV files |
| `SST_NEWTON_ARMIJO` | `1e-4` | Sufficient decrease constant |
| `SST_NEWTON_STEP_TOL` | `epsilon^(2/3)` | Relative step termination |
| `SST_NEWTON_RESTORE_TOL` | `1e-10` | Relative merit restoration tolerance |
| `SST_NEWTON_INTERPOLATION_MIN/MAX` | `0.1 / 0.5` | Backtracking safeguards |
| `SST_NEWTON_BACKTRACKS` | `20` | Trial evaluations per direction |
| `SST_NEWTON_RETRIES` | `16` | Additional directions after the initial attempt; covers the default CFL range |
| `SST_NEWTON_CFL_MIN/MAX` | `0.001 / 1e6` | Bounds on SST CFL |
| `SST_NEWTON_CFL_SHRINK/GROW` | `0.25 / 2` | Pseudo-time contraction/growth |
| `SST_NEWTON_MODEL_POOR/GOOD` | `0.25 / 0.75` | Model agreement thresholds |
| `SST_NEWTON_FD_STEP` | `1e-8` | Global normalized finite-difference step |
| `SST_NEWTON_FD_RELATIVE_STEP` | `1e-4` | Cap relative to local variable magnitude |
| `SST_NEWTON_LINEAR_ITER` | `80` | FGMRES subspace limit, at most 1000 |
| `SST_NEWTON_LINEAR_TOL` | `1e-3` | Initial/reset Krylov target |
| `SST_NEWTON_FORCING_MIN/MAX` | `1e-5 / 0.1` | Forcing bounds and maximum true linear defect |
| `SST_NEWTON_FORCING_GAMMA/POWER` | `0.9 / 1.5` | Forcing update parameters |

Initial SST CFL is `CFL_NUMBER * CFL_REDUCTION_TURB`, clipped to the configured SST CFL bounds. The controller owns SST CFL thereafter; the existing flow CFL controller remains active if requested. Every attempt derives local SST time steps from the current mean-flow time steps and CFL values.

For an unglobalized Newton control, explicitly disable line search, recovery and accepted-step CFL adaptation. The steady-merit option remains useful for comparisons; it is not required to use recovery.
Disabling line search retains the finite-state and true-linear-defect checks. An inaccurate direction can still terminate the solve when recovery is disabled.

## Diagnostics and terminal failure

`sst_newton.csv` records every attempted direction: current/next CFL, line-search status, controller decision, Krylov iterations and requested/reported/true linear residuals, base/accepted merits, step, evaluations, slope, predicted reduction, model agreement, restoration errors and physical merit. `sst_newton_trials.csv` records each base, trial and rollback evaluation with both merits. These files append within a working directory; use a new directory for independent runs.

The four `*_residual_evaluations` columns count physical residual evaluations by the shared interface, cumulatively for flow, turbulence, transition and species. They exclude conventional outer-iteration work. Even an SST-only probe evaluates flow to refresh auxiliary boundary data, so this cost must be counted in future comparisons.

When recovery cannot continue, the integration returns a numerical-failure status. The iteration and driver veto convergence, force normal final output, and write an additional binary restart named `<RESTART_FILENAME>_sst_newton_failure.dat`, including when ordinary output is disabled. The executable finalizes normally, prints `Numerical Failure`, and returns a nonzero exit code. It does not call `MPI_Abort` for an exhausted numerical recovery. Invalid configuration is still reported through SU2's normal configuration-error path.

The failure restart contains the restored SST base state and the mean-flow state held during that attempted SST update. It is an unconverged segregated state. Restarting initializes the controller again from configuration; diagnostic CSV records preserve the failed CFL, forcing and attempt history.

Convergence fields remain user-controlled. A density-only test can stop while SST residuals remain large. Include `RMS_TKE` and `RMS_DISSIPATION` in `CONV_FIELD` when requiring turbulence residual convergence as well, and inspect the physical merit independently of the pseudo-time merit.

The reconstruction fallback can be discontinuous. Finite-difference products, descent estimates and smooth convergence assumptions can therefore fail even with this controller. Recovery provides a controlled attempt to change the direction; it does not guarantee SST convergence.

## Recorded validation

The following results are historical prototype evidence, before the corrected baseline and shared residual interface. They do not validate the current source. Current checks and artifacts are listed in `NewtonKrylovRefactor.md`.

The prototype was validated with GCC 9.4, a release MPI/OpenMP build, and the epsilon branch at `732528ac56` as its baseline. Normal comparisons use two MPI ranks, one OpenMP thread, flow CFL 1000 and initial SST CFL 100. The limiter continues to update and turbulence MUSCL remains enabled.

| NACA0012 case | Accepted SST updates | Recovered updates | Outcome |
| --- | ---: | ---: | --- |
| Fresh start, budget 1000 | 1000 | 422 | Completes the budget; 499 rejected directions retried; not converged |
| Stalled restart, density criterion | 24 | 1 | Reaches density target; SST residuals remain large |
| Stalled restart, early convergence disabled | 66 | 1 | Stops at the CFL floor on an unacceptable true linear defect |
| Backtracking budget 1, recovery budget 0 | 3 | 0 | Controlled retry-limit exit, exact rollback and failure restart |
| Backtracking budget 1, minimum CFL 100 | 3 | 0 | Controlled CFL-floor exit, exact rollback and failure restart |

The fresh-start final log10 RMS residuals are `rho=-9.885714493`, `k=-9.244042521`, `omega=-2.766771374`. The density-only stalled run stops at `rho=-11.01027468`, but `omega=-4.301141225`; this is not full turbulence convergence. Continuing that run reaches a true linear defect of about `2.81` at the CFL floor, above the configured maximum `0.1`, and returns numerical failure.

Every accepted globalized update satisfies Armijo. Rejected directions have exactly zero measured nodal and merit restoration errors. All recovery attempts retain the same base merit, decrease CFL and recompute the direction. Disabling diagnostics produces exactly the same failure-checkpoint values. The checkpoint loads successfully into the legacy solver for five further iterations.

Additional checks: legacy SA and SST histories match every field of their previous 20-iteration controls exactly; four-rank and two-thread smoke cases complete and exercise recovery; the documented fresh-start failure case exits as expected. The feature unit tests pass 98 assertions, including parameter parsing. All 60 supported visible unit cases pass 100273 assertions; the MLP fluid-model case is excluded because MLPCPP is disabled. The seven pure globalization unit cases also pass with `-O3 -ffast-math`, and all nine isolated NEMO convergence-migration checks pass.

These results validate controller behavior and expose the remaining SST/linearization limitation. They do not establish a converged SST solution or convergence acceleration over develop. Full integration regressions and differentiation/GPU builds have not been run.

## References

- [PETSc Newton backtracking](https://petsc.org/release/manualpages/SNES/SNESLINESEARCHBT/): Armijo acceptance and polynomial interpolation.
- [PETSc implicit pseudo-time stepping implementation](https://petsc.org/release/src/ts/impls/pseudo/posindep.c.html): implicit pseudo-time equations, rejected-step recovery and limits.
- [PETSc trust-region parameters](https://petsc.org/release/manualpages/SNES/SNESNEWTONTR/): model agreement and safeguarded contraction/growth. The present controller changes pseudo-time; it does not implement a dogleg trust-region solve.
- [KINSOL mathematical considerations](https://sundials.readthedocs.io/en/latest/kinsol/Mathematics_link.html): scaling, inexact Newton and Eisenstat-Walker forcing.
- [Kelley and Keyes, Convergence Analysis of Pseudo-Transient Continuation](https://epubs.siam.org/doi/10.1137/S0036142996304796): pseudo-transient continuation for steady PDE systems.
