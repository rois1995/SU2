# Native adaptation — current handoff

Updated2026-10-31. Full explicit goal remains active: strictly accepted ONERA M6
Euler CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer
and usable disk restart; then dedicated BL/constraint/MPI qualification and the
corresponding RANS500k workflow. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8and
requested heights. Actual counts are separate from metric complexity.

## Authoritative current state

**TERMINAL, unsuccessful:** `euler_build166_consistent_merit50k`.
Exec62090terminal1after six-rank native time-budget failure. Only then verified
mpirun1804149received SIGTERM to prevent unchanged retry. All10owned handles
independently absent; all1396pins unchanged. No owned heavy job is live; C++ safe
to edit. Driver1804058,time1804147,timeout1804148,ranks1804153–1804158are gone.
Two initial read-only stop selectors assumed wrong DETAIL ordering/per-rank final
summary emission; both aborted before snapshots/signals. Corrected selector used
six master-gathered native records plus each rank's own exit row, reverified all
pins/identities, preserved evidence, then signalled only the launcher.

Source688e13d29a; build165/166qualification00f25cc49e.166only corrected two new
fixture assumptions; production unchanged. Build165PASS540.908s;166PASS39.154s.
Focused29cases/3154assertions PASS; connected serial145/20403PASS38.002s;
MPI6 151cases/rank PASS44.090s. CFD SHA256:
7b1d8c083b3de76893065474fecbf82792602397a468491c2eec2ac2f014de6e.
ConfigSHA5b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1,
byte-identical164; initial CFD restart also identical. Qualification is not M6 acceptance.

166construction coarsen and generations1–21match164counts/debts/edge/census fields.
Size closesgen21/3339.282native seconds,720963private cells,maxedge1.7999928685,
size0,geometry3126. Quality entry3376.033s has52476q-bad/53533J-bad, complete.
New ownership moves382827cells/14.947s. Native event max5412.469s; refine max5344.128s.
Quality1968.095s/158replicated rounds/928local quality commits does not close:
exit725096private cells,724975measured (121unmeasured),52048q-bad/52903J-bad,
geometry2940,size0; qDebt7927.75294/JDebt13877.06174. Partial debt reduction2.533%
is not acceptance. Sampled tree peak12.632GiB; steady quality8.13GiB, ample RAM.
Rank0quality upper-query1427.877s, scope-build493.183s (overlap). Peers wait.
CPU contention varies~.77–.97/rank/wall; no isolated timing comparison.
**Zero accepted M6 events/artifacts/transfers/adapted disk restarts.**

Evidence root:`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Run:`m6_runs/euler_build166_consistent_merit50k/terminal_assessment.json`,
`event1_time_budget_stop_20261031/{stop_receipt,terminal_handles}.json`, full logs,
rank stdout/resources, stage/cost/profile/provenance receipts. Raw perf stays local.
Latest pushed checkpointa36047c968(steering/live evidence); terminal checkpoint pending.

## Active connected milestone — latest user steering

Read [geometry query reuse + boundary smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md)
for both latest instructions verbatim. Terminal166confirms expensive geometry/query
work as the active computational blocker behind slow whole-field quality closure.
Implement ONE connected change across coarsening/splitting/quality:
representative expensive M6 query capture using existing facilities/evidence;
separate support discovery/preparation/traversal/repeated-trial costs; query reusable
prepared geometry/subtrees instead of copying/rebuilding whole scoped unions;
safe unchanged-face/star/certificate/query reuse with exact identity, coordinate/
association/neighbor invalidation, active-reference accounting and transient admission.
Boundary smoothing belongs in the SAME change: candidates/rejections/certificates/
accepted moves/whole-patch debt per second, unchanged context reuse, measured cheap
bounds/order, rejected-star invalidation. Measure interior separately. No blind trial
cuts, boundary freezing, mandatory CAD, independent framework or new baseline campaign.
Preserve all strict gates, construction/cache/ownership improvements and whole-patch
merit. Validate seams/holes/separate bodies/features/heights/cached-vs-recomputed/
exhaustion, one serial/MPI milestone, then immediately complete M6. Fixtures not completion.

Older complete M6 private stage shards are available (build18/19entry+observation,
build48entry/rejected-exit). Verify their hashes/authority/ABI/schema before using
as representative query input; they are NOT exact166continuation or CFD restart.
See166run/geometry_query_capture_inventory.mdand linked active handbook. No exact
166query payload was captured; counters/profiles alone are not replay inputs.

[Assessment of supplied findings](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md)
records confirmed queue/slot/cost limits, short-edge hypothesis, startup-field scope,
restart-name/cycle risks, interpolation/midpoint policies and guidance-vs-validity.
**Frozen target acceptance risk:** current-nodal PublishMetrics/field evaluation can
change the spatial P1target after topology changes; identity preservation alone
proves no immutable numerical field. Quantify/resolve required semantics before
any strict frozen-field claim. Original conservative CFD donor state is distinct.
Unrelated scheduling/coarsen/interpolation changes stay [backlog](Handbook/ADAPTATION_BACKLOG.md).

## Required reading / operating rules

ReadAGENTS.md, [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md),
[quality evidence](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md),
[cache/ownership design](Handbook/ADAPTATION_CACHE_PHASE_20261030.md).
One owned heavy workload, nice19, build2–4workers(helper3), boundedMPI6, libs1,
TMPDIRon4TBcompile-tmp; check actual host load/RAM before sustained work.
Verify exact handles terminal before pinned C++ edits; freeze code/binary/helpers/
config during campaigns. Reuse proven evidence only with runtime/input provenance.
Regular reviewable pushes authorized tohttps://github.com/rois1995/SU2.git,
branchcodex/native-3d-bl; no approval pending. Preserve unrelated dirty files/logs.
Bounded routine read-only tasks may use efficient agents; primary owns integrated
design/source review/acceptance. Current ownership_survey agent is done, no edits/jobs.

Prior history is preserved in [previous handoff](Handbook/History/HANDOFF_Codex_before_geometry_reuse_20261031.md)
and linked handbooks; historical builds/live notes never override current state.
