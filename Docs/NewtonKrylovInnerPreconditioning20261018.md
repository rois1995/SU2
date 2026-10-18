# Native inner preconditioning — 18 October 2026

The three-iteration native inner-preconditioning candidate is **under application
assessment, not adopted**. Its purpose is to replace expensive verified residual
products with cheaper assembled-matrix work. No residual, derivative verification,
true-defect, acceptance or endpoint agreement tolerance changes. This is a separate
change in preconditioning strategy, rather than a claim about removing wrapper overhead.

The existing optional inner solve first required a correctness fix. An independent
identity-matrix test fails with the unchanged implementation: three inner iterations
return an incorrect flow correction at unit scale and zero corrections at physical
scales 1e-32 and 1e-200. The unchanged one-pass preconditioner passes the same cases.
The native solver's absolute zero test and squared norms depend on physical units;
an exact first-direction orthogonalization breakdown can also return zero completed
iterations without applying its correction.

The local candidate normalizes each block's physical right-hand side by its global
maximum magnitude before the optional native solve and rescales the result afterward.
It handles zero and nonfinite input collectively. If the native solver completes
zero iterations, it applies the original one-pass inverse. The generic native
linear solver is unchanged. Normalization adds one maximum reduction per block
application; applications must establish whether stronger preconditioning pays for it.

| Matched SST original-control prefix, 1×1 | Retained d27 | Candidate |
|---|---:|---:|
| Accepted updates | 20 | 20 |
| Flow residual assemblies | 3382 | 2275 |
| Turbulence residual assemblies | 3818 | 2538 |
| Outer Krylov iterations | 430 | 258 |
| Native inner iterations | 0 | 890 |
| Retries | 3 | 3 |
| Backtracks | 0 | 0 |
| Solver-loop seconds | 82.086 | 55.908 |

These are unconverged prefixes with identical configuration hashes. Both timing
records are contended and ineligible; they establish reduced residual/Krylov work,
not a convergence-time speedup. Newton counters are measured cumulative controller
telemetry; native residual counts in the companion CSV are inferred from its
single fine-grid implicit update and labelled in the complete application ledger.

All 13 candidate regression jobs pass: the focused scaling/mass/probe tests on
1×1, 1×4, 2×2 and 2×4; all 62 native equation cases on 1×1 and 2×2; 91 supported
ordinary cases; and species-boundary, residual-failure and turbo-support CLI checks
on 1×1 and 2×2. The scaling test also checks zero input and a nonfinite input on
one MPI rank. One earlier CLI setup failed before executing the solver because
the guard and CLI used the same output directory; that failure is preserved and
the corrected retry passes. No timing assertions were added.

Native SA produces byte-identical restart files to retained d27 on both layouts
(151 updates). The complete stricter SA 1×1 candidate qualifies against both the
paired 298-update native state and the canonical 151-update state. Their maximum
pointwise errors are 3.23e-7 and 7.89e-6, below the unchanged 1e-5 limit. The maximum
normalized active-equation RMS is 3.14e-13. All 166 accepted steps use alpha=1,
pass Armijo and have true defects at most 0.08455; every recorded rollback error
is zero. Both compared configurations have SHA256
`2d1d9d892adc1a5c3551c60ece5c0f11d8db4ca11c92acc72bf65664fe425b65`.

| Qualified SA, identical diagonal/adaptive CFL/forcing controls, 1×1 | Retained d27 | Candidate |
|---|---:|---:|
| Accepted updates | 179 | 166 |
| Flow residual assemblies | 33415 | 24574 |
| Turbulence residual assemblies | 33415 | 24574 |
| Outer Krylov iterations | 7603 | 5359 |
| Native inner iterations | 0 | 31177 |
| Retries | 89 | 82 |
| Backtracks | 0 | 0 |
| Solver-loop seconds | 764.796 | 556.675 |
| End-to-end seconds | 765.385 | 557.225 |
| Peak rank RSS, KiB | 171400 | 138708 |

This qualifies a 26.5% assembly reduction and 29.5% outer Krylov reduction on this
application, at the cost of the listed assembled inner work. Both timings are
contended and ineligible. There is no repeated eligible median or general speedup
claim. `inner_candidate_sa_1x1_assessment.json` pins the compared results and verifies
all acceptance checks. The complete stricter SA 2×2 candidate is still running.
Broader SST, transition and periodic application assessment remains necessary
before changing the common default.

The [CSV](NewtonKrylovInnerPreconditioning20261018.csv) records work, timing,
memory, physical checks and hashes, including in-progress states. All paths below
are relative to the existing `application_qualification_20261018` evidence directory:

- `inner_scaling_detector/low_priority_result.json`: unchanged-code failing detector,
  binary/source hashes and exact command. This tiny correctness test ran at nice19;
  it is not performance evidence.
- `inner_candidate/manifest.json` and `source.patch`: immutable candidate source,
  build command and all three binary hashes. CFD SHA256:
  `9af5d6a785202e623bfa05cfd613c46f38061600909f27bb85e895f6dd20e2f2`.
- `inner_candidate_checks/commands.json`: all 14 attempts, including the preserved
  harness failure and 13 successful distinct jobs.
- `inner_candidate_application_commands.json`: exact application commands.
- `coarser_applications/sst_before20/1x1/coupled` and
  `inner_candidate_applications/sst_original20/1x1/coupled`: matched prefix evidence.

The previous debugger-only inner-preconditioning prototype is retained separately
under `inner_preconditioner_probe`; it is not substituted for compiled-candidate
results. All application jobs remain serially scheduled with host/RAM monitoring.
