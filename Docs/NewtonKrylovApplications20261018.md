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

Definitive SA comparisons also require the pinned151-update native reference (history stop-13).
The conservative audit preserves the original paired-check result, uses the same state/quantity
thresholds, and never changes numerical acceptance. The CSV retains `qualified` from the original
paired check and adds `comparison_qualified` plus complete conservative errors/reference hashes.
Coverage and eligible comparison counts below use `comparison_qualified`.

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
The pinned637 original-control cohort is complete (30 trials). Neither cold VKI nor T3A
qualified in any mode; all their Newton trials timed out. Original T3A native also timed out
(695 updates on1x1,1299 on2x2). Its coupled recovery retained CFL0.390625 after4 rejects.
Final-code d27 original-control and separate control cohorts remain in progress.
All18 native151-update SA timing attempts qualified physically but crossed the quiet-machine
limit during execution. None is eligible. Final-code fixed-control Newton margin attempts
hit120-second limits on both layouts; these are failures, not time-to-convergence samples.
Forcing studies explicitly start the existing adaptive strategy at0.1 (true-defect limit stays0.1);
adaptive-CFL-only studies retain the original0.001 initial linear target. These are configuration changes.
The unstarted control plan was revised from108 to126 trials after the retained-CFL/work results:
matched fixed-CFL100 trials were added for warm SA/SST/nozzle on both layouts. Nozzle control
trials use density history stop-13 after the original stop missed the all-component target.
A further10 coupled trials reverse the existing lower-block order, with identical forcing
and adaptive-CFL settings. This includes turbulence feedback in the flow correction; benefit
is unproven. Three SST1x1 pilot trials add existing ILU fill1, with matched native settings
and the same adaptive forcing/CFL controls. Further fill-level expansion requires evidence.
The139-trial plan records intended settings separately from actual results below.
All old plans/audits remain preserved. Preparation checks match106 Newton/native pairs;
these are setup checks, not measured convergence results. Both versions' early SA states
miss the conservative151-update state reference; tightened-stop controls remain necessary.

## Coverage

| Application | Layout | Actual trials | Qualified endpoints | Eligible timing repetitions |
|---|---|---:|---:|---:|
| naca_sa | 1x1 | 27 | 10 | 0 |
| naca_sa | 2x2 | 23 | 9 | 0 |
| naca_sst | 1x1 | 12 | 2 | 0 |
| naca_sst | 2x2 | 9 | 2 | 0 |
| nozzle_sst | 1x1 | 26 | 0 | 0 |
| nozzle_sst | 2x2 | 9 | 0 | 0 |
| flatplate_lm | 1x1 | 15 | 0 | 0 |
| flatplate_lm | 2x2 | 6 | 0 | 0 |
| periodic_stator | 1x1 | 3 | 0 | 0 |
| periodic_stator | 2x2 | 0 | 0 | 0 |
| periodic_vki | 1x1 | 12 | 0 | 0 |
| periodic_vki | 2x2 | 9 | 0 | 0 |

## Current run ledger

[Machine-readable ledger](NewtonKrylovApplications20261018.csv). Every actual setting and failure
is retained; `prepared` and `running` entries have not completed.

