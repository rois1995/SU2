# Native 3D development goals

Goal1 was activated on2026-10-10 after the pushed2D reuse checkpoint
038fdfa5786fb3f6af392d97257bfbb2bb29f036. **The user revised Goal1 on2026-10-12:
remove performance qualification and all cluster-related work; implement and close
Goal1 using only locally available resources.** This revision supersedes the earlier
performance/cluster completion gates. Original evidence is preserved; the previous
plan is archived in
integration_evidence/native_3d_core_v1/GOAL1_PRE_LOCAL_SCOPE_PLAN_20261012.md.

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Implementation branch: codex/native-3d-transport-accounting.
Qualified batching baseline:3345e2bc9d9561045e459f89ea046a3c7583fd5d.
Pushed execution-policy snapshot/handoff:7dd0aff6d12d7213781f1e6ce6cca0d184ebd81f.
Goals2–4 remain prepared, not activated. Preserve AdapNoExt, other workers and the
existing2D/MMG implementation. Goal1 remains ACTIVE until the local audit below passes.

## Current local-only execution contract

Use small sequential correctness builds/cases and the available local MPI1/2/4
ranks, including N=M and M<N. Before each heavy job inspect host load, available
memory and competing jobs. User permits correctness-only MPI under contention:
one owned heavy job, bounded CPU affinity, low priority, initially MPI<=4/build-j2
and one numerical-library thread. Never terminate unrelated processes. Keep the
existing pinned build running; poll its actual handle rather than restarting a
quiet process. Announce each new testcase folder with its full absolute path.

Do not perform performance campaigns, scaling/throughput/instrumentation-overhead
qualification, cost-weight calibration, cluster preparation, scheduler submission
or cluster analysis for Goal1. Existing timing instrumentation and archived cluster
evidence are retained. Cheap counter/accounting assertions in functional regression
may remain, but provide no efficiency claim and are not separate completion gates.
Performance and larger-resource qualification are deferred work, not missing local
Goal1 requirements. Do not claim speedup, scalability or CFD affordability.

Functional safety remains mandatory: explicit finite work bounds, optional deadline
semantics, memory admission, MPI-consistent controls and decisions, dependency/stale/
conflict checks, atomic rejection/publication and bounded diagnostic output. Preserve
original sensor-only P1 interpolation, geometric BL at actual queries where supported,
finer sensor requests, fade and ADAP_HESSIAN_NOISE=0. Production3DBL/composed-edge
integration remains explicitly unsupported in this goal; never silently validate a
sensor substitute. No geometry/metric/quality tolerance weakening to obtain closure.

Retain compact source/build/binary/provenance, inputs/logs, independent certificates,
inspectable accepted/rejected meshes and donor/accepted/current+BDF states needed for
replay. Preserve historical failed receipts and annotate collector-only corrections;
do not promote failed aggregates. Update HANDOFF_Codex.md, findings and requirement
audit, review/commit/push qualified checkpoints, and keep unqualified work labelled.

## Goal 1 — Locally validated MPI tetrahedral core and adaptable planar boundaries

Objective: implement and locally validate an opt-in native tetrahedral adaptation
core on simple faceted domains, with adaptable planar boundary triangles, bounded
robust local operators, complete distributed dependencies and atomic state handling,
cost-weighted partition functionality, explicit M<=N workers and return to N CFD
ranks. This is an implementation/correctness goal, not performance qualification.
Reuse existing remesh, distributed search, transfer/rebuild/output and transaction
infrastructure. Do not complete from a single split, an API-only prototype or only
frozen-metric arithmetic tests; include repeated actual driver replacement/restart
and actual metric/Hessian construction controls.

Deliver and audit locally:

1. Robust3D orientation/tensor arithmetic, canonical identities/incidence and
   sensor-only original-P1 tetrahedral queries with bounded caches. Keep frozen
   meaningful shape/sliver/length criteria: q>=.20, scaledJ>=.05, full original-P1
   metric edge<=1.8. Test numerical/adversarial cases before relying on campaigns.
