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
The serial original-control SST segregated attempts with default0 and FLOW3/scalars0
both reach their 3600-second resource caps without qualifying. Those results remain
in the table; the full parallel comparison is outstanding. One owned solver runs
at a time. Remaining common NACA endpoints are still outstanding; T3A/nozzle/VKI
and quiet timing remain deferred.

Work-only jobs may coexist with one external compiler,
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

## SST progress against numerical work

The [SST work milestones CSV](NewtonKrylovSSTWorkMilestones20261018.csv) tracks
original-control density-history crossings separately from endpoint qualification.
It updates as trials finish. Native counts remain explicitly inferred; Newton
counts use the same cumulative-controller boundaries as the application ledger.
The helper `residual_work_milestones.py` has assert-based checks for first crossings,
failed-attempt work and cumulative per-group counters.

For example, SST1×1 reaches density history−11 in123 native updates with4920 linear
iterations, versus55 default0 segregated updates with4344 outer Krylov iterations.
The native assembly count is123 per system (inferred); Newton measures12465 FLOW
and1830 TURB assemblies. Fewer updates or similar linear counts therefore do not
imply similar computational work: native products use assembled matrices, whereas
Newton products refresh physical residuals. Counts are not equal-cost operations.

Native history samples the assembled residual before its implicit update; Newton
history uses the refreshed accepted state. This convention difference is retained
explicitly in the CSV. These density-only crossings are supporting progress,
not matched physical endpoints, exact convergence-work comparisons or speedups.
Only the independent full endpoint checks establish qualification.

During the full default0 SST1×1 run, the first FLOW retry occurs at iteration134:
derivative verification fails after80 completed Krylov iterations and recovery
reduces FLOW CFL1000→250. TURB retains1000. The initial20-update prefix had no
retry; its observation must not be generalized to the complete run. Controls
remained unchanged through its resource cap. The saved endpoint remains incomplete.

## Qualified final-build serial replay

The serial adaptive coupled SA baseline now qualifies on the current c04b executable
in 179 updates, with 33415 assemblies per system, 7603 Krylov iterations, 89 retries
and no backtracking. Its restart bytes and all 30 original diagnostic columns match
the earlier serial baseline. The new reason columns directly classify every retry
as derivative verification; each rejected solve completes zero Krylov iterations.
This resolves the earlier uncertainty about those zero-iteration failures. Accepted
steps use alpha 1, true defects remain below 0.1 and rollback errors are zero.
The final-build physical reporter exactly matches the frozen auditor. Observed times
remain ineligible. The fixed-forcing serial comparison is incomplete, as described below.

A 30-second CPU-cycle sample of the live serial tail records no lost samples. Flat
symbol shares include cached least-squares gradients 19.74%, Roe flux 10.22% and
limiters 6.87%. These samples combine necessary live refreshes and repeated work;
they do not identify an avoidable fraction or provide phase wall times. Numerical
columns remain identical after sampling. Source/build/configuration hashes and the
raw record are retained in `release_sampling/1792353801499247196/` under the
application evidence root. Endpoint proof is
`fixed_forcing_005_20261018/serial_adaptive_qualified_assessment.json`.

## Serial fixed forcing: work-limited counterexample

Fixed nominal forcing 0.05 does not reproduce its parallel work benefit in serial.
After 564 updates it has used 40134 FLOW and TURB assemblies, 2653 completed Krylov
iterations and 284 retries, all derivative-verification failures. There is no
backtracking. Accepted CFLs fall mainly to 31.25–125, whereas the qualified adaptive
baseline uses mainly 1000–2000. To the supporting density-history −12.4 crossing,
fixed forcing uses 68 updates and 5122 assemblies per system; adaptive forcing uses
6 updates and 1088. These crossings are progress checks, not qualified endpoints.

The fixed attempt was stopped gracefully after exceeding the qualified adaptive
baseline's 33415-assembly budget. The request observes 40032 assemblies; finishing
the current update gives 40134. Fresh density RMS 2.449e-13 and maximum normalized
RMS 8.266e-12 pass, but canonical point error 4.822e-5 exceeds 1e-5. Its observed
1648.623-second loop time is resource use, not time to convergence. The record is
`incomplete_work_cap`; fewer completed Krylov iterations and lower observed RSS
do not qualify an efficiency improvement. It might converge with further work.

