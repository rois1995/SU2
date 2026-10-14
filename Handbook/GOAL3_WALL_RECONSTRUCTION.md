# Goal 3 wall reconstruction — current capability and limits

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. Full Goal3 ACTIVE and incomplete.

## Connected construction

A retained-topology path splits the requested physical edge and inserts normal
wall points through visibility cavities inside the admitted patch. Completed wall
children remain protected. The whole wall/transition replacement is private until
geometry, first height/normal footprints and full actual-query metric/progress
checks pass. It does not require one background point to see the whole skin.

When partial actual-field repair stagnates, its remaining bad cell identifies an
artificial interface face. Importing that neighbor can remove the face while its
vertices remain fixed; a still-constrained interior vertex then requests its full
star. Complete versioned dependencies and reservations precede reconstruction.
Partial shape improvement is retained only privately for diagnosis and remeasurement,
never published as an admissible replacement. Later fallback geometry cannot
overwrite the first strategy's actual missing dependency.

Imported cavities remain64 cells. Already constructed private inputs can explicitly
use the existing128-cell replacement bound; default accepted-cavity validation and
all local Cone admissions remain64. No ceilings, metric profile or acceptance
thresholds were raised. Sensor-only original-P1 interpolation, actual-query BL,
finer demands, core/fade and source immutability are preserved. Fresh identities
still protect moved-node cache and transfer contracts.

## Actual blockage and replay

The source is the original six-tet scalar P1 `diag(4+12*x)` and square-wall BL
(h0 .1,g1.2,T.3,core4,fade .27→.3). The earlier common-background path rejected the
full event at work4096, longest edge4.3997755. Coordinated movement improved it
but still rejected with453 cells and longest edge2.5437949.

The actual later rejected mesh gives a31-cell edge/apex-star union at the wall/far
seam. Retained reconstruction is geometrically valid immediately; common-background
reconstruction needs11 growth steps to reach geometry, then still fails shape.
The retained proposal stagnates after37 repair trials: one flip, no moves, residual
loss .0167721. Its remaining bad cell touches face1/8399/101671. Importing the face
neighbor gives32 cells without improving shape. Importing the complete interior
vertex101671 star gives35 cells; one flip and two moves then pass local refinement:
qmin .201843,Jmin .055509, changed-edge excess .469426→.044541. These identities
appear only in the saved test fixture, never production dispatch.

A centroid insertion/reconnection lookahead increased work37→165 without improving
the residual. It was removed from production and archived. The first replay also
omitted cyclically reordered physical triangles; that extraction was rejected and
fixed, with assertions requiring both seam faces. An initial fixture initializer
compile failure and interrupted build wrappers are preserved; build freshness was
verified before every executed stage.

Replay folder: `integration_evidence/native_3d_bl_v1/connectivity_replay_v1/visibility`.
The fixture represents the current rejected complete mesh; it does not claim to
be the exact last imported engine cavity. Its record pins the source audit input.

## Complete stronger-field evidence

The complete native construction and removal now PASS on MPI1/2/4, including
independent exact positive volume/coverage/disjointness, original facets/markers,
original-P1/full composed lengths and shape, first height and normal footprints.

- Serial:649 BL cells/169 points→265 sensor-only cells/79 points.
- MPI2:649 BL cells/169 points→268 sensor-only cells/80 points.
- MPI4:649 BL cells/169 points→260 sensor-only cells/78 points.
- Canonical BL coordinates, tetra connectivity and physical marker faces are
  identical across ranks; admissible coarsening need not give a unique minimal grid.
- BL qmin .2150760901,Jmin .0503312313,Lmax1.7987346333; exact height error0,
  minimum footprint0, centroid tilt maximum .8333333.
- Removal qmin .2000634593,Jmin .0788247559,Lmax1.7987346333.
- Nine focused serial integration controls PASS (1596 assertions), including old
  construction/removal, curved geometry, private bounds and atomic refusal.
- Current initialized RANS/SA transfer/restart audits pass MPI1/2; MPI4 was interrupted before completion and remains required.

Folders: `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/sensor`,
`sensor_mpi2`, `sensor_mpi4`; integration controls are in
`integration_evidence/native_3d_bl_v1/connectivity_replay_v1/integration`.
Accepted and rejected grids, raw records, independent inputs and source/build/binary/
thin-object pins remain inspectable. All previous failures are retained.

The serial accepted engine records10.10s construction and .54s removal, with1375
and207 charged work respectively. These are small-case diagnostic samples, not
scaling or complete lifecycle measurements. Metric/edge work and cache updates
remain large components. Do not compare accepted timings against prior rejected
runs as a speedup, or infer adaptation/CFD affordability without CFD/repartition/
transfer/output costs and representative uncontended cases.

## Remaining scope and next connected milestone

Complete the current MPI1/2/4 qualification and actual-policy current+BDF/turbulence/
transport/SU2/CGNS/reference/restart checks before widening the fields and geometry.
Then address thin/curved/interacting walls and feature junctions as complete
capabilities against difficult targets. Preserve original strict gates and diagnose
height/geometry/metric incompatibility separately from operator limitations.

Prescribed altitude and footprint containment do not establish orthogonal or
structured multilayer coverage. Existing gap nodes can lie below the prescribed
wall-apex height; layer coverage/alignment need separate diagnosis. The non-native
576-cell control remains a feasibility witness, not a native or CFD result.
Analytic BL distance/normal, general cases, actual RANS convergence/wall shear/forces,
matching M6 where practical, and full lifecycle performance remain open. No arbitrary
feasibility, prism/hex support or full Goal3 completion is claimed.

See [current overall goal](GOAL3_CURRENT.md) and
[full requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