| Case | Layout | Mode | Initial | Implementation | Control | History stop | Status | Comparison pass | Updates | Loop s | End-to-end s |
|---|---|---|---|---|---|---:|---|---|---:|---:|---:|
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-4 | -12 | diagnostic_complete | False | 1 | — | 1.989 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-6 | -12 | diagnostic_complete | False | 1 | — | 2.426 |
| nozzle_sst | 1x1 | segregated | original | application_release | fd_step_1e-8 | -12 | diagnostic_complete | False | 1 | — | 4.579 |
| nozzle_sst | 1x1 | coupled | original | application_release | forcing | -12 | iteration_cap | False | 1 | 3.482 | 3.961 |
| nozzle_sst | 1x1 | segregated | original | application_release | forcing | -12 | iteration_cap | False | 1 | 3.301 | 3.765 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-3_relative_0.01 | -12 | diagnostic_complete | False | 1 | — | — |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-5_relative_0.01 | -12 | diagnostic_complete | False | 1 | — | — |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-5 | -12 | diagnostic_complete | False | 1 | — | 2.727 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-6 | -12 | diagnostic_complete | False | 1 | — | 2.880 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | fd_step_1e-7 | -12 | diagnostic_complete | False | 1 | — | 1.765 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -18.0 | rejected | False | 1 | 3.068 | 3.656 |
| naca_sa | 1x1 | native | original | recovery_release | original | -18.0 | rejected | False | 13 | 1.650 | 2.167 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -18.0 | rejected | False | 1 | 3.038 | 3.635 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -18.0 | rejected | False | 1 | 1.768 | 2.289 |
| naca_sa | 2x2 | native | original | recovery_release | original | -18.0 | rejected | False | 19 | 1.699 | 2.216 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -18.0 | rejected | False | 1 | 2.619 | 3.085 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.913 | 19.465 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | solver_failure | False | 0 | — | 1.432 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | solver_failure | False | 0 | — | 1.349 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | solver_failure | False | 0 | — | 1.356 |
| naca_sa | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | rejected | False | 11 | 13.852 | 14.424 |
| naca_sa | 1x1 | native | original | baseline_4712d82375 | original | -12 | reference_audit_failed | False | 11 | 3.351 | 3.967 |
| naca_sa | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | reference_audit_failed | False | 11 | 25.907 | 26.736 |
| naca_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | timeout | False | 16 | — | 121.018 |
| naca_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 200 | 73.236 | 73.902 |
| naca_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 72.321 | 72.900 |
| nozzle_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 5.296 | 5.773 |
| nozzle_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 200 | 4.396 | 4.807 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 5.328 | 5.779 |
| periodic_stator | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 20 | 114.285 | 114.932 |
| periodic_stator | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 200 | 19.509 | 20.140 |
| periodic_stator | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | timeout | False | 18 | — | 121.040 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 20 | 129.516 | 130.661 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 200 | 82.402 | 83.438 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | timeout | False | 16 | — | 181.072 |
| periodic_vki | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | solver_failure | False | 0 | — | 1.707 |
| periodic_vki | 1x1 | native | original | baseline_4712d82375 | original | -12 | solver_failure | False | 0 | — | 1.662 |
| periodic_vki | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | solver_failure | False | 0 | — | 1.622 |
| nozzle_sst | 1x1 | segregated | original | probe_metric | original | -12 | iteration_cap | False | 20 | 7.975 | 8.448 |
| flatplate_lm | 1x1 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 43.213 | 44.499 |
| flatplate_lm | 1x1 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 0.426 | 1.813 |
| flatplate_lm | 1x1 | segregated | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 12.930 | 14.264 |
| flatplate_lm | 2x2 | coupled | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 24.777 | 25.717 |
| flatplate_lm | 2x2 | native | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 0.252 | 1.207 |
| flatplate_lm | 2x2 | segregated | original | baseline_4712d82375 | original | -11 | iteration_cap | False | 1 | 7.193 | 8.197 |
| naca_sa | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 2.500 | 3.051 |
| naca_sa | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.125 | 0.689 |
| naca_sa | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 2.562 | 3.124 |
| naca_sa | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.765 | 1.446 |
| naca_sa | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.080 | 0.687 |
| naca_sa | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.220 | 1.718 |
| naca_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 31.357 | 31.971 |
| naca_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 1.089 | 1.651 |
| naca_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 18.969 | 19.599 |
| naca_sst | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 11.792 | 12.336 |
| naca_sst | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 0.690 | 1.217 |
| naca_sst | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 8 | 6.359 | 6.862 |
| nozzle_sst | 1x1 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.668 | 2.131 |
| nozzle_sst | 1x1 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.017 | 0.507 |
| nozzle_sst | 1x1 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.449 | 1.855 |
| nozzle_sst | 2x2 | coupled | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.782 | 1.255 |
| nozzle_sst | 2x2 | native | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.008 | 0.472 |
| nozzle_sst | 2x2 | segregated | original | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.737 | 1.154 |
| periodic_vki | 1x1 | coupled | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 3.856 | 4.817 |
| periodic_vki | 1x1 | native | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.040 | 0.586 |
| periodic_vki | 1x1 | segregated | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 6.793 | 7.368 |
| periodic_vki | 2x2 | coupled | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 2.068 | 2.587 |
| periodic_vki | 2x2 | native | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 0.033 | 0.584 |
| periodic_vki | 2x2 | segregated | cold | baseline_4712d82375 | original | -12 | iteration_cap | False | 1 | 1.999 | 2.499 |
| flatplate_lm | 1x1 | coupled | original | probe_metric | original | -11 | timeout | False | 50 | — | 301.067 |
| flatplate_lm | 1x1 | native | original | probe_metric | original | -11 | rejected | False | 695 | — | 301.077 |
| flatplate_lm | 1x1 | segregated | original | probe_metric | original | -11 | timeout | False | 23 | — | 301.059 |
| flatplate_lm | 2x2 | coupled | original | probe_metric | original | -11 | rejected | False | 103 | — | 301.039 |
| flatplate_lm | 2x2 | native | original | probe_metric | original | -11 | rejected | False | 1299 | — | 301.040 |
| flatplate_lm | 2x2 | segregated | original | probe_metric | original | -11 | timeout | False | 30 | — | 301.072 |
| naca_sa | 1x1 | coupled | original | probe_metric | original | -12 | rejected | False | 11 | 7.754 | 8.336 |
| naca_sa | 1x1 | native | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 1.376 | 1.905 |
| naca_sa | 1x1 | segregated | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 13.804 | 15.252 |
| naca_sa | 2x2 | coupled | original | probe_metric | original | -12 | rejected | False | 11 | 4.042 | 4.537 |
| naca_sa | 2x2 | native | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 1.052 | 1.606 |
| naca_sa | 2x2 | segregated | original | probe_metric | original | -12 | reference_audit_failed | False | 11 | 6.705 | 7.160 |
| naca_sst | 1x1 | coupled | original | probe_metric | original | -12 | rejected | False | 236 | — | 301.066 |
| naca_sst | 1x1 | native | original | probe_metric | original | -12 | qualified | True | 258 | 40.705 | 41.323 |
| naca_sst | 1x1 | segregated | original | probe_metric | original | -12 | rejected | False | 202 | — | 301.034 |
| naca_sst | 2x2 | coupled | original | probe_metric | original | -12 | rejected | False | 425 | — | 301.044 |
| naca_sst | 2x2 | native | original | probe_metric | original | -12 | qualified | True | 258 | 26.195 | 26.766 |
| naca_sst | 2x2 | segregated | original | probe_metric | original | -12 | rejected | False | 465 | — | 301.071 |
| nozzle_sst | 1x1 | coupled | original | probe_metric | original | -12 | timeout | False | 2139 | — | 301.040 |
| nozzle_sst | 1x1 | native | original | probe_metric | original | -12 | rejected | False | 548 | 15.533 | 15.969 |
| nozzle_sst | 1x1 | segregated | original | probe_metric | original | -12 | timeout | False | 1576 | — | 301.032 |
| nozzle_sst | 2x2 | coupled | original | probe_metric | original | -12 | iteration_cap | False | 3000 | 183.077 | 183.560 |
| nozzle_sst | 2x2 | native | original | probe_metric | original | -12 | rejected | False | 546 | 5.656 | 6.100 |
| nozzle_sst | 2x2 | segregated | original | probe_metric | original | -12 | iteration_cap | False | 3000 | 289.357 | 289.778 |
| periodic_vki | 1x1 | coupled | cold | probe_metric | original | -12 | rejected | False | 235 | — | 301.042 |
| periodic_vki | 1x1 | native | cold | probe_metric | original | -12 | iteration_cap | False | 3000 | 111.915 | 112.478 |
| periodic_vki | 1x1 | segregated | cold | probe_metric | original | -12 | rejected | False | 436 | — | 301.073 |
| periodic_vki | 2x2 | coupled | cold | probe_metric | original | -12 | rejected | False | 448 | — | 301.059 |
| periodic_vki | 2x2 | native | cold | probe_metric | original | -12 | iteration_cap | False | 3000 | 65.549 | 66.163 |
| periodic_vki | 2x2 | segregated | cold | probe_metric | original | -12 | rejected | False | 911 | — | 301.061 |
| flatplate_lm | 1x1 | coupled | original | recovery_release | original | -11 | running | False | — | — | — |
| flatplate_lm | 1x1 | native | original | recovery_release | original | -11 | rejected | False | 710 | — | 301.052 |
| flatplate_lm | 1x1 | segregated | original | recovery_release | original | -11 | timeout | False | 20 | — | 301.050 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 41.410 | 41.998 |
| naca_sa | 1x1 | native | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 1.378 | 1.907 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 9.339 | 9.848 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -12 | rejected | False | 11 | 16.219 | 16.788 |
| naca_sa | 2x2 | native | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 0.929 | 1.423 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -12 | reference_audit_failed | False | 11 | 5.350 | 5.869 |
| naca_sst | 1x1 | coupled | original | recovery_release | original | -12 | rejected | False | 216 | — | 301.041 |
| naca_sst | 1x1 | native | original | recovery_release | original | -12 | qualified | True | 258 | 36.407 | 36.936 |
| naca_sst | 1x1 | segregated | original | recovery_release | original | -12 | timeout | False | 52 | — | 301.053 |
| naca_sst | 2x2 | coupled | original | recovery_release | original | -12 | rejected | False | 445 | — | 301.036 |
| naca_sst | 2x2 | native | original | recovery_release | original | -12 | qualified | True | 258 | 27.401 | 28.073 |
| naca_sst | 2x2 | segregated | original | recovery_release | original | -12 | timeout | False | 84 | — | 301.048 |
| nozzle_sst | 1x1 | coupled | original | recovery_release | original | -12 | rejected | False | 1183 | — | 301.035 |
| nozzle_sst | 1x1 | native | original | recovery_release | original | -12 | rejected | False | 548 | 11.236 | 11.734 |
| nozzle_sst | 1x1 | segregated | original | recovery_release | original | -12 | rejected | False | 537 | — | 301.088 |
| nozzle_sst | 2x2 | coupled | original | recovery_release | original | -12 | rejected | False | 2142 | — | 301.024 |
| nozzle_sst | 2x2 | native | original | recovery_release | original | -12 | rejected | False | 546 | 5.792 | 6.197 |
| nozzle_sst | 2x2 | segregated | original | recovery_release | original | -12 | rejected | False | 2438 | — | 301.039 |
| periodic_vki | 1x1 | coupled | cold | recovery_release | original | -12 | rejected | False | 235 | — | 301.074 |
| periodic_vki | 1x1 | native | cold | recovery_release | original | -12 | iteration_cap | False | 3000 | 121.860 | 122.402 |
| periodic_vki | 1x1 | segregated | cold | recovery_release | original | -12 | rejected | False | 452 | — | 301.023 |
| periodic_vki | 2x2 | coupled | cold | recovery_release | original | -12 | rejected | False | 479 | — | 301.031 |
| periodic_vki | 2x2 | native | cold | recovery_release | original | -12 | iteration_cap | False | 3000 | 67.211 | 67.698 |
| periodic_vki | 2x2 | segregated | cold | recovery_release | original | -12 | rejected | False | 1106 | — | 301.052 |
| naca_sa | 1x1 | coupled | original | recovery_release | original | -13.0 | timeout | False | 29 | — | 121.027 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.319 | 19.860 |
| naca_sa | 1x1 | segregated | original | recovery_release | original | -13.0 | timeout | False | 65 | — | 121.032 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.857 | 19.360 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.616 | 20.191 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.952 | 19.476 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.144 | 19.646 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.769 | 19.259 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.861 | 19.394 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 19.029 | 19.568 |
| naca_sa | 1x1 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 18.743 | 19.302 |
| naca_sa | 2x2 | coupled | original | recovery_release | original | -13.0 | timeout | False | 84 | — | 121.046 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.894 | 13.346 |
| naca_sa | 2x2 | segregated | original | recovery_release | original | -13.0 | timeout | False | 66 | — | 121.080 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.122 | 13.658 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.084 | 13.597 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.632 | 13.143 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.597 | 13.229 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.219 | 13.754 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.390 | 13.904 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 12.795 | 13.242 |
| naca_sa | 2x2 | native | original | recovery_release | original | -13.0 | qualified | True | 151 | 13.170 | 13.704 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery | original | -12 | iteration_cap | False | 1 | 3.954 | 4.414 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery_checked | fd_step_1e-8 | -12 | diagnostic_complete | False | 20 | — | 19.253 |
| nozzle_sst | 1x1 | segregated | original | probe_recovery_checked | fd_step_1e-8 | -12 | diagnostic_complete | False | 1 | — | 4.708 |

