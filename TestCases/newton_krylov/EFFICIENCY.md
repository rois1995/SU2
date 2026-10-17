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
