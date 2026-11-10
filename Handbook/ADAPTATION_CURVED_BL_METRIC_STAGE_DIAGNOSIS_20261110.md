# Curved BL metric stage: measured work, repeated evaluation and gaps

2026-11-10. Read-only diagnosis of completed build329 metric construction. The
subsequent remeshing attempt remains frozen; consult HANDOFF_Codex.md for live
handles. No pinned code, configuration or input changed, and no new baseline,
profiling campaign or CFD job was launched for this analysis.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Evidence: `R/m6_runs/rans_curved329_v1/metric_build_completed.log`,
`metric_build_assessment.json` and `metric_stage_attribution_bounds.json`.

## Concrete diagnosis

The expensive path is composed BL-target evaluation, including the final nodal
composition/audit; the printed support-correction timer is not the whole metric
cost. Hessian recovery costs0.543s. Metric construction costs1426.532s, of which
1424.828s belongs to the broad `complexity_gradation_audit` bucket.

For this ONE constructor/ONE assessment execution, each rank's preparation timer
contains constructor plus refresh time; its assessment timer also contains that
refresh. Therefore constructor+assessment is bounded above by the intentionally
loose sum of their recorded maxima,318.079+207.543=525.622s. At least899.206s of
the broad phase lies outside those calls. That residual includes final nodal
composition, transported-edge audits, other normalization work and waiting. It is
not an exclusive geometry timer. Never add the two overlapping timers as a cost
breakdown or label the residual entirely curved certification.

The final nodal audit makes exactly66760 target-composition calls:61330 owned
points plus5430 halo duplicates. Each evaluates base composition. For the recorded
core1e-4 and HGRAD1.3, the continuation-support lower bound is381.149m; the saved
wing bounds need only100m padding to cover the entire original domain. Thus the
curved base wall is not rejected by the support AABB at those nodes. This path
performs at least66760 base closest certifications, plus conditional weight/full
evaluations. Its elapsed time is not separately recorded.

## Attribution by category

| Category | Confirmed work | Limit of attribution |
|---|---|---|
| Initial wall-policy/rule setup |1.655s in the initial geometry_rule timer, before the broad phase. |This is model/policy and cell-rule setup, not total prepared-region construction time. |
| Support proposal and donor discovery |8192 proposals;6516 inside;798785 support-build units;19451083 support-PDF candidates;1026 rank donor batches, corresponding to171 rounds of at most8 local proposals per rank. |Preparation timing includes routing, certification and waits. These PDF candidates are not closest-reference ADT root candidates. |
| Prepared curved geometry |ClosestCurved reuses exact immutable facet/subregion nets and hulls in the existing bounded pool. Query bounds, clearance and final differential evaluation remain fresh. |Existing closest_region hit/miss/build-time counters are not serialized for this metric stage. Successful root-discovery versus preparation versus subdivision costs are missing. |
| Curved-distance/normal certification |The exact summed metric geometry work is53473769 units; per-rank range4222169–12188969. |Units are not unique regions or seconds. Successful-query seed/root/refinement splits were not emitted. Failure-only326/328 partitions cannot be extrapolated to this completed stage. |
| Identical repeated requests |Extend retains6516 full SamplePoints results. The immediate first Assess repeats full SamplePoints at the same binary64 coordinates/model/version because cachedTarget changes from null. |This establishes6516 redundant full sample requests, including empty results; it does not count identical low-level closest calls or their saved time. |
| Full/base and weight evaluation |Refresh records13032 full/base sample requests. OptionA.Weight can additionally evaluate base wall distance before composition evaluates base again. |Weight's distanceEvaluations counter is not printed here. Full/base physical authority agrees, but D1 tensors, support boxes and complete samples can differ. |
| Nodal target evaluation |66760 calls include5430 repeated halo locations. Weight and conditional full composition add evaluations beyond base. |The audit has no independent wall/CPU timer or closest-query counter. Nearby-point frequency and overlap with retained proposal points are not recorded. |
| Density arithmetic/background |6516 correction evaluations cost0.00636s; background arithmetic0.11593s. Background tensors build once per rank, with no reuse across trials because only one trial ran. |These exclude geometry preparation/refresh and do not explain the1424.8s bucket. |
| Rank imbalance and collectives |Charged geometry work differs by2.887x; phase wall times are nearly identical. CPU spans1279.289–1288.722s. |CPU includes polling. Reported reduction0.01298s and halo0.000745s cover particular calls, not all Vote/exchange/join waiting. No useful-work/wait share is established. |