## Original-control implementation comparison

Each row retains its convergence status. Assemblies are FLOW/TURB/TRANS;
Krylov work is outer/inner; R/B is retries/backtracks. Native assembly counts are
inferred from fine-grid updates and labelled in the CSV; Newton counts are measured.
Native linear work sums the reported iterations of every active system; it has no nested solves.
Timeout counters omit incomplete attempts and are lower bounds. Timings are contended
observations, never eligible time-to-convergence measurements. A saved timeout state
is only its last checkpoint. State errors use the native reference recorded for that row.

| Case | Layout | Implementation | Mode | Status | Updates | Assemblies F/T/X | Krylov outer/inner | R/B | Loop/E2E s | Max normalized RMS | Max point error |
|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| flatplate_lm | 1x1 | probe_metric | coupled | timeout | 50 | 1114/1241/1241 | 125/0 | 4/21 | —/301.067 | — | — |
| flatplate_lm | 1x1 | probe_metric | native | rejected | 695 | 695/695/695 | 10421/0 | —/— | —/301.077 | 4.520e-04 | — |
| flatplate_lm | 1x1 | probe_metric | segregated | timeout | 23 | 1581/251/257 | 477/0 | 2/17 | —/301.059 | — | — |
| flatplate_lm | 2x2 | probe_metric | coupled | rejected | 103 | 2096/2337/2337 | 239/0 | 4/21 | —/301.039 | 1.673e+00 | — |
| flatplate_lm | 2x2 | probe_metric | native | rejected | 1299 | 1299/1299/1299 | 19483/0 | —/— | —/301.040 | 8.428e-04 | — |
| flatplate_lm | 2x2 | probe_metric | segregated | timeout | 30 | 2961/322/356 | 773/0 | 2/17 | —/301.072 | — | — |
| naca_sa | 1x1 | probe_metric | coupled | rejected | 11 | 341/395/— | 51/0 | 3/0 | 7.754/8.336 | 1.196e-10 | 1.658e-05 |
| naca_sa | 1x1 | probe_metric | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 1.376/1.905 | 1.963e-10 | 0.000e+00 |
| naca_sa | 1x1 | probe_metric | segregated | reference_audit_failed | 11 | 444/429/— | 429/0 | 4/0 | 13.804/15.252 | 1.310e-11 | 2.946e-06 |
| naca_sa | 2x2 | probe_metric | coupled | rejected | 11 | 364/421/— | 53/0 | 3/0 | 4.042/4.537 | 9.982e-11 | 1.776e-05 |
| naca_sa | 2x2 | probe_metric | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 1.052/1.606 | 2.256e-10 | 0.000e+00 |
| naca_sa | 2x2 | probe_metric | segregated | reference_audit_failed | 11 | 465/372/— | 379/0 | 4/0 | 6.705/7.160 | 1.308e-11 | 3.215e-06 |
| naca_sst | 1x1 | probe_metric | coupled | rejected | 236 | 12482/13946/— | 1450/0 | 4/0 | —/301.066 | 1.625e-06 | — |
| naca_sst | 1x1 | probe_metric | native | qualified | 258 | 258/258/— | 10173/0 | —/— | 40.705/41.323 | 8.922e-12 | 0.000e+00 |
| naca_sst | 1x1 | probe_metric | segregated | rejected | 202 | 10793/4311/— | 4116/0 | 2/0 | —/301.034 | 1.567e-07 | — |
| naca_sst | 2x2 | probe_metric | coupled | rejected | 425 | 23363/26043/— | 2633/0 | 4/0 | —/301.044 | 1.660e-06 | — |
| naca_sst | 2x2 | probe_metric | native | qualified | 258 | 258/258/— | 10320/0 | —/— | 26.195/26.766 | 8.919e-12 | 0.000e+00 |
| naca_sst | 2x2 | probe_metric | segregated | rejected | 465 | 21517/12143/— | 8524/0 | 2/0 | —/301.071 | 2.970e-08 | — |
| nozzle_sst | 1x1 | probe_metric | coupled | timeout | 2139 | 31987/34739/— | 2746/0 | 8/0 | —/301.040 | — | — |
| nozzle_sst | 1x1 | probe_metric | native | rejected | 548 | 548/548/— | 6235/0 | —/— | 15.533/15.969 | 7.647e-09 | — |
| nozzle_sst | 1x1 | probe_metric | segregated | timeout | 1576 | 24309/28673/— | 19884/0 | 8/0 | —/301.032 | — | — |
| nozzle_sst | 2x2 | probe_metric | coupled | iteration_cap | 3000 | 43920/47505/— | 3578/0 | 8/0 | 183.077/183.560 | 7.859e-03 | — |
| nozzle_sst | 2x2 | probe_metric | native | rejected | 546 | 546/546/— | 6568/0 | —/— | 5.656/6.100 | 7.649e-09 | — |
| nozzle_sst | 2x2 | probe_metric | segregated | iteration_cap | 3000 | 44084/64886/— | 47505/0 | 8/0 | 289.357/289.778 | 7.863e-03 | — |
| periodic_vki | 1x1 | probe_metric | coupled | rejected | 235 | 10998/13792/— | 2794/0 | 0/0 | —/301.042 | 2.412e-05 | — |
| periodic_vki | 1x1 | probe_metric | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 111.915/112.478 | 2.419e-05 | — |
| periodic_vki | 1x1 | probe_metric | segregated | rejected | 436 | 12036/5055/— | 4263/0 | 1/0 | —/301.073 | 3.417e-05 | — |
| periodic_vki | 2x2 | probe_metric | coupled | rejected | 448 | 20542/25734/— | 5192/0 | 0/0 | —/301.059 | 7.479e-06 | — |
| periodic_vki | 2x2 | probe_metric | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 65.549/66.163 | 2.799e-05 | — |
| periodic_vki | 2x2 | probe_metric | segregated | rejected | 911 | 24411/11295/— | 9110/0 | 1/21 | —/301.061 | 4.110e-05 | — |
| flatplate_lm | 1x1 | recovery_release | coupled | running | — | —/—/— | —/— | —/— | —/— | — | — |
| flatplate_lm | 1x1 | recovery_release | native | rejected | 710 | 710/710/710 | 10646/0 | —/— | —/301.052 | 7.939e-04 | — |
| flatplate_lm | 1x1 | recovery_release | segregated | timeout | 20 | 1509/222/214 | 380/0 | 2/18 | —/301.050 | — | — |
| naca_sa | 1x1 | recovery_release | coupled | reference_audit_failed | 11 | 1629/2055/— | 423/0 | 0/0 | 41.410/41.998 | 1.334e-11 | 1.199e-06 |
| naca_sa | 1x1 | recovery_release | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 1.378/1.907 | 1.963e-10 | 0.000e+00 |
| naca_sa | 1x1 | recovery_release | segregated | reference_audit_failed | 11 | 170/427/— | 388/0 | 0/0 | 9.339/9.848 | 1.257e-11 | 2.255e-06 |
| naca_sa | 2x2 | recovery_release | coupled | rejected | 11 | 1262/1564/— | 301/0 | 1/0 | 16.219/16.788 | 3.006e-11 | 1.352e-05 |
| naca_sa | 2x2 | recovery_release | native | reference_audit_failed | 11 | 11/11/— | 440/0 | —/— | 0.929/1.423 | 2.256e-10 | 0.000e+00 |
| naca_sa | 2x2 | recovery_release | segregated | reference_audit_failed | 11 | 260/372/— | 363/0 | 0/0 | 5.350/5.869 | 1.253e-11 | 2.789e-06 |
| naca_sst | 1x1 | recovery_release | coupled | rejected | 216 | 12074/13496/— | 1415/0 | 4/0 | —/301.041 | 1.515e-06 | — |
| naca_sst | 1x1 | recovery_release | native | qualified | 258 | 258/258/— | 10173/0 | —/— | 36.407/36.936 | 8.922e-12 | 0.000e+00 |
| naca_sst | 1x1 | recovery_release | segregated | timeout | 52 | 11661/1698/— | 4050/0 | 0/0 | —/301.053 | — | — |
| naca_sst | 2x2 | recovery_release | coupled | rejected | 445 | 23585/26433/— | 2846/0 | 4/0 | —/301.036 | 1.398e-06 | — |
| naca_sst | 2x2 | recovery_release | native | qualified | 258 | 258/258/— | 10320/0 | —/— | 27.401/28.073 | 8.919e-12 | 0.000e+00 |
| naca_sst | 2x2 | recovery_release | segregated | timeout | 84 | 20828/2309/— | 7305/0 | 0/0 | —/301.048 | — | — |
| nozzle_sst | 1x1 | recovery_release | coupled | rejected | 1183 | 30261/33665/— | 3402/0 | 5/0 | —/301.035 | 3.538e-03 | — |
| nozzle_sst | 1x1 | recovery_release | native | rejected | 548 | 548/548/— | 6235/0 | —/— | 11.236/11.734 | 7.647e-09 | — |
| nozzle_sst | 1x1 | recovery_release | segregated | rejected | 537 | 33622/11133/— | 13529/0 | 3/3 | —/301.088 | 4.508e-06 | — |
| nozzle_sst | 2x2 | recovery_release | coupled | rejected | 2142 | 70006/77519/— | 7502/0 | 6/0 | —/301.024 | 2.932e-04 | — |
| nozzle_sst | 2x2 | recovery_release | native | rejected | 546 | 546/546/— | 6568/0 | —/— | 5.792/6.197 | 7.649e-09 | — |
| nozzle_sst | 2x2 | recovery_release | segregated | rejected | 2438 | 48375/59744/— | 49612/0 | 5/0 | —/301.039 | 4.445e-02 | — |
| periodic_vki | 1x1 | recovery_release | coupled | rejected | 235 | 10998/13792/— | 2794/0 | 0/0 | —/301.074 | 2.412e-05 | — |
| periodic_vki | 1x1 | recovery_release | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 121.860/122.402 | 2.419e-05 | — |
| periodic_vki | 1x1 | recovery_release | segregated | rejected | 452 | 12461/5263/— | 4442/0 | 1/0 | —/301.023 | 3.417e-05 | — |
| periodic_vki | 2x2 | recovery_release | coupled | rejected | 479 | 21720/27191/— | 5471/0 | 0/0 | —/301.031 | 7.479e-06 | — |
| periodic_vki | 2x2 | recovery_release | native | iteration_cap | 3000 | 3000/3000/— | 12000/0 | —/— | 67.211/67.698 | 2.799e-05 | — |
| periodic_vki | 2x2 | recovery_release | segregated | rejected | 1106 | 28888/13407/— | 10607/0 | 1/21 | —/301.052 | 4.884e-05 | — |

