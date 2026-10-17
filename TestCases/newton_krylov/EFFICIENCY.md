# Newton efficiency measurements, 17 October 2026

The implementation reduces residual work and duplicated temporary memory while
preserving the original-control nonlinear trajectory. The starting source is
`29a4c3c58add4b0164714a91ef330fa3a01654dc`; the qualified implementation is
`4712d82375e00ede15e5177ab38865eaaaa3e9f5`. Every validated commit was pushed
immediately to `rois1995/SU2:feature_LineSearch`. The earlier physical-property
checkpoint in [PERFORMANCE.md](PERFORMANCE.md) precedes this baseline; its counts
are not silently substituted for the current before measurements.

Original-control comparisons and configuration experiments are separate. Three
alternating before/after pairs cover native, segregated and coupled SA/SST/active
LM and the rotational-periodic axis fixture on 1x1 and 2x2: 144 solver runs. Every
pair has identical configuration, work counts and complete restart bytes.
Available CPU time was affected by persistent desktop activity and intermittent
foreign builds. Observed medians are reported below, but **there is no qualified
uncontended or general speedup claim**. Work reductions, memory measurements and
correctness checks do not depend on interpreting those noisy elapsed times.
End-to-end time covers the CFD invocation, MPI startup, initialization and output;
solver-loop time is its native `WRT_PERFORMANCE` timer. Queue waits and the later
independent physical reporter are outside both CFD measurement intervals.

## Implemented changes

| Commit | Change | Safety boundary |
|---|---|---|
| `37c0956d7e` | Separate flow boundary closure from full volume residual assembly for auxiliary probes; reuse settled closure coefficients within an evaluation. | Original weak/strong/periodic boundary order and live dependency refresh retained. Bounded edge mass fluxes, custom sources and external feedback retain full assembly. MUSCL gradients and frozen history stay live. |
| `eb09c08050` | Share evaluation/history scratch between sequential segregated groups. | Accepted states, row masks, density coordinates, relaxation, reconstruction history and controller/Krylov state stay private. Independent sets keep independent scratch; incompatible layouts reject safely. |
| `3bf731f9d3` | Evaluate only the row consumed by a lower-block preconditioner coupling product. | All input columns and live dependencies remain present. Outer operator, true-defect and slope products retain full residuals and original verification. |
| `4ab295b0d7` | Use native property-only preprocessing after turbulent transport updates instead of repeating unchanged flow gradients. | Direct native comparisons check every primitive property, unchanged gradients and exact rollback; wall functions and external feedback retain full preprocessing. |
| `4712d82375` | Reuse settled flow gradients in the final preparation of supported auxiliary boundary probes. | Eligibility is collective across ranks, combined with an existing reduction. Reconstruction, species, wall functions, hybrid modes, sensors and unsupported boundaries fall back to full preprocessing. |

Reuse is local to one settled residual evaluation; there is no general cache
across state changes, retries, CFL/timestep or configuration changes. Native
boundary integration retains its order. Prescribed rows, halos, pseudo-time
coordinates, live cross derivatives, restart behavior, exact rollback and failure
reporting remain checked. No physical target, true-defect limit, line-search
acceptance or derivative-verification setting was loosened. No new dependencies
or unrelated equation-system features were added.

## Memory

On 263169 points, three alternating before/after pairs per model and mode
(54 runs) retain identical one-update work and restart bytes. Shared scratch
reduces segregated peak RSS as follows:

| Model | Before, MiB | Shared scratch, MiB | Saved, MiB |
|---|---:|---:|---:|
| SA | 1105.5 | 1064.7 | 40.8 |
| SST | 1248.6 | 1204.9 | 43.7 |
| Active LM | 1626.1 | 1511.7 | 114.4 |

These CFL 0.05 one-update runs measure allocation, not convergence or runtime
scaling. Native/coupled controls have no consistent saving (changes from 0.26 MiB
lower to 1.64 MiB higher). Both per-process peak RSS and RSS at the same collective
restart output are retained in `memory_complete_513/`; their medians agree here.
The observer was validated with empty MPI ranks and is absent from production.
Available RAM stayed at least 38.3 GiB and swap use was constant in this batch.
Elapsed times are excluded because external CPU use reached 7.1 cores.

## Correctness qualification

The final implementation passes all 60 native equation tests on 1x1, 1x4, 2x2
and 2x4, plus the 91 supported ordinary unit tests, periodic/gradient-cache/output
checks, independent restart oracle and species/residual-failure CLI checks on
1x1 and 2x2. Valgrind reports zero errors for the affected refresh, workspace,
property and bound-probe tests. Tests compare full/auxiliary and requested-row
products, positive/negative/repeated/zero perturbations, SA/SST/active LM,
incompressible and variable-density flow, species and existing flamelet coverage,
translated/rotated periodic boundaries, row masks, bounds, frozen reconstruction
history and exact rollback. A boundary changed only on its owning rank exercises
collective fallback. The five serial converged families retain every work count
and restart byte across implementation batches; final original-control SA/SST/LM
and axis comparisons repeat on both layouts. Additional BC and periodic SA/SST/LM
endpoints pass both layouts. Configuration trials separately exercise backtracking,
failed solves, retries and resolved blocks without relaxing acceptance checks.

