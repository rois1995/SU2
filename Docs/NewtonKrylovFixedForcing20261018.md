# Existing fixed forcing 0.05 — application qualification

This study changes existing configuration controls without introducing solver code. Newton uses nominal linear
target 0.05 with adaptive forcing disabled. Existing CFL-retry tightening remains active; accepted
targets can therefore be lower. The true-defect limit remains 0.1. Derivative verification, acceptance,
physical residuals, full-state and quantity requirements are unchanged. This does not adopt the earlier
physical-merit forcing or bounded-restart experiments.

The first coupled SA 2x2 run qualifies after 356 updates: 57365 assemblies per system and 13375 Krylov
iterations, compared with 83590 and 22279 in the earlier qualified 333-update adaptive-forcing run.
That is 31.4% less assembly work and 40.0% less Krylov work, with 6.9% more updates. A fresh comparison
on the same final binary remains required for strict implementation/configuration attribution.
These contended times are observations; no timing speedup is established.

All 178 retries are derivative-verification failures. Accepted Krylov work falls from 20297 to 11427;
failed-attempt work is similar (1982 versus 1948). Thus most of the saving is in accepted solves.
The largest completed solve is 39 iterations, versus 80 before. Peak rank RSS is 79452 versus
103304 KiB; retained Krylov storage also depends on the largest solve. No separate memory-allocation
implementation change is inferred from this configuration comparison.

The qualified endpoint has density RMS 9.883e-15 and maximum active normalized RMS 3.347e-13.
Against the canonical native state, component error is 1.231e-7 and point error 7.811e-6, within
the unchanged 1e-6 and 1e-5 limits. All accepted steps use alpha=1; rollback errors are zero.
Native accuracy 0.05 now qualifies on both layouts with the final gradient-reuse binary:
158 updates and 1030 reported linear iterations on 1x1; 151 updates and 4209 on 2x2.
Both pass the canonical native state audit and every frozen physical and quantity requirement.
The original native tolerance 1e-10 uses 151 updates and 6040 reported linear iterations.
Serial Newton with fixed forcing and segregated qualification remain outstanding.

The [CSV](NewtonKrylovFixedForcing20261018.csv) retains configurations, source/build/binary hashes,
per-system physical checks, quantities, memory, failed work and exact evidence paths. Native assemblies
are inferred fine-grid updates; Newton assembly counters are measured. Native linear work sums separate
equation solves and is not a like-for-like Krylov-vector width comparison with a coupled solve.

| Case | Layout | Mode | Nominal linear target | Adaptive forcing | Updates | FLOW/TURB | Linear work | Retries | Loop / elapsed s | Peak rank KiB | Status | Qualified | Evidence |
|---|---|---|---:|---|---:|---:|---:|---:|---:|---:|---|---|---|
| naca_sa | 2x2 | coupled | 0.05 | NO | 356 | 57365/57365 | 13375 | 178 | 1797.690 / 1798.629 | 79452 | qualified | True | fixed_forcing_005_applications/naca_sa/2x2/coupled |
| naca_sa | 1x1 | native | 0.05 | NO | 158 | 158/158 | 1030 |  | 18.092 / 18.840 | 78096 | qualified | True | fixed_forcing_005_matched/naca_sa/1x1/native_accuracy/native |
| naca_sa | 2x2 | coupled | 0.1 | YES |  | / |  |  | — / — |  | running | False | fixed_forcing_005_matched/naca_sa/2x2/adaptive/coupled |
| naca_sa | 2x2 | native | 0.05 | NO | 151 | 151/151 | 4209 |  | 51.890 / 52.558 | 60404 | qualified | True | fixed_forcing_005_matched/naca_sa/2x2/native_accuracy/native |
| naca_sa | 2x2 | coupled | 0.1 | YES | 333 | 83590/83590 | 22279 | 166 | 1090.736 / 1091.372 | 103304 | qualified | True | naca_work_endpoints/naca_sa/2x2/coupled_default0 |
| naca_sa | 1x1 | coupled | 0.1 | YES | 179 | 33415/33415 | 7603 | 89 | 764.796 / 765.385 | 171400 | qualified | True | sa_stricter_controls/1x1/diagonal/coupled |
| naca_sa | 1x1 | native | 1E-10 | NO | 151 | 151/151 | 6040 |  | 18.889 / 19.440 | 95628 | qualified | True | static_review_correctness_v2_applications/naca_sa/1x1/native |
| naca_sa | 2x2 | native | 1E-10 | NO | 151 | 151/151 | 6040 |  | 13.307 / 13.824 | 61952 | qualified | True | static_review_correctness_v2_applications/naca_sa/2x2/native |

This table includes earlier references as labelled by their source/binary hashes. Fresh same-binary
qualification, both layouts and the remaining application matrix are still required. NEMO is deferred.
