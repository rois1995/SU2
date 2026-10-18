# Recover verified derivatives from cancellation

The SST nozzle application exposed a failure mode in derivative refinement. Halving a
probe already dominated by cancellation did not improve its verified derivative. Repeated
linear failures reduced the original CFL1000 to0.0153. On numerical source637177c3e4,
changing only the FD setting from1e-8 to1e-6 accepted the first update at CFL1000 with
zero retries, against the unchanged independent defect and acceptance limits. That result
is a configuration diagnostic, separate from the implementation below.

The implementation retains the original refinement sequence. If it fails, verified common
compressible flow products may try one100-times larger starting step, bounded by the same
relative limit and feasibility checks. The same5e-4 derivative verification remains mandatory.
A successful starting-scale adjustment is reused only within this linearization base; every
product is still verified and SetBase resets the adjustment. Legacy and unverified products
retain their existing behavior. No vector allocation or residual/state scaling is added.

The independent true-defect check remains necessary: the nozzle candidate rejected a
CFL250 attempt with defect0.100464 against the unchanged0.1 limit. Acceptance, rollback,
boundary closure, live dependencies, pseudo-time coordinates and physical targets are unchanged.

## Matched original-control work prefixes

Both versions use the original nozzle mesh, restart, physics and numerical controls,
including FD1e-8 and fixed CFL1000. Controller cumulative assembly counters exclude the
initial driver evaluation in both versions. These are capped prefixes, not convergence results.

| Updates | Version | Flow assemblies | Turb assemblies | Krylov work | Retries | Retained flow CFL | Fresh density RMS |
|---:|---|---:|---:|---:|---:|---:|---:|
| 1 | Before recovery | 572 | 16 | 116 | 8 | 0.0152588 | — |
| 1 | Recovery | 501 | 19 | 147 | 2 | 62.5 | — |
| 20 | Before recovery | 895 | 368 | 373 | 8 | 0.0152588 | 3.57508e-3 |
| 20 | Recovery | 2005 | 399 | 716 | 2 | 62.5 | 4.83158e-6 |

The20-update candidate spends more work and advances flow farther. Its turbulence
normalized RMS remains above target (maximum1.75e-6), as does flow (maximum0.00528).
Neither endpoint qualifies. All observed times are contention-ineligible; there is no
application time-to-convergence or general speedup claim. Full convergence, controls and
quiet repeated measurements remain part of the active application goal.

## Validation and reproduction

Evidence is preserved under the workspace's
`newton_krylov_refactor_20261015/application_qualification_20261018/`:

- `probe_recovery_checked/manifest.json` and `source.patch`: exact source and binary hashes.
- `probe_recovery_checks/commands.json`: all61 Newton equation tests on1x1/2x2,
  the91 supported units, focused derivative/recovery tests through2x4, and species,
  failure-path and capability CLI checks on both layouts. All pass.
- The homogeneous mass-action test retains its original1e-5 accuracy check at the original
  probe. The added default-step case checks the known action against the existing5e-4
  operator contract and checks fewer residual calls on repeated products of the same base.
  Earlier rejected test probes and failed development assertions remain preserved.
- `candidate_fd_resolution/`, `recovery_work_diagnostic/`, `recovery_prefix20/`: configurations,
  exact commands, logs, counters and independent physical report. No timing assertions.

The source test selector is `[NewtonMassResolution]`. Use the pinned test binary and the
recorded working directory/MPI environment from `probe_recovery_checks/commands.json`.
The maintained [application ledger](NewtonKrylovApplications20261018.md) records full trials,
configuration hashes, failed runs and remaining qualification work.