The exact work number above is53473769; it includes charged cache maintenance.
For the verified one-curved-wall full/base setup, one trial, bypassed strict
quadrature and disabled direct audit, a conservative closest-call upper bound is:

`19548 sample requests +6516 possible refresh-weight requests +3*66760 nodal requests =226344`.

Each closest query now charges at most64 optional cache units. Hence at most
14486016 units,27.09% of the observed total, can be that maintenance; at least
38987753 units,72.91%, are seed/root/refinement work. This rules out the previous
maintenance-dominated work-cap failure as the dominant current charged-work
category. It does not bound uncharged key construction, map lookup, locks or
allocation time, and does not establish an elapsed-time percentage.

## Existing reuse and invariant data

The constructor sets profileVersion correctly. Initial full samples are still
valid when the target is first attached: same full model, sample version and stored
coordinates, with no intervening target update. Base samples and blend weights
have not yet been prepared. Full-sample reuse at that point has a concrete basis;
its total-stage speed benefit remains unmeasured. Refine/append and changed-profile
callers must be covered before any production change.

Full/base share the original authority, wall grouping/facet order, requested
h0/growth/thickness and Hausdorff inputs. Their feature angle differs. Immutable
surface definitions and an intrinsic closest foot/differential at an exact point
are independent of sensor scale. A complete Sample additionally contains normal
spacing, inherited tangent interpolation, D1, fade/continuation and logarithms.
Sharing authority alone does not make full/base Samples interchangeable.

Existing retention already covers donor nodes/barycentrics, full/base sample
vectors, weights, exact-sensor-equality background reuse and immutable curved
regions. ClosestCurved has no completed-answer memo here; surface-union memo hits
belong to another query path. Existing accepted associations are not distance
certificates. Nearby coordinates require fresh position-dependent evaluation.

Any reuse must retain exact authority/version, complete support, coordinates,
precision and association semantics, including deterministic tied-foot/component
ownership, plus profile identity when reusing Samples.
Numeric sample version alone is not model identity: full/base complete Samples
remain different even if their version numbers match. ApplySample rejects stale
profile identities. Base identity currently relies on
the coordinated TargetPolicy lifecycle; audit it independently for a change.
Keep active-reference accounting, transient admission, time/work checks and bounded
memory. An uncertain bound or identity mismatch falls back to certification.

##500k feasibility and inherited tangent demand

The current policy reports provisional outside_estimated_bounds:
1174528.386 with empirical indicator108761.874, requested500000. The sensor-only
complexity is2.08846 at its coarsest floor. This is not a qualified500k target or
a proven physical lower bound.

Source confirms that wall-edge covariance supplies the tangent size field, which
is interpolated through rotated vertex logarithms and intersected with flow.
Disabling the additional TangentialFloor does not remove this tangent block.
The actual global marginal contribution remains unmeasured: aggregate
sensor/correction totals do not separate inherited tangents, normal law, D1 and
continuation, and the retained query coordinates/tensors/weights were not exported.

The existing repository VTU reader independently verifies that
`flow_adap_00000.vtu` contains61330 unadapted points and finite fields. Exported
Metric_XX/YY/ZZ are1e-4 (Float32), off-diagonals approximately1e-19: these are the
sensor-only floor. The file does not expose the composed BL target needed for
component attribution. `initial_vtu_metric_field_check.json` records hashes and
ranges. Use the effective composed policy, with sensor fields labeled separately,
for the first meaningful BL inspection; this original-grid VTU is not that result.

