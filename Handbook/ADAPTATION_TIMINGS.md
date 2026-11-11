# Adaptation timings and computational work

Updated2026-11-11. Maintain this file after each completed meaningful stage or
integration result. [Current handoff](../HANDOFF_Codex.md) owns live handles;
[stage inputs/outputs and acceptance](ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md)
own the development sequence. R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

Latest completed target observation:371same saved-CFD MPI6 metric-only stage,
terminal0/1739.083s. Target remains provisional; no surface construction or usable
BL followed. Latest construction result remains364's preserved surface deadline
failure. Requested500k is nominal complexity, not a node count or acceptance gate.
Euler293's completed workflow remains preserved. 373bounded saved-target diagnostic failed in chord preparation;374diagnostic
capture build and focused checks passed; the single saved-target chord replay is running. No new global metric or CFD run.

This is the maintained timing/work record requested by the user. Update current
stage rows and the efficiency table after each meaningful completed result; retain
matched historical comparisons. Running work belongs in the handoff and must not
be presented as a completed timing. Missing counters remain explicitly unmeasured.

## Current runtime by stage

These are observed times, not forecasts. “Not reached” is not zero cost.

| Section | Latest measured wall time | CPU / synchronization | Work and useful output |
|---|---:|---|---|
| 1. Saved CFD seed and geometry | Initial geometry max1.4644s; CFD solve not separately attributed here | Geometry maxCPU≈1.3871s/rank; includes partition/migration/preprocessing | Saved303 RANS+SA mesh/reference/restart reused;61330source points/303359input tets; coarse reasonable coefficients, not convergence/wall-resolution proof |
| 2. Complete metric construction/normalization |371max1732.127s, including57.672authority setup | CPU≈1684.7–1687.5s/rank; includes possible MPI polling |3distinct evaluated fields/4normalizer requests;8192proposals/6516inside; estimate504040.450±48545.704 empirical indicator; core nodal140142.106; target suitability pending |
| 3. Coupled surface preparation |391.6188s | Measurement CPU311.9047s is nested/overlapping; native FIRST_ROW uses serial guard |122kernel work,18commits;51891cached of303310current cells; readiness incomplete |
| 4. Directions and first row | Not reached by full364 | Not measured for complete stage | Saved bounded358 row exists:5columns/25cells, strict initial geometry/height/contact pass, q-bad24/J-bad25; not a usable BL |
| 5. Successive growth | Not reached by full364 | Not measured | Physical progression/coverage on actual generated complete BL unqualified |
| 6. Outer transition and full closure | Not reached by full364 | Not measured | Metric continuation exists; usable mesh transition and strict complete closure remain unproven |
| 7. Transfer, adapted CFD, second adaptation, restart | Not reached for accepted RANS/BL | Not measured | Euler293 completed these obligations; RANS remains outstanding |

First-event geometry/metric evidence: `R/m6_runs/rans_curved364_v1/initial_cfd_metric_entry.log`,
`first_event_surface_failure_report.json`, `assessment.json`, `resources.jsonl`.
The full364 wrapper1972.886s includes setup/output/authorized stop and a later prefix
after first-event failure. It is not a pure first-adaptation time.

## Latest metric-stage detail —371

Source: R/metric_stage371_assessment.json and its terminal case. SameAF7CFD entry,
Mach-onlyL1/nominal500k/normal controls. Core export covers61330source nodes and
303359original tets; no new mesh was constructed. Actual target policy changed,
so this is NOT a pure speed comparison with364.

| Section | Maximum accumulated wall s / work | Interpretation |
|---|---:|---|
| Complete metric |1732.12698|Includes authority setup57.67186 and Hessian.53508|
| Metric build |1673.92964|Nested;364was1199.254 with a different target and1field|
| Complexity/gradation/nodal audit |1672.09312|Nested; nodal composition still unsplit|
| Retained preparation |612.37501|Includes geometry profile refreshes also timed in assessments|
| Assessment |510.41761|Overlaps preparation refresh; do not add them|
| Background / correction / reduction |.28463/.01897/.0000727|Nested; not geometry preparation or total MPI wait|
| Sensor gradation |43.15338|639accumulated sweeps,9275366owned-point visits,6200653tensor updates|
| Gradation halo / reduction |1.01036/37.55872|Nested maxima, not additive or total MPI wait|
| Geometry queries |6516initial preparation;39096trial requests|3distinct fields;13032trial requests in364|
| Background determinants / correction queries |20021694/19548|6donor index builds;0donor searches during density assessment|