2. Bounded split/collapse/movement/reconnection, including face/edge/vertex cavities
   and coordinated planar surface/volume changes. Admit complete stars; validate
   privately; preserve geometry, marker seams and atomic publication. Exercise
   repeated refinement/coarsening and rotated/spatially varying anisotropy.
3. MPI1/2/4 metadata/payload/reservation/commit, stale/conflict/common-veto/empty-rank
   behavior; cost-weighted ParMETIS migration as a functional path, explicit M<=N
   workers including M<N, return/rebuild on all N ranks, source authority and state
   preservation. Weight quality, optimality and balance are not performance gates.
4. Integrated blockage diagnosis and bounded execution: distinguish geometry,
   measurement, shape/sliver, frozen-progress, resource/work/time and operator-search
   failure; report bounded representative locations and residuals. Local operator
   blockage is not proof that no valid mesh exists. Distinguish finite exploration
   of rejected directions from certified size/quality/removal progress. Retain
   bounded repair escalation; replace the unexplained production512-round cutoff
   with a declared finite work policy and consistent MPI decisions. Verify history/
   identity allocation and memory admission when productive work exceeds512epochs.
5. Optional deadline and configured shared refinement/coarsening work controls:
   verify both successful and exhausted cases without replacing accepted mesh,
   sensor authority, solution or current/BDF history. Document safe-point overrun
   and the exact clock scope; a native-entry/prepublication deadline is not a
   hard metric-to-CFD-ready ceiling. Current rejection stops production collectively;
   do not imply automatic retry/skip or a newly written recovery checkpoint.
6. Independent local certificates on boxes/wedges and declared aspect/thin cases:
   positive volumes, conformity/manifold boundaries, no overlaps/gaps, finite-facet
   coverage and marker preservation; actual original-P1 metric residuals and shape/
   sliver/length thresholds; actual SU2/CGNS output/reload/reference sidecars, solution/
   current+BDF transfers and supported restart. Verify N=M/M<N and repeated events.
   Different partitions may produce different meshes; independently audit each.
7. Actual metric/Hessian and driver lifecycle controls, relevant unchanged2D/MMG
   regression and one broad integrated local regression after the new policy
   qualifies. Verify exact current sources, complete configured68-object driver,
   linked production archives/members and binary. Publish a requirement-by-requirement
   LOCAL_GOAL1_COMPLETION_AUDIT.md/json and a pushed source/evidence checkpoint.

Current state: batching baseline has passed complete local manufactured-domain,
MPI1/2/4, independent output/state and actual lifecycle/MMG checks. Execution-policy
changes are UNQUALIFIED working source; controller33248 is building the actual full
driver before focused2case policy MPI1/2/4 checks. Finish this live controller, fix
substantive findings, then qualify actual shared limits/deadline/diagnostic locations,
productive >512 operation, whole driver output/state cases and the broad regression.
The provisional automatic work allowance is an explicit operational cap, not a
feasibility proof or calibrated affordability envelope.

Completion requires direct current evidence for all seven local deliverables. No
curved geometry/CAD, robust production3DBL, full physical unsteady3D, convergence/
forces, performance/scaling or general affordability claim belongs to this closure.
Historical wider-resource/performance gaps are deferred explicitly, not represented
as proven. Larger or unsupported cavities/fields may reject under declared limits;
state the demonstrated local correctness envelope and remaining capability limits.

## Deferred qualification

Performance profiling/optimization, repeated timings, scaling, practical rank/size
and RSS/allocator calibration, weight/balance calibration, true MPI wait/wire and
instrumentation overhead, cluster tooling/runs and CFD/adaptation cost ratios are
outside current Goal1. Preserve their existing code/reports for a later explicitly
activated goal. Do not perform or wait for them to close local Goal1.

Prepared later goals below retain their earlier scope; they are not activated by
this revision. Their historical shared performance contract is in the archived
pre-local plan and can be reconsidered when the user activates them.

## Goal 2 — Adaptable curved geometry and actual Euler lifecycle

