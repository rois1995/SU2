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
capture build/focused checks passed; replay374failed at its original atlas deadline.
375completes breakpoint preparation but not the chord. Shared certificate380is
terminal with more integrand progress but no complete chord; no global metric/CFD rerun.

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

380shared closest-certificate pool: the same blocked actual query completes
preparation in191.679s versus375's242.075s and reaches4534rather than1925integrand
evaluations before the same deadline. This is a demonstrated bounded-query progress
gain; different point/partition trajectories and incomplete final length prevent
an accepted-adaptation or fixed-work speedup claim. Details below.

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

##374 completed chord attribution and375 hypothesis

Same saved371target recipe and exact first20-cell context; terminal1/358.197s,
pins intact, actual-host owned handles terminal. Source: R/geometry_target374_assessment.json.

| Work | Measurement | Meaning |
|---|---:|---|
| Chord geometry preparation |293.28856s|0edge integrand compositions |
| Point-projection requests |10627|3687classified intervals; includes midpoint/endpoints |
| Projection wall/work |292.58914s /4690841units|Largest single query.07851s; many queries dominate |
| Prepared cubic region reuse |4112119hits/395452misses|Geometry-only reuse, not point-result memoization |
| Prepared-region rebuilding |13.05939s|Nested in projection cost; not the principal measured wall cost |
| Remaining crossing uncertainty |1.25e-8 vs1e-8 required|Active interval7.1e-15wide, requested point error3.6e-18; incomplete |

375is an unqualified hypothesis: per-wall/per-chord exact-point successful
projection certificates, reusable only for equal/looser accuracy requests, with
original support and full candidate bounds retained together. Memo storage uses
spare existing admission; deadline/work checks remain. No target/limit/gate changes.
No hit fraction, work reduction or speedup is established yet.

##375 completed outcome — preparation progresses, complete chord still blocked

R/geometry_target375_assessment.json: actual terminal1/361.336s, pins intact and
all owned actual-host handles terminal. Build42.162s; focused serial178.614s,
424assertions/4cases passed. These check timings are not adaptation speedups.

| Same exact M6 context |374|375|
|---|---:|---:|
| Breakpoint geometry preparation s |293.2886, incomplete|242.0751, complete|
| Edge integrand evaluations |0|1925|
| Edge composition s |0|50.1013 before deadline|
| Total context geometry work |4700941|4268956|
| Final chord |Unavailable|Unavailable|

The new total work includes integrand certification; the old work does not. This
is increased useful progress under the same deadline, not an identical-work timing
comparison. No full-chord, target-stage or accepted-BL speedup is established.
Successful preparation does not export local memo-hit fraction; it remains unknown.
New prepared-region stats:3798987hits/318108misses,11.1531srebuild,198412200Bretained.
The final refusal is a composed-point query at original2e-12accuracy, not the
near-roundoff breakpoint query. Existing OptionA weight/base/full composition
paths can repeat geometry at the exact same point; safe shared certificate reuse
is the next assessment, not yet implemented.

##376–380 shared-certificate development/qualification (actual result pending)

The shared immutable geometry scopes pool now retains complete successful closest
certificates under exact point/hint keys, current outward accuracy and existing
quota/work/deadline contracts;375's separate per-chord memo is removed. This is
one connected correction to the same point-certification bottleneck. Main M6 timing
and work gains remain unmeasured until the current frozen replay terminates.

| Check | Wall s | Result/scope |
|---|---:|---|
|376build|580.7767|Pass; header change rebuilt47targets, not adaptation time |
|377build|48.1419|Pass; optional allocation fallback/current rounding domain |
|377closest check|11.0762|3cases,1018of1021assertions pass; zero answer stores due maintenance starvation |
|378build|44.1305|Pass; reserve2units WITHIN original64maintenance cap |
|378serial|323.1961|32of33cases pass;3failures all one older curved-height fixture |
|379build/check|52.1295/10.9428|Production unchanged; test's physical placement still fails second existing facet-altitude gate; cold/cached same12failures |
|380build/check|60.1408/10.8360|Production byte-identical378;1879assertions/6cases pass with both height gates and cold/cached controls |
|380MPI6|84.9852|84of85cases pass EACH rank; all changed cache/constraint cases pass; unrelated planar fixed-count case fails9assertions/rank; whole run is NOT green |

32passing378cases are retained after common-pin audit: only the corrected test file
and test_driver differ; production CFD and loaded-library/input/production-source
pins agree. MPIraw33.72MB log is losslessly archived as1.79MB gzip with roundtrip/hash
receipt; see closest380_qualification.json. No exact cache-hit fraction or accepted
mesh speedup is claimed from these fixtures. Native MPI BL generation remains open.
The actual380replay is the SAME first20-cell saved context/target recipe and original
300s atlas deadline; its final work/result will determine the next action.