The uninstrumented rebuild matches all three qualified binary hashes exactly;
temporary profiling edits are restored. The failed rank-local first prototype
(`MPI_ERR_TRUNCATE`) is preserved in `boundary_prep_correctness/` and
`boundary_prep_failed_candidate.json`; it was never committed. The collective
implementation passes the rank-local boundary-change regression. Cancelled
contention preflights, harness-only postprocessing failures and all capped/failed
configuration trials remain in the evidence directory.

## Repeated original-control comparison

All implementation changes are committed through `4712d82375`. The uninstrumented
release rebuild is byte-identical to the qualified candidate, and its build
options match starting commit `29a4c3c58a`. Three alternating before/after pairs
cover SA/SST/active LM and the existing rotational-periodic axis fixture on 1x1
and 2x2: **144 solver runs**. Every one of the 72 mode pairs retains its complete
restart bytes, outer/Krylov/native-inner work and configuration hash. Independent
uncached physical residuals, prescribed values, state/quantities, active transition,
true defects and rollback checks all pass.

The following counts distinguish full residual assemblies from boundary-only
work. Native counts in release runs are outer-iteration inference, independently
confirmed against phase scopes; Newton counts are measured telemetry. Full flow
assembly includes its boundaries. An auxiliary boundary-only evaluation is
additional work and is not counted as a full assembly.

| Case | Layout | Mode | Outer / Krylov, unchanged | Full flow before → after | Turbulence before → after | Transition before → after |
|---|---|---|---:|---:|---:|---:|
| SA | 1x1 | native | 3327 / 36381 | 3327 → 3327 | 3327 → 3327 | — |
| SA | 1x1 | segregated | 2927 / 19749 | 56977 → 34886 | 25020 → 25020 | — |
| SA | 1x1 | coupled | 2927 / 14360 | 49210 → 34850 | 49210 → 49210 | — |
| SA | 2x2 | native | 3327 / 63031 | 3327 → 3327 | 3327 → 3327 | — |
| SA | 2x2 | segregated | 2927 / 29036 | 66264 → 43929 | 25264 → 25264 | — |
| SA | 2x2 | coupled | 2927 / 23344 | 67178 → 43834 | 67178 → 67178 | — |
| SST | 1x1 | native | 3488 / 38141 | 3488 → 3488 | 3488 → 3488 | — |
| SST | 1x1 | segregated | 3058 / 21184 | 60930 → 36466 | 27524 → 27524 | — |
| SST | 1x1 | coupled | 3119 / 15367 | 52568 → 37201 | 52568 → 52568 | — |
| SST | 2x2 | native | 3488 / 66054 | 3488 → 3488 | 3488 → 3488 | — |
| SST | 2x2 | segregated | 3058 / 33771 | 73517 → 45999 | 30578 → 30578 | — |
| SST | 2x2 | coupled | 3119 / 25072 | 71978 → 46906 | 71978 → 71978 | — |
| LM | 1x1 | native | 1416 / 17127 | 1416 → 1416 | 1416 → 1416 | 1416 → 1416 |
| LM | 1x1 | segregated | 1414 / 6434 | 27023 → 8920 | 9573 → 9573 | 11364 → 11364 |
| LM | 1x1 | coupled | 5615 / 6205 | 57927 → 45517 | 57927 → 51722 | 57927 → 51722 |
| LM | 2x2 | native | 1416 / 32414 | 1416 → 1416 | 1416 → 1416 | 1416 → 1416 |
| LM | 2x2 | segregated | 1414 / 10985 | 31574 → 11182 | 10470 → 10470 | 12756 → 12756 |
| LM | 2x2 | coupled | 5615 / 11496 | 73800 → 50808 | 73800 → 62304 | 73800 → 62304 |
| Axis | 1x1 | native | 1319 / 100353 | 1319 → 1319 | — | — |
| Axis | 1x1 | segregated | 14 / 375 | 474 → 474 | — | — |
| Axis | 1x1 | coupled | 14 / 375 | 474 → 474 | — | — |
| Axis | 2x2 | native | 1319 / 105519 | 1319 → 1319 | — | — |
| Axis | 2x2 | segregated | 14 / 574 | 673 → 673 | — | — |
| Axis | 2x2 | coupled | 14 / 574 | 673 → 673 | — | — |

These are **observed contended medians in seconds**, not an uncontended speedup
result. Native control times also varied; complete ranges and rank process CPU
are retained in `original_medians.json`. Only one owned numerical job ran at a
time. When an external build appeared, the owned coordinator was stopped while
the current guarded triplet finished, then resumed after three clear observations.
No foreign process was changed. All affected times remain excluded from speedup
claims. Available RAM stayed well above the 8 GiB guard; swap growth never tripped it.