Geometry work rank0–5:12371453/15620824/14312446/13305466/9866131/5823393;
sum71299713, max/min≈2.68. Versus364work53473769(+33.3%), metric build+39.6%,
with3distinct evaluated fields versus1. This establishes no efficiency gain/loss
for an unchanged target. Successful tangent recomposition primitives alone did
not eliminate actual M6 profile geometry refreshes; cause/reuse safety needs diagnosis.

Provisional composed estimate504040.450±48545.704empirical indicator (no bound),
tangentfactor.28717458874925855. Core nodal complexity140142.106;50.42%ofthat
core-only nodal measure is beyondradius20. Do not call it composed allocation.
Core gradation reaches256sweep cap with0directed edges over1+1e-5; fixed point not
proved. Composed audit133745bad directed edges/worst2.71976e6 (364:293237/796263).
Those counts do not certify continuous target conformity or BL suitability.

Peak sampled process-tree RSS3408777216B(≈3.17GiB), largest sampled process
605880320B(≈.56GiB); neither is a true physical whole-job peak. Geometry-reservation
peak10673107716B(≈9.94GiB) is ledger admission, not RSS. No resource pressure stop.

## Metric-stage detail — full364

| Section | Wall maximum | Work / interpretation |
|---|---:|---|
| Mach Hessian |0.40331s | Within complete metric stage; not the dominant cost |
| Metric build |1199.2540s | Includes normalization, composition and audit |
| Geometry rule/setup |1.25337s | Nested metric-build section |
| Complexity + gradation + composed audit |1197.98374s | Nested build section |
| Retained proposal preparation |251.27573s | Nested;6516preparation geometry point requests |
| Composed assessment |160.40929s | Nested; includes profile preparation/refresh |
| Background integration |0.093245s |6673898reported determinant evaluations; nested |
| Correction density evaluation |0.006216s |6516correction queries; excludes geometry preparation |
| Assessment reduction |0.00002244s | One reported reduction timer, not total MPI wait |
| Sensor gradation |≈0.041425s |1sweep/0updates; this does not certify composed gradation |

Geometry work by rank0–5:
`10136289,12188969,10368433,8959758,7598151,4222169`;
sum53473769units, max/min≈2.89. Work includes charged cache maintenance and is
not a count of unique geometric regions. Trial requests13032(full/base), rank-profile
refreshes6, one trial. Existing counters do not split every successful closest query
into seed/root/refinement work; failure-only splits must not be extrapolated.

Do NOT add the nested/max timers above. RankCPU includes possible MPI polling.
Join-call duration is not total MPI wait. Full busy-versus-wait attribution is a
recorded gap; no derived wait percentage is justified here.

Frozen371source review identifies one concrete unsplit section inside that total:
after normalization, CSolver forms the composed tensor once at EVERY local node
(owned + halos) for the composed-gradation audit. This work is outside the retained
complexity preparation/assessment timers but inside complexity_gradation_audit.
Final donor statistics reuse sensor-only tensors and do NOT repeat this composition.
No separate duration/work delta exists for the nodal composition in current evidence;
it is a candidate attribution gap, not a proved fraction of the1199s total.


## Surface-preparation detail and matching-window history

Same saved303 CFD/authority/normal constraints; construction remains validity-first.
Rows below are failed attempts under the same approximately391.6s surface window,
not accepted-adaptation timings. Geometry-work units and kernel-work units differ.

| Build | Surface wall s | Seed wall s | Seeded / cached cells | Kernel work / commits | Geometry work | Core requests / edge evaluations |
|---|---:|---:|---|---|---:|---|
|359|391.5928|Not separately recorded|cached17540|0/0|273738|Not recorded in its report|
|362|391.5997|327.4159|49280/49346|99/14|1944277|203168/296298|
|364|391.6188|311.7963|51808/51891|122/18|2213641|181894/284869|

