# Newton static review verification — 18 October 2026

Assessed review revision: `e4fb1967982cfc007ab678a35a6e1a2d2978ad5b711cff2580b8ef07b1555c8e`, including the updated sections 5b–5d. The supplied review remains unchanged. Original revisions, failed detectors and commands are preserved under `newton_krylov_refactor_20261015/application_qualification_20261018/static_review_20261018/` in the workspace parent.

The review identifies real correctness hazards. Several proposed performance changes would alter existing numerical contracts and were not adopted. Residual/state/quantity targets, derivative verification, the configured true-defect limit and acceptance rules remain unchanged. Common inner preconditioning retains its existing default of zero iterations. The forcing signal is unchanged.

## Verified fixes

| Finding | Verification and implemented change |
|---|---|
| H2 | The new factory detector failed six assertions before the fix. `NEWTON_KRYLOV=YES` alone again selects the legacy integration; `NEWTON_EQUATIONS` explicitly selects the common controller. Pushed as `7e9337d9921d5bebeac1654a634648acf417de62`. |
| ES1: fixed CL | Repeated residual/restoration calls changed AoA from 0 to 0.6, 1.0 and 1.4 degrees with unchanged primary state. Trial preprocessing now freezes this feedback; the FLOW owner applies the pending update once before capturing its base. FLOW-only, TURB-only and coupled tests pass on both layouts. |
| ES1: actuator disk | Source tracing confirms that repeated preprocessing can increment `BCThrust_Counter` and overwrite stored thrust. Common equation groups now reject this unsupported feedback path. Separating its controller history from live boundary calculations remains necessary before support can be claimed. |
| LP7 | Forced periodic-buffer growth hung the old 2×2 projector until its 20-second timeout. Both equation-set paths now allocate collectively before entering the master region. Forced-growth tests pass on both layouts. |
| LP6 | Explicit periodic inverse checks missed by 9.75e-5 in 2D and 5.85e-4 in 3D. The explicit block action and inverse now use the same partial-cell volume as common pseudo-time. Halo output remains scratch consumed through the established communication/projection paths. |
| Additional MPI issue | With `COMM_LEVEL=MINIMAL`, the scaled residual can be small while rank-local raw RMS values straddle the resolved threshold. The old parallel detector hangs. Resolution now requires collective agreement. The test permits legitimate stagnation while forbidding an unresolved group from idling on a subset of ranks. |
| H3 | Relaxed direction targets are capped by the unchanged true-defect limit. Automatic reconstruction uses the actual adaptive target, retaining the native CFL reference where configured. A factor-four relaxation test verifies the cap. |
| LP3 | Native block-diagonal startup omits matrix-free lower cross products. A focused test verifies that its block solve performs no residual assemblies. |
| LP8 | Lower-coupling failures retain their first category and count, so a subsequent NaN direction cannot hide the original cause. Tests cover collective failure and counter reset. |
| L1, ES4, LP4 | Reused the existing bit-level finite check for resolved tolerances, RMS accumulators and Krylov orthogonalization. A compiler experiment confirms that standard checks work with the current `-fno-finite-math-only`, whereas unrestricted `-ffast-math` removes them. NaN/Inf products preserve the preceding valid basis. This is not evidence of false convergence in the measured applications. |
| ES3 | Live compressible temperature, pressure and sound speed are checked for every supported fluid model. A nonphysical van der Waals trial is rejected and restored exactly. The suspected pre-fix acceptance of a bad real-gas state was not reproduced. |
| L2 | A valid physical configuration with zero FD step reproduces the old uncaught setup exception. Driver setup failures now use `SU2_MPI::Error`; direct library validation still throws, preserving its existing tests. |
| L3 | The pseudo-time failure path establishes an explicit reference before checking explicit rollback merit. The rare original false-abort scenario was not reproduced; rollback checks remain active. |
| L4, ES7 | Shared validation messages use a generic Newton prefix. The restoration comment distinguishes primary/history restoration from relaxation restoration. |

## Contracts and remaining costs