| Case | Layout | Mode | Loop median before → after | End-to-end median before → after | Final normalized RMS, max | State / quantity difference, max |
|---|---|---|---:|---:|---:|---:|
| SA | 1x1 | native | 1.232 → 1.176 | 1.724 → 1.623 | 2.20e-11 | 0.00e+00 / 0.00e+00 |
| SA | 1x1 | segregated | 8.674 → 6.320 | 9.059 → 6.765 | 1.12e-10 | 8.46e-07 / 3.34e-07 |
| SA | 1x1 | coupled | 8.424 → 6.635 | 8.828 → 7.042 | 1.12e-10 | 8.46e-07 / 3.34e-07 |
| SA | 2x2 | native | 7.861 → 7.706 | 8.264 → 8.159 | 2.20e-11 | 0.00e+00 / 0.00e+00 |
| SA | 2x2 | segregated | 61.143 → 47.518 | 61.504 → 47.891 | 1.12e-10 | 8.46e-07 / 3.34e-07 |
| SA | 2x2 | coupled | 66.761 → 54.153 | 67.135 → 54.623 | 1.12e-10 | 8.46e-07 / 3.34e-07 |
| SST | 1x1 | native | 1.318 → 1.243 | 1.723 → 1.776 | 2.04e-12 | 0.00e+00 / 0.00e+00 |
| SST | 1x1 | segregated | 11.147 → 8.309 | 11.522 → 8.702 | 9.86e-12 | 7.10e-08 / 2.71e-08 |
| SST | 1x1 | coupled | 10.826 → 9.192 | 11.313 → 9.605 | 7.13e-12 | 4.64e-08 / 1.76e-08 |
| SST | 2x2 | native | 8.829 → 8.397 | 9.247 → 8.893 | 2.04e-12 | 0.00e+00 / 0.00e+00 |
| SST | 2x2 | segregated | 81.643 → 63.030 | 82.047 → 63.443 | 9.86e-12 | 7.10e-08 / 2.71e-08 |
| SST | 2x2 | coupled | 92.472 → 70.206 | 92.843 → 70.561 | 7.13e-12 | 4.64e-08 / 1.76e-08 |
| LM | 1x1 | native | 0.648 → 0.634 | 1.070 → 1.073 | 1.01e-11 | 0.00e+00 / 0.00e+00 |
| LM | 1x1 | segregated | 5.920 → 4.350 | 6.289 → 4.735 | 1.03e-11 | 4.57e-10 / 4.83e-11 |
| LM | 1x1 | coupled | 17.176 → 14.016 | 17.529 → 14.427 | 1.04e-11 | 4.46e-10 / 1.61e-14 |
| LM | 2x2 | native | 6.151 → 3.886 | 6.549 → 4.385 | 1.01e-11 | 0.00e+00 / 0.00e+00 |
| LM | 2x2 | segregated | 53.867 → 31.110 | 54.266 → 31.467 | 1.03e-11 | 4.57e-10 / 4.83e-11 |
| LM | 2x2 | coupled | 153.761 → 111.461 | 154.146 → 111.823 | 1.04e-11 | 4.42e-10 / 1.53e-15 |
| Axis | 1x1 | native | 14.819 → 14.936 | 15.228 → 15.336 | 3.08e-13 | 0.00e+00 / 0.00e+00 |
| Axis | 1x1 | segregated | 0.350 → 0.251 | 0.769 → 0.683 | 5.95e-14 | 1.44e-09 / 2.70e-08 |
| Axis | 1x1 | coupled | 0.357 → 0.246 | 0.819 → 0.619 | 5.95e-14 | 1.44e-09 / 2.70e-08 |
| Axis | 2x2 | native | 27.492 → 28.843 | 27.919 → 29.234 | 3.08e-13 | 0.00e+00 / 0.00e+00 |
| Axis | 2x2 | segregated | 1.158 → 0.743 | 1.571 → 1.120 | 5.83e-14 | 1.68e-09 / 3.74e-08 |
| Axis | 2x2 | coupled | 0.994 → 0.663 | 1.520 → 1.071 | 5.83e-14 | 1.68e-09 / 3.74e-08 |

These retain the fixtures' original controls: flat plates start with fixed CFL 20;
the axis fixture retains native fixed CFL 10 and Newton adaptive CFL starting 10.
The subsequent control study gives native its own higher-CFL and adaptive trials.
Native has lower observed elapsed times on the fixed-CFL coarse turbulent fixtures. Newton retains
14 updates compared with 1,319 for native on the axis fixture; this is not a claim about
fully tuned native performance or other meshes.

Measured serial segregated flow-gradient calls fall from 284,887 to 91,864 for SA,
353,126 to 121,634 for SST and 144,857 to 40,814 for LM. The starting-profile numerical
sources match `29a4c3c58a`; archived SST/LM2x2 profiles are absent and are left null,
with the measured intermediate `4ab295b0d7` comparison retained separately.
Boundary-only flow counts remain 22,091/24,464/18,103 in those three serial cases.
For SA, 31,127 matrix-free probes comprise 19,749 Krylov products plus 5,689 true-defect
and 5,689 slope products. Explicit bases, return-to-base refreshes, accepted trials
and final outer refreshes account for every remaining wrapper call. Refinement
counts are zero in these first-order fixtures; reconstructed/flamelet regression
checks still retain verification. Native residual/Jacobian kernels are interleaved,
so their joint scope is labelled rather than presented as a Jacobian-only timer.

## Measured remaining work

The following final SA serial profile gives **calls / inclusive seconds**.
The observer and desktop contention affect these seconds; they are useful for
attribution, not release timing comparisons. Scopes nest: never sum the rows.
MPI/OpenMP/allocation hooks observe the master thread and overlap the scopes.
They are not an exhaustive byte count of allocations on worker/runtime threads.
Counts are logical team calls
per rank, including native residual/Jacobian kernels at the same boundaries.