364 seed batches6476;3626160. Both report zero core cache hits and edge reuse in
this surface report; these are not all geometry-cache hit counters. Composed-field
calls0 here: intrinsic guidance still performs expensive geometry work. In364,
seeding occupies79.62%ofsurface wall time. Measured q/J were intentionally omitted;
zero listed q/J failures in a partial construction capture is not shape closure.

Sources: `R/m6_runs/rans_curved{359,362,364}_v1/assessment.json` and their preserved
first-event reports. Actual364 partial mesh61312used nodes/303310tets; no complete
geometry/shape/size census. Work must ultimately be judged against readiness,
coverage and accepted-mesh progress, not commits alone.

## Main efficiency changes with evidence

| Change | Demonstrated computational effect | Limits |
|---|---|---|
|329 bounded optional cubic-cache maintenance|At most64optional cache work units per closest query, preserving original certification cap. Previously captured328query now completes in5791units;325control1314units. Both retain original2e-12query error and16384cap.|Primitive work/correctness gain; not a full-target timing comparison. See [complexity evidence](ADAPTATION_CURVED_BL_COMPLEXITY_20261110.md).|
|360–362 unique-node surface guidance and incremental seeding|Cached measurement coverage17540→49346cells(2.81x) within matching391.6s window; enabled14commits instead of none.|Connected changes, so gains are not apportioned between them. Readiness still failed; no final q/J improvement claimed.|
|364 existing-batch planning reuse|Versus362:5.13%more seeded cells,5.16%more cached cells,4.77%less seed wall time; requests203168→181894(−10.47%), edge evaluations296298→284869(−3.86%).|Full metric work/target IDENTICAL; surface trajectory differs. Modest wall differences remain contention-sensitive. Surface readiness still failed.|
|364 bounded matching patch controls|Root20-cell seed0.55455→0.37551s; smooth14-cell0.30381→0.11588s; exact cached/single-cell edges and flags agree.|Controls, not global speedup. Related failed363fixture and source are preserved; compare engine seeding, not differently grouped direct calls. See `R/seed364_assessment.json`.|

Full362→364 metric wall1253.731→1199.254s with EXACT same per-rank geometry work,
composed complexity and byte-identical metric-entry restart. This establishes NO
metric work reduction; do not credit it as an optimization speedup.

The new tangential-profile/recomposition API has limited tensor checks but NO
established production speedup or accepted-mesh benefit. Successful cache reuse
counters and lower estimated complexity are not efficiency milestones. Add a gain
only after matching work/progress and scope are demonstrated.

## Preserved Euler comparison — no material speedup established

Euler293 remains the completed workflow baseline. The296 boundary-reuse replay
is retained as a useful negative performance result, not credited as a major gain.
At complete census round24512, both have733163cells,10q-bad,4J-bad,945geometry-bad
and0size-bad, with matching complete/cache flags:

| Measure |293|296| Interpretation |
|---|---:|---:|---|
| Quality elapsed s |2006.0965|1972.9508|1.65% lower wall, contention-sensitive |
| Net q-bad removed/s |13.4799|13.7064|Same census trajectory; no material causal speedup proved |
| Charged upper-query work |1135505146|1135505146|No work reduction |
| Certification traversal work |1002244651|1002244651|No traversal reduction |
| Boundary candidates / accepted moves |374616/16416|374616/16416|Identical work/output at this census |

Source: R/m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k/
quality_comparison293.json, whose hashed source prefixes identify the compared
complete windows. This census is before final closure; it is not a whole accepted
adaptation timing. The preserved Euler293 completion and disk-restart evidence
remain authoritative for acceptance. Join timers are not total MPI wait.

## Development checks — separate from production timings