| Finding | Assessment |
|---|---|
| H1 | A tiny step or apparent noise floor does not establish that requested physical/state targets are satisfied. Existing resolved-group exits remain; stagnation is not converted into zero-step success. Cauchy-only or unattainable residual requests can still terminate in failure. |
| H4, M2 | The full-operator defect check is intentional for common solves, including startup and relaxed direction models. Small budgets can fail it. H2 restores legacy defaults without weakening this opt-in check. |
| M1 | Recovery can permanently reduce CFL with adaptation disabled. The SST evidence exhibits this cost. Automatically regrowing CFL would change the requested controls and requires a separately qualified configuration comparison. |
| P1 | Physical-merit forcing was already tested and not adopted: the full applications did not qualify and recovery increased. The cited qualified SA runs do not show collapse to etaMin. |
| P2, LP1, LP5 | Verification costs multiple residual evaluations. Probe size and feasibility depend on direction; verifying one direction does not qualify every later Krylov direction. Per-product and independent defect/slope verification remain active. |
| LP2 | Refinement can explore an expensive failing sequence. Coarser-probe experiments already exist and did not establish a qualified application improvement. No monotonic-error assumption or weaker derivative test was introduced. |
| ES2, P3, P6 | Repeated gradient/property/boundary refreshes and explicit base preparation remain candidates. Explicit/implicit paths can differ at round-off; live SST TKE changes flow thermodynamics during auxiliary probes. Static counts alone do not justify removing these operations. |
| P4 | Qualification retains diagnostics and restart/failure evidence. Reopening files remains a cost; existing configuration can disable diagnostics. |
| P5 | Reusing the accepted trial's physical merit is a local opportunity. Reduction fusion requires its own implementation measurement; its estimated gain is not a measured application speedup. |
| ES5 | Native periodic projection is an intentional integrated correction with dedicated tests and converged evidence in `NewtonKrylovPeriodicBaselineResults.json`. Historical short-iteration reference differences remain documented; references were not rewritten. |
| ES6 | Requested residuals are outputs; dependency residual buffers can be scratch. Outer reporting recomputes active residuals before reading them. No current intervening reader was found. |
| ES8, LP9 | Shared evaluation scratch and lazy Krylov allocation already exist. Accepted bases/history remain independent. Retained basis capacity and per-owner vectors still cost memory. |
| Section 4 | Registry/layout limitations are real extension constraints. This review does not add radiation, NEMO, multizone, unsteady or general new-equation support. |

## Measured correction to sections 5b–5d

| Qualified coupled SA measurement | 1×1 | 2×2 |
|---|---:|---:|
| Updates | 179 | 333 |
| Retries / update | 89 / 179 = 0.4972 | 166 / 333 = 0.4985 |
| Accepted-step Krylov work | 7603 | 20297 |
| Failed-attempt Krylov work | 0 | 1982 |
| Median accepted linear target | 9.25149e-4 | 1.002189e-3 |
| Minimum accepted linear target | 4.67403e-5 | 8.16909e-5 |
| Accepted targets at etaMin=1e-5 | 0 | 0 |
| Accepted solves reaching 80 iterations | 0 | 72 / 333 |

The quoted 67 Krylov iterations/update includes failed attempts; the accepted-step parallel mean is 60.95. Rejected intervals consume 12,915 of 83,590 FLOW assemblies (15.45%). Most assembly work belongs to accepted updates. The absolute retry count does not show a higher retry rate in parallel. This case uses diagonal blocks, so extra lower-coupling products do not explain its cost.

| New recovery finding | Verification |
|---|---|
| R1 | Recovery currently shrinks CFL regardless of cause. However, keeping the partial solution and testing its true defect already happens. All 166 rejected SA partial solutions have finite true defects between 0.306 and 1.0; 165 completed 1–14 Krylov iterations. Keeping CFL alone cannot make those directions pass the unchanged gate. Verification is normalized against the full pseudo-time operator, so changing CFL also changes conditioning and the relative verification scale. |
| R2 | Tightening on retry is real; independent, constant per-product failure probability is an unverified model. In fixed-control SST, accepted FLOW median K fell from 80 to 35 to 15 as CFL fell from 1000 to 250 to 62.5. Median accepted target remained 0.001. These observations do not demonstrate the proposed positive failure feedback. |
| R3 | Reassembly/refactorization on CFL retry is real; operator masses have changed. Krylov storage is retained, and the partial solution is checked before rejection. The measured rejected work is 15.45% of assemblies, not evidence that every retry costs an average full accepted step. Reusing an old operator or basis action requires accounting for the changed mass. |
| R4 | An insufficient linear solve can limit CFL, but none of the cited SA rejected solves reached the 80-iteration budget: all stopped within 14 iterations. Raising the budget alone does not address the unavailable products. Loosening the true-defect limit conflicts with the frozen qualification contract. |
| R5 | Coupled groups share CFL by design; segregated controllers retain separate CFL state. Legacy startup reset and disabled adaptation remain explicit policy costs. No new growth/reset policy is inferred from these observations. |
| R6 | Bounded terminal failure is intentional. Any native recovery direction would still need independent verification, admissibility and the same acceptance checks. An unchecked fallback is not a valid performance fix. |

