# Newton efficiency work, 17 October 2026

This is an active continuation of the [previous assessment](PERFORMANCE.md),
starting from `29a4c3c58add4b0164714a91ef330fa3a01654dc`. The goal also includes
memory, coupled preconditioning, synchronization, a larger practical case and
fair convergence-control comparisons. Higher fixed CFL trials follow the
implementation work and will be reported separately for native and Newton.

## Local residual refresh changes

Native volume and boundary integration now have separate entry points, retaining
the original boundary order. Auxiliary-only Newton evaluations refresh flow
boundary states without traversing volume flux/source kernels. Bounded scalars
retain full flow assembly for their live edge mass fluxes. Custom sources retain
full evaluation/failure checks, as do fixed-lift, engine and actuator-disk feedback.
MUSCL still refreshes its gradients/limiters; saved reconstruction counters remain
frozen, and auxiliary owners never commit flow reconstruction history.

Boundary closure reuses the preceding live coefficients when its global primary
state is unchanged, while preserving state/property checks. Changing strong
values triggers the normal refresh. No evaluation survives a new wrapper call,
state/configuration change or CFL retry. Laminar refreshes also omit the second
flow preprocessing previously used to copy newly updated turbulent viscosity.
External flow-feedback configurations retain their preprocessing cadence.

The final batch passes 58 native cases on 1x1 and 2x1. Focused tests compare
auxiliary residuals and characteristic states against full assembly at positive,
negative, repeated and zero trial steps, with MUSCL, nonzero frozen reconstruction
history, bounded scalars, SA/SST/LM, species and the existing flamelet fixture.
They check exact rollback and custom-source failure reporting.

Original-control serial SA/SST/active-LM comparisons retain **every outer and
Krylov count and every restart byte**, for all three modes. Independent uncached
physical/state/quantity checks also pass for mean flow and basic species.

| Segregated case | Outer / Krylov, unchanged | Full flow assemblies before → after | Other assemblies, unchanged |
|---|---:|---:|---:|
| SA | 2927 / 19749 | 56977 → 34886 | turbulence 25020 |
| SST | 3058 / 21184 | 60930 → 36466 | turbulence 27524 |
| SST + active LM | 1414 / 6434 | 27023 → 8920 | turbulence 9573; transition 11364 |

These are measured full-residual counters; boundary-only work is not counted as
full flow assembly. Coupled full-residual counts are unchanged in this batch.
Dependency-refresh savings require separate phase attribution. The runs were
continuously monitored for CPU contention, available RAM, swap growth and
process-tree RSS. A busy host excluded their wall times from performance claims.
One low-priority compiler or serial correctness job was used; no speedup is
claimed from these samples. Strict quiet-host repeated timings remain pending.

Source/binary/evidence hashes and detailed work counts are in
[NewtonKrylovEfficiency20261017.json](../../Docs/NewtonKrylovEfficiency20261017.json).
Raw evidence is preserved beside the checkout under
`newton_krylov_refactor_20261015/efficiency_20261017/`, including the initial dirty
documentation diff, cancelled contention preflight and a validation-harness
postprocessing error. The latter occurred after both native suites passed; the
remaining endpoint jobs resumed in a fresh directory.

## Shared evaluation scratch

Sequential groups now share the three per-system evaluation buffers, the flow
admission copy and the saved reconstruction counters. Accepted primary/old states,
row masks, relaxation, density coordinates, accepted/next reconstruction history,
and controller/Krylov state stay private. Sharing is local to one integration
container; independent equation sets still allocate their own scratch. Layout
checks reject attempts to resize buffers referenced by another owner.

The interleaved-owner test captures different bases, evaluates alternating groups
and checks exact restoration of each independent base. Smaller and larger layout
mismatches are rejected without damaging the existing owner. All 59 native cases
pass on 1x1 and 2x2. The five converged families (SA, SST, active LM, mean flow and
basic species) retain every original-control work count and every restart byte
against the residual-refresh version in all three solver modes.

On 263169 points, three alternating before/after pairs per model and mode
(54 solver runs total) preserve every one-update work count and every restart
byte. The shared pool's median process peak RSS is:

| Segregated model | Before, MiB | Shared pool, MiB | Saved, MiB |
|---|---:|---:|---:|
| SA | 1105.5 | 1064.7 | 40.8 |
| SST | 1248.6 | 1204.9 | 43.7 |
| LM | 1626.1 | 1511.7 | 114.4 |

These fixed one-update runs use CFL 0.05 on both versions to measure allocation,
not convergence or elapsed-time performance. Per-rank peak RSS and resident RSS
at the same collective restart output are observed; both have identical medians
here. Native/coupled controls have no consistent saving: median changes range
from 0.26 MiB lower to 1.64 MiB higher. Those modes do not duplicate sequential
workspaces. The observer's writer self-check includes empty MPI ranks; only
profiling binaries load this observer. Production code has no memory observer.