The work cap changes no numerical controls or acceptance bounds. A metadata guard
prevents graceful interruption from being counted as convergence, even if a saved
state passes every physical check. A copied-state self-check verifies both that
guard and the unchanged qualification of an uncapped state. The prior qualifier,
exact stop action and unsuccessful evidence are retained in
`fixed_forcing_005_20261018/{work_limit_selfchecks,serial_fixed_work_limit_action,serial_fixed_work_limited_assessment}.json`.
The fixed-forcing queue stops for assessment; its segregated jobs were not launched.
The subsequent segregated study is described below.

Reproduce the copied-state work-cap check from the evidence root with
`python3 check_work_limit.py`. It asserts that neither a solver process nor changes
to original result, restart, configuration or physical-target files occur.

## Original segregated SA and isolated CFL recovery

Original-control segregated SA on 1×1 reaches its FLOW-work budget without
qualifying: 480 updates, 34049 FLOW and 18831 TURB assemblies, 6703 FLOW plus
15469 TURB Krylov iterations, two retries and no backtracking. The budget is the
qualified serial coupled baseline's 33415 FLOW assemblies; the monitor observes
33978 when requesting interruption, and the solver finishes the current update.
This FLOW-only budget does not establish an overall cost ratio between modes.

Both rejections are derivative-verification failures, at iterations 1 and 5. Their
log intervals consume only 96 FLOW and 2 TURB assemblies, about 0.28% of total FLOW
work. FLOW accepts one update at CFL 1000, four at 250 and 475 at 62.5. TURB retains
1000 throughout. All accepted steps use alpha=1, true defects remain below 0.1 and
rollback errors are zero. Most observed work therefore belongs to accepted steps
after recovery, rather than repeated rejection or backtracking.

Fresh density RMS 2.778e-13 and maximum normalized RMS 3.570e-13 pass; component
state agreement passes, but point error 2.954e-5 misses 1e-5. The endpoint is
`incomplete_work_cap`, with no convergence time assigned. Observed loop/elapsed
time is 1670.995/1671.756 seconds and peak rank RSS 130988 KiB. These are resource
measurements; the host is contended and memory remains safe. It might converge
with further work under these controls.

The serial segregated follow-up changes **only `NEWTON_ADAPT_CFL=NO` to `YES`**.
Reversing that one edit reproduces the original configuration byte for byte.
Forcing stays off, nominal linear target stays 0.001, inner iterations stay 0 and
history stop stays −14. All frozen physical/state/quantity gates remain fixed.
It starts from the same supplied initial state with the same final c04b executable.
This isolates a configuration change; no implementation gain or default is selected.

Exact commands, source/configuration hashes, completed original assessment and
queue state are in `sa_original_segregated_command.json` and
`sa_original_segregated_progress/{original_work_limited_assessment,adaptive_cfl_only_preflight,adaptive_cfl_queue,source_unchanged_check}.json`.
The queue runs one solver at a time and stops for assessment after the CFL-only
endpoint. A first monitor inside the sandbox could not see host processes and
exited without signaling; that evidence is retained separately from the successful
host monitor. No foreign work was modified.

Reproduce each configuration from the evidence root with a fresh output directory:

```sh
python3 application_case.py --case naca_sa --mode segregated --control original \
  --phase qualification --work-only --graceful-limit --cfl 1000 --history-margin 2 \
  --updates 10000 --timeout 7200 --ranks 1 --threads 1 \
  --release muscl_boundary_gradient_v2_release --output FRESH_ORIGINAL \
  --reference static_review_correctness_v2_applications/naca_sa/1x1/native
```

Use `--control adaptive` and another fresh directory for the isolated CFL change.
The numerical deck is reproducible by this command. The original archived attempt
also uses the separately recorded FLOW-work interruption; running the command
alone continues until convergence or its time/update resource cap.

