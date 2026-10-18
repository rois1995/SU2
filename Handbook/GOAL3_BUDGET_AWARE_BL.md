# Opt-in budget-aware BL — Goal3 extension

Authorized2026-10-18. Status: controlled accepted opt-in MPI workflows, strict-default compatibility and
saved-state M6 selection qualified in the scopes below; full-goal gates remain open.
Full Goal3 remains active; no existing gate has been removed.

## Reaffirmed active scope

The user's full policy specification remains part of the existing active Goal3;
see `budget_aware_BL_contract` in the requirement audit. This does not restart or
replace the implemented policy. Current freedoms remain outer-normal growth only,
under explicitly permitted limits; tangential demand, thickness/support and fade
stay unchanged. The calibrated global-error work allocator, enabled complete curved/
interacting workflows and2D extension remain unfinished. M6 budget selection and its
separate exact-facet construction obstruction must continue to be reported apart.

## Evidence and decisions

M6 coarsest-sensor HMAX100 endpoint estimates790200.50 +/-27958.45 HEURISTIC
against100000, almost entirely BL correction. The background near2 belongs to that
intentionally coarsened endpoint; original flow-derived sensor complexity is around
150000. Neither metric complexity nor its uncertainty is an exact tetrahedron count
or proven infeasibility bound. The original RAW9 normalization field and owner
UPPER6 construction field remain separate immutable references.

Diagnose composed correction by wall distance and surface region and inspect normal
versus tangential spacing before selecting a relaxation. Wall-face tangential sizes
can remain expensive away from the wall; changing them requires a declared geometry/
accuracy justification. Current targets prescribe continuous spacing, not removable
layers. Deleting rows cannot satisfy the same target. Halving thickness gave only
about10% reduction in an idealized current-profile calculation, not an eightfold fix.

## Smallest proposed first policy

One globally shared parameter in[0,1] permits ONLY declared outer-normal growth
beyond an explicitly protected physical wall-distance band. First height, geometry,
protected original resolution, tangential target, original fade/support and finer
sensor requests remain mandatory. Require explicit opt-in and global protected
band/maximum outer-growth limits applying to every wall; do not invent acceptable accuracy defaults.
Diagnose whether this freedom can address the cost before activating it. A support
change is not part of this first candidate; report unchanged thickness explicitly.

Assess each complete sensor-plus-BL trial through original unclipped sensor-family
normalization. Search bounded ordered candidates without assuming monotonicity;
report the smallest OBSERVED admissible relaxation and unresolved intervals rather
than claiming a continuous optimum. Use estimated attainable, estimated budget-
incompatible and integration-limited outcomes, with uncertainty explicitly heuristic.
If declared mandatory requirements exceed budget, reject explicitly and atomically.

Broadcast profile/version, reduce unique owned cost and global uncertainty; no new
whole-mesh gather. Absolute-global-error allocation remains a completion requirement; the retained
cloud provider currently has a fixed bounded work budget and heuristic indicators. Freeze profile throughout local edits and transfer. A new
profile requires an explicit stage boundary, current source-family checks and cache
invalidation. Retained geometric samples/cuts and scalar edges must not cross a
changed profile silently. Existing N/M participation and collective failures remain.

## Required evidence (current qualification below supersedes historical notes)

- Disabled policy reproduces strict behavior; invalid controls reject collectively.
- Feasible, incompatible and integration-limited budgets, nonmonotone trial control.
- Curved and interacting walls, first-height/protected/finer/fade audits.
- MPI1/2/4 N=M/M<N agreement/version and atomic rejection; accepted real workflow.
- M6 selection reported separately from complete construction acceptance.
- Bounded trials/preparation/query counts, cold/reused memory/communication and total
  event clocks; no speedup claim from a rejected mesh or contended component alone.

## Diagnosis and preserved implementation history (superseded by qualification below)

Two saved8192-point clouds give coarsest BL corrections805274/805597 with heuristic
3SE28016/28029. MPI2/4 rows match exactly. Near-wall protected demand is substantial:
first-cloud10*h0 correction96587 +/-12196;50*h0 gives223596 +/-18285. Tip/span last
quarter contributes~42%; fine tangential spacing persists far from the wall.
Independent one-wall/isotropic spectral probe reproduces native baseline within
8.1e-9 scaled point-density drift. Protecting h0 with maximum growth2 leaves
263--265k correction; growth10 leaves125--128k. A diagnostic near-limit growth1e6
leaves107--110k, near/beyond100k even before the small background. No acceptable
physical limits were selected and no exact infeasibility proof is claimed.
[Diagnosis/probe/work/receipts](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_cost_v1/README.md).

