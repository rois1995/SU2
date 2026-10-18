# Native inner preconditioning — 18 October 2026

The three-iteration native inner-preconditioning candidate is **not adopted as
the common default**. Its purpose is to replace expensive verified residual
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

The committed correctness fix normalizes each block's physical right-hand side by its global
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
Linear iteration counters count completed iterations. Incomplete attempts and a
zero-iteration inner-solve fallback still consume work; residual counters and CPU
profiles capture costs that these iteration totals alone cannot describe.

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
all acceptance checks. The stricter SA 2×2 candidate reaches its wall limit and
fails state agreement, as detailed below.
Remaining archived-candidate assessments continue to characterize this strategy.
The checkout has been restored byte-for-byte to the committed default of zero
inner iterations. Candidate source, patch, binaries and failed evidence are retained;
`inner_candidate_default_decision.json` records the decision and hashes.

The matched SA segregated trials with adaptive CFL/forcing and history stop -13
also remain unqualified at their 300-second limits: retained/candidate updates
are 126/122 on 1×1 and 142/258 on 2×2. Both candidate endpoints pass fresh residual
checks but fail full-state pointwise agreement. More updates do not substitute
for that requirement; complete work and failure-time rows remain in the CSV.

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

## SA 2×2 failure and work to density milestones

The candidate reaches its 1200-second limit after 574 updates, with 107971
assemblies per system, 26656 completed outer Krylov iterations, 159180 native
inner iterations, 288 retries and no backtracking. Peak rank RSS is 94432 KiB;
peak sampled owned-tree RSS is 248588 KiB. Its 1200.933-second loop and
1201.498-second elapsed time describe a failed stopping run. They are contended
and additionally ineligible because of CPU sampling.

An independent lightweight comparison of the saved endpoint fails both state
references: maximum point error 1.058365e-5 against canonical native151 and
1.815037e-5 against native298, versus the unchanged 1e-5 bound. A separate nice19
residual-only evaluation passes every physical residual target (density RMS
8.73191e-14, maximum normalized active RMS 3.15239e-12), zero boundary projection,
and physical quantities against both native states. All 574 accepted steps pass
Armijo with alpha=1 and true defects at most 0.08202; every rollback error is zero.
The endpoint still fails the full-state and stopping requirements. After the other
build finished, the primary qualifier independently rejected pointwise agreement
and produced a physical report identical to the low-priority report. Its output
was untouched. The one-second low-priority correctness evaluation uses a separate
output directory and provides no performance measurement. Evidence:
`inner_candidate_sa_2x2_state_assessment.json`,
`inner_candidate_sa_2x2_independent_assessment.json` and
`inner_candidate_sa_2x2_independent_report_command.json`.

The [milestone CSV](NewtonKrylovInnerMilestones20261018.csv) compares progress with
retained d27. These decks differ only in history stops -13/-14, which are inactive
through every milestone below. The exact other configuration text matches.
Counts are measured cumulative controller assemblies, excluding driver initialization.

| First density-history crossing | Updates, d27 / candidate | Flow assemblies, d27 / candidate | Outer Krylov, d27 / candidate |
|---|---:|---:|---:|
| -12.4 | 12 / 54 | 3113 / 9078 | 836 / 2109 |
| -12.5 | 33 / 134 | 9280 / 25102 | 2537 / 6028 |
| -12.6 | 53 / 214 | 15154 / 41084 | 4149 / 9968 |
| -12.7 | 73 / 293 | 21008 / 56512 | 5755 / 13826 |

Turbulence counts equal flow counts here. These are supporting trajectory
milestones, **not all-equation or full-state convergence**. They show that the
serial work reduction does not extend to this layout: reaching density history
-12.7 consumes about 2.69 times the assemblies, plus 82605 native inner iterations.
At that crossing the retained solver accepts CFL 1000 and the candidate CFL 250;
the candidate spends many accepted updates at CFL 125–250. This is evidence
against adopting three inner iterations as a universal default. The other
application assessments remain separately scheduled. No elapsed-time ratio is used.
`inner_candidate_sa_2x2_milestones.json` pins source configuration and telemetry hashes.

