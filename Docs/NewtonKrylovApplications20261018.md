# Newton–Krylov application qualification — 18 October 2026

Work in progress. Numerical implementation: `4712d82375e00ede15e5177ab38865eaaaa3e9f5`;
source checkout at study start: `8e61376ecd234414ce583c9b8f995e2bd62f4c12`.

This study uses existing application meshes, independently of the earlier generated fixtures.
All inputs, initial user edits, binaries, build options and source hashes are frozen under
`/media/rausa/4TB/SU2_Versions/SU2_v850_Develop_FixCGNSOutput/newton_krylov_refactor_20261015/application_qualification_20261018`. The two pre-existing dirty reports are preserved.

| Application | Mesh points | Initial condition |
|---|---:|---|
| NACA0012 SA | 14576 | Original supplied SA restart |
| NACA0012 SST | 14576 | Original supplied SST restart |
| Axisymmetric SST nozzle | 4980 | Original supplied restart |
| Physical T3A SST–LM flat plate | 68129 | Tutorial free stream |
| Periodic transonic SST stator (exploratory) | 17447 | Original supplied restart |
| Periodic VKI SA turbine | 15860 | Original supplied ASCII restart |

Discovery preserves the original spatial discretization, physics and CFL controls.
It caps work explicitly and retains failed runs. Output is restricted to restart plus history,
with performance reporting enabled identically in all modes. T3A wall force monitoring and nozzle
inlet/outlet monitoring add measurements. Common Newton uses fixed CFL and fixed forcing
in the original-control discovery; recovery can still reduce CFL. Its coupled preconditioner
uses the lower block option. These choices will be tested separately.

The first three T3A setup attempts used an incorrect runner edit (`TIME_MARCHING=STEADY`).
The parser requires the original `NO` spelling; the edit is corrected and the failures are
retained as runner errors. The VKI deck uses the current `ITER` spelling for its old `EXT_ITER`.

Original case stopping may monitor density alone. Discovery completion is therefore **not**
qualification of every active equation. Fresh residuals, complete restart agreement and physical
quantities are required before any result can be described as qualified.

Frozen endpoint requirements retain each application's original density RMS target
(1e-12; T3A 1e-11), require normalized physical RMS <=1e-9 for **every active component**,
and bound scaled boundary projection by1e-12. Full-state component relative Linf <=1e-6,
pointwise scaled Linf <=1e-5 (scale floor 1e-3 of component maximum), and quantities
agree within relative1e-5 / absolute1e-8. Transition must be active. These checks are
independent of the solver's unchanged acceptance and derivative verification.

Initial endpoint checks: warm NACA SA native and segregated pass the frozen targets.
Coupled passes physical residual checks but misses pointwise state agreement (SA field
relative scaled error1.66e-5 versus1e-5). It remains rejected. No target was relaxed.
SST and nozzle discovery endpoints miss the density target. The first SST coupled timeout
has no saved final restart; its endpoint is unverified. All capped and failed times describe
work to termination, not time to convergence.

The Giles stator is exploratory: `CTurboIteration` updates its boundary averages outside
the Newton residual wrapper. Their live derivative and rollback contract needs a separate
check. The established VKI turbine uses standard inlet/outlet boundaries and is the primary
substantial periodic candidate. Generated periodic fixtures remain supporting regressions.

No numerical code changes have been made in this study. Quiet-machine repetitions, full
physical qualification, cold-start comparisons, both layouts, and the CFL/forcing/preconditioner
study remain outstanding. Current discovery times are observations only.

## Coverage