## Remaining work cost on final code

The first SA1x1 history-margin trial of each mode uses final numerical source d27f4b37f6,
the same supplied restart, CFL1000 and density history stop-13. The native endpoint qualifies;
both Newton trials hit120 seconds. All timings failed quiet eligibility.
The coupled prefix has zero retries/backtracking, yet assembles approximately160 flow and201
turbulence residuals per completed outer update. Its1161 Krylov iterations are authoritative
attempt telemetry; summing coupled per-system history fields would incorrectly double them.
This isolates a remaining product/preconditioning work cost even with every step accepted.
It does not establish an application time-to-convergence ratio. Krylov counts also operate
on different block widths; they are not equal-cost units between modes.

| Mode | Status | Updates | Flow assemblies | Turb assemblies | Krylov work | Retries | Backtracks |
|---|---|---:|---:|---:|---:|---:|---:|
| coupled | timeout | 29 | 4654 | 5836 | 1161 | 0 | 0 |
| native | qualified | 151 | 151 | 151 | 6040 | — | — |
| segregated | timeout | 65 | 4991 | 2586 | 3022 | 2 | 0 |

## Final-code CPU sampling

One10-second periodic VKI2x2 coupled window on final code produced3186 samples with no reported loss.
Self-cycle shares include live cached least-squares gradients15.26%, limiter calculation7.50%,
flow flux6.81%, turbulence edge flux4.16%, periodic projection4.56%, state restoration3.20%,
and ILU application2.89%. These are selected leaf symbols, not complete phase totals.
They describe CPU cycles, not wall-time fractions, blocked waiting, call counts or speedup.
The instrumented baseline phase tables remain separate. Sampling further disqualifies
the affected run from timing comparisons. Raw records, commands, PID identities and hashes
are under `release_sampling/1792294833149198230/`.
Optional inline debug lookup emitted DWARF section-size warnings; the separate no-inline
ELF-symbol report avoids those lookups. The initial report and raw data are preserved.
[Selected-symbol CSV](NewtonKrylovApplicationSampling20261018.csv).