The [SA work-milestone CSV](NewtonKrylovSAWorkMilestones20261018.csv) now tracks
the same final-build studies on both layouts, including original and adaptive-CFL
segregated runs as their histories become available. It uses the existing tested
crossing/counter helpers; regenerate it with `python3 residual_work_milestones.py`
from the evidence root. These density-only crossings are supporting progress,
with no physical/state qualification at the crossing itself.

| Density-history crossing | Segregated control | Updates | FLOW / TURB assemblies | Krylov work | Retries |
|---|---|---:|---:|---:|---:|
| −12.4 | Original | 101 | 7519 / 4026 | 4714 | 2 |
| −12.4 | CFL adaptation only | 60 | 7052 / 3465 | 3899 | 31 |
| −12.5 | Original | 345 | 24599 / 13566 | 15962 | 2 |
| −12.5 | CFL adaptation only | 213 | 23809 / 12966 | 14184 | 107 |

At −12.5, adaptation reduces updates by 38.3%, FLOW assemblies by 3.2%, TURB
assemblies by 4.4% and linear iterations by 11.1%. All 107 retries are measured
derivative-verification failures. Their log intervals consume 7240 FLOW assemblies
(30.4% of counted FLOW work), versus 96 in the original-control prefix. Intervals
include bases and intervening reports; these are not isolated failure kernels or
time fractions. This explains why the update reduction does not translate into
a comparable work reduction. These crossings do not qualify a convergence
improvement or justify a new default; the completed capped assessment follows.

## Completed CFL-only segregated SA assessment

The 1×1 adaptive-CFL-only run reaches its 7200-second resource cap after 1,217
updates, with 132,431 FLOW/69,295 TURB assemblies and 75,782 outer Krylov iterations.
Its density history ends at −13.172 rather than the unchanged −14 stop. Status is
**incomplete_time_cap**; observed loop 7,203.274 s/end-to-end 7,203.906 s are resource
costs, not convergence times. The executable remains c04b; the pending source
optimization was not used. Only CFL adaptation differs from the original
segregated configuration; forcing remains off and inner work remains 0.

All 610 retries are measured FLOW derivative-verification failures, with 3,844
discarded Krylov iterations and 732 failed products. Their log intervals account
for 40,360 FLOW assemblies (30.5% of the reported total), including rollback/base
work and intervening reports. Only two retries have zero completed iterations.
TURB has no retries; it consumes 61,289 accepted Krylov iterations. There are no
backtracks. Accepted FLOW CFLs are mostly 31.25–250; every nonzero accepted FLOW
step requests doubling. TURB reaches 1e6. This supports investigating product
verification and CFL cycling, rather than attributing this case to backtracking
or assuming that a duplicate base refresh explains most of its work.

Failed products also occur inside 1,206 of the 1,217 accepted FLOW attempts.
These retain partial Krylov directions that pass the unchanged independent
true-defect and globalization checks. Their log intervals contain 91,104 FLOW
assemblies and 10,468 useful Krylov iterations; these totals include successful
work and cannot be assigned entirely to failed products. Only 11 accepted FLOW
attempts have no failed product. Accepted-attempt logs do not identify each
failed product's reason, so this category is not automatically classified as
derivative-verification failure. The separate accounting is preserved in
`sa_original_segregated_progress/adaptive_accepted_product_work.json`.

The independently refreshed saved state passes the frozen physical, boundary,
state and quantity bounds: density RMS 6.730e−14, maximum normalized RMS 6.730e−14,
zero boundary projection, maximum component error 8.442e−8 and point error 1.493e−6
against canonical native. The canonical audit remains unqualified because the
run was interrupted; passing saved-state gates does not convert the cap into a
converged endpoint. Peak rank RSS 142,304 KiB, minimum available RAM 40,188,924 KiB,
and no memory guard termination were observed. Foreign CPU work makes timing
ineligible. No convergence-work ratio or general speedup is inferred.

The assessment, raw-file hashes, all rejection intervals and accepted CFL
distribution are preserved in
`sa_original_segregated_progress/adaptive_cfl_time_limited_assessment.json`
under the application evidence directory. Reproduce with the earlier command,
`--control adaptive`, and a fresh output directory. The maintained application
and milestone tables retain its incomplete status.

