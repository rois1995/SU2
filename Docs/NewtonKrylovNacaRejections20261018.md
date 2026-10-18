# NACA application convergence and rejection work

18 October 2026. Work in progress. Quiet-machine repetitions are deferred. The next
milestone is qualified SA/SST native, segregated and coupled endpoints on 1×1 and
2×2, with fewer residual assemblies and less linear work. T3A LM, the nozzle and
VKI follow that milestone; NEMO remains deferred.

The broad dispatchers were stopped only after their numerical/reporting children
finished normally. Eight candidate controls completed; the two pending nozzle
candidate jobs and the remaining broad control plan are deferred. Exact child
commands, terminal results and dispatcher identities are preserved in
`naca_priority_dispatcher_hold.json`, `naca_priority_broad_stopped.json` and
`inner_candidate_control_commands.json` under the application evidence directory.
No numerical run was interrupted by this scheduling change.

The frozen density/all-component residual, boundary projection, full-state and
quantity requirements remain unchanged. A time cap is **incomplete**, irrespective
of residual progress. Observed time and peak RSS remain resource measurements;
none of the following contended comparisons establishes a wall-clock speedup.
The [complete CSV](NewtonKrylovApplications20261018.csv) contains every attempted
setting, hashes, endpoint errors, memory and observed time. Its `rejection_work`
field attributes cumulative assembly deltas and linear work to recorded reasons.
Those assembly intervals include base preparation and intervening outer refresh;
they are not isolated flux-kernel counts. Native assembly counts retain their
explicitly inferred label. Missing historical rejection reasons are not inferred
from zero completed Krylov iterations.

## Existing preconditioner evidence

These are a separate strategy experiment: common inner default 0 versus candidate
3, with the same nonlinear controls. Default 0 is retained. The native inner-RHS
normalization correctness fix remains committed as `e47c7b14cd`.

| SA diagonal coupled | Inner | Updates | FLOW / TURB assemblies | Outer Krylov | Native inner work | Retries | Backtracks | Qualification |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| 1×1 | 0 | 179 | 33415 / 33415 | 7603 | 0 | 89 | 0 | Qualified |
| 1×1 | 3 | 166 | 24574 / 24574 | 5359 | 31177 | 82 | 0 | Qualified |
| 2×2 | 3 | 574 | 107971 / 107971 | 26656 | 159180 | 288 | 0 | Incomplete, 1200 s cap; full-state agreement missed |

At the supporting density-history −12.7 milestone on 2×2, default 0 used 73 updates,
21008 assemblies per system and 5755 outer Krylov iterations. Candidate 3 used
293 updates, 56512 assemblies and 13826 outer plus 82605 native-inner iterations.
The history stops differ (−13/−14) but neither is active through this milestone.
This is 2.69× the residual assemblies to a supporting milestone, **not** work to
qualified convergence. See [the milestone CSV](NewtonKrylovInnerMilestones20261018.csv).

Historical logs provide only partial mechanism evidence. All 288 parallel candidate
rejections have finite true defects above 0.1; 133 FGMRES orthogonalization warnings
also occur. That does not prove that every rejection originates in the defect check:
an earlier failed derivative can stop Krylov, leaving a partial direction whose
independent defect is finite. The 89 default-0 serial rejections complete zero
Krylov iterations, but there are no initial-guess-convergence messages. Zero
iterations therefore does not justify labeling these as an absolute-tolerance exit.

## Diagnostic change and validation

The local change adds an optional thread-local failure result to the existing
matrix-free product: invalid direction, no feasible residual probe, or unresolved
derivative verification. The controller retains the **first** failed product in
a solve, records the rejection stage, and counts failed products and inadmissible
line-search trials. A finite independent defect failure with no failed product
is labeled `TRUE_LINEAR_DEFECT`. Existing line-search status remains available;
mixed finite/inadmissible trials retain the status plus the inadmissible count.
Base residual, pseudo-time and final-statistics failures have distinct labels.

Probe sizes, refinement limits, derivative verification, linear defect bound,
acceptance, rollback, CFL and forcing are unchanged. No additional collective
reduction is introduced. New CSV files append three diagnostic columns; appending
to a legacy CSV retains its header and column count, with reasons unavailable in
that legacy file. Header inspection happens once per controller. Focused tests
cover accepted/resolved updates, failed solves, bounded retries, rollback,
unrepresentable probes, invalid directions, infeasible probes and both CSV schemas.

