# Native 3D development goals

Prepared 2026-10-10 at the user's request. Goal 1 was explicitly activated
on 2026-10-10 after publishing the completed 2D reuse checkpoint. Goals 2–4
remain prepared for later activation; the old partition goal was cleared.
Active implementation branch: codex/native-3d-core, based on
038fdfa5786fb3f6af392d97257bfbb2bb29f036.

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Planning branch: codex/native-unsteady-performance, publication checkpoint
d44cc986c2880ac3bf7e975342efc257f55cd8a9.
Create a separate implementation branch from the validated integration checkpoint
when Goal 1 is activated; retain the performance branch and leave AdapNoExt and
other sessions' work untouched. Record the actual branch and source SHA.

## Shared execution, correctness and performance contract

The user's latest policy is local-first: small correctness cases and short pilots
locally, then larger correctness, repeated timing and scaling campaigns on the
cluster. Before every local build/run inspect load, memory and competing jobs;
defer if resources are busy. One owned heavy job at a time, initially MPI <= 4,
build -j2, numerical-library threads = 1. Never terminate unrelated processes.
Cluster runs use existing SGE/qsub conventions; do not require Git on compute
nodes or exclusive-node allocation. Announce the full working case folder before
each new case, and print working and retained result folders in runners.

Use an opt-in native tetrahedral path alongside existing native 2D and MMG.
Reuse existing mesh/transfer/output and transaction infrastructure where practical,
without a preliminary rewrite of all 2D classes. Preserve rejection and accepted
state atomically, original sensor-only P1 donor interpolation, geometric BL
evaluation at actual queries including remote/private candidates, finer sensor
demands, current fade and ADAP_HESSIAN_NOISE=0. Do not weaken frozen targets or
guards to obtain completion. Different partitions can produce different meshes:
audit each independently; byte identity across partitions is not a requirement.

The frozen 2D pre-development regression checkpoint is complete: cluster job
582358 passes all 16 remeshes/eight strict pairs. Review and evidence:
integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582358/REVIEW.md.
Keep the tested source/binary pins and failed historical evidence. New production
changes still require the relevant 2D regression and actual lifecycle checks;
this frozen matrix is not a new unsteady-CFD or composed-gradation certificate.

For every goal, performance is a deliverable, not a later cleanup:

- Measure complete adaptation wall time, starting with metric preparation and
  ending with an accepted geometry/solution/history ready to resume CFD. Separately
  record metric/Hessian/prediction/gradation/complexity, import and donor indexing,
  work estimation, partition graph/ParMETIS/migration, candidate selection and
  dependency exchange, private reconstruction, validation/commit, return to N CFD
  ranks, geometry/solver reconstruction and solution/history transfer. Record
  production mesh/state output separately and include it in operational overhead
  when requested. External audits and collection are separately timed.
- Make phase accounting explicit: exclusive phases close against the parent
  wall time; nested detail and MPI wait are labelled, not added twice. Never sum
  per-rank maxima and call that elapsed time. Distinguish private CPU from wait.
- Break actual metric-query work into donor search, sensor interpolation and
  geometric BL composition with bounded aggregate timers/counters. Measure cache
  requests/hits/misses, reconstruction attempts/accepted edits/rejections, patch
  sizes, communication bytes/collectives and per-rank max/mean work/time. Sample
  detailed locations rather than logging every query. Measure instrumentation
  overhead with a disabled-detail control before using it for timing claims.
- Retain peak RSS and admitted working-buffer bounds, including migration and
  private scratch. Report initial/final mesh sizes, anisotropy, target residuals
  and accepted operation counts so different workloads remain distinguishable.
- Use the same source/binary/config/input for fixed-work comparisons; also report
  matched numerical-target comparisons when operators change the resulting mesh.
  At least three sequential measured repetitions support timing claims; report
  spread and contention. Single pilots provide diagnostics only. Report useful
  accepted work per second as well as total seconds; fast rejection is not success.
