# Native3D metric-query donor routing investigation

Source checkpoint bb87e97fe1d89ee69ae8deac8fb447d0996f2d5e. Evidence:
full_star_facet_n8_n1_m1_v1/case.stdout, second coarsening NATIVE3D_DETAIL row.
This is one contended correctness run, not repeated performance evidence.

Observed exclusive coarsening total85.10494s: donor import67.84491s, seed4.18429s,
metric validation5.98695s, update6.48989s; remaining exclusive phases sum to the
parent. There were121private-query fallbacks,2572donor batches,4545418ADT donor
candidates,1698110filtered candidates and2053383reused donor candidates. Point
search0.14378s/interpolation0.00566s are nested query subcosts across measurement,
validation and update, not exclusive phases to add again. Edge trace9.92633s is
another nested geometry subcost. Owner search72.83671s and geometric filter
70.14941s span all imports, including seed/update; they must not be added to the
exclusive67.84491s private donor phase. This attributes the main work to geometric
filtering rather than point-tensor interpolation. These numbers locate work; do not claim a
speedup or cluster scalability from them.

Common/src/adaptation/CNativeDonor3D.cpp ImportMetricCells presently sends a
whole-cell AABB and tetrahedron to potential source owners. The owner tests the
centroid and up to six closed edge segments against each candidate donor. Shared
positive donors already have caller-union reuse; expensive negative candidates
can still receive several geometric tests. Common/src/adaptation/
CNativePredicates3D.cpp IntersectsSegment uses exact signs and rational-root
fallback for unresolved crossings. Do not weaken those predicates.

Candidate smallest change: only for the existing actual-query import mode on
sources exceeding the256donor cap, route the centroid point and each of six edge
segments by its own smaller AABB; exact-test only that segment at the owner.
Keep full-cell ImportCells unchanged for reconstruction with unknown future
queries. Public metric-import footprint remains exactly the same closed union,
and returned donors remain sorted by canonical key. Source-only P1/actual-query
BL/transport/accepted mesh and operation trajectories must remain unchanged.

Required review before implementation:
- Bound/admit expanded request counts and buffers before allocations, using
  existing saturated memory helpers; preserve empty-owner collectives/rejection.
- Keep centroid, closed endpoints, plane/edge/corner touches and unresolved
  crossing donors. Smaller boxes are routing hints, never a geometry certificate.
- Verify current callers only use this import for known centroid/edge queries;
  fresh movement/reconnection proposals must refresh their actual-query support.
- Compare exact returned donor sets against the present cell-box/any-segment
  reference on sources above256cells, MPI1/2/4, empty owners, rotated/thin/touching
  cases and budget failures. Add one shared regression, not a second public API.
- Check original-P1 residuals, full frozen event mesh/operation/state equivalence,
  rejection preservation and complete phase/memory/traffic accounting. Prepare
  cluster repetitions for total overhead, increased request transport and private
  geometric-filter CPU; no speed claim before those measurements.

Alternative small kernel optimization: structurally prove exact zero when all
four finite orientation points share a coordinate, before general integer
fallback. It preserves exact degeneracy, but needs its own adversarial checks and
measured fallback counts. Do not bundle both changes before attribution. Neither
candidate has been implemented in production at this checkpoint. Keep source
pins stable while the live longer-budget diagnostic runs.
