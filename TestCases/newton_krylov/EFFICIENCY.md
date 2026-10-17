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

The expanded pool's repeated 263169-point memory measurements are running with
native and coupled controls. These fixed one-update runs measure allocation,
not convergence or elapsed-time performance. Per-rank peak RSS and RSS at the
same collective restart output are observed; the latter avoids masking live
workspace savings with a larger mesh-initialization peak. Results remain pending.

Lower-block residual work, further dependency-refresh savings, full family/layout
qualification, quiet repeated timings and the higher-CFL study remain open.
This report does not mark the overall goal complete.
