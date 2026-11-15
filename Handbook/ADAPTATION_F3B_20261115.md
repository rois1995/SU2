# F3b — implemented; stopped at final assessment failure

2026-11-15. **F3b is not fully accepted.** The one numerical workflow exited normally; both complete-event timer gates and the independent physical validator passed. The final assessment helper then failed with `TypeError: dict() got multiple values for keyword argument 'root'`. Stopped at this first failed assessment as requested. No helper fix, rerun, later check or second implementation was made. A5/B5 are withdrawn and untouched.

Evidence: `R/f3b_20261115`, where R is `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`. See `first_failure.json`, `final_assessment_failure.txt`, `full_workflow/status.json` and the saved event/physical receipts. Implementation checkpoint: `48f07ad1a8` (pushed). The final failure and results are preserved by the closing checkpoint.

## Implementation

Implemented reviewer handoff section 2.0b as written, using its SHA-sealed snapshot. Four production/test files changed: timer header, engine, distributed directory and timer fixture.

- Exactly two new operation names: JoinWait/`join_wait`, Clearance/`clearance`.
- 77 ordinary scalar reductions use TimedAllreduce, without World collective/election counter effects. The unique private_work_used reduction stays direct inside JoinWait. Source has 78 invocations on 77 lines; arguments/order are preserved.
- The legacy join timestamp/counter lines are unchanged. Clearance entries, contact assembly, progress/census and validate_private outer work have the requested scopes. Nested child subtraction remains exclusive; no catch-all.
- No numerical rule, collective order, cache quota, budget, memory option or acceptance gate changed. Join/balance optimization remains with the reviewer.

Build504 froze 1,543 source/build inputs. CFD SHA `2ba0d897c186cad1533ed1e1da9c1f5931b0afc077005075f6a0dad9e75dfb51`; test SHA `29217e683cd2a16111fdcc1852035712e210027da3e29cee56de360513af3f90`. Every campaign preserved source/binary/input pins. All owned job handles are terminal. No compiled source or pinned helper changed during the run.

## Ordered checks — each run once

| Check | Result | Seconds |
|---|---|---:|
| (a) Build504 | PASS, 1,543 pins unchanged; nice19/j4/libs1/4TB scratch | 490.164 |
| (a) Timer units | PASS, 9 cases / 25 assertions, including the new two-name case | 4.509 |
| (b) MPI6 first collapse | PASS, work 90,809,869; counts 480/473/346/263/469/464; zero refinements; coverage 96.465–96.687% | 330.164 outer |
| Conditional C492 | PASS, 462 splits / 304,988 positive cells; first 336 edits exactly P6c; zero refinements; 98.966% native coverage | 543.473 outer |
| Conditional 481/current-target | PASS, 56 cases / 5,943 assertions; 611,527 target assertions | 124.530 / 155.802 wrappers |
| Conditional 2D MPI1/2/4 | PASS, 39 cases per rank | 8.298 / 8.139 / 8.119 wrappers |
| (c) One two-event numerical workflow | Normal exit 0; all owned handles terminal; pins unchanged | 5,082.303 process / 5,091.350 controller |
| (c) Complete-event timer gates | PASS on every rank for both events | See below |
| (c) Independent physical workflow validator | PASS, no errors; strict mesh, transfer, continued CFD, time and memory | See below |
| (c) Final milestone assessor | **FAIL: reporting-helper TypeError at line 40; stopped** | — |
| (c) All six payloads versus P6c | **Not reached after failure** | — |

44 of 422 preserved objects changed, including the conditional checks' executed objects. Therefore all affected build503 checks were repeated once after (b), before (c); no invalid evidence reuse. Receipts are `object_reuse_decision.json`, `check_b_receipt.json`, `C492_repeat_receipt.json` and `final_checks_sequence.json`.

## Complete-event observations already recorded

| Event | Whole native max s | Nested engine max s | Reported rank peak RSS sum B | Actual points / cells | Coverage range % |
|---|---:|---:|---:|---|---|
| 1 | 2683.412 | 2596.907 | 13,950,185,472 | 146,676 / 726,442 | 96.911–97.586 |
| 2 | 1977.215 | 1926.176 | 17,696,780,288 | 168,790 / 855,760 | 97.460–97.942 |

Both events satisfy 5,400 s and decimal 20 GB limits. Reported process lifetime peaks are summed; sampled RSS is not substituted. Host 1-minute load ranged 2.33–9.96. Whole-event, engine, phase and call clocks overlap and must not be added. No controlled speedup claim follows from these times.

| Event | Rank | Coverage % | Quality gap s | JoinWait s | Legacy join s | Join difference % | Clearance exclusive s | Old-cavity exclusive s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
|1|0|97.404|19.855|552.996|552.996|0.000132|109.587|0.245|
|1|1|97.142|27.590|485.542|485.543|0.000125|121.606|0.300|
|1|2|97.586|13.634|553.177|553.177|0.000090|75.960|0.169|
|1|3|97.265|22.684|539.773|539.773|0.000102|98.016|0.236|
|1|4|96.911|30.518|635.707|635.708|0.000119|126.171|0.331|
|1|5|97.524|13.397|580.103|580.104|0.000129|75.164|0.171|
|2|0|97.525|11.989|327.289|327.290|0.000137|93.795|0.165|
|2|1|97.554|12.532|366.676|366.676|0.000121|92.996|0.233|
|2|2|97.460|13.677|202.795|202.795|0.000141|92.132|0.215|
|2|3|97.935|3.861|461.196|461.196|0.000086|43.484|0.046|
|2|4|97.942|3.518|460.457|460.458|0.000113|33.650|0.035|
|2|5|97.879|6.827|293.354|293.354|0.000159|57.089|0.101|

The 95% complete-event coverage and 1% join-agreement gates pass. The largest relative join difference is 1.5894e-6 (0.000159%). Prediction ranges are observations to compare, not extra gates: quality unattributed was 13.397–30.518 s in event 1 and 3.518–13.677 s in event 2, versus approximately 15–25 s predicted. Some event-1 and all event-2 coverage values exceed the approximate 96.5–97% prediction. No tuning was done. Clearance and old-cavity values above are exclusive; inclusive legacy times and repeated snapshots are nonadditive. Old-cavity includes old edge/field work, not only q/J.

## First failure and remaining scope

`assess_complete_milestone.py:40` constructs `dict(root=..., rank=..., **row)` for old-cavity report rows. Each row already has root/rank keys. Python raises before the helper returns its lifecycle-root aggregate or calls payload_identity. No `complete_milestone_receipt.json` was produced. The numerical outputs remain local, but all-six P6c identity and the aggregate receipt remain unestablished by this final check. This is a reporting helper failure, not a reported numerical acceptance failure.

The independent physical receipt and event timer receipts remain valid within their stated scope. They do not substitute for the uncompleted identity check. No fresh isolated disk restart was run, and reuse is not extended to unverified payload identity.

`source_and_readability_review.md` records all 78 argument tuples/order and unchanged World/join counters. New helper/closure bodies are four lines each; new functions satisfy the 80/15-line limits. Header and fixture clang-format18 dry-runs pass for baseline/current; legacy engine/distributed formatting and large-body debt remain explicit. No broad reformat or Adapt extraction.

No further work under this request after reporting the failure. Curved BL remains unaccepted (surface not ready), dedicated BL MPI and RANS500k remain open. Design8.2–8.7 require user acceptance; A5/B5 remain withdrawn.
