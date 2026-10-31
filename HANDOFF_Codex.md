# Native adaptation — current handoff

Updated2026-10-31. Full explicit goal ACTIVE: strictly accepted ONERA M6 Euler
CFD→mesh→CFD→mesh at50k metric complexity, conservative admissible transfer and
usable isolated disk restart; then dedicated BL/constraint/MPI qualification and
corresponding RANS500k workflow. Actual point/cell counts are separate from metric
complexity. Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8and requested heights.

## Authoritative current live state

**No owned heavy job live.** Connected MPI6 exec28854TERMINAL0/278.590s;
198cases/rank PASS (assertions30176/32916/30935/30408/30878/30413 in log order).
All9sampled host handles absent, all runtime/source pins independently rehashed.
Case:staged57/cases/geometry_reuse174_mpi6_20261031T022606_2230896.
Qualification summary:staged57/geometry_reuse174_integration_summary.json.
Next immediate action is full M6 launch below after checkpoint/push and host check.
No further C++ changes planned; keep build174source/binary coherent.

Build174PASS54.150s,1296pins unchanged/all13sampled handles absent. Focused serial
`geometry_reuse174_serial_20261031T022425_2226738`PASS45cases/3674assertions,
46.510s, all1388pins unchanged/all4handles absent. Includes fixed1M-work noncoplanar
graph, holes/notch/separate bodies, cache/recomputed/refusal, smoothing/heights and
exact saved M6 queries. MPI uses previous166qualified connected filters plus
NativeMappedComponent3D/NativeCurvedAtlas3D/NativeGeometryBackend3D; hidden M6 query
replay is serial only. Build174CFD SHA256:
60ffadf8e8c68b6771b940e72b510b62088da6f560679cb825e8ff31477d3a13.

Latest pushed498277b1a4contains source through171. Next checkpoint includes current
qualified174source: narrow existing-trace diagnostics plus overlap-conditioned plane
lift bound. Full M6 is still pending; no accepted M6 event/artifact/transfer/restart.

## Connected candidate and evidence

Read [latest user specification + design/evidence](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md).
Shared coarsen/split/quality backend reuses per-facet distance searches and smaller
joined witnesses. Full exact support remains in lower-distance forest; witness
keys use exact immutable selected pieces/resolution/tolerance, answer keys retain
full support/coordinates. Active refs/accounting and transient admission preserved.
Boundary smoothing reuses unchanged private context/atlas with retained overlap
charge; exhaustive shape-only rejected-star cache invalidates on EVERY commit,
never after resource/validity/geometry/edge failures. Boundary/interior trial/reject/
guidance/move/debt/time counters are separate. These count PRIVATE optimizer work;
published operations and whole-field debt remain separate. No blind trial cuts,
boundary freezing, mandatory CAD, new scenario switch or gate relaxation.

Prepared plane lift bounds now restrict flat-source barycentric weights using
outward projected half-plane coefficients + admitted XY shift, then a simplex-box
dual bound on ±normal residual. Keeps finite footprint/holes, displacement, model
error, full-support distances and all caps. Primary proof + bounded read-only review.
This resolves graph certificate's fixed1Mwork failure; no cap increase.

Representative12M6 projected-star queries (older verifiedbuild48authority/data,
NOT exact166state/continuation/restart): payload bit-identical, all12passing classes
preserved and cached/recomputed equality. Query1.03543→.401954s OBSERVED, not isolated
speedup. New facet.0832373/witness.00954483/selection.00128230/traversal.306159s;
witness724of10756support pieces. Work334868→409280 (more cheap box checks), identical
repeatwork1060. No capture/replay RSS comparison. Receipts:
`staged57/geometry_capture168_baseline_receipt.json`,
`geometry_reuse174_query_comparison.json`, pinned serial174output.

One unsuccessful convex-hull-only fallback172was preserved then removed: same graph
failure, slower M6sample.173trace proves all2048model/128physical pieces form one
accepted graph/full witness, no footprint/storage refusal; fragmentation hypotheses
disproved for that case.174addresses loose plane extrapolation. Exact172/173source
archives and trace receipts preserved locally, must include in next checkpoint.