The diagnostic build and eight fresh affected regression jobs pass: all 63 native
cases on 1×1 and 2×2, plus species boundaries, residual-failure handling and turbo
support checks on both layouts. The ordinary 91-case result is reused only for
its byte-identical binary; it was not rerun. Build peak process-tree RSS was
796064 KiB and minimum available memory 40984544 KiB; no memory guard fired.

Six matched application checks pass their intended checks: four native endpoints
qualify (SA151 and SST397 on both layouts), with byte-identical retained native
states where a matched prior result exists. This establishes the missing stable
SST2×2 native reference. Both SA20-update diagnostic prefixes exactly reproduce
every original telemetry column. Their physical endpoints remain unqualified;
these prefixes diagnose mechanisms, not convergence performance.

Default0 SA retries in these prefixes are all derivative-verification failures:
9 on1×1 and10 on2×2, costing measured log-interval assemblies351 and747 per system,
respectively. Their completed outer Krylov work is0 and110. No admissibility or
line-search retry occurs. These are measured prefix results, not inferred labels
for every historical rejection. The three-inner replay is complete and exactly reproduces both historical SA
prefixes. Serial3/3 has9 derivative-verification retries; parallel3/3 has3 such
retries and8 true-defect retries with no failed product. Every one of those8
meets its reported Krylov target before failing the independent defect bound.
They consume1262 of3384 assemblies per system (37.3%); all rejected intervals
consume47.2%. These data expose an unreliable computed Krylov residual for that
strategy; they do not establish the underlying arithmetic cause or an SU2 bug.

The original-control SST prefixes also reproduce every historical telemetry
column. Segregated SST has no retries on either layout: accepted solves consume
3952/3946 FLOW assemblies and1226/1234 outer Krylov iterations. Its TURB counts
are336/339. Coupled SST has3 derivative-verification retries on each layout,
consuming667/668 FLOW and742/754 TURB assemblies. No observed prefix rejection
is caused by line-search or trial admissibility.

A separate **FLOW3/scalars0** experiment isolates the two native block policies.
Its source patch/binaries are archived, its63-case2×2 suite passes, and both
production source and the shared build are restored to default0 afterwards.
Parallel SA has no true-defect rejection in this20-update variant, but still has
9 derivative-verification retries. Serial SA improves assembly work only1.5%
and adds native-inner work; this is insufficient to adopt a universal strategy.

For SST segregated, FLOW3/scalars0 reduces20-update FLOW assemblies3952→2263 on1×1
and3946→1944 on2×2 (42.7%/50.7%), with no retries and slightly smaller full-state
errors at the same update count. Outer Krylov work falls1226→663 and1234→680,
while native-inner work adds1699/1722 iterations. The actual state errors remain
large and fail qualification. This supports a **full endpoint comparison**, not
a speedup or convergence-work claim. CFL, forcing and physical controls are
unchanged in this isolation. Default0 remains committed.

The full default0 parallel coupled SA endpoint now qualifies (details below).
Paired original-control SST segregated endpoints are running next (default0 and
FLOW3/scalars0 on both layouts), each with3600s and10000-update resource limits. They are resource
limits; reaching either leaves an incomplete result. One owned solver runs at a
time. Remaining common NACA endpoints are still outstanding; T3A/nozzle/VKI and
quiet timing remain deferred.

Correctness/work jobs run at nice19 and may coexist with one external compiler,
provided available RAM remains at least8GiB. Other solver/multiple-compiler jobs
still gate execution. The existing memory/swap monitor remains active. This
explicit work-only policy cannot be combined with `--quiet`; a harness check
rejects that combination before creating output. Other work is not modified.
Timing eligibility is false. The two original idle build gates were stopped before
launching children; their source snapshots and wait logs remain preserved.

Reproduce a native check from the evidence root, choosing a fresh output directory:

```sh
nice -n 19 python3 application_case.py --case naca_sst --mode native \
  --phase qualification --work-only --graceful-limit --control original \
  --cfl 1000 --history-margin 1 --updates 10000 --timeout 600 \
  --ranks 2 --threads 2 --release naca_rejection_v2_release --output FRESH_DIRECTORY
```