| Scope | Native | Segregated | Coupled |
|---|---:|---:|---:|
| Solver loop | 1 / 1.234 | 1 / 6.908 | 1 / 6.886 |
| Explicit residual wrapper | 0 / 0.000 | 51121 / 4.162 | 46282 / 4.324 |
| Implicit preparation wrapper | 0 / 0.000 | 5856 / 0.605 | 2928 / 0.401 |
| Boundary closure | 0 / 0.000 | 56978 / 0.592 | 49211 / 0.506 |
| Dependency refresh | 0 / 0.000 | 56978 / 0.955 | 49211 / 0.812 |
| Full flow preprocessing | 6654 / 0.074 | 91864 / 0.883 | 84061 / 0.792 |
| Flow property-only preparation | 0 / 0.000 | 79069 / 0.344 | 63571 / 0.274 |
| Flow gradients | 6654 / 0.028 | 91864 / 0.314 | 84061 / 0.289 |
| Primitive properties | 6654 / 0.026 | 170933 / 0.603 | 147632 / 0.524 |
| Flow residual, no Jacobian | 0 / 0.000 | 31957 / 0.852 | 31922 / 0.850 |
| Flow residual + Jacobian | 3327 / 0.143 | 2929 / 0.140 | 2928 / 0.144 |
| Turbulence residual, no Jacobian | 0 / 0.000 | 22091 / 0.344 | 46282 / 0.711 |
| Turbulence residual + Jacobian | 3327 / 0.068 | 2929 / 0.077 | 2928 / 0.062 |
| Native preconditioner construction | 6654 / 0.109 | 5689 / 0.118 | 5854 / 0.115 |
| Native preconditioner application | 36381 / 0.175 | 19749 / 0.123 | 28720 / 0.161 |
| Block preconditioner application | 0 / 0.000 | 19749 / 0.167 | 14360 / 1.647 |
| Full matrix-free product | 0 / 0.000 | 31127 / 2.953 | 20214 / 2.381 |
| Derivative refinement | 0 / 0.000 | 0 / 0.000 | 0 / 0.000 |
| State capture | 0 / 0.000 | 5854 / 0.024 | 2927 / 0.010 |
| State restoration | 0 / 0.000 | 48194 / 0.108 | 46282 / 0.095 |
| Newton diagnostics | 0 / 0.000 | 17232 / 0.318 | 8781 / 0.242 |
| History I/O | 3327 / 0.054 | 2927 / 0.056 | 2927 / 0.056 |
| Result I/O | 3328 / 0.001 | 2928 / 0.002 | 2928 / 0.002 |
| MPI_Allreduce hook | 342072 / 0.016 | 1280771 / 0.069 | 1142081 / 0.062 |
| MPI_Waitall hook | 0 / 0.000 | 0 / 0.000 | 0 / 0.000 |
| GOMP_barrier hook | 825743 / 0.118 | 9152576 / 1.300 | 8638647 / 1.214 |
| malloc hook | 226996 / 0.017 | 812757 / 0.054 | 728155 / 0.051 |
| free hook | 395726 / 0.020 | 960093 / 0.053 | 870054 / 0.047 |
| aligned_allocation hook | 155564 / 0.020 | 135845 / 0.031 | 130368 / 0.036 |

The complete `final_phase_summary.json` retains SA/SST/LM on both layouts,
per-rank inclusive/exclusive scopes, original/intermediate counters, communication
and allocation hooks. The archived SST/LM2x2 starting profile is absent and stays
null; its intermediate comparison is retained. `profile_counter_batch/` additionally
covers axis and all periodic model fixtures. Boundary-only work is explicitly
separate from full flow assemblies. Native residual and Jacobian assembly share
interleaved kernels, so a separate Jacobian-only time would be misleading.

I/O counts describe calls to the instrumented diagnostic/history/result scopes,
including calls that decide no output is due. They are not physical disk-write
or filesystem-syscall counts. Final release inner-Krylov counters are all zero;
native preconditioner applications remain separately measured above.

SA segregated still executes 56,977 wrappers: 31,127 finite-difference probes
(19,749 Krylov + 5,689 independent defects + 5,689 slopes), 5,689 each of
explicit base, return-to-base and accepted-trial refreshes, 2,927 outer refreshes,
5,854 advance preparations and two constraint setups. Only 5,689 of 5,854
attempts need a nonzero step; 165 are resolved. All nonzero accepted steps use
alpha one and no CFL retry occurs. Thus backtracking does not explain this
original-control slowdown. These are measured counters, cross-checked against
`sa_final_call_attribution.json`; first-order refinement counts are zero.

Residual wrappers include live properties, closure, gradients, halos and repeated
reductions. Removing full flow flux assembly therefore does not remove their
entire cost. SA retains 22,091 auxiliary boundary-only evaluations and roughly
1.28 million all-reduces. The last gradient optimization adds no collective:
SA all-reduce counts remain identical to the preceding implementation.

The coupled lower preconditioner retains extra physical coupling products:
its inclusive scope is about 24% of this instrumented SA loop. Native
preconditioner construction is about 2% of the final Newton loop, so freezing
it is not an established large win. Snapshot copying remains a smaller serial
cost; persistent accepted states stay private to preserve exact rollback.
Millions of OpenMP synchronization boundaries remain expensive on tiny
partitions. No general thread-serialization or new Jacobian approximation is
introduced without a qualified numerical and scaling comparison.

## Higher CFL and existing controls

These configuration trials follow the implementation measurements. All physical, state,
quantity, derivative, true-defect and acceptance targets remain unchanged. Counts below
are outer updates, not a runtime ranking. `qualification_failed` includes iteration
caps and independently rejected endpoints; the CSV retains the specific reason.