A scale check illustrates why wall triangulation matters without quantifying that
actual contribution. For an independent planar triangle of area A, the source's
face covariance C=(2/3)sum(e e^T) gives A/sqrt(det(C_tangent))=sqrt(3)/4. The38800 such
triangles give16800.893 surface metric-area units. The requested normal law alone
integrates to37.97395 metric-depth units over0.01m at h0=2e-6/growth1.2. Their product
is637996.2 in this element-local planar proxy. This excludes production vertex-log
interpolation, curvature/tube Jacobians, overlaps, D1, flow and outer continuation.
It is neither an actual layer count nor a decomposition of1174528.386, and proves
no500k impossibility. The required production component comparison is still open.

Use the existing actual-point/cavity diagnostics at the first representative saved
BL patch. Quantify ordered diagnostic counterfactuals at identical certified
positions/weights/sensor tensors; component interactions are nonlinear, so do not
add independent densities. Recovering the original8192 positions would require
more than approximate spatial matching; do not launch a new baseline campaign to
manufacture that comparison. A later connected validation can aggregate component
results while existing query data is already available.

## Decision and qualification boundary

Preserve frozen329's outcome before selecting ONE correction. The demonstrated
repetitions above are starting points; no production change or timing improvement
is established by this audit. The provisional target floor and actual construction
outcome may take priority over another optimization.

Retain the queued Coulaud/MAdLib investigation: physical curvature/tolerance demand;
separate flow/geometry/requested-height and inherited-tangent contributions,
composition and gradation; realized directional conformity, alignment and q/J on
complete surface patches plus neighboring volume. Smooth patches and genuine
features need separate controls; a patch seam is not a physical feature.

Keep the no-CAD authority sufficient and every strict gate/budget. Surface-normal
metric extension does not prescribe BL first height; L2 quadrature geometry error
does not replace strict certification. No new scheduler, representation, config,
norm/sensor change or silently weakened heights/raised complexity. Focused checks
and an appropriate MPI milestone must precede immediate same-seed full BL/RANS
return. Two accepted meshes, admissible conservative flow/SA transfer, finite
continued CFD and actual adapted disk restart remain required.

## Subsequent remesher setup: current observation limit

A bounded read-only follow-up found no demonstrated collective-alignment error in
PrepareMobility, normalization-factory eligibility or ImportGeometry. Unequal local
mesh sizes and runnable ranks do not establish a mismatch or deadlock. The factory
creates a job; it does not execute normalization trials at construction time.

Two setup atlas budgets lack a within_time callback: PrepareMobility in
CNativeRemesher3D.cpp:82 and JointAffineCoverage in CNativeImport3D.cpp:438. The latter
can construct revision-cached GeometryCertificates at:478 before subsequent local
deadline checkpoints. These operations remain work/memory bounded, but their
inside-call time is not continuously checked by those budgets. This is a source
observation, not evidence locating the live run or proving an overrun.

Existing eventual receipts contain remesher phase totals and import faces_seconds,
source_envelope_seconds, payload time, work/limit, contacts and distances. Their
publication often waits for the enclosing operation to return. Preserve the frozen
outcome and inspect those counters before attributing the quiet interval, choosing
a correction or authorizing a stop. No new profiler or competing implementation.

## Source anchors

- CSolver.cpp:2906–3035,3313–3325,3656–3704,3765–3832.
- CNativeComplexity3D.cpp:112–184,186–301,321–344,350–438.
- CNativeBLTarget3D.hpp:62–66,93–129,237–267,283–302.
- CBoundaryLayerMetric.cpp:372–429,939–1019,1964–1999,2010–2254,4000–4014,4171–4198.
- CPhysicalGeometry.cpp:2108–2122 sums nPoint including halos.
