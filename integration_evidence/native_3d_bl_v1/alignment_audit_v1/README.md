# First-row footprint validation

Parent checkpoint: `531025fceaa8c45b6655a56281ddf6887ce9b55e`,
branch `codex/native-3d-bl`.

`baseline.py` independently evaluates exact rational normal-foot barycentrics on
all six unchanged production BL meshes. Each has one normal foot outside its
wall triangle, caused by merging the two previously normal apices. Those meshes
pass the previously declared height/shape/length gates; they fail this additional
first-row alignment criterion. Historical certificates remain unchanged.

Production now evaluates the normal foot in each actual wall triangle using
long-double tangential areas without creating a rounded projected point. Small
arithmetic uncertainty at an edge is admitted; an unresolved footprint rejects.
The existing first-row initialization, candidate validation and final acceptance
all use the shared check. The original metric and geometry gates remain intact.
No solver/mesh transition modes, target relaxation, wall-tensor spreading,
centroid-angle threshold, fixed boundary, or prohibition on all apex removal was
introduced. Any removal whose resulting first row remains valid can still pass.

`run.py 1`, then `run.py 2` and `run.py 4`, reuse the actual production RANS/SA
fixture and provenance collector from the preceding checkpoint. `audit.py N`
independently checks exact geometry, full composed edges, wall footprints,
height, original P1 sensor transport, conservative flow/BDF states, SU2/CGNS
reload and actual restart. Run only after the current binary is completely built.
Cases are retained in `mpiN`; failed attempts keep their own logs and snapshots.
This is initialized manufactured-state validation, not a CFD accuracy or
performance qualification. General curved/interacting fields and multilayer
coverage remain Goal3 work.

Sensor and composed-field gradation remain separate. The published fixture's
localized geometric BL with its unchanged fade violates the sensor's HGRAD=1.3
transport bound. For a wall normal eigenvalue 100 and outer coarse eigenvalue 1,
the bound between those endpoints requires distance at least
`(10 - 1)/(10*log(1.3))`, about 3.43. Its prescribed BL thickness is only 0.3.
Thus enforcing that particular global bound by changing connectivity is
impossible for this frozen field. Widening its support would change the current
fade/localization policy; coarsening the fine demand would violate its wall
constraint. This endpoint observation is a necessary condition for the probed
transport rule, not a global field certificate or a replacement for an audit.