| Case | Layout | Fixed CFL | Native | Segregated | Coupled |
|---|---|---:|---:|---:|---:|
| sa | 1x1 | 100 | qualification_failed (10000) | 588 | 587 |
| sa | 1x1 | 1000 | qualification_failed (10000) | 71 | 896 |
| sa | 2x2 | 100 | qualification_failed (10000) | 588 | 587 |
| sa | 2x2 | 1000 | qualification_failed (10000) | 69 | 71 |
| sst | 1x1 | 100 | qualification_failed (10000) | 610 | 622 |
| sst | 1x1 | 1000 | qualification_failed (10000) | 74 | 248 |
| sst | 2x2 | 100 | qualification_failed (10000) | 610 | 622 |
| sst | 2x2 | 1000 | qualification_failed (10000) | 242 | 74 |
| lm | 1x1 | 100 | 296 | 294 | 4495 |
| lm | 1x1 | 1000 | 56 | 41 | 7184 |
| lm | 2x2 | 100 | 296 | 294 | 4495 |
| lm | 2x2 | 1000 | 56 | 41 | 7184 |
| bc | 1x1 | 100 | qualification_failed (10000) | 589 | 589 |
| bc | 1x1 | 1000 | qualification_failed (10000) | 71 | 938 |
| bc | 2x2 | 100 | qualification_failed (10000) | 589 | 2347 |
| bc | 2x2 | 1000 | qualification_failed (10000) | 70 | 938 |
| axis | 1x1 | 100 | 288 | 120 | 120 |
| axis | 1x1 | 1000 | 192 | 19 | 19 |
| axis | 2x2 | 100 | 301 | 120 | 120 |
| axis | 2x2 | 1000 | 266 | 19 | 19 |
| periodic_sa | 1x1 | 100 | qualification_failed (10000) | 969 | 969 |
| periodic_sa | 1x1 | 1000 | qualification_failed (10000) | 110 | 110 |
| periodic_sa | 2x2 | 100 | qualification_failed (10000) | 969 | 969 |
| periodic_sa | 2x2 | 1000 | qualification_failed (10000) | 110 | 110 |
| periodic_sst | 1x1 | 100 | qualification_failed (10000) | 858 | 875 |
| periodic_sst | 1x1 | 1000 | qualification_failed (10000) | 99 | 100 |
| periodic_sst | 2x2 | 100 | qualification_failed (10000) | 858 | 875 |
| periodic_sst | 2x2 | 1000 | qualification_failed (10000) | 342 | 346 |
| periodic_lm | 1x1 | 100 | 405 | 403 | 6240 |
| periodic_lm | 1x1 | 1000 | 58 | 54 | 6654 |
| periodic_lm | 2x2 | 100 | 405 | 403 | qualification_failed (10000) |
| periodic_lm | 2x2 | 1000 | 58 | 54 | 6653 |

The following adaptive runs start at CFL 20 (axis: 10). Native schedules are
matched-wide `(0.25,2,0.001,1e6,0.001,0)`, default `(0.1,1.2,10,100,0.001,0)`
and slow-wide `(0.25,1.2,0.001,1e6,0.001,0)`. Their native linear target stays
`1e-6`. Newton CFL-only keeps its original linear controls; EW enables the
existing adaptive forcing. Diagonal uses the same adaptive CFL/EW controls
as lower-block coupled EW, changing only the preconditioner option.
Newton group telemetry records the CFL used and the proposed next CFL.
Native history reports local statistics at its write point after adaptation;
those values can describe the next update rather than the step just taken.

| Case | Layout | Native wide / default / slow | SG CFL / EW | CPL CFL / EW / diagonal |
|---|---|---|---|---|
| sa | 1x1 | qualification_failed (10000) / qualification_failed (10000) / qualification_failed (10000) | 21 / 21 | 23 / 23 / 23 |
| sa | 2x2 | qualification_failed (10000) / qualification_failed (10000) / qualification_failed (10000) | 21 / 21 | 23 / 24 / 23 |
| sst | 1x1 | qualification_failed (10000) / qualification_failed (10000) / qualification_failed (10000) | 28 / 25 | 25 / 25 / 25 |
| sst | 2x2 | qualification_failed (10000) / qualification_failed (10000) / qualification_failed (10000) | 28 / 28 | 28 / 28 / 28 |
| lm | 1x1 | 1418 / 1417 / 1417 | 14 / 14 | 16 / 16 / 16 |
| lm | 2x2 | 1418 / 1417 / 1417 | 14 / 15 | 16 / 16 / 16 |
| bc | 1x1 | qualification_failed (10000) / qualification_failed (10000) / qualification_failed (10000) | 21 / 21 | 26 / 25 / 26 |
| bc | 2x2 | qualification_failed (10000) / qualification_failed (10000) / qualification_failed (10000) | 21 / 21 | 29 / 26 / 29 |
| axis | 1x1 | qualification_failed (556) / 698 / 574 | 14 / 14 | 14 / 14 / — |
| axis | 2x2 | qualification_failed (1064) / qualification_failed (811) / qualification_failed (883) | 14 / 15 | 14 / 15 / — |
| periodic_sa | 1x1 | qualification_failed (10000) / 2281 / qualification_failed (10000) | 17 / 17 | 17 / 17 / 17 |
| periodic_sa | 2x2 | qualification_failed (10000) / 6575 / qualification_failed (10000) | 17 / 17 | 17 / 17 / 17 |
| periodic_sst | 1x1 | qualification_failed (10000) / 2455 / 2283 | 18 / 18 | 17 / 17 / 17 |
| periodic_sst | 2x2 | qualification_failed (10000) / 5409 / 3585 | 18 / 18 | 17 / 17 / 17 |
| periodic_lm | 1x1 | 1961 / 1963 / 1963 | 15 / 14 | 50 / 44 / 46 |
| periodic_lm | 2x2 | 1963 / 1962 / 1962 | 15 / 14 | 52 / 44 / 19 |

