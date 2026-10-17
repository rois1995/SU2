# Exact declared midpoint queries, never a point-query fallback

Full Goal3 ACTIVE/incomplete. Progressive full measurement passes serial/MPI2/MPI4
and actual N4/M2 RANS/BDF/SU2/CGNS/restart; stronger finer4096-work failure remains.
Full M6 now clears first-row initialization and first16-cell seed batches in.13s,
then rejects its first volume proposal at51.5s. No volume edit or M6 acceptance.

Diagnostic-only repeat and independent exact rational audit prove the failing
coordinate is the rounded midpoint of ORIGINAL vertices6758/6804. All3 original
containing tetrahedra were ALREADY ADMITTED. The ideal midpoint has exact weights
(1/2,1/2,0,0); rounding offsets(-5.55e-17,-6.94e-18,0) leave the physical original
volume. This is not missing MPI support; do not add global gathers, donor inflation,
constant extrapolation, tolerance relaxation or a new reconstruction fallback.
Witnesses/capsules/failed cases are in progressive_measurement_v1 and both
construction_progressive[_diagnostic]_v1 folders.

Add BarycentricMidpoint using the existing bounded exact-integer determinant scheme:
evaluate endpoint determinant sums at1/2 BEFORE rounding, preserve closed/isolated
physical contacts and physical-over-extension priority. Original vertex pairs use
exact half/unit weights without integer reconstruction. QueryMidpoint uses an
outward one-ULP search box around the represented coordinate, then exact containment.
It interpolates ORIGINAL sensor-only P1 at the specified ideal midpoint, composes BL
at the SAME represented coordinate as before, and NEVER populates ordinary Query's
point cache. Query(Point), final cell/edge/geometry/height/transfer checks stay strict.

Use this shared operation at explicit midpoint guide/score queries in engine and
coordinated reconstruction. Arbitrary coordinates and centroids retain Query;
full edge integrals remain unchanged. No endpoint-tensor averaging across source
pieces, no retargeting/finer-demand/fade changes, no new cache/configuration.
Validate the exact M6 witness, no cache poisoning, isolated source-priority contact,
original pieces/unsupported queries and repeat composition. Then focused scalar
checks and one changed full M6 attempt before wider MPI/lifecycle qualification.
Count work/resource costs; exact arithmetic is not free. Source/build/binary/helper
provenance, rejected experiments and every full-goal gate must remain intact.