The cohort retained at least 38.3 GiB available RAM; swap use stayed constant.
External CPU usage reached 7.1 cores, so **none of these elapsed times qualifies
as performance evidence**. Raw reports, exact commands, hashes and ranges are in
`memory_complete_513/` and `workspace_memory_summary.json` under the evidence root.

## Requested rows in lower-block products

The lower preconditioner now evaluates only the residual row it consumes. Every
input column and live cross response remains present; boundary closure, property
refreshes, halo communication, frozen history and finite-difference sizing remain
unchanged. Skipped scalar systems still preprocess for dependent gradients, using
the existing refresh mode to retain settled prescribed masks. Bounded mass fluxes,
custom flow sources and external feedback retain their full-flow fallback.
Only unverified physical preconditioner coupling can request a partial row.
The outer operator, true-defect verification and globalization always use full
physical residuals with the original controls.

All 59 native tests pass on 1x1 and 2x2, including full-vs-requested products for
SA/SST/active LM, variable-density mixtures, the existing flamelet fixture,
translated/rotated periodic directions, prescribed rows and mixed/simplex bounds.
The comparisons check other-row zeros, skipped assembly counts and unchanged
prescribed masks. Independent centered-stencil checks remain in place.

All five converged families retain every outer/Krylov count and every restart
byte against the pooled version in all three modes. Native and segregated work
counts are unchanged. Coupled residual counts are:

| Model | Outer / Krylov, unchanged | Flow before → after | Turbulence before → after | Transition before → after |
|---|---:|---:|---:|---:|
| SA | 2927 / 14360 | 49210 → 34850 | 49210 → 49210 | — |
| SST | 3119 / 15367 | 52568 → 37201 | 52568 → 52568 | — |
| LM | 5615 / 6205 | 57927 → 45517 | 57927 → 51722 | 57927 → 51722 |

These are measured assembly counts. Wall-time results remain excluded for CPU
contention; lower-preconditioner inclusive time still requires a fresh profile.
Full family/layout qualification, fresh phase measurements, quiet repeated timings
and the higher-CFL study remain open. This goal is still active.

## Reuse gradients after transport updates

The second flow pass inside one dependency refresh now runs native common
preprocessing to copy updated turbulent transport properties without recomputing
unchanged temperature/velocity gradients, reconstruction gradients, vorticity and
strain. Its property calculations, admission counters and reductions remain live.
First/final flow preprocessing is unchanged. Wall functions and external feedback
retain the complete second pass. There is no cache across residual wrappers.

A direct native comparison changes turbulent transport data and requires exact
agreement of every primitive property with full flow preprocessing. Primary
values, primitive/reconstruction gradients, vorticity and strain remain identical,
and exact rollback is checked. It covers compressible SA/SST/active LM,
incompressible SA/SST and a variable-density mixture with SST, including separate
Green–Gauss reconstruction gradients.

All 60 native cases pass on 1x1, 1x4, 2x2 and 2x4. The five original-control
converged families retain every work count and every restart byte against the
requested-row version, in all three modes. The historical quiet gradient profiles
motivated this change; new phase counts/times and release timings remain pending.
Current contended timings are excluded. Higher-CFL studies follow implementation
measurement and will retain all existing physical/state/acceptance requirements.

## Reuse gradients during auxiliary boundary preparation

First-order SA/SST/LM probes with standard far-field, wall, symmetry or periodic
boundaries now retain the temperature/velocity gradients computed after strong
closure in the same residual wrapper. Native common preprocessing still refreshes
properties, admission checks, sensors and residual preparation. Full preprocessing
remains for flow/scalar reconstruction, species, wall functions, hybrid modes,
Roe dissipation sensors, streamwise-periodic recovery and other boundary types.
All flow-selected residuals still use complete preprocessing.

Ranks can own different boundary types. Eligibility is therefore reduced together
with the existing primary-state closure check, with no additional collective.
Every successful evaluation assigns it again before use; it is not a cache across
wrappers, retries, state changes or configuration changes. The first prototype's
MPI communication mismatch is preserved in `boundary_prep_correctness/` and
`boundary_prep_failed_candidate.json`; that prototype was not committed.

Full-vs-auxiliary tests include positive/negative/repeated/zero steps, separate
flow and scalar MUSCL fallbacks, compressible and incompressible SA/SST, active LM,
species/flamelet, frozen reconstruction history and exact rollback. Changing a
boundary only on its owning rank checks collective fallback. All 60 native cases
pass on 1x1, 1x4, 2x2 and 2x4. All five converged families retain every original
work count and restart byte against the property-only version in all three modes.
Final phase counts and repeated release timing comparisons follow; busy-host
correctness times remain excluded. Higher-CFL trials have not started.

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
Native remains faster on the fixed-CFL coarse turbulent fixtures. Newton retains
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

Final supported/periodic/CLI/restart checks pass, and Valgrind including the new
refresh comparisons reports zero errors. Additional periodic endpoints and the
separate higher-CFL/native-tuning/refined-mesh studies are still in progress.