The additional native-only sweep checks CFL 30 and 50 because aggressive
fixed/adaptive native settings failed on several turbulent fixtures. These
retain the original linear and physical targets and use a 180 s practical
wall cap; capped trials are not claims of convergence failure at unlimited cost.

Native LM additionally receives fixed CFL 10000/100000. Native adaptation
updates flow/turbulence/species local CFL but omits transition in
`CSolver::AdaptCFLNumber`; `CScalarSolver::SetTime_Step` uses each scalar's
own local CFL. This source-derived limitation is retained, not changed for
the benchmark. No claim that the matched-wide native schedule is optimal follows.

| Case | Layout | Native fixed 30 | Native fixed 50 | Native fixed 10000 / 100000 |
|---|---|---:|---:|---|
| sa | 1x1 | 2292 | 1461 | — |
| sa | 2x2 | 2292 | 1461 | — |
| sst | 1x1 | 2454 | qualification_failed (10000) | — |
| sst | 2x2 | 2454 | qualification_failed (10000) | — |
| lm | 1x1 | 949 | 576 | 51 / 50 |
| lm | 2x2 | 949 | 576 | 51 / 50 |
| bc | 1x1 | 2295 | 1462 | — |
| bc | 2x2 | 2295 | 1462 | — |
| axis | 1x1 | 565 | 408 | — |
| axis | 2x2 | 566 | 412 | — |
| periodic_sa | 1x1 | 3616 | 2259 | — |
| periodic_sa | 2x2 | 3616 | 2259 | — |
| periodic_sst | 1x1 | 3302 | 2273 | — |
| periodic_sst | 2x2 | 3302 | 2272 | — |
| periodic_lm | 1x1 | 1313 | 794 | 49 / 49 |
| periodic_lm | 2x2 | 1313 | 794 | 49 / 49 |

## Refined fixtures

These generated refinements retain the original state/quantity requirements.
The initial inherited threshold let native CFL20/30 satisfy residual checks
while differing by 2.35e-6 in state, exceeding the unchanged 1e-6 agreement
limit. That batch was stopped after its current case, with all evidence
preserved in `larger_control_study/`. The final comparison in
`larger_strict_control_study/` tightens every raw and normalized residual
threshold by 100x, uniformly for all modes; state/quantity/derivative/linear
and acceptance requirements are unchanged. These stricter results are a
separate accuracy configuration, not an implementation timing change.
Their labelled update budget is 50000 for every mode, allowing refinement
more iterations without changing the physical stopping or agreement limits.
Fixed runs have a 120 s wall cap and adaptive runs a 300 s cap. A timeout
is not a converged performance result. All Newton endpoints compare with
the same independently qualified native reference within each cohort.

| Case | Layout | Mode | Controls / initial CFL | Outcome |
|---|---|---|---|---:|
| sa_33 | 1x1 | native | fixed / 20 | 5032 |
| sa_33 | 1x1 | native | fixed / 30 | 3574 |
| sa_33 | 1x1 | native | fixed / 50 | timeout (—) |
| sa_33 | 1x1 | native | native_default / 20 | timeout (—) |
| sa_33 | 1x1 | native | fixed / 1000 | timeout (—) |
| sa_33 | 1x1 | segregated | fixed / 20 | 4481 |
| sa_33 | 1x1 | coupled | fixed / 20 | 4481 |
| sa_33 | 1x1 | segregated | adaptive_cfl / 20 | 24 |
| sa_33 | 1x1 | coupled | adaptive_cfl / 20 | 36 |
| sa_33 | 1x1 | segregated | adaptive_forcing / 20 | 24 |
| sa_33 | 1x1 | coupled | adaptive_forcing / 20 | 42 |
| sa_33 | 1x1 | coupled | adaptive_diagonal / 20 | 43 |
| sst_33 | 1x1 | native | fixed / 20 | timeout (—) |
| sst_33 | 1x1 | native | fixed / 30 | timeout (—) |
| sst_33 | 1x1 | native | fixed / 50 | timeout (—) |
| sst_33 | 1x1 | native | native_default / 20 | timeout (—) |
| sst_33 | 1x1 | native | fixed / 1000 | timeout (—) |
| sst_33 | 1x1 | — | — | No qualified native reference within unchanged targets and practical caps; Newton comparisons not launched. |
| lm_33 | 1x1 | native | fixed / 20 | qualification_failed (989) |
| lm_33 | 1x1 | native | fixed / 30 | qualification_failed (745) |
| lm_33 | 1x1 | native | fixed / 50 | qualification_failed (551) |
| lm_33 | 1x1 | native | native_default / 20 | 654 |
| lm_33 | 1x1 | native | fixed / 1000 | qualification_failed (282) |
| lm_33 | 1x1 | segregated | fixed / 20 | 725 |
| lm_33 | 1x1 | coupled | fixed / 20 | timeout (—) |
| lm_33 | 1x1 | segregated | adaptive_cfl / 20 | 18 |
| lm_33 | 1x1 | coupled | adaptive_cfl / 20 | 44 |
| lm_33 | 1x1 | segregated | adaptive_forcing / 20 | 17 |
| lm_33 | 1x1 | coupled | adaptive_forcing / 20 | 47 |
| lm_33 | 1x1 | coupled | adaptive_diagonal / 20 | 47 |
| sa_65 | 1x1 | native | fixed / 20 | timeout (—) |
| sa_65 | 1x1 | native | fixed / 30 | timeout (—) |
| sa_65 | 1x1 | native | fixed / 50 | timeout (—) |
| sa_65 | 1x1 | native | native_default / 20 | timeout (—) |
| sa_65 | 1x1 | native | fixed / 1000 | timeout (—) |
| sa_65 | 1x1 | — | — | No qualified native reference within unchanged targets and practical caps; Newton comparisons not launched. |
| sa_65 | 2x2 | native | fixed / 20 | timeout (—) |
| sa_65 | 2x2 | native | fixed / 30 | timeout (—) |
| sa_65 | 2x2 | native | fixed / 50 | timeout (—) |
| sa_65 | 2x2 | native | native_default / 20 | timeout (—) |
| sa_65 | 2x2 | native | fixed / 1000 | timeout (—) |
| sa_65 | 2x2 | — | — | No qualified native reference within unchanged targets and practical caps; Newton comparisons not launched. |