An overbroad generic serial_full170was stopped after preserved assertion failures:
1388pins/4exactidentities verified, only timeout2146037signalled, terminal143/270.596s,
all4handles gone. NOT a passing campaign. Older source-envelope/count/timer-scope
expectations outside166qualified filters are assessed in active handbook/backlog;
do not fix unrelated fixtures or rerun generic serial_full in this cycle.
Relevant work-estimator fixture171nowexplicit16MiB + actual retained-piece assertion,
keeping strict work>dand rank-independent ownership checks; production quotas unchanged.
Serial171192/193cases passed (graph failed);174focused resolves that shared failure.

## Next actions

1. Let MPI174finish frozen. Inspect actual result, rehash pins and independently
   verify ALL sampled host PIDs terminal before any new job/source edit. If failure,
   preserve/fix only demonstrated connected blocker; do not claim validation passed.
2. On pass, save combined qualification/provenance, checkpoint/push reviewable source
   and receipts, then IMMEDIATELY full M6 (fresh attempt directory):
   `python3 integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/run_m6.py build174 euler_build174_geometry_reuse50k 2`
   Check actual host load/RAM and no owned handles before launch; source/runtime/input
   pins authoritative. Existing config/inputs endorsed50k,2events,native5400s bound.
3. Compare matching coarsen/split/quality geometry work, useful debt/time progress,
   transition/cache costs and aggregate sampled memory with terminal166. Nested timers
   overlap; old scope timer included facets. Do not infer speedup from changed CPU
   service/quotas or equate target complexity to actual counts.
4. Require TWO actually accepted events, strict final meshes/reference publication,
   conservative/admissible transfer and finite continued CFD. After workflow terminal,
   isolated COPY of actual accepted final mesh/native_ref/matching solution, verify
   headers/coordinates/positive state, then actual RESTART_SOL=YES/ADAP_LOOP=NO run
   with distinct output basename. Initial61330point restart is not adapted evidence.
5. Then dedicated curved/junction/interface/subsequent-op/redistribution/height BL
   qualification under MPI, implement its demonstrated needs, and RANS500k workflow.

## Last actual M6 outcome / persistent risks

`euler_build166_consistent_merit50k` TERMINAL unsuccessful. All10owned handles absent,
1396pins unchanged. First event native time_budget before authorized stop of only
verified mpirun1804149. No accepted mesh/transfer/adapted restart. Full failure and
stop evidence under staged57/m6_runs/euler_build166_consistent_merit50k.
Construction matched164through21: size closes3339.282s/720963private cells. Quality
entry3376.033s:52476q-bad/53533J-bad/3126geometry-bad. Eventmax5412.469s; quality
1968.095s/158rounds/928commits only2.533%partial debt reduction;121unmeasured cells.
Rank0quality geometry upper1427.877s includes493.183scope-build; peers wait. Peak
sampled tree12.632GiB, steady8.13GiB, ample RAM; CPU contention .77–.97service limits
wall comparisons. Initial CFD one iteration is functional workflow, not converged flow.

[Supplied findings assessment](Handbook/ADAPTATION_FINDINGS_ASSESSMENT_20261031.md)
records queue/slots/short-edge hypotheses, startup-field scope and restart risks.
**Acceptance risk still open:** current-nodal tensor transport can change spatial P1
field after topology changes; identity preservation alone does not prove immutable
numerical target. Quantify/resolve required semantics before strict frozen-field
claim. Original conservative CFD donors are distinct. Loop resume can collide with
restart basenames and rejected events consume cycles; isolated restart must avoid
collision, two requested events never imply two accepted events.

## Operating rules / links

ReadAGENTS.md, [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md),
[quality evidence](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md),
[cache/ownership design](Handbook/ADAPTATION_CACHE_PHASE_20261030.md), active handbook.
One owned heavy workload,nice19,build2–4workers(helper3),MPI6bounded,libs1,TMPDIR4TB
compile-tmp. Verify terminal handles before pinned C++ edits. Reuse evidence with
source/build/binary/runtime/config/input provenance. Preserve unrelated dirty logs.
Regular pushes authorized tohttps://github.com/rois1995/SU2.git,
branchcodex/native-3d-bl; no approval pending. Use git -c submodule.recurse=false and
--ignore-submodules=all due unrelated broken Eigen worktree. Force-add ONLY whitelisted
evidence, never binaries/meshes/restarts/raw perf. Escalated host/git tools authorized.
Bounded efficient agentownership_surveyis done, no edits/jobs. Primary owns acceptance.
