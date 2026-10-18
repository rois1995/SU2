# Bounded Krylov restart assessment — 18 October 2026

The one-restart recovery experiment is **not adopted**. Production sources are restored
byte-for-byte to `71ed31ddea276abd45220625c1cdb045b5b2c1aa`; common inner preconditioning
retains its default of zero additional iterations. The experiment has no qualified full
endpoint, and its early assembly saving reverses at subsequent density milestones.
The full trial was interrupted by a resource guard, not a demonstrated numerical failure.
No full-endpoint performance ratio or speedup is claimed.

## Experiment and verification

Qualified parallel SA diagnostics motivated the test: 165 derivative-verification
rejections retained short partial Krylov solutions whose independently checked defects
missed the unchanged 0.1 limit. The candidate restarted once from the verified residual
`b - (J+D)x`, at the same captured base, CFL and preconditioner. Both solves shared the
existing 80-iteration budget. Scaling the restart tolerance by the partial true defect
retained the original absolute linear target. The combined correction received another
independent full-product check before the existing endpoint, admissibility and Armijo checks.
Existing packed scratch held the increment; no extra packed vector or configuration option
was introduced. Derivative verification, forcing, acceptance and physical targets stayed fixed.

All eleven candidate regression jobs passed: 69 integration cases on each layout,
92 ordinary cases, and species, failure/restart, turbomachinery and setup checks on both
layouts. CSV appending retained both earlier schemas. The 20-update application check
exercised an accepted restart, failed restarts with ordinary CFL recovery, zero-progress
exclusion and the total iteration budget. Its first four attempt rows match production.

## Matched application work

Both prefix decks have identical configuration hashes, as do both full-run decks.
Assembly counters are measured and include initialization and rejected work. Times are
contended observations; the interrupted elapsed time is time to resource termination.
The [CSV](NewtonKrylovRestartAssessment20261018.csv) retains hashes, memory, residuals,
state/quantity agreement and evidence paths.

| Parallel SA | Status | Updates | FLOW/TURB assemblies | Outer K | Retries | Restarts/accepted | Loop / elapsed s |
|---|---|---:|---:|---:|---:|---:|---:|
| Production prefix | Supporting | 20 | 5464/5464 | 1482 | 10 | 0/0 | 69.982 / 70.462 |
| Restart prefix | Supporting | 20 | 4689/4689 | 1177 | 10 | 6/1 | 55.847 / 56.349 |
| Earlier production endpoint | Qualified | 333 | 83590/83590 | 22279 | 166 | 0/0 | 1090.736 / 1091.372 |
| Restart full attempt | Resource interruption | 207 | 65056/65056 | 17406 | 103 | 99/1 | unavailable / 1248.127 |

Of 99 restarted attempts, only one was accepted. All 98 others have two failed products,
a finite checked final defect, derivative-verification rejection and 23–43 completed
iterations, below the 80-iteration budget. Given the source path and absence of lower
coupling actions, these counters indicate another unavailable Arnoldi action rather than
budget exhaustion or failure to evaluate the combined direction. Exact incremental restart
K was not separately logged. All accepted steps retain alpha=1; rollback errors are zero.

| Density-history milestone | Production updates / assemblies / K | Restart updates / assemblies / K |
|---|---:|---:|
| -12.4 | 12 / 3114 / 836 | 4 / 673 / 159 |
| -12.5 | 33 / 9281 / 2537 | 37 / 10240 / 2685 |
| -12.6 | 53 / 15155 / 4149 | 57 / 16741 / 4439 |
| -12.8 | 93 / 26803 / 7344 | 97 / 29618 / 7912 |
| -13 | 131 / 37666 / 10318 | 136 / 42132 / 11274 |

These are density-only work milestones, not all-equation endpoint comparisons. The saved
candidate checkpoint passes the frozen physical/state/quantity checks, with maximum paired
component error 9.38e-9 and point error 1.02e-6. It is a checkpoint, not the state of all
207 completed updates; no iteration-specific work-to-state claim is assigned to it.
The interrupted run remains unqualified even though that saved state passes the checks.

## Resource interruption and preserved evidence

The existing global-swap guard stopped only the owned numerical tree when swap usage grew
from 1,135,616 to 1,204,224 KiB, exceeding its 65,536 KiB growth limit. Available RAM was
39,195,868 KiB at that sample; peak sampled owned-tree RSS was 267,764 KiB. Multiple foreign
compiler jobs were observed and left untouched. This does not establish a solver memory
regression. Rank peak counters and the normal solver-loop timer were unavailable after
interruption. Neither a resource limit nor missing timing establishes convergence failure.

The original runner labelled the nonzero launcher exit `solver_failure`; that raw result
is preserved. The runner and ledger now distinguish memory-guard interruption, with six
status checks and a generated-table assertion. The ledger also records saved-state kind,
minimum available RAM and guard evidence. Physical qualification is not overridden.

Evidence is under `application_qualification_20261018/` in the workspace archive:

- `bounded_restart_release/manifest.json`, `source.patch`: candidate source/build hashes and binaries.
- `bounded_restart_checks/commands.json`: all eleven passing guarded jobs.
- `bounded_restart_20261018/sa_parallel_{prefix,full}_command.json`: exact application commands.
- `bounded_restart_20261018/assessment.json`: complete metrics, matching hashes and milestones.
- `bounded_restart_applications/naca_sa/2x2/full/coupled/`: raw logs, counters, checkpoint,
  physical report, host observations, stopped process identities and resource classification.
- `bounded_restart_20261018/production_sources_restored.json`: exact restored source hashes.
- `bounded_restart_20261018/claude/`: user-approved, source-free Opus 5.5 high discussion,
  response and local disposition; tools, hooks, MCP and implicit memory were disabled.

The next limited study uses existing fixed forcing 0.05 with adaptive forcing disabled,
on the unchanged production implementation. It is a configuration experiment, distinct
from changing the forcing signal or adopting new recovery. All physical, derivative,
true-defect and acceptance requirements remain fixed. Native linear accuracy and both
layouts still require matched qualification; this run alone cannot establish a general gain.
