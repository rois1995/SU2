# Boundary query performance — next connected cycle

User steering2026-11-09; performance analysis only, no production performance edits.
Full293 now completed TWO accepted/transferred/persisted meshes with finite continued
CFD; actual isolated first-adapted disk restart passed MPI6. All11fullworkflow and
10restart actualHOST handles terminal; no ownedheavyjob active. Both independent
matched checkpoints are retained in staged57/m6_inputs/euler_adapted293_event{1,2}_v1.
Frozen driver had already entered event2 when checkpoint1 was verified; its outcome
was preserved before any source edits. Euler milestone is now verified; full goal
still includes this requested performance cycle, dedicated BL/MPI and RANS500k.

After that boundary, prioritize ONE connected performance cycle using build291 and
subsequent first-event evidence. Reuse SAME original saved CFD seed, Mach-onlyL1,
50k target, constraints,5400native/14400outer budgets and current validity-first
construction. Compare complete first-adaptation replay; no simultaneous sensor,
norm, target or construction-policy experiment. Then return to the preserved
adapted CFD checkpoint for adaptation2 if still required. Separate actual isolated
adapted disk restart remains mandatory. Never overwrite the preserved checkpoint.

## Confirmed static observations, not demonstrated speedups

- MeshOptimizer::Commit clears all rejected_smoothing after every commit.
  RememberRejectedSmooth currently stores vertex IDs, so simply retaining it would
  be unsafe. Prove complete relevant cavity dependencies and invalidate on changes
  to neighbors, coordinates, bindings, support, metric state and phase/options.
- SmoothBoundary already builds its patch, exports old parameters, caches old merit,
  prepares a projected context/atlas, screens shape/edges and lazily measures old
  geometry. Retain these facilities and whole-patch merit. Inspect remaining repeated
  work across MoveVertex/PrivateParameters/Finish/GeometryGuidanceExcess; do not
  assume a second projection reproduces the first coordinate or association.
- MoveVertex currently reconstructs PrivateParameters(old), Incidence, SkinMap,
  CheckSurface and Nodes on each trial. Reusable old-side context must preserve
  complete interfaces, exact positivity/topology, associations, features, bodies,
  requested heights, storage/work/time admission and current transport authority.
  Fresh trial positions and their metrics/geometry still require evaluation.
- GeometryGuidanceExcess creates a private parameter context and visits complete
  patch.surface. Existing exact-coordinate/support certificate facilities are the
  starting point. Reuse unchanged contributions only with valid coordinates,
  association/support/tolerance/authority identity. Accepted association is not a
  distance proof. Uncertain conservative bounds fall back to full certification.
- CNativeAffineReference scope misses copy selected prepared triangles, then build
  a TriangleUnion. Existing prepared leaf forest is already available. Investigate
  direct reusable leaf/subtree query execution while preserving exact witness
  support identity, conservative finite-domain bounds, active-reference accounting,
  transient admission and ordering/fallback correctness. Separate identical-query
  hits from nearby positions changing selected-piece keys; no benefit assumed.

Existing counters decide the connected scope: geometry traversal/preparation,
trial/move yield, interior versus boundary work, q/J retirement and introduction,
net whole-field removal/s, geometry-bad counts and total accepted first-event time.
Rank CPU and busy/join counters account for contention; join-call time is not total
MPI wait. Overlapping smooth/guidance/certificate clocks must not be added.
Changed-cell measurement/private-to-public reuse and clearance-index rebuilds are
secondary unless their counters demonstrate the active bottleneck. Unrelated
hypotheses belong in ADAPTATION_BACKLOG.md.

## Qualification and return

State the demonstrated limitation and exact dependency design before implementation.
Use existing ownership/reservation/publication, metric/certificate/cache machinery;
no new scheduler/configuration/CAD requirement. Focused checks plus ONE appropriate
MPI milestone: seams, genuine features, separate bodies, requested heights, stale
neighbors/dependencies, cached/recomputed equivalence, empty ranks and resource
exhaustion. Freeze source/build/runtime/helpers/inputs during each campaign.
Then immediately replay complete first adaptation under unchanged gates/budgets.
Compare work/progress at matching stages and explain trajectory changes. Commit
counts/debt alone are insufficient (291 window:8246 q-bad retired,6859 introduced).
Return to preserved adapted-CFD state after qualification; keep logical event
identity and matching restart provenance explicit across isolated directories.

## Actual completed first-event293 counters

FIRST293 is accepted/transferred/persisted with finite adaptedCFD. Its independently
verified matching mesh/reference/restart copy is staged57/m6_inputs/
euler_adapted293_event1_v1. Actual isolated disk restart now PASSED; event2 also
completed and is independently preserved before any production performance edits.

first_event_performance_counters.json in293attempt seals exact first-event prefixes
and comparisons to291. Native3640.719s versus2913535.671s; all164matching counts/flags/
comparedwork exact. Boundary292056starsearches,8reusehits,395093candidates,17177moves;
2174.545aggregate rank-seconds versus interior65.269. Scope364250misses/builds,
1080.444aggregate rank-seconds; traversal4896.194rank-seconds. These overlapping
clocks are NOTadditive and are not critical-path wall shares. They establish real
boundary/scope work worth targeting, not a proven optimization or safe new cache.

Further source review: RememberRejectedSmooth already limits retention to finite
pure-shape rejection with no validity/edge/geometry failures and no exhausted
atlas work/time or geometry_failure. Preserve that exclusion. Dependency-specific
invalidation may reuse this existing set without a new scheduler, but complete
old/new cavity dependencies, projected-association semantics, metric state and
transient limits must be demonstrated first. No production change yet.

Existing-facility review: PlanarReference::HasAcceptedFaceCertificate verifies
retained face identity, BoundaryPoints, association and face/trace coverage. It
is used to avoid repeating support/coverage validation; it contains no numeric
distance bound and must not substitute for SurfaceDeviation.
GeometryGuidanceExcess still builds PrivateParameters and obtains SurfaceDeviation
for complete patch.surface. Reuse must retain an actually evaluated numeric bound
under exact coordinate/association/coverage/support/tolerance/authority validity,
preserve complete-patch accumulation order and fall back on uncertainty. Existing
point-keyed frozen tensor reuse in boundary linearizations must remain intact.
No new certificate or production reuse mechanism has been implemented.

The eight boundary reuse hits do not count eligible pure-shape insertions or
distinguish commit invalidations from capacity resets. Do not attribute low reuse
solely to Commit or assume dependency-specific retention has a material gain.
Test unchanged versus changed complete dependencies and assess actual replay yield.

Completed driver summary293: FIRST Adapt3819.85s, initial Solve40.6008s and
following adapted Solve152.95s; second Adapt2440.84s/final Solve146.688s.
workflow293_driver_timing_summary.json seals actual rounded rows and full log hash.
In this steady path ComputeMetric is in RunTimeLoop postprocessing (Solve timer);
Adapt covers subsequent remesh/replacement/transfer/mesh+reference output. Use
3819.85s as total accepted first-adaptation comparison alongside native3640.719s,
matching-stage work/progress and CPU/busy/join. Native2913535.671s is a native-only
comparison;291 failed IO and has no completed whole-adaptation timing baseline.