| Check | Wall s | Scope/result |
|---|---:|---|
|366focused serial|4.8622|393assertions/8cases, pass; limited tensor/profile behavior|
|366five actual original contexts|103.9693|306735assertions/1case, pass; density/shape controls, not normalized target or BL acceptance|
|368focused serial|5.8376|420assertions/9cases, pass; includes refreshed full/base integration and analytic oblique-advancement distinction|
|368saved actual row/neighbor comparison|138.1867|306789assertions/1case, pass within diagnosis scope;25-cell row/84-cell union, not a usable BL|
|369read-only saved-row sensitivity|0.0349|Reuses25cells/tensors; independent q/J agreement1.67e-16. Controls only, not production timing.|
|369target-only current geometry/directions|78.3906|303453 assertions/1case, pass;42surface samples across5actual M6 contexts; no construction or CFD advance. Uniform-core diagnostic, not the normalized CFD target.|
|370joint-normalization focused serial|75.7528 wrapper;71.33 test process|1096assertions/16cases, pass;70.94s userCPU includes synthetic planar normalization. No production timing claim.|
|370connected MPI6|5.4247 wrapper;1.12 test process|841assertions/16cases EACH rank; synthetic joint-scalar solve is serial-only, so this is not a speed comparison.|
|370actual saved-CFD metric-only|65.9329 wrapper;61.85 process tree|Failed first coarse trial: D1 intersection sum not SPD; pins intact, no CFD advance/construction. No credible target/checkpoint.|
|371endpoint correction serial|68.6061 wrapper|1096assertions/16cases, pass; production CSolver-only correction. No speedup attribution from this test.|
|371actual saved-CFD metric-only|1739.083 wrapper;1732.127 metric|429364assertions across6ranks, pass; exact conserved values unchanged and independent Mach error6.66e-16. Provisional target, not accepted BL.|
|372test-only diagnostic build|18.1463|Failed Tensor/ApplyPoint type mismatch; no production source change.|
|373test-only diagnostic build|198.2218|Pass; production CFD binary byte-identical371.|
|374diagnostic-only build|202.2323|Pass; diagnostic replay context/counters only, no algorithmic speedup.|
|374focused chord/normalization serial|257.5903 wrapper|424assertions/4cases, pass; includes expired-budget replay and existing curved chord cases. Not production performance.|
|373actual saved-field target diagnostic|355.9990 wrapper;294.8753 first context|Failed atlas deadline;20centroid queries/compositions complete;2edge integrations,2source pieces,293.9426s edge geometry,0edge compositions.7associated wall samples; no construction.|

369 measured context work/time: failing-chord79units/0.12217s, smooth-upper129/0.17698s,
leading-edge130/0.20864s, symmetry-interface59/0.08340s and sharp-edge candidate130/0.20092s.
These local timers exclude shared setup and direction preparation and do not account
for the full78.3906s wrapper. No matched production speedup is established.
Receipt: `R/geometry369_assessment.json` (terminal0, source/input pins intact,
owned handles terminal). Sampled largest individual RSS1.35GiB is not whole-job peak.

Build times are not adaptation timings:365failed294.321s(test compile type),
366passed308.338s,367failed270.292s(test-only unsupported reduction type),
368passed68.130s(incremental test correction),369passed244.469s(test-only diagnostic exit). No full workflow was repeated for
these development checks. Current handles remain in HANDOFF_Codex.md.

## Comparison and maintenance rules

For each completed meaningful stage, update its current row and link immutable
receipts. Preserve older comparable rows when they explain an efficiency change.
Record source/binary/input/helper hashes, ranks/threads, constraints, limits, target
policy and actual mesh size. Match stages or fixed work windows; report trajectory
changes. Mark provisional/partial outputs and uncertainty explicitly.

Keep wall, rankCPU, busy time, join/collective timers, kernel work, geometry work,
query counts, commits and useful progress separate. Attribute overlap explicitly;
mark absent measurements instead of estimating them from CPU. Under contention,
prefer work and useful-progress counters alongside wall time; report host load and
available memory from existing resource receipts. Use one low-priority owned job
and single-threaded libraries. No new profiling campaign is needed just to populate
a missing entry.

Full364 sampled summed process-tree RSS maximum5784432640B(≈5.39GiB), not a true
physical-memory/whole-job peak; shared pages can be counted more than once.
Geometry reservation12879626240B is ledger admission, not physical RSS. These
quantities must remain distinct when judging resource pressure or performance.


Latest demonstrated stage2blocker (373): the FIRST20-cell chord-preparation query
consumes293.94s before any edge integrand composition,4.836754Mcontext geometrywork,
under the existing300s atlas deadline. This is a direct section measurement, not
an MPI wait attribution. Wrapper CPU350.34s/user +1.00s/system versus351.67sprocess
wall; serial probe, no memory pressure. The measured cost lies in geometric cut
preparation; exact repeated queries versus conservative refinement remain unknown.
374adds bounded failure-only context/counters; no optimization or timing gain yet.
