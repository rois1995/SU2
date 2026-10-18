# Existing fixed forcing 0.05 — application qualification

This study changes existing configuration controls without introducing solver code. Newton uses nominal linear
target 0.05 with adaptive forcing disabled. Existing CFL-retry tightening remains active; accepted
targets can therefore be lower. The true-defect limit remains 0.1. Derivative verification, acceptance,
physical residuals, full-state and quantity requirements are unchanged. This does not adopt the earlier
physical-merit forcing or bounded-restart experiments.

The same-final-binary coupled SA 2x2 pair now qualifies in both configurations. Fixed forcing takes
356 updates, 57365 assemblies per system and 13375 Krylov iterations; adaptive forcing takes
333 updates, 83590 assemblies per system and 22279 Krylov iterations. This is 31.4% less assembly
work and 40.0% less Krylov work, with 6.9% more updates. Exact configuration comparison finds only
NEWTON_LINEAR_TOL and NEWTON_ADAPTIVE_FORCING changed. This is an existing-control result.
Both runs reproduce their archived earlier-build restart bytes and every attempt column.
Observed loop times are 3378.044 and 1815.426 seconds; both are contended and timing-ineligible.

All 178 retries are derivative-verification failures. Accepted Krylov work falls from 20297 to 11427;
failed-attempt work is similar (1982 versus 1948). Thus most of the saving is in accepted solves.
Rejected log intervals consume 12915 versus 13274 assemblies per system; this work does not decrease.
These intervals include bases, rollback and intervening reporting; they are not isolated failure kernels.
The largest completed solve is 39 iterations, versus 80 before. Peak rank RSS is 78696 versus
103428 KiB; retained Krylov storage also depends on the largest solve. No separate memory-allocation
implementation change is inferred from this configuration comparison.

Nominal forcing 0.05 retains retry tightening: 179 accepted targets are 0.05, 176 are 0.0125 and
one is 0.003125. Maximum accepted true defect is 0.050429, below the unchanged 0.1 bound.
At least 40125 of the 57365 assemblies per system are required by three residual probes per
completed verified MUSCL product on this trajectory. This is a source-derived lower bound, not
a measured phase count or wall-time fraction. The qualified native 2x2 accuracy-0.05 run uses
151 inferred assemblies per system. Matrix-free product work remains a substantial limitation.

The qualified endpoint has density RMS 9.883e-15 and maximum active normalized RMS 3.347e-13.
Against the canonical native state, component error is 1.231e-7 and point error 7.811e-6, within
the unchanged 1e-6 and 1e-5 limits. All accepted steps use alpha=1; rollback errors are zero.
Native accuracy 0.05 now qualifies on both layouts with the final gradient-reuse binary:
158 updates and 1030 reported linear iterations on 1x1; 151 updates and 4209 on 2x2.
Both pass the canonical native state audit and every frozen physical and quantity requirement.
The original native tolerance 1e-10 uses 151 updates and 6040 reported linear iterations.
The serial fixed-forcing trial is incomplete and misses pointwise state agreement; segregated
qualification remains outstanding.

The final-build serial adaptive baseline now qualifies in 179 updates, with 33415 assemblies per
system and 7603 Krylov iterations. Its restart bytes and all 30 original diagnostic columns match
the earlier serial baseline. All 89 retries are now directly classified as derivative-verification
failures and complete zero Krylov iterations.

Fixed nominal 0.05 does not generalize to this serial trajectory. After 564 updates it has used
40134 assemblies per system and 2653 completed Krylov iterations, with 284 derivative-verification
retries and no backtracking. Accepted CFLs fall mainly to 31.25-125. Fresh residuals pass, but the
canonical point error 4.822e-5 exceeds 1e-5. It was stopped gracefully after exceeding the qualified
adaptive baseline assembly budget 33415: the request observed 40032 and the final count is 40134.
This is incomplete_work_cap, not a convergence result. Its observed loop time 1648.623 seconds
is resource-use time. Lower completed linear work and observed RSS do not qualify an efficiency gain.
The fixed-forcing queue stopped for assessment; its segregated jobs were not launched.
The subsequent original-control/CFL-only segregated study is tracked in
[the NACA report](NewtonKrylovNacaRejections20261018.md).

A 30-second, 99-Hz CPU-cycle sample of the live serial adaptive tail loses no samples. Flat symbol shares
include cached least-squares gradients 19.74%, Roe flux 10.22% and limiters 6.87%. Inlined work is
attributed to its out-of-line symbol. This window combines necessary live refreshes and any repeated
work; it does not separate them or measure phase wall times. Numerical columns remain unchanged.

The frozen physical auditor and final-build reporter produce exactly identical reports for the
qualified native SA 1x1 state, the serial adaptive endpoint and both coupled SA 2x2 endpoints. A separate immutable
intermediate Newton checkpoint passes all frozen residual, state and quantity checks. It has no
iteration metadata and is supporting evidence only; no convergence work or time is assigned to it.

The [CSV](NewtonKrylovFixedForcing20261018.csv) retains configurations, source/build/binary hashes,
per-system physical checks, quantities, memory, failed work and exact evidence paths. Native assemblies
are inferred fine-grid updates; Newton assembly counters are measured. Native linear work sums separate
equation solves and is not a like-for-like Krylov-vector width comparison with a coupled solve.
Completed Newton totals include the final outer-reporting assembly after the last logged attempt.
Earlier raw records mislabeled this offset as an initial evaluation. Counts are unchanged; measured
method entries confirm the offset in 20 archived application profiles. Interrupted totals are estimates.