## Native history stopping margin

Some native histories stop before the fresh endpoint residual meets its
physical target. The following near-miss rechecks lower only the history
stop by one decade. Every independent physical, normalized, state and
quantity requirement stays identical to the source trial. This is a
separate, stricter stopping configuration, not an implementation change;
original rejected results remain recorded in the tables and CSV.

| Case | Layout | Native controls / initial CFL | Rechecked outcome |
|---|---|---|---:|
| axis | 1x1 | adaptive_cfl / 10 | 610 |
| axis | 2x2 | adaptive_cfl / 10 | 1154 |
| axis | 2x2 | native_default / 10 | 902 |
| axis | 2x2 | native_slow / 10 | 978 |
| lm_33 | 1x1 | fixed / 20 | 1111 |
| lm_33 | 1x1 | fixed / 30 | 839 |
| lm_33 | 1x1 | fixed / 50 | 623 |
| lm_33 | 1x1 | fixed / 1000 | 324 |

## Conservative refined references

A separate follow-up tries native fixed CFL5 on the three cohorts without
a reference, using the same strict physical/state limits and the native
history margin above. All modes receive a600s cap. Newton comparisons are
launched only when that native reference qualifies; this extends the budget
and changes controls, without relaxing accuracy.

| Case | Layout | Mode | Controls / initial CFL | Outcome |
|---|---|---|---|---:|
| sst_33 | 1x1 | native | fixed / 5 | qualification_failed (50000) |
| sst_33 | 1x1 | — | — | Native CFL5 did not qualify within600s; no comparison or accuracy relaxation. |
| sa_65 | 1x1 | native | fixed / 5 | timeout (—) |
| sa_65 | 1x1 | — | — | Native CFL5 did not qualify within600s; no comparison or accuracy relaxation. |
| sa_65 | 2x2 | native | fixed / 5 | timeout (—) |
| sa_65 | 2x2 | — | — | Native CFL5 did not qualify within600s; no comparison or accuracy relaxation. |

Completed control trials: 222/222; additional native trials: 40/40; refined trials: 39; native history rechecks: 8.
All observed elapsed times remain excluded from speedup claims because of
CPU contention and single samples. Per-system assembly counts, Krylov/native
inner work, observed elapsed times, actual CFL, final agreement, reasons,
configuration hashes and evidence paths are retained in `control_outcomes.csv`.

## Interpretation and current limits

The implementation savings are measured with the original controls first: work
counts and complete restart bytes are identical before/after, while unnecessary
full flow assemblies and repeated gradient calculations decrease. The separate
control study changes the nonlinear trajectory. On strict SA33, native CFL30
qualifies in 3,574 updates with 7,148 full assemblies; segregated adaptive forcing
qualifies in 24 updates with 555 flow and 255 turbulence assemblies. This is a
work reduction under matched physical/state requirements. The observed loop
times 18.61 s / 0.739 s are single contended samples, not a general speedup result.

Fixed-control SA33 still costs 4,481 Newton updates, with 50,514 full flow assemblies
for segregated mode, versus 5,032 native updates/assemblies. More accurate matrix-free
products do not buy enough fewer outer updates in that regime to offset their
fully refreshed residual cost. Independent defect and slope checks add probes
even when every accepted step uses alpha one. These contracts remain intact.
Coupled lower preconditioning adds further physical coupling products; diagonal
preconditioning often lowers assembly work despite needing more Krylov iterations.
Stronger preconditioning and fewer nonlinear updates remain more promising than
freezing its relatively cheap construction, but no new Jacobian approximation or
multilevel solver is claimed here.

Native also receives useful tuning: planar SA/BC CFL 50 qualify in 1,461/1,462 updates,
SST CFL 30 in 2,454, and LM CFL 100000 in 50 (periodic LM: 49), on both layouts. Consequently,
Newton is not universally faster and a small outer-count advantage in coupled LM
cannot offset many more residual evaluations. Native history near misses are
rechecked with a stricter history gate while preserving all physical targets.
For refined LM, this yields a qualified native CFL 1000 reference in 324 updates,
improving on the 654-update default schedule; the original rejected result is
preserved beside the stricter stopping configuration.

