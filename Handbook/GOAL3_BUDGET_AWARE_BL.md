# Opt-in budget-aware BL — Goal3 extension

Authorized2026-10-18. Status: native cost diagnosis qualified in its saved scope; connected policy draft
implemented and BUILDING, NOT yet validated.
Full Goal3 remains active; no existing gate has been removed.

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
sensor requests remain mandatory. Require explicit opt-in and per-wall protected
band/maximum outer-growth limits; do not invent acceptable accuracy defaults.
Diagnose whether this freedom can address the cost before activating it. A support
change is not part of this first candidate; report unchanged thickness explicitly.

Assess each complete sensor-plus-BL trial through original unclipped sensor-family
normalization. Search bounded ordered candidates without assuming monotonicity;
report the smallest OBSERVED admissible relaxation and unresolved intervals rather
than claiming a continuous optimum. Use estimated attainable, estimated budget-
incompatible and integration-limited outcomes, with uncertainty explicitly heuristic.
If declared mandatory requirements exceed budget, reject explicitly and atomically.

Broadcast profile/version, reduce unique owned cost and global uncertainty; no new
whole-mesh gather. Allocate integration effort by absolute global error using
existing bounded policy. Freeze profile throughout local edits and transfer. A new
profile requires an explicit stage boundary, current source-family checks and cache
invalidation. Retained geometric samples/cuts and scalar edges must not cross a
changed profile silently. Existing N/M participation and collective failures remain.

## Completion evidence still required

- Disabled policy reproduces strict behavior; invalid controls reject collectively.
- Feasible, incompatible and integration-limited budgets, nonmonotone trial control.
- Curved and interacting walls, first-height/protected/finer/fade audits.
- MPI1/2/4 N=M/M<N agreement/version and atomic rejection; accepted real workflow.
- M6 selection reported separately from complete construction acceptance.
- Bounded trials/preparation/query counts, cold/reused memory/communication and total
  event clocks; no speedup claim from a rejected mesh or contended component alone.

## Native diagnosis and policy draft (2026-10-18)

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
build44141 is LIVE and reuses completed objects; inspect its actual handle before
restarting. The type fix and connected stage review are now applied; source frozen. Primitive curved/interacting/finer/stale-cache tests and
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
It has NOT been compiled/run yet; included in reviewed build44141.
Use the existing closed saved restart/config, explicit experimental permitted
limits and a fresh case folder. The newly recovered original family is from that
saved state; do not relabel it as the historical RAW9 archive or rescale clipped
frozen sensor values. Coarsest endpoints share geometric constraints, but complete
flow-derived fields remain separate references. No Solve/Run/CFD iteration occurs.

Pre-test review: fix_primitive_reference.py is PREPARED/not applied. After build44141
terminates, apply it and one-object incremental build. Nominal sphere offset.03 is
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
