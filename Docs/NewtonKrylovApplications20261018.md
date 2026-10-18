# Newton–Krylov application qualification — 18 October 2026

Work in progress. Baseline numerical implementation: `4712d82375e00ede15e5177ab38865eaaaa3e9f5`;
source checkout at study start: `8e61376ecd234414ce583c9b8f995e2bd62f4c12`.
Committed corrections: momentum probes `637177c3e4` and bounded verified-probe recovery `d27f4b37f6`.
See [momentum probes](NewtonKrylovProbeMetric20261018.md) and [recovery evidence](NewtonKrylovProbeRecovery20261018.md).

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
| Periodic VKI SA turbine | 15860 | Explicit cold start; supplied restart has incompatible point ordering/count |

Discovery preserves the original spatial discretization, physics and CFL controls.
It caps work explicitly and retains failed runs. Output is restricted to restart plus history,
with performance reporting enabled identically in all modes. Later long trials also save restart
checkpoints every100 updates; timeouts label the saved state as a checkpoint, never a converged endpoint.
T3A wall force monitoring and nozzle
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

Initial endpoint checks: warm NACA SA native and segregated pass against the initial
11-update native reference. A conservative native151-update reference (density history stop
tightened from-12 to-13) exposes pointwise SA differences8.15e-5 for the original native
and8.35e-5 for segregated, above1e-5. Early agreement is therefore insufficient evidence
of a stable final state. Original records are retained; margin trials use the stricter reference.
Coupled passes physical residual checks but misses initial pointwise state agreement (SA field
relative scaled error1.66e-5 versus1e-5). It remains rejected. No target was relaxed.
SST and nozzle discovery endpoints miss the density target. The first SST coupled timeout
has no saved final restart; its endpoint is unverified. All capped and failed times describe
work to termination, not time to convergence.

The Giles stator is exploratory: `CTurboIteration` updates its boundary averages outside
the Newton residual wrapper. The tested capability guard now rejects this unsupported setup.
The established VKI turbine uses standard inlet/outlet boundaries and is the primary
substantial periodic candidate. Generated periodic fixtures remain supporting regressions.

The committed momentum finite-difference correction and bounded verified-probe recovery are
under application validation. The focused independent
derivative detector fails on the baseline and passes on the candidate; state/residual scales
and derivative/acceptance tolerances are unchanged. Its immutable source patch and binaries are
under `probe_metric/` and `recovery_release/`. Neither establishes an application speedup.
Quiet repetitions, full
physical qualification, cold-start comparisons, both layouts, and the CFL/forcing/preconditioner
study remain outstanding. Current discovery times are observations only.
The full native nozzle stops after548 updates but its fresh normalized transverse momentum
RMS7.65e-9 misses1e-9. Before bounded probe recovery, fixed-control Newton retains CFL0.0153.
The recovery prefix retains62.5 and advances density farther while spending more work;
see its separate before/after work table. Neither20-update endpoint qualifies.
SST native258-update endpoints qualify on both layouts; fixed-control Newton times out.
Cold VKI original-control trials have not qualified in any mode. T3A remains in progress.
All18 native151-update SA timing attempts qualified physically but crossed the quiet-machine
limit during execution. None is eligible. Final-code fixed-control Newton margin attempts
hit120-second limits on both layouts; these are failures, not time-to-convergence samples.
Forcing studies explicitly start the existing adaptive strategy at0.1 (true-defect limit stays0.1);
adaptive-CFL-only studies retain the original0.001 initial linear target. These are configuration changes.

## Coverage

| Application | Layout | Actual trials | Qualified endpoints | Eligible timing repetitions |
|---|---|---:|---:|---:|
| naca_sa | 1x1 | 20 | 13 | 0 |
| naca_sa | 2x2 | 17 | 11 | 0 |
| naca_sst | 1x1 | 9 | 1 | 0 |
| naca_sst | 2x2 | 6 | 1 | 0 |
| nozzle_sst | 1x1 | 23 | 0 | 0 |
| nozzle_sst | 2x2 | 6 | 0 | 0 |
| flatplate_lm | 1x1 | 12 | 0 | 0 |
| flatplate_lm | 2x2 | 3 | 0 | 0 |
| periodic_stator | 1x1 | 3 | 0 | 0 |
| periodic_stator | 2x2 | 0 | 0 | 0 |
| periodic_vki | 1x1 | 9 | 0 | 0 |
| periodic_vki | 2x2 | 6 | 0 | 0 |

## Current run ledger

[Machine-readable ledger](NewtonKrylovApplications20261018.csv). Every actual setting and failure
is retained; `prepared` and `running` entries have not completed.

