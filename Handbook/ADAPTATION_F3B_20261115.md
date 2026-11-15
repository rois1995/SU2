# F3b — reviewed timing correction, 2026-11-15

Status: build,checks(a)(b)andallchanged-objectconditionalchecksPASS. The single full two-event check(c)is active(tool96758). No F3b acceptance claim yet. A5/B5 are withdrawn and untouched.

Evidence: R/f3b_20261115, R=integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57. Reviewer source is the SHA-sealed handoff snapshot in that directory, section2.0b and section3step3. Base source856b89ccb4; current build504 has1543frozen source/build inputs. CFD SHA2ba0d897c186cad1533ed1e1da9c1f5931b0afc077005075f6a0dad9e75dfb51; test SHA29217e683cd2a16111fdcc1852035712e210027da3e29cee56de360513af3f90.

Implementation is the proposal's connected instrumentation change only:

- Exactly two operation names: JoinWait/`join_wait`, Clearance/`clearance`.
- 77 ordinary scalar reductions use the new TimedAllreduce helper. The unique private_work_used join stays a direct reduction inside JoinWait. There are78invocations on77source lines because one line has two calls; arguments, order and return types are preserved.
- World collective/election counters and the legacy join timestamp/counter lines are unchanged. The wrapper names wall time without routing through World.
- Clearance/clearance-batch entries, contact-list assembly, progress/census work and validate_private outer work have the requested scopes. Nested operations still subtract child time; no engine catch-all or additional operation.
- No numerical expression, collective order, join behavior, cache quota, memory option, budget or acceptance rule changes. The reviewer retains the balance/join-wait design.

## Ordered checks

| Check | Result | Seconds |
|---|---|---:|
|(a) Build504 |PASS; all1543pins unchanged; nice19/j4/libs1/4TBscratch |490.164|
|(a) Timer units |PASS9cases/25assertions, including the single new two-name case |4.509|
|(b) Six-rank first collapse |PASS90809869work,480/473/346/263/469/464counts,0refinements; every rankcoverage>=95%; allhandles terminal at prescribed boundary |330.164outer|
|Conditional C492 |PASS462splits/304988positivecells,first336EXACTP6c,0refinements;native98.966%coverage |543.473outer|
|Conditional481/current-target/2D1/2/4 |AllPASSonce:56cases5943assertions;611527targetassertions;39cases/rank2D |124.530/155.802/8.298/8.139/8.119wrappers|
|(c) One full2event milestone |Active once; complete-eventcoverage/join/P6cidentity/time/RSSpending |—|

Check(b)coverage96.465–96.687%. Build503object comparison changed44of422objects, including engine/directory, the C492fixture and executed2Dobjects. Therefore none of the affected503checks is silently reused: repeat each once after(b), before(c). object_reuse_decision.json records exact paths/hashes. Source remains frozen throughout validation; stop first actual failed check without another change/run.

## Complete-event checks and predictions

Check(c)must establish coverage>=95%for every complete event/rank; JoinWait versus that rank's sum of all legacy join_call_seconds windows within1%; all six mesh/ref/matching-restart payloads byte-identical to P6c; native<=5400s/event and sum of reported rank lifetime peakRSS<=20000000000B/event. The existing independent physical/transfer/continued-CFD validator remains required. A missing rank/event/exit/counter fails the report. Sampled RSS is not the gate.

The controller checks completed-event coverage/join automatically and stops the verified owned run on its first failure. It preserves the original refinement/time/memory/mesh gates. New analysis helpers are pinned before launch. Parser self-checks exercise two events, one-percent/zero/nonfinite joins, missing ranks and embedded MPI prefixes. A read-only replay of the failed F3 logs reproduced the reviewer's legacy totals:550.042468/483.305393/550.983788/536.330461/631.703344/575.210202s,29windows/rank.

Report predictions alongside observations without tuning: quality unattributed15–25s/rank, coverage96.5–97%. These ranges are predictions; the explicit acceptance gates remain95%coverage and1%join agreement. Inclusive call times, repeated snapshots and overlapping legacy counters are not additive. Separate production-root coverage from the serial fixture's loading/observation/reporting overhead.

## Source/readability and remaining work

source_and_readability_review.md verifies all78argument tuples/order, unchanged World/join counters and only the prescribed scopes. The new helper body is4lines; the join closure4lines; new functions obey80line/15line limits. Header/fixture clang-format18dry-runPASS at HEAD and current. Legacy engine/distributed formatting and large-body debt remain explicit; narrow instrumentation does not perform the design's later Adapt extraction.

No numerical retry or independent timer variant is authorized after failure. Retain every receipt and phase/call-site count, update current handoffs/timings, and push reviewable checkpoints. F3bcomplete=false until every required check passes. Curved BL readiness, dedicated BL MPI, RANS500k and design8.2–8.7 remain open; no implementation of the latter before user acceptance.