Current connected policy draft adds ADAP_BL_BUDGET=NO by default, with explicitly
required ADAP_BL_PROTECT_DISTANCE and ADAP_BL_OUTER_GROWTH_MAX. Native3D only for now;
2D/MMG opt-in is explicitly rejected while their existing behavior remains. One
lambda blends normal slopes beyond the protected distance, keeping tangents,
geometry, thickness/support and fade. At most9 ordered samples, no monotonicity
assumption; uncertain earlier profiles are not skipped to force more relaxation.
Original-family scale normalization then freezes the selected profile. Models/samples/
strict rules/workspaces check version; source support is retained while profile-
dependent samples rebuild and are counted. Selected family must survive driver and
N/M remeshing; missing/unfrozen selected profiles reject rather than reverting.

Initial build7876 TERMINAL exit1 at235/310: ambiguous Metric alias in mandatory
cost calculation. Its failed log/resources/status are preserved. The reviewed
build44141 subsequently passed, reusing completed objects. The type fix and connected stage review are now applied; source frozen. Primitive curved/interacting/finer/stale-cache tests and
actual cfg-driven RANS replacement tests are implemented but NOT run. Disabled
compatibility, incompatible/uncertain/feasible profiles, atomicity/MPI, independent
complete field checks, M6 selection and event-cost qualification remain. Global
uncertainty calibration/absolute-error work allocation is still incomplete; fixed
retained-cloud heuristics must not be represented as that completed capability.
[Draft source scope and checks](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/DESIGN.md).

Review while first build runs: `budget_bl_policy_v1/finish_stage_review.py` is
APPLIED ONCE after initial build7876 terminated; do not rerun the mutator. It keeps config opt-in
correction available even after an initially accurate normalization, rejects clear
estimated over-budget outcomes at original-family bounds/final bounded correction
before publication, restricts the primitive to supported3D, and adds atomic refusal.
The draft budget3 cube needs explicitly permitted growth10 to exercise nonzero
selection: its coarse baseline is~3.7 and growth2 still~3.3. This is an analytic
fixture-design correction before tests, not a rejected native run or physical M6
accuracy permission. No original mesh gates are changed. Reviewed build44141 incorporates this connected batch. After it completes, run
campaign.py primitives/disabled/enabled checks.
Do not run the knowingly incomplete first draft's budget3/growth2 workflow.

Saved-state M6 qualification can avoid another CFD step: a new hidden
[NativeSavedMetric3D] fixture draft calls Preprocess(0) and public ComputeMetric()
(sensor/Hessian/original-family normalization only), then checks flow/SA/reference
unchanged. It is now INCLUDED by CNativeImport3D_tests.cpp through the applied stage review.
It was compiled in the reviewed build; the saved-state runs are recorded below.
Use the existing closed saved restart/config, explicit experimental permitted
limits and a fresh case folder. The newly recovered original family is from that
saved state; do not relabel it as the historical RAW9 archive or rescale clipped
frozen sensor values. Coarsest endpoints share geometric constraints, but complete
flow-derived fields remain separate references. No Solve/Run/CFD iteration occurs.

Historical pre-test correction: fix_primitive_reference.py was APPLIED ONCE;
the focused incremental build passed. Do not rerun this mutator. Nominal sphere offset.03 is
not exactly the represented point's wall distance; protect/check the actual distance
and verify the declared half-slope spacing increment. This changes a test reference,
not a physical constraint, native query predicate, geometry/height or quality gate.

First native checks: primitives PASS4 cases/256 assertions. Disabled_serial FAILS
only its exact legacy diagnostic string assertion (unnecessary wording change).
Enabled_serial rejects before metric initialization because SU2's preliminary
base config calls postprocessing with UNKNOWN dimension0; the new check wrongly
required3 at that stage. No enabled normalization/construction ran. Both receipts/
capsules/logs are preserved at policy root. Fix grouped: retain legacy text when
disabled, accept dimension0 until actual zone check/model3D guard. New checks use
fresh qualification_v2 directories; do not overwrite first failures.