Build/source/config hashes and exact commands are retained in
`naca_rejection_v2_release/manifest.json`, `naca_rejection_v2_checks/commands.json`
and `naca_rejection_v2_application_commands.json`. Rejection aggregation and the
work gate have assert-based self-checks (`rejection_work.py`, `work_gate.py`).

Evidence root:
`/media/rausa/4TB/SU2_Versions/SU2_v850_Develop_FixCGNSOutput/newton_krylov_refactor_20261015/application_qualification_20261018`.

## Qualified parallel coupled SA endpoint

Default0 coupled SA on2×2 now completes normally in333 updates with the exact
serial-qualified diagonal/adaptive-CFL/forcing configuration (config SHA
`2d1d9d892adc1a5c3551c60ece5c0f11d8db4ca11c92acc72bf65664fe425b65`).
It uses83590 residual assemblies per system,22279 outer Krylov iterations,
166 retries and no backtracking. Every accepted step usesα=1, the independent
true-defect bound passes, and rollback errors are zero. Fresh density RMS is
9.869e-15, maximum normalized active-equation RMS3.275e-13, boundary projection0;
canonical full-state component/point errors1.223e-7/7.831e-6 pass. All quantities
pass the frozen bounds. Peak rank RSS103304KiB, sampled tree266528KiB, minimum
available memory38941292KiB; no memory guard fires.

All166 retries are measured derivative-verification failures. Their log intervals
consume12915 assemblies per system (15.45%) and1982 completed Krylov iterations.
Thus most assembly work remains in accepted updates even in this retry-heavy run.
There are no measured true-defect-only, admissibility or line-search rejections.
This supports retaining default0 relative to the incomplete universal3 parallel
experiment; it does not establish a general parallel scaling or speedup result.
Observed loop/E2E1090.736/1091.372s are ineligible for timing conclusions.

Exact command and independent checks:
`naca_sa_parallel_endpoint_command.json`, `naca_sa_parallel_qualified_assessment.json`.
The source/config/restart/reporter hashes are pinned in those files and the full
CSV. The earlier immutable checkpoint progress check remains supporting evidence:
its point error2.378e-5 missed the target; the final endpoint passes. Checkpoints
carry no iteration metadata, so no exact convergence work is inferred from them.

<!-- measured NACA results -->

## Current native endpoints

| Case | Layout | Status | Updates | F/T assemblies | Native linear work | Peak rank KiB | Loop / E2E s |
|---|---|---|---:|---:|---:|---:|---:|
| naca_sa | 1x1 | qualified | 151 | 151/151 | 6040 | 95276 | 21.418 / 21.960 |
| naca_sa | 2x2 | qualified | 151 | 151/151 | 6040 | 61696 | 16.050 / 16.558 |
| naca_sst | 1x1 | qualified | 397 | 397/397 | 15164 | 103360 | 60.211 / 60.786 |
| naca_sst | 2x2 | qualified | 397 | 397/397 | 15880 | 65440 | 43.669 / 44.102 |

Native assembly counts in this table are inferred; linear work is measured in native history. The [focused CSV](NewtonKrylovNacaRejections20261018.csv) preserves full-state, quantity, residual and hash evidence.

## Full Newton endpoint comparisons

SA rows share identical diagonal/adaptive-CFL/forcing controls and history−14. An incomplete row is not convergence work. All times are observed and ineligible for speedup claims.

| Inner F/T | Case | Layout | Mode / control | Status | Updates | F/T assemblies | Outer/inner work | R/B | Peak rank KiB | Loop / E2E s | Max point error |
|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| 3/3 | naca_sa | 1x1 | coupled / diagonal | qualified | 166 | 24574/24574 | 5359/31177 | 82/0 | 138708 | 556.675 / 557.225 | 7.890e-06 |
| 3/3 | naca_sa | 2x2 | coupled / diagonal | incomplete_time_cap | 574 | 107971/107971 | 26656/159180 | 288/0 | 94432 | 1200.933 / 1201.498 | 1.058e-05 |
| 0/0 | naca_sa | 2x2 | coupled / diagonal | qualified | 333 | 83590/83590 | 22279/0 | 166/0 | 103304 | 1090.736 / 1091.372 | 7.831e-06 |
| 0/0 | naca_sst | 1x1 | segregated / original | running | — | / | / | / | — | — / — | — |
| 0/0 | naca_sa | 1x1 | coupled / diagonal | qualified | 179 | 33415/33415 | 7603/0 | 89/0 | 171400 | 764.796 / 765.385 | 7.851e-06 |