| Case | Layout | Mode | Nominal linear target | Adaptive forcing | Updates | FLOW/TURB | Linear work | Retries | Loop / elapsed s | Peak rank KiB | Status | Qualified | Evidence |
|---|---|---|---:|---|---:|---:|---:|---:|---:|---:|---|---|---|
| naca_sa | 2x2 | coupled | 0.05 | NO | 356 | 57365/57365 | 13375 | 178 | 1797.690 / 1798.629 | 79452 | qualified | True | fixed_forcing_005_applications/naca_sa/2x2/coupled |
| naca_sa | 1x1 | coupled | 0.1 | YES | 179 | 33415/33415 | 7603 | 89 | 1489.580 / 1490.316 | 170972 | qualified | True | fixed_forcing_005_matched/naca_sa/1x1/adaptive/coupled |
| naca_sa | 1x1 | coupled | 0.05 | NO | 564 | 40134/40134 | 2653 | 284 | 1648.623 / 1649.319 | 118584 | incomplete_work_cap | False | fixed_forcing_005_matched/naca_sa/1x1/fixed/coupled |
| naca_sa | 1x1 | native | 0.05 | NO | 158 | 158/158 | 1030 |  | 18.092 / 18.840 | 78096 | qualified | True | fixed_forcing_005_matched/naca_sa/1x1/native_accuracy/native |
| naca_sa | 2x2 | coupled | 0.1 | YES | 333 | 83590/83590 | 22279 | 166 | 3378.044 / 3378.847 | 103428 | qualified | True | fixed_forcing_005_matched/naca_sa/2x2/adaptive/coupled |
| naca_sa | 2x2 | coupled | 0.05 | NO | 356 | 57365/57365 | 13375 | 178 | 1815.426 / 1816.065 | 78696 | qualified | True | fixed_forcing_005_matched/naca_sa/2x2/fixed/coupled |
| naca_sa | 2x2 | native | 0.05 | NO | 151 | 151/151 | 4209 |  | 51.890 / 52.558 | 60404 | qualified | True | fixed_forcing_005_matched/naca_sa/2x2/native_accuracy/native |
| naca_sa | 2x2 | coupled | 0.1 | YES | 333 | 83590/83590 | 22279 | 166 | 1090.736 / 1091.372 | 103304 | qualified | True | naca_work_endpoints/naca_sa/2x2/coupled_default0 |
| naca_sa | 1x1 | coupled | 0.1 | YES | 179 | 33415/33415 | 7603 | 89 | 764.796 / 765.385 | 171400 | qualified | True | sa_stricter_controls/1x1/diagonal/coupled |
| naca_sa | 1x1 | native | 1E-10 | NO | 151 | 151/151 | 6040 |  | 18.889 / 19.440 | 95628 | qualified | True | static_review_correctness_v2_applications/naca_sa/1x1/native |
| naca_sa | 2x2 | native | 1E-10 | NO | 151 | 151/151 | 6040 |  | 13.307 / 13.824 | 61952 | qualified | True | static_review_correctness_v2_applications/naca_sa/2x2/native |

This table includes earlier references labelled by source/binary hashes. The same-binary 2x2 pair
is complete; the serial fixed-forcing attempt remains incomplete. Segregated comparisons and the
remaining application matrix are pending.
Default inner preconditioning remains zero. This result does not select a general forcing default.
NEMO is deferred.

Reproduce from the application evidence directory, using fresh output directories:

```sh
python3 fixed_forcing_005_case.py --case naca_sa --mode native --control original \
  --phase qualification --work-only --graceful-limit --history-margin 1 \
  --updates 10000 --timeout 1800 --ranks 2 --threads 2 \
  --release muscl_boundary_gradient_v2_release --output FRESH_NATIVE \
  --reference static_review_correctness_v2_applications/naca_sa/2x2/native

python3 fixed_forcing_005_case.py --case naca_sa --mode coupled --control diagonal \
  --phase qualification --work-only --graceful-limit --cfl 1000 --history-margin 2 \
  --updates 10000 --timeout 7200 --ranks 2 --threads 2 \
  --release muscl_boundary_gradient_v2_release --output FRESH_COUPLED \
  --reference static_review_correctness_v2_applications/naca_sa/2x2/native
```

Use `application_case.py` for the adaptive-forcing coupled baseline with the same other arguments.
Use ranks/threads 1/1 for serial. The timeout is a resource cap; hitting it leaves an incomplete result.
Exact commands and queue outcomes are in `fixed_forcing_005_20261018/matched_commands.json` and
`followup_commands.json`. Counter-boundary evidence is in `counter_offset_checks.json` and
`counter_label_checks.json` in that directory. Final-build verification is preserved in
`final_build_matched_assessment.json`, `final_build_adaptive_qualified_assessment.json` and
`final_build_{adaptive,fixed}_auditor_check.json`; the native check is `final_build_auditor_check.json`.
Serial proof: `serial_adaptive_qualified_assessment.json` and `final_build_serial_adaptive_auditor_check.json`.
The unsuccessful serial control is preserved in `serial_fixed_work_limited_assessment.json` and
`serial_fixed_work_limit_action.json`. The work-cap qualifier has a copied-state check: an identical
physically qualified state stays qualified without a cap and is incomplete with a cap. Results are
in `work_limit_selfchecks.json`; the original qualifier is archived before the metadata change.
Reproduce the copied-state guard check with `python3 check_work_limit.py`; it launches no solver.
The live profile is `release_sampling/1792353801499247196/` under the application evidence root.
Checkpoint checks are under `checkpoints/`.
Raw run evidence is preserved.