| Application | Layout | Actual trials | Qualified endpoints | Eligible timing repetitions |
|---|---|---:|---:|---:|
| naca_sa | 1x1 | 3 | 2 | 0 |
| naca_sa | 2x2 | 0 | 0 | 0 |
| naca_sst | 1x1 | 3 | 0 | 0 |
| naca_sst | 2x2 | 0 | 0 | 0 |
| nozzle_sst | 1x1 | 3 | 0 | 0 |
| nozzle_sst | 2x2 | 0 | 0 | 0 |
| flatplate_lm | 1x1 | 6 | 0 | 0 |
| flatplate_lm | 2x2 | 0 | 0 | 0 |
| periodic_stator | 1x1 | 3 | 0 | 0 |
| periodic_stator | 2x2 | 0 | 0 | 0 |
| periodic_vki | 1x1 | 0 | 0 | 0 |
| periodic_vki | 2x2 | 0 | 0 | 0 |

## Current run ledger

[Machine-readable ledger](NewtonKrylovApplications20261018.csv). Every actual setting and failure
is retained; `prepared` and `running` entries have not completed.

| Case | Layout | Mode | Initial | Control | Status | Updates | Loop s | End-to-end s |
|---|---|---|---|---|---|---:|---:|---:|
| flatplate_lm | 1x1 | coupled | original | original | solver_failure | 0 | — | 1.432 |
| flatplate_lm | 1x1 | native | original | original | solver_failure | 0 | — | 1.349 |
| flatplate_lm | 1x1 | segregated | original | original | solver_failure | 0 | — | 1.356 |
| naca_sa | 1x1 | coupled | original | original | rejected | 11 | 13.852 | 14.424 |
| naca_sa | 1x1 | native | original | original | qualified | 11 | 3.351 | 3.967 |
| naca_sa | 1x1 | segregated | original | original | qualified | 11 | 25.907 | 26.736 |
| naca_sst | 1x1 | coupled | original | original | timeout | 16 | — | 121.018 |
| naca_sst | 1x1 | native | original | original | iteration_cap | 200 | 73.236 | 73.902 |
| naca_sst | 1x1 | segregated | original | original | iteration_cap | 20 | 72.321 | 72.900 |
| nozzle_sst | 1x1 | coupled | original | original | iteration_cap | 20 | 5.296 | 5.773 |
| nozzle_sst | 1x1 | native | original | original | iteration_cap | 200 | 4.396 | 4.807 |
| nozzle_sst | 1x1 | segregated | original | original | iteration_cap | 20 | 5.328 | 5.779 |
| periodic_stator | 1x1 | coupled | original | original | iteration_cap | 20 | 114.285 | 114.932 |
| periodic_stator | 1x1 | native | original | original | iteration_cap | 200 | 19.509 | 20.140 |
| periodic_stator | 1x1 | segregated | original | original | timeout | 18 | — | 121.040 |
| flatplate_lm | 1x1 | coupled | original | original | prepared | — | — | — |
| flatplate_lm | 1x1 | native | original | original | iteration_cap | 200 | 82.402 | 83.438 |
| flatplate_lm | 1x1 | segregated | original | original | timeout | 16 | — | 181.072 |

## Reproduction

Input and build hashes: `manifest.json` in the evidence directory.
Each trial preserves `run.cfg`, exact command, runner hash, exit status, log, history,
Newton attempt/trial diagnostics and per-rank process resources. Discovery commands:

```sh
python3 application_case.py --case naca_sa --mode native --updates 200 --timeout 120 --output NEW_DIRECTORY
```

Run from the evidence directory using the preserved runner and manifest. The runner
defers to external compiler/solver jobs. The enclosing `pilots_monitor` records host CPU/RAM
and swap throughout; it explicitly marks these discovery observations timing-ineligible.

Solver-loop time is the native SU2 performance timer. End-to-end time includes MPI startup,
initialization and final output, excluding the contention gate and independent endpoint oracle.
Rank peak RSS comes from `/usr/bin/time`; the sum of rank peaks is an upper bound on their
simultaneous total. Process-tree RSS is sampled once per second. Profiling binaries will be
kept separate from release timing. Native per-system counts require measured profile confirmation;
Newton cumulative attempt counters exclude incomplete attempts on a timeout unless a complete
profile is available. Such failure counters are lower bounds. No timing assertions are used.