Qualification_v2: primitives PASS4/256; disabled PASS22/2056. Enabled budget3
reaches a private constructed candidate but rejects publication correctly at the
original-family lower scale: provisional sampled3.119586 +/-0.145765 becomes
3.162588 +/-0.041057 on finer candidate proposal (indicator3.55x smaller). The
profile froze atlambda.5 because the initial estimator was uncertain. This is
budget-selection failure, not proof construction is impossible or accepted workflow.
No case20 execution/whole campaign success; both failed campaigns remain.

Root correction: DO NOT force already-supported planar strict integration onto
sampling for policy selection. Preserve original strict planar/analytic rules and
existing bounded fallback for genuinely difficult integration. Versioned rules/
workspaces already handle profile changes. Include the protected-distance cut even
atlambda0. The same strict rule also reports a mandatory lower contribution, with
INFINITE uncertainty for a partial protected band (its separate convergence is
unproven); when all support is protected, full-field estimate/indicator applies.
No integration ceiling, physical limit, mesh gate or operator fallback is changed.
New qualification_v3 needed after current strategy build.

Qualification_v3: strict planar selection reacheslambda.875, initial3.0000309
+/-0.0003300. Backend COMPLETE in2 stages/work57, final sampled2.936869 +/-0.046818,
16 cells at final coarsen. Then the atomic-refusal FIXTURE fails: a newly parsed
limited config lacks mesh-bound marker order/send-receive entries, causing import
of the unchanged ORIGINAL CFD mesh to reject before a second native event. Existing
normalization-test setup already copies these records; new fixture omitted it.
No evidence the constructed mesh is nonmanifold. Whole workflow remains FAIL until
transfer/publication/reimport and independent checks finish. Marker setup corrected
using existing pattern; current/BDF/projection and final sensor data now retained.
New qualification_v4 required; no production mesh/constraint fallback.


## Qualified v4 complete workflows

All v4 jobs terminal. Opt-in initialized RANS/SA dual-time current+BDF cases at
MPI1, MPI2 and N4/M2 pass, two budgets each. Independent references integrate the
same frozen composed field rather than adapted-node refits:

| Budget | Frozen lambda/version | Final cells | Independent mass | Budget discrepancy |
|---|---|---|---|---|
|3|.875 /8|16|2.95498841845|-1.50039%|
|20|0 /1|240|19.9993054819|-0.00347259%|

All ranks select the same profiles, versions and original-family sensor scales.
MPI1/2 transaction topology can differ; independent q/J/full-edge and geometry/
height checks pass on each. Final statistical estimates differ with the actual
proposal mesh and their errors are covered in these observed cases (not a proven
bound). Current+BDF conservation defects <=2.0e-16, projection <=2.7e-16; original-
P1 transported sensor residual and first-height error are zero in this fixture.
The limited-work event rejects atomically, preserving state/reference/profile.

Disabled controls:22 cases/2056 assertions pass at v2 with source-qualified relevant
behavior; v4 N4/M2 old normalization runtime passes both scales, independent full
geometry/metric/conservation, SU2+CGNS reload and actual restart. Primitive4/256
checks cover nonmonotone/uncertain/incompatible controls, curved/interacting wall
composition, finer sensors, stale sample/workspace rejection and frozen profile.
Enabled policy CGNS/restart and complete curved/interacting policy workflows are
not yet independently established by the primitive or disabled checks.

Performance warning: serial budget3 strict planar initialization uses90 cell-rule
builds/3,716,752 density evaluations,8.197s quadrature including4.215s selection;
budget20 uses36/1,235,416/2.608s, selection.571s. Original-family correction reuses
geometry and does not rerun selection (selection_seconds=0). These contended small
cases establish bounded work, not efficiency or a complete-lifecycle speedup.
Fixed-cloud uncertainty calibration and global absolute-error allocation remain
open. See policy evidence/README for final M6 selection and resource receipts.


## Saved-state M6 selection and uncertainty-stage correction

Growth2/protected2e-6, budget100000, MPI4: all nine sampled profiles exceed budget,
from790200.50 +/-27958.45 to260488.18 +/-10780.29. Protected lower contribution
17173.58 +/-6184.98 does NOT itself exceed100000. Selection rejects collectively;
this is a sampled-profile conflict, not proof of mandatory/continuum infeasibility.
No CFD advance and no construction attempt.

Budget300000 first reaches an unresolved profile atlambda.75: coarsest308423.48
+/-12078.10. The original implementation then spent933 gradation sweeps/35.20M
owned-point visits/86.33s gradation, ending310554.64 +/-12099.09, still uncertain.
Launcher102.70s, metric93.48s. It also reached the sensor gradation sweep limit
(48249 directed residual violations); this is NOT a certified constructed field.
Both complete source/binary/config/input receipts and logs are preserved.