## SST with adaptive CFL and forcing

The complete 300-second serial SST trial still fails the fresh density requirement.
Its configuration exactly matches retained d27 (SHA256
`cb7511f199e2df65a9b23bd5e54f6240c415291940a66aba75963a8668f9ab5a`).
It uses the lower block preconditioner; this is a separate control setting from
the fixed-CFL original prefix above. All frozen endpoint requirements remain.

| SST adaptive controls, 1×1 | Retained d27 | Candidate |
|---|---:|---:|
| Updates before wall limit | 61 | 78 |
| Flow / turbulence assemblies | 12160 / 13641 | 12036 / 13549 |
| Completed outer Krylov | 1445 | 1486 |
| Completed native inner Krylov | 0 | 4379 |
| Retries / backtracks | 33 / 0 | 42 / 0 |
| Fresh density RMS | 2.543e-7 | 2.137e-7 |
| Solver-loop / elapsed seconds | 300.602 / 301.182 | 301.469 / 302.072 |
| Peak rank RSS, KiB | 164712 | 147768 |
| Full comparison | Fail | Fail |

More updates and a modestly smaller density residual at similar assembly work
do not establish lower cost to a qualified endpoint. Both timings are contended
and ineligible. This full application also shows why the one-third work reduction
in the original 20-update prefix is insufficient evidence for a general benefit.
The [focused CSV](NewtonKrylovInnerPreconditioning20261018.csv) automatically includes
all ten completed baselines and each new application as it runs. Exact paired
results are in `inner_candidate_completed_controls_assessment.json`; commands and
configuration checks are in `inner_candidate_control_commands.json`. LM and the
other application assessments continue before any default change.

## Active LM and substantial periodic applications

Both matched coupled T3A SST–LM trials reach their 600-second limits. LM remains
active: the candidate has 1555 sampled points with intermittency between 0.1 and
0.9 and 66317 above 0.9. The periodic VKI SA turbine uses matched cold starts,
existing ILU and assembled native limit 20; both coupled trials reach 300-second
limits. Neither application meets its fresh density target.

| Application | Inner iterations | Updates | Flow / turb / transition assemblies | Outer / native inner Krylov | Retries / backtracks | Fresh density RMS | Peak rank KiB |
|---|---:|---:|---:|---:|---:|---:|---:|
| T3A SST–LM | 0 | 121 | 2409 / 2594 / 2594 | 185 / 0 | 52 / 5 | 5.552e-6 | 387212 |
| T3A SST–LM | 3 | 158 | 2401 / 2578 / 2578 | 177 / 536 | 8 / 55 | 7.009e-6 | 416964 |
| Periodic VKI SA | 0 | 71 | 10211 / 13333 / — | 3122 / 0 | 8 / 7 | 4.980e-9 | 192440 |
| Periodic VKI SA | 3 | 59 | 8741 / 11397 / — | 2656 / 15932 | 11 / 3 | 3.246e-8 | 216964 |

Fewer LM retries and more updates do not establish better physical convergence:
its candidate endpoint has a larger density residual and higher peak memory.
Its 55 backtracks include 52 accepted partial steps, with a minimum alpha of 0.01.
Every accepted step passes Armijo and true-defect checks (maximum 0.081901);
every recorded state and merit rollback error is zero. Periodic candidate
acceptance and rollback checks also pass, but its endpoint is less converged.

LM loop/elapsed times are 604.730/605.793 seconds for retained d27 and
602.353/603.465 for the candidate. Periodic times are 307.827/308.448 and
302.470/303.043 respectively. All describe work until interruption, are contended
and ineligible, and provide no time-to-convergence ratio. Each pair's complete
configuration hashes match. `inner_candidate_lm_periodic_assessment.json` preserves
work, memory, activity and per-attempt contract checks; the focused CSV supplies
full source/build/configuration hashes and exact evidence paths.

## Verification of the committed default