## Verified serial SA derivative trace

A read-only debugger child reproduces the first failed original-control FLOW
product on the pinned c04b executable. Its restart, history and all diagnostics
match the plain two-update replay byte for byte, and the attempts match the
completed original-run prefix. The failed product follows 42 successful FLOW
products. It exhausts two 17-sample searches; all 34 feasibility checks and full
residual evaluations succeed. Saved operator/physical vectors confirm a fixed
mass action to normalized difference 5.70e−17.

| Starting sequence | First scalar step | First eligible step | Relative error there | Required limit |
|---|---:|---:|---:|---:|
| Original | 4.5163e−10 | 1.1291e−10 | 5.6895e−2 | 5e−4 |
| Existing 100× recovery | 4.5163e−8 | 1.1291e−8 | 5.9244e−4 | 5e−4 |

Every eligible sample fails the unchanged verification test. Error grows
approximately as `1/h`: fitted log slopes over refinements 2–6 are −0.972 and
−1.007. This captured event is consistent with cancellation-dominated
differencing, rather than failed state feasibility or changing pseudo-time
mass. It does not classify every later failed product. The coarser sequence's
second sample has smaller error but is ineligible; the mandatory third sample
and existing tolerance remain unchanged.

Commands, step/error CSV, raw vector hashes and the independent curve assessment
are in `sa_derivative_replay_20261018/{queue.json,probe_curve.csv,curve_assessment.json}`
under the application evidence directory. These are supporting mechanism checks,
not convergence or timing measurements. The earlier
[larger-probe experiment](NewtonKrylovProbeRange20261018.md) already recovered
individual derivatives without establishing an application benefit. This new
trace does not justify reinstating it or weakening verification.

## Local efficiency changes: original-control checks

Two small source changes remove work whose result is unused. Rejected true-defect
attempts retain their full rollback refresh and omit the preceding duplicate base
refresh. The first two Richardson refinements retain every residual probe and
history update while skipping unused norms and mass scratch actions; verification
still begins with the third sample, at the same tolerance. No cache, option or
new persistent storage is added.

Both 70-case native Newton suites and the affected species-boundary, residual-failure,
turbomachinery and setup CLI checks pass on 1×1 and 2×2. The ordinary test executable
is byte-identical, so its existing 92-case result is reused. All six application
prefixes reproduce states, physical reports and trial decisions byte for byte.

These are **20-update, unqualified prefixes**, with identical configurations. Counts
compare retained c04b with combined candidate 1ad21449. Outer updates, linear work,
retries and backtracking remain unchanged; only the expected failed-attempt base
assembly is omitted. They establish correctness and local work savings, not
convergence cost or a runtime speedup.

| Case | Layout | Mode | FLOW assemblies before → after | TURB assemblies before → after | Krylov work, unchanged | Retries, unchanged |
|---|---|---|---:|---:|---:|---:|
| naca_sa | 1x1 | segregated | 1810 → 1808 | 786 → 786 | 907 | 2 |
| naca_sa | 2x2 | segregated | 3810 → 3809 | 678 → 678 | 1606 | 1 |
| naca_sst | 1x1 | segregated | 3952 → 3952 | 336 → 336 | 1226 | 0 |
| naca_sst | 2x2 | segregated | 3946 → 3946 | 339 → 339 | 1234 | 0 |
| naca_sst | 1x1 | coupled | 3382 → 3379 | 3818 → 3815 | 430 | 3 |
| naca_sst | 2x2 | coupled | 3521 → 3518 | 4022 → 4019 | 500 | 3 |

Hashes and commands are preserved in `verified_norms_20261019/prefix_assessment.json`,
the release manifests, and both validation JSON files in the application evidence
directory. The maintained application CSV records all observed times and memory
values; timings are ineligible.