##380 actual saved-target result — fresh certification remains dominant

Same first20-cell context and exact371selected core/fresh policy recipe. Terminal1/
362.580s, pins intact, actual owned handles terminal; R/geometry_target380_assessment.json.

| Measured work |375|380|
|---|---:|---:|
| Complete breakpoint preparation s |242.0751|191.6789|
| Edge integrand evaluations before same deadline |1925|4534|
| Edge composition s |50.1013|100.3078|
| Context geometry work |4268956|4350564|
| Complete chord |No|No|

Preparation wall is20.82%lower; integrand progress2.355x at1.91%more charged total
work. More accurate reused certificates can alter certified partitions/point visits;
this is increased progress on the same blocked query, not identical work or accepted
mesh speedup. Geometry preparation and composition are separate top-level edge
phases here; prepared-region rebuild12.056s is nested, not additive.

Complete answer hits6272, misses9969, hit share38.62%;1551misses have an existing
certificate too wide for the new request. Other misses combine new/absent/evicted
entries; do not label them all nearby trials.9968stores/0refusals. Prepared regions
3836771hits/339648misses,216044768Bretained. Successful closest queries still lack
seed/root/refinement timing split; normal-candidate jitter vs true quadrature
variation is unresolved. Latest user ordering defers projection seeds: qualify whole-edge exclusion first,
then investigate purpose-aware measurement precision, retaining the full proof. More cache counters alone
will not close this case. No target/constraint/budget change or new baseline campaign.


##381 — whole-edge exclusion qualification

R/edge_exclusion381_qualification.json. Build444.505s; serial5.264s (2589assertions/
17cases); MPI6selected milestone5.792s (24cases/rank,2812–3015assertions/rank);
actual saved5-context diagnostic82.024s, all terminal/pins intact. No full CFD,
normalization or generation repeated. Actual largest sampled individual RSS
1462628352B, not whole-job peak. Qualification window wall times include launch and
resource sampling; these are not production edge timings or fixed-work speedups.

All5original saved edges remain on detailed path when complete transition/blend
support is considered. All5translated distant controls using actual M6 authority
agree exactly with the existing background tensor (three sample positions each).
These are explicitly synthetic controls, not original flow edges. Curved unit check
shows0closest-query work for the excluded path versus positive work for forced
existing fallback with identical cuts. Counters record attempts/proofs/fallbacks,
wall/edge preparation avoided and composed samples bypassed; current actual-success
`continue` skips the cumulative dump, to correct in the next diagnostic.
No saving on the blocked original chord, no accepted-mesh/time improvement claimed.


##382 — bounded edge-accuracy diagnosis, no production policy change

R/edge_accuracy382_assessment.json and edge_accuracy382_profile_controls.json.
Build238.253s; production CFD byte-identical381. Serial regression4.632s.
Saved difficult context TERMINAL1/359.211s, all owned handles terminal/pins intact;
original300s geometry deadline,600s outer timeout. Test process354.35swall,
352.70suser+1.15ssystem; single-rank99%CPU, so this sample does not exhibit a large
CPU/wall contention loss. It is instrumented test work, not production stage speed.
Largest sampled individual RSS1798021120B, not whole-job peak.

| Phase | Wall s | Charged geometry work | Useful output |
|---|---:|---:|---|
| Geometry preparation |186.895|2842933|296cuts, completed |
| Diagnostic whole GL10 |.2981|3634|estimate39.19917; uncertified |
| Widest cut interval, unchanged integration engine |106.016|1692736|7384evaluations; deadline refusal, no reference scalar |

The whole GL10error and widest-interval GL10error remain UNKNOWN. No accepted edge,
insertion point, complete patch or BLmesh was produced. Prepared geometry rebuild
15.254s is nested in geometry query work, not additive. Existing complete answers
6069hits/13042misses/1551too-wide refinements; prepared regions3789254hits/440129misses,
436550592Bretained. More samples/cache hits do not establish useful convergence.

296cuts imply295initial G7/K15panels/4425samples for one source piece. This helps
explain substantial minimum integration work, but the widest interval alone also
refuses after7384samples: adaptive local difficulty is demonstrated. Its cause
(true variation, certified-point/normal jitter or numerical tolerance) remains
unresolved. Do not attribute all work to initial panels or deep refinement alone.
Comparison with380is diagnostic trajectory/progress, not a fixed-work timing gain.

