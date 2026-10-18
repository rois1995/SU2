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
for every historical rejection. The three-inner replay is the next focused check;
no further parameter change is selected before that comparison.

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

<!-- measured NACA results -->

## Current native endpoints

| Case | Layout | Status | Updates | F/T assemblies | Native linear work | Peak rank KiB | Loop / E2E s |
|---|---|---|---:|---:|---:|---:|---:|
| naca_sa | 1x1 | qualified | 151 | 151/151 | 6040 | 95276 | 21.418 / 21.960 |
| naca_sa | 2x2 | qualified | 151 | 151/151 | 6040 | 61696 | 16.050 / 16.558 |
| naca_sst | 1x1 | qualified | 397 | 397/397 | 15164 | 103360 | 60.211 / 60.786 |
| naca_sst | 2x2 | qualified | 397 | 397/397 | 15880 | 65440 | 43.669 / 44.102 |

Native assembly counts in this table are inferred; linear work is measured in native history. The [focused CSV](NewtonKrylovNacaRejections20261018.csv) preserves full-state, quantity, residual and hash evidence.

## Measured rejection prefixes

These are update-limited mechanism checks, not qualified endpoints or convergence-work comparisons. Times remain observations only. Rejected assembly intervals use the boundary described above.

| Inner | Case | Layout | Group | Reason | Rejections | Retries | F/T assemblies | Outer/inner linear work |
|---:|---|---|---|---|---:|---:|---:|---:|
| 0 | naca_sa | 1x1 | newton_coupled | DERIVATIVE_VERIFICATION | 9 | 9 | 351/351 | 0/0 |
| 0 | naca_sa | 2x2 | newton_coupled | DERIVATIVE_VERIFICATION | 10 | 10 | 747/747 | 110/0 |