The matched 2×2 SST segregated profile measures **4,752 fewer Allreduce calls per
rank: 92,966 → 88,214 (5.11%)**. Recorded OpenMP barrier calls fall by 19,008 per
rank: 1,272,544 → 1,253,536. Counts cover the whole solver process on its main
thread, including setup and output, with the same profiler and configuration.
Restart, history and diagnostic bytes remain identical. Point-to-point MPI calls,
recorded loop-end calls and aligned-allocation calls remain unchanged. General
allocation/free counts vary between ranks; this establishes neither a memory
saving nor a wall-time speedup.

Full native SA accuracy-0.05 replays qualify on both layouts and reproduce their
archived restart, history, physical report and linear work exactly: 158 updates /
1,030 linear iterations on 1×1; 151 updates / 4,209 on 2×2. The full coupled SA replay also qualifies at 179 updates, with unchanged 7,603
Krylov iterations, 89 retries and no backtracking. Its restart, physical report,
trial logs and all other attempt columns match the retained version exactly.
The removed duplicate refresh saves 89 assemblies per active system: 33,415 →
33,326 (0.266%). This is a modest work saving; the original verification/recovery
cost and convergence trajectory remain.

Native history stop −13 and Newton history stop −14 are retained exactly from
their archived decks; their frozen physical/state gates are common. Cross-solver
conclusions must label that margin difference. The reduction and native proofs
are in `verified_norms_20261019/profile_assessment.json`; full endpoint comparisons
are in `verified_norms_20261019/full_endpoint_assessment.json`.

## Full endpoints after the local efficiency changes

All rows qualify against the frozen physical, boundary, state and quantity gates,
and reproduce their retained restart and independent physical report byte for byte.
They compare the same configuration before and after implementation changes. Native
assembly counts are inferred from fine-grid updates; coupled counts are measured.
Observed times and rank RSS are retained below, with **all timings ineligible**.
These single contended measurements establish neither time nor memory savings.

| Mode / layout | Updates, unchanged | FLOW/TURB assemblies before → after | Linear work, unchanged | Loop seconds before → after | E2E seconds before → after | Peak rank KiB before → after |
|---|---:|---:|---:|---:|---:|---:|
| native / 1x1 | 158 | 158/158 → 158/158 | 1030 | 18.092 → 14.390 | 18.840 → 15.096 | 78096 → 78236 |
| native / 2x2 | 151 | 151/151 → 151/151 | 4209 | 51.890 → 43.692 | 52.558 → 44.365 | 60404 → 60140 |
| coupled / 1x1 | 179 | 33415/33415 → 33326/33326 | 7603 | 1489.580 → 1702.597 | 1490.316 → 1703.262 | 170972 → 171012 |

The full SA coupled endpoint still contains 89 derivative-verification retries;
the successful replay validates the local changes without resolving the larger
numerical-work limitation. Parallel coupled requalification, segregated SA/SST
endpoints, and the remaining application matrix are still outstanding.

## Parallel three-inner failure: independently reconstructed model

A four-update replay of the archived three-inner 2×2 executable reproduces its
original attempt prefix exactly. Read-only GDB children capture all 38 basis
inputs/products, combination weights, reduced matrix/right-hand side, orthogonal
basis and the independently evaluated combined direction. Traced and plain
restart, history and both diagnostic files are byte-identical. This is a supporting
mechanism check, with no convergence or timing claim.

The selected event is outer iteration 3, attempt 1, CFL 500. Every product is finite
and no product failure is logged. FGMRES meets target 0.00196893, but the unchanged
independent 0.1 true-defect limit rejects the direction:

| Check | Measured relative value |
|---|---:|
| FGMRES reported residual | 0.001700043129318 |
| Residual from recorded basis-product combination | 0.001700043129326 |
| Fresh product of the combined direction | 0.232097222014 |
| Difference between fresh and combined basis products, divided by RHS norm | 0.232100093235 |
| Direction reconstruction error | 3.059e−13 |
| Reduced backsolve residual, divided by RHS norm | 2.715e−13 |

Coefficients are identical across ranks. The 39-vector Gram matrix differs from
identity by at most 5.676e−15. The reduced triangular system has condition estimate
1.362e5; the sum of weighted basis-product norms is 9,080 times the RHS norm.
These measurements show substantial cancellation and amplification in this model.
They are not a rigorous bound on finite-difference error.