The bounded strict study has 16 qualified endpoints in 39 trials; three cohorts
lack a native reference, so 21 planned Newton comparisons were not launched.
An independent check of the saved SST33 timeout gives physical RMS 3.29e-5, confirming
that this endpoint genuinely misses its target. The two SA65 fixed20 timeouts
produced no final restart; their physical endpoints remain unverified. Native CFL 5
with a 600 s budget is a separate follow-up, never a relaxation of accuracy. Its
completed results, if any, are shown above; pending attempts make no timing claim.

The conservative follow-up is complete: SST33 at native CFL 5 reaches the
50,000-update cap, and both SA65 layouts time out at 600 s. None supplies the
required native reference, so these larger matched comparisons remain incomplete.
No Newton endpoint or speedup is claimed for those cohorts. The final quiet-host
preflight is also preserved: 83 observations all exceed the external-CPU limit;
it is cancelled before any CFD launch. Repeated observed medians remain reported
with contention, and no uncontended or general speedup claim is made.

## Reproduction and limits

The evidence root is
`/media/rausa/4TB/SU2_Versions/SU2_v850_Develop_FixCGNSOutput/newton_krylov_refactor_20261015/efficiency_20261017`.
`baseline_manifest.json`, `final_release/manifest.json` and the final audit contain
source, binary and build-option hashes. Every retained run includes its exact
configuration and hash; command ledgers retain the executable, arguments, layout
and working directory. `original_medians.json` contains all release samples and
ranges; `final_phase_summary.json` contains phase counts/times. Configuration
outcomes are in [the CSV](../../Docs/NewtonKrylovControls20261017.csv), with
full raw reports/restarts/logs under `cfl_study/`, `larger_strict_control_study/`
and `moderate_native_study/`, with separate history-margin rechecks and
`extended_refined_references/` follow-up. The superseded refinement calibration remains
under `larger_control_study/`, including the state-agreement failure.

Qualified production code: `4712d82375e00ede15e5177ab38865eaaaa3e9f5`.
SHA-256:

| Binary | SHA-256 |
|---|---|
| Baseline SU2_CFD | `185fbc3d8758d34ffa8af71b3d37759823cb2caf0947b5ae6c236d8892fa08f7` |
| Final SU2_CFD | `75b0d848ee24bca74d7d472a612bbca34986757c77339ff9cfc748edf06340d1` |
| Final test_driver | `023743e2965ab287ab92efaaefb4c8c05caa9cf9d85f161300641f3f7a691a43` |
| Final newton_equation_tests | `905f30d4a59c46286f0806dd4bbec8be31555eab5f94cf805ed10c7a2a26ee01` |

Example commands below require fresh output directories/repeat numbers. The
strict original-control guard refuses a busy host; `--correctness-only` explicitly
excludes timings. Run one owned solver/build job at a time. MPI uses ROMIO321 and
a local temporary directory; OpenMP waits passively. No external process is stopped
or reconfigured by the runner. Foreign builds/solvers gate later launches, and
guards retain CPU, process-tree RSS, available RAM and swap observations. The
minimum available-RAM policy is 8 GiB with at most 64 MiB swap growth per guarded
batch. Instrumented profiles never substitute for release timings.

```bash
ROOT=/media/rausa/4TB/SU2_Versions/SU2_v850_Develop_FixCGNSOutput
ART="$ROOT/newton_krylov_refactor_20261015"
EFF="$ART/efficiency_20261017"
SRC="$ROOT/scalar_limiter_sst_globalization_20261015/source"
export TMPDIR="$ART/mpi_tmp" OMPI_MCA_orte_tmpdir_base="$ART/mpi_tmp"
export OMPI_MCA_io=romio321 OMP_WAIT_POLICY=PASSIVE OMP_NUM_THREADS=1

# One matched triplet per version. Repeat three times in alternating order.
python3 "$EFF/run_efficiency_case_boundary.py" --version baseline \
  --case sa --layout 1x1 --repeat 9001
python3 "$EFF/run_efficiency_case_boundary.py" --version final_release \
  --case sa --layout 1x1 --repeat 9001

# A separately labelled high-CFL configuration trial, retaining the original oracle.
python3 "$EFF/run_guarded_tree.py" --correctness-only \
  --evidence "$EFF/reproduce_cfl_guard" --cwd "$ROOT" -- \
  python3 "$EFF/cfl_case.py" --case sa --mode segregated --variant fixed \
  --cfl 1000 --ranks 1 --threads 1 \
  --reference "$EFF/benchmarks/final_release/sa/1x1/1" \
  --output "$EFF/reproduce_sa_cfl1000"

# Supported ordinary units; full exact qualification commands remain in the ledgers.
cd "$ART/equation_set_verification_work/units"
mpirun --bind-to none -n 1 "$EFF/final_release/test_driver" \
  --input-file "$ART/goal_completion_20261016/supported_units_final.txt"
```

The common residual cost is smaller, but fully refreshed finite-difference
products still cost substantially more than native assembled matrix products.
Independent defects/slopes, boundary closure, live transport refresh and exact
restoration remain necessary numerical work. Lower-block coupling and many small
reductions/OpenMP barriers remain costly. Shared scratch reduces duplicated
temporary storage; accepted controller states remain separate. No claim about
asymptotic scaling, production-mesh speedup or a universally robust CFL follows
from these small fixtures. Larger one-update memory runs are not converged
benchmarks. A production mesh and quiet-host repeated timings remain additional
qualification work, not evidence that can be supplied by relaxing targets.