Cheap analytic pure-normal hn controls: GL10whole error reaches1.701%; retaining
its known clamp break still gives1.472%at1mm. Near0.9/1.8threshold controls can change
action classification when cuts are omitted. Exact profile formulas do not cover
curvature/metric intersections/Option-A/fade or prove a usable M6action bound.


##383 — complete exact outside-edge integration bypass

R/edge_exclusion383_qualification.json. Build434.447s; focused serial4.648s
(2664assertions/23cases); MPI6selected connected milestone5.523s
(25cases EACHrank,2826–3029assertions/rank); actual5-context qualification78.990s.
All pins intact/owned actual-host identities terminal. Largest sampled individual
actual RSS1462792192B, not whole-job peak. These are qualification window timings;
78.990versus38182.024does not establish a speedup under varying setup/contention.

Proven-outside identity now reaches EXISTING analytic P1sensor integration/inverse,
rather than G7/K15identity samples. All5translated actual-authority controls have
exact target/background length equality and0composed edge samples. Focused varying-
anisotropic control agrees with forced numerical reference and preserves balanced
insertion; stale versions refuse. Final full-policy counters:20wall attempts,
15proofs/5fallbacks,10edge preparations skipped,15direct sample compositions bypassed.
Counts include explicit synthetic-control calls and are not original-flow savings;
analytic samples are not performed and therefore do not increment sample counter.
All5original actual edges stay on detailed path. No complete-mesh/time improvement
or accepted curved BL claimed. No gate, profile, configured limit or normal law changed.


##383 bounded point replay — accuracy attribution

R/edge_accuracy383_point_assessment.json; saved382refusal +nearby positions,
12queries only, existing probe/no code change. Terminal0/12.861swrapper,
248assertions/one case; actual-host handles terminal/pins intact. Reference load
7.085s, policy built7.423s cumulative, all queries done7.615s cumulative.2870charged
geometry work. Largest sampled individual RSS341549056B, not whole-job peak;
4625561376B is conservative admission peak, not measured process RSS.

Identical coarse/tight repeats cost1work. Other requests cost232–332work. At the
same physical point,2e-12vs2e-14distance tolerance changes foot3.8065e-8,
normal2.7683e-6; derived normal-only chord speed changes4.8824e-5relative. Coarse
nearby positions share a discrete foot before jumping. This confirms candidate-
normal discretization and distance-only accuracy insufficiency for reproducible
metric sampling; it does not quantify all original quadrature error. The tighter
answer is not an exact normal certificate. Original tangential policy here differs
from selected371policy, so full target eigenvalues are NOT production comparisons.
Selected384hypothesis reuses existing minimizer as an interval-certified upper
witness with128maximum optional charged callback units total; full certification
and unchanged total budget remain. Benefit must be established on saved full chord.


##384–385 bounded precise upper witnesses (actual chord pending)

384build466.488s fails one new test's private-API reference; production compiles,
pins intact/all155owned actual-host handles terminal.385test-only correction uses
independent long-double Bernstein evaluation; incremental build28.110s passes.
50focused serial cases/4977assertions pass63.635s. MPI6passes29cases EVERYrank
(3925–4128assertions/rank)/12.602s. No primitive campaign is BLgeneration acceptance.

Same12-point primitive replay385passes12.894swrapper/3180work versus38312.861s/
2870work. Normal differences under coarse/tight request now5.55e-17–1.67e-16, feet
≤1.99e-18.10proposal improvements/310additional charged work. This establishes
candidate stability on those points, NOT reduced fresh-query work or wall speedup.
Setup dominates these tiny runs; do not infer performance from their total times.
Source/probe stats show initial winning witnesses are vertices and skip polish;
these queries improve only after lower traversal. The original full chord is frozen
under unchanged300s geometry/600souter, source/binary/input pinned. Its outcome
must decide whether this is materially useful for stage2and the active blocker.


##385 actual full chord and386active-edge correction (full result pending)

385full chord TERMINAL1/358.142s, pins intact/all owned identities terminal;
R/geometry_target385_assessment.json. Preparation137.985s vs380191.679s,27.99%lower
wall;6219integrand evaluations vs4534,37.16%more progress before same deadline.
Composition153.566s, total contextwork4839168,430653projection work,10705attempts/
6987improvements. Changed candidate/cut trajectory; no fixed-work or complete-edge
speedup. Largest sampled individual RSS1723019264B, not whole-job peak.

New root replay385TERMINAL0/11.538s,5696work: all10fresh proposals cap at64,0improve;
normal variation2.06e-7–5.96e-7. Isolated capturedfacet numerical test finds wasted
projected2Dbacktracking against active constraint. Stock64cap fails3/3starts; stock
full loop87–166callbacks. Existing edge/KKT handoff22–31callbacks converges3/3.
Not a geometric certificate or accepted patch; controlled solver-work diagnosis.