The fresh-product discrepancy is mainly FLOW: its contribution is 0.232009,
versus 0.006509 for TURB, both normalized by the same full RHS norm. For this event,
the recorded combination and reported residual agree; the discrepancy appears
when evaluating the combined direction. This does not distinguish roundoff,
nonsmoothness, or direction-dependent probe scales, and does not establish an
Arnoldi/backsolve bookkeeping defect. Other rejected attempts have not received
this complete vector reconstruction.

The existing default remains zero inner iterations. The three-inner strategy has
not demonstrated reliable application convergence on both layouts, and accepting
its reported residual instead of checking the true defect would accept this bad
direction. The complete raw-vector hashes, commands and checks are in
`preconditioner_linearity_20261019/{ready.json,conditioning_assessment.json}` and
`traced/combination_assessment.json` under the application evidence directory.
The arithmetic helper also passes independent linear/nonlinear and partition
selfchecks. NumPy 1.24.4, already installed, supplies only the optional Gram/condition
analysis; the residual reconstruction uses independent compensated sums.

## SA references with the same history stopping control

The new native references tighten the history stop from −13 to −14, matching the
full Newton runs. Native linear tolerance remains 0.05. All frozen physical,
boundary, full-state and quantity requirements pass, including the independent
canonical audit. The table uses the current executable, SHA `1ad2144949e92b12…`,
whose 21 numerical source hashes map to committed `5c0cb273e3`. The original
precommit build manifest is preserved; `committed_source_provenance.json` supplies
the mapping rather than rewriting its recorded HEAD.

| Mode / control | Layout | Updates | FLOW / TURB assemblies | Linear work | Peak rank KiB | Loop / E2E s | Maximum normalized physical RMS | Canonical point error |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| Native, fixed linear tolerance 0.05 | 1×1 | 304 | 304 / 304 | 1765 | 78780 | 26.071 / 26.715 | 1.163e−11 | 7.566e−6 |
| Native, fixed linear tolerance 0.05 | 2×2 | 299 | 299 / 299 | 7787 | 60348 | 71.133 / 71.680 | 1.007e−11 | 7.571e−6 |
| Coupled, diagonal / adaptive CFL and forcing | 1×1 | 179 | 33326 / 33326 | 7603 | 171012 | 1702.597 / 1703.262 | 3.257e−13 | 7.851e−6 |

Native assemblies are inferred from fine-grid implicit updates; Newton assemblies
are measured controller counters plus the final outer-reporting assembly. Native
linear work sums its separate equation blocks; coupled work counts five-variable
Krylov steps, so these counts are not equal-cost operations. All observed times
are ineligible for speedup or scaling claims. These are separately labelled
control settings, not an implementation-only comparison.

The native histories expose the block-level layout dependence:

| Layout | FLOW linear iterations | TURB linear iterations | FLOW iterations per update | Updates using all 20 permitted FLOW iterations |
|---|---:|---:|---|---:|
| 1×1 | 916 | 849 | 3 on 300 updates, 4 on 4 updates | 0 |
| 2×2 | 5950 | 1837 | 19 on 30 updates, 20 on 269 updates | 269 |

Every reported FLOW relative linear residual meets 0.05; the largest is 0.049759
on 2×2. These are residuals for the native assembled approximate matrix. Independent
finite-difference defects of native steps have not been measured. TURB histories
include 62 and 65 zero-work updates, respectively; their cause is not inferred
from iteration counts alone. Both endpoints still pass all frozen physical and
state gates. The difference shows why a fixed inner-iteration budget need not
give comparable preconditioning across layouts; it does not establish a benefit
for any replacement strategy. The common default remains zero inner iterations.

The application CSV and focused CSVs now include `linear_work_by_group`: native
history fields remain separate, while each Newton controller records its outer
and native-inner totals over all logged attempts. Coupled work is counted once
under its coupled group. Missing legacy inner diagnostics remain `null`. Running
results remain pending. All previous fields in the 321-row ledger are unchanged;
the 300 rows with recorded aggregate linear work reproduce their existing totals.
The aggregation selfchecks cover native block sums, rejected-attempt work and
missing diagnostics. Evidence and generator/helper hashes are preserved in
`native_linear_breakdown_20261019/assessment.json` under the application evidence
root; original reports and helpers remain archived beside it.

