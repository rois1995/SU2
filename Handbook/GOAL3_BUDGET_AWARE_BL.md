# Opt-in budget-aware BL — Goal3 extension

Authorized2026-10-18. Status: diagnosis/design; NOT implemented or qualified.
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

## Current execution

The already-built exact-midpoint/progressive-measurement package is being qualified
first so its failures and source/binary evidence remain independently reviewable.
No relaxation has been applied to that M6 construction experiment.
