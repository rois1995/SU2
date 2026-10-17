# Local updates of the existing finite-feature chord envelope

Base2ab7efce4169697e4d1da163a79e5c51047cf58d; full Goal3 ACTIVE/incomplete.
Previous goal turn = PROGRESS: exact ranking checkpoint published; whole M6 now
initializes all38,800 parents but times out during mandatory full seeding before
volume operations. Seed geometry reaches172.95s on one worker. Existing selection
already retains shared edges; do not add an overlapping cache or weaken seeding.

Missing capability: complete geometry-resolved seed queries at useful cost. The
current nearest-feature lower envelope scans/copies every old piece on each finite
feature insertion, even for restricted domains and for candidates that never win.
Use binary location of the overlapping domain, preserve the SAME comparison/root/
emission arithmetic there, do not mutate a losing envelope, splice only changed
pieces and merge its two boundaries. Preserve total piece, feature and comparison
work limits. No approximate distance cull, quadrature change, target change, new
geometry authority, normalization/tolerance, cache or configuration. Original P1,
actual geometric BL/finer/fade and final mesh contracts stay unchanged.

Scope: shared Common chord helper; actual CBoundaryLayerMetric edge cut callers
(strict3D and correction integration); existing envelope/BL integration tests and
actual frozen M6 geometry. 2D/analytic paths do not use this faceted3D helper.
The existing long-double geometry is numerical, not an outward-rounded proof.

Before broader native runs: compile one standalone legacy/candidate driver, compare
exact distance values, cuts, feature/comparison/piece counts and work exhaustion on
actual wall geometry and bounded original M6 edges plus nontrivial analytic controls.
Record preparation and kernel cost separately. Promote only measured useful work
reduction; rejected M6 progress cannot establish accepted adaptation speedup.
One owned job/nice19/build-j1/thread1/MPI<=4; inspect load/RAM, no CFD/cluster work.
