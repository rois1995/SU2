# F3 exclusive adaptation timers — 2026-11-15

Status: **F3 is incomplete. Its first complete-event coverage check FAILED.** The ledger reconciles, but named coverage is68.357–74.357%, below95%. The owned workflow was stopped at that first failure, all handles are terminal, and no repair/retry was made. Source checkpoint0a80893973 is pushed. Curved BL/RANS remains open.

Evidence root R/f3_exclusive_20261115, where R is integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57. Current source is frozen for build503 (1543 source/build inputs). CFD SHA256 f607ff3ea35d7ebd59e59016c0dedaba21be39fc246cb9b653c4a85281bf934a; test SHA256 bb2364859858fb274c2b2418967672c3b0c69796af246c77bfd26f6d2020aedc.

## What the tree measures

One fixed-storage, thread-local ledger records root → phase → operation. Lexical nesting subtracts the full child interval from its parent. Repeated operation keys aggregate exclusive time; inclusive call time is explicitly nonadditive. Phase-only and root residuals remain unattributed and are excluded from coverage. Each local row names root, boundary and rank; no reporting collective is added. Repeated boundary snapshots are cumulative, not additive. `final` is distinct from the live `exit` snapshot.

The existing timings/performance flag enables the tree. OFF performs no timer clocks, heap allocation or output. Storage is 3736 bytes per Session plus lexical stack frames; owning native roots account for the ledger through existing caller admission. No numerical expression, query purpose, cache quota, algorithm, configured budget or acceptance gate changes.

| Lifecycle/service | Instrumented work |
|---|---|
| Metric | Sensors, Hessian, target composition; nested reference/mobility preparation |
| Preparation/import | Geometry import, donor indexing/import, mobility policy, entity geometry, target-checkpoint restoration |
| Construction/surface/quality | Existing selection/planning, candidates, topology/publication, actual census, field composition and geometry services |
| Old-cavity measurement | Optimizer old merits/direct old-shape scans plus generic old metric comparison; includes old edge/field work, not a pure q/J arithmetic microbenchmark |
| Communication | Existing World elections, reductions and routed exchange protocol, including packing/waiting; this is wall time, not inferred MPI CPU |
| Final proof | Existing geometry/height certification and final completeness work |
| Return/normalization | Post-engine complexity reassessment, normalization and reader slices; nested mobility remains visible |
| Replace/output | Mesh/solver import, geometry checks, solution import, finalization, adapted-mesh/reference output |

Each root has its own explicit denominator. The serial fixture's loading/observation/reporting outside production-service roots is reported separately; no whole-process 95% claim is made from narrower roots. Existing legacy counters remain available and overlap; do not add them to the new exclusives.

## Validation so far

| Check | Result | Seconds |
|---|---|---:|
| Geometry + timer units, build502 | 87 cases / 20603 assertions pass; eight timer cases include nesting, same-key recursion, live snapshots, unwinding, phase restoration and OFF silence |162.665|
| Final changed metric + timer checks, build503 |47 cases /1475 assertions pass; six geometry/timer object hashes remain identical to502 |4.542|
| Six-rank first collapse, build502 |Exact90809869work;480/473/346/263/469/464;0refinements; all owned handles terminal after prescribed stop |330.222outer;233.337max native|
| Serial saved C492, build503 |471splits;305010positive private cells; first336edits exactlyP6c;0refinements |542.483outer;389.899engine root|
|481merit |56cases/5943assertions pass |120.604test;122.428wrapper|
|Saved current target |611527assertions pass |144.537test;150.094wrapper|
|Native2D1/2/4ranks |39cases/rank, all pass |8.212/8.235/8.230wrappers|
|Full two-event Euler milestone |First native event accepted; complete-event coverage FAILED, stopped before event2 |2862.811outer until terminal;2659.995max native|

The extra generic old-comparison scope was added after the bounded502 collapse, without changing its numerical expression. Final-source exact collapse counts/work are therefore enforced again inside the already-required full milestone, not inferred from the old binary. Earlier successful geometry evidence is supported by unchanged object hashes and runtime pins; the changed metric path was checked on503.

Measured named coverage: collapse root95.571–96.014% across six ranks; completed metric roots99.9978–99.9985%. Serial final engine98.3562%, with6.4093s unattributed; target-checkpoint root99.9999985%. Row sums, root residual and coverage equations pass. Serial root totals420.6171s leave121.8655s of fixture/observation/reporting outside those roots. This is a scope limitation, not named timer coverage.

Serial BL lifetime peak RSS is3,323,510,784B (GNU time), below20GB decimal. Other completed serial/MPI checks retain exact serial or conservatively bounded descendant peaks. The intentionally terminated collapse has no final native peak row; sampled VmHWM receipts are preserved and are not asserted to be the final rank-peak sum. The full milestone must pass the sum of native-reported rank lifetime peaks<=20,000,000,000B for each event. Native limit remains5400s per event; modeled12288MiB/rank is unchanged and does not enforce the global gate.