The existing coupled endpoint also passes a direct comparison with the new
serial native −14 reference: maximum component error 1.745e−8, point error
2.876e−7, and quantity errors within the unchanged limits. This additional check
is separate evidence; its original qualification record remains immutable.
Native samples pre-update residuals while Newton samples refreshed accepted
states; the fresh endpoint audits supply the common physical comparison.

The history threshold also governs the common controller's resolved-group test.
Changing it can affect late group idling. It must therefore remain explicit in
configuration comparisons, even when the frozen physical requirements pass.

An isolated segregated study changes only nominal linear tolerance 0.001→0.05;
adaptive CFL and adaptive forcing both remain disabled. Its matched 20-update
serial prefix uses FLOW/TURB 1281/591 and 619 Krylov steps, versus 1808/786 and
907 originally. Both have two derivative-verification retries and no backtracking.
FLOW CFL falls from 1000 through 250 to 62.5 and remains there; TURB retains
1000. Density history after 20 updates is −12.36798 versus −12.36689. Neither
prefix qualifies, so this is a work/progress observation, not reduced work to
convergence. Full 1×1 and 2×2 endpoints remain pending, followed by the known
parallel coupled replay. Resource caps remain incomplete results.

Reproduce the stricter native reference from the evidence root, choosing a fresh
output directory and the desired rank/thread pair:

```sh
nice -n 19 python3 fixed_forcing_005_case.py --case naca_sa --mode native \
  --phase qualification --work-only --graceful-limit --control original \
  --cfl 1000 --history-margin 2 --updates 10000 --timeout 1800 \
  --ranks 1 --threads 1 --release verified_norms_release --output FRESH_DIRECTORY \
  --reference static_review_correctness_v2_applications/naca_sa/1x1/native
```

The native config SHA is
`ac9706841a87cb66f33ead52199e6b5fb97a9749ecfdab8b8faf6ba50b03c4c1`;
the coupled config remains
`2d1d9d892adc1a5c3551c60ece5c0f11d8db4ca11c92acc72bf65664fe425b65`.
All seven selected commands, configuration-only differences and input hashes are
in `sa_accuracy_005_original_20261019/plan.json`; reference results are under
`sa_accuracy_005_original/naca_sa/`. Direct agreement and supporting-prefix data
are `existing_coupled_against_native14.json` and `first20_work.json` in the study
directory. The [complete CSV](NewtonKrylovApplications20261018.csv) includes these
references and preserves the earlier −13 results.

## Accepted segregated tail: measured CPU costs

A 30-second read-only CPU-cycle sample of the current SA1×1 accuracy-only run
records 2920 samples with no lost samples. The executable remains `1ad21449…`,
config `1366f79bc6fbfff855b0d9a3c220275f3b5d570cd81d6d451401838665b5d59b`.
This is an accepted-tail window around updates 599–610, not a whole-run profile
or a timing benchmark. All newly logged attempts have alpha 1, true defect below
0.1 and exact rollback. There are no rejections in this window.

| Leaf computation | Share of recorded cycles |
|---|---:|
| Cached least-squares gradients | 26.67% |
| Roe viscous flux kernel | 8.66% |
| Limiters | 8.45% |
| Residual wrapper itself | 6.12% |
| State validity checks | 5.59% |
| Primitive/property conversion | 5.21% |
| Solution restoration plus numerical-history save/restore | 4.33% |
| ILU preconditioner construction | 1.57% |

Caller stacks partition the gradient share into 13.30% through the initial FLOW
dependency refresh, 11.59% through other FLOW preprocessing, including the
boundary-only path, and 1.79% through solution gradients. These are disjoint leaf
cycle shares, not inclusive caller percentages. They do not separate FLOW-owned
probes from auxiliary-owned probes, and therefore do not measure an avoidable
fraction. In particular, live field gradients remain necessary when their inputs
change. The result supports investigating narrowly scoped unchanged-field reuse
before reducing preconditioner construction frequency.