- Exercise N=M and M<N early. Increase cluster size/ranks progressively rather than
  going directly to 192 cores. Include partition/migration and return/transfer in
  every rank-choice comparison. Stop increasing ranks when total cost worsens.
  Address the measured dominant cost before enlarging the next expensive campaign.
- Preserve compact provenance, configs, logs, phase/rank summaries, independent
  audits, representative initial/accepted meshes and corresponding donor/accepted
  states needed for replay. Keep rejected meshes with reason/location when useful.
  Deduplicate inputs with verified-copy fallback; retain failures. Avoid full flow
  output series and per-candidate dumps by default. Close exported dependencies.
  Update HANDOFF_Codex.md and findings with scope, limits and evidence.

On real CFD workloads define R = total adaptation overhead / CFD computation
between adaptation events, measured on the same N-rank allocation. Include
scheduled metric sampling/prediction between events in the numerator, charge
shared work once, and record diagnostic/output costs separately. The engineering
target is R <= 0.20; the minimum affordability gate is R < 1 on the declared
demonstrated envelope. These are proposed acceptance targets, not existing results
or a reason to weaken accuracy or lengthen the cadence beyond physical needs.
Early frozen-mesh stages establish costs and limits without claiming CFD
affordability. No universal wall-second budget is invented before measuring cases.

## Goal 1 — MPI-aware tetrahedral core and adaptable planar boundaries

Activation objective: Implement and validate an opt-in native MPI tetrahedral
adaptation core on simple faceted domains, including surface refinement/coarsening,
bounded atomic distributed edits and complete cost profiling. Preserve the 2D
checkpoint and all shared contracts; establish the practical correctness,
memory and performance envelope before progressing to curved geometry.

Reuse: CSimplexMesh, CRemesher, passive communication, distributed search,
memory admission, mesh reconstruction/output and the existing transaction design.
Extend triangle-specific donor/import/partition and ownership records for
tetrahedra, triangular faces and edge stars. Keep full edge/vertex dependencies:
face adjacency alone cannot establish a safe cavity edit.

Deliver:
1. Stable 3D orientation and metric arithmetic, canonical IDs and incidence,
   sensor-only tetrahedral P1 queries and bounded caches. Define and freeze
   meaningful tetrahedral quality/sliver acceptance tests before validation;
   do not transplant the 2D q >= 0.18 threshold.
2. Split, collapse, movement and bounded reconnection sufficient for repeated
   refinement/coarsening, including the required face/edge reconnections. Admit
   complete dependencies, validate privately and commit atomically. Diagnose
   non-reconstructible or over-budget cavities rather than looping indefinitely.
3. Adapt boundary triangles on immutable planar facets while preserving marker
   junctions and feature edges geometrically. Rebuild neighboring volume cells
   together; surface connectivity is allowed to change from the first milestone.
4. Distributed conflict/stale-dependency/failure tests, weighted partitioning,
   explicit worker communicator M <= N, and accepted mesh return/reconstruction.
   Recompute work weights for tetrahedra rather than copying 2D coefficients.

Correctness gate: manufactured constant and spatially varying rotated anisotropic
metrics on boxes/wedges, sliver adversaries, partition-interface cavities and
repeat refine/coarsen cycles on MPI 1/2/4. Require positive volumes, conforming
incidence/manifold surface, no overlap/gaps, domain/marker preservation, frozen
field residuals and declared length/quality bounds. Independently validate 3D
mesh output in SU2 and CGNS, plus rejection without accepted-state mutation.
Choose a small matrix of aspect ratios and sizes before running it, then expand
to locate the first failure. Successful single edge splits do not complete this goal.

Performance gate: closed phase/rank accounting, measured query subcosts and bounded
memory; sequential size and M=N/M<N pilots followed by cluster repetitions.
Publish total-cost and accepted-work rates, dominant phase, imbalance and first
demonstrated size/rank limit. Retest that the existing 2D path retains its numerical
contracts and investigate timing regressions against measured variability.
Do not advance with an unexplained unbounded scan, memory growth or retry loop.

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