| Case | Layout | Mode | Initial | Implementation | Control | History stop | Status | Updates | Loop s | End-to-end s |
|---|---|---|---|---|---|---:|---|---:|---:|---:|
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-4 | -12 | diagnostic_complete | 1 | — | 1.989 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-6 | -12 | diagnostic_complete | 1 | — | 2.426 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-8 | -12 | diagnostic_complete | 1 | — | 4.579 |
| nozzle_sst | 1x1 | coupled | original | application_release | forcing | -12 | iteration_cap | 1 | 3.482 | 3.961 |
| nozzle_sst | 1x1 | segregated | original | application_release | forcing | -12 | iteration_cap | 1 | 3.301 | 3.765 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-3_relative_0.01 | -12 | diagnostic_complete | 1 | — | — |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-5_relative_0.01 | -12 | diagnostic_complete | 1 | — | — |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-5 | -12 | diagnostic_complete | 1 | — | 2.727 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-6 | -12 | diagnostic_complete | 1 | — | 2.880 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-7 | -12 | diagnostic_complete | 1 | — | 1.765 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | solver_failure | 0 | — | 1.432 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | solver_failure | 0 | — | 1.349 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | solver_failure | 0 | — | 1.356 |
| naca_sa | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | rejected | 11 | 13.852 | 14.424 |
| naca_sa | 1x1 | native | original | baseline_4712d82375 | original | -12 | qualified | 11 | 3.351 | 3.967 |
| naca_sa | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | qualified | 11 | 25.907 | 26.736 |
| naca_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | timeout | 16 | — | 121.018 |
| naca_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 200 | 73.236 | 73.902 |
| naca_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 20 | 72.321 | 72.900 |
| nozzle_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 20 | 5.296 | 5.773 |
| nozzle_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 200 | 4.396 | 4.807 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 20 | 5.328 | 5.779 |
| periodic_stator | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 20 | 114.285 | 114.932 |
| periodic_stator | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 200 | 19.509 | 20.140 |
| periodic_stator | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | timeout | 18 | — | 121.040 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | 20 | 129.516 | 130.661 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | 200 | 82.402 | 83.438 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | timeout | 16 | — | 181.072 |
| periodic_vki | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | solver_failure | 0 | — | 1.707 |
| periodic_vki | 1x1 | native | original | baseline_4712d82375 | original | -12 | solver_failure | 0 | — | 1.662 |
| periodic_vki | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | solver_failure | 0 | — | 1.622 |
| nozzle_sst | 1x1 | segregated | original | probe_metric | original | -12 | iteration_cap | 20 | 7.975 | 8.448 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | 1 | 43.213 | 44.499 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | 1 | 0.426 | 1.813 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | iteration_cap | 1 | 12.930 | 14.264 |
| flatplate_lm | 2x2 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | 1 | 24.777 | 25.717 |
| flatplate_lm | 2x2 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | 1 | 0.252 | 1.207 |
| flatplate_lm | 2x2 | segregated | original | baseline_4712d82375 | original | -11 | iteration_cap | 1 | 7.193 | 8.197 |
| naca_sa | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 2.500 | 3.051 |
| naca_sa | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.125 | 0.689 |
| naca_sa | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 2.562 | 3.124 |
| naca_sa | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.765 | 1.446 |
| naca_sa | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.080 | 0.687 |
| naca_sa | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 1.220 | 1.718 |
| naca_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 8 | 31.357 | 31.971 |
| naca_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 8 | 1.089 | 1.651 |
| naca_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 8 | 18.969 | 19.599 |
| naca_sst | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 8 | 11.792 | 12.336 |
| naca_sst | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 8 | 0.690 | 1.217 |
| naca_sst | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 8 | 6.359 | 6.862 |
| nozzle_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 1.668 | 2.131 |
| nozzle_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.017 | 0.507 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 1.449 | 1.855 |
| nozzle_sst | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.782 | 1.255 |
| nozzle_sst | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.008 | 0.472 |
| nozzle_sst | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.737 | 1.154 |
| periodic_vki | 1x1 | coupled | cold | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 3.856 | 4.817 |
| periodic_vki | 1x1 | native | cold | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.040 | 0.586 |
| periodic_vki | 1x1 | segregated | cold | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 6.793 | 7.368 |
| periodic_vki | 2x2 | coupled | cold | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 2.068 | 2.587 |
| periodic_vki | 2x2 | native | cold | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 0.033 | 0.584 |
| periodic_vki | 2x2 | segregated | cold | baseline_4712d82375 | original | -12 | iteration_cap | 1 | 1.999 | 2.499 |
| flatplate_lm | 1x1 | coupled | original | probe_metric | original | -11 | running | — | — | — |
| flatplate_lm | 1x1 | native | original | probe_metric | original | -11 | rejected | 695 | — | 301.077 |
| flatplate_lm | 1x1 | segregated | original | probe_metric | original | -11 | timeout | 23 | — | 301.059 |
| naca_sa | 1x1 | coupled | original | probe_metric | original | -12 | rejected | 11 | 7.754 | 8.336 |
| naca_sa | 1x1 | native | original | probe_metric | original | -12 | qualified | 11 | 1.376 | 1.905 |
| naca_sa | 1x1 | segregated | original | probe_metric | original | -12 | qualified | 11 | 13.804 | 15.252 |
| naca_sa | 2x2 | coupled | original | probe_metric | original | -12 | rejected | 11 | 4.042 | 4.537 |
| naca_sa | 2x2 | native | original | probe_metric | original | -12 | qualified | 11 | 1.052 | 1.606 |
| naca_sa | 2x2 | segregated | original | probe_metric | original | -12 | qualified | 11 | 6.705 | 7.160 |
| naca_sst | 1x1 | coupled | original | probe_metric | original | -12 | rejected | 236 | — | 301.066 |
| naca_sst | 1x1 | native | original | probe_metric | original | -12 | qualified | 258 | 40.705 | 41.323 |
| naca_sst | 1x1 | segregated | original | probe_metric | original | -12 | rejected | 202 | — | 301.034 |
| naca_sst | 2x2 | coupled | original | probe_metric | original | -12 | rejected | 425 | — | 301.044 |
| naca_sst | 2x2 | native | original | probe_metric | original | -12 | qualified | 258 | 26.195 | 26.766 |
| naca_sst | 2x2 | segregated | original | probe_metric | original | -12 | rejected | 465 | — | 301.071 |
| nozzle_sst | 1x1 | coupled | original | probe_metric | original | -12 | timeout | 2139 | — | 301.040 |
| nozzle_sst | 1x1 | native | original | probe_metric | original | -12 | rejected | 548 | 15.533 | 15.969 |
| nozzle_sst | 1x1 | segregated | original | probe_metric | original | -12 | timeout | 1576 | — | 301.032 |
| nozzle_sst | 2x2 | coupled | original | probe_metric | original | -12 | iteration_cap | 3000 | 183.077 | 183.560 |
| nozzle_sst | 2x2 | native | original | probe_metric | original | -12 | rejected | 546 | 5.656 | 6.100 |
| nozzle_sst | 2x2 | segregated | original | probe_metric | original | -12 | iteration_cap | 3000 | 289.357 | 289.778 |
| periodic_vki | 1x1 | coupled | cold | probe_metric | original | -12 | rejected | 235 | — | 301.042 |
| periodic_vki | 1x1 | native | cold | probe_metric | original | -12 | iteration_cap | 3000 | 111.915 | 112.478 |
| periodic_vki | 1x1 | segregated | cold | probe_metric | original | -12 | rejected | 436 | — | 301.073 |
| periodic_vki | 2x2 | coupled | cold | probe_metric | original | -12 | rejected | 448 | — | 301.059 |
| periodic_vki | 2x2 | native | cold | probe_metric | original | -12 | iteration_cap | 3000 | 65.549 | 66.163 |
| periodic_vki | 2x2 | segregated | cold | probe_metric | original | -12 | rejected | 911 | — | 301.061 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -13.0 | timeout | 29 | — | 121.027 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 19.319 | 19.860 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -13.0 | timeout | 65 | — | 121.032 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 18.857 | 19.360 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 19.616 | 20.191 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 18.952 | 19.476 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 19.144 | 19.646 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 18.769 | 19.259 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 18.861 | 19.394 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 19.029 | 19.568 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | 151 | 18.743 | 19.302 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -13.0 | timeout | 84 | — | 121.046 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 12.894 | 13.346 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -13.0 | timeout | 66 | — | 121.080 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 13.122 | 13.658 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 13.084 | 13.597 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 12.632 | 13.143 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 12.597 | 13.229 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 13.219 | 13.754 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 13.390 | 13.904 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 12.795 | 13.242 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | 151 | 13.170 | 13.704 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery | original | -12 | iteration_cap | 1 | 3.954 | 4.414 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery_checked | fd_step_1e-8 | -12 | diagnostic_complete | 20 | — | 19.253 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery_checked | fd_step_1e-8 | -12 | diagnostic_complete | 1 | — | 4.708 |