The committed code with zero inner iterations, including the scaling correctness fix, has been
rebuilt with guarded single-job compilation. Its CFD SHA256 is
`8744f6d2628c68c3649528e63e47ded53da6ccd00cf09375ee2ad3a0bd065971`.
Twelve affected regression jobs pass again, including all 62 native equation cases
on 1×1 and 2×2, focused checks through 2×4, and species-boundary, residual-failure
and turbo-support checks on both requested layouts. The ordinary 91-case result
is reused for its byte-identical `f310351f...` driver; it is not presented as a
new execution. `inner_scaling_release_checks_ready.json` distinguishes these.

The build took 12.193 seconds, with sampled compiler-tree RSS 433088 KiB and no
memory guard failure. Its performance is not a solver measurement. Source files
match the committed code; the immutable `inner_scaling_release/manifest.json`
records the complete source tree identifier, relevant file hashes, binaries,
build command and options hash. Native SA on both layouts again produces
byte-identical 151-update restart files to retained d27. The common SST 20-update
prefix matches both the complete restart and every attempt-telemetry byte
(3382 flow assemblies, 3818 turbulence assemblies, 430 outer Krylov iterations).
This is numerical equivalence on the checked applications, not timing equivalence.
`inner_scaling_release_equivalence.json` pins all comparisons and verifies that
compiled source matches the committed code. The archived candidate workflow has
resumed; its current application was allowed to finish normally before rebuilding.

## Remaining CPU work on 2×2

A ten-second CPU-cycle sample of both owned candidate SA ranks collected 3216
samples with zero loss. This is one contended window during the strict-history
2×2 application. It makes that application ineligible for timing comparisons even
if its host samples were otherwise quiet. The [sampling CSV](NewtonKrylovInnerSampling20261018.csv)
preserves complete symbols and hashes; selected **self-cycle** shares are:

| Work | Sampled self cycles |
|---|---:|
| Live least-squares gradients using cached geometry | 16.52% |
| Flow flux | 12.24% |
| Native assembled matrix-vector product | 6.59% |
| Limiter | 6.57% |
| State validation | 6.24% |
| Native ILU application | 4.57% |
| SA scalar flux | 4.53% |
| Pseudo-time action | 2.83% |
| Solution restoration | 2.21% |
| Numerical-history restoration | 0.97% |

These are neither phase wall-time fractions nor call counts. In particular,
`ComputeILUPreconditioner` is the native **application** routine, not factorization.
The sample shows substantial live gradient/flux/limiter work remains after geometry
caching; it also exposes the added assembled work of the stronger preconditioner.
It does not establish that state validation or restoration can safely be removed.
Current state checks guard communication, strong boundaries, property refresh and
boundary closure separately. No check is removed based on this sample.

Evidence: `release_sampling/1792317842382208780/{manifest,summary}.json`,
`perf.data`, `record.log` and `flat_no_inline.txt`. The manifest records rank process
identities, exact profiling command and source/configuration/binary hashes. All
targets were verified descendants of the existing candidate supervisor with the
expected executable and application working directory. Other work was untouched.


## Qualified default0 parallel application

The narrowed work-first study now qualifies the missing coupled SA2×2 default0
endpoint:333 updates,83590 assemblies per system,22279 outer Krylov iterations,
166 retries and0 backtracks. Native remains qualified on both layouts. Every
retry in this full default0 run is newly measured as derivative verification;
rejected log intervals consume12915 assemblies/system (15.45%). Accepted steps
retain most of the work. The independent physical/state/quantity checks pass,
including canonical point error7.831e-6 below1e-5 and exact rollback.

The universal3 parallel trial remains incomplete with107971 assemblies/system
and failed state agreement. Same nonlinear configuration hashes match; no
threshold changes. [The NACA assessment](NewtonKrylovNacaRejections20261018.md)
and the focused CSV preserve the qualified-versus-incomplete distinction,
source/config hashes, memory, observed times and mechanism checks. Times are
contended/ineligible; this is work and qualification evidence.

A separate FLOW3/scalars0 isolation reduces SST segregated20-update FLOW
assemblies42.7%/50.7% on1×1/2×2 with no retries, but its states are unqualified.
Full matched SST endpoints are running before any strategy is adopted. The
committed default remains0; the old broad candidate schedule is deferred.