Section 5c's four-system counts are upper bounds on API calls: NACA SA/SST have two active systems. API calls, actual messages, collective payload and transport bytes must be distinguished. Static estimates do not establish a 10× improvement or large-rank scaling.

## Validation

All eleven fresh jobs pass: 69 native integration cases on each of 1×1 and 2×2, 92 selected ordinary cases, plus species boundaries, residual failure/restart, turbomachinery guards and setup/capability failures on each layout. Commands, hashes, memory observations and logs are preserved in `static_review_correctness_v2_checks/` and `static_review_correctness_v2_release/`.

Initial test setup failures are preserved: unsupported real-gas far-field boundaries, missing Reynolds number, and missing capture/timestep preparation in the reduced-communication detector. Its final assertions check collective behavior and rollback, rather than requiring an artificial tiny source to converge.

The application pairs below retain identical configuration hashes, work counts, restart bytes and all original Newton telemetry columns. Native endpoints pass frozen all-equation and stable-reference checks. Prefixes are supporting compatibility checks and remain unqualified as endpoints. Native assembly counts are inferred from fine-grid implicit updates; Newton counters are measured. Observed times are work-only and timing-ineligible.

| Case | Layout | Mode | Controls | Updates | FLOW/TURB | Outer/inner work | Observed solver seconds: before → after | Role |
|---|---|---|---|---:|---:|---:|---:|---|
| SA | 1x1 | native | original | 151 | 151/151 | 6040/0 | 21.418 → 18.889 | qualified endpoint |
| SA | 2x2 | native | original | 151 | 151/151 | 6040/0 | 16.050 → 13.307 | qualified endpoint |
| SST | 1x1 | native | original | 397 | 397/397 | 15164/0 | 60.211 → 65.074 | qualified endpoint |
| SST | 2x2 | native | original | 397 | 397/397 | 15880/0 | 43.669 → 47.730 | qualified endpoint |
| SA | 1x1 | coupled | diagonal | 20 | 3681/3681 | 802/0 | 85.491 → 84.779 | 20-update prefix |
| SST | 1x1 | segregated | original | 20 | 3952/336 | 1226/0 | 102.571 → 103.107 | 20-update prefix |
| SST | 1x1 | coupled | original | 20 | 3382/3818 | 430/0 | 93.144 → 92.129 | 20-update prefix |
| SA | 2x2 | coupled | diagonal | 20 | 5464/5464 | 1482/0 | 74.231 → 69.982 | 20-update prefix |
| SST | 2x2 | segregated | original | 20 | 3946/339 | 1234/0 | 59.187 → 51.690 | 20-update prefix |
| SST | 2x2 | coupled | original | 20 | 3521/4022 | 500/0 | 50.954 → 46.646 | 20-update prefix |

The accompanying CSV retains before/after timings, peak memory, residual/state/quantity agreement and source/build/configuration hashes. These correctness fixes do not demonstrate a general application speedup.

Legacy ramp and ONERA registered short-history checks still miss their existing tolerances on this branch and archived controls; references remain unchanged. Restored ONERA histories exactly match the pre-migration archive across numerical columns. Ramp differences are confined to residual-history columns; force/linear columns agree. An archived ramp replay stalled in visualization MPI I/O after reaching its iteration limit. Its stack and stopped process identities are preserved; visualization-disabled replays isolate numerical work. This is not a time-to-convergence result.

The preceding SST FLOW3/scalars0 run reached its 3600-second cap: 500 updates, FLOW/TURB 51362/13686, outer/inner work 18170/36294, two derivative-verification retries and no backtracking. State agreement failed. Default0 likewise remains incomplete: 1254 updates, 133911/24743 assemblies, 41283 outer iterations and two derivative retries. Unequal contended caps cannot establish a convergence-policy improvement.