## Supporting prefix work

All rows below stop at20 updates and remain unqualified. These compare work and progress, not convergence cost.

| Inner F/T | Case | Layout | Mode | F/T assemblies | Outer/inner linear work | Retries | Max component / point error |
|---|---|---|---|---:|---:|---:|---:|
| 3/0 | naca_sa | 1x1 | coupled | 3624/3624 | 907/2411 | 9 | 8.563e-07 / 5.927e-05 |
| 3/0 | naca_sa | 2x2 | coupled | 3984/3984 | 888/2745 | 9 | 1.367e-06 / 6.309e-05 |
| 3/0 | naca_sst | 1x1 | segregated | 2263/336 | 663/1699 | 0 | 8.778e-04 / 9.066e-02 |
| 3/0 | naca_sst | 2x2 | segregated | 1944/339 | 680/1722 | 0 | 8.829e-04 / 9.102e-02 |
| 3/3 | naca_sa | 1x1 | coupled | 3038/3038 | 706/4171 | 9 | 7.987e-07 / 5.519e-05 |
| 3/3 | naca_sa | 2x2 | coupled | 3384/3384 | 774/4547 | 11 | 1.323e-06 / 9.160e-05 |
| 0/0 | naca_sa | 1x1 | coupled | 3681/3681 | 802/0 | 9 | 9.266e-07 / 6.426e-05 |
| 0/0 | naca_sa | 2x2 | coupled | 5464/5464 | 1482/0 | 10 | 1.109e-06 / 7.726e-05 |
| 0/0 | naca_sst | 1x1 | coupled | 3382/3818 | 430/0 | 3 | 1.855e-03 / 1.617e-01 |
| 0/0 | naca_sst | 1x1 | segregated | 3952/336 | 1226/0 | 0 | 8.812e-04 / 9.086e-02 |
| 0/0 | naca_sst | 2x2 | coupled | 3521/4022 | 500/0 | 3 | 1.668e-03 / 1.448e-01 |
| 0/0 | naca_sst | 2x2 | segregated | 3946/339 | 1234/0 | 0 | 8.832e-04 / 9.155e-02 |

## Measured rejection prefixes

These are update-limited mechanism checks, not qualified endpoints or convergence-work comparisons. Times remain observations only. Rejected assembly intervals use the boundary described above.

| Inner F/T | Case | Layout | Group | Reason | Rejections | Retries | F/T assemblies | Outer/inner linear work |
|---:|---|---|---|---|---:|---:|---:|---:|
| 3/0 | naca_sa | 1x1 | newton_coupled | DERIVATIVE_VERIFICATION | 9 | 9 | 351/351 | 0/27 |
| 3/0 | naca_sa | 2x2 | newton_coupled | DERIVATIVE_VERIFICATION | 9 | 9 | 1380/1380 | 306/945 |
| 3/3 | naca_sa | 1x1 | newton_coupled | DERIVATIVE_VERIFICATION | 9 | 9 | 371/371 | 1/60 |
| 3/3 | naca_sa | 2x2 | newton_coupled | DERIVATIVE_VERIFICATION | 3 | 3 | 334/334 | 68/426 |
| 3/3 | naca_sa | 2x2 | newton_coupled | TRUE_LINEAR_DEFECT | 8 | 8 | 1262/1262 | 339/2034 |
| 0/0 | naca_sa | 1x1 | newton_coupled | DERIVATIVE_VERIFICATION | 9 | 9 | 351/351 | 0/0 |
| 0/0 | naca_sa | 2x2 | newton_coupled | DERIVATIVE_VERIFICATION | 10 | 10 | 747/747 | 110/0 |
| 0/0 | naca_sst | 1x1 | newton_coupled | DERIVATIVE_VERIFICATION | 3 | 3 | 667/742 | 73/0 |
| 0/0 | naca_sst | 2x2 | newton_coupled | DERIVATIVE_VERIFICATION | 3 | 3 | 668/754 | 85/0 |