## Measured baseline work prefixes

These are instrumented, capped prefixes, not convergence comparisons or eligible timing results.
Counts are measured method scopes on rank zero; every rank agrees with the telemetry.
Full per-rank inclusive/exclusive phase times and communication/allocation hooks are in
[the profile CSV](NewtonKrylovApplicationProfiles20261018.csv).
Do not add inclusive times across nested scopes. Profiles retain their original source snapshot.

| Case | Layout | Mode | Updates | Flow | Turb | Trans | Wrapper calls | Verified products | Preconditioner builds |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| flatplate_lm | 1x1 | coupled | 1 | 168 | 186 | 186 | 199 | 28 | 15 |
| flatplate_lm | 1x1 | native | 1 | 1 | 1 | 1 | 0 | 0 | 3 |
| flatplate_lm | 1x1 | segregated | 1 | 52 | 23 | 17 | 78 | 10 | 4 |
| flatplate_lm | 2x2 | coupled | 1 | 168 | 186 | 186 | 199 | 28 | 15 |
| flatplate_lm | 2x2 | native | 1 | 1 | 1 | 1 | 0 | 0 | 3 |
| flatplate_lm | 2x2 | segregated | 1 | 54 | 24 | 18 | 82 | 10 | 4 |
| naca_sa | 1x1 | coupled | 1 | 111 | 125 | 0 | 120 | 19 | 8 |
| naca_sa | 1x1 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| naca_sa | 1x1 | segregated | 1 | 129 | 35 | 0 | 154 | 21 | 5 |
| naca_sa | 2x2 | coupled | 1 | 77 | 82 | 0 | 78 | 9 | 6 |
| naca_sa | 2x2 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| naca_sa | 2x2 | segregated | 1 | 127 | 29 | 0 | 146 | 25 | 5 |
| naca_sst | 1x1 | coupled | 8 | 1253 | 1413 | 0 | 1403 | 177 | 18 |
| naca_sst | 1x1 | native | 8 | 8 | 8 | 0 | 0 | 0 | 16 |
| naca_sst | 1x1 | segregated | 8 | 769 | 105 | 0 | 845 | 199 | 17 |
| naca_sst | 2x2 | coupled | 8 | 931 | 1035 | 0 | 1023 | 123 | 22 |
| naca_sst | 2x2 | native | 8 | 8 | 8 | 0 | 0 | 0 | 16 |
| naca_sst | 2x2 | segregated | 8 | 535 | 128 | 0 | 633 | 115 | 18 |
| nozzle_sst | 1x1 | coupled | 1 | 189 | 200 | 0 | 190 | 21 | 18 |
| nozzle_sst | 1x1 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| nozzle_sst | 1x1 | segregated | 1 | 211 | 17 | 0 | 212 | 23 | 11 |
| nozzle_sst | 2x2 | coupled | 1 | 189 | 200 | 0 | 190 | 21 | 18 |
| nozzle_sst | 2x2 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| nozzle_sst | 2x2 | segregated | 1 | 211 | 19 | 0 | 214 | 23 | 11 |
| periodic_vki | 1x1 | coupled | 1 | 134 | 172 | 0 | 170 | 40 | 2 |
| periodic_vki | 1x1 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| periodic_vki | 1x1 | segregated | 1 | 287 | 11 | 0 | 290 | 91 | 3 |
| periodic_vki | 2x2 | coupled | 1 | 130 | 168 | 0 | 166 | 40 | 2 |
| periodic_vki | 2x2 | native | 1 | 1 | 1 | 0 | 0 | 0 | 2 |
| periodic_vki | 2x2 | segregated | 1 | 183 | 12 | 0 | 187 | 56 | 3 |