The inspected point-wise OpenMP loops include implicit barriers, and vector dot products
start with a barrier. No missing barrier was identified at these inspected temporary-vector
boundaries; this limited audit does not establish the absence of every threading defect.

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
The isolated native output-location trial (`isolated_io/`) matches configuration, binary
and restart hashes of the workspace run, but also fails quiet eligibility. It proves no
storage or implementation timing gain. All files/links were verified before temporary cleanup.
Control plans/audits: `control_plan.json`, `control_setup_audit.json`; original108 versions
are retained with the `_original108` suffix. No physical or agreement target changed.
Final-code binary/source hashes: `recovery_release/manifest.json`; original-control commands:
`qualified_original_commands.json` / `qualified_recovery_commands.json`; controls: `control_commands.json`.
The control study uses `--graceful-limit`: at the nominal wall budget the harness signals
one owned solver rank through SU2's existing interruption handler, completing the current
outer update and saving restart, solver-loop time and rank RSS. A launcher timeout at
budget+60s remains the fallback. This changes failure capture; it changes no numerical target.
Six native/segregated/coupled checks on1x1/2x2 preserve final measurements and remain
unqualified despite exit0. A physically converged stored-state detector also rejects a
wall-limited exit. Deliberate two-second checks are labelled harness regressions, and
the synthetic detector is excluded from the trial ledger. Prior timeouts remain untouched.
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
