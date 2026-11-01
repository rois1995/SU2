# Conservative winding screening in the shared geometry path

The missing capability remains strictly accepted M6 quality closure, followed by
residual geometry/size closure, shape reclosure, conservative admissible transfer,
continued CFD and an isolated adapted disk restart; then BL/MPI and RANS500k.
Build198retry1 naturally time-budget rejected. Latest complete window has
2985q/1636J/3098geometry bad; terminal EXIT is incomplete and counts unknown.
All10owned handles absent/all1401pins unchanged before source changes.

Measured limitation: traversal is93.855%of upper work. Existing saved-query trace
attributes141128of198551 traversal units to finite coverage; this older12query
population is not a current late-star sample or a CPU-cost estimate. Source review
finds unconditional side predicates in planar point/centroid winding even when an
edge cannot contribute. Mapped component containment already has a conservative
border prefilter. Reuse that principle, without a new cache/envelope/scheduler.

Connected change: CNativeBoundary3D.cpp PlanarReference::Contains computes a side
only for a height-straddling edge, retaining endpoint/on-edge checks first.
ContainsFace skips edges strictly outside the triangle height interval: they
neither cross the triangle nor straddle its exact centroid. Bounds on vertex
heights can certify centroid-side signs; uncertain cases retain the existing exact
centroid predicate. Compute edge-side only on a centroid-height straddle, but
retain full border-interior intersection checks for every potentially intersecting
edge, including holes entirely below/above the centroid. CNativeAffineReference.cpp
mapped component face winding likewise defers the side predicate after its existing
exact centroid-side tests. No rounded centroid or sampling proof.

Callers: finite projected footprint certification, planar/mapped geometry admission,
coarsening, splitting, quality and boundary smoothing share these checks. APIs,
configuration, construction operators, metrics/heights, bindings/support identities,
cache/ref lifetimes, transient admission and all final gates remain unchanged.
Kernel work accounting charges actual predicate calls; sticky exhaustion remains.
No change to fair/worst-first queues, private slots, publication/dependencies,
rejected history or complete-census migration. CAD remains optional.

Validation: extend existing finite rotated multi-hole/notch/separate-body checks
with a hole below the query centroid and a valid triangle above it. Existing exact
centroid/tangency/resource cases remain. Focused serial saved12query replay compares
all output bounds and cached/recomputed results to198, recording traversal savings
rather than assuming them. Then affected serial checks and one MPI6 milestone
cover curved seams/features/bodies/heights/migration/empty ranks/resources. Reuse
passing unaffected evidence with current provenance. No extra profiler/GDB or
old-code M6 baseline campaign. Immediately fulltwoeventM6 from the same saved CFD
seed at50k/native5400/unchanged gates after qualification. Net wholefield q/Jbad/s
and geometry counts determine utility, not reduced predicate counts or fixtures.

This is a selected connected candidate, not an established speedup or accepted mesh.