## Reproduction

Input and build hashes: `manifest.json` in the evidence directory.
Final-code binary/source hashes: `recovery_release/manifest.json`; original-control commands:
`qualified_original_commands.json` / `qualified_recovery_commands.json`; controls: `control_commands.json`.
Each trial preserves `run.cfg`, exact command, runner hash, exit status, log, history,
Newton attempt/trial diagnostics and per-rank process resources. Discovery commands:

```sh
python3 application_case.py --case naca_sa --mode native --updates 200 --timeout 120 --output NEW_DIRECTORY
```

Run from the evidence directory using the preserved runner and manifest. The runner
defers to external compiler/solver jobs. The enclosing `pilots_monitor` records host CPU/RAM
and swap throughout; correctness-only monitors never confer timing eligibility.
Only a physically qualified --quiet run whose every host observation passes may enter timing medians.

Solver-loop time is the native SU2 performance timer. End-to-end time includes MPI startup,
initialization and final output, excluding the contention gate and independent endpoint oracle.
Rank peak RSS comes from `/usr/bin/time`; the sum of rank peaks is an upper bound on their
simultaneous total. Process-tree RSS is sampled once per second. Profiling binaries will be
kept separate from release timing. Native per-system counts require measured profile confirmation;
Newton cumulative attempt counters exclude incomplete attempts on a timeout unless a complete
profile is available. Such failure counters are lower bounds. No timing assertions are used.