No causal speedup is claimed from elapsed differences or unequal completed work. The BL surface remains unready with incomplete census; these positive private cells are not an accepted BL mesh.

## Source and readability

Initial compile500 failed in40.148s because OptimizeOptions has no timings member. That failure/patch is preserved. Read-only review also caught live reporting using stored instead of snapshot rows and an ambiguous final boundary label; both were corrected before numerical validation and covered by tests. Compile501 passed500.128s; completing lifecycle roots before numerical checks required incremental50270.145s. Completing the old generic comparison scope required50320.146s. No failed numerical check was retried and no numerical recovery variant was added.

New header functions are<=80lines (largest45); the new old-comparison closure is6lines. Full clang-format18 dry-run passes the new header/fixture. Existing large-function/format debt is explicitly documented in `readability_report.md/json`, including ten longest functions and baseline failures. Narrow instrumentation leaves existing algorithm extraction for the reviewer-approved design sequence; no premature Adapt refactor is claimed.

Detailed receipts: source_freeze.json, build_status.json, reused502_evidence_receipt.json, collapse_timer_receipt.json, BL_timer_and_numerical_receipt.json, final_checks_sequence.json and per-case pins/status/resources. Raw payloads stay local with SHA seals. Complete-event failure, terminal/source/resource and gap receipts are described below. A corrected reviewer proposal is required before another F3 change/run.

## Complete-event failure and stop

Event1 completed natively on all6ranks in2659.952–2659.995s, with q/J/size/geometry bad counts0 and726442 global cells. The published mesh and reference match P6c byte for byte. Native reported rank peak-RSS sum is13,923,971,072B, passing20GB. Continued CFD began on the transferred solution; its matching event1 restart had not been written when stopped. Event2 did not start. Full transfer/continued-CFD/second-event/restart acceptance is NOT claimed.

All row-sum, root-residual, nonnegative-record and coverage-equation checks pass. Only named coverage fails, on every rank. Times below are exclusive wall seconds; the root residual is outside all operation rows. Phase tags follow lexical service/round scopes, not a sampled EnginePhase variable.

| Rank | Coverage % | All unattributed s | Quality unattributed s | Construction unattributed s | Root residual s | Old-cavity calls / exclusive s |
|---|---:|---:|---:|---:|---:|---:|
|0|72.5375|730.513218|666.968689|53.529259|10.015269|265232 / 0.245839536|
|1|74.3565|682.125805|619.987385|52.155777|9.982643|311385 / 0.294138044|
|2|73.8931|694.453263|626.325130|58.191887|9.936245|205637 / 0.167606595|
|3|73.3386|709.204988|645.555865|53.720672|9.928450|301804 / 0.233208408|
|4|68.3573|841.708407|773.172399|58.581730|9.954277|333516 / 0.333073803|
|5|73.0044|718.092670|650.377998|57.862604|9.852067|178067 / 0.190869797|

Old-cavity measurement is0.168–0.333s/rank for178067–333516calls; it includes old edge/field work and is not a pure q/J arithmetic measurement. Old-edge limit scans in smoothing also remain outside this node; it is not an exhaustive old-cavity cost claim. It cannot explain the620–773s/rank quality residual. Rank0 named examples: final-proof geometry bounds410.952s/31622calls; quality geometry bounds284.795s/114719calls; quality communication267.937s/425504calls. Inclusive call totals overlap and are not added.

The residual is measured by phase, not assigned to an unmeasured function. `gap_source_review.md` records exact uncovered source regions and distinguishes omissions from unproved cost attribution. No broad engine catch-all was added to manufacture coverage, and the95% target was not weakened.

The handoff says: “For each item: implement the change as written, run its checks once, and report. If a check fails, do NOT add a second change of your own”. Accordingly, the verified owned application group691205/start24495741 received SIGTERM after the complete-event coverage result. Its controller was left alive to seal receipts; all owned handles were then verified dead and all1543source pins/both binaries unchanged. Raw controller `stop_reason=normal_process_exit` means its child exited, not successful completion: exit=-15 and `coverage_first_failure_stop.json` record the intentional failure stop. No source change or numerical retry follows this failed check.

Authoritative receipts: event1_complete_coverage_receipt.json, event1_gap_attribution.json, event1_numerical_resource_receipt.json, final_source_collapse_receipt.json, terminal_source_resource_receipt.json, and full_workflow/{status.json,coverage_first_failure_stop.json,final_source_first_collapse_gate.json}. Large payloads remain local and SHA-sealed. Full workflow acceptance is false because it was intentionally stopped; this is a timing-instrumentation acceptance failure, not a reported mesh/time/memory failure of event1.