The diagnostic-file snapshots add 12 FLOW and 11 TURB accepted rows: 72 and 264
Krylov steps, respectively, with cumulative counter deltas of 552 FLOW and 342
TURB assemblies. Snapshot endpoints straddle an outer update; these are measured
log-interval deltas, not exact phase call counts. Earlier rows remain unchanged.
The host remains contended and the run stays ineligible for timing conclusions.
CPU-cycle sampling also does not measure blocked I/O or synchronization time;
these shares cannot establish that such costs are negligible.

Raw profiles, before/after diagnostics, PID/start identity, commands and hashes
are in `release_sampling/1792372543938788179/`. `cpu_assessment.json` uses recorded
periods rather than unweighted sample counts. Its parser passes synthetic
header/frame and leaf-versus-caller checks; its total recorded period exactly
matches perf's report. Reproduce the analysis from the application evidence root:

```sh
python3 release_sampling/analyze_cpu_sample.py release_sampling/1792372543938788179
```

No additional solver was launched and no numerical code or controls changed.
The full accuracy-only endpoint study remains pending; this sample does not
qualify the current state or show reduced work to convergence.

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

Configurations remain labelled per row. Coupled SA baselines use diagonal/adaptive-CFL/forcing controls; original segregated trials retain their supplied controls. An incomplete row is not convergence work. All times are observed and ineligible for speedup claims.

| Inner F/T | Case | Layout | Mode / control | Status | Updates | F/T assemblies | Outer/inner work | R/B | Peak rank KiB | Loop / E2E s | Max point error |
|---|---|---|---|---|---:|---:|---:|---:|---:|---:|---:|
| 0/0 | naca_sa | 1x1 | coupled / diagonal | qualified | 179 | 33415/33415 | 7603/0 | 89/0 | 170972 | 1489.580 / 1490.316 | 7.851e-06 |
| 0/0 | naca_sa | 1x1 | coupled / diagonal | incomplete_work_cap | 564 | 40134/40134 | 2653/0 | 284/0 | 118584 | 1648.623 / 1649.319 | 4.822e-05 |
| 0/0 | naca_sa | 2x2 | coupled / diagonal | qualified | 333 | 83590/83590 | 22279/0 | 166/0 | 103428 | 3378.044 / 3378.847 | 7.831e-06 |
| 0/0 | naca_sa | 2x2 | coupled / diagonal | qualified | 356 | 57365/57365 | 13375/0 | 178/0 | 78696 | 1815.426 / 1816.065 | 7.811e-06 |
| 3/3 | naca_sa | 1x1 | coupled / diagonal | qualified | 166 | 24574/24574 | 5359/31177 | 82/0 | 138708 | 556.675 / 557.225 | 7.890e-06 |
| 3/3 | naca_sa | 2x2 | coupled / diagonal | incomplete_time_cap | 574 | 107971/107971 | 26656/159180 | 288/0 | 94432 | 1200.933 / 1201.498 | 1.058e-05 |
| 0/0 | naca_sa | 2x2 | coupled / diagonal | qualified | 333 | 83590/83590 | 22279/0 | 166/0 | 103304 | 1090.736 / 1091.372 | 7.831e-06 |
| 0/0 | naca_sst | 1x1 | segregated / original | incomplete_time_cap | 1254 | 133911/24743 | 41283/0 | 2/0 | 174404 | 3600.846 / 3601.375 | 5.200e-06 |
| 3/0 | naca_sst | 1x1 | segregated / original | incomplete_time_cap | 500 | 51362/13686 | 18170/36294 | 2/0 | 142676 | 3601.092 / 3601.715 | 1.669e-05 |
| 0/0 | naca_sa | 1x1 | segregated / adaptive | incomplete_time_cap | 1217 | 132431/69295 | 75782/0 | 610/0 | 142304 | 7203.274 / 7203.906 | 1.493e-06 |
| 0/0 | naca_sa | 1x1 | segregated / original | incomplete_work_cap | 480 | 34049/18831 | 22172/0 | 2/0 | 130988 | 1670.995 / 1671.756 | 2.954e-05 |
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