386build38.121s passes; serial51cases/5001assertions59.336s; MPI6passes53cases EACHrank
(5163–5366assertions/rank)/64.434s. Same12root points:5320work vs5696(-6.60%),10of10
improvements/264proposal work, central coarse/tight foot+normal identical. Midpoint
3300work vs3853180(+3.77%); retain both work increases and decreases. Tiny probe
wall11.481/11.507s is setup-dominated; no per-query wall-speed claim. Actual full chord
is frozen under unchanged original300s geometry/600souter, awaiting useful completion.


##386 actual — cheaper preparation, full chord still refused

R/geometry_target386_assessment.json. Terminal1/355.005s, pins intact/owned actual-host
identities terminal. Same source seed/core/policy/config/limits; candidate/cut path
changes. Preparation106.972s vs385137.985s vs380191.679s (44.19%lower than380).
8359integrand evaluations vs6219vs4534 (84.36%more than380before same deadline).
Composition186.889s,total edge293.869s,contextwork5273871; projection12389attempts/
11272improvements/391575work. Region rebuild16.511s is nested, not additive. Whole
edge length still unavailable: no accepted-edge/mesh or full-workflow speedup.
Largest sampled individual RSS1739042816B, not whole-job peak.

New endpoint12point replay11.536s/5423work finds same-foot/different-sided-normal
results at identical queries (.0175036normal difference), while conservative
unprepared wall sample uses identical displacement normal. One same-facet pair still
has2.06e-7normal discrepancy. Thus primitive side ambiguity is confirmed, but full
production target impact remains unproven; prepared mobility differs from this
probe. Next measure complete actual tensor/directional sensitivity and integration
error, rather than optimizing more query variants from counters alone.


##387 prepared production-target attribution (bounded stage2diagnostic)

R/target_stability387_assessment.json. Test-only build196.243s; CFD binary identical
386. Actual loader+prepared mobility+selected371target12point diagnostic61.793s,
5519point geometrywork/.344410spoint evaluations. Total context17407work includes
surface planning/directions/centroid diagnostics; timers are nested, not additive.
Largest sampled individual RSS1408823296B, not whole-job peak. CPU ticks retained
perprocess in assessment; no MPI wait claim from this serial diagnostic.

Useful result is causal attribution, not measured speedup: same exact released
edge binding and foot changes complete tensor2.5306–2.5369% and chord speed
.0149648–.0151072% when certified closest chooses a different incident side.
Another dimension2same-facet pair has2.50e-7relative speed discrepancy. No complete
chord, target checkpoint, new surface or BLmesh was produced.388tests deterministic
exact-supported-side selection; no increased budget or relaxed accuracy.


388build64.133s; focusedserial52cases/7288assertions251.554s includes existing
curved mapped-authority test191.873s plus other focused cases. Prepared12point
M6replay61.718s,5738point geometrywork vs3875519(+3.97%),.35924spoint evaluation
vs.34441s. Additional exact-support visits are charged, so no per-query savings
claim: useful output is removal of the2.53%side-only tensor ambiguity, with exact
same-binding normals/tensors/chord speeds now bit-identical. Interior-foot residual
unchanged. MPI and original whole-chord outcome pending; no workflow speedup yet.


##388 terminal original chord — no demonstrated speedup

R/geometry_target388_assessment.json, seam388_qualification.json. Actual TERMINAL1/
355.9998s wrapper at unchanged300s atlas/600souter, pins intact/all42owned actual-host
388identities terminal. No scalar/constructed patch/mesh. Same saved CFD/core/scales,
physical constraints and limits; corrected exact-supported side can change integrand
and cut/refinement trajectory, so this is not identical fixed-work timing.

| Quantity |386 |388 |Interpretation |
|---|---:|---:|---|
| Geometry preparation s |106.9717|111.0878|3.85%increase; no gain |
| Integrand evaluations before deadline |8359|8163|2.34%fewer, no complete result |
| Composition s |186.8893|182.7027|Nested in edge time; trajectory differs |
| Total edge s |293.8695|293.7983|Same shared budget, not speedup |
| Context geometrywork |5273871|5114485|3.02%lower, no useful completion |
| Prepared-region build s |16.5108|15.3719|Nested; do not add to preparation |