Connected correction: enabled-policy normalization stops provisionally at the
original coarsest scale when its estimate overlaps budget. Fixed retained samples
cannot resolve a smaller correction; the same frozen field must be reassessed at
the next explicit stage. This does not claim an exact root or weaken any mesh gate.
Strict disabled behavior unchanged. Dedicated serial/MPI uncertainty controls and
saved-state M6 rerun are required. Enabled runtime fixture is extended through the
existing SU2/CGNS round-trip and actual current+BDF restart helpers in the same batch.


The v5 uncertainty fixture failed because it supplied3.694 to the existing unsigned
integer ADAP_COMPLEXITY option, which was read as3. Production logic was not the
cause. Preserve this failed control. Replacement uses integer budget4 and a coarsest
sensor spacing obtained from the independent one-wall integral, not a fitted native
value. Existing config semantics remain unchanged. Focused test-only rebuild.


V6: uncertainty stopping passes serial/MPI4 (one normalization trial, original
coarsest scale); primitives/strict controls pass. Enabled restart extension aborts
in its fixture: after constructing the restarted driver it dereferences Reference
before Backend() initializes the native authority. The existing restart fixture
calls Backend first. Apply that same initialization and retain the v6 abort/stack/
receipts. No production source, physical acceptance gate or native repair changes.


## Current qualified checkpoint (supersedes all historical pending notes)

All jobs terminal. V7 primitives4/256, strict22/2056, uncertain serial/MPI4 and
complete enabled serial/N4M2 workflows PASS. Independent audit now also covers
SU2/CGNS reload and actual current+BDF restart with SA fields; restart flow/BDF
defect zero. V4 retains MPI2 scope. No mesh acceptance contract was weakened.

M6 new300k selection:lambda.75/version7,308423.48 +/-12078.10, integration_limited,
original coarsest scale, flow/SA/reference unchanged, no CFD/construction. Metric
93.477->6.552s by skipping unresolved finer-scale trials (provisional scale changes;
not a same-field kernel/lifecycle speedup).7 rather than933 gradation sweeps;
429310 rather than35.20M visits. Preparation3.505s, retained max15.46MB/rank,
profile refresh31956 geometry queries, zero repeated donor searches. Background
still46.72M determinants. Observed process memory is not aggregate peak.

M6 MPI2/4 coarsest trial estimates/indicators agree to2.04e-13 relative. Budget100k
remains sampled-profile incompatible; budget1000 explicitly reports the protected
lower requirement17174 +/-6185 exceeds budget. Independent saved coarse-endpoint
holdouts forlambda.75 give314146 +/-12153 and312539 +/-11852; both lower heuristic
endpoints slightly exceed300k.300k is therefore NOT demonstrated attainable.
Indicators are heuristic; stage reassessment remains necessary. No new accepted M6
mesh, full curved/interacting policy lifecycle, calibrated global allocator,
converged accuracy or complete-event affordability claim.

[Final scoped report, actual grids, costs and failures](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/README.md).
`report.py` verifies complete receipts and final helper/source/build/binary pins
without rerunning SU2. Full Goal3 remains active. No cluster work or CFD advancement.


## 2026-10-18 — bounded evidence stages resolve M6 profile selection

Worked: retain original geometry/donor indexes across at most two correction-stream
extensions; rescan every earlier profile before freezing. No extra whole-mesh
gather, support/fade/tangent change, cap increase or donor refit. Serial28/10625,
MPI4 and independently audited opt-in/default N4M2 RANS/SA/BDF/SU2/CGNS/restart PASS.
Resource-refusal control preserves the prior complete estimate atomically.

M6 .75 at8192:308423 +/-12078 unresolved;32768:308123 +/-5972 estimated incompatible
with300k. Select.875/version22:281779 +/-5604; full original-family normalized field
295068 +/-5769, -1.644% budget, still integration_limited. MPI2/4 agree <=1.072e-11.
Both independent saved holdouts support the profile distinction; no exact reference
or rigorous confidence claim. Budget100k remains a nine-profile estimated conflict,
not proof mandatory requirements themselves exceed it. No M6 construction here.

