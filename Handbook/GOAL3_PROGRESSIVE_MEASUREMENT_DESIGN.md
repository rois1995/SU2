# Next connected capability: progressively measured native3D remeshing

Design only; NOT implemented or qualified. Full Goal3 remains ACTIVE/incomplete.
The last complete M6 run initializes38,800 parents in26.79s, then times out at300s
while measuring32,832/536,159 PRIVATE cells; no volume edits/transfer. Geometric edge
preparation reaches172.95s on rank2. Existing selection already retains shared-edge
integrals. Copy avoidance did not help; active-support geometry alone improves a
bounded component sample by~11%, insufficient evidence for another whole-M6 retry.

A sealed64-parent sample supplies actual-query composed tensors at448 selected
children:375 fail final centroid shape thresholds (diagnostic minimumq.01593,
J.0007117). This is intermediate geometry, not infeasibility or a whole-wing survey.
Full seeding nevertheless integrates every edge before allowing a volume operation.
These measurements may certify topology that early reconstruction will discard.
Test that scheduling hypothesis, rather than assuming construction will succeed.

## Smallest complete strategy

Interleave the EXISTING bounded full-cell measurement batches with ordinary complete
cavity transactions. Do not invent approximate edge lengths, another metric cache,
new point placement, a target scale, solver/testcase modes or weaker mesh gates.
Reuse the existing distinction between working cells and fully measured selection
cells. A cell absent from selection is explicitly unmeasured, never admissible.
All stored/reused EdgeMeasure records remain actual, complete composed integrals.

1. Register the complete mesh/source/directory and initialize requested wall heights
   as today. Retain the existing finite initial-ID stream. Fully measure one existing
   import-sized batch per rank before selecting work. Skip stream IDs removed by an
   earlier transaction or already measured during its update.
2. Operate on fully measured candidate seeds. Directory import/reservation still
   admits complete local/remote dependencies, including unmeasured neighbors.
   Existing private old/new-patch field, geometry, positivity, height, shape, length
   and progress validation remains authoritative; no partial proposal is published.
3. Retire cached edges/drop selection records only for removed cells that were
   measured. Fully measure every replacement/update as today. Keep original source
   geometry/tensors, actual-query BL/finer/fade and frozen identity unchanged.
4. When no actionable measured seed remains, measure the next finite batch BEFORE
   recovery, stagnation or infeasibility decisions. Recovery seeds only known fully
   measured invalid cells. New full measurements join existing repair tracking.
5. Accept only when EVERY current working cell has a complete strict measurement
   and all ordinary completion conditions hold. Round/work/time/memory exhaustion
   retains atomic rejection; partial progress cannot trigger transfer/publication.

## Integration audit before editing

Selection::Add/Cached/Drop/Recover, cell/edge users and repair counters; Adapt's
initial measure, selection/completion/stagnation flow and removed/added update;
selection-bound accounting and diagnostics. Selection::Batch/FilterCollapse may
see only measured local stars: they are proposal/pruning hints, so inspect every
caller and preserve complete directory/cavity validation, conflict admission and
changed-vertex reactivation. Do not assume partial local metadata is a complete star.
The actual directory/source must always retain the WHOLE working mesh.

Expose current working/measured/unmeasured counts and bounded measurement batches
using existing diagnostics where possible. No new cap/configuration just for tests.
Clock covers initial construction, all measurement, edits and final completion.

## Required evidence

A focused fixture must perform a real edit whose complete cavity includes initially
unmeasured cells (including remote ownership), then attain an accepted fully checked
mesh; also test removed pending IDs, empty publishers, M<N, exhausted limits and
unchanged accepted state. Reuse meaningful original-P1/actual-BL/geometry/transfer
oracles. Run the known difficult M6 field under unchanged300s only after the whole
scheduling change, separating startup, edits, eventual full certification and
construction acceptance. Compare rejected work/query counts as well as progress;
earlier startup alone is not a full-event speedup or accepted curved mesh.

Batch implementation before MPI qualification. One owned heavy job/nice19/thread1/
build-j1/MPI<=4; inspect host RAM/load and live handles; announce new case folders.