Activation objective: Extend the validated native tetrahedral core to
geometry-conforming curved-surface adaptation with preserved features and marker
junctions, and validate actual 3D Euler remesh/transfer/restart cycles. Profile
projection, surface-volume reconstruction and transfer within total adaptation
cost, including partition changes, under the shared contracts.

Reuse: the Goal 1 core, distributed locators/projection, conservative transfer
and driver lifecycle. Extend the native immutable reference from 2D curves to 3D
surfaces and feature curves with stable provenance across adaptation and restart.

Deliver:
1. Surface split/collapse/movement and volume reconstruction as coupled proposals,
   with projection, intersection/orientation and geometry-error validation.
   Preserve feature curves and junctions; reject projection across components.
2. Testable geometry authority: analytic surfaces for manufactured tests and
   retained triangulated geometry for faceted input. Subdivision cannot recover
   missing curvature. Define tolerances before running each campaign; CAD support
   is a later extension unless an existing provider meets this milestone's needs.
3. Actual steady Euler adaptation on stationary geometry with changing surface
   discretization, distributed solution transfer, solver rebuild and adapted-mesh
   restart.
   Validate at least a smooth manufactured field and a resolved shock case.

Correctness gate: sphere/cylinder or equivalent analytic references, a surface
feature/junction case, refinement/coarsening and partition-interface surface
edits. Check geometric error, surface/volume conformity, markers, volume balance,
metric contracts, flow conservation/positivity and restart. Include a practical
3D wing Euler case after small cases pass. Compare CFD accuracy at a declared
common stopping criterion; mesh contracts alone do not prove force accuracy.

Performance gate: separate surface search/projection, failed coupled proposals,
volume repair and transfer costs; measure their growth with surface area/size and
curvature. Compare complete Euler cycle cost, including donor indexing,
repartition/return and reconstruction. Record R and its variability alongside
accuracy and mesh/work counts. Optimize measured projection/query or rejection
hotspots before proceeding to the BL campaign; retain the Goal 1 scaling controls.

## Goal 3 — Robust 3D BL construction and removal from coarse grids

Activation objective: Make native 3D adaptation construct usable anisotropic
near-wall tetrahedra and transitions from coarse Euler-type grids, and remove
unneeded BL refinement for Euler targets, with adaptable surfaces, actual-query
geometric BL and robust MPI edits. Validate viscous solution transfer and CFD
behavior while measuring the complete BL adaptation cost and practical limits.

Reuse: 3D CBoundaryLayerMetric evaluation, stable tensor intersection, sensor
gradation, the curved-surface core and distributed RANS transfer. New work includes
native 3D composition/reference integration, thin-region complexity quadrature and
coordinated wall/near-wall/transition reconstruction.

Deliver:
1. One geometric BL composition used by remeshing and complexity integration at
   actual points. Resolve thin regions inside coarse tetrahedra; independently
   check integration convergence against known planar-layer integrals and
   refined curved references. Do not spread fine wall tensors over entire cells.
2. Coupled wall-face, near-wall point and transition edits preserving prescribed
   first height, finer sensor requests, tangential anisotropy and fade. Diagnose
   incompatible height/metric/geometry requests separately from operator failure.
   Define first-height, alignment and layer coverage diagnostics explicitly;
   exact tetrahedral altitude alone is not a geometric layer-stack certificate.
3. Planar single walls, curved walls, feature/junction regions and interacting
   layers with refinement/coarsening. Quantify growth, skew/alignment and
   transition quality; no prism/hex or structured multilayer guarantee is implied.
4. Sensor and composed sensor-plus-BL gradation audits separately at nodes and
   independent query points. Report residuals without interpreting sensor-only
   success as combined-field success. Any correction belongs before target
   freezing and must preserve finer demands and thin-layer localization.