388region hits3990134/misses462013; answer hits6762/misses11806/refinements139;
projection12193attempts/11132improvements/385334work;349719360Bprepared retained.
CPU350.19user+1.05system vs351.55command wall (99%CPU), so major contention is not
supported for this serial replay. CPU does not prove all geometry work useful.
Largest sampled individual RSS1720934400B; not whole-job peak. Starting mesh remains
61330points/303359tets; no adapted counts. New refusing point and raw log retained.

Component qualification: serial52cases/7288assertions251.554s; MPI6passes54cases
EVERYrank/305.900s (6786–7091assertions/rank), including empty ranks/migration/stale
state/resources. These establish affected contracts, NOT native MPI BLgeneration.
Next is bounded existing-panel/error attribution and purpose-specific measurement
requirements. Do not claim speedup from stabilized normals or fewer queries alone.


##389 bounded edge-work attribution (production binary unchanged388)

Build389198.183s changes ONLYtestfixture tracing. Raw existing edge callback returns
same tensors/algorithm; source/runtime/input pins intact. Original chord TERMINAL1/
355.933s at SAME300s atlas deadline:111.0386spreparation,182.8614scomposition,
293.9068stotaledge,8158attempted evaluations. Trace itself adds overhead; these times
are diagnostic, not a new baseline/performance result. Largest sampled individual
RSS1721245696B, not whole-job peak. No complete scalar/new mesh; source61330nodes/
303359tets unchanged. R/metric_edge389_attribution.json.

| Work | Count/units | Useful finding |
|---|---:|---|
| Geometric cuts / initial panels |259/258|Initial coverage completes |
| Initial quadrature evaluations |3870|47.44%of8157successful samples |
| Complete refinement evaluations |4275|52.41%;12further samples in partial last panel |
| Complete measured panels / maxdepth |543/9|Depth20cap is not the failure |
| Known initial/refinement geometrywork |1427161/1791945|Exclude query0/unmeasured baseline and unfinished lastpanel work |
| Dominant root interval evaluations |3480|3465refinement samples =81.05%of complete refinement work |
| Dominant root remaining estimator |1.82010e-6|81.92%of total2.22186e-6estimated error |
| Lastcomplete partition estimate/allowance |42.06898746/4.207e-8|NOT converged/accepted;53xerror allowance remains |

The dominant interval t=[.000294775210939,.000589896934322]has only~1.14%of estimated
integral but~82%of estimated error. The sum uses ACTIVEleaves; split parents are not
double-counted. Parent survives incomplete lastsplit. Reconstructed numerical
G7/K15differences are NOT conservative error enclosures. Long-double trace matching
uses2e-18ONLYoffline, not production integration tolerance. Parser self-check covers
source constants, node order/pair swaps, tiny rounded cuts, bad full blocks and
partial coverage. First cumulative samplework1888199includes prep and is unassigned;
it must not be counted as one query's work.

Existing prepared12point probe (6actual largest-error leaf nodes, default/tighter
certificates) TERMINAL0/61.7246s,6646point geometrywork/.397647spoint evaluation,
RSS1408557056Bsampledindividual. Two fixed-query pairs change near-seam interior to
edge binding, foot6.51e-11/2.27e-10, normal~.01752, fulltensor~2.53%, chordspeed~.015%;
scalarhnonly~8.6e-8relative. Geometry-result instability is a measured contributor,
not proof that all variation is noise or that relaxed scalar precision is safe.

Reuse existing standalone minimizer on TWOactual nets/sixquerypoints: facet35672
points0/1cap at64while30335converges19/39. Diagnosticfull-loop allowance4096still
leaves two nonconverged cases; no production limit was raised. Earliest existing
KKT condition sometimes precedes return, but early checking alone cannot fix all.
Next review EXISTINGcompetitive prepared supports for valid upper witnesses within
original work admission, global lower/clearance proof and exact body/support.
No speedup or usable BLclaim follows from these diagnostics; remain stage2.


##390/391 connected prepared-support witness integration (qualification ongoing)

390build36.160s/serial53cases7299assertions247.538s pass. Hotspot12point diagnostic
61.711s/7935work fixes originalpairs0/1but regresses pair3to2.53%tensor/.015%speed
mismatch because an alternate failure consumes the only post-traversal opportunity
when the initial vertex had skipped its solve. No390MPI/fullchord campaign; exact
source390patch/hash and failure retained. Passing serial did not close this gap.

391build36.159s; eligibility19assertions5.307s; actualhotspot6pairs61.175s/8454point
work/.382740spoint evaluations. ALLsixpairs bit-identical foot/normal/fulltensor/
chordspeed/scalarhn. Pointwork+27.2%vs3896646, not a speedup. Additional scan/unused
originalproposal opportunity are charged. Serial/MPI/fullchord qualification pending;
normal stability alone does not establish construction or workflow improvement.
