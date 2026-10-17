# Restrict geometry-only feature competition to active BL support

Full Goal3 ACTIVE/incomplete. Previous interval/splice replay found no useful gain
and its production/test changes were withdrawn; preserve that evidence separately.
The existing finite-feature envelope admits all wall features in a broad chord box,
including competition where the wall metric has already faded to zero. Bound the
geometry problem to the existing thickness plus existing on-wall tolerance and
coordinate-roundoff padding. Original field, target composition, fade, finer sensor
demands and strict final mesh/transfer contracts remain unchanged.

A constant-radius feature represents inactive background. Before insertion, clip
each finite face/edge/vertex residual to3 affine coordinate slabs. Every residual
with Euclidean distance<=radius satisfies all3 slabs in exact arithmetic; padding
covers ordinary floating construction error. Existing distance competition still
selects exact numerical nearest features within the radius. Outside it, the helper
returns min(distance,radius), never a replacement for an actual metric query.
Default unbounded users retain existing behavior. All feature/piece/comparison/cut
ceilings remain; the constant feature consumes a feature slot. No new config/cache,
quadrature/tolerance/field policy or input/flow-class dispatch. This long-double
geometry is numerical, not a proven outward-rounded certificate.

Affected callers: shared ChordEnvelope, faceted3D CBoundaryLayerMetric edge cuts,
strict edge/correction integration and their fixtures.2D and analytic paths retain
existing kernels. Validate active crossings/distance against published envelope,
including disconnected narrow supports, then full target edge integration and
saved difficult-cell references before one actual changed M6 attempt if warranted.
Performance claims must include preparation/memory and distinguish component time
from accepted full remeshing. One owned job/nice19/thread1/build-j1/MPI<=4.