Correctness gate: independently audited coarse Euler-to-BL and BL-to-Euler on
MPI 1/2/4 with M=N/M<N, then a practical 3D RANS fixture (the matching ONERA M6
SA input is available). Require geometry, first height/coverage, tetrahedral
quality/length, frozen/transported metric, conservation, turbulence admissibility,
restart and atomic rejection checks. Assess viscous convergence and wall shear/
forces at declared accuracy criteria. Retain failure coordinates and rejected
meshes. State aspect/curvature/gap and composed-gradation limits explicitly.

Performance gate: isolate thin-band integration/complexity iterations, wall
queries, near-wall reconstruction and transition retries; measure cost per wall
face and accepted repair, patch-size distributions, memory and rank imbalance.
Compare cold construction from an Euler grid with updates of an existing BL.
Use measured cost to evaluate partition weights and M, including all migration/
return costs. Publish R on accepted RANS cycles and the dominant remaining cost;
do not claim readiness from a finite SPD metric or a quickly rejected mesh.

## Goal 4 — Affordable repeated unsteady 3D adaptation and scaling

Activation objective: Integrate the validated native tetrahedral and BL
capabilities into repeated 3D unsteady adaptation with configurable prediction,
conservative time-history transfer and restart on N CFD ranks, using measured
partition/rank choices. Demonstrate accuracy and adaptation affordability on
declared Euler and RANS workloads with complete profiling and cluster scaling.

Reuse: existing WINDOW_AVERAGE/PREDICT driver logic, selectable snapshot count
and cadence, velocity/acceleration tracking and temporal filtering, plus
distributed conservative transfer. Extend native 2D reference/state assumptions
where necessary. FIXED_POINT, moving physical geometry, adjoints and mixed
volume elements remain separate goals.

Deliver:
1. Repeated actual adaptation on time-dependent Euler and wall-bounded RANS
   cases, first small locally then on the cluster. Transfer current state,
   required BDF histories, turbulence and predictor history consistently.
   Compute Hessians only when scheduled sampling/metric construction needs them,
   not unconditionally at every CFD step. Validate fallback after history loss.
2. Restart from adapted meshes mid-cycle; failure/rejection and worker-communicator
   tests with N=M and M<N. Preserve the original accepted donor until successful
   replacement and preserve the immutable geometry reference across restart.
3. Accuracy controls at matched physical time and stopping criteria: conservation,
   positivity, histories, shock/feature tracking and viscous quantities as
   appropriate. Use existing MMG as a measured comparator where available;
   native correctness and success must not depend on invoking MMG.

Correctness gate: several adaptation events, at least one restart and a forced
rejection followed by resumed CFD, with independent mesh/metric/geometry/height
and time-history audits. Compare temporal quantities with a declared reference;
mesh validity alone is insufficient. Keep the 2D lifecycle regression reusable.

Performance gate: after correctness pilots, at least three sequential repetitions
on selected rank/size points, fixed-work and matched-accuracy comparisons, and
full timing distributions per event. Include scheduled metric sampling, prediction,
gradation/complexity, repartitioning, remeshing, validation, return/rebuild,
transfer and requested output. Report wall time, core-hours, memory, MPI waiting
and adaptation fraction over the full run; evaluate R <= 0.20 as the engineering
target and require R < 1 for the stated affordable envelope. Measure amortization
of donor indexes, caches and partition weights over successive events.

Escalate cluster ranks progressively toward available node capacity only while
total cost and memory justify it. Recommend M and partition strategy from evidence.
Mid-remesh repartitioning or automatic rank selection is conditional on persistent
measured movable hotspots and a demonstrated migration payback; neither is a
prerequisite. If the affordability gate fails, this goal is incomplete: optimize
the dominant measured phase and repeat matched validation rather than declaring
success or weakening accuracy.

## Completion and activation

Activate one goal at a time using its activation objective plus this file's
shared contract and section deliverables. Do not mark a goal complete because a
prototype produces a mesh: its correctness, performance evidence, practical
limits, inspectable results and updated handoff must all be present.
A published implementation must report the target branch and verified commit SHA.
The four sections describe intended work; no native 3D support, performance
improvement or completed validation is claimed by this planning document.