Cost: MPI4 metric137.055s, nested gradation102.455s; MPI2 metric169.793s,
gradation132.241s.27complexity trials/937sweeps. Fixed proposal work798785,
59.327M PDF candidates,389065 correction queries; one donor index/rank survives
both extensions. Retained integration228.8MB/rank MPI4 or336.3MB MPI2. Compared
with earlier unresolved coarsest target, this is a different finer sensor field,
not a speedup/regression experiment. No full-lifecycle affordability established.

Independent scalar reference21.20654634 is covered at128/256/512 observed stages,
but errors/indicators are nonmonotone. Heuristic tails/optional stopping and
per-cell global absolute-error allocation remain open, as do difficult accepted
construction, composed gradation and2D opt-in policy. Preserved initial tight-MPI
fixture failure/interrupted builds; measured-quota fixture corrects test admission
without changing production limits. [Complete report/evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/adaptive_budget_evidence_v1/README.md).


## 2026-10-18 — complete M6 attempt with the selected budget profile

Complexity initialization no longer blocks this field: .875/version22 and
295068.448185 +/-5769.124264 reproduce exactly. Construction nevertheless rejects
atomically at300.434s, no transfer/accepted wing.38800 private first-row publications
and1707 subsequent edits,2200rounds/34useful-progress windows;2351remaining edges,
6211cached invalid cells. Last cone/interface obstruction and rank2q=.18018<.20
are retained. Time exhaustion is not stagnation or proof of infeasibility.

One bounded MPI4 run, zero CFD advances. Native import2.082/source.856/workload
measurement18.524/repartition1.605/migration.241/refine277.082s; nested reconstruction
max120.450s,600streamed retries and326764kernel tracked calls/rank. Residual kernel
time includes synchronization/unattributed work, not a measured pure-MPI total.
Target initialization99.193s versus137.055s earlier is not an implementation speedup.
Exact61330-node sensor snapshots/profile/core and closed inputs are now saved for
replay; clipped tensors must not replace the raw family or be rescaled. No rejected
private whole-mesh grid was exposed by the backend. No new fallback or cap change.
Next: complete expanded-proposal dependency admission/concurrency and retained
support, because composed BL edits currently serialize for that missing guarantee.
[Audited outcome and sections](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/selected_profile_construction_v1/README.md).


## 2026-10-18 — complete-proposal concurrency, scope and negative M6 result

Final full-old-vertex reservations, versioned imports, conflict deferral without
blocking and bounded summed private work permit concurrent faceted composed REFINE.
Completed peers retain metric diagnoses while serving other queries. No new cache,
constructor fallback, physical/metric gate, gather or limit change. Serial9/735,
MPI4nine cases/rank and actual opt-in/default N4M2 independently audited RANS/SA/
BDF/conservation/height/full-metric/SU2/CGNS/restart PASS.4controlled commits/epoch.

Same exact archived M6 sensor/profile/core, no new Hessian/normalization/CFD step:
3simultaneous commits,1707->1955private edits,326764->280568kernel calls/rank.
Still atomically rejects300.609s; residual counts do not establish improvement.
No accepted wing, transfer or event speedup claim. Original raw-family correction
is unavailable in this construction-only replay; clipped tensors never rescaled.

Persistent reported edge/face recurs. Offline original input witness has5tets,
that face and exact cell104931centroid; outside BL support, sensor edge2.60882>1.8.
Late working-cavity equality/feasibility remains unproven. Next use a local frozen
replay to identify the missing construction freedom, not another fallback/cache/
limit variant. Ledger is now a concise index with its full1004-line history
preserved separately. [Report and viewable witness](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/concurrent_composed_proposals_v1/README.md).

## Frozen M6 construction feasibility update

Budget/profile selection remains an estimated cost decision. An exact-coordinate
certificate now establishes a separate frozen-target geometry obstruction: root
SYMMETRY crease211--3481 has only the midpoint as an interior binary64 point, but
that subdivision leaves full composed length11.57>1.8. Near-coplanar finite facets
are distinct under the current exact authority. No additional normal relaxation
is silently selected to hide this, and the original sensor donors/fade/geometry
stay. A declared geometry-reference decision is separate work.
[Certificate](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1/README.md).

Construction follow-up adds strict exact-facet resolution diagnosis to the existing
composed-field path, with no profile/normalization changes. The captured selected
M6 crease collectively rejects at MPI1/2/4 while preserving input. Enabled and
strict-default N4M2 workflow audits pass; the budget estimate still does not imply
construction feasibility. [Final diagnostic scope](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/strict_geometry_preflight_v1/README.md).
