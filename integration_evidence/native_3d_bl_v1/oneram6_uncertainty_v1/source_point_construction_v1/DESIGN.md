# Source-consistent coordinate construction (WIP, not qualified)

Exact packet82937de1a5 proves the M6 half-edge leaves the original field because
of ordinary nearest-coordinate rounding, despite all source support being present.
The adjacent inward rounding corner has positive local children and covered edges.
This is a coordinate-construction capability gap, not grounds to weaken predicates.

Implement exact componentwise adjacent binary64 bounds of (1-t)*a+t*b with dyadic
integer arithmetic already in the predicate kernel. Enumerate at most8 corners,
nearest first, and retain only an exact original/declared-extension contained point.
No expanding displacement search, epsilon containment or geometry/metric weakening.
A caller must still certify geometry, first height, positivity, quality, full edges,
transfer and exact dependencies. Empty feasible corner set rejects explicitly.

Balanced splitting uses binary64 parameter trials (53 bits versus1e-8 relative
length tolerance), actual represented chord integrals, and original P1 at the actual
contained point. Central physical-edge proposals use the SAME constructor; raw
outside midpoint substitution is removed. Identical rejected central placements are
not repeated. All three balanced-placement callers share this policy. CacheSensor
is extracted from the existing insertion path, preserving eviction/error handling,
and source construction seeds that same literal-point cache. Ordinary Query and
TraceSegment never snap/extrapolate or inherit ideal-point weights.

Scope: new small point-construction header/predicate helper; Field API/cache and
balanced placement; regular/central engine callers; arithmetic and actual-witness
controls. Need independent exact Fraction audit of exported bounds, focused serial/
MPI tests, actual N4M2 workflow and unchanged frozen M6 before capability claims.
No accepted M6, boundary/height or performance result follows from the local packet.
